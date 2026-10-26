#include "vtk_scene.hpp"

#include <QColor>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>
#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCaptionActor2D.h>
#include <vtkCellArray.h>
#include <vtkCommand.h>
#include <vtkCutter.h>
#include <vtkFloatArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkLightKit.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSkybox.h>
#include <vtkTextProperty.h>
#include <vtkTexture.h>
#include <vtkTransform.h>
#include <vtkUnsignedCharArray.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

using forcedisplay::Category;
using forcedisplay::Kind;
using forcedisplay::Vec3;

namespace {

constexpr double pi = 3.14159265358979323846;
constexpr double dashPx = 8.0;
constexpr double gapPx = 5.0;
constexpr int arcSegments = 48;
const QString selectionColor = QStringLiteral("#ffd166");

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

// Äquirektangulärer Himmel (Z oben): dunkler Zenit, heller Dunst am
// Horizont, gedämpfte Sonne, darunter dunkles Wasser. Dient als Skybox und
// als Umgebungslicht für Spiegelungen.
vtkSmartPointer<vtkTexture> makeSkyTexture()
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
            if (elevation >= 0.0) {
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
        property->SetMetallic(1.0);
        property->SetRoughness(0.28);
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
    }
    return 1.0;
}

} // namespace

VtkSceneWidget::VtkSceneWidget(QWidget* parent)
    : QWidget(parent)
    , vtkWidget_(new QVTKOpenGLNativeWidget(this))
    , renderWindow_(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , renderer_(vtkSmartPointer<vtkRenderer>::New())
    , overlay_(vtkSmartPointer<vtkRenderer>::New())
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(vtkWidget_);
    renderWindow_->SetNumberOfLayers(2);
    renderWindow_->AddRenderer(renderer_);
    renderWindow_->AddRenderer(overlay_);
    renderWindow_->SetMultiSamples(0);
    vtkWidget_->setRenderWindow(renderWindow_);
    renderer_->SetBackground(0.035, 0.065, 0.11);
    renderer_->SetBackground2(0.08, 0.16, 0.23);
    renderer_->GradientBackgroundOn();
    renderer_->GetActiveCamera()->SetPosition(0.0, -1.0, 0.0);
    renderer_->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
    // Overlay für „durch Körper sichtbar“: eigene Ebene mit geleertem
    // Tiefenpuffer, gleiche Kamera.
    overlay_->SetLayer(1);
    overlay_->SetActiveCamera(renderer_->GetActiveCamera());
    overlay_->InteractiveOff();
    overlay_->PreserveDepthBufferOff();
    renderObserver_ = renderWindow_->AddObserver(
        vtkCommand::StartEvent, this, &VtkSceneWidget::onRenderStart);
    vtkWidget_->installEventFilter(this);
    leverLines_ = vtkSmartPointer<vtkPolyData>::New();
    leverActor_ = makeOverlayActor(leverLines_);

    // Licht: Lichtset statt Einzellicht, Kantenglättung, Tiefenschälung für
    // Transparenz, Umgebungsverdeckung und Himmel als Umgebungslicht.
    renderer_->AutomaticLightCreationOff();
    renderer_->RemoveAllLights();
    vtkNew<vtkLightKit> lights;
    lights->SetKeyLightIntensity(0.85);
    lights->AddLightsToRenderer(renderer_);
    renderer_->UseFXAAOn();
    renderWindow_->SetAlphaBitPlanes(1);
    renderer_->UseDepthPeelingOn();
    renderer_->SetMaximumNumberOfPeels(8);
    renderer_->SetOcclusionRatio(0.05);
    renderer_->UseSSAOOn();
    renderer_->SetSSAOKernelSize(48);
    renderer_->SSAOBlurOn();
    skyTexture_ = makeSkyTexture();
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

    waterMesh_ = makePolarMesh(true);
    waterActor_ = makeColoredActor(waterMesh_);
    applySurface(waterActor_->GetProperty(), DisplayPart::Surface::Glossy);
    waterActor_->GetProperty()->SetRoughness(0.12);
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
}

void VtkSceneWidget::addEnvironmentActors()
{
    renderer_->AddActor(skybox_);
    renderer_->AddActor(deepActor_);
    renderer_->AddActor(waterActor_);
    renderer_->AddActor(gridActor_);
    renderer_->AddActor(trailActor_);
}

VtkSceneWidget::~VtkSceneWidget()
{
    renderWindow_->RemoveObserver(renderObserver_);
}

