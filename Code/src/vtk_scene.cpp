#include "vtk_scene.hpp"
#include <QJsonArray>
#include "navigation.hpp"
#include "segelphysik/wind.hpp"
#include <QScrollArea>

#include <QColor>
#include <QEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QWheelEvent>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QCursor>
#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>
#include <QPainterPath>
#include <QRegion>

#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCaptionActor2D.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkCellArray.h>
#include <vtkCellPicker.h>
#include <vtkClipPolyData.h>
#include <vtkCommand.h>
#include <vtkContourTriangulator.h>
#include <vtkCutter.h>
#include <vtkDataArray.h>
#include <vtkFloatArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkHDRReader.h>
#include <vtkImageData.h>
#include <vtkLightKit.h>
#include <vtkLight.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataTangents.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderState.h>
#include <vtkShadowMapBakerPass.h>
#include <vtkShadowMapPass.h>
#include <vtkShaderProperty.h>
#include <vtkUniforms.h>
#include <vtkSkybox.h>
#include <vtkSphereSource.h>
#include <vtkSSAOPass.h>
#include <vtkStripper.h>
#include <vtkTextProperty.h>
#include <vtkTextActor.h>
#include <vtkTexture.h>
#include <vtkTriangleFilter.h>
#include <vtkTransform.h>
#include <vtkUnsignedCharArray.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <map>
#include <cmath>
#include <vector>
#include <limits>
#include <stdexcept>

using forcedisplay::Category;
using forcedisplay::Kind;
using forcedisplay::Vec3;

namespace {

class SceneRenderer final : public vtkOpenGLRenderer {
public:
    static SceneRenderer* New();
    vtkTypeMacro(SceneRenderer, vtkOpenGLRenderer);
    bool sunShadows = false;
    unsigned int shadowResolution = 1024;

    void DeviceRenderOpaqueGeometry(vtkFrameBufferObjectBase* fbo = nullptr) override
    {
        if (!sunShadows) {
            this->Superclass::DeviceRenderOpaqueGeometry(fbo);
            return;
        }
        if (!shadows_) shadows_ = vtkSmartPointer<vtkShadowMapPass>::New();
        shadows_->GetShadowMapBakerPass()->SetResolution(shadowResolution);
        std::vector<vtkProp*> litProps;
        std::vector<vtkProp*> unlitProps;
        for (int i = 0; i < this->PropArrayCount; ++i) {
            auto* actor = vtkActor::SafeDownCast(this->PropArray[i]);
            if (actor && actor->GetProperty()->GetLighting()
                && !vtkSkybox::SafeDownCast(actor)) {
                litProps.push_back(actor);
            } else {
                unlitProps.push_back(this->PropArray[i]);
            }
        }
        vtkRenderState state(this);
        state.SetPropArrayAndCount(litProps.data(), static_cast<int>(litProps.size()));
        state.SetFrameBuffer(fbo);
        shadows_->GetShadowMapBakerPass()->Render(&state);
        if (this->GetUseSSAO()) {
            if (!ambientOcclusion_) ambientOcclusion_ = vtkSmartPointer<vtkSSAOPass>::New();
            ambientOcclusion_->SetDelegatePass(shadows_);
            ambientOcclusion_->SetRadius(this->GetSSAORadius());
            ambientOcclusion_->SetBias(this->GetSSAOBias());
            ambientOcclusion_->SetKernelSize(this->GetSSAOKernelSize());
            ambientOcclusion_->SetBlur(this->GetSSAOBlur());
            ambientOcclusion_->Render(&state);
            this->NumberOfPropsRendered += ambientOcclusion_->GetNumberOfRenderedProps();
        } else {
            shadows_->Render(&state);
            this->NumberOfPropsRendered += shadows_->GetNumberOfRenderedProps();
        }
        // Shadow shaders require view-space vertices, which unlit helpers and
        // the skybox intentionally do not provide.
        for (auto* prop : unlitProps) {
            this->NumberOfPropsRendered += prop->RenderOpaqueGeometry(this);
        }
    }

    void ReleaseGraphicsResources(vtkWindow* window) override
    {
        if (ambientOcclusion_) ambientOcclusion_->ReleaseGraphicsResources(window);
        if (shadows_) shadows_->ReleaseGraphicsResources(window);
        this->Superclass::ReleaseGraphicsResources(window);
    }

private:
    vtkSmartPointer<vtkShadowMapPass> shadows_;
    vtkSmartPointer<vtkSSAOPass> ambientOcclusion_;
};

vtkStandardNewMacro(SceneRenderer);

constexpr double pi = 3.14159265358979323846;
constexpr double proceduralTexturePeriod = 0.35;
constexpr double dashPx = 8.0;
constexpr double gapPx = 5.0;
constexpr int arcSegments = 48;
const QString selectionColor = QStringLiteral("#ffd166");

Vec3 readStateVector(const QJsonObject& state, const QString& field)
{
    const auto values = state.value(field).toArray();
    if (values.size() != 3) {
        throw std::runtime_error(
            QStringLiteral("Core-Zustand enthält keinen gültigen 3D-Vektor: %1")
                .arg(field).toStdString());
    }
    Vec3 result{};
    for (qsizetype axis = 0; axis < 3; ++axis) {
        const auto value = values.at(axis);
        if (!value.isDouble() || !std::isfinite(value.toDouble())) {
            throw std::runtime_error(
                QStringLiteral("Core-Zustand enthält nicht endliche Vektordaten: %1")
                    .arg(field).toStdString());
        }
        result[static_cast<std::size_t>(axis)] = value.toDouble();
    }
    return result;
}

void validateWindSnapshot(const QJsonObject& state)
{
    const auto value = state.value(QStringLiteral("wind"));
    if (value.isUndefined()) return;
    if (!value.isObject()) throw std::runtime_error("Core-Winddaten sind kein Objekt.");
    const auto wind = value.toObject();
    for (const auto& field : {QStringLiteral("zeit_s"), QStringLiteral("schritt"),
                              QStringLiteral("modell_fassung")}) {
        if (wind.value(field).isUndefined() || wind.value(field) != state.value(field))
            throw std::runtime_error("Core-Winddaten gehoeren zu einem anderen Ergebnisstand.");
    }
}

void readWindNavigation(const QJsonObject& state, navigation::Values& values)
{
    validateWindSnapshot(state);
    const auto wind = state.value(QStringLiteral("wind")).toObject();
    if (wind.isEmpty()) {
        values.windDiagnostic = QStringLiteral("Winddaten nicht verfügbar");
        return;
    }
    const auto marker = [](const QJsonObject& object) -> std::optional<double> {
        const auto angle = object.value(QStringLiteral("herkunft_welt_grad"));
        if (object.value(QStringLiteral("kompass_sichtbar")).toBool() &&
            angle.isDouble() && std::isfinite(angle.toDouble()))
            return angle.toDouble();
        return {};
    };
    values.trueWindDirection = marker(wind.value(QStringLiteral("wahr")).toObject());
    values.apparentWindDirection = marker(wind.value(QStringLiteral("scheinbar")).toObject());
    values.windDiagnostic = wind.value(QStringLiteral("diagnose")).toString();
    std::optional<Vec3> mast;
    for (const auto& entry : state.value(QStringLiteral("referenzpunkte")).toArray()) {
        const auto point = entry.toObject();
        if (point.value(QStringLiteral("id")).toString() != QStringLiteral("mastspitze")) continue;
        if (point.value(QStringLiteral("positionsstatus")).toString() == QStringLiteral("gueltig"))
            mast = readStateVector(point, QStringLiteral("position_welt_m"));
        break;
    }
    if (!mast) {
        if (values.windDiagnostic.isEmpty()) values.windDiagnostic = QStringLiteral("Kein Mast");
        return;
    }
    const auto speed = wind.value(QStringLiteral("referenzgeschwindigkeit_m_s"));
    const auto height = wind.value(QStringLiteral("referenzhoehe_m"));
    if (!speed.isDouble() || !height.isDouble() || !std::isfinite(speed.toDouble()) ||
        !std::isfinite(height.toDouble())) {
        values.windDiagnostic = QStringLiteral("Ungültiges Windprofil");
        return;
    }
    segelphysik::WindKonfiguration config;
    config.referenzgeschwindigkeit_m_s = speed.toDouble();
    config.referenzhoehe_m = height.toDouble();
    const double upper = (std::floor((*mast)[2] / 5.) + 1.) * 5.;
    if (!std::isfinite(upper) || upper <= .0002) {
        values.windDiagnostic = QStringLiteral("Ungültige Masthöhe");
        return;
    }
    try {
        for (int i = 0; i <= 40; ++i) {
            const double fraction = i / 40.;
            const double h = .5 + (upper - .5) * fraction * fraction;
            values.windProfile.push_back({h, segelphysik::windprofil(config, h)});
        }
        values.windReference = std::array<double, 2>{height.toDouble(), speed.toDouble()};
    } catch (const std::invalid_argument& error) {
        values.windProfile.clear();
        values.windDiagnostic = QString::fromUtf8(error.what());
    }
}

Vec3 normalized(const Vec3& v)
{
    const double length = forcedisplay::norm(v);
    return length > 0.0 ? forcedisplay::scale(v, 1.0 / length) : Vec3{0.0, 0.0, 0.0};
}

Vec3 anyPerpendicular(const Vec3& direction)
{
    const Vec3 axis = std::abs(direction[0]) < 0.9 ? Vec3{1.0, 0.0, 0.0} : Vec3{0.0, 1.0, 0.0};
    return normalized(forcedisplay::cross(direction, axis));
}

Vec3 inverseRotate(const forcedisplay::Pose& pose, const Vec3& v)
{
    forcedisplay::Pose inverse = pose;
    inverse.quaternionXyzw[0] = -pose.quaternionXyzw[0];
    inverse.quaternionXyzw[1] = -pose.quaternionXyzw[1];
    inverse.quaternionXyzw[2] = -pose.quaternionXyzw[2];
    return forcedisplay::toWorldVector(inverse, v);
}

// Wasserlinie um einen kleinen Betrag entlang der nach außen gerichteten
// Flächennormalen versetzen. Bei Rümpfen ohne Wanddicke (offene Schalen) liegt
// die Linie sonst exakt auf der Fläche und scheint von innen durch.
void offsetWaterline(vtkPolyData* cut, double offset, vtkPolyData* out)
{
    out->DeepCopy(cut);
    vtkDataArray* normals = cut->GetPointData()->GetNormals();
    vtkPoints* source = cut->GetPoints();
    if (normals == nullptr || source == nullptr) return;
    auto moved = vtkSmartPointer<vtkPoints>::New();
    moved->SetNumberOfPoints(source->GetNumberOfPoints());
    for (vtkIdType i = 0; i < source->GetNumberOfPoints(); ++i) {
        double p[3];
        double n[3];
        source->GetPoint(i, p);
        normals->GetTuple(i, n);
        moved->SetPoint(i, p[0] + offset * n[0], p[1] + offset * n[1], p[2] + offset * n[2]);
    }
    out->SetPoints(moved);
}

// Punktnormalen eines Teils so ausrichten, dass sie im Mittel vom Schwerpunkt
// weg zeigen (auch für offene Schalen, bei denen AutoOrient nicht greift).
vtkSmartPointer<vtkPolyData> outwardNormals(vtkPolyData* mesh)
{
    auto filter = vtkSmartPointer<vtkPolyDataNormals>::New();
    filter->SetInputData(mesh);
    filter->SplittingOff();
    filter->ConsistencyOn();
    filter->ComputePointNormalsOn();
    filter->ComputeCellNormalsOff();
    filter->Update();
    auto result = vtkSmartPointer<vtkPolyData>::New();
    result->DeepCopy(filter->GetOutput());
    vtkDataArray* normals = result->GetPointData()->GetNormals();
    vtkPoints* points = result->GetPoints();
    if (normals == nullptr || points == nullptr || points->GetNumberOfPoints() == 0) return result;
    double center[3] = {0.0, 0.0, 0.0};
    const vtkIdType count = points->GetNumberOfPoints();
    for (vtkIdType i = 0; i < count; ++i) {
        double p[3];
        points->GetPoint(i, p);
        for (int k = 0; k < 3; ++k) center[k] += p[k] / static_cast<double>(count);
    }
    double sum = 0.0;
    for (vtkIdType i = 0; i < count; ++i) {
        double p[3];
        double n[3];
        points->GetPoint(i, p);
        normals->GetTuple(i, n);
        sum += n[0] * (p[0] - center[0]) + n[1] * (p[1] - center[1]) + n[2] * (p[2] - center[2]);
    }
    if (sum < 0.0) {
        for (vtkIdType i = 0; i < count; ++i) {
            double n[3];
            normals->GetTuple(i, n);
            n[0] = -n[0];
            n[1] = -n[1];
            n[2] = -n[2];
            normals->SetTuple(i, n);
        }
    }
    return result;
}

using Loop2 = std::vector<std::array<double, 2>>;

bool insideLoop(const Loop2& loop, double x, double y)
{
    bool in = false;
    const std::size_t n = loop.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const auto& a = loop[i];
        const auto& b = loop[j];
        if ((a[1] > y) != (b[1] > y) && x < (b[0] - a[0]) * (y - a[1]) / (b[1] - a[1]) + a[0]) {
            in = !in;
        }
    }
    return in;
}

bool insideAny(const std::vector<Loop2>& loops, double x, double y)
{
    for (const auto& loop : loops) {
        if (insideLoop(loop, x, y)) return true;
    }
    return false;
}

double signedArea(const Loop2& loop)
{
    double area = 0.0;
    for (std::size_t k = 0; k < loop.size(); ++k) {
        const auto& a = loop[k];
        const auto& b = loop[(k + 1) % loop.size()];
        area += a[0] * b[1] - b[0] * a[1];
    }
    return 0.5 * area;
}

// Mehrheit der Punkte von inner liegt in outer.
bool loopWithin(const Loop2& inner, const Loop2& outer)
{
    if (outer.size() < 3 || inner.empty()) return false;
    std::size_t hits = 0;
    for (const auto& p : inner) hits += insideLoop(outer, p[0], p[1]) ? 1 : 0;
    return 2 * hits > inner.size();
}

// Teilintervalle [t0, t1] der Strecke a-b, die außerhalb aller Schleifen liegen.
std::vector<std::array<double, 2>> outsideIntervals(
    const std::vector<Loop2>& loops, const double a[2], const double b[2])
{
    std::vector<double> ts{0.0, 1.0};
    const double d[2] = {b[0] - a[0], b[1] - a[1]};
    for (const auto& loop : loops) {
        for (std::size_t k = 0; k < loop.size(); ++k) {
            const auto& p = loop[k];
            const auto& q = loop[(k + 1) % loop.size()];
            const double e[2] = {q[0] - p[0], q[1] - p[1]};
            const double den = d[0] * e[1] - d[1] * e[0];
            if (std::abs(den) < 1e-18) continue;
            const double w[2] = {p[0] - a[0], p[1] - a[1]};
            const double t = (w[0] * e[1] - w[1] * e[0]) / den;
            const double u = (w[0] * d[1] - w[1] * d[0]) / den;
            if (t > 0.0 && t < 1.0 && u >= 0.0 && u <= 1.0) ts.push_back(t);
        }
    }
    std::sort(ts.begin(), ts.end());
    std::vector<std::array<double, 2>> kept;
    for (std::size_t k = 0; k + 1 < ts.size(); ++k) {
        const double t0 = ts[k];
        const double t1 = ts[k + 1];
        if (t1 - t0 < 1e-9) continue;
        const double tm = 0.5 * (t0 + t1);
        if (insideAny(loops, a[0] + tm * d[0], a[1] + tm * d[1])) continue;
        if (!kept.empty() && std::abs(kept.back()[1] - t0) < 1e-12) {
            kept.back()[1] = t1;
        } else {
            kept.push_back({t0, t1});
        }
    }
    return kept;
}

vtkSmartPointer<vtkTransform> poseTransform(const forcedisplay::Pose& pose)
{
    auto transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(pose.position[0], pose.position[1], pose.position[2]);
    const Vec3 q{pose.quaternionXyzw[0], pose.quaternionXyzw[1], pose.quaternionXyzw[2]};
    const double length = forcedisplay::norm(q);
    if (length > 1e-12) {
        const double angle = 2.0 * std::atan2(length, pose.quaternionXyzw[3]);
        transform->RotateWXYZ(angle * 180.0 / pi, q[0] / length, q[1] / length, q[2] / length);
    }
    return transform;
}

void applyCoreMatrix(vtkPolyData* mesh, const QJsonArray& values)
{
    if (mesh == nullptr || values.size() != 16)
        throw std::runtime_error("Core-Elementtransformation muss eine 4x4-Matrix enthalten.");
    double matrix[16]{};
    for (qsizetype i = 0; i < 16; ++i) {
        const auto value = values.at(i);
        if (!value.isDouble() || !std::isfinite(value.toDouble()))
            throw std::runtime_error("Core-Elementtransformation enthaelt keinen endlichen Wert.");
        matrix[i] = value.toDouble();
    }
    auto* points = mesh->GetPoints();
    if (points == nullptr) return;
    for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i) {
        double p[3];
        points->GetPoint(i, p);
        const double transformed[3] = {
            matrix[0]*p[0] + matrix[1]*p[1] + matrix[2]*p[2] + matrix[3],
            matrix[4]*p[0] + matrix[5]*p[1] + matrix[6]*p[2] + matrix[7],
            matrix[8]*p[0] + matrix[9]*p[1] + matrix[10]*p[2] + matrix[11]};
        points->SetPoint(i, transformed);
    }
    points->Modified();
    mesh->Modified();
}

void setActorColor(vtkActor* actor, const QString& hex)
{
    const QColor color(hex);
    actor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
}

// Sammelt Linien und Dreiecke eines Pfeils oder Bogens in Weltkoordinaten.
struct PolyBuilder {
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> cells;

    vtkIdType add(const Vec3& p) { return points->InsertNextPoint(p[0], p[1], p[2]); }
    void segment(const Vec3& a, const Vec3& b)
    {
        const vtkIdType ids[2] = {add(a), add(b)};
        cells->InsertNextCell(2, ids);
    }
    void polyline(const QVector<Vec3>& line)
    {
        if (line.size() < 2) {
            return;
        }
        std::vector<vtkIdType> ids;
        ids.reserve(static_cast<std::size_t>(line.size()));
        for (const auto& p : line) {
            ids.push_back(add(p));
        }
        cells->InsertNextCell(static_cast<vtkIdType>(ids.size()), ids.data());
    }
    void dashed(const Vec3& a, const Vec3& b, double dash, double gap)
    {
        const Vec3 delta = forcedisplay::sub(b, a);
        const double length = forcedisplay::norm(delta);
        if (!(length > 0.0) || !(dash > 0.0)) {
            return;
        }
        const Vec3 direction = forcedisplay::scale(delta, 1.0 / length);
        for (double t = 0.0; t < length; t += dash + gap) {
            segment(forcedisplay::add(a, forcedisplay::scale(direction, t)),
                forcedisplay::add(a, forcedisplay::scale(direction, std::min(t + dash, length))));
        }
    }
    void cone(const Vec3& tip, const Vec3& direction, double length, double radius)
    {
        const Vec3 u = anyPerpendicular(direction);
        const Vec3 v = forcedisplay::cross(direction, u);
        const Vec3 base = forcedisplay::sub(tip, forcedisplay::scale(direction, length));
        const vtkIdType tipId = add(tip);
        constexpr int sides = 10;
        vtkIdType ring[sides];
        for (int i = 0; i < sides; ++i) {
            const double angle = 2.0 * pi * i / sides;
            ring[i] = add(forcedisplay::add(base,
                forcedisplay::add(forcedisplay::scale(u, radius * std::cos(angle)),
                    forcedisplay::scale(v, radius * std::sin(angle)))));
        }
        for (int i = 0; i < sides; ++i) {
            const vtkIdType triangle[3] = {tipId, ring[i], ring[(i + 1) % sides]};
            cells->InsertNextCell(3, triangle);
        }
    }
    void storeLines(vtkPolyData* data)
    {
        data->Initialize();
        data->SetPoints(points);
        data->SetLines(cells);
        data->Modified();
    }
    void storePolys(vtkPolyData* data)
    {
        data->Initialize();
        data->SetPoints(points);
        data->SetPolys(cells);
        data->Modified();
    }
};

vtkSmartPointer<vtkActor> makeOverlayActor(vtkPolyData* data)
{
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(data);
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->LightingOff();
    actor->PickableOff();
    return actor;
}

// Umgebung: Wasser, Himmel und Raster.
constexpr int waterRings = 56;
constexpr int waterSegments = 112;
constexpr double horizonFactor = 90.0;
constexpr int gridHalfLines = 40;
const double waterColor[3] = {0.07, 0.30, 0.42};
const double deepColor[3] = {0.015, 0.06, 0.10};
const double horizonColor[3] = {0.46, 0.57, 0.66};

double smoothstep(double edge0, double edge1, double x)
{
    const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

double mix(double a, double b, double t)
{
    return a + (b - a) * t;
}

// 1, 2 oder 5 mal Zehnerpotenz, nicht größer als value.
double niceStep(double value)
{
    if (!(value > 0.0)) {
        return 1.0;
    }
    const double power = std::pow(10.0, std::floor(std::log10(value)));
    const double mantissa = value / power;
    return power * (mantissa >= 5.0 ? 5.0 : mantissa >= 2.0 ? 2.0 : 1.0);
}

unsigned char toByte(double value)
{
    return static_cast<unsigned char>(std::lround(std::clamp(value, 0.0, 1.0) * 255.0));
}

vtkSmartPointer<vtkTexture> makeProceduralTexture(int kind)
{
    constexpr int size = 128;
    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions(size, size, 1);
    image->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double wave = 0.5 + 0.25 * std::sin(0.19 * x + 0.13 * std::sin(0.11 * y))
                + 0.25 * std::sin(0.07 * y - 0.17 * x);
            unsigned char rgba[4] = {255, 255, 255, 255};
            if (kind == 0) {
                const auto shade = toByte(0.96 + 0.04 * wave);
                rgba[0] = rgba[1] = rgba[2] = shade;
            } else if (kind == 1) {
                rgba[0] = 255;
                rgba[1] = toByte(0.24 + 0.18 * wave);
                rgba[2] = 255;
            } else {
                const double dx = 0.018 * std::cos(0.19 * x);
                const double dy = 0.018 * std::sin(0.17 * y);
                rgba[0] = toByte(0.5 + dx);
                rgba[1] = toByte(0.5 + dy);
                rgba[2] = 255;
            }
            auto* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x, y, 0));
            std::copy(rgba, rgba + 4, pixel);
        }
    }
    auto texture = vtkSmartPointer<vtkTexture>::New();
    texture->SetInputData(image);
    texture->InterpolateOn();
    texture->MipmapOn();
    texture->RepeatOn();
    texture->SetColorModeToDirectScalars();
    if (kind == 0) texture->UseSRGBColorSpaceOn();
    return texture;
}

vtkSmartPointer<vtkTexture> makePreviewTexture(bool cloth, int channel)
{
    constexpr int size = 256;
    // A repeat-local graphical seam, not a reconstruction of physical sail seams.
    const auto clothSeam = [](double u, double v) {
        const double distance = std::remainder(u - 0.5, 1.0);
        const double fold = std::exp(-std::pow(distance / 0.012, 2.0));
        const double stitchRow = std::exp(-std::pow((distance - 0.022) / 0.009, 2.0));
        const double stitchPhase = std::remainder(v * 16.0, 1.0);
        const double stitch = stitchRow * std::exp(-std::pow(stitchPhase / 0.23, 4.0));
        return std::array<double, 2>{fold, stitch};
    };
    const auto seamHeight = [&clothSeam](double u, double v) {
        const auto seam = clothSeam(u, v);
        return 0.7 * seam[0] + seam[1];
    };
    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions(size, size, 1);
    image->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double u = static_cast<double>(x) / size;
            const double v = static_cast<double>(y) / size;
            const double grainPhase = 2.0 * pi * (22.0 * v + 0.4 * std::sin(2.0 * pi * u));
            const double threadX = std::cos(2.0 * pi * 32.0 * u);
            const double threadY = std::cos(2.0 * pi * 32.0 * v);
            const double groove = std::pow(0.5 + 0.5 * std::cos(2.0 * pi * 4.0 * v), 40.0);
            const double detail = cloth ? 0.5 + 0.25 * threadX + 0.25 * threadY
                                        : 0.5 + 0.5 * std::sin(grainPhase);
            unsigned char rgb[3];
            if (channel == 0) {
                if (cloth) {
                    const auto seam = clothSeam(u, v);
                    const double shade = std::clamp(
                        0.83 + 0.14 * detail - 0.12 * seam[0] + 0.08 * seam[1], 0.0, 1.0);
                    rgb[0] = toByte(shade);
                    rgb[1] = toByte(shade * 0.99);
                    rgb[2] = toByte(shade * 0.94);
                } else {
                    const double shade = (0.65 + 0.3 * detail) * (1.0 - 0.65 * groove);
                    rgb[0] = toByte(0.72 * shade);
                    rgb[1] = toByte(0.46 * shade);
                    rgb[2] = toByte(0.22 * shade);
                }
            } else if (channel == 1) {
                rgb[0] = toByte(0.92 + 0.08 * detail);
                rgb[1] = toByte(cloth ? 0.85 + 0.15 * detail : 0.55 + 0.35 * detail);
                rgb[2] = 255;
            } else {
                constexpr double step = 1.0 / size;
                const double dx = cloth ? 0.1 * std::sin(2.0 * pi * 32.0 * u)
                        - 0.18 * (seamHeight(u + step, v) - seamHeight(u - step, v))
                                        : 0.018 * std::cos(2.0 * pi * u);
                const double dy = cloth ? 0.1 * std::sin(2.0 * pi * 32.0 * v)
                        - 0.18 * (seamHeight(u, v + step) - seamHeight(u, v - step))
                                        : 0.09 * std::cos(grainPhase);
                rgb[0] = toByte(0.5 + dx);
                rgb[1] = toByte(0.5 + dy);
                rgb[2] = 252;
            }
            auto* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x, y, 0));
            std::copy(rgb, rgb + 3, pixel);
        }
    }
    auto texture = vtkSmartPointer<vtkTexture>::New();
    texture->SetInputData(image);
    texture->InterpolateOn();
    texture->MipmapOn();
    texture->RepeatOn();
    if (channel == 0) texture->UseSRGBColorSpaceOn();
    return texture;
}

