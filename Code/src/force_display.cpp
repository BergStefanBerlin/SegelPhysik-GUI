#include "force_display.hpp"

#include <QJsonArray>
#include <QJsonValue>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace forcedisplay {

namespace {

constexpr double pi = 3.14159265358979323846;

std::optional<Vec3> readOptionalVector(const QJsonValue& value, const QString& name)
{
    if (value.isNull() || value.isUndefined()) {
        return std::nullopt;
    }
    if (!value.isArray() || value.toArray().size() != 3) {
        throw std::runtime_error(
            QStringLiteral("Core-Zustand enthält keinen gültigen 3D-Vektor: %1")
                .arg(name)
                .toStdString());
    }
    const auto array = value.toArray();
    Vec3 vector{};
    for (qsizetype i = 0; i < 3; ++i) {
        if (!array.at(i).isDouble() || !std::isfinite(array.at(i).toDouble())) {
            throw std::runtime_error(
                QStringLiteral("Core-Zustand enthält nicht endliche Vektordaten: %1")
                    .arg(name)
                    .toStdString());
        }
        vector[static_cast<std::size_t>(i)] = array.at(i).toDouble();
    }
    return vector;
}

Vec3 readVector(const QJsonValue& value, const QString& name)
{
    const auto vector = readOptionalVector(value, name);
    if (!vector) {
        throw std::runtime_error(
            QStringLiteral("Core-Zustand enthält keinen 3D-Vektor: %1")
                .arg(name)
                .toStdString());
    }
    return *vector;
}

QStringList readStrings(const QJsonValue& value)
{
    QStringList result;
    for (const auto& item : value.toArray()) {
        result << item.toString();
    }
    return result;
}

GroupSum readGroupSum(const QJsonObject& object)
{
    GroupSum sum;
    sum.group = object.value(QStringLiteral("gruppe")).toString();
    sum.force = readOptionalVector(
        object.value(QStringLiteral("kraft_modell_n")), QStringLiteral("kraft_modell_n"));
    sum.moment = readOptionalVector(
        object.value(QStringLiteral("moment_s_nm")), QStringLiteral("moment_s_nm"));
    sum.sources = readStrings(object.value(QStringLiteral("quellen")));
    return sum;
}

double segmentDistance(const QPointF& a, const QPointF& b, const QPointF& p)
{
    const double dx = b.x() - a.x();
    const double dy = b.y() - a.y();
    const double lengthSquared = dx * dx + dy * dy;
    double t = 0.0;
    if (lengthSquared > 0.0) {
        t = std::clamp(((p.x() - a.x()) * dx + (p.y() - a.y()) * dy) / lengthSquared,
            0.0, 1.0);
    }
    const double ex = a.x() + t * dx - p.x();
    const double ey = a.y() + t * dy - p.y();
    return std::sqrt(ex * ex + ey * ey);
}

} // namespace

double dot(const Vec3& a, const Vec3& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Vec3 cross(const Vec3& a, const Vec3& b)
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]};
}

double norm(const Vec3& a)
{
    return std::sqrt(dot(a, a));
}