void VtkSceneWidget::clearModel()
{
    renderer_->RemoveAllViewProps();
    overlay_->RemoveAllViewProps();
    addEnvironmentActors();
    bodies_.clear();
    visuals_.clear();
    arcBasis_.clear();
    scaled_.clear();
    screenShapes_.clear();
    track_.clear();
    trailActor_->VisibilityOff();
    forces_ = {};
    hasState_ = false;
    selected_.clear();
    forceReference_ = 0.0;
    momentReference_ = 0.0;
    viewBoundsValid_ = false;
    geometryDirty_ = true;
    renderWindow_->Render();
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
            auto normals = vtkSmartPointer<vtkPolyDataNormals>::New();
            normals->SetInputData(part.mesh);
            normals->SetFeatureAngle(35.0);
            normals->SplittingOn();
            normals->ConsistencyOn();
            auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(normals->GetOutputPort());
            auto bodyActor = vtkSmartPointer<vtkActor>::New();
            bodyActor->SetMapper(mapper);
            if (part.hasColor) {
                bodyActor->GetProperty()->SetColor(part.color[0], part.color[1], part.color[2]);
            } else {
                bodyActor->GetProperty()->SetColor(
                    i % 2 == 0 ? 0.82 : 0.43, i % 2 == 0 ? 0.87 : 0.72, i % 2 == 0 ? 0.91 : 0.82);
            }
            const double opacity = applySurface(bodyActor->GetProperty(), part.surface);
            bodyActor->GetProperty()->BackfaceCullingOff();
            renderer_->AddActor(bodyActor);

            auto plane = vtkSmartPointer<vtkPlane>::New();
            auto cutter = vtkSmartPointer<vtkCutter>::New();
            cutter->SetInputData(part.mesh);
            cutter->SetCutFunction(plane);
            auto waterlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            waterlineMapper->SetInputConnection(cutter->GetOutputPort());
            auto waterlineActor = vtkSmartPointer<vtkActor>::New();
            waterlineActor->SetMapper(waterlineMapper);
            waterlineActor->GetProperty()->SetColor(0.12, 0.92, 1.0);
            waterlineActor->GetProperty()->SetLineWidth(3.0);
            waterlineActor->GetProperty()->SetLighting(false);
            renderer_->AddActor(waterlineActor);

            BodyActor body;
            body.mesh = part.mesh;
            body.body = bodyActor;
            body.opacity = opacity;
            body.waterline = waterlineActor;
            body.localWaterPlane = plane;
            body.cutter = cutter;
            bodies_.push_back(body);
        }

        forceRenderer()->AddActor(leverActor_);
        leverActor_->VisibilityOff();

        applyBodyStyle();
        updateWaterPlane();
        frameBodies();
        framePending_ = true;
        renderWindow_->Render();
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
    try {
        if (bodies_.empty()) {
            throw std::runtime_error("Kein Modell für die 3D-Ansicht geladen.");
        }
        const auto pose = forcedisplay::readPose(state);
        const auto transform = poseTransform(pose);
        const Vec3 waterOrigin = inverseRotate(pose,
            {-pose.position[0], -pose.position[1], -pose.position[2]});
        const Vec3 waterNormal = inverseRotate(pose, {0.0, 0.0, 1.0});
        for (auto& body : bodies_) {
            body.body->SetUserTransform(transform);
            body.waterline->SetUserTransform(transform);
            body.localWaterPlane->SetOrigin(waterOrigin[0], waterOrigin[1], waterOrigin[2]);
            body.localWaterPlane->SetNormal(waterNormal[0], waterNormal[1], waterNormal[2]);
            body.cutter->Update();
        }
        pose_ = pose;
        forces_ = forces;
        forces_.contributions += forcedisplay::derivedContributions(forces);
        hasState_ = true;
        currentTime_ = state.value(QStringLiteral("zeit_s")).toDouble();
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
        ensureReferences();
        updateScaling();
        geometryDirty_ = true;
        updateWaterPlane();
        if (framePending_) {
            framePending_ = false;
            frameBodies();
        } else {
            keepBodiesInView();
        }
        renderer_->ResetCameraClippingRange();
        renderWindow_->Render();
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
    forces_ = {};
    hasState_ = false;
    scaled_.clear();
    track_.clear();
    currentTime_ = 0.0;
    trailActor_->VisibilityOff();
    geometryDirty_ = true;
    requestRender();
}

void VtkSceneWidget::updateTrail()
{
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
    const bool moved = options.visibility != options_.visibility;
    const bool rescale = options.forceMode != options_.forceMode
        || options.momentMode != options_.momentMode;
    options_ = options;
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
        renderWindow_->Render();
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
    return options_.visibility == Visibility::ThroughBody ? overlay_.Get() : renderer_.Get();
}

void VtkSceneWidget::moveForceActors()
{
    auto move = [this](vtkActor* actor) {
        renderer_->RemoveActor(actor);
        overlay_->RemoveActor(actor);
        forceRenderer()->AddActor(actor);
    };
    for (auto& visual : visuals_) {
        move(visual.lineActor);
        move(visual.headActor);
    }
    if (!bodies_.empty()) {
        move(leverActor_);
    }
}

void VtkSceneWidget::applyBodyStyle()
{
    const bool through = options_.forces && options_.visibility == Visibility::ThroughBody;
    for (auto& body : bodies_) {
        body.body->GetProperty()->SetOpacity(through ? std::min(body.opacity, 0.72) : body.opacity);
    }
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
    for (auto& visual : visuals_) {
        visual.lineActor->VisibilityOff();
        visual.headActor->VisibilityOff();
    }
    leverActor_->VisibilityOff();
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
}

