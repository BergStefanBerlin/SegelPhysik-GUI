#include "display_geometry.hpp"
#include "core_adapter.hpp"
#include "graphics_settings.hpp"
#include "vtk_scene.hpp"
#include "main_window.hpp"
#include "navigation.hpp"

#include <segelphysik/katalog.hpp>
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QKeyEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSurfaceFormat>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>
#include <QScrollArea>
#include <QSlider>
#include <QSizeGrip>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkNew.h>
#include <vtkLogger.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkCellArray.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkOutputWindow.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <algorithm>
#include <cmath>
#include <future>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {
class RenderErrors final : public vtkOutputWindow {
public:
    static RenderErrors* New() { return new RenderErrors; }
    void DisplayErrorText(const char* text) override
    {
        ++count;
        std::cerr << text;
    }
    int count = 0;
};

void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
QByteArray digest(const QImage& image)
{
    return QCryptographicHash::hash(
        QByteArrayView(reinterpret_cast<const char*>(image.constBits()), image.sizeInBytes()),
        QCryptographicHash::Sha256);
}
void waitUntil(const std::function<bool()>& ready, const char* message)
{
    QElapsedTimer timeout;
    timeout.start();
    while (!ready() && timeout.elapsed() < 15000) {
        QApplication::processEvents();
    }
    require(ready(), message);
}

void testPicking(VtkSceneWidget& scene, const QJsonObject& state,
    const forcedisplay::ForceState& forces, QJsonObject& report)
{
    using namespace forcedisplay;
    auto* widget = scene.findChild<QVTKOpenGLNativeWidget*>();
    require(widget != nullptr && forces.reference.has_value(), "picking fixture widget/reference absent");
    const auto pose = readPose(state);
    const auto reference = toWorldPoint(pose, *forces.reference);
    std::optional<Contribution> chosen;
    for (const auto& contribution : forces.contributions) {
        if (contribution.kind != Kind::Force || !isDrawable(contribution)) continue;
        const auto point = contribution.frame == QStringLiteral("welt")
            ? *contribution.point : toWorldPoint(pose, *contribution.point);
        const auto vector = contribution.frame == QStringLiteral("welt")
            ? *contribution.vector : toWorldVector(pose, *contribution.vector);
        const auto arm = perpendicularLeverArm(sub(point, reference), vector);
        if (arm && norm(*arm) > 0.01) {
            chosen = contribution;
            break;
        }
    }
    require(chosen.has_value(), "actual force with nonzero lever arm absent");
    // Keep actual force/pose/reference values; isolate selection and duplicate
    // only its display identity for the explicit overlap-menu test.
    chosen->category = Category::Test;
    auto isolated = forces;
    isolated.contributions = {*chosen};
    QString error;
    require(scene.setState(state, isolated, &error), "picking fixture state");
    const auto savedCamera = scene.cameraView();
    auto* renderer = widget->renderWindow()->GetRenderers()->GetFirstRenderer();
    auto project = [&](const Vec3& point) {
        renderer->SetWorldPoint(point[0], point[1], point[2], 1.0);
        renderer->WorldToDisplay();
        const auto* display = renderer->GetDisplayPoint();
        return QPointF(display[0] / widget->devicePixelRatioF(),
            (widget->renderWindow()->GetSize()[1] - display[1]) / widget->devicePixelRatioF());
    };
    const auto attack = chosen->frame == QStringLiteral("welt")
        ? *chosen->point : toWorldPoint(pose, *chosen->point);
    auto sendClick = [&](const QPointF& point) {
        const QPointF global = widget->mapToGlobal(point.toPoint());
        QMouseEvent press(QEvent::MouseButtonPress, point, global,
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease, point, global,
            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(widget, &press);
        QApplication::sendEvent(widget, &release);
    };
    QString picked;
    scene.onPicked = [&](const QString& id) { picked = id; };
    for (int profile = 0; profile < 2; ++profile) {
        const auto settings = graphics::defaults(profile);
        scene.setGraphicsProfile(settings);
        auto options = scene.options();
        options.forces = true;
        options.leverArm = false;
        options.visibility = settings.throughBody ? VtkSceneWidget::Visibility::ThroughBody
                                                 : VtkSceneWidget::Visibility::Spatial;
        scene.setOptions(options);
        require(scene.setCameraView(savedCamera), "picking camera restore");
        scene.renderNow();
        const auto click = project(attack);
        require(widget->rect().contains(click.toPoint()), "projected attack point outside viewport");
        picked.clear();
        scene.setSelected({});
        sendClick(click);
        waitUntil([&] { return !picked.isEmpty(); }, "actual mouse pick callback absent");
        require(picked == chosen->id && scene.selected() == chosen->id, "actual mouse pick wrong contribution");
        options.leverArm = true;
        scene.setOptions(options);
        scene.renderNow();
        const auto vector = chosen->frame == QStringLiteral("welt")
            ? *chosen->vector : toWorldVector(pose, *chosen->vector);
        const auto arm = perpendicularLeverArm(sub(attack, reference), vector);
        const auto foot = add(reference, *arm);
        vtkActor* leverActor = nullptr;
        auto* renderers = widget->renderWindow()->GetRenderers();
        renderers->InitTraversal();
        while (auto* candidateRenderer = renderers->GetNextItem()) {
            auto* actors = candidateRenderer->GetActors();
            actors->InitTraversal();
            while (auto* actor = actors->GetNextActor()) {
                auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
                if (!mapper || !actor->GetVisibility()) continue;
                auto* mesh = mapper->GetInput();
                if (!mesh || mesh->GetNumberOfPoints() < 2) continue;
                const auto* color = actor->GetProperty()->GetColor();
                if (std::abs(color[0] - 1.0) > 0.01 || std::abs(color[1] - 209.0 / 255.0) > 0.01)
                    continue;
                double a[3], b[3];
                mesh->GetPoint(0, a);
                mesh->GetPoint(1, b);
                if (norm(sub({a[0], a[1], a[2]}, reference)) < 1e-5
                    && norm(sub({b[0], b[1], b[2]}, foot)) < 1e-5) {
                    leverActor = actor;
                }
            }
        }
        require(leverActor != nullptr, "selected lever-arm actor lacks actual perpendicular endpoints");
        const auto referenceScreen = project(reference);
        const auto footScreen = project(foot);
        require(std::isfinite(referenceScreen.x()) && std::isfinite(footScreen.x()),
            "lever-arm endpoint projection invalid");
        options.leverArm = false;
        scene.setOptions(options);
        scene.renderNow();
        require(!leverActor->GetVisibility(), "lever-arm actor remains visible when disabled");
    }
    auto duplicate = *chosen;
    duplicate.id += QStringLiteral(":overlap-test");
    isolated.contributions.push_back(duplicate);
    require(scene.setState(state, isolated, &error), "overlap fixture state");
    require(scene.setCameraView(savedCamera), "overlap camera restore");
    scene.renderNow();
    picked.clear();
    bool menuVisited = false;
    QTimer menu;
    QObject::connect(&menu, &QTimer::timeout, [&] {
        auto* popup = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!popup) return;
        for (auto* action : popup->actions()) {
            if (action->data().toString() == duplicate.id) {
                menuVisited = true;
                popup->setActiveAction(action);
                QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                QApplication::sendEvent(popup, &enter);
                return;
            }
        }
        popup->close();
    });
    menu.start(10);
    sendClick(project(attack));
    waitUntil([&] { return !picked.isEmpty(); }, "overlap menu did not produce selection");
    menu.stop();
    require(menuVisited && picked == duplicate.id && scene.selected() == duplicate.id,
        "overlap menu did not select requested contribution");
    scene.onPicked = {};
    scene.setSelected({});
    require(scene.setState(state, forces, &error), "restore actual force state");
    require(scene.setCameraView(savedCamera), "restore post-picking camera");
    report.insert(QStringLiteral("mousePickingBothProfiles"), true);
    report.insert(QStringLiteral("overlapMenuSelection"), true);
    report.insert(QStringLiteral("leverArmActualEndpointsAndToggleBothProfiles"), true);
    report.insert(QStringLiteral("pickingFixture"),
        QStringLiteral("Actual Core force/pose/reference, isolated category; duplicate display ID only for overlap"));
}