vtkSmartPointer<vtkTexture> makeWaterNormalTexture()
{
    constexpr int size = 256;
    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions(size, size, 1);
    image->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double a = 0.21 * std::sin(2.0 * pi * x / size)
                + 0.09 * std::sin(4.0 * pi * (x + y) / size);
            const double b = 0.19 * std::cos(2.0 * pi * y / size)
                + 0.08 * std::sin(4.0 * pi * (x - y) / size);
            const unsigned char rgb[3] = {toByte(0.5 + a), toByte(0.5 + b), 246};
            auto* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x, y, 0));
            std::copy(rgb, rgb + 3, pixel);
        }
    }
    auto texture = vtkSmartPointer<vtkTexture>::New();
    texture->SetInputData(image);
    texture->InterpolateOn();
    texture->MipmapOn();
    texture->RepeatOn();
    texture->SetColorModeToDirectScalars();
    return texture;
}

void addPlanarTextureCoordinates(vtkPolyData* mesh)
{
    double bounds[6];
    mesh->GetBounds(bounds);
    const double extent[3] = {std::max(bounds[1] - bounds[0], 1e-9),
        std::max(bounds[3] - bounds[2], 1e-9), std::max(bounds[5] - bounds[4], 1e-9)};
    int axes[3] = {0, 1, 2};
    std::sort(axes, axes + 3, [&extent](int a, int b) { return extent[a] > extent[b]; });
    auto coords = vtkSmartPointer<vtkFloatArray>::New();
    coords->SetName("TextureCoordinates");
    coords->SetNumberOfComponents(2);
    coords->SetNumberOfTuples(mesh->GetNumberOfPoints());
    for (vtkIdType i = 0; i < mesh->GetNumberOfPoints(); ++i) {
        double point[3];
        mesh->GetPoint(i, point);
        const double u = (point[axes[0]] - bounds[2 * axes[0]]) / proceduralTexturePeriod;
        const double v = (point[axes[1]] - bounds[2 * axes[1]]) / proceduralTexturePeriod;
        coords->SetTuple2(i, u, v);
    }
    mesh->GetPointData()->SetTCoords(coords);
}

vtkSmartPointer<vtkTransform> reflectedPoseTransform(const forcedisplay::Pose& pose)
{
    const auto poseMatrix = poseTransform(pose);
    vtkNew<vtkMatrix4x4> reflection;
    reflection->Identity();
    reflection->SetElement(2, 2, -1.0);
    vtkNew<vtkMatrix4x4> matrix;
    vtkMatrix4x4::Multiply4x4(reflection, poseMatrix->GetMatrix(), matrix);
    auto transform = vtkSmartPointer<vtkTransform>::New();
    transform->SetMatrix(matrix);
    return transform;
}

// Äquirektangulärer Himmel (Z oben): dunkler Zenit, heller Dunst am
// Horizont, gedämpfte Sonne, darunter dunkles Wasser. Dient als Skybox und
// als Umgebungslicht für Spiegelungen.
vtkSmartPointer<vtkTexture> makeSkyTexture(bool inspection = false)
{
    constexpr int width = 1024;
    constexpr int height = 512;
    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions(width, height, 1);
    image->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
    const Vec3 sun = normalized({-0.55, -0.62, 0.56});
    for (int j = 0; j < height; ++j) {
        const double elevation = ((j + 0.5) / height - 0.5) * pi;
        for (int i = 0; i < width; ++i) {
            const double azimuth = ((i + 0.5) / width) * 2.0 * pi;
            const Vec3 direction{std::cos(elevation) * std::cos(azimuth),
                std::cos(elevation) * std::sin(azimuth), std::sin(elevation)};
            double rgb[3];
            if (inspection) {
                rgb[0] = 198.0 / 255.0;
                rgb[1] = 149.0 / 255.0;
                rgb[2] = 165.0 / 255.0;
            } else if (elevation >= 0.0) {
                const double t = std::pow(elevation / (0.5 * pi), 0.45);
                const double zenith[3] = {0.07, 0.15, 0.27};
                for (int c = 0; c < 3; ++c) {
                    rgb[c] = mix(horizonColor[c], zenith[c], t);
                }
                const double sunDot = std::max(0.0, forcedisplay::dot(direction, sun));
                const double glow = 0.55 * std::pow(sunDot, 64.0) + 0.18 * std::pow(sunDot, 6.0);
                rgb[0] += glow * 1.0;
                rgb[1] += glow * 0.93;
                rgb[2] += glow * 0.80;
            } else {
                const double t = std::pow(-elevation / (0.5 * pi), 0.35);
                for (int c = 0; c < 3; ++c) {
                    rgb[c] = mix(mix(horizonColor[c], waterColor[c], 0.55), deepColor[c], t);
                }
            }
            auto* pixel = static_cast<unsigned char*>(image->GetScalarPointer(i, j, 0));
            for (int c = 0; c < 3; ++c) {
                pixel[c] = toByte(rgb[c]);
            }
        }
    }
    auto texture = vtkSmartPointer<vtkTexture>::New();
    texture->SetInputData(image);
    texture->InterpolateOn();
    texture->MipmapOn();
    texture->RepeatOn();
    texture->SetColorModeToDirectScalars();
    return texture;
}

// Polarnetz: Mittelpunkt plus waterRings Ringe mit waterSegments Punkten.
vtkSmartPointer<vtkPolyData> makePolarMesh(bool withNormals)
{
    auto mesh = vtkSmartPointer<vtkPolyData>::New();
    vtkNew<vtkPoints> points;
    const vtkIdType count = 1 + static_cast<vtkIdType>(waterRings) * waterSegments;
    points->SetNumberOfPoints(count);
    for (vtkIdType i = 0; i < count; ++i) {
        points->SetPoint(i, 0.0, 0.0, 0.0);
    }
    vtkNew<vtkCellArray> cells;
    auto ring = [](int r, int s) {
        return static_cast<vtkIdType>(1 + r * waterSegments + (s % waterSegments));
    };
    for (int s = 0; s < waterSegments; ++s) {
        const vtkIdType triangle[3] = {0, ring(0, s), ring(0, s + 1)};
        cells->InsertNextCell(3, triangle);
    }
    for (int r = 0; r + 1 < waterRings; ++r) {
        for (int s = 0; s < waterSegments; ++s) {
            const vtkIdType quad[4] = {ring(r, s), ring(r + 1, s), ring(r + 1, s + 1), ring(r, s + 1)};
            cells->InsertNextCell(4, quad);
        }
    }
    mesh->SetPoints(points);
    mesh->SetPolys(cells);
    vtkNew<vtkFloatArray> textureCoordinates;
    textureCoordinates->SetNumberOfComponents(2);
    textureCoordinates->SetNumberOfTuples(count);
    textureCoordinates->Fill(0.0);
    mesh->GetPointData()->SetTCoords(textureCoordinates);
    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetName("rgba");
    colors->SetNumberOfComponents(4);
    colors->SetNumberOfTuples(count);
    colors->Fill(0.0);
    mesh->GetPointData()->SetScalars(colors);
    if (withNormals) {
        vtkNew<vtkFloatArray> normals;
        normals->SetNumberOfComponents(3);
        normals->SetNumberOfTuples(count);
        for (vtkIdType i = 0; i < count; ++i) {
            normals->SetTuple3(i, 0.0, 0.0, 1.0);
        }
        mesh->GetPointData()->SetNormals(normals);
    }
    return mesh;
}

vtkSmartPointer<vtkActor> makeColoredActor(vtkPolyData* data)
{
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(data);
    mapper->ScalarVisibilityOn();
    mapper->SetColorModeToDirectScalars();
    mapper->SetScalarModeToUsePointData();
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->PickableOff();
    actor->UseBoundsOff();
    return actor;
}

// Setzt das Material; liefert die Grunddeckkraft der Oberfläche. Die
// Grundfarbe muss bereits gesetzt sein (Klarlack, Kantenfarbe).
double applySurface(vtkProperty* property, DisplayPart::Surface surface)
{
    double color[3];
    property->GetColor(color);
    property->SetInterpolationToPBR();
    property->SetMetallic(0.0);
    property->SetRoughness(0.5);
    switch (surface) {
    case DisplayPart::Surface::Metal:
    case DisplayPart::Surface::BrushedMetal:
        property->SetMetallic(1.0);
        property->SetRoughness(0.28);
        break;
    case DisplayPart::Surface::Cloth:
        property->SetRoughness(0.95);
        break;
    case DisplayPart::Surface::Wood:
        property->SetRoughness(0.75);
        break;
    case DisplayPart::Surface::Glossy:
        property->SetRoughness(0.22);
        break;
    case DisplayPart::Surface::Matte:
        property->SetRoughness(0.82);
        break;
    case DisplayPart::Surface::Default:
        break;
    case DisplayPart::Surface::Chrome:
        // Spiegelnd: zeigt Himmel und Wasser über die Umgebungsbeleuchtung.
        property->SetMetallic(1.0);
        property->SetRoughness(0.1);
        break;
    case DisplayPart::Surface::Lacquer:
        // Bonbonlack: farbige Basis unter klarer, hochglänzender Lackschicht.
        property->SetRoughness(0.45);
        property->SetCoatStrength(1.0);
        property->SetCoatRoughness(0.02);
        property->SetCoatIOR(1.6);
        property->SetCoatColor(1.0, 1.0, 1.0);
        break;
    case DisplayPart::Surface::Glass:
        property->SetRoughness(0.02);
        property->SetBaseIOR(1.9);
        property->SetEdgeVisibility(true);
        property->SetEdgeColor(0.5 + 0.5 * color[0], 0.5 + 0.5 * color[1], 0.5 + 0.5 * color[2]);
        property->SetLineWidth(2.5);
        return 0.5;
    case DisplayPart::Surface::Glow:
        // Neon: unbeleuchtet in voller Farbe, helle Kanten zeigen die Form.
        property->SetInterpolationToFlat();
        property->LightingOff();
        property->SetEdgeVisibility(true);
        property->SetEdgeColor(0.9 + 0.1 * color[0], 0.9 + 0.1 * color[1], 0.9 + 0.1 * color[2]);
        property->SetLineWidth(3.0);
        break;
    case DisplayPart::Surface::Iridescent:
        // Farbumschlag: metallisch, zum Rand hin Komplementärfarbe.
        property->SetMetallic(1.0);
        property->SetRoughness(0.18);
        property->SetEdgeTint(1.0 - color[0], 1.0 - color[1], 1.0 - color[2]);
        property->SetCoatStrength(0.6);
        property->SetCoatRoughness(0.05);
        break;
    case DisplayPart::Surface::Faceted:
        // Flächennormalen ohne Glättung, dunkle Kanten.
        property->SetInterpolationToFlat();
        property->SetRoughness(0.35);
        property->SetEdgeVisibility(true);
        property->SetEdgeColor(0.08, 0.08, 0.1);
        property->SetLineWidth(2.5);
        break;
    case DisplayPart::Surface::Wireframe:
        property->SetRepresentationToWireframe();
        property->SetMetallic(1.0);
        property->SetRoughness(0.15);
        property->RenderLinesAsTubesOn();
        property->SetLineWidth(6.0);
        break;
    case DisplayPart::Surface::Pearl:
        // Perlmutt: helle Basis, farbig getönter Klarlack.
        property->SetColor(0.97, 0.96, 0.94);
        property->SetRoughness(0.3);
        property->SetCoatStrength(1.0);
        property->SetCoatRoughness(0.08);
        property->SetCoatColor(color);
        break;
    case DisplayPart::Surface::Invisible:
        break;
    }
    return 1.0;
}

DisplayPart::Surface effectiveSurface(DisplayPart::Surface original, int preview)
{
    switch (preview) {
    case 1: return DisplayPart::Surface::Lacquer;
    case 2: return DisplayPart::Surface::Cloth;
    case 3: return DisplayPart::Surface::Wood;
    case 4: return DisplayPart::Surface::BrushedMetal;
    default: return original;
    }
}

void copyMaterial(vtkProperty* target, vtkProperty* source)
{
    target->DeepCopy(source);
    // VTK 9.3 DeepCopy copies raw texture pointers without Register(), while
    // RemoveAllTextures() unregisters them. Balance ownership for every copy.
    for (const auto& entry : target->GetAllTextures()) {
        entry.second->Register(target);
    }
}

void setMaterialTexture(vtkProperty* property, const char* name, vtkTexture* texture)
{
    if (property->GetTexture(name) == texture) return;
    property->RemoveTexture(name);
    if (texture) property->SetTexture(name, texture);
}

void setActorEnvironmentLighting(vtkActor* actor, bool enabled)
{
    auto* shader = actor->GetShaderProperty();
    const std::pair<const char*, const char*> replacements[] = {
        {"  vec3 iblSpecular = prefilteredSpecularColor * specularBrdf;",
            "  vec3 iblSpecular = vec3(0.0);"},
        {"  vec3 iblDiffuse = (1.0 - F0) * (1.0 - metallic) * irradiance * albedo;",
            "  vec3 iblDiffuse = vec3(0.0);"},
        {"  color += iblSpecularClearCoat;", "  color += vec3(0.0);"}
    };
    for (const auto& replacement : replacements) {
        shader->ClearFragmentShaderReplacement(replacement.first, false);
        if (!enabled) {
            shader->AddFragmentShaderReplacement(
                replacement.first, false, replacement.second, false);
        }
    }
}

} // namespace

VtkSceneWidget::VtkSceneWidget(QWidget* parent)
    : QWidget(parent)
    , vtkWidget_(new QVTKOpenGLNativeWidget(this))
    , renderWindow_(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , renderer_(vtkSmartPointer<SceneRenderer>::New())
    , overlay_(vtkSmartPointer<vtkRenderer>::New())
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->addWidget(vtkWidget_);
    renderWindow_->SetNumberOfLayers(2);
    renderWindow_->AddRenderer(renderer_);
    renderWindow_->AddRenderer(overlay_);
    renderWindow_->SetMultiSamples(0);
    vtkWidget_->setRenderWindow(renderWindow_);
    inspectionPicker_ = vtkSmartPointer<vtkCellPicker>::New();
    inspectionPicker_->SetTolerance(0.0005);
    navigation_ = new navigation::Overlay(vtkWidget_);
    navigation_->installEventFilter(this);
    for (auto* child : navigation_->findChildren<QWidget*>())
        if (child->objectName() != QStringLiteral("navigationDragHandle")
            && child->objectName() != QStringLiteral("navigationSizeGrip"))
            child->installEventFilter(this);
    viewName_ = new QLabel(vtkWidget_);
    viewName_->setObjectName(QStringLiteral("viewName"));
    viewName_->setAttribute(Qt::WA_TransparentForMouseEvents);
    viewName_->setStyleSheet(QStringLiteral(
        "background:rgba(11,20,32,210); color:#e8eef5; padding:2px 4px;"
        "font-family:'Segoe UI'; font-size:11px;"));
    viewName_->hide();
    centerLabel_ = vtkSmartPointer<vtkBillboardTextActor3D>::New();
    centerLabel_->SetInput("S");
    centerLabel_->PickableOff();
    centerLabel_->GetTextProperty()->SetColor(0.1, 0.3, 0.8);
    centerLabel_->GetTextProperty()->BoldOn();
    centerLabel_->VisibilityOff();
    buoyancyLabel_ = vtkSmartPointer<vtkBillboardTextActor3D>::New();
    buoyancyLabel_->SetInput("A");
    buoyancyLabel_->PickableOff();
    buoyancyLabel_->GetTextProperty()->BoldOn();
    buoyancyLabel_->VisibilityOff();
    defaultViewButton_ = new QPushButton(QStringLiteral("Defaultansicht"), vtkWidget_);
    defaultViewButton_->setObjectName(QStringLiteral("defaultView"));
    defaultViewButton_->setStyleSheet(QStringLiteral(
        "QPushButton { background:#1b2b3d; color:#e8eef5; border:1px solid #344c61;"
        "border-radius:3px; padding:2px; font-family:'Segoe UI'; font-size:10px; }"));
    defaultViewButton_->hide();
    connect(defaultViewButton_, &QPushButton::clicked, this, &VtkSceneWidget::resetOrthographicCamera);
    renderer_->SetBackground(0.035, 0.065, 0.11);
    renderer_->SetBackground2(0.08, 0.16, 0.23);
    renderer_->GradientBackgroundOn();
    renderer_->GetActiveCamera()->SetPosition(0.0, -1.0, 0.0);
    renderer_->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
    // Keep analytical helpers out of scene passes (the shadow baker enumerates
    // all renderer props, ignoring the filtered render-state prop array).
    overlay_->SetLayer(1);
    overlay_->SetActiveCamera(renderer_->GetActiveCamera());
    overlay_->InteractiveOff();
    overlay_->PreserveDepthBufferOff();
    renderObserver_ = renderWindow_->AddObserver(
        vtkCommand::StartEvent, this, &VtkSceneWidget::onRenderStart);
    vtkWidget_->installEventFilter(this);
    leverLines_ = vtkSmartPointer<vtkPolyData>::New();
    leverActor_ = makeOverlayActor(leverLines_);
    lateralLines_ = vtkSmartPointer<vtkPolyData>::New();
    lateralActor_ = makeOverlayActor(lateralLines_);
    lateralLabel_ = vtkSmartPointer<vtkBillboardTextActor3D>::New();
    lateralLabel_->SetInput("LDP");
    lateralLabel_->PickableOff();
    lateralLabel_->GetTextProperty()->SetFontSize(16);
    lateralLabel_->GetTextProperty()->BoldOn();
    sailPoint_ = vtkSmartPointer<vtkPolyData>::New();
    sailActor_ = makeOverlayActor(sailPoint_);
    sailLabel_ = vtkSmartPointer<vtkBillboardTextActor3D>::New();
    sailLabel_->SetInput("SDP (Ersatz)");
    sailLabel_->PickableOff();
    sailLabel_->GetTextProperty()->SetFontSize(16);
    sailLabel_->GetTextProperty()->BoldOn();
    courseArrowLines_ = vtkSmartPointer<vtkPolyData>::New();
    courseArrowHead_ = vtkSmartPointer<vtkPolyData>::New();
    courseArrowActor_ = makeOverlayActor(courseArrowLines_);
    courseArrowHeadActor_ = makeOverlayActor(courseArrowHead_);
    overlay_->AddActor(courseArrowActor_);
    overlay_->AddActor(courseArrowHeadActor_);
    setActorColor(courseArrowActor_, QStringLiteral("#73f3d4"));
    setActorColor(courseArrowHeadActor_, QStringLiteral("#73f3d4"));
    courseArrowActor_->VisibilityOff();
    courseArrowHeadActor_->VisibilityOff();
    for (std::size_t i = 0; i < windVisuals_.size(); ++i) {
        auto& visual = windVisuals_[i];
        visual.lines = vtkSmartPointer<vtkPolyData>::New();
        visual.head = vtkSmartPointer<vtkPolyData>::New();
        visual.lineActor = makeOverlayActor(visual.lines);
        visual.headActor = makeOverlayActor(visual.head);
        overlay_->AddActor(visual.lineActor);
        overlay_->AddActor(visual.headActor);
        const auto color = i == 0 ? QStringLiteral("#57b6ff") : QStringLiteral("#ff9a32");
        setActorColor(visual.lineActor, color);
        setActorColor(visual.headActor, color);
        visual.lineActor->GetProperty()->SetLineWidth(i == 0 ? 6.f : 3.f);
        visual.lineActor->VisibilityOff();
        visual.headActor->VisibilityOff();
    }

    // Licht: Lichtset statt Einzellicht, Kantenglättung, Tiefenschälung für
    // Transparenz, Umgebungsverdeckung und Himmel als Umgebungslicht.
    renderer_->AutomaticLightCreationOff();
    renderer_->RemoveAllLights();
    baseLights_ = vtkSmartPointer<vtkLightKit>::New();
    baseLights_->SetKeyLightIntensity(0.85);
    baseLights_->AddLightsToRenderer(renderer_);
    sunLight_ = vtkSmartPointer<vtkLight>::New();
    sunLight_->SetLightTypeToSceneLight();
    sunLight_->SetPosition(-55.0, -62.0, 56.0);
    sunLight_->SetFocalPoint(0.0, 0.0, 0.0);
    sunLight_->SetPositional(false);
    sunLight_->SetIntensity(0.9);
    sunLight_->SetShadowAttenuation(0.65f);
    renderer_->AddLight(sunLight_);
    fillLight_ = vtkSmartPointer<vtkLight>::New();
    fillLight_->SetLightTypeToSceneLight();
    fillLight_->SetPosition(14.0, -8.0, 9.0);
    fillLight_->SetFocalPoint(0.0, 0.0, 0.0);
    fillLight_->SetPositional(false);
    fillLight_->SetIntensity(0.28);
    renderer_->AddLight(fillLight_);
    renderer_->UseFXAAOn();
    renderWindow_->SetAlphaBitPlanes(1);
    renderer_->UseDepthPeelingOn();
    renderer_->SetMaximumNumberOfPeels(8);
    renderer_->SetOcclusionRatio(0.05);
    renderer_->UseSSAOOn();
    renderer_->SetSSAOKernelSize(48);
    renderer_->SSAOBlurOn();
    skyTexture_ = makeSkyTexture();
    proceduralSkyTexture_ = skyTexture_;
    renderer_->SetEnvironmentTexture(skyTexture_);
    renderer_->SetEnvironmentUp(0.0, 0.0, 1.0);
    renderer_->SetEnvironmentRight(1.0, 0.0, 0.0);
    renderer_->UseImageBasedLightingOn();
    skybox_ = vtkSmartPointer<vtkSkybox>::New();
    skybox_->SetTexture(skyTexture_);
    skybox_->SetProjectionToSphere();
    skybox_->SetFloorPlane(0.0, 0.0, 1.0, 0.0);
    skybox_->SetFloorRight(1.0, 0.0, 0.0);
    skybox_->PickableOff();
    skybox_->UseBoundsOff();
    proceduralColorTexture_ = makeProceduralTexture(0);
    proceduralOrmTexture_ = makeProceduralTexture(1);
    proceduralNormalTexture_ = makeProceduralTexture(2);
    woodColorTexture_ = makePreviewTexture(false, 0);
    woodOrmTexture_ = makePreviewTexture(false, 1);
    woodNormalTexture_ = makePreviewTexture(false, 2);
    clothColorTexture_ = makePreviewTexture(true, 0);
    clothOrmTexture_ = makePreviewTexture(true, 1);
    clothNormalTexture_ = makePreviewTexture(true, 2);
    waterNormalTexture_ = makeWaterNormalTexture();
    waterNormalTransform_ = vtkSmartPointer<vtkTransform>::New();
    waterNormalTexture_->SetTransform(waterNormalTransform_);

    waterMesh_ = makePolarMesh(true);
    waterDisplay_ = vtkSmartPointer<vtkPolyData>::New();
    waterDisplay_->ShallowCopy(waterMesh_);
    waterTangents_ = vtkSmartPointer<vtkPolyDataTangents>::New();
    waterTriangles_ = vtkSmartPointer<vtkTriangleFilter>::New();
    waterTriangles_->SetInputData(waterDisplay_);
    waterTangents_->SetInputConnection(waterTriangles_->GetOutputPort());
    auto waterMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    waterMapper->SetInputConnection(waterTangents_->GetOutputPort());
    waterMapper->ScalarVisibilityOn();
    waterMapper->SetColorModeToDirectScalars();
    waterMapper->SetScalarModeToUsePointData();
    waterActor_ = vtkSmartPointer<vtkActor>::New();
    waterActor_->SetMapper(waterMapper);
    waterActor_->PickableOff();
    waterActor_->UseBoundsOff();
    applySurface(waterActor_->GetProperty(), DisplayPart::Surface::Glossy);
    waterActor_->GetProperty()->SetRoughness(0.12);
    waterLevelLine_ = vtkSmartPointer<vtkPolyData>::New();
    waterLevelActor_ = makeOverlayActor(waterLevelLine_);
    waterLevelActor_->GetProperty()->SetColor(0.38, 0.60, 0.74);
    waterLevelActor_->PickableOff();
    waterLevelActor_->UseBoundsOff();
    waterLevelActor_->VisibilityOff();
    deepMesh_ = makePolarMesh(false);
    deepActor_ = makeColoredActor(deepMesh_);
    deepActor_->GetProperty()->LightingOff();
    gridMesh_ = vtkSmartPointer<vtkPolyData>::New();
    gridActor_ = makeColoredActor(gridMesh_);
    gridActor_->GetProperty()->LightingOff();
    trailMesh_ = vtkSmartPointer<vtkPolyData>::New();
    trailActor_ = makeColoredActor(trailMesh_);
    trailActor_->GetProperty()->LightingOff();
    trailActor_->GetProperty()->SetLineWidth(2.5);
    trailActor_->VisibilityOff();
    motionMesh_ = vtkSmartPointer<vtkPolyData>::New();
    motionActor_ = makeColoredActor(motionMesh_);
    motionActor_->GetProperty()->LightingOff();
    motionActor_->GetProperty()->SetLineWidth(2.0);
    wakeMesh_ = vtkSmartPointer<vtkPolyData>::New();
    wakeActor_ = makeColoredActor(wakeMesh_);
    wakeActor_->GetProperty()->LightingOff();
    wakeActor_->GetProperty()->SetLineWidth(1.5);
    addEnvironmentActors();

    // Achsenkreuz unten links in eigener, kleiner Ansicht.
    axesRenderer_ = vtkSmartPointer<vtkRenderer>::New();
    axesRenderer_->SetLayer(1);
    axesRenderer_->InteractiveOff();
    axesRenderer_->GetActiveCamera()->ParallelProjectionOn();
    axesRenderer_->GetActiveCamera()->SetParallelScale(1.25);
    axesActor_ = vtkSmartPointer<vtkAxesActor>::New();
    axesActor_->SetTotalLength(1.0, 1.0, 1.0);
    axesActor_->SetShaftTypeToCylinder();
    axesActor_->SetCylinderRadius(0.04);
    axesActor_->SetConeRadius(0.45);
    axesActor_->SetXAxisLabelText("x");
    axesActor_->SetYAxisLabelText("y");
    axesActor_->SetZAxisLabelText("z");
    for (auto* caption : {axesActor_->GetXAxisCaptionActor2D(), axesActor_->GetYAxisCaptionActor2D(),
             axesActor_->GetZAxisCaptionActor2D()}) {
        caption->GetCaptionTextProperty()->SetColor(0.85, 0.92, 1.0);
        caption->GetCaptionTextProperty()->ShadowOff();
        caption->GetCaptionTextProperty()->ItalicOff();
    }
    axesActor_->PickableOff();
    axesRenderer_->AddActor(axesActor_);
    renderWindow_->AddRenderer(axesRenderer_);
    graphicsProfile_ = graphics::defaults(0);
    applyGraphicsProfile();
}