void VtkSceneWidget::rebuildForceGeometry()
{
    hideForceVisuals();
    screenShapes_.clear();
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

bool VtkSceneWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == vtkWidget_) {
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

void VtkSceneWidget::rebuildEnvironment()
{
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
    auto* deepPoints = deepMesh_->GetPoints();
    auto* colors = vtkUnsignedCharArray::SafeDownCast(waterMesh_->GetPointData()->GetScalars());
    auto* deepColors = vtkUnsignedCharArray::SafeDownCast(deepMesh_->GetPointData()->GetScalars());
    auto setVertex = [&](vtkIdType id, double x, double y) {
        points->SetPoint(id, x, y, 0.0);
        deepPoints->SetPoint(id, x, y, -depth);
        const double r = std::hypot(x - cx, y - cy);
        // Auslauf zum Horizont: das Wasser geht in den Himmelsdunst über.
        const double fade = 1.0 - smoothstep(0.2 * radius, radius, r);
        const double haze = 0.75 * smoothstep(0.08 * radius, 0.85 * radius, r);
        // Fresnel: flacher Blickwinkel deckender, Blick von oben durchsichtiger.
        const Vec3 toCamera = normalized({position[0] - x, position[1] - y, position[2]});
        const double grazing = std::pow(1.0 - std::abs(toCamera[2]), 2.5);
        const double alpha = fade * std::clamp(0.34 + 0.6 * grazing, 0.0, 0.95);
        const unsigned char rgba[4] = {toByte(mix(waterColor[0], horizonColor[0], haze)),
            toByte(mix(waterColor[1], horizonColor[1], haze)),
            toByte(mix(waterColor[2], horizonColor[2], haze)), toByte(alpha)};
        colors->SetTypedTuple(id, rgba);
        const unsigned char deep[4] = {toByte(deepColor[0]), toByte(deepColor[1]),
            toByte(deepColor[2]), toByte(0.8 * fade)};
        deepColors->SetTypedTuple(id, deep);
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
    deepPoints->Modified();
    colors->Modified();
    deepColors->Modified();
    waterMesh_->Modified();
    deepMesh_->Modified();

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
            ids.clear();
            for (int k = -gridHalfLines; k <= gridHalfLines; ++k) {
                const double along = (direction == 0 ? gy : gx) + k * step;
                const double x = direction == 0 ? fixed : along;
                const double y = direction == 0 ? along : fixed;
                const double d = std::hypot(x - focal[0], y - focal[1]);
                const double alpha = (major ? 0.32 : 0.13) * (1.0 - smoothstep(0.15 * extent, extent, d));
                ids.push_back(gridPoints->InsertNextPoint(x, y, lift));
                const unsigned char rgba[4] = {205, 232, 255, toByte(alpha)};
                gridColors->InsertNextTypedTuple(rgba);
            }
            gridLines->InsertNextCell(static_cast<vtkIdType>(ids.size()), ids.data());
        }
    }
    gridMesh_->Initialize();
    gridMesh_->SetPoints(gridPoints);
    gridMesh_->SetLines(gridLines);
    gridMesh_->GetPointData()->SetScalars(gridColors);
    gridMesh_->Modified();
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
    frameBounds(bounds, 1.2);
    renderWindow_->Render();
}

void VtkSceneWidget::keepBodiesInView()
{
    double bounds[6];
    if (!bodyBounds(bounds)) {
        return;
    }
    bool inside = true;
    for (int corner = 0; corner < 8 && inside; ++corner) {
        renderer_->SetWorldPoint(bounds[(corner & 1) ? 1 : 0],
            bounds[(corner & 2) ? 3 : 2], bounds[(corner & 4) ? 5 : 4], 1.0);
        renderer_->WorldToView();
        double view[3];
        renderer_->GetViewPoint(view);
        inside = std::abs(view[0]) <= 0.92 && std::abs(view[1]) <= 0.92;
    }
    if (inside) {
        return;
    }
    // Nur erweitern: bisher sichtbare Lagen bleiben im Bild, Blickrichtung bleibt erhalten.
    if (viewBoundsValid_) {
        for (int axis = 0; axis < 3; ++axis) {
            bounds[axis * 2] = std::min(bounds[axis * 2], viewBounds_[axis * 2]);
            bounds[axis * 2 + 1] = std::max(bounds[axis * 2 + 1], viewBounds_[axis * 2 + 1]);
        }
    }
    std::copy(bounds, bounds + 6, viewBounds_);
    viewBoundsValid_ = true;
    renderer_->ResetCamera(bounds);
    renderer_->GetActiveCamera()->Zoom(0.95);
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
    std::copy(copy, copy + 6, viewBounds_);
    viewBoundsValid_ = true;
}