void testHelperOcclusion(const QJsonObject& referenceState,
    const forcedisplay::ForceState& referenceForces, const QDir& evidence, QJsonObject& report)
{
    VtkSceneWidget scene;
    scene.resize(640, 480);
    vtkNew<vtkPoints> points;
    points->InsertNextPoint(-3, 0, -2);
    points->InsertNextPoint(3, 0, -2);
    points->InsertNextPoint(3, 0, 4);
    points->InsertNextPoint(-3, 0, 4);
    vtkNew<vtkCellArray> triangles;
    const vtkIdType first[] = {0, 1, 2};
    const vtkIdType second[] = {0, 2, 3};
    triangles->InsertNextCell(3, first);
    triangles->InsertNextCell(3, second);
    DisplayPart occluder;
    occluder.elementId = QStringLiteral("occluding-sail-fixture");
    occluder.mesh = vtkSmartPointer<vtkPolyData>::New();
    occluder.mesh->SetPoints(points);
    occluder.mesh->SetPolys(triangles);
    occluder.surface = DisplayPart::Surface::Matte;
    occluder.hasColor = true;
    std::fill(std::begin(occluder.color), std::end(occluder.color), 0.4);
    QString error;
    require(scene.setModel({occluder}, &error), "helper occlusion fixture model");
    auto state = referenceState;
    state.insert(QStringLiteral("position_m"), QJsonArray{0, 0, 0});
    state.insert(QStringLiteral("orientierung_xyzw"), QJsonArray{0, 0, 0, 1});
    auto forces = referenceForces;
    auto contribution = forces.contributions.front();
    contribution.id = QStringLiteral("occluded-helper-fixture");
    contribution.kind = forcedisplay::Kind::Force;
    contribution.category = forcedisplay::Category::Test;
    contribution.frame = QStringLiteral("welt");
    contribution.point = forcedisplay::Vec3{0, 1, 1};
    contribution.vector = forcedisplay::Vec3{1, 0, 0};
    forces.contributions = {contribution};
    auto moment = contribution;
    moment.id = QStringLiteral("occluded-moment-fixture");
    moment.kind = forcedisplay::Kind::Moment;
    moment.category = forcedisplay::Category::MomentSum;
    moment.point = forcedisplay::Vec3{-1, 2, 2};
    moment.vector = forcedisplay::Vec3{0, 1, 0};
    forces.contributions.push_back(moment);
    require(scene.setState(state, forces, &error), "helper occlusion fixture state");
    scene.setReferences(1, 1);
    scene.show();
    QApplication::processEvents();
    const VtkSceneWidget::CameraView camera{{0, -10, 1}, {0, 0, 1}, {0, 0, 1}};
    require(scene.setCameraView(camera), "helper occlusion camera");
    auto profile = graphics::defaults(1);
    for (auto feature : {graphics::Feature::Water, graphics::Feature::Grid,
             graphics::Feature::Axes, graphics::Feature::Trail, graphics::Feature::Waterline,
             graphics::Feature::Reflection}) {
        profile.enabled[static_cast<std::size_t>(feature)] = false;
    }
    for (bool shadows : {false, true}) {
        for (bool ssao : {false, true}) {
            profile.enabled[static_cast<std::size_t>(graphics::Feature::Shadows)] = shadows;
            profile.enabled[static_cast<std::size_t>(graphics::Feature::SSAO)] = ssao;
            scene.setGraphicsProfile(profile);
            auto options = scene.options();
            options.visibility = VtkSceneWidget::Visibility::Spatial;
            options.forces = false;
            scene.setOptions(options);
            const auto baseline = scene.captureImage();
            options.forces = true;
            scene.setOptions(options);
            const auto spatial = scene.captureImage();
            require(digest(spatial) == digest(baseline),
                "occluded helper leaks into scene color/shadows/SSAO");
            options.visibility = VtkSceneWidget::Visibility::ThroughBody;
            scene.setOptions(options);
            const auto throughBody = scene.captureImage();
            require(digest(throughBody) != digest(baseline), "through-body helper must remain visible");
            options.visibility = VtkSceneWidget::Visibility::Spatial;
            scene.setOptions(options);
            require(digest(scene.captureImage()) == digest(baseline), "spatial depth not restored after through-body");
            if (shadows && ssao) {
                require(spatial.save(evidence.filePath(QStringLiteral("helper_occluded.bmp")), "BMP"),
                    "helper occlusion screenshot");
                require(throughBody.save(evidence.filePath(QStringLiteral("helper_through_body.bmp")), "BMP"),
                    "helper through-body screenshot");
            }
        }
    }
    report.insert(QStringLiteral("helperScenePassIsolationAndOcclusion"), true);
}

void test420Dynamic(const QString& models, const segelphysik::Modellkatalog& catalog,
    const QDir& evidence, QJsonObject& report)
{
    const auto* entry = catalog.eintrag("420");
    require(entry != nullptr, "420 absent from catalog");
    const auto parts = toDisplayParts(
        segelphysik::darstellungsgeometrie(catalog, entry->modell_id, entry->modell_fassung));
    const auto modelPath = QDir(models).filePath(QStringLiteral("420/modell.json"));
    const auto validation = validateModel(modelPath);
    require(validation.valid, "420 validation");
    QString error;
    auto live = LiveSimulation::create(modelPath, validation, 1.0, 0.01, false, &error);
    require(live != nullptr, "420 live simulation creation");
    std::atomic_bool cancellation{false};
    live->run(cancellation);
    const auto progress = live->progress();
    require(progress.finished && progress.states > 50, "420 live simulation states");
    VtkSceneWidget scene;
    scene.resize(800, 800);
    require(scene.setModel(parts, &error), "420 model display");
    QHash<QString, QString> groups;
    for (const auto& part : parts) groups.insert(part.elementId, part.group);
    scene.setElementGroups(groups);
    scene.show();
    QApplication::processEvents();
    const VtkSceneWidget::CameraView camera{{8, -12, 8}, {0, 0, 2}, {0, 0, 1}};
    require(scene.setCameraView(camera), "420 camera");
    for (int profileIndex = 0; profileIndex < 2; ++profileIndex) {
        scene.setGraphicsProfile(graphics::defaults(profileIndex));
        for (qsizetype i = 0; i < progress.states; ++i) {
            const auto state = live->stateJson(i, &error);
            require(!state.isEmpty(), "420 live state JSON");
            require(scene.setState(state, forcedisplay::parseForceState(state), &error),
                "420 live state display");
            scene.renderNow();
            if (i == 0 || i == 50 || i == progress.states - 1) {
                const auto cameraView = scene.cameraView();
                const auto expectedValues = navigation::derive(forcedisplay::readPose(state),
                    forcedisplay::sub(cameraView.position, cameraView.focalPoint));
                const std::array<navigation::Instrument::Kind, 3> kinds{
                    navigation::Instrument::Kind::Compass, navigation::Instrument::Kind::Roll,
                    navigation::Instrument::Kind::Pitch};
                const std::array<const char*, 3> names{
                    "navigation_compass", "navigation_roll", "navigation_pitch"};
                for (std::size_t n = 0; n < kinds.size(); ++n) {
                    navigation::Instrument expected(kinds[n], nullptr);
                    expected.setValues(expectedValues);
                    auto* actual = scene.findChild<QWidget*>(QString::fromLatin1(names[n]));
                    require(actual && dynamic_cast<navigation::Instrument*>(actual)->renderedSvg()
                        == expected.renderedSvg(), "420 instruments do not match selected snapshot/camera");
                }
            }
            if (i == 0 || i == 10 || i == 50 || i == progress.states - 1) {
                const auto prefix = QStringLiteral("420_dynamic_%1_%2").arg(profileIndex).arg(i);
                require(scene.captureImage().save(evidence.filePath(prefix + QStringLiteral(".bmp")), "BMP"),
                    "420 dynamic image");
                if (i == 50) {
                    const auto withoutTrail = scene.captureImage();
                    auto withTrail = graphics::defaults(profileIndex);
                    require(!withTrail.on(graphics::Feature::Trail), "course trail must default off");
                    withTrail.enabled[static_cast<std::size_t>(graphics::Feature::Trail)] = true;
                    scene.setGraphicsProfile(withTrail);
                    const auto enabledImage = scene.captureImage();
                    require(digest(enabledImage) != digest(withoutTrail), "optional 420 course trail lost");
                    require(enabledImage.save(evidence.filePath(
                        prefix + QStringLiteral("_trail_enabled.bmp")), "BMP"), "420 optional course trail");
                    auto* widget = scene.findChild<QVTKOpenGLNativeWidget*>();
                    auto* actors = widget->renderWindow()->GetRenderers()->GetFirstRenderer()->GetActors();
                    actors->InitTraversal();
                    vtkActor* trail = nullptr;
                    while (auto* actor = actors->GetNextActor()) {
                        auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
                        if (!mapper) continue;
                        auto* mesh = mapper->GetInput();
                        if (!mesh || mesh->GetNumberOfLines() != 1
                            || mesh->GetNumberOfPoints() != i + 1) continue;
                        auto* colors = mesh->GetPointData()->GetScalars();
                        if (!colors || colors->GetNumberOfComponents() != 4) continue;
                        double rgba[4];
                        colors->GetTuple(0, rgba);
                        if (rgba[0] == 242 && rgba[1] == 242 && rgba[2] == 228) trail = actor;
                    }
                    require(trail && trail->GetVisibility(), "420 actual course-trail actor absent");
                    scene.setGraphicsProfile(graphics::defaults(profileIndex));
                    require(!trail->GetVisibility(), "420 course trail remains visible when disabled");
                    require(digest(scene.captureImage()) == digest(withoutTrail),
                        "420 disabling course trail must restore the exact image");
                }
            }
        }
    }
    report.insert(QStringLiteral("420DynamicStates"), static_cast<int>(progress.states));
    report.insert(QStringLiteral("420CourseTrailDefaultsOffAndOptional"), true);
    report.insert(QStringLiteral("420NavigationSelectedSnapshotBothProfiles"), true);
}