void VtkSceneWidget::addEnvironmentActors()
{
    if (inspectionAxes_) renderer_->AddActor(inspectionAxes_);
    renderer_->AddActor(skybox_);
    renderer_->AddActor(deepActor_);
    renderer_->AddActor(waterActor_);
    renderer_->AddActor(waterLevelActor_);
    renderer_->AddActor(gridActor_);
    renderer_->AddActor(trailActor_);
    renderer_->AddActor(motionActor_);
    renderer_->AddActor(wakeActor_);
}

VtkSceneWidget::~VtkSceneWidget()
{
    renderWindow_->RemoveObserver(renderObserver_);
}

void VtkSceneWidget::setUpdatesActive(bool active)
{
    updatesActive_ = active;
    if (auto* interactor = renderWindow_->GetInteractor())
        interactor->SetEnableRender(active);
    vtkWidget_->setUpdatesEnabled(active);
}

void VtkSceneWidget::updateDockedNavigation(const QJsonObject& state,
    const forcedisplay::ForceState& forces)
{
    const auto pose = forcedisplay::readPose(state);
    const auto view = cameraView();
    auto offset = forcedisplay::sub(view.position, view.focalPoint);
    if (bodyFixedCamera_)
        offset = forcedisplay::toWorldVector(pose, inverseRotate(cameraPose_, offset));
    auto values = navigation::derive(pose, offset,
        readStateVector(state, QStringLiteral("geschwindigkeit_welt_m_s")));
    values.courseRateRadS = forces.courseRateRadS;
    values.courseRateStatus = forces.courseRateStatus;
    values.approximate = !forces.stationary.isEmpty()
        && !forces.stationary.value(QStringLiteral("konvergiert")).toBool();
    readWindNavigation(state, values);
    navigation_->setValues(values);
}

void VtkSceneWidget::renderScene()
{
    if (updatesActive_) renderWindow_->Render();
}

std::shared_ptr<const DisplayMeshData> VtkSceneWidget::prepareMesh(
    const std::shared_ptr<DisplayMeshCache>& cache, const QJsonArray& matrix)
{
    if (matrix.isEmpty()) return cache->base;
    if (cache->transformed && cache->matrix == matrix) return cache->transformed;
    auto prepared = std::make_shared<DisplayMeshData>();
    prepared->mesh = vtkSmartPointer<vtkPolyData>::New();
    prepared->mesh->DeepCopy(cache->base->mesh);
    prepared->material = vtkSmartPointer<vtkPolyData>::New();
    prepared->material->DeepCopy(cache->base->material);
    applyCoreMatrix(prepared->mesh, matrix);
    applyCoreMatrix(prepared->material, matrix);
    prepared->cutMesh = prepared->mesh->GetNumberOfPolys() == 0
        ? prepared->mesh : outwardNormals(prepared->mesh);
    cache->matrix = matrix;
    cache->transformed = prepared;
    ++workCounters_.meshPreparations;
    return prepared;
}

void VtkSceneWidget::clearModel()
{
    waterLevelActor_->VisibilityOff();
    orthographicScale_ = 0.0;
    orthographicPan_ = {};
    cameraDragButton_ = Qt::NoButton;
    resetMotionHistory();
    renderer_->RemoveAllViewProps();
    inspectionPreviewActors_.clear();
    inspectionGridLabels_.clear();
    overlay_->RemoveAllViewProps();
    overlay_->AddActor(courseArrowActor_);
    overlay_->AddActor(courseArrowHeadActor_);
    for (auto& visual : windVisuals_) {
        overlay_->AddActor(visual.lineActor);
        overlay_->AddActor(visual.headActor);
        visual.lineActor->VisibilityOff();
        visual.headActor->VisibilityOff();
    }
    addEnvironmentActors();
    bodies_.clear();
    dryLoops_.clear();
    visuals_.clear();
    arcBasis_.clear();
    scaled_.clear();
    screenShapes_.clear();
    track_.clear();
    trailActor_->VisibilityOff();
    forces_ = {};
    hasState_ = false;
    cameraPose_ = {};
    selected_.clear();
    forceReference_ = 0.0;
    momentReference_ = 0.0;
    geometryDirty_ = true;
    renderScene();
}

bool VtkSceneWidget::setModel(const QVector<DisplayPart>& parts, QString* error)
{
    try {
        clearModel();
        if (parts.isEmpty()) {
            throw std::runtime_error("Keine darstellbare Modellgeometrie.");
        }
        for (qsizetype i = 0; i < parts.size(); ++i) {
            const auto& part = parts.at(i);
            auto cache = part.preparation;
            if (cache->source != part.mesh || cache->sourceTime != part.mesh->GetMTime()) {
                auto base = std::make_shared<DisplayMeshData>();
                base->mesh = part.mesh;
                base->material = vtkSmartPointer<vtkPolyData>::New();
                base->material->DeepCopy(part.mesh);
                addPlanarTextureCoordinates(base->material);
                base->cutMesh = part.mesh->GetNumberOfPolys() == 0
                    ? part.mesh : outwardNormals(part.mesh);
                cache->source = part.mesh;
                cache->sourceTime = part.mesh->GetMTime();
                cache->base = base;
                cache->transformed.reset();
                ++workCounters_.meshPreparations;
            }
            const auto bodyMesh = cache->base->mesh;
            const auto materialMesh = cache->base->material;
            auto triangles = vtkSmartPointer<vtkTriangleFilter>::New();
            triangles->SetInputData(materialMesh);
            auto normals = vtkSmartPointer<vtkPolyDataNormals>::New();
            normals->SetInputConnection(triangles->GetOutputPort());
            normals->SetFeatureAngle(35.0);
            normals->SplittingOn();
            normals->ConsistencyOn();
            auto tangents = vtkSmartPointer<vtkPolyDataTangents>::New();
            tangents->SetInputConnection(normals->GetOutputPort());
            auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            if (bodyMesh->GetNumberOfPolys() == 0) mapper->SetInputData(bodyMesh);
            else mapper->SetInputConnection(tangents->GetOutputPort());
            auto bodyActor = vtkSmartPointer<vtkActor>::New();
            bodyActor->SetMapper(mapper);
            double baseColor[3] = {i % 2 == 0 ? 0.82 : 0.43,
                i % 2 == 0 ? 0.87 : 0.72, i % 2 == 0 ? 0.91 : 0.82};
            if (part.hasColor) {
                std::copy(part.color, part.color + 3, baseColor);
            }
            bodyActor->GetProperty()->SetColor(baseColor[0], baseColor[1], baseColor[2]);
            const double opacity = applySurface(bodyActor->GetProperty(), part.surface);
            bodyActor->GetProperty()->BackfaceCullingOff();
            if (bodyMesh->GetNumberOfPolys() == 0) {
                bodyActor->GetProperty()->SetPointSize(7.0);
                bodyActor->GetProperty()->RenderPointsAsSpheresOn();
            }
            renderer_->AddActor(bodyActor);

            auto plane = vtkSmartPointer<vtkPlane>::New();
            auto cutter = vtkSmartPointer<vtkCutter>::New();
            cutter->SetInputData(cache->base->cutMesh);
            cutter->SetCutFunction(plane);
            auto waterlineData = vtkSmartPointer<vtkPolyData>::New();
            auto waterlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            waterlineMapper->SetInputData(waterlineData);
            auto waterlineActor = vtkSmartPointer<vtkActor>::New();
            waterlineActor->SetMapper(waterlineMapper);
            waterlineActor->GetProperty()->SetColor(0.12, 0.92, 1.0);
            waterlineActor->GetProperty()->SetLineWidth(3.0);
            waterlineActor->GetProperty()->SetLighting(false);
            renderer_->AddActor(waterlineActor);
            auto reflectionClipper = vtkSmartPointer<vtkClipPolyData>::New();
            reflectionClipper->SetInputConnection(tangents->GetOutputPort());
            reflectionClipper->SetClipFunction(plane);
            reflectionClipper->InsideOutOff();
            auto reflectionMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            reflectionMapper->SetInputConnection(reflectionClipper->GetOutputPort());
            auto reflectionActor = vtkSmartPointer<vtkActor>::New();
            reflectionActor->SetMapper(reflectionMapper);
            copyMaterial(reflectionActor->GetProperty(), bodyActor->GetProperty());
            reflectionActor->GetProperty()->SetOpacity(0.24 * opacity);
            reflectionActor->GetProperty()->SetAmbient(0.18);
            reflectionActor->GetProperty()->SetDiffuse(0.75);
            reflectionActor->GetProperty()->SetSpecular(0.35);
            reflectionActor->GetProperty()->BackfaceCullingOff();
            auto* reflectionShader = reflectionActor->GetShaderProperty();
            reflectionShader->AddVertexShaderReplacement("//VTK::PositionVC::Dec", true,
                "//VTK::PositionVC::Dec\nout float reflectionHeight;\n", false);
            reflectionShader->AddVertexShaderReplacement("//VTK::PositionVC::Impl", true,
                "//VTK::PositionVC::Impl\n"
                "reflectionHeight = dot(vertexMC.xyz - reflectionWaterOrigin, reflectionWaterNormal);\n", false);
            reflectionShader->AddFragmentShaderReplacement("//VTK::PositionVC::Dec", true,
                "//VTK::PositionVC::Dec\nin float reflectionHeight;\n", false);
            // PBR textures may replace opacity during lighting; fade the final alpha instead.
            reflectionShader->AddFragmentShaderReplacement("//VTK::TCoord::Impl", true,
                "//VTK::TCoord::Impl\n"
                "gl_FragData[0].a *= 1.0 - smoothstep(0.15, 1.5, reflectionHeight);\n", false);
            const double initialOrigin[3] = {0., 0., 0.};
            const double initialNormal[3] = {0., 0., 1.};
            reflectionShader->GetVertexCustomUniforms()->SetUniform3f("reflectionWaterOrigin", initialOrigin);
            reflectionShader->GetVertexCustomUniforms()->SetUniform3f("reflectionWaterNormal", initialNormal);
            reflectionActor->PickableOff();
            reflectionActor->UseBoundsOff();
            reflectionActor->VisibilityOff();
            renderer_->AddActor(reflectionActor);
            // Unsichtbare Teile (Luftkörper) zählen weiter für die trockenen Bereiche.
            if (part.surface == DisplayPart::Surface::Invisible) {
                bodyActor->VisibilityOff();
                waterlineActor->VisibilityOff();
            }

            BodyActor body;
            body.mesh = bodyMesh;
            body.materialMesh = materialMesh;
            body.preparation = cache;
            body.prepared = cache->base;
            body.body = bodyActor;
            body.triangles = triangles;
            body.normals = normals;
            body.tangents = tangents;
            std::copy(baseColor, baseColor + 3, body.baseColor);
            body.opacity = opacity;
            body.surface = part.surface;
            body.waterline = waterlineActor;
            body.reflection = reflectionActor;
            body.reflectionClipper = reflectionClipper;
            body.waterlineData = waterlineData;
            body.elementId = part.elementId;
            body.group = part.group;
            body.waterlineOffset = 0.004 * std::max(part.mesh->GetLength(), 0.25);
            body.localWaterPlane = plane;
            body.cutter = cutter;
            bodies_.push_back(body);
        }

        forceRenderer()->AddActor(leverActor_);
        leverActor_->VisibilityOff();

        forceRenderer()->AddActor(lateralActor_);
        forceRenderer()->AddActor(lateralLabel_);
        forceRenderer()->AddActor(sailActor_);
        forceRenderer()->AddActor(sailLabel_);
        forceRenderer()->AddActor(centerLabel_);
        forceRenderer()->AddActor(buoyancyLabel_);
        sailActor_->VisibilityOff();
        sailLabel_->VisibilityOff();
        lateralActor_->VisibilityOff();
        lateralLabel_->VisibilityOff();
        pressurePoints_ = {};
        updateWaterPlane();
        frameBodies();
        applyGraphicsProfile();
        renderScene();
        return true;
    } catch (const std::exception& exception) {
        if (error != nullptr) {
            *error = QString::fromUtf8(exception.what());
        }
        return false;
    }
}

bool VtkSceneWidget::setState(const QJsonObject& state,
    const forcedisplay::ForceState& forces, QString* error)
{
    if (!updatesActive_) return true;
    ++workCounters_.states;
    try {
        if (bodies_.empty()) {
            throw std::runtime_error("Kein Modell für die 3D-Ansicht geladen.");
        }
        validateWindSnapshot(state);
        const auto pose = forcedisplay::readPose(state);
        const auto velocityWorld = readStateVector(
            state, QStringLiteral("geschwindigkeit_welt_m_s"));
        const auto transform = poseTransform(pose);
        QHash<QString, QJsonArray> elementMatrices;
        const auto matrixValues = state.value(
            QStringLiteral("elementtransformationen_modell")).toArray();
        for (const auto& value : matrixValues) {
            const auto entry = value.toObject();
            const auto id = entry.value(QStringLiteral("element_id")).toString();
            const auto matrix = entry.value(QStringLiteral("matrix_zeilenweise")).toArray();
            if (id.isEmpty() || matrix.size() != 16 || elementMatrices.contains(id))
                throw std::runtime_error("Core-Zustand enthaelt eine ungueltige Elementtransformation.");
            elementMatrices.insert(id, matrix);
        }
        const Vec3 waterOrigin = inverseRotate(pose,
            {-pose.position[0], -pose.position[1], -pose.position[2]});
        const Vec3 waterNormal = inverseRotate(pose, {0.0, 0.0, 1.0});
        const auto reflectionTransform = reflectedPoseTransform(pose);
        // Schnittlinien je Teil zu Schleifen verketten. Trocken ist das Innere der
        // äußersten geschlossenen Schleifen nicht gefluteter Teile; dort gibt es
        // weder Wasserfläche noch Wasserlinie (z. B. Cockpit, Schwert im Rumpf).
        struct CutLoop {
            std::size_t body = 0;
            Loop2 world;
            bool closed = false;
            bool dry = false;
            double area = 0.0;
        };
        std::vector<CutLoop> loops;
        // Ein Teil gilt als geflutet, wenn ein Element seiner Gruppe vollgelaufen ist
        // (z. B. der unsichtbare Cockpit-Luftkörper in der Gruppe des Rumpfs).
        QSet<QString> floodedGroups;
        for (const auto& id : forces.flooded) {
            const auto group = elementGroups_.value(id);
            if (!group.isEmpty()) floodedGroups.insert(group);
        }
        const auto isFlooded = [&](const BodyActor& body) {
            return forces.flooded.contains(body.elementId)
                || (!body.group.isEmpty() && floodedGroups.contains(body.group));
        };
        std::vector<vtkSmartPointer<vtkPolyData>> joined(bodies_.size());
        for (std::size_t b = 0; b < bodies_.size(); ++b) {
            auto& body = bodies_[b];
            const auto matrix = elementMatrices.constFind(body.elementId);
            if (!matrixValues.isEmpty() && matrix == elementMatrices.cend())
                throw std::runtime_error("Core-Zustand ohne Transformation fuer sichtbares Element.");
            const auto prepared = prepareMesh(body.preparation,
                matrixValues.isEmpty() ? QJsonArray{} : matrix.value());
            if (body.prepared != prepared) {
                body.prepared = prepared;
                body.mesh = prepared->mesh;
                body.materialMesh = prepared->material;
                body.triangles->SetInputData(body.materialMesh);
                if (body.mesh->GetNumberOfPolys() == 0)
                    vtkPolyDataMapper::SafeDownCast(body.body->GetMapper())->SetInputData(body.mesh);
                body.cutter->SetInputData(prepared->cutMesh);
            }
            body.body->SetUserTransform(transform);
            body.waterline->SetUserTransform(transform);
            body.reflection->SetUserTransform(reflectionTransform);
            body.localWaterPlane->SetOrigin(waterOrigin[0], waterOrigin[1], waterOrigin[2]);
            body.localWaterPlane->SetNormal(waterNormal[0], waterNormal[1], waterNormal[2]);
            body.reflection->GetShaderProperty()->GetVertexCustomUniforms()->SetUniform3f(
                "reflectionWaterOrigin", waterOrigin.data());
            body.reflection->GetShaderProperty()->GetVertexCustomUniforms()->SetUniform3f(
                "reflectionWaterNormal", waterNormal.data());
            body.reflectionClipper->Modified();
            body.cutter->Update();
            auto stripper = vtkSmartPointer<vtkStripper>::New();
            stripper->SetInputData(body.cutter->GetOutput());
            stripper->JoinContiguousSegmentsOn();
            stripper->Update();
            joined[b] = stripper->GetOutput();
            vtkPoints* points = joined[b]->GetPoints();
            vtkCellArray* lines = joined[b]->GetLines();
            if (points == nullptr || lines == nullptr) continue;
            const bool flooded = isFlooded(body);
            lines->InitTraversal();
            vtkIdType count = 0;
            const vtkIdType* ids = nullptr;
            while (lines->GetNextCell(count, ids)) {
                CutLoop loop;
                loop.body = b;
                double perimeter = 0.0;
                for (vtkIdType k = 0; k < count; ++k) {
                    double p[3];
                    points->GetPoint(ids[k], p);
                    const Vec3 w = forcedisplay::toWorldPoint(pose, {p[0], p[1], p[2]});
                    if (!loop.world.empty()) {
                        perimeter += std::hypot(w[0] - loop.world.back()[0], w[1] - loop.world.back()[1]);
                    }
                    loop.world.push_back({w[0], w[1]});
                }
                if (count >= 3) {
                    const double gap = std::hypot(loop.world.front()[0] - loop.world.back()[0],
                        loop.world.front()[1] - loop.world.back()[1]);
                    loop.closed = gap <= 0.05 * perimeter;
                    if (ids[0] == ids[count - 1]) loop.world.pop_back();
                }
                loop.area = std::abs(signedArea(loop.world));
                loop.dry = loop.closed && !flooded;
                loops.push_back(std::move(loop));
            }
        }
        for (auto& loop : loops) {
            if (!loop.dry) continue;
            for (const auto& other : loops) {
                if (&other != &loop && other.closed && !isFlooded(bodies_[other.body])
                    && other.area > loop.area && loopWithin(loop.world, other.world)) {
                    loop.dry = false;
                    break;
                }
            }
        }
        dryLoops_.clear();
        for (const auto& loop : loops) {
            if (loop.dry) dryLoops_.push_back(loop.world);
        }
        std::size_t next = 0;
        for (std::size_t b = 0; b < bodies_.size(); ++b) {
            auto& body = bodies_[b];
            offsetWaterline(joined[b], body.waterlineOffset, body.waterlineData);
            auto kept = vtkSmartPointer<vtkCellArray>::New();
            if (vtkCellArray* lines = joined[b]->GetLines()) {
                lines->InitTraversal();
                vtkIdType count = 0;
                const vtkIdType* ids = nullptr;
                while (lines->GetNextCell(count, ids)) {
                    const auto& loop = loops[next++];
                    bool hidden = false;
                    for (const auto& other : loops) {
                        if (other.dry && &other != &loop && other.area > loop.area
                            && loopWithin(loop.world, other.world)) {
                            hidden = true;
                            break;
                        }
                    }
                    if (!hidden) kept->InsertNextCell(count, ids);
                }
            }
            body.waterlineData->SetLines(kept);
            body.waterlineData->Modified();
        }
        const auto previousView = cameraView();
        const auto localPosition = inverseRotate(cameraPose_,
            forcedisplay::sub(previousView.position, cameraPose_.position));
        const auto localTarget = inverseRotate(cameraPose_,
            forcedisplay::sub(previousView.focalPoint, cameraPose_.position));
        const auto localUp = inverseRotate(cameraPose_, previousView.viewUp);
        auto* camera = renderer_->GetActiveCamera();
        const auto position = forcedisplay::toWorldPoint(pose, localPosition);
        const auto target = forcedisplay::toWorldPoint(pose, localTarget);
        const auto up = forcedisplay::toWorldVector(pose, localUp);
        if (bodyFixedCamera_) {
            camera->SetPosition(position.data());
            camera->SetFocalPoint(target.data());
            camera->SetViewUp(up.data());
        } else {
            const auto displacement = forcedisplay::sub(pose.position, cameraPose_.position);
            const auto worldPosition = forcedisplay::add(previousView.position, displacement);
            const auto worldTarget = forcedisplay::add(previousView.focalPoint, displacement);
            camera->SetPosition(worldPosition.data());
            camera->SetFocalPoint(worldTarget.data());
        }
        cameraPose_ = pose;
        pose_ = pose;
        velocityWorld_ = velocityWorld;
        forces_ = forces;
        forces_.contributions += forcedisplay::derivedContributions(forces);
        hasState_ = true;
        currentTime_ = state.value(QStringLiteral("zeit_s")).toDouble();
        updateWaterAnimation();
        {
            double bounds[6];
            double center[3] = {0.0, 0.0, 0.0};
            if (!bodies_.empty()) {
                bodies_.front().mesh->GetBounds(bounds);
                for (const auto& body : bodies_) {
                    double single[6];
                    body.mesh->GetBounds(single);
                    for (int axis = 0; axis < 3; ++axis) {
                        bounds[axis * 2] = std::min(bounds[axis * 2], single[axis * 2]);
                        bounds[axis * 2 + 1] = std::max(bounds[axis * 2 + 1], single[axis * 2 + 1]);
                    }
                }
                for (int axis = 0; axis < 3; ++axis) {
                    center[axis] = 0.5 * (bounds[axis * 2] + bounds[axis * 2 + 1]);
                }
            }
            const Vec3 point = forcedisplay::toWorldPoint(pose, {center[0], center[1], center[2]});
            if (track_.isEmpty() || currentTime_ > track_.back().time) {
                track_.push_back({currentTime_, point});
            }
        }
        updateTrail();
        updateWaterMotion();
        ensureReferences();
        updateScaling();
        geometryDirty_ = true;
        currentState_ = state;
        updateWaterPlane();
        renderer_->ResetCameraClippingRange();
        renderScene();
        return true;
    } catch (const std::exception& exception) {
        if (error != nullptr) {
            *error = QString::fromUtf8(exception.what());
        }
        return false;
    }
}

void VtkSceneWidget::clearState()
{
    currentState_ = {};
    for (auto& visual : windVisuals_) {
        visual.lineActor->VisibilityOff();
        visual.headActor->VisibilityOff();
    }
    windArrowGeometry_ = {};
    const auto previousPosition = cameraPose_.position;
    orthographicScale_ = 0.0;
    orthographicPan_ = {};
    cameraDragButton_ = Qt::NoButton;
    const auto previousView = cameraView();
    const auto localPosition = inverseRotate(cameraPose_,
        forcedisplay::sub(previousView.position, cameraPose_.position));
    const auto localTarget = inverseRotate(cameraPose_,
        forcedisplay::sub(previousView.focalPoint, cameraPose_.position));
    const auto localUp = inverseRotate(cameraPose_, previousView.viewUp);
    resetMotionHistory();
    for (auto& body : bodies_) {
        body.prepared = body.preparation->base;
        body.mesh = body.prepared->mesh;
        body.materialMesh = body.prepared->material;
        body.triangles->SetInputData(body.materialMesh);
        if (body.mesh->GetNumberOfPolys() == 0)
            vtkPolyDataMapper::SafeDownCast(body.body->GetMapper())->SetInputData(body.mesh);
        body.body->SetUserTransform(nullptr);
        body.waterline->SetUserTransform(nullptr);
        body.reflection->SetUserTransform(nullptr);
        body.cutter->SetInputData(body.prepared->cutMesh);
        body.waterlineData->Initialize();
    }
    pose_ = {};
    cameraPose_ = {};
    auto* camera = renderer_->GetActiveCamera();
    if (bodyFixedCamera_) {
        camera->SetPosition(localPosition.data());
        camera->SetFocalPoint(localTarget.data());
        camera->SetViewUp(localUp.data());
    } else {
        const auto position = forcedisplay::sub(previousView.position, previousPosition);
        const auto target = forcedisplay::sub(previousView.focalPoint, previousPosition);
        camera->SetPosition(position.data());
        camera->SetFocalPoint(target.data());
    }
    dryLoops_.clear();
    forces_ = {};
    velocityWorld_.reset();
    hasState_ = false;
    scaled_.clear();
    track_.clear();
    currentTime_ = 0.0;
    trailActor_->VisibilityOff();
    updateWaterPlane();
    geometryDirty_ = true;
    requestRender();
}

