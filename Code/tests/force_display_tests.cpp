#include "force_display.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>
#include <limits>
#include <iostream>
#include <stdexcept>

using namespace forcedisplay;

namespace {

int checks = 0;

void require(bool condition, const char* message)
{
    ++checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}

bool nearVec(const Vec3& a, const Vec3& b, double tolerance = 1e-9)
{
    return near(a[0], b[0], tolerance) && near(a[1], b[1], tolerance)
        && near(a[2], b[2], tolerance);
}

QJsonObject parse(const char* json)
{
    return QJsonDocument::fromJson(QByteArray(json)).object();
}

void testScale()
{
    for (const auto mode : {ScaleMode::Logarithmic, ScaleMode::Linear}) {
        require(near(scaleMagnitude(0.0, 50.0, mode).u, 0.0), "u(0) must be 0.");
        require(near(scaleMagnitude(50.0, 50.0, mode).u, 1.0), "u(R) must be 1.");
        double previous = -1.0;
        for (int i = 0; i <= 200; ++i) {
            const double u = scaleMagnitude(i * 0.5, 50.0, mode).u;
            require(u >= previous, "Scale is not monotonic.");
            previous = u;
        }
        const auto over = scaleMagnitude(80.0, 50.0, mode);
        require(over.overScale && near(over.u, 1.0), "Over-scale value not marked/clamped.");
        require(!scaleMagnitude(50.0, 50.0, mode).overScale, "R itself is not over scale.");
        const auto small = scaleMagnitude(1e-6, 50.0, mode);
        require(small.smallValue && small.u < smallValueFraction, "Small value not marked.");
        require(!scaleMagnitude(0.0, 50.0, mode).smallValue, "Zero is not a small value.");
        require(!scaleMagnitude(25.0, 50.0, mode).smallValue, "Medium value marked small.");
        for (const double bad : {0.0, -1.0, std::nan(""), std::numeric_limits<double>::infinity()}) {
            bool rejected = false;
            try {
                scaleMagnitude(1.0, bad, mode);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            require(rejected, "Invalid reference was accepted.");
        }
    }
    const double m0 = 0.5;
    require(near(scaleMagnitude(0.5, 50.0, ScaleMode::Logarithmic).u,
                std::log1p(0.5 / m0) / std::log1p(50.0 / m0)),
        "Logarithmic formula differs from specification.");
    require(near(scaleMagnitude(10.0, 50.0, ScaleMode::Linear).u, 0.2),
        "Linear formula differs from specification.");
    require(near(*refitReference(10.0), 11.0), "Refit is not 1.1 x maximum.");
    require(!refitReference(0.0), "Refit without maximum must be empty.");
    require(near(pixelLength(scaleMagnitude(50.0, 50.0, ScaleMode::Linear),
                     maxArrowPx, minArrowPx), maxArrowPx),
        "Full scale must reach the maximum pixel length.");
    require(pixelLength(scaleMagnitude(0.0, 50.0, ScaleMode::Linear),
                maxArrowPx, minArrowPx) == 0.0,
        "Zero must have no pixel length.");
}

void testPoseAndArrow()
{
    // 90° um z: Modell-x zeigt nach Welt-y.
    const double s = std::sqrt(0.5);
    const auto state = parse(
        R"({"position_m":[1,2,3],"orientierung_xyzw":[0,0,0.7071067811865476,0.7071067811865476]})");
    const auto pose = readPose(state);
    require(near(pose.quaternionXyzw[3], s), "Quaternion not normalized.");
    require(nearVec(toWorldPoint(pose, {1, 0, 0}), {1, 3, 3}),
        "Point must be rotated and translated.");
    require(nearVec(toWorldVector(pose, {1, 0, 0}), {0, 1, 0}),
        "Vector must only be rotated.");
    const auto arrow = arrowGeometry(toWorldPoint(pose, {1, 0, 0}),
        toWorldVector(pose, {0, 0, -5}), 2.0, 0.5);
    require(arrow.has_value(), "Arrow missing.");
    require(nearVec(arrow->start, {1, 3, 3}), "Arrow origin is not the attack point.");
    require(nearVec(arrow->direction, {0, 0, -1}), "Arrow direction wrong.");
    require(nearVec(arrow->tip, {1, 3, 1}), "Arrow tip wrong.");
    require(nearVec(arrow->headBase, {1, 3, 1.5}), "Arrow head base wrong.");
    require(!arrowGeometry({0, 0, 0}, {0, 0, 0}, 2.0, 0.5), "Null vector produced an arrow.");
}

void testArc()
{
    const Vec3 center{1, 1, 1};
    const auto arc = momentArc(center, {0, 0, 10}, 2.0, 48, std::nullopt);
    require(arc.has_value(), "Arc missing.");
    require(arc->points.size() == 49, "Arc segment count wrong.");
    for (const auto& p : arc->points) {
        require(near(dot(sub(p, center), {0, 0, 1}), 0.0), "Arc leaves its plane.");
        require(near(norm(sub(p, center)), 2.0), "Arc radius wrong.");
    }
    // Rechte-Hand-Regel: Drehsinn positiv um den Momentenvektor.
    const auto first = sub(arc->points.at(0), center);
    const auto second = sub(arc->points.at(1), center);
    require(dot(cross(first, second), {0, 0, 1}) > 0.0, "Arc turns against the moment.");
    require(dot(cross(sub(arc->tip, center), arc->tangent), {0, 0, 1}) > 0.0,
        "Arc head points against the turning sense.");
    const double sweep = std::acos(dot(first, sub(arc->tip, center)) / 4.0);
    require(near(sweep, 2.0 * 3.14159265358979323846 - 240.0 * 3.14159265358979323846 / 180.0, 1e-9),
        "Arc does not span 240 degrees.");
    const auto flipped = momentArc(center, {0, 0, -10}, 2.0, 48, arc->basis);
    require(dot(cross(sub(flipped->points.at(0), center),
                    sub(flipped->points.at(1), center)),
                {0, 0, -1}) > 0.0,
        "Reversed moment must reverse the turning sense.");
    // Stabile Basis: kleine Richtungsänderung -> kleine Basisänderung.
    const auto tilted = momentArc(center, {0.02, 0.01, 10}, 2.0, 48, arc->basis);
    require(norm(sub(tilted->basis, arc->basis)) < 0.01, "Arc basis jumped.");
    require(!momentArc(center, {0, 0, 0}, 2.0, 48, std::nullopt), "Zero moment produced an arc.");
}

void testLeverArm()
{
    const Vec3 force{3, -4, 2};
    const auto arm = perpendicularLeverArm({1, 2, 5}, force);
    require(arm.has_value(), "Lever arm missing.");
    require(near(dot(*arm, force), 0.0, 1e-12), "Lever arm not perpendicular to F.");
    require(nearVec(cross(*arm, force), cross({1, 2, 5}, force)),
        "Lever arm must keep the moment r x F.");
    require(!perpendicularLeverArm({1, 2, 3}, {0, 0, 0}), "Lever arm for F = 0.");
}

const char* v1State = R"({
 "vertragsversion":"1","modell_id":"m","status":"konvergiert","zeit_s":0.5,"schritt":5,
 "position_m":[0,0,0],"orientierung_xyzw":[0,0,0,1],
 "moment_bezug_modell_m":[0,0,0],
 "vektoren":[
  {"id":"Rumpf:gewicht","element_id":"Rumpf","art":"kraft","einheit":"N","frame":"modell","punkt_m":[0,0,0],"vektor":[0,0,-100]},
  {"id":"Rumpf:auftrieb","element_id":"Rumpf","art":"kraft","einheit":"N","frame":"modell","punkt_m":[0,0,-1],"vektor":[0,0,100]},
  {"id":"Kiel:sonstwas","element_id":"Kiel","art":"kraft","einheit":"N","frame":"modell","punkt_m":[0,0,-1],"vektor":[1,0,0]},
  {"id":"gesamt:moment","element_id":"","art":"moment","einheit":"N·m","frame":"modell","punkt_m":[0,0,0],"vektor":[0,3,0]}
 ]})";