void testMainWindow(const QString& models, const QDir& evidence, QJsonObject& report)
{
    require(evidence.mkpath(QStringLiteral("gui-test-settings")), "isolated GUI settings directory");
    QCoreApplication::setOrganizationName(QStringLiteral("SegelPhysikTests"));
    QCoreApplication::setApplicationName(QStringLiteral("GraphicsIntegration"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
        evidence.filePath(QStringLiteral("gui-test-settings")));
    QSettings().clear();
    bool unexpectedDialog = false;
    QTimer errors;
    QObject::connect(&errors, &QTimer::timeout, [&] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (auto* message = qobject_cast<QMessageBox*>(widget)) {
                std::cerr << message->text().toStdString() << '\n';
                unexpectedDialog = true;
                message->reject();
            }
        }
    });
    errors.start(50);
    MainWindow window;
    window.setModelFolder(models);
    window.selectInitialModel(QStringLiteral("folkeboot"));
    window.show();
    QPushButton* run = nullptr;
    for (auto* button : window.findChildren<QPushButton*>()) {
        if (button->text() == QStringLiteral("Core-Lauf starten")) run = button;
    }
    auto* stop = window.findChild<QPushButton*>(QStringLiteral("stopSimulation"));
    auto* mode = window.findChild<QComboBox*>(QStringLiteral("simulationMode"));
    auto* duration = window.findChild<QDoubleSpinBox*>(QStringLiteral("simulationDuration"));
    auto* profiles = window.findChild<QComboBox*>(QStringLiteral("graphicsProfile"));
    auto* vectors = window.findChild<QCheckBox*>(QStringLiteral("displayVectors"));
    auto* status = window.findChild<QLabel*>(QStringLiteral("status"));
    auto* cameraCombo = window.findChild<QComboBox*>(QStringLiteral("cameraPreset"));
    auto* navigationCheck = window.findChild<QCheckBox*>(QStringLiteral("displayNavigation"));
    auto* slider = window.findChild<QSlider*>();
    auto* values = window.findChild<QTreeWidget*>();
    require(run && stop && mode && duration && profiles && vectors && slider && values && status
        && cameraCombo && navigationCheck,
        "MainWindow integration controls absent");
    waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "Folkeboot GUI load timeout");
    require(!unexpectedDialog, "unexpected model load error");
    VtkSceneWidget* scene = nullptr;
    for (auto* widget : window.findChildren<QWidget*>()) {
        if (auto* candidate = dynamic_cast<VtkSceneWidget*>(widget)) scene = candidate;
    }
    require(scene != nullptr, "MainWindow scene absent");
    auto* overlay = scene->findChild<QWidget*>(QStringLiteral("navigationOverlay"));
    require(overlay && overlay->isVisible(), "viewport navigation overlay absent");
    require(cameraCombo->isEnabled() && cameraCombo->count() == 15, "14 camera presets plus free view");
    const auto selectCamera = [cameraCombo](int index) {
        cameraCombo->setCurrentIndex(index);
        require(QMetaObject::invokeMethod(cameraCombo, "activated", Q_ARG(int, index)),
            "activate camera preset dropdown");
    };
    const auto initialCamera = scene->cameraView();
    for (const auto& preset : navigation::cameraPresets()) {
        const auto index = cameraCombo->findData(QString::fromLatin1(preset.id));
        require(index > 0 && cameraCombo->itemText(index) == QString::fromUtf8(preset.label),
            "legacy camera preset ID/label absent");
        selectCamera(index);
        const auto applied = scene->cameraView();
        const auto offset = forcedisplay::sub(applied.position, applied.focalPoint);
        require(forcedisplay::dot(offset, preset.direction) > 0
            && forcedisplay::norm(forcedisplay::cross(offset, preset.direction))
                < 1e-8 * forcedisplay::norm(offset), "camera preset world direction");
        require(forcedisplay::norm(forcedisplay::sub(applied.viewUp, preset.up)) < 1e-8,
            "camera preset up vector");
    }
    QString cameraError;
    require(!scene->setCameraPreset(QStringLiteral("invalid"), &cameraError)
        && !cameraError.isEmpty(), "unknown camera preset requires explicit error");
    require(scene->setCameraView(initialCamera), "restore camera after presets");
    selectCamera(cameraCombo->currentIndex());
    const auto reapplied = scene->cameraView();
    require(reapplied.position[2] < reapplied.focalPoint[2], "reselecting same preset must reapply camera");
    require(scene->setCameraView(initialCamera), "restore camera after repeated preset");
    navigationCheck->setChecked(false);
    require(!overlay->isVisible(), "navigation visibility off");
    navigationCheck->setChecked(true);
    require(overlay->isVisible(), "navigation visibility on");
    vectors->setChecked(false);
    require(profiles->isEnabled(), "hidden vectors disable profile selection");
    vectors->setChecked(true);
    mode->setCurrentIndex(1);
    duration->setValue(2.0);
    waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "GUI mode change readiness");
    run->click();
    waitUntil([&] { return slider->maximum() > 2 || unexpectedDialog; }, "GUI live states absent");
    require(!unexpectedDialog, "unexpected live error");
    slider->setValue(0);
    const auto camera = scene->cameraView();
    auto* compassWidget = scene->findChild<QWidget*>(QStringLiteral("navigation_compass"));
    auto* compass = dynamic_cast<navigation::Instrument*>(compassWidget);
    require(compass, "live GUI compass absent");
    scene->renderNow();
    const auto initialCompass = compass->renderedSvg();
    auto rotatedCamera = camera;
    const auto offset = forcedisplay::sub(camera.position, camera.focalPoint);
    rotatedCamera.position = forcedisplay::add(camera.focalPoint, {-offset[1], offset[0], offset[2]});
    require(scene->setCameraView(rotatedCamera), "actual camera orbit");
    scene->renderNow();
    require(compass->renderedSvg() != initialCompass, "camera marker does not follow actual camera");
    require(scene->setCameraView(camera), "restore actual camera");
    scene->renderNow();
    require(compass->renderedSvg() == initialCompass, "camera marker restoration");
    QStringList before;
    for (QTreeWidgetItemIterator item(values); *item; ++item) {
        for (int column = 0; column < values->columnCount(); ++column) before << (*item)->text(column);
    }
    require(!before.isEmpty(), "GUI force table empty");
    profiles->setCurrentIndex(1);
    QStringList after;
    for (QTreeWidgetItemIterator item(values); *item; ++item) {
        for (int column = 0; column < values->columnCount(); ++column) after << (*item)->text(column);
    }
    require(before == after && slider->value() == 0, "profile switch changed selected result or values");
    selectCamera(cameraCombo->findData(QStringLiteral("oben_steuerbord_bug")));
    QStringList afterCamera;
    for (QTreeWidgetItemIterator item(values); *item; ++item)
        for (int column = 0; column < values->columnCount(); ++column) afterCamera << (*item)->text(column);
    require(before == afterCamera && slider->value() == 0, "camera preset changed selected physical state");
    require(scene->setCameraView(camera), "restore camera after physical-state check");
    const auto switchedCamera = scene->cameraView();
    require(camera.position == switchedCamera.position && camera.focalPoint == switchedCamera.focalPoint,
        "GUI profile switch changed camera");
    QAction* graphicsAction = nullptr;
    for (auto* action : window.menuBar()->actions()) {
        if (action->text() == QStringLiteral("Setup") && action->menu()) {
            require(!action->menu()->actions().isEmpty(), "Setup menu empty");
            graphicsAction = action->menu()->actions().front();
        }
    }
    require(graphicsAction && graphicsAction->text().startsWith(QStringLiteral("Grafik")),
        "Graphics must be first Setup action");
    bool setupVisited = false;
    QTimer::singleShot(0, [&] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (auto* dialog = qobject_cast<QDialog*>(widget)) {
                if (dialog->objectName() == QStringLiteral("graphicsSetup")) {
                    setupVisited = true;
                    stop->click();
                    dialog->reject();
                }
            }
        }
    });
    graphicsAction->trigger();
    require(setupVisited, "Setup not usable during live run");
    waitUntil([&] { return !stop->isEnabled() || unexpectedDialog; }, "GUI cancellation timeout");
    require(!unexpectedDialog && status->text().contains(QStringLiteral("abgebrochen")),
        "GUI cancellation did not preserve partial status");
    require(slider->maximum() > 0, "GUI cancellation lost recorded states");
    profiles->setCurrentIndex(0);
    profiles->setCurrentIndex(1);
    require(slider->value() == 0, "post-cancel profile switch lost playback selection");
    report.insert(QStringLiteral("mainWindowIntegration"), true);
    for (auto* area : window.findChildren<QScrollArea*>())
        if (area->widget() && area->widget()->isAncestorOf(cameraCombo))
            area->ensureWidgetVisible(cameraCombo);
    QApplication::processEvents();
    require(window.grab().save(evidence.filePath(QStringLiteral("navigation_mainwindow.bmp")), "BMP"),
        "navigation MainWindow screenshot");
    report.insert(QStringLiteral("navigationOverlayAnd14CameraPresets"), true);
    report.insert(QStringLiteral("devicePixelRatio"), window.devicePixelRatioF());
    navigationCheck->setChecked(false);
    window.close();
    require(QSettings().value(QStringLiteral("darstellung/navigation")).toBool() == false,
        "navigation visibility must be saved");
    MainWindow reopened;
    auto* restoredNavigation = reopened.findChild<QCheckBox*>(QStringLiteral("displayNavigation"));
    require(restoredNavigation && !restoredNavigation->isChecked(),
        "navigation visibility must be restored in the next window");
    reopened.close();
}