Vec3 add(const Vec3& a, const Vec3& b)
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Vec3 sub(const Vec3& a, const Vec3& b)
{
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Vec3 scale(const Vec3& a, double factor)
{
    return {a[0] * factor, a[1] * factor, a[2] * factor};
}

// --- Skalierung ------------------------------------------------------------

bool isValidReference(double reference)
{
    return std::isfinite(reference) && reference > 0.0;
}

ScaledMagnitude scaleMagnitude(double magnitude, double reference, ScaleMode mode)
{
    if (!isValidReference(reference)) {
        throw std::invalid_argument("Skalenreferenz muss endlich und positiv sein.");
    }
    if (!std::isfinite(magnitude) || magnitude < 0.0) {
        throw std::invalid_argument("Betrag muss endlich und nicht negativ sein.");
    }
    ScaledMagnitude result;
    if (mode == ScaleMode::Linear) {
        result.u = magnitude / reference;
    } else {
        const double m0 = reference / 100.0;
        result.u = std::log1p(magnitude / m0) / std::log1p(reference / m0);
    }
    result.overScale = magnitude > reference;
    result.u = std::clamp(result.u, 0.0, 1.0);
    result.smallValue = magnitude > 0.0 && result.u < smallValueFraction;
    return result;
}

std::optional<double> refitReference(double currentMaximum)
{
    if (!std::isfinite(currentMaximum) || currentMaximum <= 0.0) {
        return std::nullopt;
    }
    return 1.1 * currentMaximum;
}

double pixelLength(const ScaledMagnitude& scaled, double maximumPx, double minimumPx)
{
    if (scaled.u <= 0.0) {
        return 0.0;
    }
    return std::max(scaled.u * maximumPx, minimumPx);
}

// --- Pose ------------------------------------------------------------------

Pose readPose(const QJsonObject& state)
{
    Pose pose;
    pose.position = readVector(state.value(QStringLiteral("position_m")),
        QStringLiteral("position_m"));
    const auto value = state.value(QStringLiteral("orientierung_xyzw"));
    if (!value.isArray() || value.toArray().size() != 4) {
        throw std::runtime_error("Core-Zustand enthält kein Quaternion xyzw.");
    }
    const auto array = value.toArray();
    double normSquared = 0.0;
    for (qsizetype i = 0; i < 4; ++i) {
        if (!array.at(i).isDouble() || !std::isfinite(array.at(i).toDouble())) {
            throw std::runtime_error(
                "Core-Zustand enthält kein endliches Quaternion xyzw.");
        }
        pose.quaternionXyzw[static_cast<std::size_t>(i)] = array.at(i).toDouble();
        normSquared += array.at(i).toDouble() * array.at(i).toDouble();
    }
    if (normSquared <= 0.0) {
        throw std::runtime_error("Core-Zustand enthält ein Null-Quaternion.");
    }
    const double inverse = 1.0 / std::sqrt(normSquared);
    for (auto& component : pose.quaternionXyzw) {
        component *= inverse;
    }
    return pose;
}

Vec3 toWorldVector(const Pose& pose, const Vec3& v)
{
    const Vec3 q{pose.quaternionXyzw[0], pose.quaternionXyzw[1], pose.quaternionXyzw[2]};
    const double w = pose.quaternionXyzw[3];
    const Vec3 t = scale(cross(q, v), 2.0);
    return add(add(v, scale(t, w)), cross(q, t));
}

Vec3 toWorldPoint(const Pose& pose, const Vec3& p)
{
    return add(toWorldVector(pose, p), pose.position);
}

// --- Beiträge --------------------------------------------------------------

ForceState parseForceState(const QJsonObject& object)
{
    ForceState state;
    state.contract = object.value(QStringLiteral("vertragsversion")).toString();
    state.modelId = object.value(QStringLiteral("modell_id")).toString();
    state.time = object.value(QStringLiteral("zeit_s")).toDouble();
    state.step = static_cast<qint64>(object.value(QStringLiteral("schritt")).toDouble());
    state.status = object.value(QStringLiteral("status")).toString();
    state.diagnostic = object.value(QStringLiteral("diagnose")).toString();
    state.reference = readOptionalVector(
        object.value(QStringLiteral("moment_bezug_modell_m")),
        QStringLiteral("moment_bezug_modell_m"));
    const bool v2 = state.contract == QStringLiteral("2");
    for (const auto& id : object.value(QStringLiteral("geflutete_elemente")).toArray()) {
        state.flooded.push_back(id.toString());
    }
    for (const auto& value : object.value(QStringLiteral("vektoren")).toArray()) {
        if (!value.isObject()) {
            throw std::runtime_error("Core-Zustand enthält einen ungültigen Vektoreintrag.");
        }
        const auto item = value.toObject();
        Contribution c;
        c.id = item.value(QStringLiteral("id")).toString();
        c.elementId = item.value(QStringLiteral("element_id")).toString();
        const auto art = item.value(QStringLiteral("art")).toString();
        if (art == QStringLiteral("kraft")) {
            c.kind = Kind::Force;
        } else if (art == QStringLiteral("moment")) {
            c.kind = Kind::Moment;
        } else {
            continue;
        }
        c.unit = item.value(QStringLiteral("einheit")).toString();
        if (c.unit.isEmpty() || c.unit.contains(QChar(0xFFFD))) {
            c.unit = c.kind == Kind::Force ? QStringLiteral("N") : QStringLiteral("N·m");
        }
        c.frame = item.value(QStringLiteral("frame")).toString();
        c.source = item.value(QStringLiteral("quelle")).toString();
        c.group = item.value(QStringLiteral("gruppe")).toString();
        c.role = item.value(QStringLiteral("rolle")).toString();
        c.pointKind = item.value(QStringLiteral("punktart")).toString();
        c.state = v2 ? item.value(QStringLiteral("zustand")).toString()
                     : QStringLiteral("gueltig");
        if (c.state.isEmpty()) {
            c.state = QStringLiteral("gueltig");
        }
        c.errorId = item.value(QStringLiteral("fehler_id")).toString();
        c.point = readOptionalVector(item.value(QStringLiteral("punkt_m")), c.id);
        c.vector = readOptionalVector(item.value(QStringLiteral("vektor")), c.id);
        c.momentS = readOptionalVector(item.value(QStringLiteral("moment_s_nm")), c.id);
        c.category = categorize(c);
        state.contributions.push_back(c);
    }

    if (v2) {
        state.hasBalance = true;
        state.balanceStatus = object.value(QStringLiteral("bilanzstatus")).toString();
        state.expected = readStrings(object.value(QStringLiteral("erwartete_beitraege")));
        state.missing = readStrings(object.value(QStringLiteral("fehlende_beitraege")));
        const auto sums = object.value(QStringLiteral("summen")).toObject();
        if (sums.value(QStringLiteral("gesamt")).isObject()) {
            state.total = readGroupSum(sums.value(QStringLiteral("gesamt")).toObject());
        }
        for (const auto& group : sums.value(QStringLiteral("gruppen")).toArray()) {
            state.groups.push_back(readGroupSum(group.toObject()));
        }
        const auto equilibrium = object.value(QStringLiteral("gleichgewicht"));
        state.equilibriumPresent = equilibrium.isObject();
        state.equilibrium = equilibrium.toObject();
        if (state.total && state.total->force) {
            Contribution sum;
            sum.id = QStringLiteral("summe:kraft");
            sum.kind = Kind::Force;
            sum.unit = QStringLiteral("N");
            sum.frame = sums.value(QStringLiteral("gesamt")).toObject()
                            .value(QStringLiteral("frame")).toString();
            sum.source = QStringLiteral("summe");
            sum.group = QStringLiteral("gesamt");
            sum.role = QStringLiteral("gesamtsumme");
            sum.pointKind = QStringLiteral("bezugspunkt");
            sum.state = QStringLiteral("gueltig");
            sum.point = state.reference;
            sum.vector = state.total->force;
            sum.momentS = state.total->moment;
            sum.fromSums = true;
            sum.category = Category::ForceSum;
            state.contributions.push_back(sum);
        }
    }
    return state;
}

QVector<Contribution> derivedContributions(const ForceState& state)
{
    QVector<Contribution> result;
    for (const Category category : {Category::Weight, Category::Buoyancy}) {
        const bool weight = category == Category::Weight;
        Vec3 force{0.0, 0.0, 0.0};
        Vec3 weightedPoint{0.0, 0.0, 0.0};
        Vec3 moment{0.0, 0.0, 0.0};
        double magnitudes = 0.0;
        bool anyForce = false;
        bool anyMoment = false;
        QString frame;
        for (const auto& c : state.contributions) {
            if (c.category != category || c.kind != Kind::Force
                || c.state != QStringLiteral("gueltig") || !c.vector) {
                continue;
            }
            if (frame.isEmpty()) {
                frame = c.frame;
            } else if (c.frame != frame) {
                continue;
            }
            anyForce = true;
            force = add(force, *c.vector);
            const double magnitude = norm(*c.vector);
            if (c.point && magnitude > 0.0) {
                weightedPoint = add(weightedPoint, scale(*c.point, magnitude));
                magnitudes += magnitude;
            }
            if (c.momentS) {
                anyMoment = true;
                moment = add(moment, *c.momentS);
                if (state.reference) {
                    Contribution part;
                    part.id = c.id + QStringLiteral(":moment");
                    part.elementId = c.elementId;
                    part.kind = Kind::Moment;
                    part.unit = QStringLiteral("N·m");
                    part.frame = c.frame;
                    part.source = c.source;
                    part.group = c.group;
                    part.role = QStringLiteral("einzelbeitrag");
                    part.pointKind = QStringLiteral("bezugspunkt");
                    part.state = QStringLiteral("gueltig");
                    part.point = state.reference;
                    part.vector = c.momentS;
                    part.category = category;
                    result.push_back(part);
                }
            }
        }
        if (!anyForce) {
            continue;
        }
        const QString base = weight ? QStringLiteral("anzeige:gewicht")
                                    : QStringLiteral("anzeige:auftrieb");
        Contribution total;
        total.id = base;
        total.kind = Kind::Force;
        total.unit = QStringLiteral("N");
        total.frame = frame;
        total.source = weight ? QStringLiteral("schwerkraft") : QStringLiteral("hydrostatik");
        total.group = QStringLiteral("gewicht_auftrieb");
        total.role = QStringLiteral("gruppensumme");
        total.pointKind = QStringLiteral("angriffspunkt");
        total.state = QStringLiteral("gueltig");
        // Gewichte bzw. Auftriebe sind parallel: der betragsgewichtete Mittelpunkt
        // ist der Angriffspunkt der Resultierenden (Schwer- bzw. Auftriebspunkt).
        if (magnitudes > 0.0) {
            total.point = scale(weightedPoint, 1.0 / magnitudes);
        }
        total.vector = force;
        if (anyMoment) {
            total.momentS = moment;
        }
        total.category = category;
        total.total = true;
        result.push_back(total);
        if (anyMoment && state.reference) {
            Contribution totalMoment = total;
            totalMoment.id = base + QStringLiteral(":moment");
            totalMoment.kind = Kind::Moment;
            totalMoment.unit = QStringLiteral("N·m");
            totalMoment.pointKind = QStringLiteral("bezugspunkt");
            totalMoment.point = state.reference;
            totalMoment.vector = moment;
            totalMoment.momentS = moment;
            result.push_back(totalMoment);
        }
    }
    return result;
}

Category categorize(const Contribution& c)
{
    if (c.fromSums) {
        return Category::ForceSum;
    }
    const bool isSum = c.role == QStringLiteral("gesamtsumme")
        || c.role == QStringLiteral("gruppensumme")
        || c.source == QStringLiteral("summe")
        || c.id.startsWith(QStringLiteral("gesamt:"));
    if (isSum) {
        return c.kind == Kind::Moment ? Category::MomentSum : Category::ForceSum;
    }
    if (!c.source.isEmpty()) {
        if (c.source == QStringLiteral("schwerkraft")) {
            return Category::Weight;
        }
        if (c.source == QStringLiteral("hydrostatik")) {
            return Category::Buoyancy;
        }
        if (c.source == QStringLiteral("nicht_zugeordnet")) {
            return Category::Unassigned;
        }
        if (c.source.startsWith(QStringLiteral("test"))
            || c.source == QStringLiteral("fremdkraft")) {
            return Category::Test;
        }
        return Category::Other;
    }
    if (c.id.endsWith(QStringLiteral(":gewicht"))) {
        return Category::Weight;
    }
    if (c.id.endsWith(QStringLiteral(":auftrieb"))) {
        return Category::Buoyancy;
    }
    return Category::Unassigned;
}

QString colorFor(Category category, const QString& group)
{
    switch (category) {
    case Category::Weight:
    case Category::Buoyancy:
        return QStringLiteral("#2166ac");
    case Category::ForceSum:
    case Category::MomentSum:
        return QStringLiteral("#dddddd");
    case Category::Test:
        return QStringLiteral("#d7301f");
    case Category::Unassigned:
        return QStringLiteral("#d01c8b");
    case Category::Other:
        break;
    }
    static const char* palette[] = {"#1b9e77", "#d95f02", "#e6ab02", "#66a61e", "#a6761d"};
    const auto index = static_cast<std::size_t>(qHash(group) % 5U);
    return QString::fromLatin1(palette[index]);
}

bool isDashed(Category category)
{
    return category == Category::Buoyancy;
}

QString colorForContribution(const Contribution& c)
{
    return c.total ? QStringLiteral("#67a9cf") : colorFor(c.category, c.group);
}

QString categoryName(Category category)
{
    switch (category) {
    case Category::Weight:
        return QStringLiteral("Gewicht");
    case Category::Buoyancy:
        return QStringLiteral("Auftrieb");
    case Category::ForceSum:
        return QStringLiteral("ΣF");
    case Category::MomentSum:
        return QStringLiteral("ΣM");
    case Category::Test:
        return QStringLiteral("Test-/Fremdkraft");
    case Category::Unassigned:
        return QStringLiteral("nicht zugeordnet");
    case Category::Other:
        return QStringLiteral("weitere Quelle");
    }
    return {};
}

QString contributionLabel(const Contribution& c)
{
    QString name;
    switch (c.category) {
    case Category::Weight:
    case Category::Buoyancy:
        name = categoryName(c.category);
        if (c.kind == Kind::Moment) {
            name = QStringLiteral("Moment ") + name;
        }
        if (c.total) {
            name += QStringLiteral(" gesamt");
        }
        break;
    case Category::ForceSum:
    case Category::MomentSum:
        name = categoryName(c.category);
        break;
    default:
        name = c.id.isEmpty() ? categoryName(c.category) : c.id;
        break;
    }
    if (!c.elementId.isEmpty()) {
        return QStringLiteral("%1 · %2").arg(c.elementId, name);
    }
    return name;
}

QString groupLabel(const Contribution& c)
{
    if (c.category == Category::ForceSum || c.category == Category::MomentSum) {
        return QStringLiteral("Summen");
    }
    if (c.group == QStringLiteral("gewicht_auftrieb")
        || (c.group.isEmpty()
            && (c.category == Category::Weight || c.category == Category::Buoyancy))) {
        return QStringLiteral("Gewicht und Auftrieb");
    }
    return c.group.isEmpty() ? QStringLiteral("Weitere") : c.group;
}

bool isDrawable(const Contribution& c)
{
    return c.state == QStringLiteral("gueltig") && c.point && c.vector
        && norm(*c.vector) > 0.0;
}

// --- Zahlen und Zustände ---------------------------------------------------

QString formatNumber(double value)
{
    if (!std::isfinite(value)) {
        return QStringLiteral("ungültig");
    }
    if (value == 0.0) {
        return QStringLiteral("0");
    }
    const double magnitude = std::abs(value);
    int exponent = static_cast<int>(std::floor(std::log10(magnitude)));
    const double factor = std::pow(10.0, exponent - 3);
    const double rounded = std::round(value / factor) * factor;
    if (std::abs(rounded) >= std::pow(10.0, exponent + 1)) {
        ++exponent;
    }
    if (exponent < -3 || exponent >= 7) {
        return QString::number(value, 'e', 3);
    }
    return QString::number(rounded, 'f', std::max(0, 3 - exponent));
}

int decimalsFor(const QString& unit)
{
    return unit == QStringLiteral("m") ? 2 : 0;
}

QString formatFixed(double value, int decimals)
{
    if (!std::isfinite(value)) {
        return QStringLiteral("ungültig");
    }
    QString text = QString::number(value, 'f', decimals);
    // Gerundete Kleinstwerte nicht als "-0" anzeigen.
    if (text.startsWith(QLatin1Char('-'))) {
        bool allZero = true;
        for (const QChar ch : text.mid(1)) {
            if (ch != QLatin1Char('0') && ch != QLatin1Char('.')) {
                allZero = false;
                break;
            }
        }
        if (allZero) {
            text.remove(0, 1);
        }
    }
    return text;
}

QString formatValue(double value, const QString& unit)
{
    return formatFixed(value, decimalsFor(unit));
}

QString formatQuantity(double value, const QString& unit)
{
    return QStringLiteral("%1 %2").arg(formatValue(value, unit), unit);
}

QString stateText(const QString& state)
{
    if (state == QStringLiteral("gueltig") || state.isEmpty()) {
        return QStringLiteral("gültig");
    }
    if (state == QStringLiteral("null")) {
        return QStringLiteral("Null");
    }
    if (state == QStringLiteral("deaktiviert")) {
        return QStringLiteral("deaktiviert");
    }
    if (state == QStringLiteral("nicht_berechnet")) {
        return QStringLiteral("nicht berechnet");
    }
    if (state == QStringLiteral("ungueltig")) {
        return QStringLiteral("ungültig");
    }
    if (state == QStringLiteral("veraltet")) {
        return QStringLiteral("veraltet");
    }
    return state;
}

bool showsValues(const Contribution& c)
{
    return c.state == QStringLiteral("gueltig") && c.vector.has_value();
}

BalanceSummary balanceSummary(const ForceState& state)
{
    BalanceSummary summary;
    if (!state.hasBalance) {
        summary.title = QStringLiteral("Solverstatus: %1").arg(
            state.status.isEmpty() ? QStringLiteral("–") : state.status);
        summary.lines << QStringLiteral("Gleichgewicht nicht bewertet (Ergebnisvertrag V1)");
        for (const auto& c : state.contributions) {
            if (c.category == Category::MomentSum && c.vector) {
                summary.lines << QStringLiteral("ΣM um S: %1")
                                     .arg(formatQuantity(norm(*c.vector), c.unit));
            }
        }
        return summary;
    }
    summary.complete = state.balanceStatus == QStringLiteral("vollstaendig");
    summary.title = summary.complete
        ? QStringLiteral("Gesamtbilanz")
        : QStringLiteral("Verfügbare Teilsumme – unvollständig");
    if (state.total && state.total->force) {
        summary.lines << QStringLiteral("ΣF: %1")
                             .arg(formatQuantity(norm(*state.total->force), QStringLiteral("N")));
    } else {
        summary.lines << QStringLiteral("ΣF: nicht berechnet");
    }
    QString reference = QStringLiteral("S");
    if (state.reference) {
        reference = QStringLiteral("S = (%1; %2; %3)")
                        .arg(formatQuantity((*state.reference)[0], QStringLiteral("m")),
                            formatQuantity((*state.reference)[1], QStringLiteral("m")),
                            formatQuantity((*state.reference)[2], QStringLiteral("m")));
    }
    if (state.total && state.total->moment) {
        summary.lines << QStringLiteral("ΣM um %1: %2")
                             .arg(reference,
                                 formatQuantity(norm(*state.total->moment), QStringLiteral("N·m")));
    } else {
        summary.lines << QStringLiteral("ΣM: nicht berechnet");
    }
    summary.lines << QStringLiteral("Bilanzstatus: %1 · Solverstatus: %2")
                         .arg(state.balanceStatus.isEmpty() ? QStringLiteral("–")
                                                            : state.balanceStatus,
                             state.status.isEmpty() ? QStringLiteral("–") : state.status);
    if (!summary.complete) {
        summary.lines << (state.missing.isEmpty()
                ? QStringLiteral("Lücken: vom Core nicht einzeln benannt")
                : QStringLiteral("Lücken: %1").arg(state.missing.join(QStringLiteral(", "))));
    }
    if (state.equilibriumPresent) {
        const auto& e = state.equilibrium;
        summary.lines << QStringLiteral("Gleichgewicht: Residuum %1 (Toleranz %2), %3 (Toleranz %4)")
                             .arg(formatQuantity(e.value(QStringLiteral("residuum_kraft_n")).toDouble(),
                                      QStringLiteral("N")),
                                 QStringLiteral("%1 N").arg(formatNumber(
                                     e.value(QStringLiteral("krafttoleranz_n")).toDouble())),
                                 formatQuantity(e.value(QStringLiteral("residuum_moment_nm")).toDouble(),
                                     QStringLiteral("N·m")),
                                 QStringLiteral("%1 N·m").arg(formatNumber(
                                     e.value(QStringLiteral("momenttoleranz_nm")).toDouble())));
        QStringList freedoms;
        for (const auto& item : e.value(QStringLiteral("freiheitsgrade")).toArray()) {
            freedoms << item.toString();
        }
        summary.lines << QStringLiteral("Freiheitsgrade: %1 · Iterationen: %2")
                             .arg(freedoms.isEmpty() ? QStringLiteral("–")
                                                     : freedoms.join(QStringLiteral(", ")))
                             .arg(e.value(QStringLiteral("iterationen")).toInt());
    } else {
        summary.lines << QStringLiteral("Gleichgewicht nicht bewertet");
    }
    return summary;
}

// --- Geometrie -------------------------------------------------------------

std::optional<ArrowGeometry> arrowGeometry(
    const Vec3& start, const Vec3& vector, double length, double headLength)
{
    const double magnitude = norm(vector);
    if (!(magnitude > 0.0) || !std::isfinite(magnitude) || !(length > 0.0)
        || !std::isfinite(length)) {
        return std::nullopt;
    }
    ArrowGeometry arrow;
    arrow.direction = scale(vector, 1.0 / magnitude);
    arrow.start = start;
    arrow.length = length;
    arrow.tip = add(start, scale(arrow.direction, length));
    arrow.headBase = sub(arrow.tip,
        scale(arrow.direction, std::clamp(headLength, 0.0, length)));
    return arrow;
}

Vec3 stableArcBasis(const Vec3& normal, const std::optional<Vec3>& previous)
{
    const double n = norm(normal);
    const Vec3 unit = scale(normal, 1.0 / n);
    auto project = [&unit](const Vec3& v) { return sub(v, scale(unit, dot(v, unit))); };
    if (previous) {
        const auto projected = project(*previous);
        const double length = norm(projected);
        if (length > 0.3) {
            return scale(projected, 1.0 / length);
        }
    }
    // Weltachse mit dem kleinsten Anteil an der Normalen: gut konditioniert.
    std::size_t axis = 0;
    for (std::size_t i = 1; i < 3; ++i) {
        if (std::abs(unit[i]) < std::abs(unit[axis])) {
            axis = i;
        }
    }
    Vec3 seed{};
    seed[axis] = 1.0;
    const auto projected = project(seed);
    return scale(projected, 1.0 / norm(projected));
}

std::optional<ArcGeometry> momentArc(const Vec3& center, const Vec3& moment,
    double radius, int segments, const std::optional<Vec3>& previousBasis)
{
    const double magnitude = norm(moment);
    if (!(magnitude > 0.0) || !std::isfinite(magnitude) || !(radius > 0.0)
        || segments < 2) {
        return std::nullopt;
    }
    ArcGeometry arc;
    arc.normal = scale(moment, 1.0 / magnitude);
    arc.basis = stableArcBasis(arc.normal, previousBasis);
    const Vec3 second = cross(arc.normal, arc.basis);
    const double sweep = arcSweepDegrees * pi / 180.0;
    arc.points.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        const double angle = sweep * i / segments;
        arc.points.push_back(add(center,
            add(scale(arc.basis, radius * std::cos(angle)),
                scale(second, radius * std::sin(angle)))));
    }
    arc.tip = arc.points.back();
    arc.tangent = add(scale(arc.basis, -std::sin(sweep)), scale(second, std::cos(sweep)));
    return arc;
}