void testV1()
{
    const auto state = parseForceState(parse(v1State));
    require(state.contributions.size() == 4, "V1 contributions missing.");
    require(state.contributions.at(0).category == Category::Weight, "V1 weight mapping.");
    require(state.contributions.at(1).category == Category::Buoyancy, "V1 buoyancy mapping.");
    require(state.contributions.at(2).category == Category::Unassigned, "V1 unknown mapping.");
    require(state.contributions.at(3).category == Category::MomentSum, "V1 moment mapping.");
    require(state.contributions.at(3).kind == Kind::Moment, "V1 moment kind.");
    require(colorFor(Category::Weight) == QStringLiteral("#2166ac") && !isDashed(Category::Weight),
        "Weight style.");
    require(colorFor(Category::Buoyancy) == QStringLiteral("#2166ac") && isDashed(Category::Buoyancy),
        "Buoyancy style.");
    require(colorFor(Category::ForceSum) == QStringLiteral("#dddddd"), "Sum style.");
    require(colorFor(Category::Unassigned) != colorFor(Category::Test), "Test/unassigned colors.");
    require(!state.hasBalance, "V1 has no balance fields.");
    const auto summary = balanceSummary(state);
    require(summary.title.contains(QStringLiteral("konvergiert")), "V1 card shows solver status.");
    require(summary.lines.join('\n').contains(QStringLiteral("nicht bewertet")),
        "V1 card must state that equilibrium is not assessed.");
    require(contributionLabel(state.contributions.at(0)) == QStringLiteral("Rumpf · Gewicht"),
        "Label wrong.");
    require(groupLabel(state.contributions.at(1)) == QStringLiteral("Gewicht und Auftrieb"),
        "Group wrong.");
}