void VtkSceneWidget::resetMotionHistory()
{
    motionHistory_.clear();
    motionStateCount_ = 0;
    lastMotion_.reset();
    motionDiagnostic_.clear();
    motionMesh_->Initialize();
    wakeMesh_->Initialize();
    motionActor_->VisibilityOff();
    wakeActor_->VisibilityOff();
}

VtkSceneWidget::MotionGeometry VtkSceneWidget::motionGeometry() const
{
    return {motionHistory_.points().size(), motionMesh_->GetNumberOfLines(), wakeMesh_->GetNumberOfLines()};
}

void VtkSceneWidget::invalidateMotionHistory(qsizetype processed, const QString& diagnostic)
{
    resetMotionHistory();
    motionStateCount_ = processed;
    motionDiagnostic_ = diagnostic;
}

bool VtkSceneWidget::appendMotionStates(const QVector<QJsonObject>& states, QString* error)
{
    try {
        for (const auto& state : states) {
            const auto pose = forcedisplay::readPose(state);
            auto vector = [&state](const char* key) {
                const auto a = state.value(QString::fromLatin1(key)).toArray();
                if (a.size() != 3) throw std::runtime_error("Fahrtdarstellung: CORE-Bezugsvektor fehlt.");
                Vec3 result{};
                for (qsizetype i = 0; i < 3; ++i) {
                    if (!a[i].isDouble() || !std::isfinite(a[i].toDouble()))
                        throw std::runtime_error("Fahrtdarstellung: CORE-Bezugsvektor ungueltig.");
                    result[static_cast<std::size_t>(i)] = a[i].toDouble();
                }
                return result;
            };
            const auto reference = vector("moment_bezug_modell_m");
            watermotion::Input input;
            input.time = state.value(QStringLiteral("zeit_s")).toDouble(-1);
            input.reference = reference;
            input.center = forcedisplay::toWorldPoint(pose, reference);
            input.velocity = vector("geschwindigkeit_welt_m_s");
            QVector<Vec3> contour;
            const auto origin = inverseRotate(pose, {-pose.position[0], -pose.position[1], -pose.position[2]});
            const auto normal = inverseRotate(pose, {0.0, 0.0, 1.0});
            for (auto& body : bodies_) {
                body.localWaterPlane->SetOrigin(origin.data());
                body.localWaterPlane->SetNormal(normal.data());
                body.cutter->Update();
                auto* cut = body.cutter->GetOutput();
                auto* points = cut->GetPoints();
                if (!points || cut->GetNumberOfLines() == 0) continue;
                for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i) {
                    double p[3];
                    points->GetPoint(i, p);
                    contour.push_back(forcedisplay::toWorldPoint(pose, {p[0], p[1], p[2]}));
                }
            }
            input.contact = watermotion::contactFromContour(contour, input.velocity);
            motionDiagnostic_ = !input.contact && std::hypot(input.velocity[0], input.velocity[1]) > 1e-10
                ? QStringLiteral("Keine verwertbare Wasserlinienkontur: kein neues Kielwasser, keine erfundenen Kontaktanker.")
                : QString();
            motionHistory_.append(input, graphicsProfile_);
            lastMotion_ = input;
            ++motionStateCount_;
            ++workCounters_.motionInputs;
        }
        return true;
    } catch (const std::exception& exception) {
        if (error) *error = QString::fromUtf8(exception.what());
        return false;
    }
}

void VtkSceneWidget::updateWaterMotion()
{
    using F = graphics::Feature;
    const auto& history = motionHistory_.points();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto lines = vtkSmartPointer<vtkCellArray>::New();
    auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colors->SetNumberOfComponents(4);
    auto edge = [&](Vec3 a, Vec3 b, double alpha, bool foam) {
        a[2] = b[2] = 0.012;
        const vtkIdType ids[2]{points->InsertNextPoint(a.data()), points->InsertNextPoint(b.data())};
        lines->InsertNextCell(2, ids);
        const unsigned char rgba[4]{static_cast<unsigned char>(foam ? 205 : 75),
            static_cast<unsigned char>(foam ? 230 : 205),
            static_cast<unsigned char>(foam ? 230 : 245), toByte(alpha)};
        colors->InsertNextTypedTuple(rgba);
        colors->InsertNextTypedTuple(rgba);
    };
    auto publish = [&](vtkPolyData* mesh) {
        mesh->Initialize();
        mesh->SetPoints(points);
        mesh->SetLines(lines);
        mesh->GetPointData()->SetScalars(colors);
        mesh->Modified();
    };
    if (graphicsProfile_.on(F::WaterTrack)) {
        for (std::size_t i = 1; i < history.size(); ++i) {
            const auto& a = history[i - 1];
            const auto& b = history[i];
            if (!b.connected) continue;
            const double fade = std::clamp(1.0 - (currentTime_ - b.time) / graphicsProfile_.trackAge, 0.0, 1.0);
            edge(a.center, b.center, fade * 0.8, false);
        }
        if (graphicsProfile_.on(F::DistanceMarks)) {
            int budget = graphicsProfile_.trackBudget;
            for (std::size_t i = history.size(); i > 1 && budget > 0; --i) {
                const auto& a = history[i - 2];
                const auto& b = history[i - 1];
                if (!b.connected || b.distance <= a.distance) continue;
                const double spacing = graphicsProfile_.markSpacing;
                const double quotient = b.distance / spacing;
                if (!std::isfinite(quotient) || b.distance - spacing == b.distance) {
                    motionDiagnostic_ = QStringLiteral("Distanzmarkenabstand ist fuer diesen Fahrweg numerisch nicht darstellbar; Marken ausgesetzt.");
                    break;
                }
                double mark = std::floor(quotient) * spacing;
                while (mark > a.distance && budget-- > 0) {
                    const double t = (mark - a.distance) / (b.distance - a.distance);
                    const auto p = forcedisplay::add(a.center,
                        forcedisplay::scale(forcedisplay::sub(b.center, a.center), t));
                    const Vec3 side{-b.direction[1] * 0.12, b.direction[0] * 0.12, 0.0};
                    const double fade = std::clamp(1.0 - (currentTime_ - b.time) / graphicsProfile_.trackAge, 0.0, 1.0);
                    edge(forcedisplay::sub(p, side), forcedisplay::add(p, side), fade * 0.9, false);
                    mark -= spacing;
                }
            }
        }
    }
    publish(motionMesh_);
    motionActor_->SetVisibility(graphicsProfile_.on(F::WaterTrack) && lines->GetNumberOfCells() > 0);
    points = vtkSmartPointer<vtkPoints>::New();
    lines = vtkSmartPointer<vtkCellArray>::New();
    colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colors->SetNumberOfComponents(4);
    if (graphicsProfile_.on(F::Wake)) {
        auto wakePoint = [&](const watermotion::Point& p, double sign) {
            const double age = std::max(0.0, currentTime_ - p.time);
            const double spread = p.contact->width * 0.25
                + std::min(p.speed, 4.0) * std::min(age, 20.0) * 0.06;
            const Vec3 side{-p.direction[1] * sign * spread, p.direction[0] * sign * spread, 0.0};
            return forcedisplay::add(p.contact->trailing, side);
        };
        for (std::size_t i = 1; i < history.size(); ++i) {
            const auto& a = history[i - 1];
            const auto& b = history[i];
            if (!b.connected || !a.contact || !b.contact) continue;
            const double fade = std::clamp(1.0 - (currentTime_ - b.time) / graphicsProfile_.trackAge, 0.0, 1.0);
            for (double sign : {-1.0, 1.0}) edge(wakePoint(a, sign), wakePoint(b, sign),
                fade * std::min(0.55, b.speed * 0.22), true);
        }
        if (lastMotion_ && lastMotion_->contact) {
            const auto& input = *lastMotion_;
            const double speed = std::hypot(input.velocity[0], input.velocity[1]);
            if (speed > 1e-10) {
                const auto& c = *input.contact;
                const Vec3 direction{input.velocity[0] / speed, input.velocity[1] / speed, 0.0};
                const Vec3 side{-direction[1] * c.width * 0.35, direction[0] * c.width * 0.35, 0.0};
                const auto back = forcedisplay::sub(c.leading, forcedisplay::scale(direction,
                    std::min(c.width * 0.4, speed * 0.15)));
                const double intensity = std::min(0.65, speed * 0.13);
                edge(c.leading, forcedisplay::add(back, side), intensity, true);
                edge(c.leading, forcedisplay::sub(back, side), intensity, true);
            }
        }
    }
    publish(wakeMesh_);
    wakeActor_->SetVisibility(graphicsProfile_.on(F::Wake) && lines->GetNumberOfCells() > 0);
}

void VtkSceneWidget::updateTrail()
{
    if (!graphicsProfile_.on(graphics::Feature::Trail)) {
        trailActor_->VisibilityOff();
        return;
    }
    qsizetype count = 0;
    while (count < track_.size() && track_.at(count).time <= currentTime_ + 1e-12) {
        ++count;
    }
    if (count < 2) {
        trailActor_->VisibilityOff();
        return;
    }
    vtkNew<vtkPoints> points;
    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetNumberOfComponents(4);
    std::vector<vtkIdType> ids;
    ids.reserve(static_cast<std::size_t>(count));
    for (qsizetype i = 0; i < count; ++i) {
        const auto& p = track_.at(i).point;
        ids.push_back(points->InsertNextPoint(p[0], p[1], p[2]));
        // Ältere Abschnitte verblassen.
        const double age = static_cast<double>(i) / static_cast<double>(count - 1);
        const unsigned char rgba[4] = {242, 242, 228, toByte(0.12 + 0.78 * age)};
        colors->InsertNextTypedTuple(rgba);
    }
    vtkNew<vtkCellArray> lines;
    lines->InsertNextCell(static_cast<vtkIdType>(ids.size()), ids.data());
    trailMesh_->Initialize();
    trailMesh_->SetPoints(points);
    trailMesh_->SetLines(lines);
    trailMesh_->GetPointData()->SetScalars(colors);
    trailMesh_->Modified();
    trailActor_->VisibilityOn();
}

void VtkSceneWidget::setOptions(const DisplayOptions& options)
{
    if (!updatesActive_) return;
    const auto visibility = orthographicView_ == OrthographicView::Free
        ? options.visibility : Visibility::ThroughBody;
    const bool moved = visibility != options_.visibility;
    const bool rescale = options.forceMode != options_.forceMode
        || options.momentMode != options_.momentMode;
    options_ = options;
    options_.visibility = visibility;
    if (moved) {
        moveForceActors();
    }
    if (rescale) {
        updateScaling();
    }
    applyBodyStyle();
    geometryDirty_ = true;
    requestRender();
}

void VtkSceneWidget::setGraphicsProfile(const graphics::Profile& profile)
{
    if (!updatesActive_) return;
    auto effective = profile;
    if (orthographicView_ != OrthographicView::Free && !inspectionMode_) {
        effective = graphics::Profile{};
        for (auto feature : {graphics::Feature::Water, graphics::Feature::Waterline, graphics::Feature::Smooth,
                 graphics::Feature::FXAA})
            effective.enabled[static_cast<std::size_t>(feature)] = true;
        effective.waterOpacity = 0.14;
        effective.throughBody = true;
    }
    const bool historyChanged = graphicsProfile_.trackAge != effective.trackAge
        || graphicsProfile_.trackSpacing != effective.trackSpacing
        || graphicsProfile_.markSpacing != effective.markSpacing
        || graphicsProfile_.trackBudget != effective.trackBudget;
    if (historyChanged) resetMotionHistory();
    graphics::Profile sanitized = effective;
    sanitized.bodyOpacity = std::isfinite(effective.bodyOpacity)
        ? std::clamp(effective.bodyOpacity, 0.0, 1.0) : graphicsProfile_.bodyOpacity;
    sanitized.waterOpacity = std::isfinite(effective.waterOpacity)
        ? std::clamp(effective.waterOpacity, 0.0, 1.0) : graphicsProfile_.waterOpacity;
    sanitized.normalScale = std::isfinite(effective.normalScale)
        ? std::clamp(effective.normalScale, 0.0, 1.0) : graphicsProfile_.normalScale;
    sanitized.waterPeriod = std::isfinite(effective.waterPeriod)
        ? std::clamp(effective.waterPeriod, 0.02, 20.0) : graphicsProfile_.waterPeriod;
    sanitized.quality = std::clamp(effective.quality, 0, 2);
    if (graphicsProfile_.enabled == sanitized.enabled
        && graphicsProfile_.bodyOpacity == sanitized.bodyOpacity
        && graphicsProfile_.waterOpacity == sanitized.waterOpacity
        && graphicsProfile_.normalScale == sanitized.normalScale
        && graphicsProfile_.waterPeriod == sanitized.waterPeriod
        && graphicsProfile_.quality == sanitized.quality
        && graphicsProfile_.throughBody == sanitized.throughBody
        && graphicsProfile_.liveReduction == sanitized.liveReduction
        && graphicsProfile_.environmentPath == sanitized.environmentPath) {
        if (!historyChanged) return;
    }
    const bool moved = options_.visibility
        != (sanitized.throughBody ? Visibility::ThroughBody : Visibility::Spatial);
    graphicsProfile_ = sanitized;
    options_.visibility = sanitized.throughBody ? Visibility::ThroughBody : Visibility::Spatial;
    if (moved) moveForceActors();
    ++workCounters_.profiles;
    applyGraphicsProfile();
    geometryDirty_ = true;
    requestRender();
}

double VtkSceneWidget::renderNow()
{
    if (!updatesActive_) return 0.0;
    const auto start = std::chrono::steady_clock::now();
    renderScene();
    const auto end = std::chrono::steady_clock::now();
    lastRenderDurationMs_ =
        std::chrono::duration<double, std::milli>(end - start).count();
    return lastRenderDurationMs_;
}

bool VtkSceneWidget::setMaterialPreview(const QString& elementId, int style)
{
    if (style < 0 || style > 4) return false;
    bool found = false;
    for (auto& body : bodies_) {
        if (body.elementId == elementId && body.surface != DisplayPart::Surface::Invisible) {
            body.materialPreview = style;
            found = true;
        }
    }
    if (!found) return false;
    applyBodyStyle();
    geometryDirty_ = true;
    requestRender();
    return true;
}

QImage VtkSceneWidget::captureImage()
{
    renderNow();
    return vtkWidget_->grab().toImage();
}

VtkSceneWidget::CameraView VtkSceneWidget::cameraView() const
{
    CameraView view;
    auto* camera = renderer_->GetActiveCamera();
    camera->GetPosition(view.position.data());
    camera->GetFocalPoint(view.focalPoint.data());
    camera->GetViewUp(view.viewUp.data());
    return view;
}

bool VtkSceneWidget::parallelProjection() const
{
    return renderer_->GetActiveCamera()->GetParallelProjection() != 0;
}

double VtkSceneWidget::parallelScale() const
{
    return renderer_->GetActiveCamera()->GetParallelScale();
}

void VtkSceneWidget::setViewName(const QString& name)
{
    viewName_->setText(name);
    viewName_->setVisible(!name.isEmpty());
    layoutViewControls();
}

void VtkSceneWidget::layoutViewControls()
{
    defaultViewButton_->ensurePolished();
    viewName_->ensurePolished();
    const int width = vtkWidget_->width();
    const int height = vtkWidget_->height();
    viewName_->adjustSize();
    viewName_->move(std::max(0, width - viewName_->width() - 4),
        std::max(0, height - viewName_->height() - 4));
    viewName_->raise();
    const int buttonWidth = std::max(defaultViewButton_->sizeHint().width(),
        defaultViewButton_->fontMetrics().horizontalAdvance(defaultViewButton_->text()) + 8);
    if (orthographicView_ != OrthographicView::Free)
        setMinimumSize(std::max(buttonWidth, viewName_->width()) + 8,
            defaultViewButton_->sizeHint().height() + viewName_->height() + 12);
    defaultViewButton_->resize(std::min(std::max(1, width - 8), buttonWidth),
        defaultViewButton_->sizeHint().height());
    defaultViewButton_->move(std::max(0, width - defaultViewButton_->width() - 4), 4);
    defaultViewButton_->raise();
}

void VtkSceneWidget::resetOrthographicCamera()
{
    orthographicPan_ = {};
    orthographicScale_ = 0.0;
    cameraDragButton_ = Qt::NoButton;
    geometryDirty_ = true;
    requestRender();
}

void VtkSceneWidget::setOrthographicView(OrthographicView view)
{
    cancelInspectionDrag();
    inspectionPresetId_.clear();
    orthographicView_ = view;
    waterLevelActor_->VisibilityOff();
    defaultViewButton_->setVisible(view != OrthographicView::Free);
    defaultViewButton_->setToolTip(QStringLiteral("Nur diese Kamera neu einpassen; Zoom und Verschiebung zurücksetzen."));
    if (view != OrthographicView::Free)
        vtkWidget_->setToolTip(QStringLiteral(
            "Mausrad oder rechte Maustaste: Zoom. Linke oder mittlere Maustaste: Verschieben. Blickrichtung bleibt fest."));
    layout()->setContentsMargins(view == OrthographicView::Free ? 1 : 0,
        view == OrthographicView::Free ? 1 : 0, view == OrthographicView::Free ? 1 : 0,
        view == OrthographicView::Free ? 1 : 0);
    axesRenderer_->SetDraw(view == OrthographicView::Free);
    renderer_->GetActiveCamera()->SetParallelProjection(view != OrthographicView::Free);
    if (view != OrthographicView::Free) setGraphicsProfile(graphicsProfile_);
    layoutViewControls();
    resetOrthographicCamera();
}

void VtkSceneWidget::updateOrthographicCamera()
{
    if (orthographicView_ == OrthographicView::Free) return;
    double bounds[6];
    if (!bodyBounds(bounds)) return;
    if (inspectionMode_ && !inspectionPresetId_.isEmpty()) {
        const auto& presets = navigation::cameraPresets();
        const auto preset = std::find_if(presets.cbegin(), presets.cend(),
            [this](const auto& value) {
                return inspectionPresetId_ == QString::fromLatin1(value.id);
            });
        if (preset == presets.cend()) return;
        const auto unit = [](const Vec3& value) {
            const double length = forcedisplay::norm(value);
            return length > 1e-12 ? forcedisplay::scale(value, 1.0 / length) : Vec3{};
        };
        const Vec3 direction = unit(forcedisplay::toWorldVector(cameraPose_, preset->direction));
        const Vec3 up = unit(forcedisplay::toWorldVector(cameraPose_, preset->up));
        const Vec3 right = unit(forcedisplay::cross(up, direction));
        if (forcedisplay::norm(direction) < 1e-9 || forcedisplay::norm(up) < 1e-9
            || forcedisplay::norm(right) < 1e-9)
            return;
        Vec3 center{0.5 * (bounds[0] + bounds[1]), 0.5 * (bounds[2] + bounds[3]),
            0.5 * (bounds[4] + bounds[5])};
        double halfHeight = 0.1, halfWidth = 0.1, radius = 0.1;
        for (int corner = 0; corner < 8; ++corner) {
            const Vec3 offset = forcedisplay::sub(
                {bounds[(corner & 1) ? 1 : 0], bounds[(corner & 2) ? 3 : 2],
                    bounds[(corner & 4) ? 5 : 4]}, center);
            const auto dot = [](const Vec3& a, const Vec3& b) {
                return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
            };
            halfHeight = std::max(halfHeight, std::abs(dot(offset, up)));
            halfWidth = std::max(halfWidth, std::abs(dot(offset, right)));
            radius = std::max(radius, forcedisplay::norm(offset));
        }
        const double aspect = static_cast<double>(std::max(1, vtkWidget_->width()))
            / std::max(1, vtkWidget_->height());
        if (orthographicScale_ <= 0.0)
            orthographicScale_ = 1.12 * std::max(halfHeight, halfWidth / aspect);
        center = forcedisplay::add(center,
            forcedisplay::add(forcedisplay::scale(right, orthographicPan_.x()),
                forcedisplay::scale(up, orthographicPan_.y())));
        auto* camera = renderer_->GetActiveCamera();
        camera->ParallelProjectionOn();
        const auto position = forcedisplay::add(center, forcedisplay::scale(direction, 3.0 * radius));
        camera->SetFocalPoint(center.data());
        camera->SetPosition(position.data());
        camera->SetViewUp(up.data());
        camera->SetParallelScale(orthographicScale_);
        waterLevelActor_->VisibilityOff();
        return;
    }
    Vec3 center{0.5 * (bounds[0] + bounds[1]), 0.5 * (bounds[2] + bounds[3]), 0.0};
    const auto longitudinal = forcedisplay::toWorldVector(cameraPose_, {1.0, 0.0, 0.0});
    const auto port = forcedisplay::toWorldVector(cameraPose_, {0.0, 1.0, 0.0});
    // At vertical pitch use the horizontal port axis to retain a defined heading.
    const double yaw = std::hypot(longitudinal[0], longitudinal[1]) > 1e-10
        ? std::atan2(longitudinal[1], longitudinal[0]) : std::atan2(-port[0], port[1]);
    const Vec3 forward{std::cos(yaw), std::sin(yaw), 0.0};
    Vec3 direction{};
    Vec3 up{0.0, 0.0, 1.0};
    if (orthographicView_ == OrthographicView::Port)
        direction = {-forward[1], forward[0], 0.0};
    else if (orthographicView_ == OrthographicView::Stern)
        direction = forcedisplay::scale(forward, -1.0);
    else {
        direction = {0.0, 0.0, 1.0};
        up = forward;
    }
    const auto right = forcedisplay::cross(up, direction);
    double halfHeight = 0.1, halfWidth = 0.1, radius = 0.1;
    for (int corner = 0; corner < 8; ++corner) {
        const Vec3 offset = forcedisplay::sub(
            {bounds[(corner & 1) ? 1 : 0], bounds[(corner & 2) ? 3 : 2],
                bounds[(corner & 4) ? 5 : 4]}, center);
        const auto dot = [](const Vec3& a, const Vec3& b) {
            return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
        };
        halfHeight = std::max(halfHeight, std::abs(dot(offset, up)));
        halfWidth = std::max(halfWidth, std::abs(dot(offset, right)));
        radius = std::max(radius, forcedisplay::norm(offset));
    }
    const double aspect = static_cast<double>(std::max(1, vtkWidget_->width()))
        / std::max(1, vtkWidget_->height());
    if (orthographicScale_ <= 0.0)
        orthographicScale_ = 1.12 * std::max(halfHeight, halfWidth / aspect);
    center = forcedisplay::add(center,
        forcedisplay::add(forcedisplay::scale(right, orthographicPan_.x()),
            forcedisplay::scale(up, orthographicPan_.y())));
    auto* camera = renderer_->GetActiveCamera();
    const auto position = forcedisplay::add(center, forcedisplay::scale(direction, 3.0 * radius));
    camera->ParallelProjectionOn();
    camera->SetFocalPoint(center.data());
    camera->SetPosition(position.data());
    camera->SetViewUp(up.data());
    camera->SetParallelScale(orthographicScale_);
    const bool waterLevel = orthographicView_ != OrthographicView::Top;
    waterLevelActor_->SetVisibility(waterLevel);
    if (waterLevel) {
        const Vec3 waterCenter{center[0], center[1], 0.0};
        const auto halfLine = forcedisplay::scale(right, orthographicScale_ * aspect * 1.1);
        PolyBuilder line;
        line.segment(forcedisplay::sub(waterCenter, halfLine),
            forcedisplay::add(waterCenter, halfLine));
        line.storeLines(waterLevelLine_);
        waterLevelActor_->GetProperty()->SetLineWidth(
            static_cast<float>(vtkWidget_->devicePixelRatioF()));
    }
}

bool VtkSceneWidget::setInspectionCameraPreset(const QString& id, QString* error)
{
    if (!inspectionMode_) {
        if (error) *error = QStringLiteral("Kameraauswahl ist nur in der Bootsauswahl verfügbar.");
        return false;
    }
    cancelInspectionDrag();
    inspectionPresetId_.clear();
    if (id.isEmpty()) {
        setOrthographicView(OrthographicView::Free);
        inspectionEditTarget_.clear();
        vtkWidget_->unsetCursor();
        return true;
    }
    const auto& presets = navigation::cameraPresets();
    const auto preset = std::find_if(presets.cbegin(), presets.cend(),
        [&id](const auto& value) { return id == QString::fromLatin1(value.id); });
    if (preset == presets.cend()) {
        if (error) *error = QStringLiteral("Unbekannte Kameraansicht: %1").arg(id);
        return false;
    }
    const bool axisView = id == QStringLiteral("backbord")
        || id == QStringLiteral("steuerbord") || id == QStringLiteral("bug")
        || id == QStringLiteral("heck") || id == QStringLiteral("oben")
        || id == QStringLiteral("unten");
    if (!axisView) {
        setOrthographicView(OrthographicView::Free);
        if (!setCameraPreset(id, error)) return false;
        inspectionEditTarget_.clear();
        vtkWidget_->unsetCursor();
        return true;
    }
    double bounds[6];
    if (!bodyBounds(bounds)) {
        if (error) *error = QStringLiteral("Keine Modellgeometrie für die Kameraansicht vorhanden.");
        return false;
    }
    setOrthographicView(OrthographicView::Top);
    inspectionPresetId_ = id;
    updateOrthographicCamera();
    geometryDirty_ = true;
    requestRender();
    return true;
}