void testBodyFixedCamera(const QString& models, const QString& evidencePath)
{
    using namespace forcedisplay;
    const auto catalog = segelphysik::modellkatalog(std::filesystem::path(models.toStdWString()));
    const auto* entry = catalog.eintrag("420");
    require(entry, "camera test requires 420");
    const auto parts = toDisplayParts(segelphysik::darstellungsgeometrie(
        catalog, entry->modell_id, entry->modell_fassung));
    const auto path = QDir(models).filePath(QStringLiteral("420/modell.json"));
    const auto validation = validateModel(path);
    require(validation.valid, "420 camera test validation");
    std::atomic_bool cancellation{false};
    const auto simulation = runSimulation(path, validation, true, 0.1, 0.01, &cancellation);
    require(simulation.completed && simulation.states.size() > 2, "short real 420 camera fixture");
    VtkSceneWidget scene;
    scene.resize(1000, 850);
    QString error;
    require(scene.setModel(parts, &error), "camera fixture model");
    scene.show();
    QApplication::processEvents();
    auto state = simulation.states.front();
    auto* overlay = scene.findChild<QWidget*>(QStringLiteral("navigationOverlay"));
    require(overlay && overlay->size() == QSize(180, 564), "initial navigation size in actual VTK viewport");
    auto* grip = overlay->findChild<QSizeGrip*>(QStringLiteral("navigationSizeGrip"));
    require(grip, "actual viewport resize grip");
    const QPointF local(4, 4);
    const auto global = grip->mapToGlobal(local.toPoint());
    QMouseEvent press(QEvent::MouseButtonPress, local, global,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(grip, &press);
    QMouseEvent move(QEvent::MouseMove, local, global + QPoint(100, 200),
        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(grip, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, local, global + QPoint(100, 200),
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(grip, &release);
    require(overlay->size() == QSize(280, 764), "resize grip must grow instruments in actual VTK viewport");
    require(scene.setState(state, parseForceState(state), &error), "camera initial state");
    require(scene.setCameraPreset(QStringLiteral("oben_steuerbord_bug"), &error), "body preset");
    const auto localView = [](const VtkSceneWidget::CameraView& view, const Pose& pose) {
        Pose inverse;
        inverse.quaternionXyzw = {-pose.quaternionXyzw[0], -pose.quaternionXyzw[1],
            -pose.quaternionXyzw[2], pose.quaternionXyzw[3]};
        return VtkSceneWidget::CameraView{
            toWorldVector(inverse, sub(view.position, pose.position)),
            toWorldVector(inverse, sub(view.focalPoint, pose.position)),
            toWorldVector(inverse, view.viewUp)};
    };
    const auto sameView = [](const VtkSceneWidget::CameraView& actual,
                              const VtkSceneWidget::CameraView& expected) {
        require(norm(sub(actual.position, expected.position)) < 1e-8, "body-fixed camera position drift");
        require(norm(sub(actual.focalPoint, expected.focalPoint)) < 1e-8, "body-fixed camera target drift");
        require(norm(sub(actual.viewUp, expected.viewUp)) < 1e-8, "body-fixed camera up drift");
    };
    auto reference = localView(scene.cameraView(), readPose(state));
    for (const auto& liveState : simulation.states) {
        require(scene.setState(liveState, parseForceState(liveState), &error), "420 camera tracking state");
        sameView(localView(scene.cameraView(), readPose(liveState)), reference);
    }
    // Exercise translation and every rotational axis beyond the small real wave motion.
    for (int axis = 0; axis < 3; ++axis) {
        state.insert(QStringLiteral("position_m"), QJsonArray{10.0 * axis, -7.0, 2.0});
        QJsonArray quaternion{0.0, 0.0, 0.0, 0.5};
        quaternion[axis] = std::sqrt(0.75);
        state.insert(QStringLiteral("orientierung_xyzw"), quaternion);
        require(scene.setState(state, parseForceState(state), &error), "translated/rotated camera fixture");
        sameView(localView(scene.cameraView(), readPose(state)), reference);
        for (const auto& preset : navigation::cameraPresets()) {
            require(scene.setCameraPreset(QString::fromLatin1(preset.id), &error), "rotated body preset");
            const auto bodyView = localView(scene.cameraView(), readPose(state));
            const auto direction = sub(bodyView.position, bodyView.focalPoint);
            if (preset.direction[2] == 0.0) {
                const auto view = scene.cameraView();
                require(std::abs(view.position[2]) < 1e-10
                    && std::abs(view.focalPoint[2]) < 1e-10,
                    "side/bow/stern camera and target must be on water plane");
                auto expected = toWorldVector(readPose(state), preset.direction);
                expected[2] = 0;
                const auto actual = sub(view.position, view.focalPoint);
                require(dot(actual, expected) > 0 && norm(cross(actual, expected)) < 1e-8 * norm(actual),
                    "water-level camera must use horizontal yacht axis projection");
                require(norm(sub(view.viewUp, {0, 0, 1})) < 1e-8, "water-level up");
            } else {
                require(dot(direction, preset.direction) > 0
                    && norm(cross(direction, preset.direction)) < 1e-8 * norm(direction),
                    "preset must use current yacht axes, not world axes");
                require(norm(sub(bodyView.viewUp, preset.up)) < 1e-8, "preset body-fixed up");
            }
        }
        reference = localView(scene.cameraView(), readPose(state));
    }
    auto edited = scene.cameraView();
    edited.position = add(edited.position, {2.0, 1.0, 0.5});
    edited.focalPoint = add(edited.focalPoint, {0.5, 0.25, 0.0});
    require(scene.setCameraView(edited), "free camera edit");
    reference = localView(scene.cameraView(), readPose(state));
    scene.frameStates(simulation.states);
    sameView(localView(scene.cameraView(), readPose(state)), reference);
    require(scene.setState(simulation.states.front(), parseForceState(simulation.states.front()), &error),
        "rewind with edited camera");
    sameView(localView(scene.cameraView(), readPose(simulation.states.front())), reference);
    scene.clearState();
    require(scene.setState(state, parseForceState(state), &error), "camera after clear state");
    sameView(localView(scene.cameraView(), readPose(state)), reference);
    scene.renderNow();
    QDir evidence(evidencePath);
    require(evidence.mkpath(QStringLiteral(".")), "camera evidence directory");
    require(scene.captureImage().save(evidence.filePath(QStringLiteral("body_fixed_camera.bmp")), "BMP"),
        "camera evidence image");
    require(scene.grab().save(evidence.filePath(QStringLiteral("resizable_navigation.bmp")), "BMP"),
        "overlay evidence image");
    for (const auto* id : {"backbord", "steuerbord", "bug", "heck"}) {
        require(scene.setCameraPreset(QString::fromLatin1(id), &error), "water-level preset");
        const auto anchored = localView(scene.cameraView(), readPose(state));
        auto* viewport = scene.findChild<QVTKOpenGLNativeWidget*>();
        auto* renderer = viewport->renderWindow()->GetRenderers()->GetFirstRenderer();
        for (const auto& part : parts) {
            double bounds[6];
            part.mesh->GetBounds(bounds);
            for (int corner = 0; corner < 8; ++corner) {
                const auto point = toWorldPoint(readPose(state),
                    {bounds[(corner & 1) ? 1 : 0], bounds[(corner & 2) ? 3 : 2],
                        bounds[(corner & 4) ? 5 : 4]});
                renderer->SetWorldPoint(point[0], point[1], point[2], 1);
                renderer->WorldToView();
                double projected[3];
                renderer->GetViewPoint(projected);
                require(std::abs(projected[0]) <= 1 && std::abs(projected[1]) <= 1,
                    "water-level camera must fit full hull and mast, not clip the sail");
            }
        }
        require(scene.captureImage().save(evidence.filePath(
            QStringLiteral("water_level_%1.bmp").arg(QString::fromLatin1(id))), "BMP"), "water-level image");
        require(scene.setState(simulation.states.front(), parseForceState(simulation.states.front()), &error),
            "water-level preset tracking");
        sameView(localView(scene.cameraView(), readPose(simulation.states.front())), anchored);
        require(scene.setState(state, parseForceState(state), &error), "water-level fixture restore");
    }
    std::cout << "Body-fixed camera: short real 420 run, translation, roll/pitch/yaw, 14 presets, "
                 "free edit, rewind and clear-state passed\n";
}

void testWaterMotion(const QString& models, const QString& evidencePath)
{
    using namespace forcedisplay;
    QDir evidence(evidencePath);
    require(evidence.mkpath(QStringLiteral(".")), "water-motion evidence directory");
    const auto catalog = segelphysik::modellkatalog(std::filesystem::path(models.toStdWString()));
    QJsonObject report;
    for (const QString& id : {QStringLiteral("420"), QStringLiteral("folkeboot")}) {
        const auto* entry = catalog.eintrag(id.toStdString());
        require(entry != nullptr, "motion model absent");
        const auto parts = toDisplayParts(segelphysik::darstellungsgeometrie(
            catalog, entry->modell_id, entry->modell_fassung));
        const auto path = QDir(models).filePath(id + QStringLiteral("/modell.json"));
        const auto validation = validateModel(path);
        QString error;
        auto live = LiveSimulation::create(path, validation, 1.0, 0.01, false, &error, {},
            TestDriveOptions{true, 200, 0.1});
        require(live != nullptr, "real test-drive render fixture initialization");
        std::atomic_bool cancellation{false};
        live->run(cancellation);
        const auto progress = live->progress();
        if (progress.status != QStringLiteral("abgeschlossen"))
            std::cerr << progress.diagnostic.toStdString() << '\n';
        require(progress.status == QStringLiteral("abgeschlossen"), "real test-drive run failed");
        const auto last = live->stateJson(progress.states - 1, &error);
        const auto force = parseForceState(last);
        require(force.contract == QStringLiteral("3"), "test-drive not V3");
        bool capped = false;
        for (qsizetype i = 1; i < progress.states; ++i) {
            const auto s = live->stateJson(i, &error);
            const auto v = s.value(QStringLiteral("geschwindigkeit_welt_m_s")).toArray();
            require(std::hypot(v[0].toDouble(), v[1].toDouble()) <= 0.1000000001, "real speed cap violated");
            capped = capped || parseForceState(s).testLimiterApplied;
        }
        require(capped, "short real model did not exercise limiter");
        VtkSceneWidget scene;
        scene.resize(1000, 740);
        require(scene.setModel(parts, &error), "motion scene load");
        scene.show();
        QApplication::processEvents();
        auto profile = graphics::defaults(0);
        profile.trackSpacing = 0.005;
        profile.markSpacing = 0.02;
        profile.trackBudget = 64;
        profile.enabled[static_cast<std::size_t>(graphics::Feature::Wake)] = true;
        scene.setGraphicsProfile(profile);
        const auto source = live->motionStates(0, progress.states, &error);
        require(error.isEmpty() && source.size() == progress.states, "complete raw history");
        require(scene.appendMotionStates(source, &error), "motion history append");
        require(scene.setState(last, force, &error), "motion state display");
        scene.renderNow();
        const auto geometry = scene.motionGeometry();
        require(geometry.supports > 2 && geometry.supports <= 64 && geometry.trackLines > 0,
            "actual water path/budget absent");
        require(geometry.trackLines <= 128 && geometry.wakeLines <= 130, "GPU geometry exceeds proportional budget");
        for (const auto& p : scene.motionHistory().points()) {
            require(p.center[2] == 0, "technical track left water plane");
        }
        const auto camera = scene.cameraView();
        const auto snapshot = QJsonDocument(last).toJson();
        const auto originalHistory = scene.motionHistory().points();
        profile.enabled[static_cast<std::size_t>(graphics::Feature::Wake)] = false;
        scene.setGraphicsProfile(profile);
        require(scene.motionGeometry().wakeLines == 0 && scene.motionGeometry().trackLines > 0, "technical-only option");
        require(scene.captureImage().save(evidence.filePath(id + QStringLiteral("_technik.bmp")), "BMP"), "technical image");
        profile.enabled[static_cast<std::size_t>(graphics::Feature::WaterTrack)] = false;
        scene.setGraphicsProfile(profile);
        require(scene.motionGeometry().trackLines == 0 && scene.motionGeometry().wakeLines == 0,
            "both motion options off; distance marks must suspend");
        profile.enabled[static_cast<std::size_t>(graphics::Feature::WaterTrack)] = true;
        profile.enabled[static_cast<std::size_t>(graphics::Feature::Wake)] = true;
        auto optical = graphics::defaults(1);
        optical.trackSpacing = profile.trackSpacing;
        optical.markSpacing = profile.markSpacing;
        optical.trackBudget = profile.trackBudget;
        scene.setGraphicsProfile(optical);
        scene.renderNow();
        require(scene.motionGeometry().trackLines == 0, "technical path visible in optical defaults");
        require(scene.captureImage().save(evidence.filePath(id + QStringLiteral("_optik.bmp")), "BMP"), "optical image");
        require(norm(sub(scene.cameraView().position, camera.position)) < 1e-10
            && QJsonDocument(live->stateJson(progress.states - 1, &error)).toJson() == snapshot,
            "graphics profile changed camera/physics");
        scene.resetMotionHistory();
        const qsizetype middle = progress.states / 2;
        require(scene.appendMotionStates(live->motionStates(0, middle + 1, &error), &error), "rewind reconstruction");
        require(scene.appendMotionStates(live->motionStates(middle + 1, progress.states, &error), &error), "forward reconstruction");
        require(scene.motionHistory().points().size() == originalHistory.size(), "replay changes sample count");
        for (std::size_t i = 0; i < originalHistory.size(); ++i) {
            require(norm(sub(scene.motionHistory().points()[i].center, originalHistory[i].center)) < 1e-12,
                "replay changes sampled geometry");
        }
        scene.resetMotionHistory();
        QVector<QJsonObject> visualStates;
        const auto initialPosition = last.value(QStringLiteral("position_m")).toArray();
        for (int i = 0; i <= 20; ++i) {
            auto s = last;
            s.insert(QStringLiteral("zeit_s"), i * 0.2);
            s.insert(QStringLiteral("position_m"), QJsonArray{initialPosition[0].toDouble() + i * 0.25,
                initialPosition[1].toDouble(), initialPosition[2].toDouble()});
            s.insert(QStringLiteral("geschwindigkeit_welt_m_s"), QJsonArray{1.25, 0, 0});
            visualStates.push_back(s);
        }
        profile.trackSpacing = 0.25;
        profile.markSpacing = 1.0;
        scene.setGraphicsProfile(profile);
        require(scene.appendMotionStates(visualStates, &error), "synthetic motion visual history");
        require(scene.setState(visualStates.back(), parseForceState(visualStates.back()), &error), "synthetic motion display");
        require(scene.motionGeometry().wakeLines > 0, "valid real-model contours did not create wake");
        const auto frozen = digest(scene.captureImage());
        scene.renderNow();
        require(digest(scene.captureImage()) == frozen, "paused motion geometry/animation changed without simulation time");
        require(scene.captureImage().save(evidence.filePath(id + QStringLiteral("_synthetic_motion.bmp")), "BMP"),
            "synthetic motion image");
        auto noContact = visualStates.back();
        auto airPosition = noContact.value(QStringLiteral("position_m")).toArray();
        airPosition[2] = 100.0;
        noContact.insert(QStringLiteral("position_m"), airPosition);
        scene.resetMotionHistory();
        require(scene.appendMotionStates({noContact}, &error), "no-contact input");
        require(scene.setState(noContact, parseForceState(noContact), &error), "no-contact display");
        require(scene.motionGeometry().wakeLines == 0
            && scene.graphicsStatus().contains(QStringLiteral("Keine verwertbare Wasserlinienkontur")),
            "no contact must suppress wake and diagnose");
        auto brokenMotion = noContact;
        brokenMotion.remove(QStringLiteral("moment_bezug_modell_m"));
        error.clear();
        require(!scene.appendMotionStates({brokenMotion}, &error) && !error.isEmpty(), "missing COM reference accepted");
        scene.invalidateMotionHistory(1, error);
        require(scene.setState(last, force, &error) && scene.motionGeometry().trackLines == 0,
            "motion data error must preserve valid model view");
        report.insert(id, QJsonObject{{QStringLiteral("states"), static_cast<int>(progress.states)},
            {QStringLiteral("modelFingerprint"), validation.fingerprint},
            {QStringLiteral("supports"), static_cast<int>(geometry.supports)},
            {QStringLiteral("trackLines"), geometry.trackLines},
            {QStringLiteral("wakeLines"), geometry.wakeLines},
            {QStringLiteral("capExercised"), capped},
            {QStringLiteral("replayInvariant"), true},
            {QStringLiteral("contactAndNoContact"), true},
            {QStringLiteral("syntheticMotionImageIsNotPhysicsEvidence"), true},
            {QStringLiteral("cameraAndPhysicsInvariant"), true}});
    }

    {
        vtkNew<vtkCubeSource> cube;
        cube->SetXLength(4);
        cube->SetYLength(2);
        cube->SetZLength(1);
        cube->Update();
        DisplayPart part;
        part.elementId = QStringLiteral("neutral-geometry");
        part.mesh = cube->GetOutput();
        VtkSceneWidget scene;
        scene.resize(500, 400);
        QString error;
        require(scene.setModel({part}, &error), "simple body motion fixture");
        auto profile = graphics::defaults(0);
        profile.markSpacing = 0.5;
        profile.enabled[static_cast<std::size_t>(graphics::Feature::Wake)] = true;
        scene.setGraphicsProfile(profile);
        const Vec3 reference{1, 0.2, 0.1};
        auto fixture = [&](double time, const std::array<double, 4>& rotation,
                           const Vec3& center, const Vec3& velocity) {
            const Pose pose{{}, rotation};
            const auto position = sub(center, toWorldVector(pose, reference));
            auto array = [](const auto& values) {
                QJsonArray result;
                for (double value : values) result.append(value);
                return result;
            };
            return QJsonObject{{QStringLiteral("vertragsversion"), QStringLiteral("2")},
                {QStringLiteral("position_m"), array(position)},
                {QStringLiteral("orientierung_xyzw"), array(rotation)},
                {QStringLiteral("moment_bezug_modell_m"), array(reference)},
                {QStringLiteral("geschwindigkeit_welt_m_s"), array(velocity)},
                {QStringLiteral("zeit_s"), time}};
        };
        const double half = std::sqrt(0.5);
        auto first = fixture(0, {0,0,0,1}, {}, {});
        auto rotated = fixture(1, {0,0,half,half}, {}, {});
        require(scene.appendMotionStates({first, rotated}, &error), "pure rotation history");
        require(scene.setState(rotated, parseForceState(rotated), &error), "pure rotation display");
        require(scene.motionHistory().points().size() == 1 && scene.motionGeometry().trackLines == 0
            && scene.motionGeometry().wakeLines == 0, "rotation around fixed COM generates track/wake");
        auto drift = fixture(2, {0,0,half,half}, {0,1,0}, {0,1,0});
        auto drift2 = fixture(3, {0,0,half,half}, {0,2,0}, {0,1,0});
        require(scene.appendMotionStates({drift, drift2}, &error), "simple-body contact history");
        require(scene.setState(drift2, parseForceState(drift2), &error), "simple-body contact display");
        require(scene.motionGeometry().trackLines > 0 && scene.motionGeometry().wakeLines > 0,
            "simple-body geometric fallback absent");
        require(scene.motionHistory().points().size() == 9 && scene.motionGeometry().trackLines == 12,
            "2-m path must have eight quarter-metre edges and four half-metre markers");
        const auto contact = scene.motionHistory().points().back().contact;
        require(contact && contact->leading[1] > contact->trailing[1], "contact ignores actual direction");
        auto reverse = fixture(4, {0,0,half,half}, {0,1,0}, {0,-1,0});
        require(scene.appendMotionStates({reverse}, &error), "reverse contact history");
        const auto reversed = scene.motionHistory().points().back().contact;
        require(reversed && reversed->leading[1] < reversed->trailing[1], "reverse contact remains at nominal bow");
        report.insert(QStringLiteral("simpleBodyFixedCOMRotationDriftAndReverse"), true);
        profile.markSpacing = 1e-300;
        scene.setGraphicsProfile(profile);
        require(scene.appendMotionStates({first, rotated, drift, drift2}, &error),
            "tiny mark spacing must not invalidate raw motion data");
        require(scene.setState(drift2, parseForceState(drift2), &error),
            "tiny mark spacing must retain model display");
        require(scene.motionGeometry().trackLines == 8
            && scene.graphicsStatus().contains(QStringLiteral("numerisch nicht darstellbar")),
            "nonrepresentable markers need explicit diagnostic, finite track retained");
        report.insert(QStringLiteral("unrepresentableMarksIsolatedWithDiagnostic"), true);
    }

    QCoreApplication::setOrganizationName(QStringLiteral("SegelPhysikTests"));
    QCoreApplication::setApplicationName(QStringLiteral("WaterMotionIntegration"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
        evidence.filePath(QStringLiteral("gui-test-settings")));
    QSettings().clear();
    bool unexpectedDialog = false;
    QTimer errors;
    QObject::connect(&errors, &QTimer::timeout, [&] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (auto* message = qobject_cast<QMessageBox*>(widget)) {
                std::cerr << message->text().toStdString() << '\n';
                unexpectedDialog = true;
                message->reject();
            }
        }
    });
    errors.start(50);
    MainWindow window;
    window.setModelFolder(models);
    window.selectInitialModel(QStringLiteral("420"));
    window.show();
    auto* mode = window.findChild<QComboBox*>(QStringLiteral("simulationMode"));
    auto* duration = window.findChild<QDoubleSpinBox*>(QStringLiteral("simulationDuration"));
    auto* active = window.findChild<QCheckBox*>(QStringLiteral("testDriveActive"));
    auto* force = window.findChild<QDoubleSpinBox*>(QStringLiteral("testDriveForce"));
    auto* speed = window.findChild<QDoubleSpinBox*>(QStringLiteral("testDriveSpeed"));
    QPushButton* run = nullptr;
    for (auto* button : window.findChildren<QPushButton*>()) {
        if (button->text() == QStringLiteral("Core-Lauf starten")) run = button;
    }
    auto* stop = window.findChild<QPushButton*>(QStringLiteral("stopSimulation"));
    auto* slider = window.findChild<QSlider*>();
    require(mode && duration && active && force && speed && run && stop && slider, "temporary drive controls absent");
    waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "420 UI load");
    require(!unexpectedDialog && !active->isChecked() && force->value() == 200
        && std::abs(speed->value() - 463.0 / 90.0) < 1e-8, "temporary defaults");
    mode->setCurrentIndex(1);
    duration->setValue(2.0);
    active->setChecked(true);
    waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "UI dynamic readiness");
    require(!unexpectedDialog, "UI dynamic initialization error");
    run->click();
    waitUntil([&] { return slider->maximum() >= 4 || unexpectedDialog; }, "UI live history");
    require(!unexpectedDialog && active->isEnabled() && !force->isEnabled() && !speed->isEnabled(),
        "running drive parameter locks/live flag");
    slider->setValue(0);
    active->setChecked(false);
    waitUntil([&] { return !stop->isEnabled() || unexpectedDialog; }, "UI live completion");
    require(!unexpectedDialog && force->isEnabled() && speed->isEnabled(), "UI drive completion");
    window.close();
    report.insert(QStringLiteral("temporaryUiDefaultsLocksLiveAndRewind"), true);
    QFile file(evidence.filePath(QStringLiteral("report.json")));
    require(file.open(QIODevice::WriteOnly), "motion evidence file");
    const auto bytes = QJsonDocument(report).toJson();
    require(file.write(bytes) == bytes.size(), "motion evidence write");
    std::cout << bytes.constData();
}
}