QByteArray v2State(const char* balance, const char* missing, const char* equilibrium)
{
    return QByteArray(R"({
 "vertragsversion":"2","modell_id":"m","status":"laufend","zeit_s":1,"schritt":10,
 "position_m":[0,0,0],"orientierung_xyzw":[0,0,0,1],"moment_bezug_modell_m":[0,0,0.5],
 "vektoren":[
  {"id":"A:gewicht","element_id":"A","art":"kraft","einheit":"N","frame":"modell","punkt_m":[0,0,0],"vektor":[0,0,-10],
   "quelle":"schwerkraft","gruppe":"gewicht_auftrieb","rolle":"einzelbeitrag","punktart":"angriffspunkt","zustand":"gueltig","fehler_id":null,"moment_s_nm":[0,0,0]},
  {"id":"A:auftrieb","element_id":"A","art":"kraft","einheit":"N","frame":"modell","punkt_m":[0,0,0],"vektor":[0,0,0],
   "quelle":"hydrostatik","gruppe":"gewicht_auftrieb","rolle":"einzelbeitrag","punktart":"angriffspunkt","zustand":"null","fehler_id":null,"moment_s_nm":[0,0,0]},
  {"id":"B:auftrieb","element_id":"B","art":"kraft","einheit":"N","frame":"modell","punkt_m":null,"vektor":null,
   "quelle":"hydrostatik","gruppe":"gewicht_auftrieb","rolle":"einzelbeitrag","punktart":"angriffspunkt","zustand":"ungueltig","fehler_id":"CORE-HYD-0001","moment_s_nm":null},
  {"id":"C:auftrieb","element_id":"C","art":"kraft","einheit":"N","frame":"modell","punkt_m":null,"vektor":null,
   "quelle":"hydrostatik","gruppe":"gewicht_auftrieb","rolle":"einzelbeitrag","punktart":"angriffspunkt","zustand":"nicht_berechnet","fehler_id":null,"moment_s_nm":null},
  {"id":"D:test","element_id":"D","art":"kraft","einheit":"N","frame":"modell","punkt_m":[0,0,0],"vektor":[1,0,0],
   "quelle":"test","gruppe":"test","rolle":"einzelbeitrag","punktart":"angriffspunkt","zustand":"deaktiviert","fehler_id":null,"moment_s_nm":[0,0,0]},
  {"id":"E:frei","element_id":"E","art":"kraft","einheit":"N","frame":"modell","punkt_m":[0,0,0],"vektor":[1,0,0],
   "quelle":"nicht_zugeordnet","gruppe":"gesamt","rolle":"einzelbeitrag","punktart":"angriffspunkt","zustand":"veraltet","fehler_id":null,"moment_s_nm":[0,0,0]},
  {"id":"gesamt:moment","element_id":"","art":"moment","einheit":"N·m","frame":"modell","punkt_m":[0,0,0.5],"vektor":[0,2,0],
   "quelle":"summe","gruppe":"gesamt","rolle":"gesamtsumme","punktart":"bezugspunkt","zustand":"gueltig","fehler_id":null,"moment_s_nm":[0,2,0]}
 ],
 "beitraege_auswertung":"gelieferter_zustand",
 "summen":{"gesamt":{"gruppe":"gesamt","frame":"modell","kraft_modell_n":[0,0,-10],"moment_s_nm":[0,2,0],"quellen":["A:gewicht"]},
           "gruppen":[{"gruppe":"gewicht_auftrieb","frame":"modell","kraft_modell_n":[0,0,-10],"moment_s_nm":[0,2,0],"quellen":["A:gewicht"]}]},
 "bilanzstatus":")") + balance + R"(","erwartete_beitraege":["A:gewicht","B:auftrieb"],"fehlende_beitraege":)"
        + missing + R"(,"gleichgewicht":)" + equilibrium + "}";
}