bool VtkSceneWidget::inspectionEditView() const
{
    return inspectionMode_ && (inspectionPresetId_ == QStringLiteral("backbord")
        || inspectionPresetId_ == QStringLiteral("steuerbord")
        || inspectionPresetId_ == QStringLiteral("bug")
        || inspectionPresetId_ == QStringLiteral("heck")
        || inspectionPresetId_ == QStringLiteral("oben")
        || inspectionPresetId_ == QStringLiteral("unten"));
}

void VtkSceneWidget::setInspectionEditTarget(const QString& elementId)
{
    cancelInspectionDrag();
    inspectionEditTarget_ = elementId;
    if (inspectionEditTarget_.isEmpty()) vtkWidget_->unsetCursor();
    else {
        vtkWidget_->setCursor(Qt::OpenHandCursor);
        vtkWidget_->setFocus(Qt::OtherFocusReason);
    }
}

bool VtkSceneWidget::setInspectionRigidTransform(
    const QString& elementId, const std::array<double, 16>& matrix)
{
    BodyActor* selected = nullptr;
    for (auto& body : bodies_) {
        if (body.elementId != elementId) continue;
        if (selected) return false;
        selected = &body;
    }
    if (!selected) return false;
    auto transform = vtkSmartPointer<vtkMatrix4x4>::New();
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            transform->SetElement(row, column, matrix[static_cast<std::size_t>(row * 4 + column)]);
    selected->body->SetUserMatrix(transform);
    requestRender();
    return true;
}

void VtkSceneWidget::setInspectionHandles(const QVector<InspectionHandle>& handles)
{
    for (const auto& actor : inspectionHandleActors_) renderer_->RemoveActor(actor);
    inspectionHandleActors_.clear();
    for (const auto& label : inspectionHandleLabels_) renderer_->RemoveActor(label);
    inspectionHandleLabels_.clear();
    inspectionHandles_ = handles;
    double upRaw[3];
    double directionRaw[3];
    auto* camera = renderer_->GetActiveCamera();
    camera->GetViewUp(upRaw);
    camera->GetDirectionOfProjection(directionRaw);
    const auto unit = [](const Vec3& value) {
        const double length = forcedisplay::norm(value);
        return length > 1e-12 ? forcedisplay::scale(value, 1.0 / length) : Vec3{};
    };
    const Vec3 up = unit({upRaw[0], upRaw[1], upRaw[2]});
    const Vec3 direction = unit({directionRaw[0], directionRaw[1], directionRaw[2]});
    const Vec3 right = unit(forcedisplay::cross(direction, up));
    for (const auto& handle : handles) {
        const double pixelSize = worldPerLogicalPixel(handle.position);
        if (!(pixelSize > 0.0) || !std::isfinite(pixelSize)) continue;
        auto sphere = vtkSmartPointer<vtkSphereSource>::New();
        sphere->SetCenter(handle.position.data());
        sphere->SetRadius(pixelSize * 6.0);
        sphere->SetThetaResolution(16);
        sphere->SetPhiResolution(12);
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphere->GetOutputPort());
        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->PickableOff();
        actor->UseBoundsOff();
        if (handle.key == QStringLiteral("hals"))
            actor->GetProperty()->SetColor(1.0, 0.75, 0.2);
        else if (handle.key == QStringLiteral("schothorn"))
            actor->GetProperty()->SetColor(1.0, 0.35, 0.32);
        else
            actor->GetProperty()->SetColor(0.3, 0.85, 1.0);
        renderer_->AddActor(actor);
        inspectionHandleActors_.push_back(actor);

        auto label = vtkSmartPointer<vtkBillboardTextActor3D>::New();
        const QString labelText = handle.key == QStringLiteral("hals")
            ? QStringLiteral("Hals") : handle.key == QStringLiteral("schothorn")
            ? QStringLiteral("Schothorn") : handle.key == QStringLiteral("kopf_vorliek")
            ? QStringLiteral("Kopf · Vorliek") : QStringLiteral("Kopf · Achterliek");
        label->SetInput(labelText.toUtf8().constData());
        auto labelPosition = forcedisplay::add(handle.position,
            forcedisplay::add(forcedisplay::scale(right, pixelSize * 8.0),
                forcedisplay::scale(up, pixelSize * 8.0)));
        label->SetPosition(labelPosition.data());
        label->PickableOff();
        label->UseBoundsOff();
        label->GetTextProperty()->SetFontSize(13);
        label->GetTextProperty()->SetColor(0.95, 0.96, 1.0);
        label->GetTextProperty()->SetBold(true);
        renderer_->AddActor(label);
        inspectionHandleLabels_.push_back(label);
    }
    requestRender();
}

Vec3 VtkSceneWidget::inspectionWorldPointAt(const QPointF& point) const
{
    const double ratio = vtkWidget_->devicePixelRatioF();
    const int* size = renderWindow_->GetSize();
    auto* camera = renderer_->GetActiveCamera();
    double focal[3];
    camera->GetFocalPoint(focal);
    renderer_->SetWorldPoint(focal[0], focal[1], focal[2], 1.0);
    renderer_->WorldToDisplay();
    const double depth = renderer_->GetDisplayPoint()[2];
    renderer_->SetDisplayPoint(point.x() * ratio, size[1] - point.y() * ratio, depth);
    renderer_->DisplayToWorld();
    const double* world = renderer_->GetWorldPoint();
    if (std::abs(world[3]) < 1e-12) return {};
    return {world[0] / world[3], world[1] / world[3], world[2] / world[3]};
}

QPointF VtkSceneWidget::inspectionScreenPointForWorld(const Vec3& point) const
{
    return toLogicalScreen(point);
}

Vec3 VtkSceneWidget::inspectionWorldVectorForScreenDelta(const QPointF& delta) const
{
    auto* camera = renderer_->GetActiveCamera();
    double upRaw[3];
    double directionRaw[3];
    camera->GetViewUp(upRaw);
    camera->GetDirectionOfProjection(directionRaw);
    const auto unit = [](const Vec3& value) {
        const double length = forcedisplay::norm(value);
        return length > 1e-12 ? forcedisplay::scale(value, 1.0 / length) : Vec3{};
    };
    const Vec3 up = unit({upRaw[0], upRaw[1], upRaw[2]});
    const Vec3 direction = unit({directionRaw[0], directionRaw[1], directionRaw[2]});
    const Vec3 right = unit(forcedisplay::cross(direction, up));
    double focalRaw[3];
    camera->GetFocalPoint(focalRaw);
    const Vec3 focal{focalRaw[0], focalRaw[1], focalRaw[2]};
    const double scale = worldPerLogicalPixel(focal);
    return forcedisplay::add(forcedisplay::scale(right, delta.x() * scale),
        forcedisplay::scale(up, -delta.y() * scale));
}

bool VtkSceneWidget::setCameraView(const CameraView& view)
{
    for (const auto& vector : {view.position, view.focalPoint, view.viewUp}) {
        for (double component : vector) {
            if (!std::isfinite(component)) return false;
        }

    }
    const auto direction = forcedisplay::sub(view.focalPoint, view.position);
    if (forcedisplay::norm(direction) < 1e-9
        || forcedisplay::norm(forcedisplay::cross(direction, view.viewUp)) < 1e-9) {
        return false;
    }
    auto* camera = renderer_->GetActiveCamera();
    camera->SetPosition(view.position.data());
    camera->SetFocalPoint(view.focalPoint.data());
    camera->SetViewUp(view.viewUp.data());
    renderer_->ResetCameraClippingRange();
    geometryDirty_ = true;
    requestRender();
    return true;
}

bool VtkSceneWidget::setCameraPreset(const QString& id, QString* error)
{
    const auto& presets = navigation::cameraPresets();
    const auto it = std::find_if(presets.cbegin(), presets.cend(),
        [&id](const auto& preset) { return id == QString::fromLatin1(preset.id); });
    if (it == presets.cend()) {
        if (error) *error = QStringLiteral("Unbekannte Kameraansicht: %1").arg(id);
        return false;
    }
    double bounds[6];
    if (!bodyBounds(bounds)) {
        if (error) *error = QStringLiteral("Keine Modellgeometrie für die Kameraansicht vorhanden.");
        return false;
    }
    auto* camera = renderer_->GetActiveCamera();
    Vec3 center{};
    for (int axis = 0; axis < 3; ++axis)
        center[axis] = 0.5 * (bounds[axis * 2] + bounds[axis * 2 + 1]);
    auto direction = forcedisplay::toWorldVector(cameraPose_, it->direction);
    auto up = forcedisplay::toWorldVector(cameraPose_, it->up);
    const bool waterLevel = it->direction[2] == 0.0;
    if (waterLevel) {
        direction[2] = 0.0;
        if (forcedisplay::norm(direction) < 1e-9) {
            if (error) *error = QStringLiteral(
                "Diese Yachtachse steht senkrecht; eine horizontale Ansicht ist nicht eindeutig.");
            return false;
        }
        up = {0, 0, 1};
    }
    const auto position = forcedisplay::add(center, direction);
    camera->SetFocalPoint(center.data());
    camera->SetPosition(position.data());
    camera->SetViewUp(up.data());
    renderer_->ResetCamera(bounds);
    camera->Zoom(0.95);
    if (waterLevel) {
        const Vec3 waterTarget{center[0], center[1], 0.0};
        double radius = 0.0;
        for (int corner = 0; corner < 8; ++corner) {
            const Vec3 point{bounds[(corner & 1) ? 1 : 0],
                bounds[(corner & 2) ? 3 : 2], bounds[(corner & 4) ? 5 : 4]};
            radius = std::max(radius, forcedisplay::norm(forcedisplay::sub(point, waterTarget)));
        }
        const double aspect = static_cast<double>(std::max(1, vtkWidget_->width()))
            / std::max(1, vtkWidget_->height());
        const double halfAngle = camera->GetViewAngle() * std::acos(-1.0) / 360.0;
        const double limitingAngle = std::atan(std::tan(halfAngle)
            * (camera->GetUseHorizontalViewAngle() ? std::min(1.0, 1.0 / aspect) : std::min(1.0, aspect)));
        const double distance = std::max(0.1, radius) / std::sin(limitingAngle) * 1.05;
        const auto waterPosition = forcedisplay::add(waterTarget,
            forcedisplay::scale(direction, distance / forcedisplay::norm(direction)));
        camera->SetFocalPoint(waterTarget.data());
        camera->SetPosition(waterPosition.data());
    }
    renderer_->ResetCameraClippingRange(bounds);
    geometryDirty_ = true;
    requestRender();
    return true;
}

void VtkSceneWidget::dockNavigation(QScrollArea* destination)
{
    const bool visible = !navigation_->isHidden();
    if (destination) {
        navigation_->setDocked(true);
        destination->setWidget(navigation_);
    } else {
        auto* scroll = qobject_cast<QScrollArea*>(navigation_->parentWidget()
            ? navigation_->parentWidget()->parentWidget() : nullptr);
        if (scroll) scroll->takeWidget();
        navigation_->setParent(vtkWidget_);
        navigation_->setDocked(false);
    }
    navigation_->setVisible(visible);
}

void VtkSceneWidget::setNavigationVisible(bool visible)
{
    navigation_->setVisible(visible);
    if (visible) navigation_->raise();
    geometryDirty_ = true;
    requestRender();
}

QString VtkSceneWidget::graphicsFeatureStatus(graphics::Feature feature) const
{
    using F = graphics::Feature;
    if (!graphicsProfile_.on(feature)) return QStringLiteral("Ausgeschaltet.");
    if (feature >= F::Fresnel && feature <= F::Reflection && !graphicsProfile_.on(F::Water))
        return QStringLiteral("Nicht wirksam: Wasser ist ausgeschaltet.");
    if ((feature == F::ModelIBL || feature == F::Coat || feature == F::ColorTexture
            || feature == F::ORM || feature == F::NormalTexture || feature == F::Anisotropy)
        && !graphicsProfile_.on(F::PBR))
        return QStringLiteral("Nicht wirksam: PBR ist ausgeschaltet.");
    bool lacquer = false, textured = false, cloth = false, brushed = false, orm = false;
    for (const auto& body : bodies_) {
        const auto surface = effectiveSurface(body.surface, body.materialPreview);
        lacquer = lacquer || surface == DisplayPart::Surface::Lacquer;
        cloth = cloth || surface == DisplayPart::Surface::Cloth;
        brushed = brushed || surface == DisplayPart::Surface::BrushedMetal;
        textured = textured || surface == DisplayPart::Surface::Lacquer
            || surface == DisplayPart::Surface::Cloth || surface == DisplayPart::Surface::Wood;
        orm = orm || surface == DisplayPart::Surface::Lacquer
            || surface == DisplayPart::Surface::Cloth || surface == DisplayPart::Surface::Wood
            || surface == DisplayPart::Surface::Metal || surface == DisplayPart::Surface::BrushedMetal;
    }
    if ((feature == F::Cloth && !cloth) || (feature == F::Anisotropy && !brushed)
        || (feature == F::ColorTexture && !textured) || (feature == F::Coat && !lacquer)
        || ((feature == F::ORM || feature == F::NormalTexture) && !orm))
        return QStringLiteral("Eingeschaltet; ohne passende Modellmaterialien hier ausgelassen (kein Fehler).");
    if (feature == F::Animation)
        return !graphicsProfile_.on(F::WaterNormals)
            ? QStringLiteral("Nicht wirksam: Wasser-Normalstruktur ist ausgeschaltet.")
            : QStringLiteral("Im statischen Ergebnis ruhend: Animation folgt nur der Simulationszeit.");
    if (feature == F::DistanceMarks && !graphicsProfile_.on(F::WaterTrack))
        return QStringLiteral("Nicht wirksam: technische Fahrspur ist ausgeschaltet.");
    if (feature == F::BodyTransparency)
        return QStringLiteral("Durchsicht aktiv: maximal %1 % Körperdeckkraft (%2 % transparent). "
                              "Mehr Haken machen das Boot nicht deckender.")
            .arg(graphicsProfile_.bodyOpacity * 100., 0, 'f', 0)
            .arg((1. - graphicsProfile_.bodyOpacity) * 100., 0, 'f', 0);
    if (feature == F::Water)
        return QStringLiteral("Aktiv: Basis-Wasserdeckkraft %1 %; Fresnel verändert sie blickwinkelabhängig.")
            .arg(graphicsProfile_.waterOpacity * 100., 0, 'f', 0);
    if (feature == F::Shadows || feature == F::SSAO)
        return QStringLiteral("Aktiviert; sichtbarer Effekt hängt von Ansicht und Geometrie ab. "
                              "Starke Körperdurchsicht schwächt den plastischen Eindruck.");
    return QStringLiteral("Aktiviert; gilt für alle unterstützten Teile. Sichtbare Wirkung abhängig von Szene und Ansicht.");
}

QString VtkSceneWidget::graphicsStatus() const
{
    QStringList limitations;
    if (!motionDiagnostic_.isEmpty()) limitations << motionDiagnostic_;
    if (!motionHistory_.diagnostic().isEmpty()) limitations << motionHistory_.diagnostic();
    if (graphicsProfile_.on(graphics::Feature::Wake)) {
        limitations << QStringLiteral("Kielwasser/Bugwelle: geometrischer Fahrteffekt, keine Wellenhydrodynamik.");
    }
    if (!environmentDiagnostic_.isEmpty()) limitations << environmentDiagnostic_;
    bool hasLacquer = false;
    bool hasMetal = false;
    bool hasCloth = false;
    bool hasWood = false;
    bool hasBrushed = false;
    bool hasPreview = false;
    for (const auto& body : bodies_) {
        const auto surface = effectiveSurface(body.surface, body.materialPreview);
        hasLacquer = hasLacquer || surface == DisplayPart::Surface::Lacquer;
        hasMetal = hasMetal || surface == DisplayPart::Surface::Metal
            || surface == DisplayPart::Surface::BrushedMetal;
        hasCloth = hasCloth || surface == DisplayPart::Surface::Cloth;
        hasWood = hasWood || surface == DisplayPart::Surface::Wood;
        hasBrushed = hasBrushed || surface == DisplayPart::Surface::BrushedMetal;
        hasPreview = hasPreview || body.materialPreview != 0;
    }
    if (hasPreview) {
        limitations << QStringLiteral("Explizite Materialvorschau aktiv: temporäre GUI-Zuordnung, keine bestätigte Materialeigenschaft des Modells. Holz/Stoff verwenden lokale Planarprojektion; UV-Nähte und Rückseiten sind nicht modellgebunden.");
    }
    if (graphicsProfile_.on(graphics::Feature::Cloth) && !hasCloth) {
        limitations << QStringLiteral("Stofftransparenz ist nicht anwendbar: Das Darstellungsmodell enthält keine explizite Stoff-Oberfläche; Elementnamen werden nicht ausgewertet.");
    }
    if (graphicsProfile_.on(graphics::Feature::Anisotropy) && !hasBrushed) {
        limitations << QStringLiteral("Anisotropie ist nicht anwendbar: kein Teil mit expliziter Oberfläche für gebürstetes Metall.");
    }
    if (graphicsProfile_.on(graphics::Feature::ColorTexture) && !hasLacquer && !hasWood && !hasCloth) {
        limitations << QStringLiteral("Farbtextur ist nicht anwendbar: kein Teil mit expliziter Lack-, Holz- oder Stoff-Oberfläche.");
    }
    if (graphicsProfile_.on(graphics::Feature::ORM)
        && !hasLacquer && !hasMetal && !hasWood && !hasCloth) {
        limitations << QStringLiteral("ORM-Textur ist nicht anwendbar: kein Teil mit expliziter Lack-, Metall-, Holz- oder Stoff-Oberfläche.");
    }
    if (graphicsProfile_.on(graphics::Feature::NormalTexture)
        && !hasLacquer && !hasMetal && !hasWood && !hasCloth) {
        limitations << QStringLiteral("Material-Normal-Map ist nicht anwendbar: kein Teil mit expliziter Lack-, Metall-, Holz- oder Stoff-Oberfläche.");
    }
    if ((graphicsProfile_.on(graphics::Feature::ColorTexture)
            || graphicsProfile_.on(graphics::Feature::ORM)
            || graphicsProfile_.on(graphics::Feature::NormalTexture))
        && !graphicsProfile_.on(graphics::Feature::PBR)) {
        limitations << QStringLiteral("Materialtexturen benötigen aktiviertes PBR und sind bei ausgeschaltetem PBR nicht wirksam.");
    }
    if (graphicsProfile_.on(graphics::Feature::Coat)
        && !graphicsProfile_.on(graphics::Feature::PBR)) {
        limitations << QStringLiteral("Klarlack benötigt PBR-Materialien und ist bei ausgeschaltetem PBR nicht wirksam.");
    }
    if (graphicsProfile_.on(graphics::Feature::ModelIBL)
        && !graphicsProfile_.on(graphics::Feature::PBR)) {
        limitations << QStringLiteral("Modell-IBL benötigt PBR-Materialien und ist bei ausgeschaltetem PBR nicht wirksam.");
    }
    if (graphicsProfile_.on(graphics::Feature::Animation) && !hasState_) {
        limitations << QStringLiteral("Wassernormal-Animation wartet auf einen angezeigten Simulationszeitpunkt.");
    }
    if (graphicsProfile_.on(graphics::Feature::Animation)
        && !graphicsProfile_.on(graphics::Feature::WaterNormals)) {
        limitations << QStringLiteral("Normalstruktur-Animation benötigt aktivierte Wasser-Normalstruktur.");
    }
    if ((graphicsProfile_.on(graphics::Feature::Fresnel)
            || graphicsProfile_.on(graphics::Feature::Depth)
            || graphicsProfile_.on(graphics::Feature::WaterNormals)
            || graphicsProfile_.on(graphics::Feature::WaterIBL)
            || graphicsProfile_.on(graphics::Feature::Reflection)
            || graphicsProfile_.on(graphics::Feature::Animation))
        && !graphicsProfile_.on(graphics::Feature::Water)) {
        limitations << QStringLiteral("Wasser-Untereffekte sind nicht wirksam, solange die Wasserfläche ausgeschaltet ist.");
    }
    if (graphicsProfile_.on(graphics::Feature::Reflection)) {
        limitations << QStringLiteral("Objektspiegelungen verblassen mit Abstand zur Wasseroberfläche und sind unter Wasser ausgeblendet; transparente Materialien werden nur als Alpha-Näherung gespiegelt.");
    }
    if (graphicsProfile_.on(graphics::Feature::ColorTexture)
        || graphicsProfile_.on(graphics::Feature::ORM)
        || graphicsProfile_.on(graphics::Feature::NormalTexture)) {
        limitations << QStringLiteral("Materialtexturen verwenden modellfeste Planarprojektion und prozedurale Detailmuster; Stoff enthält grafische Naht- und Stichdetails je Texturwiederholung. Modellgebundene UV-/Segelnähte und reale textile Lichtstreuung werden nicht dargestellt.");
    }
    return limitations.join(QLatin1Char('\n'));
}

void VtkSceneWidget::setReferences(double forceReference, double momentReference)
{
    forceReference_ = forcedisplay::isValidReference(forceReference) ? forceReference : 0.0;
    momentReference_ = forcedisplay::isValidReference(momentReference) ? momentReference : 0.0;
    if (hasState_) {
        ensureReferences();
    }
    updateScaling();
    geometryDirty_ = true;
    requestRender();
}

bool VtkSceneWidget::refitReferences()
{
    double maximumForce = 0.0;
    double maximumMoment = 0.0;
    for (const auto& c : forces_.contributions) {
        if (!forcedisplay::showsValues(c)) {
            continue;
        }
        const double magnitude = forcedisplay::norm(*c.vector);
        (c.kind == Kind::Force ? maximumForce : maximumMoment) =
            std::max(c.kind == Kind::Force ? maximumForce : maximumMoment, magnitude);
    }
    bool changed = false;
    if (const auto force = forcedisplay::refitReference(maximumForce)) {
        forceReference_ = *force;
        changed = true;
    }
    if (const auto moment = forcedisplay::refitReference(maximumMoment)) {
        momentReference_ = *moment;
        changed = true;
    }
    if (changed) {
        updateScaling();
        geometryDirty_ = true;
        requestRender();
        if (onReferencesChanged) {
            onReferencesChanged();
        }
    }
    return changed;
}

std::optional<forcedisplay::ScaledMagnitude> VtkSceneWidget::scaleInfo(const QString& id) const
{
    const auto it = scaled_.constFind(id);
    if (it == scaled_.constEnd()) {
        return std::nullopt;
    }
    return *it;
}

void VtkSceneWidget::setSelected(const QString& id)
{
    if (selected_ == id) {
        return;
    }
    selected_ = id;
    geometryDirty_ = true;
    requestRender();
}

void VtkSceneWidget::requestRender()
{
    if (!bodies_.empty()) {
        renderScene();
    }
}

void VtkSceneWidget::ensureReferences()
{
    bool changed = false;
    double maximumForce = 0.0;
    double maximumMoment = 0.0;
    bool momentDelivered = false;
    for (const auto& c : forces_.contributions) {
        if (!forcedisplay::showsValues(c)) {
            continue;
        }
        const double magnitude = forcedisplay::norm(*c.vector);
        if (c.kind == Kind::Force) {
            maximumForce = std::max(maximumForce, magnitude);
        } else {
            maximumMoment = std::max(maximumMoment, magnitude);
            momentDelivered = true;
        }
    }
    if (forceReference_ <= 0.0) {
        // Ohne Masse aus dem Katalog: größte Kraft des ersten Zustands.
        forceReference_ = std::max(maximumForce, 1.0);
        changed = true;
    }
    if (momentReference_ <= 0.0 && momentDelivered) {
        // R_M aus dem ersten gültigen Zustand; ein (nahezu) verschwindendes
        // Moment im Gleichgewicht ergibt ersatzweise R_F · halbe Hüllquaderdiagonale.
        const double lever = std::max(bodyHalfDiagonal(), 1e-3);
        const double fallback = std::max(forceReference_ * lever, 1.0);
        momentReference_ = maximumMoment > 1e-6 * fallback ? maximumMoment : fallback;
        changed = true;
    }
    if (changed && onReferencesChanged) {
        onReferencesChanged();
    }
}

void VtkSceneWidget::updateScaling()
{
    scaled_.clear();
    for (const auto& c : forces_.contributions) {
        if (!forcedisplay::showsValues(c)) {
            continue;
        }
        const bool force = c.kind == Kind::Force;
        const double reference = force ? forceReference_ : momentReference_;
        if (!forcedisplay::isValidReference(reference)) {
            continue;
        }
        scaled_.insert(c.id, forcedisplay::scaleMagnitude(forcedisplay::norm(*c.vector),
                                 reference, force ? options_.forceMode : options_.momentMode));
    }
}

bool VtkSceneWidget::contributionVisible(const forcedisplay::Contribution& c) const
{
    const auto& selection = c.kind == Kind::Force ? options_.force : options_.moment;
    switch (c.category) {
    case Category::Weight:
        return c.total ? selection.weightTotal : selection.weightParts;
    case Category::Buoyancy:
        return c.total ? selection.buoyancyTotal : selection.buoyancyParts;
    case Category::ForceSum:
    case Category::MomentSum:
        return selection.sum;
    case Category::Test:
    case Category::Unassigned:
    case Category::Other:
        // Weitere Quellen (derzeit keine) bleiben sichtbar und auffällig gefärbt.
        return true;
    }
    return true;
}

vtkRenderer* VtkSceneWidget::forceRenderer() const
{
    return overlay_;
}

void VtkSceneWidget::moveForceActors()
{
    overlay_->SetPreserveDepthBuffer(options_.visibility == Visibility::Spatial);
}

