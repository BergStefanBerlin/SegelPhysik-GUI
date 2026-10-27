#pragma once

// Reine Darstellungslogik für Kraftpfeile, Momentenbögen und Wertefenster
// (G-0.2, A-G-00009 bis A-G-00017). Keine Physik: Summen, Momente und
// Zustände kommen aus dem Core; hier entstehen nur Skalierung, Geometrie,
// Zuordnung und Texte. Ohne VTK/Qt-GUI testbar.

#include <QJsonObject>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVector>

#include <array>
#include <optional>

namespace forcedisplay {

using Vec3 = std::array<double, 3>;

double dot(const Vec3& a, const Vec3& b);
Vec3 cross(const Vec3& a, const Vec3& b);
double norm(const Vec3& a);
Vec3 add(const Vec3& a, const Vec3& b);
Vec3 sub(const Vec3& a, const Vec3& b);
Vec3 scale(const Vec3& a, double factor);

// Bildschirmgrößen in logischen Pixeln (A-G-00012).
constexpr double arrowHeadPx = 10.0;
constexpr double strokePx = 3.0;
constexpr double maxArrowPx = 180.0;
constexpr double minArrowPx = 14.0;
constexpr double maxArcRadiusPx = 48.0;
constexpr double minArcRadiusPx = 10.0;
constexpr double hitTolerancePx = 6.0;
constexpr double arcSweepDegrees = 240.0;
// Grafische Schwelle für „Kleinwert“; keine physikalische Toleranz.
constexpr double smallValueFraction = 0.02;

// --- Skalierung (A-G-00011) ---------------------------------------------
enum class ScaleMode { Logarithmic, Linear };

struct ScaledMagnitude {
    double u = 0.0;
    bool overScale = false;
    bool smallValue = false;
};

bool isValidReference(double reference);
// Wirft std::invalid_argument bei ungültiger Referenz oder ungültigem Betrag.
ScaledMagnitude scaleMagnitude(double magnitude, double reference, ScaleMode mode);
// „Neu einpassen“: 1,1-Faches des aktuellen Maximums; leer, wenn es kein
// darstellbares Maximum gibt.
std::optional<double> refitReference(double currentMaximum);
double pixelLength(const ScaledMagnitude& scaled, double maximumPx, double minimumPx);

// --- Pose ----------------------------------------------------------------
struct Pose {
    Vec3 position{};
    std::array<double, 4> quaternionXyzw{0.0, 0.0, 0.0, 1.0};
};
Pose readPose(const QJsonObject& state);
Vec3 toWorldPoint(const Pose& pose, const Vec3& modelPoint);
Vec3 toWorldVector(const Pose& pose, const Vec3& modelVector);

// --- Beiträge (V1 und V2) -----------------------------------------------
enum class Kind { Force, Moment };
enum class Category { Weight, Buoyancy, ForceSum, MomentSum, Test, Unassigned, Other };

struct Contribution {
    QString id;
    QString elementId;
    Kind kind = Kind::Force;
    QString unit;
    QString frame;
    QString source;
    QString group;
    QString role;
    QString pointKind;
    QString state;   // V2-Datenzustand; bei V1 "gueltig"
    QString errorId;
    std::optional<Vec3> point;
    std::optional<Vec3> vector;
    std::optional<Vec3> momentS;
    Category category = Category::Unassigned;
    bool fromSums = false;  // aus summen.gesamt übernommen (ΣF)
    bool total = false;     // GUI-Aggregat „gesamt“ (Gewicht bzw. Auftrieb)
};

struct GroupSum {
    QString group;
    std::optional<Vec3> force;
    std::optional<Vec3> moment;
    QStringList sources;
};

struct ForceState {
    QString contract;
    QString modelId;
    double time = 0.0;
    qint64 step = 0;
    QString status;
    QString diagnostic;
    std::optional<Vec3> reference;  // Momentenbezug S (Modellframe)
    QVector<Contribution> contributions;
    bool hasBalance = false;  // V2-Bilanzfelder vorhanden
    QString balanceStatus;
    QStringList expected;
    QStringList missing;
    std::optional<GroupSum> total;
    QVector<GroupSum> groups;
    bool equilibriumPresent = false;
    QJsonObject equilibrium;
    // A-C-00047: dauerhaft vollgelaufene Elemente (tragen keinen Auftrieb).
    QStringList flooded;
    bool testLimiterApplied = false;
};

// Wirft std::runtime_error bei strukturell beschädigten Daten. `null` für
// Punkt/Vektor ist erlaubt und führt zu einem leeren optional.
ForceState parseForceState(const QJsonObject& state);
// Nur für die Grafik: Gewicht/Auftrieb gesamt (Kraft und Moment um S) sowie
// Teilmomente je Element, gebildet ausschließlich durch Aufsummieren der vom
// Core gelieferten Kräfte, Angriffspunkte und Momente um S (A-G-00014).
QVector<Contribution> derivedContributions(const ForceState& state);
Category categorize(const Contribution& contribution);
QString colorFor(Category category, const QString& group = {});
// Wie colorFor; Gesamtwerte (Contribution::total) erhalten ein helleres Blau.
QString colorForContribution(const Contribution& contribution);
bool isDashed(Category category);
QString categoryName(Category category);
QString contributionLabel(const Contribution& contribution);
QString groupLabel(const Contribution& contribution);
bool isDrawable(const Contribution& contribution);

// --- Zahlen und Zustände (A-G-00017) ------------------------------------
QString formatNumber(double value);
// Feste Auflösung je Einheit: N und N·m auf 1, m auf 0,01; nie Exponentenschreibweise.
int decimalsFor(const QString& unit);
QString formatFixed(double value, int decimals);
QString formatValue(double value, const QString& unit);
QString formatQuantity(double value, const QString& unit);
QString stateText(const QString& state);
bool showsValues(const Contribution& contribution);

struct BalanceSummary {
    QString title;
    bool complete = false;
    QStringList lines;
};
BalanceSummary balanceSummary(const ForceState& state);

// --- Geometrie (A-G-00009, A-G-00010, A-G-00015) ------------------------
struct ArrowGeometry {
    Vec3 start{};
    Vec3 tip{};
    Vec3 headBase{};
    Vec3 direction{};
    double length = 0.0;
};
std::optional<ArrowGeometry> arrowGeometry(
    const Vec3& start, const Vec3& vector, double length, double headLength);

Vec3 stableArcBasis(const Vec3& normal, const std::optional<Vec3>& previous);
struct ArcGeometry {
    QVector<Vec3> points;
    Vec3 tip{};
    Vec3 tangent{};
    Vec3 normal{};
    Vec3 basis{};
};
std::optional<ArcGeometry> momentArc(const Vec3& center, const Vec3& moment,
    double radius, int segments, const std::optional<Vec3>& previousBasis);

std::optional<Vec3> perpendicularLeverArm(const Vec3& r, const Vec3& force);

// --- Auswahl (A-G-00014) -------------------------------------------------
struct ScreenShape {
    QString id;
    QVector<QPointF> polyline;
};
double distanceToPolyline(const QVector<QPointF>& polyline, const QPointF& point);
// Alle Treffer innerhalb der Toleranz, nach Abstand sortiert, ohne Duplikate.
QStringList hitTest(const QVector<ScreenShape>& shapes, const QPointF& point,
    double tolerance = hitTolerancePx);

} // namespace forcedisplay