void testV2()
{
    const auto object = QJsonDocument::fromJson(v2State("teilsumme", R"(["B:auftrieb"])", "null")).object();
    require(!object.isEmpty(), "V2 test JSON invalid.");
    const auto state = parseForceState(object);
    require(state.hasBalance, "V2 balance missing.");
    require(state.contributions.size() == 8, "V2 contributions incl. ΣF expected.");
    const auto& sum = state.contributions.back();
    require(sum.category == Category::ForceSum && sum.fromSums, "ΣF not added from summen.");
    require(sum.point && nearVec(*sum.point, {0, 0, 0.5}), "ΣF must sit at reference S.");
    require(state.contributions.at(0).category == Category::Weight, "V2 weight via quelle.");
    require(state.contributions.at(2).category == Category::Buoyancy, "V2 buoyancy via quelle.");
    require(state.contributions.at(4).category == Category::Test, "V2 test via quelle.");
    require(state.contributions.at(5).category == Category::Unassigned, "V2 unassigned via quelle.");
    require(state.contributions.at(6).category == Category::MomentSum, "V2 moment sum.");

    // Jeder Datenzustand erscheint als Text, nie leer oder als 0.
    const char* expected[] = {"gültig", "Null", "ungültig", "nicht berechnet",
        "deaktiviert", "veraltet"};
    for (int i = 0; i < 6; ++i) {
        const auto& c = state.contributions.at(i);
        require(stateText(c.state) == QString::fromUtf8(expected[i]), "State text wrong.");
        require(showsValues(c) == (i == 0), "Only valid contributions show numbers.");
    }
    require(!isDrawable(state.contributions.at(1)), "Null contribution must not be drawn.");
    require(!isDrawable(state.contributions.at(2)), "Invalid contribution must not be drawn.");
    require(!state.contributions.at(2).point, "null point must stay empty.");
    require(isDrawable(state.contributions.at(0)), "Valid contribution must be drawn.");

    const auto partial = balanceSummary(state);
    require(!partial.complete
            && partial.title == QStringLiteral("Verfügbare Teilsumme – unvollständig"),
        "Partial balance title wrong.");
    require(partial.lines.join('\n').contains(QStringLiteral("B:auftrieb")), "Gaps not listed.");
    require(partial.lines.join('\n').contains(QStringLiteral("Gleichgewicht nicht bewertet")),
        "Missing equilibrium must be stated.");

    const auto open = balanceSummary(parseForceState(
        QJsonDocument::fromJson(v2State("ungeklaert", "[]", "null")).object()));
    require(!open.complete && open.title.contains(QStringLiteral("unvollständig")),
        "Unresolved balance must be incomplete.");

    const auto complete = balanceSummary(parseForceState(QJsonDocument::fromJson(
        v2State("vollstaendig", "[]",
            R"({"krafttoleranz_n":0.001,"momenttoleranz_nm":0.001,"residuum_kraft_n":1e-11,"residuum_moment_nm":0,"freiheitsgrade":["position_z"],"iterationen":3})"))
                                                                .object()));
    require(complete.complete && complete.title == QStringLiteral("Gesamtbilanz"),
        "Complete balance title wrong.");
    const auto text = complete.lines.join('\n');
    require(text.contains(QStringLiteral("Iterationen: 3"))
            && text.contains(QStringLiteral("position_z")),
        "Equilibrium details missing.");
    require(text.contains(QStringLiteral("Residuum 0 N (Toleranz 0.001000 N)")),
        "Residual rounded, tolerance kept readable.");
    require(text.contains(QStringLiteral(" m; ")) || !text.contains(QStringLiteral("S = (")),
        "Reference components carry the unit m.");
}