void VtkSceneWidget::setInspectionMode()
{
    inspectionMode_ = true;
    setStyleSheet(QStringLiteral("border:1px solid #704554; border-radius:12px;"));
    layout()->setContentsMargins(1, 1, 1, 1);
    updateInspectionMask();
    inspectionAxes_ = vtkSmartPointer<vtkAxesActor>::New();
    inspectionAxes_->SetXAxisLabelText("X");
    inspectionAxes_->SetYAxisLabelText("Y");
    inspectionAxes_->SetZAxisLabelText("Z");
    const std::array<std::array<double, 3>, 3> axisColors = {{
        {0.91, 0.49, 0.47}, {0.51, 0.78, 0.61}, {0.51, 0.71, 0.95}}};
    const std::array<vtkCaptionActor2D*, 3> captions = {{
        inspectionAxes_->GetXAxisCaptionActor2D(), inspectionAxes_->GetYAxisCaptionActor2D(),
        inspectionAxes_->GetZAxisCaptionActor2D()}};
    for (std::size_t i = 0; i < captions.size(); ++i) {
        captions[i]->GetTextActor()->SetTextScaleModeToNone();
        auto* text = captions[i]->GetCaptionTextProperty();
        text->SetFontSize(14);
        text->SetColor(axisColors[i].data());
        text->ShadowOff();
        text->ItalicOff();
        text->BoldOff();
    }
    inspectionAxes_->PickableOff();
    inspectionAxes_->UseBoundsOff();
    renderer_->AddActor(inspectionAxes_);
    setNavigationVisible(false);
    auto options = options_;
    options.forces = false;
    options.leverArm = false;
    options.lateralCenter = false;
    setOptions(options);
    auto profile = graphics::defaults(1);
    profile.environmentPath.clear();
    for (auto feature : {graphics::Feature::Axes, graphics::Feature::Trail,
             graphics::Feature::WaterTrack, graphics::Feature::Wake,
             graphics::Feature::Waterline, graphics::Feature::Reflection,
             graphics::Feature::Water, graphics::Feature::Sky,
             graphics::Feature::BodyTransparency})
        profile.enabled[static_cast<std::size_t>(feature)] = false;
    profile.enabled[static_cast<std::size_t>(graphics::Feature::Grid)] = true;
    proceduralSkyTexture_ = makeSkyTexture(true);
    skyTexture_ = proceduralSkyTexture_;
    loadedEnvironmentPath_.clear();
    renderer_->SetEnvironmentTexture(skyTexture_);
    skybox_->SetTexture(skyTexture_);
    setGraphicsProfile(profile);
    rebuildEnvironment();
    requestRender();
}

void VtkSceneWidget::setInspectionSelection(const QSet<QString>& elementIds, bool hasSelection)
{
    inspectionElements_ = elementIds;
    inspectionHasSelection_ = hasSelection;
    applyBodyStyle();
    requestRender();
}

double VtkSceneWidget::inspectionOpacity(const QString& elementId) const
{
    for (const auto& body : bodies_)
        if (body.elementId == elementId) return body.body->GetProperty()->GetOpacity();
    return 0.0;
}

void VtkSceneWidget::setInspectionPreview(const QVector<DisplayPart>& parts,
    const std::vector<std::array<forcedisplay::Vec3, 2>>& axes,
    const std::vector<forcedisplay::Vec3>& pivots)
{
    for (const auto& actor : inspectionPreviewActors_) renderer_->RemoveActor(actor);
    inspectionPreviewActors_.clear();
    for (const auto& part : parts) {
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(part.mesh);
        mapper->ScalarVisibilityOff();
        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->PickableOff();
        actor->UseBoundsOff();
        actor->GetProperty()->SetColor(part.color[0], part.color[1], part.color[2]);
        actor->GetProperty()->SetOpacity(0.55);
        actor->GetProperty()->SetRepresentationToWireframe();
        actor->GetProperty()->LightingOff();
        renderer_->AddActor(actor);
        inspectionPreviewActors_.push_back(actor);
    }
    if (!axes.empty()) {
        vtkNew<vtkPoints> points;
        vtkNew<vtkCellArray> lines;
        for (const auto& axis : axes) {
            const vtkIdType ids[2] = {points->InsertNextPoint(axis[0].data()),
                points->InsertNextPoint(axis[1].data())};
            lines->InsertNextCell(2, ids);
        }
        vtkNew<vtkPolyData> mesh;
        mesh->SetPoints(points);
        mesh->SetLines(lines);
        auto actor = makeOverlayActor(mesh);
        actor->PickableOff();
        actor->UseBoundsOff();
        actor->GetProperty()->SetColor(0.95, 0.85, 0.35);
        actor->GetProperty()->SetLineWidth(3.0f);
        renderer_->AddActor(actor);
        inspectionPreviewActors_.push_back(actor);
    }
    if (!pivots.empty()) {
        vtkNew<vtkPoints> points;
        vtkNew<vtkCellArray> vertices;
        for (const auto& point : pivots) {
            const auto id = points->InsertNextPoint(point.data());
            vertices->InsertNextCell(1, &id);
        }
        vtkNew<vtkPolyData> mesh;
        mesh->SetPoints(points);
        mesh->SetVerts(vertices);
        auto actor = makeOverlayActor(mesh);
        actor->PickableOff();
        actor->UseBoundsOff();
        actor->GetProperty()->SetColor(1., .85, .2);
        actor->GetProperty()->SetPointSize(14.f);
        actor->GetProperty()->RenderPointsAsSpheresOn();
        renderer_->AddActor(actor);
        inspectionPreviewActors_.push_back(actor);
    }
    requestRender();
}

void VtkSceneWidget::applyBodyStyle()
{
    for (auto& body : bodies_) {
        auto* property = body.body->GetProperty();
        property->SetColor(body.baseColor[0], body.baseColor[1], body.baseColor[2]);
        const auto surface = effectiveSurface(body.surface, body.materialPreview);
        property->SetLighting(true);
        property->SetRepresentationToSurface();
        property->SetEdgeVisibility(false);
        property->SetAnisotropy(0.0);
        property->SetCoatStrength(0.0);
        const double materialOpacity = applySurface(property, surface);
        const bool cloth = surface == DisplayPart::Surface::Cloth;
        const bool wood = surface == DisplayPart::Surface::Wood;
        const bool brushed = surface == DisplayPart::Surface::BrushedMetal;
        const bool pbr = graphicsProfile_.on(graphics::Feature::PBR);
        if (cloth || wood) {
            if (pbr && graphicsProfile_.on(graphics::Feature::ColorTexture)) {
                property->SetColor(1.0, 1.0, 1.0);
            }
            property->SetRoughness(cloth ? 0.95 : 0.75);
        }
        if (brushed) {
            if (body.materialPreview != 0) property->SetColor(0.72, 0.74, 0.77);
            property->SetRoughness(0.35);
            property->SetAnisotropy(pbr && graphicsProfile_.on(graphics::Feature::Anisotropy) ? 0.72 : 0.0);
        }
        property->SetInterpolation(graphicsProfile_.on(graphics::Feature::PBR) ? VTK_PBR : VTK_GOURAUD);
        if (!graphicsProfile_.on(graphics::Feature::Coat)) {
            property->SetCoatStrength(0.0);
        }
        const bool lacquer = surface == DisplayPart::Surface::Lacquer;
        const bool explicitTextureMaterial = lacquer || surface == DisplayPart::Surface::Metal || brushed
            || wood || cloth;
        auto* colorTexture = wood ? woodColorTexture_.Get()
            : cloth ? clothColorTexture_.Get() : proceduralColorTexture_.Get();
        auto* ormTexture = wood ? woodOrmTexture_.Get()
            : cloth ? clothOrmTexture_.Get() : proceduralOrmTexture_.Get();
        auto* normalTexture = wood ? woodNormalTexture_.Get()
            : cloth ? clothNormalTexture_.Get() : proceduralNormalTexture_.Get();
        setMaterialTexture(property, "albedoTex",
            pbr && graphicsProfile_.on(graphics::Feature::ColorTexture) && (lacquer || wood || cloth)
                ? colorTexture : nullptr);
        setMaterialTexture(property, "materialTex",
            pbr && graphicsProfile_.on(graphics::Feature::ORM) && explicitTextureMaterial
                ? ormTexture : nullptr);
        setMaterialTexture(property, "normalTex",
            pbr && graphicsProfile_.on(graphics::Feature::NormalTexture) && explicitTextureMaterial
                ? normalTexture : nullptr);
        property->SetNormalScale(static_cast<float>(graphicsProfile_.normalScale));
        const bool bodyTransparency = graphicsProfile_.on(graphics::Feature::BodyTransparency);
        const double surfaceOpacity = cloth && graphicsProfile_.on(graphics::Feature::Cloth)
            ? 0.88 : materialOpacity;
        const double opacity = inspectionMode_ && inspectionHasSelection_
            ? (inspectionElements_.contains(body.elementId) ? 1.0 : 0.15)
            : bodyTransparency ? std::min(surfaceOpacity, graphicsProfile_.bodyOpacity) : surfaceOpacity;
        property->SetOpacity(opacity);
        if (orthographicView_ != OrthographicView::Free && !inspectionMode_) {
            property->SetColor(body.baseColor[0], body.baseColor[1], body.baseColor[2]);
            property->SetOpacity(1.0);
            property->SetAmbient(0.55);
            property->SetDiffuse(0.45);
            property->SetSpecular(0.0);
            body.waterline->GetProperty()->SetColor(0.0, 0.35, 0.65);
        }

        auto* reflectionProperty = body.reflection->GetProperty();
        copyMaterial(reflectionProperty, property);
        reflectionProperty->SetOpacity(0.24 * opacity);
        reflectionProperty->SetAmbient(0.18);
        reflectionProperty->SetDiffuse(0.75);
        reflectionProperty->SetSpecular(0.35);
        setActorEnvironmentLighting(body.body, graphicsProfile_.on(graphics::Feature::ModelIBL));
        setActorEnvironmentLighting(body.reflection, graphicsProfile_.on(graphics::Feature::ModelIBL));
        body.waterline->SetVisibility(
            graphicsProfile_.on(graphics::Feature::Waterline) && body.body->GetVisibility());
    }
    updateReflectionVisibility();
}

void VtkSceneWidget::updateReflectionVisibility()
{
    const bool visible = renderer_->GetActiveCamera()->GetPosition()[2] >= 0.0
        && graphicsProfile_.on(graphics::Feature::Water)
        && graphicsProfile_.on(graphics::Feature::Reflection);
    for (auto& body : bodies_)
        body.reflection->SetVisibility(visible && body.body->GetVisibility());
}

void VtkSceneWidget::applyGraphicsProfile()
{
    updateEnvironmentTexture();
    const bool water = graphicsProfile_.on(graphics::Feature::Water);
    waterActor_->SetVisibility(water);
    deepActor_->SetVisibility(water && graphicsProfile_.on(graphics::Feature::Depth));
    gridActor_->SetVisibility(graphicsProfile_.on(graphics::Feature::Grid));
    axesActor_->SetVisibility(graphicsProfile_.on(graphics::Feature::Axes));
    skybox_->SetVisibility(graphicsProfile_.on(graphics::Feature::Sky));

    if (graphicsProfile_.on(graphics::Feature::Sky)) {
        renderer_->GradientBackgroundOn();
    } else {
        renderer_->GradientBackgroundOff();
        renderer_->SetBackground(0.055, 0.07, 0.09);
        renderer_->SetBackground2(0.055, 0.07, 0.09);
    }
    if (inspectionMode_) {
        renderer_->SetBackground(0.125, 0.094, 0.125);
        renderer_->SetBackground2(0.22, 0.137, 0.18);
        renderer_->GradientBackgroundOn();
    }
    if (orthographicView_ != OrthographicView::Free)
        renderer_->SetBackground(0.86, 0.90, 0.94);
    renderer_->SetUseFXAA(graphicsProfile_.on(graphics::Feature::FXAA));
    renderer_->SetUseSSAO(graphicsProfile_.on(graphics::Feature::SSAO));
    renderer_->SetUseDepthPeeling(graphicsProfile_.on(graphics::Feature::Peeling));
    renderer_->SetMaximumNumberOfPeels(graphicsProfile_.quality == 0 ? 4
            : graphicsProfile_.quality == 1 ? 8 : 16);
    renderer_->SetSSAOKernelSize(graphicsProfile_.quality == 0 ? 24
            : graphicsProfile_.quality == 1 ? 48 : 64);
    renderer_->SetUseImageBasedLighting(
        graphicsProfile_.on(graphics::Feature::ModelIBL)
            || graphicsProfile_.on(graphics::Feature::WaterIBL));
    // VTK's global UseShadows path skips translucency and SSAO. Compose shadows
    // only into the opaque stage instead, preserving the remaining render steps.
    renderer_->SetUseShadows(false);
    auto* sceneRenderer = SceneRenderer::SafeDownCast(renderer_);
    sceneRenderer->sunShadows = graphicsProfile_.on(graphics::Feature::Shadows);
    sceneRenderer->shadowResolution = graphicsProfile_.quality == 0 ? 512
        : graphicsProfile_.quality == 1 ? 1024 : 2048;
    const bool keyLight = graphicsProfile_.on(graphics::Feature::Lights)
        || graphicsProfile_.on(graphics::Feature::Shadows);
    sunLight_->SetSwitch(keyLight);
    fillLight_->SetSwitch(graphicsProfile_.on(graphics::Feature::Lights));
    waterActor_->GetProperty()->SetNormalTexture(
        graphicsProfile_.on(graphics::Feature::WaterNormals) ? waterNormalTexture_.Get() : nullptr);
    waterActor_->GetProperty()->SetNormalScale(static_cast<float>(graphicsProfile_.normalScale));
    waterActor_->GetProperty()->SetInterpolationToPBR();
    waterActor_->GetProperty()->SetMetallic(0.0);
    waterActor_->GetProperty()->SetBaseIOR(1.333);
    waterActor_->GetProperty()->SetRoughness(0.12);
    if (orthographicView_ != OrthographicView::Free) {
        waterActor_->GetProperty()->SetInterpolationToGouraud();
        waterActor_->GetProperty()->LightingOff();
        waterActor_->GetProperty()->SetSpecular(0.0);
    }
    setActorEnvironmentLighting(waterActor_, graphicsProfile_.on(graphics::Feature::WaterIBL));
    for (auto& body : bodies_) {
        body.normals->SplittingOn();
        body.normals->SetFeatureAngle(graphicsProfile_.on(graphics::Feature::Smooth) ? 35.0 : 0.0);
        body.reflectionClipper->SetClipFunction(body.localWaterPlane);
    }
    applyBodyStyle();
    updateWaterAnimation();
    if (!graphicsProfile_.on(graphics::Feature::Trail)) {
        trailActor_->VisibilityOff();
    } else if (hasState_) {
        updateTrail();
    }
    geometryDirty_ = true;
    updateWaterMotion();
}

void VtkSceneWidget::updateEnvironmentTexture()
{
    const QString requested = graphicsProfile_.environmentPath;
    if (requested == loadedEnvironmentPath_ && skyTexture_) return;
    loadedEnvironmentPath_ = requested;
    environmentDiagnostic_.clear();
    skyTexture_ = proceduralSkyTexture_;
    if (!requested.isEmpty()) {
        const QFileInfo file(requested);
        vtkNew<vtkHDRReader> reader;
        const auto path = file.absoluteFilePath().toUtf8();
        if (!file.isFile() || !file.isReadable() || !reader->CanReadFile(path.constData())) {
            environmentDiagnostic_ = QStringLiteral("HDR-Umgebung nicht lesbar oder ungültig: %1. Sichtbarer Ersatz: prozedurale Umgebung; der Dateiwunsch bleibt erhalten.").arg(requested);
        } else {
            reader->SetFileName(path.constData());
            reader->Update();
            auto* image = reader->GetOutput();
            if (reader->GetErrorCode() != 0 || !image || image->GetNumberOfPoints() == 0) {
                environmentDiagnostic_ = QStringLiteral("HDR-Umgebung konnte nicht geladen werden: %1. Sichtbarer Ersatz: prozedurale Umgebung; der Dateiwunsch bleibt erhalten.").arg(requested);
            } else {
                auto texture = vtkSmartPointer<vtkTexture>::New();
                texture->SetInputData(image);
                texture->InterpolateOn();
                texture->MipmapOn();
                texture->RepeatOn();
                skyTexture_ = texture;
            }
        }
    }
    renderer_->SetEnvironmentTexture(skyTexture_);
    skybox_->SetTexture(skyTexture_);
}

void VtkSceneWidget::updateWaterAnimation()
{
    const bool animate = graphicsProfile_.on(graphics::Feature::Animation)
        && graphicsProfile_.on(graphics::Feature::WaterNormals) && hasState_;
    waterNormalTransform_->Identity();
    if (animate) {
        waterNormalTransform_->Translate(0.012 * currentTime_, 0.007 * currentTime_, 0.0);
    }
    waterNormalTransform_->Modified();
}

VtkSceneWidget::Visual& VtkSceneWidget::visualFor(const QString& id)
{
    auto it = visuals_.find(id);
    if (it != visuals_.end()) {
        return *it;
    }
    Visual visual;
    visual.lines = vtkSmartPointer<vtkPolyData>::New();
    visual.head = vtkSmartPointer<vtkPolyData>::New();
    visual.lineActor = makeOverlayActor(visual.lines);
    visual.headActor = makeOverlayActor(visual.head);
    forceRenderer()->AddActor(visual.lineActor);
    forceRenderer()->AddActor(visual.headActor);
    return *visuals_.insert(id, visual);
}

void VtkSceneWidget::hideForceVisuals()
{
    centerLabel_->VisibilityOff();
    buoyancyLabel_->VisibilityOff();
    for (auto& visual : visuals_) {
        visual.lineActor->VisibilityOff();
        visual.headActor->VisibilityOff();
    }
    leverActor_->VisibilityOff();
    lateralActor_->VisibilityOff();
    lateralLabel_->VisibilityOff();
    sailActor_->VisibilityOff();
    sailLabel_->VisibilityOff();
    pressurePoints_ = {};
}

double VtkSceneWidget::worldPerLogicalPixel(const Vec3& point) const
{
    auto* camera = renderer_->GetActiveCamera();
    const int* size = renderer_->GetSize();
    if (size == nullptr || size[1] <= 0) {
        return 0.0;
    }
    const double ratio = vtkWidget_->devicePixelRatioF();
    double perDevicePixel = 0.0;
    if (camera->GetParallelProjection() != 0) {
        perDevicePixel = 2.0 * camera->GetParallelScale() / size[1];
    } else {
        double position[3];
        double direction[3];
        camera->GetPosition(position);
        camera->GetDirectionOfProjection(direction);
        const double depth = (point[0] - position[0]) * direction[0]
            + (point[1] - position[1]) * direction[1]
            + (point[2] - position[2]) * direction[2];
        if (!(depth > 1e-9)) {
            return 0.0;
        }
        perDevicePixel = 2.0 * depth * std::tan(camera->GetViewAngle() * pi / 360.0) / size[1];
    }
    return perDevicePixel * ratio;
}

QPointF VtkSceneWidget::toLogicalScreen(const Vec3& point) const
{
    renderer_->SetWorldPoint(point[0], point[1], point[2], 1.0);
    renderer_->WorldToDisplay();
    double display[3];
    renderer_->GetDisplayPoint(display);
    const double ratio = vtkWidget_->devicePixelRatioF();
    const int* size = renderWindow_->GetSize();
    return {display[0] / ratio, (size[1] - display[1]) / ratio};
}

void VtkSceneWidget::onRenderStart(vtkObject*, unsigned long, void*)
{
    ++workCounters_.renders;
    if (!updatesActive_) return;
    updateOrthographicCamera();
    auto* camera = renderer_->GetActiveCamera();
    std::array<double, 15> key{};
    camera->GetPosition(&key[0]);
    camera->GetFocalPoint(&key[3]);
    camera->GetViewUp(&key[6]);
    key[9] = camera->GetViewAngle();
    key[10] = camera->GetParallelScale();
    key[11] = camera->GetParallelProjection();
    const int* size = renderWindow_->GetSize();
    key[12] = size[0];
    key[13] = size[1];
    key[14] = vtkWidget_->devicePixelRatioF();
    if (key != cameraKey_ || geometryDirty_) {
        cameraKey_ = key;
        geometryDirty_ = false;
        rebuildForceGeometry();
        rebuildCourseArrow();
        rebuildWindArrows();
        rebuildEnvironment();
        double bounds[6];
        double overlayBounds[6];
        renderer_->ComputeVisiblePropBounds(bounds);
        overlay_->ComputeVisiblePropBounds(overlayBounds);
        if (overlayBounds[0] <= overlayBounds[1]) {
            if (bounds[0] > bounds[1]) {
                std::copy(overlayBounds, overlayBounds + 6, bounds);
            } else {
                for (int axis = 0; axis < 3; ++axis) {
                    bounds[axis * 2] = std::min(bounds[axis * 2], overlayBounds[axis * 2]);
                    bounds[axis * 2 + 1] =
                        std::max(bounds[axis * 2 + 1], overlayBounds[axis * 2 + 1]);
                }
            }
        }
        if (bounds[0] <= bounds[1]) {
            renderer_->ResetCameraClippingRange(bounds);
        }
    }
    // Wasser und Raster zählen nicht zu den Grenzen; die Fernebene reicht bis
    // zum Horizont, die Nahebene bleibt am Modell.
    double range[2];
    camera->GetClippingRange(range);
    const double far = horizonRadius_ * 1.05 + camera->GetDistance();
    const double near = std::max(std::min(range[0], 0.1 * camera->GetDistance()), far * 1e-3);
    if (far > range[1] || near < range[0]) {
        camera->SetClippingRange(near, std::max(far, range[1]));
    }

    double direction[3];
    double up[3];
    camera->GetDirectionOfProjection(direction);
    camera->GetViewUp(up);
    auto* axesCamera = axesRenderer_->GetActiveCamera();
    axesCamera->SetFocalPoint(0.35, 0.35, 0.35);
    axesCamera->SetPosition(0.35 - 5.0 * direction[0], 0.35 - 5.0 * direction[1], 0.35 - 5.0 * direction[2]);
    axesCamera->SetViewUp(up);
    axesCamera->SetClippingRange(0.5, 10.0);
    const double sidePx = 110.0 * vtkWidget_->devicePixelRatioF();
    if (size[0] > 0 && size[1] > 0) {
        axesRenderer_->SetViewport(0.0, 0.0, std::min(1.0, sidePx / size[0]), std::min(1.0, sidePx / size[1]));
    }
    const auto view = cameraView();
    auto navigationValues = navigation::derive(
        hasState_ ? std::optional<forcedisplay::Pose>(pose_) : std::nullopt,
        forcedisplay::sub(view.position, view.focalPoint), velocityWorld_);
    if (hasState_) {
        navigationValues.courseRateRadS = forces_.courseRateRadS;
        navigationValues.courseRateStatus = forces_.courseRateStatus;
        navigationValues.approximate = !forces_.stationary.isEmpty()
            && !forces_.stationary.value(QStringLiteral("konvergiert")).toBool();
        readWindNavigation(currentState_, navigationValues);
    }
    navigation_->setValues(navigationValues);
}

void VtkSceneWidget::rebuildWindArrows()
{
    windArrowGeometry_ = {};
    for (auto& visual : windVisuals_) {
        visual.lineActor->VisibilityOff();
        visual.headActor->VisibilityOff();
    }
    if (!options_.windArrows || !hasState_ || inspectionMode_) return;
    std::optional<Vec3> mast;
    for (const auto& entry : currentState_.value(QStringLiteral("referenzpunkte")).toArray()) {
        const auto point = entry.toObject();
        if (point.value(QStringLiteral("id")).toString() == QStringLiteral("mastspitze") &&
            point.value(QStringLiteral("positionsstatus")).toString() == QStringLiteral("gueltig")) {
            mast = readStateVector(point, QStringLiteral("position_welt_m"));
            break;
        }
    }
    if (!mast) return;
    const auto wind = currentState_.value(QStringLiteral("wind")).toObject();
    const std::array<QString, 2> names{QStringLiteral("wahr"), QStringLiteral("scheinbar")};
    for (std::size_t i = 0; i < names.size(); ++i) {
        const auto value = wind.value(names[i]).toObject();
        if (!value.value(QStringLiteral("pfeil_sichtbar")).toBool()) continue;
        const auto vector = readStateVector(value, QStringLiteral("stroemung_welt_m_s"));
        const double magnitude = forcedisplay::norm(vector);
        if (!std::isfinite(magnitude) || magnitude <= .1) continue;
        const double length = magnitude * .1;
        const double headLength = length * .25;
        windArrowGeometry_[i] = forcedisplay::arrowGeometry(*mast, vector, length, headLength);
        if (!windArrowGeometry_[i]) continue;
        const auto& arrow = *windArrowGeometry_[i];
        PolyBuilder line;
        PolyBuilder head;
        line.segment(arrow.start, arrow.headBase);
        head.cone(arrow.tip, arrow.direction, headLength, headLength * .4);
        line.storeLines(windVisuals_[i].lines);
        head.storePolys(windVisuals_[i].head);
        windVisuals_[i].lineActor->VisibilityOn();
        windVisuals_[i].headActor->VisibilityOn();
    }
}