std::optional<Vec3> perpendicularLeverArm(const Vec3& r, const Vec3& force)
{
    const double forceSquared = dot(force, force);
    if (!(forceSquared > 0.0) || !std::isfinite(forceSquared)) {
        return std::nullopt;
    }
    return sub(r, scale(force, dot(r, force) / forceSquared));
}

// --- Auswahl -----------------------------------------------------------------

double distanceToPolyline(const QVector<QPointF>& polyline, const QPointF& point)
{
    if (polyline.isEmpty()) {
        return std::numeric_limits<double>::infinity();
    }
    if (polyline.size() == 1) {
        return segmentDistance(polyline.front(), polyline.front(), point);
    }
    double best = std::numeric_limits<double>::infinity();
    for (qsizetype i = 1; i < polyline.size(); ++i) {
        best = std::min(best, segmentDistance(polyline.at(i - 1), polyline.at(i), point));
    }
    return best;
}

QStringList hitTest(const QVector<ScreenShape>& shapes, const QPointF& point,
    double tolerance)
{
    QVector<std::pair<double, QString>> hits;
    for (const auto& shape : shapes) {
        const double distance = distanceToPolyline(shape.polyline, point);
        if (distance > tolerance) {
            continue;
        }
        auto existing = std::find_if(hits.begin(), hits.end(),
            [&shape](const auto& hit) { return hit.second == shape.id; });
        if (existing == hits.end()) {
            hits.push_back({distance, shape.id});
        } else {
            existing->first = std::min(existing->first, distance);
        }
    }
    std::stable_sort(hits.begin(), hits.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    QStringList ids;
    for (const auto& hit : hits) {
        ids << hit.second;
    }
    return ids;
}

} // namespace forcedisplay
