#include "vtk_scene.hpp"

#include <QColor>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>
#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCommand.h>
#include <vtkCutter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPlaneSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

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
}

VtkSceneWidget::~VtkSceneWidget()
{
    renderWindow_->RemoveObserver(renderObserver_);
}

void VtkSceneWidget::clearModel()
{
    renderer_->RemoveAllViewProps();
    overlay_->RemoveAllViewProps();
    bodies_.clear();
    visuals_.clear();
    arcBasis_.clear();
    scaled_.clear();
    screenShapes_.clear();
    waterActor_ = nullptr;
    waterPlane_ = nullptr;
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
            auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputData(part.mesh);
            auto bodyActor = vtkSmartPointer<vtkActor>::New();
            bodyActor->SetMapper(mapper);
            if (part.hasColor) {
                bodyActor->GetProperty()->SetColor(part.color[0], part.color[1], part.color[2]);
            } else {
                bodyActor->GetProperty()->SetColor(
                    i % 2 == 0 ? 0.82 : 0.43, i % 2 == 0 ? 0.87 : 0.72, i % 2 == 0 ? 0.91 : 0.82);
            }
            bodyActor->GetProperty()->SetSpecular(0.35);
            bodyActor->GetProperty()->SetSpecularPower(28.0);
            bodyActor->GetProperty()->SetAmbient(0.14);
            bodyActor->GetProperty()->SetDiffuse(0.86);
            bodyActor->GetProperty()->SetInterpolationToPhong();
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
            body.waterline = waterlineActor;
            body.localWaterPlane = plane;
            body.cutter = cutter;
            bodies_.push_back(body);
        }

        waterPlane_ = vtkSmartPointer<vtkPlaneSource>::New();
        waterPlane_->SetXResolution(1);
        waterPlane_->SetYResolution(1);
        waterPlane_->SetOrigin(-1.0, -1.0, 0.0);
        waterPlane_->SetPoint1(1.0, -1.0, 0.0);
        waterPlane_->SetPoint2(-1.0, 1.0, 0.0);
        auto waterMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        waterMapper->SetInputConnection(waterPlane_->GetOutputPort());
        waterActor_ = vtkSmartPointer<vtkActor>::New();
        waterActor_->SetMapper(waterMapper);
        waterActor_->GetProperty()->SetColor(0.13, 0.48, 0.68);
        waterActor_->GetProperty()->SetOpacity(0.27);
        waterActor_->GetProperty()->SetLighting(false);
        waterActor_->PickableOff();
        renderer_->AddActor(waterActor_);
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
    geometryDirty_ = true;
    requestRender();
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
        body.body->GetProperty()->SetOpacity(through ? 0.72 : 1.0);
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
    if (waterActor_ == nullptr) {
        return;
    }
    double bounds[6];
    if (!bodyBounds(bounds)) {
        return;
    }
    const double span = std::max({bounds[1] - bounds[0], bounds[3] - bounds[2], 1.0});
    const double margin = span * 0.65;
    waterPlane_->SetOrigin(bounds[0] - margin, bounds[2] - margin, 0.0);
    waterPlane_->SetPoint1(bounds[1] + margin, bounds[2] - margin, 0.0);
    waterPlane_->SetPoint2(bounds[0] - margin, bounds[3] + margin, 0.0);
    waterPlane_->Update();
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
    for (const auto& state : states) {
        try {
            const auto point = forcedisplay::toWorldPoint(forcedisplay::readPose(state), center);
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