void VtkSceneWidget::rebuildCourseArrow()
{
    courseArrowActor_->VisibilityOff();
    courseArrowHeadActor_->VisibilityOff();
    courseArrowGeometry_.reset();
    const bool staticDirectionView = options_.staticForces
        && (orthographicView_ == OrthographicView::Free || orthographicView_ == OrthographicView::Top);
    if (!options_.courseArrow || inspectionMode_ || (!staticDirectionView && navigation_->isHidden()) || !hasState_
        || !forces_.courseRateRadS || bodies_.empty())
        return;
    if (options_.staticForces && !staticDirectionView) return;
    const auto color = options_.staticForces
        ? options_.staticStyles[static_cast<std::size_t>(forcedisplay::StaticForce::Direction)].color
        : QStringLiteral("#73f3d4");
    setActorColor(courseArrowActor_, color);
    setActorColor(courseArrowHeadActor_, color);
    double front = -std::numeric_limits<double>::infinity();
    double back = std::numeric_limits<double>::infinity();
    double left = back, right = front;
    for (const auto& body : bodies_) {
        double bounds[6];
        body.mesh->GetBounds(bounds);
        back = std::min(back, bounds[0]);
        front = std::max(front, bounds[1]);
        left = std::min(left, bounds[2]);
        right = std::max(right, bounds[3]);
    }
    const double length = front - back;
    if (!std::isfinite(length) || length <= 0.) return;
    const auto forward = forcedisplay::toWorldVector(pose_, {1., 0., 0.});
    const double horizontal = std::hypot(forward[0], forward[1]);
    if (horizontal <= 1e-10) return;
    auto start = forcedisplay::toWorldPoint(pose_, {front, .5 * (left + right), 0.});
    start[0] += .07 * length * forward[0] / horizontal;
    start[1] += .07 * length * forward[1] / horizontal;
    start[2] = .015 * length;
    courseArrowGeometry_ = forcedisplay::courseArrow(
        start, forward, *forces_.courseRateRadS, .45 * length);
    if (!courseArrowGeometry_) return;
    const auto& arrow = *courseArrowGeometry_;
    const double headLength = .05 * length;
    auto points = arrow.points;
    while (points.size() > 1 && forcedisplay::norm(
        forcedisplay::sub(points.back(), arrow.tip)) < headLength)
        points.removeLast();
    points.push_back(forcedisplay::sub(arrow.tip, forcedisplay::scale(arrow.tangent, headLength)));
    PolyBuilder lines, head;
    lines.polyline(points);
    head.cone(arrow.tip, arrow.tangent, headLength, .025 * length);
    lines.storeLines(courseArrowLines_);
    head.storePolys(courseArrowHead_);
    courseArrowActor_->GetProperty()->SetLineWidth(
        static_cast<float>(5. * vtkWidget_->devicePixelRatioF()));
    courseArrowActor_->VisibilityOn();
    courseArrowHeadActor_->VisibilityOn();
}

void VtkSceneWidget::rebuildSchematicForces()
{
    if (orthographicView_ == OrthographicView::Free && !options_.staticForces) return;
    if (!hasState_ || bodies_.empty() || inspectionMode_) return;
    const auto forceArrow = [this](const Vec3& start, const Vec3& worldForce,
                                  const QString& id, const QString& color) {
        auto* camera = renderer_->GetActiveCamera();
        const auto* direction = camera->GetDirectionOfProjection();
        const Vec3 normal{direction[0], direction[1], direction[2]};
        const auto projected = orthographicView_ == OrthographicView::Free ? worldForce
            : forcedisplay::sub(worldForce,
                forcedisplay::scale(normal, forcedisplay::dot(worldForce, normal)));
        const double magnitude = forcedisplay::norm(projected);
        const double perPixel = worldPerLogicalPixel(start);
        if (magnitude < 1e-8 || perPixel <= 0.) return;
        const auto scaled = forcedisplay::scaleMagnitude(
            magnitude, std::max(forceReference_, 1.0), forcedisplay::ScaleMode::Logarithmic);
        const double length = forcedisplay::pixelLength(scaled,
            std::clamp(vtkWidget_->width() * .45, 28., 100.), 12.) * perPixel;
        const double headLength = std::min(7. * perPixel, length * .4);
        const auto arrow = forcedisplay::arrowGeometry(start, projected, length, headLength);
        if (!arrow) return;
        PolyBuilder lines, head;
        lines.segment(start, arrow->headBase);
        head.cone(arrow->tip, arrow->direction, headLength, headLength * .4);
        auto& visual = visualFor(id);
        lines.storeLines(visual.lines);
        head.storePolys(visual.head);
        for (auto* actor : {visual.lineActor.GetPointer(), visual.headActor.GetPointer()}) {
            setActorColor(actor, color);
            actor->GetProperty()->SetLineWidth(static_cast<float>(2. * vtkWidget_->devicePixelRatioF()));
            actor->VisibilityOn();
        }
    };
    const auto marker = [this](const Vec3& center, const QString& id,
                              const QString& color, vtkBillboardTextActor3D* label) {
        auto& visual = visualFor(id);
        vtkNew<vtkPoints> points;
        points->SetDataTypeToDouble();
        const auto point = points->InsertNextPoint(center.data());
        vtkNew<vtkCellArray> vertices;
        vertices->InsertNextCell(1, &point);
        visual.lines->Initialize();
        visual.lines->SetPoints(points);
        visual.lines->SetVerts(vertices);
        setActorColor(visual.lineActor, color);
        visual.lineActor->GetProperty()->SetPointSize(
            static_cast<float>(11. * vtkWidget_->devicePixelRatioF()));
        visual.lineActor->GetProperty()->RenderPointsAsSpheresOn();
        visual.lineActor->VisibilityOn();
        label->SetPosition(center[0], center[1], center[2]);
        label->SetDisplayOffset(5, 5);
        const QColor tint(color);
        label->GetTextProperty()->SetColor(tint.redF(), tint.greenF(), tint.blueF());
        label->GetTextProperty()->SetFontSize(
            static_cast<int>(13. * vtkWidget_->devicePixelRatioF()));
        label->VisibilityOn();
    };
    const auto sums = forcedisplay::staticResultants(forces_, pose_, hydrodynamicRoles_);
    for (std::size_t i = 0; i < sums.size(); ++i) {
        const auto type = static_cast<forcedisplay::StaticForce>(i);
        const auto& style = options_.staticStyles[i];
        const auto& sum = sums[i];
        if (!style.enabled) continue;
        if (type == forcedisplay::StaticForce::Weight && forces_.reference
            && !forces_.stationary.isEmpty())
            marker(forcedisplay::toWorldPoint(pose_, *forces_.reference),
                QStringLiteral("schematic:center"), style.color, centerLabel_);
        if (!sum.pointWorld || !sum.forceWorld) continue;
        QString id = QStringLiteral("static:") + style.key;
        if (type == forcedisplay::StaticForce::Buoyancy) {
            marker(*sum.pointWorld, QStringLiteral("schematic:buoyancy"), style.color, buoyancyLabel_);
            id = QStringLiteral("schematic:buoyancy-force");
        } else if (type == forcedisplay::StaticForce::Weight) {
            id = QStringLiteral("schematic:weight-force");
        } else if (type == forcedisplay::StaticForce::Lateral) {
            if (!pressurePoints_[0]) continue;
            id = QStringLiteral("schematic:ldp-force");
        } else if (type == forcedisplay::StaticForce::Sail) {
            if (!pressurePoints_[1]) continue;
            id = QStringLiteral("schematic:sdp-force");
        }
        forceArrow(*sum.pointWorld, *sum.forceWorld, id, style.color);
    }
}

void VtkSceneWidget::rebuildForceGeometry()
{
    hideForceVisuals();
    screenShapes_.clear();
    if (hasState_ && !bodies_.empty() && !inspectionMode_) {
        const auto marker = [this](const Vec3& local, bool sail) {
            const auto point = forcedisplay::toWorldPoint(pose_, local);
            pressurePoints_[sail ? 1 : 0] = point;
            auto* data = sail ? sailPoint_.GetPointer() : lateralLines_.GetPointer();
            auto* actor = sail ? sailActor_.GetPointer() : lateralActor_.GetPointer();
            auto* label = sail ? sailLabel_.GetPointer() : lateralLabel_.GetPointer();
            vtkNew<vtkPoints> points;
            points->SetDataTypeToDouble();
            const auto id = points->InsertNextPoint(point.data());
            vtkNew<vtkCellArray> vertices;
            vertices->InsertNextCell(1, &id);
            data->Initialize();
            data->SetPoints(points);
            data->SetVerts(vertices);
            const bool auxiliary = orthographicView_ != OrthographicView::Free;
            const QColor color(options_.staticForces || auxiliary
                ? options_.staticStyles[static_cast<std::size_t>(sail
                    ? forcedisplay::StaticForce::Sail : forcedisplay::StaticForce::Lateral)].color
                : (sail ? QStringLiteral("#ffad48") : QStringLiteral("#ff4fd8")));
            actor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
            actor->GetProperty()->SetPointSize(static_cast<float>(11. * vtkWidget_->devicePixelRatioF()));
            actor->GetProperty()->RenderPointsAsSpheresOn();
            label->SetPosition(point[0], point[1], point[2]);
            label->GetTextProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
            label->GetTextProperty()->SetFontSize(
                static_cast<int>((auxiliary ? 11. : 16.) * vtkWidget_->devicePixelRatioF()));
            if (auxiliary) label->GetTextProperty()->SetJustificationToCentered();
            else label->GetTextProperty()->SetJustificationToLeft();
            label->GetTextProperty()->SetVerticalJustificationToBottom();
            actor->VisibilityOn();
            label->VisibilityOn();
        };
        if (options_.lateralCenter && forces_.lateral && forces_.lateral->position
            && forces_.lateral->status == QStringLiteral("gueltig")
            && forces_.lateral->evaluationVersion == 2)
            marker(*forces_.lateral->position, false);
        if (options_.sailCenter && forces_.sailCenter && forces_.sailCenter->position)
            marker(*forces_.sailCenter->position, true);
    }
    rebuildSchematicForces();
    if (!options_.forces || !hasState_ || bodies_.empty()) {
        return;
    }
    const double ratio = vtkWidget_->devicePixelRatioF();
    double viewDirection[3];
    renderer_->GetActiveCamera()->GetDirectionOfProjection(viewDirection);
    const Vec3 view{viewDirection[0], viewDirection[1], viewDirection[2]};

    auto worldPoint = [this](const forcedisplay::Contribution& c, const Vec3& p) {
        return c.frame == QStringLiteral("welt") ? p : forcedisplay::toWorldPoint(pose_, p);
    };
    auto worldVector = [this](const forcedisplay::Contribution& c, const Vec3& v) {
        return c.frame == QStringLiteral("welt") ? v : forcedisplay::toWorldVector(pose_, v);
    };

    for (const auto& c : forces_.contributions) {
        if (!contributionVisible(c) || !forcedisplay::isDrawable(c)) {
            continue;
        }
        const auto scaled = scaled_.constFind(c.id);
        if (scaled == scaled_.constEnd()) {
            continue;
        }
        const Vec3 start = worldPoint(c, *c.point);
        const Vec3 vector = worldVector(c, *c.vector);
        const double perPixel = worldPerLogicalPixel(start);
        if (!(perPixel > 0.0)) {
            continue;
        }
        PolyBuilder lines;
        PolyBuilder head;
        forcedisplay::ScreenShape shape;
        shape.id = c.id;
        if (c.kind == Kind::Force) {
            const double lengthPx = forcedisplay::pixelLength(
                *scaled, forcedisplay::maxArrowPx, forcedisplay::minArrowPx);
            const auto arrow = forcedisplay::arrowGeometry(start, vector, lengthPx * perPixel,
                std::min(forcedisplay::arrowHeadPx, lengthPx) * perPixel);
            if (!arrow) {
                continue;
            }
            if (forcedisplay::isDashed(c.category)) {
                lines.dashed(arrow->start, arrow->headBase, dashPx * perPixel, gapPx * perPixel);
            } else {
                lines.segment(arrow->start, arrow->headBase);
            }
            Vec3 side = normalized(forcedisplay::cross(arrow->direction, view));
            if (forcedisplay::norm(side) < 0.5) {
                side = anyPerpendicular(arrow->direction);
            }
            auto tick = [&](double alongPx, double halfPx, double slantPx) {
                const Vec3 center = forcedisplay::add(
                    arrow->start, forcedisplay::scale(arrow->direction, alongPx * perPixel));
                const Vec3 offset = forcedisplay::add(forcedisplay::scale(side, halfPx * perPixel),
                    forcedisplay::scale(arrow->direction, slantPx * perPixel));
                lines.segment(forcedisplay::sub(center, offset), forcedisplay::add(center, offset));
            };
            if (scaled->overScale) {
                // Bruchzeichen: Wert liegt über der Skalenreferenz.
                tick(0.45 * lengthPx, 5.0, 2.0);
                tick(0.45 * lengthPx + 4.0, 5.0, 2.0);
            } else if (scaled->smallValue) {
                tick(0.0, 4.0, 0.0);
            }
            head.cone(arrow->tip, arrow->direction,
                std::min(forcedisplay::arrowHeadPx, lengthPx) * perPixel,
                0.45 * forcedisplay::arrowHeadPx * perPixel);
            shape.polyline = {toLogicalScreen(arrow->start), toLogicalScreen(arrow->tip)};
        } else {
            const double radiusPx = forcedisplay::pixelLength(
                *scaled, forcedisplay::maxArcRadiusPx, forcedisplay::minArcRadiusPx);
            const auto previous = arcBasis_.constFind(c.id);
            const auto arc = forcedisplay::momentArc(start, vector, radiusPx * perPixel,
                arcSegments,
                previous == arcBasis_.constEnd() ? std::nullopt : std::optional<Vec3>(*previous));
            if (!arc) {
                continue;
            }
            arcBasis_.insert(c.id, arc->basis);
            const double headLength = std::min(forcedisplay::arrowHeadPx * perPixel,
                0.5 * radiusPx * perPixel);
            QVector<Vec3> visible = arc->points;
            visible.back() = forcedisplay::sub(arc->tip, forcedisplay::scale(arc->tangent, headLength));
            lines.polyline(visible);
            if (scaled->overScale || scaled->smallValue) {
                const Vec3 radial = arc->basis;
                const Vec3 first = arc->points.front();
                const double half = (scaled->overScale ? 5.0 : 3.0) * perPixel;
                lines.segment(forcedisplay::sub(first, forcedisplay::scale(radial, half)),
                    forcedisplay::add(first, forcedisplay::scale(radial, half)));
            }
            head.cone(arc->tip, arc->tangent, headLength,
                0.45 * forcedisplay::arrowHeadPx * perPixel);
            for (const auto& p : arc->points) {
                shape.polyline.push_back(toLogicalScreen(p));
            }
        }
        auto& visual = visualFor(c.id);
        lines.storeLines(visual.lines);
        head.storePolys(visual.head);
        const bool selected = c.id == selected_;
        const QString color = selected ? selectionColor : forcedisplay::colorForContribution(c);
        setActorColor(visual.lineActor, color);
        setActorColor(visual.headActor, color);
        visual.lineActor->GetProperty()->SetLineWidth(static_cast<float>(
            (forcedisplay::strokePx + (selected ? 2.0 : 0.0) + (c.total ? 1.0 : 0.0)) * ratio));
        visual.lineActor->VisibilityOn();
        visual.headActor->VisibilityOn();
        screenShapes_.push_back(shape);
    }

    // Hebelarm-Hilfsgeometrie für den gewählten Kraftbeitrag (A-G-00015).
    if (options_.leverArm && !selected_.isEmpty() && forces_.reference) {
        for (const auto& c : forces_.contributions) {
            if (c.id != selected_ || c.kind != Kind::Force || !forcedisplay::isDrawable(c)) {
                continue;
            }
            const Vec3 reference = forcedisplay::toWorldPoint(pose_, *forces_.reference);
            const Vec3 attack = worldPoint(c, *c.point);
            const Vec3 force = worldVector(c, *c.vector);
            const auto arm = forcedisplay::perpendicularLeverArm(
                forcedisplay::sub(attack, reference), force);
            if (!arm) {
                break;
            }
            const Vec3 foot = forcedisplay::add(reference, *arm);
            PolyBuilder lever;
            lever.segment(reference, foot);
            const double perPixel = worldPerLogicalPixel(reference);
            if (perPixel > 0.0) {
                lever.dashed(foot, attack, 3.0 * perPixel, 3.0 * perPixel);
            }
            lever.storeLines(leverLines_);
            setActorColor(leverActor_, selectionColor);
            leverActor_->GetProperty()->SetLineWidth(static_cast<float>(2.0 * ratio));
            leverActor_->VisibilityOn();
            break;
        }
    }
}

QString VtkSceneWidget::pickInspectionElement(const QPoint& position) const
{
    const double ratio = vtkWidget_->devicePixelRatioF();
    const int* size = renderWindow_->GetSize();
    const int x = qRound(position.x() * ratio);
    const int y = size[1] - qRound(position.y() * ratio) - 1;
    if (!inspectionPicker_->Pick(x, y, 0.0, renderer_)) return {};
    auto* actor = inspectionPicker_->GetActor();
    for (const auto& body : bodies_)
        if (body.body == actor && body.surface != DisplayPart::Surface::Invisible)
            return body.elementId;
    return {};
}

QString VtkSceneWidget::inspectionHandleAt(const QPoint& position, bool* ambiguous) const
{
    if (ambiguous) *ambiguous = false;
    if (!inspectionEditView() || inspectionEditTarget_.isEmpty()) return {};
    constexpr double radius = 12.0;
    QString hit;
    int hitCount = 0;
    for (const auto& handle : inspectionHandles_) {
        const auto projected = toLogicalScreen(handle.position);
        const QPointF delta = projected - QPointF(position);
        if (delta.x() * delta.x() + delta.y() * delta.y() <= radius * radius) {
            hit = handle.key;
            ++hitCount;
        }
    }
    if (hitCount > 1) {
        if (ambiguous) *ambiguous = true;
        return {};
    }
    return hit;
}

void VtkSceneWidget::cancelInspectionDrag()
{
    if (!inspectionDragActive_) return;
    const auto element = inspectionDragElement_;
    const auto handle = inspectionDragHandle_;
    const auto start = inspectionDragStart_;
    inspectionDragActive_ = false;
    inspectionDragElement_.clear();
    inspectionDragHandle_.clear();
    if (!inspectionEditTarget_.isEmpty()) vtkWidget_->setCursor(Qt::OpenHandCursor);
    else vtkWidget_->unsetCursor();
    if (onInspectionInteractionCancelled)
        onInspectionInteractionCancelled();
    else if (onInspectionDrag)
        onInspectionDrag(element, handle, start, InspectionDragPhase::Cancel);
}

bool VtkSceneWidget::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == vtkWidget_ || (navigation_->parentWidget() == vtkWidget_
            && (watched == navigation_ || navigation_->isAncestorOf(qobject_cast<QWidget*>(watched)))))
        && event->type() == QEvent::MouseButtonDblClick && onDoubleClicked) {
        const auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) {
            pressActive_ = false;
            cameraDragButton_ = Qt::NoButton;
            QTimer::singleShot(0, this, [this] { if (onDoubleClicked) onDoubleClicked(); });
            return true;
        }
    }
    if (watched == vtkWidget_) {
        if (inspectionMode_
            && (event->type() == QEvent::Hide || event->type() == QEvent::UngrabMouse)) {
            pressActive_ = false;
            inspectionRightPress_ = false;
            cameraDragButton_ = Qt::NoButton;
            cameraDragPan_ = false;
            cancelInspectionDrag();
        }
        if (inspectionMode_ && event->type() == QEvent::KeyPress) {
            const auto* key = static_cast<QKeyEvent*>(event);
            if (key->key() == Qt::Key_Escape && inspectionDragActive_) {
                cancelInspectionDrag();
                return true;
            }
        }
        if (inspectionMode_ && event->type() == QEvent::MouseButtonPress) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::RightButton) {
                inspectionRightPress_ = true;
                pressPosition_ = mouse->position().toPoint();
            } else if (mouse->button() == Qt::LeftButton) {
                pressPosition_ = mouse->position().toPoint();
                pressActive_ = true;
            }
            if (mouse->button() == Qt::LeftButton && inspectionEditView()
                && !inspectionEditTarget_.isEmpty()) {
                const QPoint position = mouse->position().toPoint();
                bool ambiguousHandle = false;
                const QString handle = inspectionHandleAt(position, &ambiguousHandle);
                if (ambiguousHandle) {
                    if (onInspectionInteractionHint)
                        onInspectionInteractionHint(QStringLiteral(
                            "Die Eckgriffe liegen in dieser Projektion zu dicht beieinander. "
                            "Bitte eine andere Achsansicht wählen."));
                    pressActive_ = false;
                    return true;
                }
                const QString element = handle.isEmpty()
                    ? pickInspectionElement(position) : inspectionEditTarget_;
                if (!element.isEmpty() && element == inspectionEditTarget_) {
                    inspectionDragActive_ = true;
                    inspectionDragElement_ = element;
                    inspectionDragHandle_ = handle;
                    inspectionDragStart_ = mouse->position();
                    vtkWidget_->setCursor(Qt::ClosedHandCursor);
                    if (onInspectionDrag)
                        onInspectionDrag(element, handle, inspectionDragStart_,
                            InspectionDragPhase::Start);
                    return true;
                }
            }
        }
        if (inspectionMode_ && event->type() == QEvent::MouseMove
            && inspectionDragActive_) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            if (onInspectionDrag)
                onInspectionDrag(inspectionDragElement_, inspectionDragHandle_,
                    mouse->position(), InspectionDragPhase::Move);
            return true;
        }
        if (inspectionMode_ && event->type() == QEvent::MouseButtonRelease) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            if (inspectionDragActive_ && mouse->button() == Qt::LeftButton) {
                if (onInspectionDrag)
                    onInspectionDrag(inspectionDragElement_, inspectionDragHandle_,
                        mouse->position(), InspectionDragPhase::Finish);
                inspectionDragActive_ = false;
                pressActive_ = false;
                inspectionDragElement_.clear();
                inspectionDragHandle_.clear();
                if (!inspectionEditTarget_.isEmpty()) vtkWidget_->setCursor(Qt::OpenHandCursor);
                return true;
            }
            if (inspectionRightPress_ && mouse->button() == Qt::RightButton) {
                const QPoint position = mouse->position().toPoint();
                inspectionRightPress_ = false;
                if ((position - pressPosition_).manhattanLength() <= 3) {
                    const auto element = pickInspectionElement(position);
                    if (!element.isEmpty() && onInspectionContextMenu)
                        onInspectionContextMenu(element, vtkWidget_->mapToGlobal(position));
                    return true;
                }
            }
            if (mouse->button() == Qt::LeftButton && !inspectionDragActive_
                && pressActive_
                && (mouse->position().toPoint() - pressPosition_).manhattanLength() <= 3) {
                const auto element = pickInspectionElement(mouse->position().toPoint());
                if (!element.isEmpty() && onInspectionPicked)
                    onInspectionPicked(element);
                pressActive_ = false;
                return !element.isEmpty();
            }
            if (mouse->button() == Qt::LeftButton) pressActive_ = false;
        }
        if (event->type() == QEvent::Resize) {
            updateInspectionMask();
            layoutViewControls();
        }
        if (orthographicView_ != OrthographicView::Free) {
            if (event->type() == QEvent::Wheel) {
                const auto* wheel = static_cast<QWheelEvent*>(event);
                updateOrthographicCamera();
                if (orthographicScale_ > 0.0)
                    orthographicScale_ = std::clamp(orthographicScale_
                        * std::pow(1.1, -wheel->angleDelta().y() / 120.0), 1e-6, 1e9);
                geometryDirty_ = true;
                requestRender();
                return true;
            }
            if (event->type() == QEvent::MouseButtonPress) {
                const auto* mouse = static_cast<QMouseEvent*>(event);
                cameraDragPosition_ = mouse->position();
                cameraDragButton_ = mouse->button();
                cameraDragPan_ = mouse->button() == Qt::MiddleButton
                    || (mouse->button() == Qt::LeftButton
                        && !(mouse->modifiers().testFlag(Qt::ShiftModifier)
                            && mouse->modifiers().testFlag(Qt::ControlModifier)));
                return true;
            }
            if (event->type() == QEvent::MouseMove) {
                const auto* mouse = static_cast<QMouseEvent*>(event);
                const auto delta = mouse->position() - cameraDragPosition_;
                cameraDragPosition_ = mouse->position();
                updateOrthographicCamera();
                if (cameraDragButton_ != Qt::NoButton && cameraDragPan_) {
                    const double unitsPerPixel = 2.0 * orthographicScale_ / std::max(1, vtkWidget_->height());
                    orthographicPan_ += QPointF(-delta.x(), delta.y()) * unitsPerPixel;
                } else if (cameraDragButton_ == Qt::RightButton
                    || (cameraDragButton_ == Qt::LeftButton
                        && mouse->modifiers().testFlag(Qt::ShiftModifier)
                        && mouse->modifiers().testFlag(Qt::ControlModifier))) {
                    if (orthographicScale_ > 0.0)
                        orthographicScale_ = std::clamp(orthographicScale_
                            * std::exp(-delta.y() * 0.01), 1e-6, 1e9);
                }
                geometryDirty_ = true;
                requestRender();
                return true;
            }
            if (event->type() == QEvent::MouseButtonRelease) {
                cameraDragButton_ = Qt::NoButton;
                return true;
            }
            if (event->type() == QEvent::Hide || event->type() == QEvent::UngrabMouse)
                cameraDragButton_ = Qt::NoButton;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            pressActive_ = mouse->button() == Qt::LeftButton;
            pressPosition_ = mouse->position().toPoint();
        } else if (event->type() == QEvent::MouseButtonRelease) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            const QPoint position = mouse->position().toPoint();
            if (pressActive_ && mouse->button() == Qt::LeftButton
                && (position - pressPosition_).manhattanLength() <= 3) {
                // Erst nach der Interaktor-Verarbeitung auswerten (Menü blockiert).
                QTimer::singleShot(0, this, [this, position] { pickAt(position); });
            }
            pressActive_ = false;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void VtkSceneWidget::pickAt(const QPoint& position)
{
    const auto hits = forcedisplay::hitTest(screenShapes_, QPointF(position));
    QString chosen;
    if (hits.size() == 1) {
        chosen = hits.front();
    } else if (hits.size() > 1) {
        QMenu menu(this);
        for (const auto& id : hits) {
            QString label = id;
            for (const auto& c : forces_.contributions) {
                if (c.id == id) {
                    label = forcedisplay::contributionLabel(c);
                    break;
                }
            }
            menu.addAction(label)->setData(id);
        }
        const auto* action = menu.exec(vtkWidget_->mapToGlobal(position));
        if (action == nullptr) {
            return;
        }
        chosen = action->data().toString();
    }
    setSelected(chosen);
    if (onPicked) {
        onPicked(chosen);
    }
}