int main(int argc, char** argv)
{
    vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_WARNING);
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
    QApplication app(argc, argv);
    QElapsedTimer startup;
    startup.start();
    vtkNew<RenderErrors> renderErrors;
    vtkOutputWindow::SetInstance(renderErrors);
    try {
        if (app.arguments().size() == 4 && app.arguments()[1] == QStringLiteral("--water-motion-only")) {
            testWaterMotion(app.arguments()[2], app.arguments()[3]);
            require(renderErrors->count == 0, "VTK error in focused water-motion test");
            return 0;
        }
        if (app.arguments().size() == 4 && app.arguments()[1] == QStringLiteral("--navigation-only")) {
            testBodyFixedCamera(app.arguments()[2], app.arguments()[3]);
            require(renderErrors->count == 0, "VTK error in focused camera test");
            return 0;
        }
        require(app.arguments().size() == 3, "expected model folder and evidence folder");
        const auto catalog = segelphysik::modellkatalog(
            std::filesystem::path(app.arguments()[1].toStdWString()));
        const auto* entry = catalog.eintrag("folkeboot");
        require(entry != nullptr, "Folkeboot absent from catalog");
        const auto parts = toDisplayParts(
            segelphysik::darstellungsgeometrie(catalog, entry->modell_id, entry->modell_fassung));
        require(!parts.isEmpty(), "Folkeboot has no display geometry");
        QJsonObject materialCounts;
        for (const auto& semantic : {
                 std::pair{DisplayPart::Surface::Lacquer, "Lacquer"},
                 std::pair{DisplayPart::Surface::Cloth, "Cloth"},
                 std::pair{DisplayPart::Surface::Wood, "Wood"},
                 std::pair{DisplayPart::Surface::BrushedMetal, "BrushedMetal"}}) {
            const auto count = std::count_if(parts.cbegin(), parts.cend(),
                [&](const auto& part) { return part.surface == semantic.first; });
            require(count > 0, "Folkeboot missing required persistent material semantic");
            materialCounts.insert(QString::fromLatin1(semantic.second), static_cast<int>(count));
        }
        VtkSceneWidget scene;
        scene.resize(1280, 720);
        QString error;
        require(scene.setModel(parts, &error), "Folkeboot setModel failed");
        const double modelLoadMs = startup.nsecsElapsed() / 1e6;
        QHash<QString, QString> groups;
        for (const auto& part : parts) groups.insert(part.elementId, part.group);
        scene.setElementGroups(groups);
        const auto modelPath = QDir(app.arguments()[1]).filePath(QStringLiteral("folkeboot/modell.json"));
        const auto validation = validateModel(modelPath);
        require(validation.valid, "Folkeboot validation");
        std::atomic_bool cancellation{false};
        const auto simulation = runSimulation(modelPath, validation, true, 0.1, 0.01, &cancellation);
        require(simulation.completed && !simulation.states.isEmpty(), "Folkeboot reference simulation");
        const auto referenceState = simulation.states.front();
        const auto forceState = forcedisplay::parseForceState(referenceState);
        require(scene.setState(referenceState, forceState, &error), "reference state display");
        scene.show();
        QApplication::processEvents();
        scene.renderNow();
        const double firstFrameMs = startup.nsecsElapsed() / 1e6;
        QDir output(app.arguments()[2]);
        require(output.mkpath(QStringLiteral(".")), "evidence directory");
        QJsonObject report;
        report.insert(QStringLiteral("catalogModelSceneLoadMs"), modelLoadMs);
        report.insert(QStringLiteral("firstFrameStartupMs"), firstFrameMs);
        report.insert(QStringLiteral("startupMeasurement"),
            QStringLiteral("Wall time after QApplication; first-frame total includes reference Core simulation"));
        report.insert(QStringLiteral("model"), QStringLiteral("folkeboot"));
        report.insert(QStringLiteral("persistentMaterialCounts"), materialCounts);
        report.insert(QStringLiteral("persistentMaterialSource"),
            QStringLiteral("Core display geometry surface metadata, no previews or name inference"));
        report.insert(QStringLiteral("modelVersion"), QString::fromStdString(entry->modell_fassung));
        report.insert(QStringLiteral("width"), 1280);
        report.insert(QStringLiteral("height"), 720);
        report.insert(QStringLiteral("referenceSimulationStatus"), simulation.status);
        report.insert(QStringLiteral("referenceStates"), static_cast<int>(simulation.states.size()));
        report.insert(QStringLiteral("referenceCoreSeconds"), simulation.runtimeSeconds);
        report.insert(QStringLiteral("measurement"), QStringLiteral("VTK Render wall time; screenshot readback measured separately"));
        testPicking(scene, referenceState, forceState, report);
        testHelperOcclusion(referenceState, forceState, output, report);
        test420Dynamic(app.arguments()[1], catalog, output, report);
        QJsonArray profiles;
        QByteArray technical;
        for (int n = 0; n < 2; ++n) {
            const auto profile = graphics::defaults(n);
            scene.setGraphicsProfile(profile);
            VtkSceneWidget::DisplayOptions options;
            options.visibility = profile.throughBody ? VtkSceneWidget::Visibility::ThroughBody
                                                    : VtkSceneWidget::Visibility::Spatial;
            scene.setOptions(options);
            for (int warmup = 0; warmup < 3; ++warmup) {
                QApplication::processEvents();
                scene.renderNow();
                require(renderErrors->count == 0, "VTK errors during profile warmup");
            }
            std::vector<double> samples;
            QImage image;
            for (int frame = 0; frame < 20; ++frame) {
                QApplication::processEvents();
                samples.push_back(scene.renderNow());
            }
            QElapsedTimer capture;
            capture.start();
            image = scene.captureImage();
            require(renderErrors->count == 0, "VTK errors during profile rendering");
            const double captureMs = static_cast<double>(capture.nsecsElapsed()) / 1e6;
            require(!image.isNull(), "empty framebuffer");
            require(image.save(output.filePath(n == 0 ? QStringLiteral("technik.bmp") : QStringLiteral("optik.bmp")), "BMP"),
                "screenshot save");
            const auto hash = digest(image);
            if (n == 0) technical = hash;
            else require(hash != technical, "profiles render identically");
            std::sort(samples.begin(), samples.end());
            QJsonObject result;
            result.insert(QStringLiteral("profile"), n == 0 ? QStringLiteral("Technik") : QStringLiteral("Optik"));
            result.insert(QStringLiteral("medianRenderMs"), samples[samples.size() / 2]);
            result.insert(QStringLiteral("p95RenderMs"), samples[18]);
            result.insert(QStringLiteral("captureMs"), captureMs);
            result.insert(QStringLiteral("framebufferWidth"), image.width());
            result.insert(QStringLiteral("framebufferHeight"), image.height());
            result.insert(QStringLiteral("diagnostic"), scene.graphicsStatus());
            profiles.append(result);
        }
        QJsonArray toggles;
        const auto initialCamera = scene.cameraView();
        const VtkSceneWidget::CameraView sailCamera{{2, -7, 5}, {0, 0, 5}, {0, 0, 1}};
        require(scene.setCameraView(sailCamera), "sail artifact close-up camera");
        require(scene.captureImage().save(output.filePath(QStringLiteral("sail_helpers_closeup.bmp")), "BMP"),
            "sail artifact close-up screenshot");
        require(scene.setCameraView(initialCamera), "restore after sail artifact close-up");
        for (std::size_t feature = 0; feature < graphics::featureCount; ++feature) {
            std::cerr << "Feature off: " << graphics::features()[feature].key << std::endl;
            auto profile = graphics::defaults(1);
            profile.enabled[feature] = false;
            scene.setGraphicsProfile(profile);
            QApplication::processEvents();
            scene.renderNow();
            const auto image = scene.captureImage();
            const auto camera = scene.cameraView();
            require(camera.position == initialCamera.position && camera.focalPoint == initialCamera.focalPoint
                && camera.viewUp == initialCamera.viewUp, "feature switch changed camera");
            require(renderErrors->count == 0, "VTK errors during feature toggle");
            require(!image.isNull(), "feature-off render failed");
            require(image.save(output.filePath(QStringLiteral("off_%1.bmp")
                    .arg(QString::fromLatin1(graphics::features()[feature].key))), "BMP"),
                "feature-off screenshot save");
            QJsonObject toggle;
            toggle.insert(QStringLiteral("feature"), QString::fromLatin1(graphics::features()[feature].key));
            toggle.insert(QStringLiteral("diagnostic"), scene.graphicsStatus());
            toggles.append(toggle);
        }
        scene.setGraphicsProfile(graphics::defaults(1));
        scene.renderNow();
        const auto naturalMaterial = digest(scene.captureImage());
        std::vector<QByteArray> materialHashes{naturalMaterial};
        for (int style = 1; style <= 4; ++style) {
            for (const auto& part : parts) {
                if (part.surface != DisplayPart::Surface::Invisible)
                    require(scene.setMaterialPreview(part.elementId, style), "material preview target rejected");
            }
            scene.renderNow();
            require(renderErrors->count == 0, "VTK errors during material preview");
            const auto image = scene.captureImage();
            const auto material = digest(image);
            require(std::find(materialHashes.begin(), materialHashes.end(), material) == materialHashes.end(),
                "material preview duplicates natural model or earlier preview");
            materialHashes.push_back(material);
            require(image.save(output.filePath(QStringLiteral("material_%1.bmp").arg(style)), "BMP"),
                "material preview screenshot");
        }
        for (const auto& part : parts) {
            if (part.surface != DisplayPart::Surface::Invisible)
                require(scene.setMaterialPreview(part.elementId, 0), "natural material restore rejected");
        }
        scene.renderNow();
        require(digest(scene.captureImage()) == naturalMaterial, "preview reset did not restore actual surface materials");
        require(scene.captureImage().save(output.filePath(QStringLiteral("natural_materials_optik.bmp")), "BMP"),
            "natural material screenshot");
        report.insert(QStringLiteral("materialDemonstration"),
            QStringLiteral("Persistent semantic materials captured without preview; four explicit all-part previews reset exactly"));
        auto normalProfile = graphics::defaults(1);
        normalProfile.normalScale = 0.0;
        scene.setGraphicsProfile(normalProfile);
        scene.renderNow();
        const auto noNormals = digest(scene.captureImage());
        normalProfile.normalScale = 0.3;
        scene.setGraphicsProfile(normalProfile);
        scene.renderNow();
        require(digest(scene.captureImage()) != noNormals, "normal strength has no visible effect");
        const auto originalPeriod = digest(scene.captureImage());
        normalProfile.waterPeriod = 1.4;
        scene.setGraphicsProfile(normalProfile);
        scene.renderNow();
        require(digest(scene.captureImage()) != originalPeriod, "water world period has no visible effect");
        scene.setGraphicsProfile(graphics::defaults(1));
        report.insert(QStringLiteral("normalStrengthAndWaterPeriod"), true);
        for (int view = 0; view < 3; ++view) {
            auto camera = initialCamera;
            const auto focus = camera.focalPoint;
            camera.focalPoint[2] = view == 0 ? focus[2] : 3.0;
            camera.position = {focus[0] + (view == 0 ? 1.0 : 30.0), focus[1] + 1.0,
                view == 0 ? 20.0 : view == 1 ? 0.3 : -8.0};
            if (view == 0) camera.viewUp = {0.0, 1.0, 0.0};
            require(scene.setCameraView(camera), "comparison camera rejected");
            scene.renderNow();
            require(scene.captureImage().save(output.filePath(QStringLiteral("view_%1.bmp").arg(view)), "BMP"),
                "comparison camera image");
        }
        require(scene.setCameraView(initialCamera), "restore comparison camera");
        const auto firstImage = digest(scene.captureImage());
        require(scene.setState(simulation.states.back(),
            forcedisplay::parseForceState(simulation.states.back()), &error), "animation last state");
        scene.renderNow();
        require(simulation.states.back().value(QStringLiteral("zeit_s")).toDouble()
                > referenceState.value(QStringLiteral("zeit_s")).toDouble(),
            "animation fixture must advance actual Core time");
        require(digest(scene.captureImage()) != firstImage, "advanced Core state renders identically");
        require(scene.setState(referenceState, forceState, &error), "animation replay first state");
        scene.renderNow();
        require(digest(scene.captureImage()) == firstImage, "same simulation time not reproducible");
        const auto frozen = scene.captureImage();
        scene.renderNow();
        require(digest(scene.captureImage()) == digest(frozen), "paused water animation changed");
        report.insert(QStringLiteral("cameraCases"), 3);
        report.insert(QStringLiteral("animationReplay"), true);
        auto rolledStart = referenceState;
        const double halfRoll = 15.0 * std::acos(-1.0) / 360.0;
        rolledStart.insert(QStringLiteral("orientierung_xyzw"),
            QJsonArray{std::sin(halfRoll), 0.0, 0.0, std::cos(halfRoll)});
        const auto rolled = runSimulation(modelPath, validation, true, 0.1, 0.01,
            &cancellation, rolledStart);
        require(rolled.completed && rolled.states.size() > 1, "Core heeled-start simulation failed");
        const auto& rolledState = rolled.states.back();
        const auto orientation = rolledState.value(QStringLiteral("orientierung_xyzw")).toArray();
        require(orientation.size() == 4 && std::abs(orientation.at(0).toDouble()) > 0.05,
            "Core heeled state lacks roll");
        require(scene.setState(rolledState, forcedisplay::parseForceState(rolledState), &error),
            "Core heeled state display");
        scene.renderNow();
        require(scene.captureImage().save(output.filePath(QStringLiteral("krangung.bmp")), "BMP"),
            "Core heeled screenshot");
        report.insert(QStringLiteral("heeledCoreState"), rolledState);
        report.insert(QStringLiteral("heeledInitialRollDegrees"), 15);
        require(scene.setModel(parts, &error), "static Folkeboot reload");
        scene.setGraphicsProfile(graphics::defaults(1));
        require(scene.setCameraView(initialCamera), "static preview camera");
        scene.renderNow();
        const auto staticPreview = digest(scene.captureImage());
        scene.renderNow();
        require(digest(scene.captureImage()) == staticPreview, "no-state water preview is not static");
        require(scene.captureImage().save(output.filePath(QStringLiteral("static_preview.bmp")), "BMP"),
            "static preview screenshot");
        const auto* secondEntry = catalog.eintrag("quader");
        require(secondEntry != nullptr, "quader absent from catalog");
        const auto secondParts = toDisplayParts(segelphysik::darstellungsgeometrie(
            catalog, secondEntry->modell_id, secondEntry->modell_fassung));
        require(scene.setModel(secondParts, &error), "second model switch");
        scene.renderNow();
        require(!scene.captureImage().isNull(), "second model framebuffer absent");
        require(scene.setModel(parts, &error), "Folkeboot switch restore");
        scene.setElementGroups(groups);
        require(scene.setState(referenceState, forceState, &error), "reference state restore after model switch");
        require(scene.setCameraView(initialCamera), "model switch camera restore");
        scene.renderNow();
        require(digest(scene.captureImage()) == firstImage, "model reload retained stale material or render state");
        require(renderErrors->count == 0, "VTK errors in heeled/static/model-switch cases");
        report.insert(QStringLiteral("staticNoStatePreview"), true);
        report.insert(QStringLiteral("modelSwitch"), QStringLiteral("folkeboot -> quader -> folkeboot; exact image restored"));
        require(output.mkpath(QStringLiteral("hdr-fixtures")), "HDR fixture directory");
        QDir assets(output.filePath(QStringLiteral("hdr-fixtures")));
        auto environment = graphics::defaults(1);
        environment.environmentPath = assets.filePath(QStringLiteral("missing.hdr"));
        scene.setGraphicsProfile(environment);
        scene.renderNow();
        require(scene.graphicsStatus().contains(
            QStringLiteral("HDR-Umgebung nicht lesbar oder ungültig: ") + environment.environmentPath),
            "missing HDR must be diagnosed explicitly");
        require(!scene.captureImage().isNull(), "missing HDR fallback render");
        environment.environmentPath = assets.filePath(QStringLiteral("test.hdr"));
        QFile hdr(environment.environmentPath);
        require(hdr.open(QIODevice::WriteOnly), "temporary HDR create");
        const QByteArray header("#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 2 +X 4\n");
        require(hdr.write(header) == header.size(), "HDR header write");
        const QByteArray pixel = QByteArray::fromHex("80a0c081");
        for (int i = 0; i < 8; ++i) require(hdr.write(pixel) == pixel.size(), "HDR pixel write");
        hdr.close();
        scene.setGraphicsProfile(environment);
        scene.renderNow();
        require(!scene.captureImage().isNull(), "valid HDR render");
        require(!scene.graphicsStatus().contains(QStringLiteral("HDR-Umgebung nicht lesbar oder ungültig:")),
            "valid HDR not accepted");
        require(!scene.graphicsStatus().contains(QStringLiteral("HDR-Umgebung konnte nicht geladen werden:")),
            "valid HDR reader failed");
        require(renderErrors->count == 0, "VTK errors during HDR rendering");
        report.insert(QStringLiteral("hdrCases"), 2);
        const auto liveReference = runSimulation(modelPath, validation, true, 0.2, 0.01, &cancellation);
        require(liveReference.completed, "live comparison reference");
        QJsonArray liveCases;
        for (int mode = -1; mode < 2; ++mode) {
            auto live = LiveSimulation::create(modelPath, validation, 0.2, 0.01, true, &error);
            require(live != nullptr, "Folkeboot live creation");
            if (mode >= 0) scene.setGraphicsProfile(graphics::defaults(mode));
            auto worker = std::async(std::launch::async, [&] { live->run(cancellation); });
            int frames = 0;
            while (!live->progress().finished) {
                if (mode >= 0) {
                    const auto progress = live->progress();
                    if (progress.states > 0) {
                        const auto state = live->stateJson(progress.states - 1, &error);
                        require(scene.setState(state, forcedisplay::parseForceState(state), &error),
                            "live state display");
                        scene.renderNow();
                        ++frames;
                    }
                    QApplication::processEvents();
                } else {
                    worker.wait_for(std::chrono::milliseconds(1));
                }
            }
            worker.get();
            const auto progress = live->progress();
            require(progress.status == QStringLiteral("abgeschlossen")
                && progress.states == liveReference.states.size(), "live lost states or status");
            const auto finalState = live->stateJson(progress.states - 1, &error);
            for (const char* field : {"position_m", "orientierung_xyzw", "vektoren"}) {
                const QString key = QString::fromLatin1(field);
                require(finalState.contains(key), "live reference field absent");
                require(finalState.value(key) == liveReference.states.back().value(key),
                    "graphics changed simulation result");
            }
            QJsonObject result;
            result.insert(QStringLiteral("profile"), mode < 0 ? QStringLiteral("No display")
                : mode == 0 ? QStringLiteral("Technik") : QStringLiteral("Optik"));
            result.insert(QStringLiteral("coreSeconds"), progress.coreSeconds);
            result.insert(QStringLiteral("wallSeconds"), progress.wallSeconds);
            result.insert(QStringLiteral("states"), static_cast<int>(progress.states));
            result.insert(QStringLiteral("frames"), frames);
            liveCases.append(result);
        }
        require(renderErrors->count == 0, "VTK errors during live comparison");
        report.insert(QStringLiteral("liveComparison"), liveCases);
        auto none = graphics::defaults(0);
        none.enabled.fill(false);
        scene.setGraphicsProfile(none);
        QApplication::processEvents();
        scene.renderNow();
        require(!scene.captureImage().isNull(), "all-off render failed");
        require(renderErrors->count == 0, "VTK errors during all-off rendering");
        report.insert(QStringLiteral("profiles"), profiles);
        report.insert(QStringLiteral("featureCases"), toggles);
        report.insert(QStringLiteral("materialPreviewCases"), 4);
        report.insert(QStringLiteral("featureOffCases"), static_cast<int>(graphics::featureCount));
        scene.hide();
        testMainWindow(app.arguments()[1], output, report);
        require(renderErrors->count == 0, "VTK errors in MainWindow integration");
        QFile file(output.filePath(QStringLiteral("report.json")));
        require(file.open(QIODevice::WriteOnly), "report open");
        require(file.write(QJsonDocument(report).toJson()) > 0, "report write");
        std::cout << "Folkeboot profiles, " << graphics::featureCount
                  << " feature-off cases and all-off rendered; evidence saved.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
