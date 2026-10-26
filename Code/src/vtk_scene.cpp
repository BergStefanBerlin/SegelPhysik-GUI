#include "vtk_scene.hpp"

#include <QJsonArray>
#include <QJsonValue>
#include <QColor>
#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCutter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPlane.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkNew.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

using Vector3 = std::array<double, 3>;

Vector3 readVector(const QJsonValue& value, const QString& name)
{
    if (!value.isArray() || value.toArray().size() != 3) {
        throw std::runtime_error(
            QStringLiteral("Core-Zustand enthält keinen gültigen 3D-Vektor: %1")
                .arg(name)
                .toStdString());
    }
    const auto array = value.toArray();
    Vector3 vector{};
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

Vector3 rotate(const Vector3& vector, const Vector3& axis, double angle)
{
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    const double dot = vector[0] * axis[0] + vector[1] * axis[1]
        + vector[2] * axis[2];
    const Vector3 cross{
        axis[1] * vector[2] - axis[2] * vector[1],
        axis[2] * vector[0] - axis[0] * vector[2],
        axis[0] * vector[1] - axis[1] * vector[0]};
    return {
        vector[0] * cosine + cross[0] * sine + axis[0] * dot * (1.0 - cosine),
        vector[1] * cosine + cross[1] * sine + axis[1] * dot * (1.0 - cosine),
        vector[2] * cosine + cross[2] * sine + axis[2] * dot * (1.0 - cosine)};
}

struct PoseTransform {
    vtkSmartPointer<vtkTransform> transform;
    Vector3 position;
    Vector3 quaternionVector;
    double quaternionW = 1.0;
};

PoseTransform readPose(const QJsonObject& state)
{
    PoseTransform pose;
    pose.position = readVector(state.value(QStringLiteral("position_m")),
        QStringLiteral("position_m"));
    const auto quaternionValue = state.value(QStringLiteral("orientierung_xyzw"));
    if (!quaternionValue.isArray() || quaternionValue.toArray().size() != 4) {
        throw std::runtime_error("Core-Zustand enthält kein Quaternion xyzw.");
    }
    const auto quaternion = quaternionValue.toArray();
    double normSquared = 0.0;
    for (qsizetype i = 0; i < 4; ++i) {
        if (!quaternion.at(i).isDouble()
            || !std::isfinite(quaternion.at(i).toDouble())) {
            throw std::runtime_error(
                "Core-Zustand enthält kein endliches Quaternion xyzw.");
        }
        const double value = quaternion.at(i).toDouble();
        normSquared += value * value;
    }
    if (normSquared <= 0.0) {
        throw std::runtime_error("Core-Zustand enthält ein Null-Quaternion.");
    }
    const double inverseNorm = 1.0 / std::sqrt(normSquared);
    pose.quaternionVector = {
        quaternion.at(0).toDouble() * inverseNorm,
        quaternion.at(1).toDouble() * inverseNorm,
        quaternion.at(2).toDouble() * inverseNorm};
    pose.quaternionW = quaternion.at(3).toDouble() * inverseNorm;
    pose.transform = vtkSmartPointer<vtkTransform>::New();
    pose.transform->Translate(
        pose.position[0], pose.position[1], pose.position[2]);
    const double vectorNorm = std::sqrt(
        pose.quaternionVector[0] * pose.quaternionVector[0]
        + pose.quaternionVector[1] * pose.quaternionVector[1]
        + pose.quaternionVector[2] * pose.quaternionVector[2]);
    if (vectorNorm > 1e-12) {
        const double angle = 2.0 * std::atan2(vectorNorm, pose.quaternionW);
        pose.transform->RotateWXYZ(
            angle * 180.0 / std::acos(-1.0),
            pose.quaternionVector[0] / vectorNorm,
            pose.quaternionVector[1] / vectorNorm,
            pose.quaternionVector[2] / vectorNorm);
    }
    return pose;
}

Vector3 modelPointToWorld(const PoseTransform& pose, const Vector3& point)
{
    const double vectorNorm = std::sqrt(
        pose.quaternionVector[0] * pose.quaternionVector[0]
        + pose.quaternionVector[1] * pose.quaternionVector[1]
        + pose.quaternionVector[2] * pose.quaternionVector[2]);
    Vector3 rotated = point;
    if (vectorNorm > 1e-12) {
        rotated = rotate(point,
            {pose.quaternionVector[0] / vectorNorm,
             pose.quaternionVector[1] / vectorNorm,
             pose.quaternionVector[2] / vectorNorm},
            2.0 * std::atan2(vectorNorm, pose.quaternionW));
    }
    return {
        rotated[0] + pose.position[0],
        rotated[1] + pose.position[1],
        rotated[2] + pose.position[2]};
}

Vector3 modelVectorToWorld(const PoseTransform& pose, const Vector3& vector)
{
    const double vectorNorm = std::sqrt(
        pose.quaternionVector[0] * pose.quaternionVector[0]
        + pose.quaternionVector[1] * pose.quaternionVector[1]
        + pose.quaternionVector[2] * pose.quaternionVector[2]);
    if (vectorNorm <= 1e-12) {
        return vector;
    }
    return rotate(vector,
        {pose.quaternionVector[0] / vectorNorm,
         pose.quaternionVector[1] / vectorNorm,
         pose.quaternionVector[2] / vectorNorm},
        2.0 * std::atan2(vectorNorm, pose.quaternionW));
}

void setActorPose(vtkActor* actor, const PoseTransform& pose)
{
    actor->SetUserTransform(pose.transform);
}

vtkSmartPointer<vtkActor> makeArrowActor(
    const Vector3& point, const Vector3& vector, double length, const QColor& color)
{
    const double magnitude = std::sqrt(
        vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
    if (magnitude <= 1e-12) {
        return nullptr;
    }
    const Vector3 direction{
        vector[0] / magnitude, vector[1] / magnitude, vector[2] / magnitude};
    const Vector3 rotationAxis{
        0.0, -direction[2], direction[1]};
    const double axisNorm = std::sqrt(
        rotationAxis[0] * rotationAxis[0]
        + rotationAxis[1] * rotationAxis[1]
        + rotationAxis[2] * rotationAxis[2]);
    const double dot = std::clamp(direction[0], -1.0, 1.0);

    vtkNew<vtkArrowSource> arrow;
    arrow->SetTipLength(0.28);
    arrow->SetTipRadius(0.09);
    arrow->SetShaftRadius(0.025);
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(arrow->GetOutputPort());
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(
        color.redF(), color.greenF(), color.blueF());
    actor->GetProperty()->SetAmbient(0.2);
    actor->GetProperty()->SetDiffuse(0.8);

    auto transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(point[0], point[1], point[2]);
    if (axisNorm > 1e-12) {
        const double angle = std::acos(dot) * 180.0 / std::acos(-1.0);
        transform->RotateWXYZ(angle,
            rotationAxis[0] / axisNorm,
            rotationAxis[1] / axisNorm,
            rotationAxis[2] / axisNorm);
    } else if (dot < 0.0) {
        transform->RotateWXYZ(180.0, 0.0, 1.0, 0.0);
    }
    transform->Scale(length, length, length);
    actor->SetUserTransform(transform);
    return actor;
}

QColor colorForForce(const QString& id, const QString& elementId)
{
    if (id.endsWith(QStringLiteral(":gewicht"))) {
        return QColor(QStringLiteral("#ff745f"));
    }
    if (id.endsWith(QStringLiteral(":auftrieb"))) {
        return QColor(QStringLiteral("#42d5ff"));
    }
    if (elementId.isEmpty()) {
        return QColor(QStringLiteral("#f4d35e"));
    }
    return QColor(QStringLiteral("#80ed99"));
}

} // namespace

VtkSceneWidget::VtkSceneWidget(QWidget* parent)
    : QWidget(parent)
    , vtkWidget_(new QVTKOpenGLNativeWidget(this))
    , renderWindow_(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , renderer_(vtkSmartPointer<vtkRenderer>::New())
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(vtkWidget_);
    renderWindow_->AddRenderer(renderer_);
    renderWindow_->SetMultiSamples(0);
    vtkWidget_->setRenderWindow(renderWindow_);
    renderer_->SetBackground(0.035, 0.065, 0.11);
    renderer_->SetBackground2(0.08, 0.16, 0.23);
    renderer_->GradientBackgroundOn();
    renderer_->GetActiveCamera()->SetPosition(0.0, -1.0, 0.0);
    renderer_->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
}

bool VtkSceneWidget::setModel(const QString& modelPath, QString* error)
{
    try {
        renderer_->RemoveAllViewProps();
        bodies_.clear();
        forceActors_.clear();
        waterActor_ = nullptr;
        waterPlane_ = nullptr;
        lastState_ = {};
        const auto parts = loadDisplayParts(modelPath);

        for (qsizetype i = 0; i < parts.size(); ++i) {
            const auto& part = parts.at(i);
            auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputData(part.mesh);
            auto bodyActor = vtkSmartPointer<vtkActor>::New();
            bodyActor->SetMapper(mapper);
            bodyActor->GetProperty()->SetColor(
                i % 2 == 0 ? 0.82 : 0.43,
                i % 2 == 0 ? 0.87 : 0.72,
                i % 2 == 0 ? 0.91 : 0.82);
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
        waterActor_->GetProperty()->SetRepresentationToSurface();
        renderer_->AddActor(waterActor_);

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

bool VtkSceneWidget::setState(
    const QJsonObject& state, bool showForces, QString* error)
{
    try {
        if (bodies_.empty()) {
            throw std::runtime_error("Kein Modell für die 3D-Ansicht geladen.");
        }
        const auto pose = readPose(state);
        for (auto& body : bodies_) {
            setActorPose(body.body, pose);
            setActorPose(body.waterline, pose);
            const double quaternionVectorNorm = std::sqrt(
                pose.quaternionVector[0] * pose.quaternionVector[0]
                + pose.quaternionVector[1] * pose.quaternionVector[1]
                + pose.quaternionVector[2] * pose.quaternionVector[2]);
            const double inverseAngle =
                -2.0 * std::atan2(quaternionVectorNorm, pose.quaternionW);
            const Vector3 inverseAxis = quaternionVectorNorm > 1e-12
                ? Vector3{pose.quaternionVector[0] / quaternionVectorNorm,
                      pose.quaternionVector[1] / quaternionVectorNorm,
                      pose.quaternionVector[2] / quaternionVectorNorm}
                : Vector3{0.0, 0.0, 1.0};
            const Vector3 waterOriginInModel = quaternionVectorNorm > 1e-12
                ? rotate({-pose.position[0], -pose.position[1], -pose.position[2]},
                      inverseAxis, inverseAngle)
                : Vector3{-pose.position[0], -pose.position[1], -pose.position[2]};
            const Vector3 waterNormalInModel = quaternionVectorNorm > 1e-12
                ? rotate({0.0, 0.0, 1.0}, inverseAxis, inverseAngle)
                : Vector3{0.0, 0.0, 1.0};
            body.localWaterPlane->SetOrigin(
                waterOriginInModel[0], waterOriginInModel[1],
                waterOriginInModel[2]);
            body.localWaterPlane->SetNormal(
                waterNormalInModel[0], waterNormalInModel[1],
                waterNormalInModel[2]);
            body.cutter->Update();
        }
        lastState_ = state;
        showForces_ = showForces;
        updateWaterPlane();
        setForceVisibility(showForces_);
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

void VtkSceneWidget::clearForceActors()
{
    for (const auto& actor : forceActors_) {
        renderer_->RemoveActor(actor);
    }
    forceActors_.clear();
}

void VtkSceneWidget::setForceVisibility(bool visible)
{
    showForces_ = visible;
    clearForceActors();
    const bool arrowsShown = visible && !lastState_.isEmpty();
    for (auto& body : bodies_) {
        body.body->GetProperty()->SetOpacity(arrowsShown ? 0.45 : 1.0);
    }
    if (arrowsShown) {
        const auto contributions =
            lastState_.value(QStringLiteral("vektoren")).toArray();
        double maximumMagnitude = 0.0;
        for (const auto& value : contributions) {
            const auto item = value.toObject();
            if (item.value(QStringLiteral("art")).toString()
                != QStringLiteral("kraft")) {
                continue;
            }
            const auto vector = readVector(
                item.value(QStringLiteral("vektor")), QStringLiteral("vektor"));
            maximumMagnitude = std::max(maximumMagnitude, std::sqrt(
                vector[0] * vector[0] + vector[1] * vector[1]
                + vector[2] * vector[2]));
        }
        if (maximumMagnitude > 1e-12) {
            double bounds[6];
            const double maxLength = bodyBounds(bounds)
                ? 0.6 * std::max({bounds[1] - bounds[0], bounds[3] - bounds[2],
                      bounds[5] - bounds[4], 1e-3})
                : 1.0;
            const auto pose = readPose(lastState_);
            for (const auto& value : contributions) {
                const auto item = value.toObject();
                if (item.value(QStringLiteral("art")).toString()
                    != QStringLiteral("kraft")) {
                    continue;
                }
                const auto point = modelPointToWorld(pose, readVector(
                    item.value(QStringLiteral("punkt_m")),
                    QStringLiteral("punkt_m")));
                const auto vector = modelVectorToWorld(pose, readVector(
                    item.value(QStringLiteral("vektor")),
                    QStringLiteral("vektor")));
                const double magnitude = std::sqrt(
                    vector[0] * vector[0] + vector[1] * vector[1]
                    + vector[2] * vector[2]);
                const double length = maxLength * magnitude / maximumMagnitude;
                const auto actor = makeArrowActor(point, vector, length,
                    colorForForce(item.value(QStringLiteral("id")).toString(),
                        item.value(QStringLiteral("element_id")).toString()));
                if (actor != nullptr) {
                    renderer_->AddActor(actor);
                    forceActors_.push_back(actor);
                }
            }
        }
    }
    renderWindow_->Render();
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
    const double span = std::max(
        {bounds[1] - bounds[0], bounds[3] - bounds[2], 1.0});
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
    double meshBounds[6] = {
        maxValue, -maxValue, maxValue, -maxValue, maxValue, -maxValue};
    for (const auto& body : bodies_) {
        double single[6];
        body.mesh->GetBounds(single);
        for (int axis = 0; axis < 3; ++axis) {
            meshBounds[axis * 2] = std::min(meshBounds[axis * 2], single[axis * 2]);
            meshBounds[axis * 2 + 1] =
                std::max(meshBounds[axis * 2 + 1], single[axis * 2 + 1]);
        }
    }
    if (meshBounds[0] == maxValue) {
        return;
    }
    // Lageunabhängige Hülle: Kugel um das Mesh-Zentrum, entlang der Bahn verschoben.
    const Vector3 center{(meshBounds[0] + meshBounds[1]) / 2.0,
        (meshBounds[2] + meshBounds[3]) / 2.0, (meshBounds[4] + meshBounds[5]) / 2.0};
    const double radius = 0.5 * std::sqrt(
        std::pow(meshBounds[1] - meshBounds[0], 2.0)
        + std::pow(meshBounds[3] - meshBounds[2], 2.0)
        + std::pow(meshBounds[5] - meshBounds[4], 2.0));
    double bounds[6] = {
        maxValue, -maxValue, maxValue, -maxValue, maxValue, -maxValue};
    for (const auto& state : states) {
        try {
            const auto point = modelPointToWorld(readPose(state), center);
            for (int axis = 0; axis < 3; ++axis) {
                bounds[axis * 2] = std::min(bounds[axis * 2], point[axis] - radius);
                bounds[axis * 2 + 1] =
                    std::max(bounds[axis * 2 + 1], point[axis] + radius);
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
            bounds[axis * 2 + 1] =
                std::max(bounds[axis * 2 + 1], viewBounds_[axis * 2 + 1]);
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