void VtkSceneWidget::updateWaterPlane()
{
    double bounds[6];
    if (!bodyBounds(bounds)) {
        return;
    }
    sceneSpan_ = std::max({bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4], 0.05});
    renderer_->SetSSAORadius(0.06 * sceneSpan_);
    renderer_->SetSSAOBias(0.002 * sceneSpan_);
    geometryDirty_ = true;
}

void VtkSceneWidget::updateInspectionMask()
{
    QPainterPath outline;
    outline.addRoundedRect(QRectF(vtkWidget_->rect()), 11.0, 11.0);
    vtkWidget_->setMask(QRegion(outline.toFillPolygon().toPolygon()));
}

void VtkSceneWidget::rebuildInspectionGrid()
{
    for (const auto& label : inspectionGridLabels_) renderer_->RemoveActor(label);
    inspectionGridLabels_.clear();
    setProperty("inspectionGridLabelCount", 0);
    double bounds[6];
    gridMesh_->Initialize();
    if (!bodyBounds(bounds)) {
        gridActor_->VisibilityOff();
        inspectionAxes_->VisibilityOff();
        return;
    }
    constexpr double step = 0.5;
    std::array<double, 3> low{}, high{};
    for (int axis = 0; axis < 3; ++axis) {
        low[axis] = std::floor(std::min(bounds[2 * axis], 0.0) / step) * step - step;
        high[axis] = std::ceil(std::max(bounds[2 * axis + 1], 0.0) / step) * step + step;
    }
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> lines;
    vtkNew<vtkCellArray> dots;
    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetNumberOfComponents(4);
    const unsigned char color[4] = {181, 145, 164, 70};
    const auto line = [&](const std::array<double, 3>& from, const std::array<double, 3>& to,
                          bool solid) {
        if (!solid) {
            double length = 0.0;
            for (int axis = 0; axis < 3; ++axis) length += std::abs(to[axis] - from[axis]);
            const int count = std::max(1, static_cast<int>(std::ceil(length / 0.1)));
            for (int i = 0; i <= count; ++i) {
                std::array<double, 3> point{};
                for (int axis = 0; axis < 3; ++axis)
                    point[axis] = from[axis] + (to[axis] - from[axis]) * i / count;
                const vtkIdType id = points->InsertNextPoint(point.data());
                dots->InsertNextCell(1, &id);
                colors->InsertNextTypedTuple(color);
            }
            return;
        }
        const vtkIdType ids[2] = {points->InsertNextPoint(from.data()),
            points->InsertNextPoint(to.data())};
        lines->InsertNextCell(2, ids);
        colors->InsertNextTypedTuple(color);
        colors->InsertNextTypedTuple(color);
    };
    // Three open reference faces; equal world-space spacing, fixed to model coordinates.
    for (int normal = 0; normal < 3; ++normal) {
        const int a = (normal + 1) % 3, b = (normal + 2) % 3;
        for (int direction = 0; direction < 2; ++direction) {
            const int fixed = direction == 0 ? a : b;
            const int varying = direction == 0 ? b : a;
            const int count = static_cast<int>(std::llround((high[fixed] - low[fixed]) / step));
            for (int i = 0; i <= count; ++i) {
                auto from = low, to = low;
                from[fixed] = to[fixed] = low[fixed] + i * step;
                to[varying] = high[varying];
                const bool solid = std::llround(from[fixed] * 2.0) % 2 == 0;
                line(from, to, solid);
            }
        }
    }
    gridMesh_->SetPoints(points);
    gridMesh_->SetLines(lines);
    gridMesh_->SetVerts(dots);
    gridMesh_->GetPointData()->SetScalars(colors);
    gridMesh_->Modified();
    gridActor_->VisibilityOn();
    gridActor_->GetProperty()->SetPointSize(1.5f);
    gridActor_->GetProperty()->RenderPointsAsSpheresOn();
    inspectionAxes_->SetTotalLength(2.0 * step, 2.0 * step, 2.0 * step);
    inspectionAxes_->VisibilityOn();
    std::vector<QRectF> labelRects;
    for (int axis = 0; axis < 3; ++axis) {
        auto endpoint = low;
        endpoint[axis] = high[axis];
        const auto screenDelta = toLogicalScreen(endpoint) - toLogicalScreen(low);
        const double screenLength = std::hypot(screenDelta.x(), screenDelta.y());
        const double spacing = std::max(1., std::ceil(
            44. * (high[axis] - low[axis]) / std::max(1., screenLength)));
        for (double coordinate = std::ceil(low[axis]/spacing)*spacing;
             coordinate <= high[axis]; coordinate += spacing) {
            auto position = low;
            position[axis] = coordinate;
            if (axis < 2) {
                position[1-axis] -= .12;
                position[2] -= .06;
            } else {
                position[0] -= .12;
                position[1] -= .12;
            }
            const auto caption = QStringLiteral("%1 m").arg(coordinate, 0, 'f', 0).toUtf8();
            const auto screen = toLogicalScreen(position);
            const double width = 8. * caption.size() + 8.;
            const QRectF labelRect(screen.x()-width/2., screen.y()-18., width, 24.);
            if (std::any_of(labelRects.begin(), labelRects.end(),
                [&](const auto& other) { return other.intersects(labelRect); })) continue;
            labelRects.push_back(labelRect);
            auto label = vtkSmartPointer<vtkBillboardTextActor3D>::New();
            label->SetInput(caption.constData());
            label->SetPosition(position.data());
            label->PickableOff();
            label->UseBoundsOff();
            label->GetTextProperty()->SetFontSize(12);
            label->GetTextProperty()->SetColor(.85,.76,.81);
            label->GetTextProperty()->SetJustificationToCentered();
            renderer_->AddActor(label);
            inspectionGridLabels_.push_back(label);
        }
    }
    setProperty("inspectionGridLabelCount", static_cast<int>(inspectionGridLabels_.size()));
}

void VtkSceneWidget::rebuildEnvironment()
{
    updateReflectionVisibility();
    if (inspectionMode_) {
        rebuildInspectionGrid();
        return;
    }
    auto* camera = renderer_->GetActiveCamera();
    double position[3];
    double focal[3];
    camera->GetPosition(position);
    camera->GetFocalPoint(focal);
    const double distance = std::max(camera->GetDistance(), 1e-3);
    horizonRadius_ = horizonFactor * std::max({distance, std::abs(position[2]), sceneSpan_});
    const double radius = horizonRadius_;
    const double cx = position[0];
    const double cy = position[1];
    const double depth = 1.2 * sceneSpan_;

    auto* points = waterMesh_->GetPoints();
    auto* colors = vtkUnsignedCharArray::SafeDownCast(waterMesh_->GetPointData()->GetScalars());
    auto* textureCoordinates = vtkFloatArray::SafeDownCast(waterMesh_->GetPointData()->GetTCoords());
    auto waterRgba = [&](double x, double y, unsigned char rgba[4]) {
        const double r = std::hypot(x - cx, y - cy);
        // Auslauf zum Horizont: das Wasser geht in den Himmelsdunst über.
        const double fade = 1.0 - smoothstep(0.2 * radius, radius, r);
        const double haze = graphicsProfile_.on(graphics::Feature::Haze)
            ? 0.75 * smoothstep(0.08 * radius, 0.85 * radius, r) : 0.0;
        // Fresnel: flacher Blickwinkel deckender, Blick von oben durchsichtiger.
        const Vec3 toCamera = normalized({position[0] - x, position[1] - y, position[2]});
        const double grazing = std::pow(1.0 - std::abs(toCamera[2]), 2.5);
        const double opacity = graphicsProfile_.on(graphics::Feature::Fresnel)
            ? mix(graphicsProfile_.waterOpacity, 0.95, grazing) : graphicsProfile_.waterOpacity;
        const double underwaterVisibility = mix(0.35, 1.0, smoothstep(-0.25, 0.0, position[2]));
        const double alpha = fade * std::clamp(opacity, 0.0, 0.95) * underwaterVisibility;
        const double inspectionWater[3] = {101.0 / 255.0, 40.0 / 255.0, 63.0 / 255.0};
        const double inspectionSky[3] = {198.0 / 255.0, 149.0 / 255.0, 165.0 / 255.0};
        const auto* water = inspectionMode_ ? inspectionWater : waterColor;
        const auto* sky = inspectionMode_ ? inspectionSky : horizonColor;
        const bool schematic = orthographicView_ != OrthographicView::Free;
        rgba[0] = toByte(schematic ? 0.38 : mix(water[0], sky[0], haze));
        rgba[1] = toByte(schematic ? 0.60 : mix(water[1], sky[1], haze));
        rgba[2] = toByte(schematic ? 0.74 : mix(water[2], sky[2], haze));
        rgba[3] = toByte(alpha);
    };
    auto setVertex = [&](vtkIdType id, double x, double y) {
        points->SetPoint(id, x, y, 0.0);
        if (textureCoordinates != nullptr) {
            textureCoordinates->SetTuple2(
                id, x / graphicsProfile_.waterPeriod, y / graphicsProfile_.waterPeriod);
        }
        unsigned char rgba[4];
        waterRgba(x, y, rgba);
        colors->SetTypedTuple(id, rgba);
    };
    setVertex(0, cx, cy);
    constexpr double growth = 7.0;
    for (int ring = 0; ring < waterRings; ++ring) {
        const double t = static_cast<double>(ring + 1) / waterRings;
        const double r = radius * (std::exp(growth * t) - 1.0) / (std::exp(growth) - 1.0);
        for (int s = 0; s < waterSegments; ++s) {
            const double angle = 2.0 * pi * s / waterSegments;
            setVertex(1 + static_cast<vtkIdType>(ring) * waterSegments + s,
                cx + r * std::cos(angle), cy + r * std::sin(angle));
        }
    }
    points->Modified();
    if (textureCoordinates != nullptr) textureCoordinates->Modified();
    colors->Modified();
    waterMesh_->Modified();
    rebuildWaterDisplay(waterRgba);
    auto deepSurfacePoints = vtkSmartPointer<vtkPoints>::New();
    deepSurfacePoints->DeepCopy(waterDisplay_->GetPoints());
    auto deepSurfaceColors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    deepSurfaceColors->SetNumberOfComponents(4);
    deepSurfaceColors->SetNumberOfTuples(deepSurfacePoints->GetNumberOfPoints());
    for (vtkIdType i = 0; i < deepSurfacePoints->GetNumberOfPoints(); ++i) {
        double p[3];
        deepSurfacePoints->GetPoint(i, p);
        p[2] -= depth;
        deepSurfacePoints->SetPoint(i, p);
        const double fade = 1.0 - smoothstep(0.2 * radius, radius, std::hypot(p[0] - cx, p[1] - cy));
        const unsigned char rgba[4] = {toByte(deepColor[0]), toByte(deepColor[1]),
            toByte(deepColor[2]), toByte(0.8 * fade)};
        deepSurfaceColors->SetTypedTuple(i, rgba);
    }
    deepMesh_->Initialize();
    deepMesh_->SetPoints(deepSurfacePoints);
    deepMesh_->SetPolys(waterDisplay_->GetPolys());
    deepMesh_->GetPointData()->SetScalars(deepSurfaceColors);
    deepMesh_->Modified();

    if (!graphicsProfile_.on(graphics::Feature::Grid)) {
        gridMesh_->Initialize();
        gridActor_->SetVisibility(false);
        return;
    }

    // Weltfestes Raster mit 1-2-5-Teilung; jede fünfte Linie kräftiger.
    const double step = niceStep(distance / 6.0);
    const double extent = gridHalfLines * step;
    const double gx = std::round(focal[0] / step) * step;
    const double gy = std::round(focal[1] / step) * step;
    const double lift = 2e-4 * distance;
    vtkNew<vtkPoints> gridPoints;
    vtkNew<vtkCellArray> gridLines;
    vtkNew<vtkUnsignedCharArray> gridColors;
    gridColors->SetNumberOfComponents(4);
    std::vector<vtkIdType> ids;
    for (int direction = 0; direction < 2; ++direction) {
        for (int line = -gridHalfLines; line <= gridHalfLines; ++line) {
            const double fixed = (direction == 0 ? gx : gy) + line * step;
            const bool major = std::llround(fixed / step) % 5 == 0;
            const auto addPoint = [&](double x, double y) {
                const double d = std::hypot(x - focal[0], y - focal[1]);
                const double alpha = (major ? 0.32 : 0.13) * (1.0 - smoothstep(0.15 * extent, extent, d));
                const unsigned char rgba[4] = {205, 232, 255, toByte(alpha)};
                gridColors->InsertNextTypedTuple(rgba);
                return gridPoints->InsertNextPoint(x, y, lift);
            };
            const auto flush = [&]() {
                if (ids.size() >= 2) {
                    gridLines->InsertNextCell(static_cast<vtkIdType>(ids.size()), ids.data());
                }
                ids.clear();
            };
            ids.clear();
            double previous[2] = {0.0, 0.0};
            for (int k = -gridHalfLines; k <= gridHalfLines; ++k) {
                const double along = (direction == 0 ? gy : gx) + k * step;
                const double current[2] = {direction == 0 ? fixed : along, direction == 0 ? along : fixed};
                if (k > -gridHalfLines) {
                    const bool near = !dryLoops_.empty()
                        && std::max(previous[0], current[0]) >= dryBox_[0]
                        && std::min(previous[0], current[0]) <= dryBox_[1]
                        && std::max(previous[1], current[1]) >= dryBox_[2]
                        && std::min(previous[1], current[1]) <= dryBox_[3];
                    if (!near) {
                        if (ids.empty()) ids.push_back(addPoint(previous[0], previous[1]));
                        ids.push_back(addPoint(current[0], current[1]));
                    } else {
                        flush();
                        for (const auto& interval : outsideIntervals(dryLoops_, previous, current)) {
                            const auto at = [&](double t, int c) {
                                return previous[c] + t * (current[c] - previous[c]);
                            };
                            ids.push_back(addPoint(at(interval[0], 0), at(interval[0], 1)));
                            ids.push_back(addPoint(at(interval[1], 0), at(interval[1], 1)));
                            if (interval[1] < 1.0) flush();
                        }
                    }
                }
                previous[0] = current[0];
                previous[1] = current[1];
            }
            flush();
        }
    }
    gridMesh_->Initialize();
    gridMesh_->SetPoints(gridPoints);
    gridMesh_->SetLines(gridLines);
    gridMesh_->GetPointData()->SetScalars(gridColors);
    gridMesh_->Modified();
}

void VtkSceneWidget::rebuildWaterDisplay(
    const std::function<void(double, double, unsigned char*)>& waterRgba)
{
    const double big = std::numeric_limits<double>::max();
    dryBox_ = {big, -big, big, -big};
    for (const auto& loop : dryLoops_) {
        for (const auto& p : loop) {
            dryBox_[0] = std::min(dryBox_[0], p[0]);
            dryBox_[1] = std::max(dryBox_[1], p[0]);
            dryBox_[2] = std::min(dryBox_[2], p[1]);
            dryBox_[3] = std::max(dryBox_[3], p[1]);
        }
    }
    waterDisplay_->Initialize();
    bool holed = false;
    if (!dryLoops_.empty()) {
        // Wasserzellen über dem Trockenbereich entfernen und die Lücke zwischen
        // ihrem Rand und den trockenen Schleifen neu triangulieren.
        const double margin = 0.03 * sceneSpan_;
        const double box[4] = {dryBox_[0] - margin, dryBox_[1] + margin, dryBox_[2] - margin,
            dryBox_[3] + margin};
        vtkPoints* fanPoints = waterMesh_->GetPoints();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->DeepCopy(fanPoints);
        auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
        colors->DeepCopy(waterMesh_->GetPointData()->GetScalars());
        auto normals = vtkSmartPointer<vtkFloatArray>::New();
        normals->DeepCopy(waterMesh_->GetPointData()->GetNormals());
        auto textureCoordinates = vtkSmartPointer<vtkFloatArray>::New();
        textureCoordinates->DeepCopy(waterMesh_->GetPointData()->GetTCoords());
        auto polys = vtkSmartPointer<vtkCellArray>::New();
        std::vector<std::pair<vtkIdType, vtkIdType>> removedEdges;
        std::map<std::pair<vtkIdType, vtkIdType>, int> edgeUse;
        vtkCellArray* fanPolys = waterMesh_->GetPolys();
        fanPolys->InitTraversal();
        vtkIdType count = 0;
        const vtkIdType* cell = nullptr;
        while (fanPolys->GetNextCell(count, cell)) {
            double lo[2] = {big, big};
            double hi[2] = {-big, -big};
            for (vtkIdType k = 0; k < count; ++k) {
                double p[3];
                fanPoints->GetPoint(cell[k], p);
                lo[0] = std::min(lo[0], p[0]);
                lo[1] = std::min(lo[1], p[1]);
                hi[0] = std::max(hi[0], p[0]);
                hi[1] = std::max(hi[1], p[1]);
            }
            if (hi[0] < box[0] || lo[0] > box[1] || hi[1] < box[2] || lo[1] > box[3]) {
                polys->InsertNextCell(count, cell);
                continue;
            }
            for (vtkIdType k = 0; k < count; ++k) {
                const vtkIdType a = cell[k];
                const vtkIdType b = cell[(k + 1) % count];
                removedEdges.emplace_back(a, b);
                ++edgeUse[{std::min(a, b), std::max(a, b)}];
            }
        }
        std::map<vtkIdType, vtkIdType> next;
        for (const auto& [a, b] : removedEdges) {
            if (edgeUse[{std::min(a, b), std::max(a, b)}] == 1) next[a] = b;
        }
        auto contourLines = vtkSmartPointer<vtkCellArray>::New();
        while (!next.empty()) {
            std::vector<vtkIdType> chain;
            const vtkIdType start = next.begin()->first;
            vtkIdType current = start;
            bool closed = false;
            while (true) {
                const auto it = next.find(current);
                if (it == next.end()) break;
                chain.push_back(current);
                current = it->second;
                next.erase(it);
                if (current == start) {
                    closed = true;
                    break;
                }
            }
            if (closed && chain.size() >= 3) {
                chain.push_back(start);
                contourLines->InsertNextCell(static_cast<vtkIdType>(chain.size()), chain.data());
            }
        }
        for (const auto& loop : dryLoops_) {
            if (loop.size() < 3) continue;
            const bool counterClockwise = signedArea(loop) > 0.0;
            std::vector<vtkIdType> chain;
            for (std::size_t k = 0; k < loop.size(); ++k) {
                const auto& p = loop[counterClockwise ? loop.size() - 1 - k : k];
                unsigned char rgba[4];
                waterRgba(p[0], p[1], rgba);
                colors->InsertNextTypedTuple(rgba);
                normals->InsertNextTuple3(0.0, 0.0, 1.0);
                textureCoordinates->InsertNextTuple2(
                    p[0] / graphicsProfile_.waterPeriod, p[1] / graphicsProfile_.waterPeriod);
                chain.push_back(points->InsertNextPoint(p[0], p[1], 0.0));
            }
            chain.push_back(chain.front());
            contourLines->InsertNextCell(static_cast<vtkIdType>(chain.size()), chain.data());
        }
        auto contour = vtkSmartPointer<vtkPolyData>::New();
        contour->SetPoints(points);
        contour->SetLines(contourLines);
        const double up[3] = {0.0, 0.0, 1.0};
        auto triangles = vtkSmartPointer<vtkCellArray>::New();
        if (contourLines->GetNumberOfCells() > 0
            && vtkContourTriangulator::TriangulateContours(
                   contour, 0, contourLines->GetNumberOfCells(), triangles, up) == 1
            && triangles->GetNumberOfCells() > 0) {
            triangles->InitTraversal();
            while (triangles->GetNextCell(count, cell)) {
                polys->InsertNextCell(count, cell);
            }
            waterDisplay_->SetPoints(points);
            waterDisplay_->SetPolys(polys);
            waterDisplay_->GetPointData()->SetScalars(colors);
            waterDisplay_->GetPointData()->SetNormals(normals);
            waterDisplay_->GetPointData()->SetTCoords(textureCoordinates);
            holed = true;
        }
    }
    if (!holed) {
        waterDisplay_->ShallowCopy(waterMesh_);
    }
    waterDisplay_->Modified();
}

bool VtkSceneWidget::bodyBounds(double bounds[6]) const
{
    const double maxValue = std::numeric_limits<double>::max();
    for (int axis = 0; axis < 3; ++axis) {
        bounds[axis * 2] = maxValue;
        bounds[axis * 2 + 1] = -maxValue;
    }
    for (const auto& body : bodies_) {
        double single[6];
        body.body->GetBounds(single);
        for (int axis = 0; axis < 3; ++axis) {
            bounds[axis * 2] = std::min(bounds[axis * 2], single[axis * 2]);
            bounds[axis * 2 + 1] = std::max(bounds[axis * 2 + 1], single[axis * 2 + 1]);
        }
    }
    return bounds[0] != maxValue;
}

double VtkSceneWidget::bodyHalfDiagonal() const
{
    double sum = 0.0;
    bool any = false;
    double bounds[6] = {};
    for (const auto& body : bodies_) {
        double single[6];
        body.mesh->GetBounds(single);
        for (int axis = 0; axis < 3; ++axis) {
            bounds[axis * 2] = any ? std::min(bounds[axis * 2], single[axis * 2]) : single[axis * 2];
            bounds[axis * 2 + 1] =
                any ? std::max(bounds[axis * 2 + 1], single[axis * 2 + 1]) : single[axis * 2 + 1];
        }
        any = true;
    }
    if (!any) {
        return 0.0;
    }
    for (int axis = 0; axis < 3; ++axis) {
        sum += std::pow(bounds[axis * 2 + 1] - bounds[axis * 2], 2.0);
    }
    return 0.5 * std::sqrt(sum);
}

void VtkSceneWidget::frameBodies()
{
    double bounds[6];
    if (bodyBounds(bounds)) {
        frameBounds(bounds, 0.85);
    }
}

void VtkSceneWidget::frameStates(const QVector<QJsonObject>& states)
{
    if (!updatesActive_) return;
    const double maxValue = std::numeric_limits<double>::max();
    double meshBounds[6] = {maxValue, -maxValue, maxValue, -maxValue, maxValue, -maxValue};
    for (const auto& body : bodies_) {
        double single[6];
        body.mesh->GetBounds(single);
        for (int axis = 0; axis < 3; ++axis) {
            meshBounds[axis * 2] = std::min(meshBounds[axis * 2], single[axis * 2]);
            meshBounds[axis * 2 + 1] = std::max(meshBounds[axis * 2 + 1], single[axis * 2 + 1]);
        }
    }
    if (meshBounds[0] == maxValue) {
        return;
    }
    // Lageunabhängige Hülle: Kugel um das Mesh-Zentrum, entlang der Bahn verschoben.
    const Vec3 center{(meshBounds[0] + meshBounds[1]) / 2.0,
        (meshBounds[2] + meshBounds[3]) / 2.0, (meshBounds[4] + meshBounds[5]) / 2.0};
    const double radius = 0.5 * std::sqrt(std::pow(meshBounds[1] - meshBounds[0], 2.0)
        + std::pow(meshBounds[3] - meshBounds[2], 2.0)
        + std::pow(meshBounds[5] - meshBounds[4], 2.0));
    double bounds[6] = {maxValue, -maxValue, maxValue, -maxValue, maxValue, -maxValue};
    QVector<TrackPoint> track;
    for (const auto& state : states) {
        try {
            const auto point = forcedisplay::toWorldPoint(forcedisplay::readPose(state), center);
            const double time = state.value(QStringLiteral("zeit_s")).toDouble();
            if (track.isEmpty() || time > track.back().time) {
                track.push_back({time, point});
            }
            for (int axis = 0; axis < 3; ++axis) {
                bounds[axis * 2] = std::min(bounds[axis * 2], point[static_cast<std::size_t>(axis)] - radius);
                bounds[axis * 2 + 1] =
                    std::max(bounds[axis * 2 + 1], point[static_cast<std::size_t>(axis)] + radius);
            }
        } catch (const std::exception&) {
        }
    }
    if (bounds[0] == maxValue) {
        return;
    }
    track_ = track;
    updateTrail();
    // Recorded trajectories must not refit or reset the body-fixed camera.
    renderScene();
}

void VtkSceneWidget::frameBounds(const double bounds[6], double zoom)
{
    // Schrägansicht von vorn-seitlich, Z oben; Rahmen um den Körper, nicht um die Wasserfläche.
    auto* camera = renderer_->GetActiveCamera();
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetPosition(0.0, -1.0, 0.0);
    camera->SetViewUp(0.0, 0.0, 1.0);
    double copy[6];
    std::copy(bounds, bounds + 6, copy);
    renderer_->ResetCamera(copy);
    camera->Azimuth(32.0);
    camera->Elevation(22.0);
    camera->Zoom(zoom);
    renderer_->ResetCameraClippingRange();
}