void testDerived()
{
    ForceState state;
    state.reference = Vec3{0, 0, 0};
    auto make = [](const QString& id, Category category, Vec3 force, Vec3 point, Vec3 moment) {
        Contribution c;
        c.id = id;
        c.elementId = id.section(':', 0, 0);
        c.kind = Kind::Force;
        c.unit = QStringLiteral("N");
        c.frame = QStringLiteral("modell");
        c.state = QStringLiteral("gueltig");
        c.category = category;
        c.vector = force;
        c.point = point;
        c.momentS = moment;
        return c;
    };
    state.contributions.push_back(
        make(QStringLiteral("A:gewicht"), Category::Weight, {0, 0, -10}, {1, 0, 0}, {0, 10, 0}));
    state.contributions.push_back(
        make(QStringLiteral("B:gewicht"), Category::Weight, {0, 0, -30}, {-1, 0, 0}, {0, -30, 0}));
    state.contributions.push_back(
        make(QStringLiteral("A:auftrieb"), Category::Buoyancy, {0, 0, 40}, {0, 0, -1}, {0, 0, 0}));
    auto invalid = make(QStringLiteral("C:gewicht"), Category::Weight, {0, 0, -99}, {5, 0, 0},
        {0, 99, 0});
    invalid.state = QStringLiteral("ungueltig");
    state.contributions.push_back(invalid);

    const auto derived = derivedContributions(state);
    const Contribution* weightTotal = nullptr;
    const Contribution* weightMoment = nullptr;
    const Contribution* buoyancyTotal = nullptr;
    int partMoments = 0;
    for (const auto& c : derived) {
        if (c.id == QStringLiteral("anzeige:gewicht")) {
            weightTotal = &c;
        } else if (c.id == QStringLiteral("anzeige:gewicht:moment")) {
            weightMoment = &c;
        } else if (c.id == QStringLiteral("anzeige:auftrieb")) {
            buoyancyTotal = &c;
        } else if (c.id.endsWith(QStringLiteral(":moment")) && !c.id.startsWith(QStringLiteral("anzeige:"))) {
            ++partMoments;
            require(c.kind == Kind::Moment && !c.total, "Part moment kind wrong.");
        }
    }
    require(weightTotal && weightTotal->total && weightTotal->kind == Kind::Force,
        "Total weight missing.");
    require(nearVec(*weightTotal->vector, {0, 0, -40}), "Invalid parts must not be summed.");
    require(nearVec(*weightTotal->point, {-0.5, 0, 0}), "Total weight must sit in G.");
    require(nearVec(*weightTotal->momentS, {0, -20, 0}), "Total weight moment wrong.");
    require(weightMoment && weightMoment->kind == Kind::Moment
            && nearVec(*weightMoment->vector, {0, -20, 0}),
        "Total weight moment vector missing.");
    require(buoyancyTotal && nearVec(*buoyancyTotal->vector, {0, 0, 40}), "Total buoyancy missing.");
    require(partMoments == 3, "One moment per valid part expected.");
    require(contributionLabel(*weightTotal).contains(QStringLiteral("gesamt")),
        "Total label must say gesamt.");
    require(colorForContribution(*weightTotal) == QStringLiteral("#67a9cf"), "Total color.");
}

void testFormat()
{
    require(formatNumber(74556.0) == QStringLiteral("74560"), "4 significant digits (large).");
    require(formatNumber(1.23456) == QStringLiteral("1.235"), "4 significant digits (unit).");
    require(formatNumber(-0.0123456) == QStringLiteral("-0.01235"), "4 significant digits (small).");
    require(formatNumber(9999.6) == QStringLiteral("10000"), "Rounding carry.");
    require(formatNumber(1.4551915e-11) == QStringLiteral("1.455e-11"), "Tiny values use exponent.");
    require(formatNumber(0.0) == QStringLiteral("0"), "Zero.");
    require(formatQuantity(2.0, QStringLiteral("N")) == QStringLiteral("2 N"), "Unit.");
    require(formatQuantity(74556.4, QStringLiteral("N")) == QStringLiteral("74556 N"),
        "Forces to 1 N.");
    require(formatValue(1.4551915e-11, QStringLiteral("N")) == QStringLiteral("0"),
        "No exponent for tiny forces.");
    require(formatValue(-0.3, QStringLiteral("N·m")) == QStringLiteral("0"), "No negative zero.");
    require(formatValue(-1.6, QStringLiteral("N·m")) == QStringLiteral("-2"), "Moments to 1 N·m.");
    require(formatValue(1.23456, QStringLiteral("m")) == QStringLiteral("1.23"), "Lengths to 0.01 m.");
    require(formatValue(-0.004, QStringLiteral("m")) == QStringLiteral("0.00"), "No negative zero (m).");
}

void testHit()
{
    QVector<ScreenShape> shapes;
    shapes.push_back({QStringLiteral("a"), {QPointF(0, 0), QPointF(100, 0)}});
    shapes.push_back({QStringLiteral("b"), {QPointF(50, -50), QPointF(50, 50)}});
    shapes.push_back({QStringLiteral("c"), {QPointF(200, 200), QPointF(300, 200)}});
    const auto both = hitTest(shapes, QPointF(51, 2));
    require(both.size() == 2 && both.at(0) == QStringLiteral("b"),
        "Multiple hits must be reported, nearest first.");
    require(hitTest(shapes, QPointF(10, 6)).size() == 1, "6 px must still hit.");
    require(hitTest(shapes, QPointF(10, 6.5)).isEmpty(), "Beyond 6 px must not hit.");
}

} // namespace

int main()
{
    try {
        testScale();
        testPoseAndArrow();
        testArc();
        testLeverArm();
        testV1();
        testV2();
        testDerived();
        testFormat();
        testHit();
        std::cout << "Force display logic passed (" << checks << " checks).\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
