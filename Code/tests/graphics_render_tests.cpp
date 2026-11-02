#include "display_geometry.hpp"
#include "core_adapter.hpp"
#include "graphics_settings.hpp"
#include "vtk_scene.hpp"
#include "main_window.hpp"
#include "navigation.hpp"
#include "boat_inspection.hpp"

#include <segelphysik/katalog.hpp>
#include <segelphysik/modell.hpp>
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
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
#include <QScrollBar>
#include <QSlider>
#include <QSizeGrip>
#include <QTimer>
#include <QTabBar>
#include <QTemporaryDir>
#include <QThread>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkNew.h>
#include <vtkLogger.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkCellArray.h>
#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkPropCollection.h>
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
#include <limits>
#include <stdexcept>
#include <thread>

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
qsizetype differingPixels(const QImage& first, const QImage& second)
{
    if (first.size() != second.size()) return std::numeric_limits<qsizetype>::max();
    const auto a = first.convertToFormat(QImage::Format_ARGB32);
    const auto b = second.convertToFormat(QImage::Format_ARGB32);
    qsizetype count = 0;
    for (int y = 0; y < a.height(); ++y) {
        const auto* rowA = reinterpret_cast<const QRgb*>(a.constScanLine(y));
        const auto* rowB = reinterpret_cast<const QRgb*>(b.constScanLine(y));
        for (int x = 0; x < a.width(); ++x) {
            if (rowA[x] != rowB[x]) ++count;
        }
    }
    return count;
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
    state.remove(QStringLiteral("elementtransformationen_modell"));
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
    const bool occlusionStateOk = scene.setState(state, forces, &error);
    if (!occlusionStateOk) std::cerr << error.toStdString() << '\n';
    require(occlusionStateOk, "helper occlusion fixture state");
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
    auto movementLive = LiveSimulation::create(modelPath, validation, 0.3, 0.01, true, &error);
    require(movementLive != nullptr, "420 movement simulation creation");
    std::atomic_bool movementCancellation{false};
    std::thread movementWorker([&] { movementLive->run(movementCancellation); });
    const auto movementDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (movementLive->progress().states < 2 && !movementLive->progress().finished
        && std::chrono::steady_clock::now() < movementDeadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    bool rudderQueued = false;
    bool swordQueued = false;
    if (!movementLive->progress().finished) {
        rudderQueued = movementLive->setMovementValue(QStringLiteral("Ruder"), 45.0, &error);
        swordQueued = movementLive->setMovementValue(QStringLiteral("Schwert"), 0.0, &error);
    }
    movementWorker.join();
    require(rudderQueued && swordQueued, "420 live movement events were not accepted");
    const auto movementProgress = movementLive->progress();
    require(movementProgress.status == QStringLiteral("abgeschlossen"),
        "420 movement simulation did not complete");
    const auto initialMovementState = movementLive->stateJson(0, &error);
    const auto movedState = movementLive->stateJson(movementProgress.states - 1, &error);
    require(movedState.value(QStringLiteral("letzte_bewegungsereigniskennung")).toDouble() == 2.0,
        "420 movement event acknowledgements are missing");
    bool rudderMoved = false;
    bool swordMoved = false;
    bool rudderAssemblyMoved = false;
    QJsonObject movedMatrices;
    for (const auto& value : movedState.value(QStringLiteral("elementtransformationen_modell")).toArray()) {
        const auto matrix = value.toObject();
        const auto id = matrix.value(QStringLiteral("element_id")).toString();
        movedMatrices.insert(id, matrix.value(QStringLiteral("matrix_zeilenweise")));
    }
    for (const auto& value : movedState.value(QStringLiteral("bewegungen")).toArray()) {
        const auto movement = value.toObject();
        const auto id = movement.value(QStringLiteral("element_id")).toString();
        const double setting = movement.value(QStringLiteral("wert")).toDouble();
        rudderMoved = rudderMoved || (id == QStringLiteral("Ruder") && setting == 45.0);
        swordMoved = swordMoved || (id == QStringLiteral("Schwert") && setting == 0.0);
    }
    rudderAssemblyMoved = !movedMatrices.value(QStringLiteral("Ruder")).isUndefined()
        && movedMatrices.value(QStringLiteral("Ruder"))
            == movedMatrices.value(QStringLiteral("Ruderkopf"));
    require(rudderMoved && swordMoved && rudderAssemblyMoved,
        "Core-confirmed rudder, sword or rudder-assembly pose is missing");
    auto renderedMovementState = initialMovementState;
    renderedMovementState.insert(QStringLiteral("elementtransformationen_modell"),
        movedState.value(QStringLiteral("elementtransformationen_modell")));
    renderedMovementState.insert(QStringLiteral("bewegungen"),
        movedState.value(QStringLiteral("bewegungen")));
    scene.setGraphicsProfile(graphics::defaults(0));
    require(scene.setState(initialMovementState,
        forcedisplay::parseForceState(initialMovementState), &error),
        "420 initial movement pose display");
    scene.renderNow();
    const auto initialMovementImage = digest(scene.captureImage());
    require(scene.setState(renderedMovementState,
        forcedisplay::parseForceState(renderedMovementState), &error),
        "420 Core-confirmed movement pose display");
    scene.renderNow();
    require(digest(scene.captureImage()) != initialMovementImage,
        "Core-confirmed movement poses did not change the complete rendered model");
    report.insert(QStringLiteral("420LiveMovementPosesAndRudderAssembly"), true);
    report.insert(QStringLiteral("420DynamicStates"), static_cast<int>(progress.states));
    report.insert(QStringLiteral("420CourseTrailDefaultsOffAndOptional"), true);
    report.insert(QStringLiteral("420NavigationSelectedSnapshotBothProfiles"), true);
}

void testWorkspaceTabs(MainWindow& window, VtkSceneWidget& scene,
    const QDir& evidence, QJsonObject& report)
{
    auto* tabs = window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
    require(tabs && tabs->count() == 3 && tabs->currentIndex() == 0,
        "three workspace tabs, initially boat selection");
    const QStringList labels{QStringLiteral("Bootsauswahl"), QStringLiteral("Statisches Modell"),
        QStringLiteral("Dynamisches Modell")};
    const std::array<QColor, 3> selectedColors{QColor("#85334b"), QColor("#244e7a"), QColor("#256344")};
    const std::array<QColor, 3> inactiveColors{QColor("#2b1923"), QColor("#111e2c"), QColor("#142a20")};
    const std::array<QColor, 3> backgrounds{QColor("#1c1118"), QColor("#0b1420"), QColor("#0d1b16")};
    auto* central = window.centralWidget();
    const auto camera = scene.cameraView();
    const auto mode = window.findChild<QComboBox*>(QStringLiteral("simulationMode"))->currentIndex();
    const auto duration = window.findChild<QDoubleSpinBox*>(
        QStringLiteral("simulationDuration"))->value();
    auto* models = window.findChild<QListWidget*>(QStringLiteral("modelList"));
    require(models != nullptr, "model list for workspace identity check");
    const auto selectedModel = models->currentItem()->data(Qt::UserRole).toString();
    for (int selected = 0; selected < 3; ++selected) {
        tabs->setCurrentIndex(selected);
        QApplication::processEvents();
        require(window.centralWidget() == central,
            "workspace switch must reuse the central widget");
        auto* inspectionTree = window.findChild<QTreeWidget*>(QStringLiteral("inspectionObjectTree"));
        require(inspectionTree && inspectionTree->isVisible() == (selected == 0),
            "only boat selection exposes object inspection");
        QPushButton* folder = nullptr;
        for (auto* button : window.findChildren<QPushButton*>())
            if (button->text() == QStringLiteral("Modellordner wählen")) folder = button;
        require(folder && folder->isVisible() == (selected == 0),
            "folder selection exists only in boat selection");
        require(window.findChild<QComboBox*>(QStringLiteral("simulationMode"))->currentIndex() == mode
            && window.findChild<QDoubleSpinBox*>(QStringLiteral("simulationDuration"))->value() == duration
            && models->currentItem()
            && models->currentItem()->data(Qt::UserRole).toString() == selectedModel,
            "workspace switch changed model or simulation inputs");
        const auto currentCamera = scene.cameraView();
        require(camera.position == currentCamera.position && camera.focalPoint == currentCamera.focalPoint
            && camera.viewUp == currentCamera.viewUp, "workspace switch changed camera");
        require(central->palette().color(QPalette::Window) == backgrounds[selected],
            "entire workspace must use selected theme");
        const auto image = tabs->grab().toImage();
        for (int tab = 0; tab < 3; ++tab) {
            require(tabs->tabText(tab) == labels[tab], "workspace tab label");
            const auto rect = tabs->tabRect(tab);
            const auto sample = QPointF(rect.left() + 10, rect.center().y()) * image.devicePixelRatio();
            require(image.pixelColor(sample.toPoint()) ==
                (selected == tab ? selectedColors[tab] : inactiveColors[tab]),
                "each tab must keep its own color, including inactive tabs");
        }
        require(window.grab().save(evidence.filePath(
            QStringLiteral("workspace_%1.bmp").arg(selected)), "BMP"), "workspace theme screenshot");
    }
    tabs->setCurrentIndex(1);
    report.insert(QStringLiteral("workspaceTabsIsolatedInspectionAndFixedInactiveColors"), true);
}

void testBoatInspection(const QString& models, const QDir& evidence, QJsonObject& report)
{
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary inspection fixture folder");
    const auto catalog = segelphysik::modellkatalog(std::filesystem::path(models.toStdWString()));
    const auto* entry = catalog.eintrag("folkeboot");
    require(entry, "Folkeboot inspection fixture");
    const QDir copy(QDir(temporary.path()).filePath(QStringLiteral("v2")));
    std::filesystem::copy(
        segelphysik::katalog_datei(catalog, entry->modell_id, entry->modell_fassung).parent_path(),
        std::filesystem::path(copy.absolutePath().toStdWString()),
        std::filesystem::copy_options::recursive);
    QCoreApplication::setOrganizationName(QStringLiteral("SegelPhysikTests"));
    QCoreApplication::setApplicationName(QStringLiteral("BoatInspection"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
    QSettings().clear();
    MainWindow window;
    window.setModelFolder(copy.absolutePath());
    window.selectInitialModel(QStringLiteral("folkeboot"));
    window.show();
    auto* tree = window.findChild<QTreeWidget*>(QStringLiteral("inspectionObjectTree"));
    auto* values = window.findChild<QTreeWidget*>(QStringLiteral("inspectionValues"));
    auto* tabs = window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
    require(tree && values && tabs, "inspection controls exist");
    require(!window.findChild<QTabBar*>(QStringLiteral("inspectionTreeTypes")),
        "one object tree replaces domain tabs");
    require(values->columnCount() == 6 && values->headerItem()->text(1) == QStringLiteral("Bereich"),
        "complete property table includes domain area");
    require(!window.findChild<QComboBox*>(QStringLiteral("inspectionMeshTypes")),
        "geometry selection must use right object tree, not a separate mesh combo");
    waitUntil([&] { return tree->topLevelItemCount() == 1; },
        "inspection snapshot load timeout");
    auto* inspectionScene = dynamic_cast<VtkSceneWidget*>(
        window.findChild<QWidget*>(QStringLiteral("inspectionScene")));
    auto* simulationScene = dynamic_cast<VtkSceneWidget*>(
        window.findChild<QWidget*>(QStringLiteral("simulationScene")));
    require(inspectionScene && simulationScene, "separate views preserve simulation scene");
    inspectionScene->renderNow();
    auto* viewport = inspectionScene->findChild<QVTKOpenGLNativeWidget*>();
    require(viewport && !viewport->mask().isEmpty()
        && !viewport->mask().contains(QPoint(0, 0))
        && viewport->mask().contains(viewport->rect().center()),
        "inspection render content is clipped to rounded corners");
    auto* renderer = viewport->renderWindow()->GetRenderers()->GetFirstRenderer();
    auto* props = renderer->GetViewProps();
    props->InitTraversal();
    bool modelAxes = false, spatialGrid = false;
    int meterLabels = 0;
    int verticalMeterLabels = 0;
    while (auto* prop = props->GetNextProp()) {
        if (auto* label = vtkBillboardTextActor3D::SafeDownCast(prop)) {
            const auto caption = QString::fromUtf8(label->GetInput());
            if (caption.endsWith(QStringLiteral(" m"))) {
                bool valid = false;
                const double coordinate = caption.chopped(2).toDouble(&valid);
                require(valid && coordinate == std::round(coordinate),
                    "grid captions show actual whole-meter coordinates");
                const double* position = label->GetPosition();
                require(std::abs(position[0]-coordinate) < 1e-10
                    || std::abs(position[1]-coordinate) < 1e-10
                    || std::abs(position[2]-coordinate) < 1e-10,
                    "meter label is attached to its actual X, Y or Z coordinate");
                if (std::abs(position[2]-coordinate) < 1e-10) ++verticalMeterLabels;
                ++meterLabels;
            }
        }
        if (auto* axes = vtkAxesActor::SafeDownCast(prop))
            modelAxes = axes->GetVisibility() && axes->GetPosition()[0] == 0.0
                && axes->GetPosition()[1] == 0.0 && axes->GetPosition()[2] == 0.0;
        auto* actor = vtkActor::SafeDownCast(prop);
        if (!actor || !actor->GetVisibility() || actor->GetUseBounds()) continue;
        auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
        if (!mapper || !mapper->GetInput()) continue;
        auto* mesh = mapper->GetInput();
        require(mesh->GetNumberOfPolys() == 0,
            "inspection has no visible water or depth surface");
        if (mesh->GetNumberOfLines() > 0) {
            double bounds[6];
            mesh->GetBounds(bounds);
            spatialGrid = spatialGrid || (bounds[0] < bounds[1] && bounds[2] < bounds[3]
                && bounds[4] < bounds[5]);
            if (bounds[0] < bounds[1] && bounds[2] < bounds[3] && bounds[4] < bounds[5]) {
                require(mesh->GetNumberOfVerts() > 0, "half-meter grid lines use point topology");
                auto* lines = mesh->GetLines();
                lines->InitTraversal();
                vtkIdType count = 0;
                const vtkIdType* ids = nullptr;
                while (lines->GetNextCell(count, ids)) {
                    require(count == 2, "solid reference lines have two endpoints");
                    double a[3], b[3];
                    mesh->GetPoint(ids[0], a);
                    mesh->GetPoint(ids[1], b);
                    bool integerCoordinate = false;
                    for (int axis = 0; axis < 3; ++axis)
                        if (a[axis] == b[axis] && std::abs(a[axis] - std::round(a[axis])) < 1e-10)
                            integerCoordinate = true;
                    require(integerCoordinate, "solid grid lines lie at whole-meter coordinates");
                }
            }
        }
    }
    require(modelAxes, "model-origin axes are visible");
    require(spatialGrid, "three-dimensional grid is visible");
    require(inspectionScene->property("inspectionGridLabelCount").toInt() >= 2,
        "lower X/Y grid axes carry meter labels");
    require(meterLabels == inspectionScene->property("inspectionGridLabelCount").toInt(),
        "meter labels are real rendered actors without duplicates");
    require(verticalMeterLabels > 1, "left vertical grid edge has multiple actual Z meter labels");
    require(!window.findChild<QLabel*>(QStringLiteral("inspectionGeometryLegend")),
        "source-color legend is removed");
    require(tree->isHeaderHidden(), "object caption is outside the inner tree");
    auto* overlay = inspectionScene->findChild<QWidget*>(QStringLiteral("navigationOverlay"));
    require(overlay && !overlay->isVisible(), "boat selection has no navigation instruments");
    QPushButton* folderButton = nullptr;
    for (auto* button : window.findChildren<QPushButton*>())
        if (button->text() == QStringLiteral("Modellordner wählen")) folderButton = button;
    require(folderButton && folderButton->isVisible(), "folder button in boat selection");
    const auto catalogRect = folderButton->parentWidget()->rect();
    const auto catalogTop = folderButton->parentWidget()->mapToGlobal(catalogRect.topLeft()).y();
    const auto catalogBottom = catalogTop + catalogRect.height();
    const auto viewportTop = inspectionScene->mapToGlobal(QPoint()).y();
    require(std::abs(catalogTop - viewportTop) <= 1
        && std::abs(catalogBottom - viewportTop - inspectionScene->height()) <= 1,
        "catalog column and actual inspection viewport must have matching bounds within 1 logical pixel");
    const auto* objectPanel = tree->parentWidget();
    require(std::abs(objectPanel->mapToGlobal(QPoint()).y() - viewportTop) <= 1
        && objectPanel->height() == folderButton->parentWidget()->height(),
        "both outer side panels align with the rendering viewport");
    const auto leftMargins = folderButton->parentWidget()->layout()->contentsMargins();
    require(objectPanel->layout()->contentsMargins() == leftMargins,
        "side panels share outer-to-inner frame padding");
    bool externalCaption = false;
    for (auto* label : objectPanel->findChildren<QLabel*>())
        if (label->text() == QStringLiteral("Objekt / Teil")) {
            externalCaption = label->mapToGlobal(QPoint()).y() + label->height()
                <= tree->mapToGlobal(QPoint()).y();
        }
    require(externalCaption, "object caption is displayed above the inner framed list");
    auto* title = window.findChild<QLabel*>(QStringLiteral("title"));
    require(title && title->mapToGlobal(QPoint()).y() < tabs->mapToGlobal(QPoint()).y(),
        "title above workspace tabs");
    auto* root = tree->topLevelItem(0);
    root->setSelected(true);
    QApplication::processEvents();
    require(values->topLevelItemCount() > 0, "root selection includes descendant properties");
    const int rootValues = values->topLevelItemCount();
    if (root->childCount()) {
        root->child(0)->setSelected(true);
        QApplication::processEvents();
        require(values->topLevelItemCount() == rootValues, "overlapping selection deduplicates properties");
    }
    QSet<QString> propertyKeys;
    QSet<QString> propertyAreas;
    for (int i = 0; i < values->topLevelItemCount(); ++i) {
        auto* row = values->topLevelItem(i);
        require(!row->text(1).isEmpty(), "every property has a Core-supplied domain area");
        propertyAreas.insert(row->text(1));
        const auto key = row->text(0) + QLatin1Char('|') + row->text(1) + QLatin1Char('|') + row->text(2);
        require(!propertyKeys.contains(key), "property table has no duplicate rows");
        propertyKeys.insert(key);
    }
    require(propertyAreas.size() >= 4, "one selection includes multiple physical property areas");
    require(values->header()->sectionResizeMode(5) == QHeaderView::Interactive
        && !values->header()->stretchLastSection(), "all six columns are manually resizable");
    const auto originalWidth = values->columnWidth(5);
    values->setColumnWidth(5, originalWidth+35);
    require(values->columnWidth(5) == originalWidth+35, "last column width actually changes");
    const auto checkFilterAlignment = [&] {
        for (int i = 0; i < values->columnCount(); ++i) {
            auto* filter = window.findChild<QLineEdit*>(QStringLiteral("inspectionFilter%1").arg(i));
            require(filter && filter->placeholderText() == QStringLiteral("*")
                && filter->text().isEmpty(), "empty filters display only a star placeholder");
            require(filter->width() == values->columnWidth(i)
                && filter->x() == values->header()->sectionViewportPosition(i),
                "each filter follows its exact column width and scrolled position");
        }
    };
    checkFilterAlignment();
    auto* headerViewport = values->header()->viewport();
    const int firstWidth = values->columnWidth(0);
    const QPointF boundary(firstWidth-1, 10);
    const QPointF dragged = boundary + QPointF(57,0);
    const QPointF global = headerViewport->mapToGlobal(boundary.toPoint());
    QMouseEvent press(QEvent::MouseButtonPress, boundary, global,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(headerViewport, &press);
    QMouseEvent move(QEvent::MouseMove, dragged, global+QPointF(57,0),
        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(headerViewport, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, dragged, global+QPointF(57,0),
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(headerViewport, &release);
    QApplication::processEvents();
    require(values->columnWidth(0) == firstWidth+57,
        "dragging the actual header boundary changes the column width");
    checkFilterAlignment();
    values->horizontalScrollBar()->setValue(values->horizontalScrollBar()->maximum());
    QApplication::processEvents();
    checkFilterAlignment();
    values->horizontalScrollBar()->setValue(0);
    auto* areaFilter = window.findChild<QLineEdit*>(QStringLiteral("inspectionFilter1"));
    auto* nameFilter = window.findChild<QLineEdit*>(QStringLiteral("inspectionFilter2"));
    require(areaFilter && nameFilter, "per-column filters exist");
    areaFilter->setText(QStringLiteral("Bewegung"));
    int visible = 0;
    for (int i = 0; i < values->topLevelItemCount(); ++i)
        if (!values->topLevelItem(i)->isHidden()) {
            ++visible;
            require(values->topLevelItem(i)->text(1) == QStringLiteral("Bewegung"),
                "area filtering hides other domains");
        }
    require(visible > 0 && visible < rootValues, "filter changes actual visible rows");
    nameFilter->setText(QStringLiteral("no-such-property"));
    for (int i = 0; i < values->topLevelItemCount(); ++i)
        require(values->topLevelItem(i)->isHidden(), "column filters combine with AND");
    areaFilter->clear(); nameFilter->clear();
    values->sortItems(3, Qt::AscendingOrder);
    double previous = -std::numeric_limits<double>::infinity();
    bool colorFound = false, pivotFound = false;
    QTreeWidgetItem* pivotRow = nullptr;
    for (int i = 0; i < values->topLevelItemCount(); ++i) {
        auto* row = values->topLevelItem(i);
        const auto numeric = row->data(3, Qt::UserRole+10);
        if (numeric.isValid()) {
            require(numeric.toDouble() >= previous, "value column sorts numerically ascending");
            previous = numeric.toDouble();
            require(row->toolTip(3).size() > 0, "numeric display retains full original tooltip");
            const auto mantissa = row->text(3).section(QLatin1Char('e'),0,0);
            require(!mantissa.contains(QLatin1Char('.'))
                || mantissa.section(QLatin1Char('.'),1).size() <= 2,
                "displayed scalar numbers have at most two decimal places");
        }
        colorFound = colorFound || row->data(3,Qt::UserRole+11).value<QColor>().isValid();
        if (row->data(0,Qt::UserRole+13).toString() == QStringLiteral("pivot")) pivotRow = row;
    }
    require(colorFound, "color property is painted as a rounded swatch");
    values->sortItems(3, Qt::DescendingOrder);
    previous = std::numeric_limits<double>::infinity();
    for (int i = 0; i < values->topLevelItemCount(); ++i) {
        const auto numeric = values->topLevelItem(i)->data(3,Qt::UserRole+10);
        if (numeric.isValid()) {
            require(numeric.toDouble() <= previous, "value column sorts numerically descending");
            previous = numeric.toDouble();
        }
    }
    if (pivotRow) {
        pivotRow->setSelected(true);
        pivotFound = inspectionScene->property("inspectionPivotCount").toInt() == 1;
        require(inspectionScene->property("inspectionMovementPoseCount").toInt() == 0,
            "pivot selection shows point and axis rather than every pose");
        values->clearSelection();
    }
    require(pivotFound, "selecting actual pivot property displays model-fixed pivot");
    const auto checkMovementBounds = [&](bool expectBounds) {
      const auto tableCamera = inspectionScene->cameraView();
      int extremes = 0;
      for (int i = 0; i < values->topLevelItemCount(); ++i) {
        auto* row = values->topLevelItem(i);
        const auto action = row->data(0,Qt::UserRole+13).toString();
        if (action != QStringLiteral("minimum") && action != QStringLiteral("maximum")) {
            if (row->text(2).startsWith(QStringLiteral("Vorschau ")))
                require(row->text(5) == QStringLiteral("nicht_vorhanden"),
                    "undefined bounds remain explicit and have no clickable preview action");
            continue;
        }
        row->setSelected(true);
        require(inspectionScene->property("inspectionMovementPoseCount").toInt() == 1,
            "selecting an available movement bound shows exactly its Core-transformed pose");
        const auto camera = inspectionScene->cameraView();
        require(camera.position == tableCamera.position && camera.focalPoint == tableCamera.focalPoint,
            "table-selected previews preserve camera");
        nameFilter->setText(QStringLiteral("no-such-property"));
        require(inspectionScene->property("inspectionMovementPoseCount").toInt() == 0,
            "filtering away selected movement bound removes its preview");
        nameFilter->clear();
        values->clearSelection();
        ++extremes;
      }
      require(expectBounds ? extremes >= 2 : extremes == 0,
          "only genuinely defined lower and upper movement bounds are selectable");
    };
    checkMovementBounds(false);
    values->sortItems(0, Qt::AscendingOrder);
    areaFilter->setText(QStringLiteral("Darstellung"));
    values->scrollToTop();
    QApplication::processEvents();
    require(window.grab().save(evidence.filePath(QStringLiteral("boat_property_colors.bmp")), "BMP"),
        "rounded color property screenshot");
    areaFilter->clear();
    tree->clearSelection();
    require(values->topLevelItemCount() == 0, "empty selection clears table");
    QString focusedElement;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        const auto elements = (*it)->data(0, Qt::UserRole + 1).toStringList();
        if (elements.size() == 1
            && inspectionScene->inspectionOpacity(elements.front()) > 0.0) {
            (*it)->setSelected(true);
            focusedElement = elements.front();
            break;
        }
    }
    require(!focusedElement.isEmpty(), "selectable displayed part exists");
    require(inspectionScene->inspectionOpacity(focusedElement) == 1.0,
        "actual tree element selection is opaque");
    tree->clearSelection();
    const auto inspectionCamera = inspectionScene->cameraView();
    int geometries = 0;
    QSet<QString> sourceIds;
    QSet<QString> elementIds;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        if ((*it)->data(0, Qt::UserRole + 4).toString() == QStringLiteral("element")) {
            const auto ids = (*it)->data(0, Qt::UserRole + 1).toStringList();
            require(ids.size() == 1 && !elementIds.contains(ids.front()),
                "each element appears once in the canonical object hierarchy");
            elementIds.insert(ids.front());
        }
        if (!(*it)->data(0, Qt::UserRole + 2).isValid()) continue;
        auto* geometry = *it;
        tree->clearSelection();
        geometry->setSelected(true);
        QApplication::processEvents();
        require(inspectionScene->property("hasInspectionMesh").toBool(), "each advertised geometry renders");
        const auto camera = inspectionScene->cameraView();
        require(camera.position == inspectionCamera.position && camera.focalPoint == inspectionCamera.focalPoint,
            "geometry changes preserve camera");
        geometry->setCheckState(0, Qt::Checked);
        tree->clearSelection();
        require(inspectionScene->property("hasInspectionMesh").toBool(),
            "checked geometry remains renderable without a selected node");
        geometry->setCheckState(0, Qt::Unchecked);
        ++geometries;
        require(!geometry->toolTip(0).isEmpty(), "geometry provenance is visible");
        if (geometry->text(0).startsWith(QStringLiteral("Quellmesh"))) {
            const auto key = geometry->parent()->text(0) + geometry->text(0);
            require(!sourceIds.contains(key), "source reused by roles is listed only once");
            sourceIds.insert(key);
        }
    }
    require(geometries > 0 && !sourceIds.isEmpty(), "source geometries exist");
    require(!elementIds.isEmpty(), "canonical object tree retains model elements");
    require(window.grab().save(evidence.filePath(QStringLiteral("boat_unified_source.bmp")), "BMP"),
        "unified source model screenshot");
    QTreeWidgetItem* derivedRoot = nullptr;
    for (QTreeWidgetItemIterator it(tree); *it; ++it)
        if ((*it)->text(0) == QStringLiteral("Abgeleitete Daten") && (*it)->childCount()) {
            derivedRoot = *it;
            break;
        }
    require(derivedRoot, "derived geometry branch exists under its element");
    require(derivedRoot->childCount() > 0, "derived geometry remains available in unified format");
    derivedRoot->setExpanded(true);
    auto* derived = derivedRoot->child(0);
    derived->setSelected(true);
    tree->scrollToItem(derived);
    QApplication::processEvents();
    require(window.grab().save(evidence.filePath(QStringLiteral("boat_unified_derived.bmp")), "BMP"),
        "unified derived geometry screenshot");
    tree->clearSelection();
    int movementPreviews = 0;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        const auto id = (*it)->data(0, Qt::UserRole + 5).toString();
        if (id.isEmpty()) continue;
        auto* previewItem = *it;
        previewItem->setCheckState(0, Qt::Checked);
        tree->clearSelection();
        inspectionScene->renderNow();
        require(inspectionScene->property("inspectionMovementPreviewCount").toInt() > 0,
            "checked movement preview is visible without changing simulation");
        const auto camera = inspectionScene->cameraView();
        require(camera.position == inspectionCamera.position && camera.focalPoint == inspectionCamera.focalPoint,
            "movement preview preserves inspection camera");
        require(previewItem->toolTip(0).contains(QStringLiteral("Normal:")),
            "movement preview explains pose colors");
        tree->scrollToItem(previewItem);
        if (!movementPreviews)
            require(window.grab().save(evidence.filePath(QStringLiteral("boat_movement_preview.bmp")), "BMP"),
                "movement preview screenshot");
        previewItem->setCheckState(0, Qt::Unchecked);
        require(inspectionScene->property("inspectionMovementPreviewCount").toInt() == 0,
            "unchecking preview removes only temporary movement graphics");
        ++movementPreviews;
    }
    require(movementPreviews > 0, "moving elements expose a preview in the common object tree");
    const auto simulationCamera = simulationScene->cameraView();
    testWorkspaceTabs(window, *simulationScene, evidence, report);
    tabs->setCurrentIndex(0);
    require(simulationScene->cameraView().position == simulationCamera.position,
        "inspection never changes simulation camera");
    const auto treeBefore = tree->topLevelItemCount();
    QFile changed(copy.filePath(QStringLiteral("modell.json")));
    require(changed.open(QIODevice::WriteOnly | QIODevice::Truncate), "change captured fixture");
    changed.write("{");
    changed.close();
    QElapsedTimer watcher;
    watcher.start();
    while (watcher.elapsed() < 1400) {
        QApplication::processEvents();
        QThread::msleep(10);
    }
    require(tree->topLevelItemCount() == treeBefore,
        "file watcher must not replace or invalidate opened inspection");
    tree->topLevelItem(0)->setSelected(true);
    require(values->topLevelItemCount() > 0, "properties still work after corrupting source file");
    const auto frozenSelection = tree->selectedItems().front()->data(0, Qt::UserRole).toString();
    const int frozenRows = values->topLevelItemCount();
    tabs->setCurrentIndex(1);
    tabs->setCurrentIndex(2);
    tabs->setCurrentIndex(0);
    require(!tree->selectedItems().empty()
        && tree->selectedItems().front()->data(0, Qt::UserRole).toString() == frozenSelection
        && values->topLevelItemCount() == frozenRows,
        "workspace switching preserves inspection node and property selection");
    require(window.grab().save(evidence.filePath(QStringLiteral("boat_inspection.bmp")), "BMP"),
        "inspection screenshot");
    tree->clearSelection();
    require(window.grab().save(evidence.filePath(QStringLiteral("boat_display.bmp")), "BMP"),
        "default boat display screenshot");

    segelphysik::InspectionMesh pointMesh;
    pointMesh.id = "point";
    pointMesh.element_id = "mass";
    pointMesh.type = "massenpunkte";
    pointMesh.topology = "punkte";
    pointMesh.points_m = {{{1.0, 2.0, 3.0}}};
    const auto points = toDisplayParts(std::vector<segelphysik::InspectionMesh>{pointMesh});
    require(points.size() == 1 && points[0].mesh->GetNumberOfVerts() == 1
        && points[0].mesh->GetNumberOfPolys() == 0, "point mesh conversion preserves topology");
    QString error;
    require(inspectionScene->setModel(points, &error), "point topology rendered without fake triangles");
    inspectionScene->setInspectionSelection({QStringLiteral("other")}, true);
    require(std::abs(inspectionScene->inspectionOpacity(QStringLiteral("mass")) - 0.15) < 1e-12,
        "nonselected context opacity is exactly 0.15");
    inspectionScene->setInspectionSelection({QStringLiteral("mass")}, true);
    require(inspectionScene->inspectionOpacity(QStringLiteral("mass")) == 1.0,
        "selected element is opaque");
    const auto singleFolder = QDir(temporary.path()).filePath(QStringLiteral("single-source"));
    segelphysik::konvertiere_modell_v3(
        std::filesystem::path(QDir(models).filePath(QStringLiteral("quader/modell.json")).toStdWString()),
        std::filesystem::path(singleFolder.toStdWString()));
    window.setModelFolder(singleFolder);
    window.selectInitialModel(QStringLiteral("quader"));
    waitUntil([&] { return tree->topLevelItemCount() == 1
        && inspectionScene->property("hasInspectionMesh").toBool(); },
        "single-source model GUI load timeout");
    inspectionScene->renderNow();
    int singleSources = 0;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        require(!(*it)->text(0).contains(QStringLiteral("Bestandsquellen")),
            "schema 3 must not retain competing source references");
        if ((*it)->text(0).startsWith(QStringLiteral("Quellmesh"))) ++singleSources;
    }
    require(singleSources == 1 && inspectionScene->property("hasInspectionMesh").toBool(),
        "strict single-source model renders through the same GUI pipeline");
    require(window.grab().save(evidence.filePath(QStringLiteral("boat_schema3.bmp")), "BMP"),
        "strict single-source render screenshot");
    const auto derivedFolder = QDir(temporary.path()).filePath(QStringLiteral("single-source-derived"));
    segelphysik::konvertiere_modell_v3(
        std::filesystem::path(QDir(models).filePath(QStringLiteral("optimist/modell.json")).toStdWString()),
        std::filesystem::path(derivedFolder.toStdWString()), true);
    window.setModelFolder(derivedFolder);
    window.selectInitialModel(QStringLiteral("optimist"));
    waitUntil([&] { return tree->topLevelItemCount() == 1
        && inspectionScene->property("hasInspectionMesh").toBool(); },
        "derived single-source boat GUI load timeout");
    QSet<QString> sourceOwners;
    QTreeWidgetItem* derivedVolume = nullptr;
    QTreeWidgetItem* derivedMass = nullptr;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        require(!(*it)->text(0).contains(QStringLiteral("Bestandsquellen")),
            "derived boat has no competing sources");
        if ((*it)->text(0).startsWith(QStringLiteral("Quellmesh"))) {
            const auto owners = (*it)->data(0,Qt::UserRole+1).toStringList();
            require(owners.size() == 1 && !sourceOwners.contains(owners.front()),
                "derived boat exposes exactly one source per element");
            sourceOwners.insert(owners.front());
        }
        if ((*it)->text(0) == QStringLiteral("hydrostatik_volumen")) derivedVolume = *it;
        if ((*it)->text(0) == QStringLiteral("massen_mesh")) derivedMass = *it;
    }
    require(derivedVolume && derivedMass && sourceOwners.size() > 1,
        "plane-cut volume and open mass shell appear as derived data under elements");
    require(derivedVolume->toolTip(0).contains(QStringLiteral("z_schnitt"))
        && derivedMass->toolTip(0).contains(QStringLiteral("ohne_deckflaeche")),
        "derived geometry controls explain the actual Core derivation");
    tree->expandAll();
    derivedVolume->setCheckState(0,Qt::Checked);
    derivedVolume->setSelected(true);
    inspectionScene->renderNow();
    require(values->topLevelItemCount() > 0, "derived element selection exposes all properties");
    bool derivedUseListed = false;
    for (QTreeWidgetItemIterator it(values); *it; ++it)
        if ((*it)->text(2) == QStringLiteral("Verwendungen")
            && (*it)->text(3).contains(QStringLiteral("hydrostatik_volumen (Ableitung)")))
            derivedUseListed = true;
    require(derivedUseListed, "source usage table includes derived physical uses, not just unchanged copies");
    require(window.grab().save(evidence.filePath(QStringLiteral("boat_schema3_derived.bmp")), "BMP"),
        "single-source boat with actual derived volume screenshot");
    tree->clearSelection();
    tree->topLevelItem(0)->setSelected(true);
    checkMovementBounds(true);
    report.insert(QStringLiteral("boatInspectionTreesMeshesValuesFixedSnapshotAndLayout"), true);
    report.insert(QStringLiteral("unifiedModelFormatRendered"), true);
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
    auto* values = window.findChild<QTreeWidget*>(QStringLiteral("simulationValues"));
    require(run && stop && mode && duration && profiles && vectors && slider && values && status
        && cameraCombo && navigationCheck,
        "MainWindow integration controls absent");
    waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "Folkeboot GUI load timeout");
    require(!unexpectedDialog, "unexpected model load error");
    VtkSceneWidget* scene = nullptr;
    for (auto* widget : window.findChildren<QWidget*>()) {
        if (auto* candidate = dynamic_cast<VtkSceneWidget*>(widget);
            candidate && candidate->objectName() == QStringLiteral("simulationScene")) scene = candidate;
    }
    require(scene != nullptr, "MainWindow scene absent");
    testWorkspaceTabs(window, *scene, evidence, report);
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
    auto* workspaceTabs = window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
    for (int index = 0; index < workspaceTabs->count(); ++index) {
        workspaceTabs->setCurrentIndex(index);
        if (index == 0) {
            auto* inspectionTree = window.findChild<QTreeWidget*>(QStringLiteral("inspectionObjectTree"));
            require(inspectionTree && inspectionTree->topLevelItemCount() > 0,
                "inspection remains usable during live simulation");
            inspectionTree->topLevelItem(0)->setSelected(true);
        }
        require(mode->currentIndex() == 1 && slider->value() == 0 && stop->isEnabled(),
            "workspace switch must not change simulation mode, selected state or stop a live run");
    }
    workspaceTabs->setCurrentIndex(2);
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
    if (unexpectedDialog || !status->text().contains(QStringLiteral("abgebrochen")))
        throw std::runtime_error("GUI cancellation did not preserve partial status: "
            + status->text().toStdString());
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

    MainWindow movementWindow;
    movementWindow.setModelFolder(models);
    movementWindow.selectInitialModel(QStringLiteral("420"));
    movementWindow.show();
    QPushButton* movementRun = nullptr;
    for (auto* button : movementWindow.findChildren<QPushButton*>()) {
        if (button->text() == QStringLiteral("Core-Lauf starten")) movementRun = button;
    }
    auto* movementStop = movementWindow.findChild<QPushButton*>(QStringLiteral("stopSimulation"));
    auto* movementMode = movementWindow.findChild<QComboBox*>(QStringLiteral("simulationMode"));
    auto* movementDuration = movementWindow.findChild<QDoubleSpinBox*>(
        QStringLiteral("simulationDuration"));
    auto* movementPanel = movementWindow.findChild<QGroupBox*>(QStringLiteral("movementPanel"));
    auto* movementStatus = movementWindow.findChild<QLabel*>(QStringLiteral("status"));
    auto* movementModelName = movementWindow.findChild<QLabel*>(QStringLiteral("modelName"));
    require(movementRun && movementStop && movementMode && movementDuration && movementPanel,
        "movement simulation controls are missing");
    waitUntil([&] {
        return (movementRun->isEnabled() && movementStatus
            && movementStatus->text() == QStringLiteral("Core-Prüfung erfolgreich")
            && movementModelName
            && movementModelName->toolTip().startsWith(QStringLiteral("420 · Fassung")))
            || unexpectedDialog;
    },
        "420 movement model load timeout");
    require(!unexpectedDialog, "unexpected movement model load error");
    auto* rudder = movementWindow.findChild<QSlider*>(QStringLiteral("movement_Ruder"));
    auto* sword = movementWindow.findChild<QSlider*>(QStringLiteral("movement_Schwert"));
    require(rudder && sword,
        "420 movement overlay controls are missing");
    require(!movementPanel->isVisible() && rudder->minimum() == 0 && rudder->maximum() == 1800
            && rudder->value() == 900 && sword->minimum() == 0 && sword->maximum() == 1000
            && sword->value() == 1000,
        "movement controls do not match model defaults or project-wide limits");
    movementMode->setCurrentIndex(1);
    movementWindow.findChild<QTabBar*>(QStringLiteral("workspaceTabs"))->setCurrentIndex(2);
    require(movementPanel->isVisible(), "dynamic movement panel is hidden");
    waitUntil([&] { return movementRun->isEnabled() || unexpectedDialog; },
        "dynamic movement run did not become available");
    require(!unexpectedDialog, "unexpected dynamic movement setup error");
    movementDuration->setValue(0.0);
    movementRun->click();
    require(movementStop->isEnabled(), "dynamic run did not enable the stop control");
    waitUntil([&] { return (rudder->isEnabled() && sword->isEnabled()) || unexpectedDialog; },
        "movement sliders did not become ready after the first Core state");
    require(!unexpectedDialog, "unexpected movement initialization error");
    rudder->setValue(1100);
    sword->setValue(500);
    auto* rudderValue = movementWindow.findChild<QLabel*>(QStringLiteral("movementValue_Ruder"));
    auto* swordValue = movementWindow.findChild<QLabel*>(QStringLiteral("movementValue_Schwert"));
    require(rudderValue && swordValue, "movement confirmation labels are missing");
    waitUntil([&] {
        return (rudderValue->text().contains(QStringLiteral("Core 20.0°"))
            && swordValue->text().contains(QStringLiteral("Core 50.0 %")))
            || unexpectedDialog;
    }, "Core did not confirm the requested live movement values");
    require(!unexpectedDialog, "unexpected live movement error");
    movementStop->click();
    waitUntil([&] { return !movementStop->isEnabled() || unexpectedDialog; },
        "live movement simulation cancellation timeout");
    require(!unexpectedDialog, "unexpected movement cancellation error");
    movementWindow.close();
    report.insert(QStringLiteral("liveMovementOverlay"), true);
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
    require(overlay != nullptr, "navigation overlay is missing from actual VTK viewport");
    const auto viewportSize = overlay->parentWidget()->size();
    require(overlay->size() == QSize(180, 670).boundedTo(viewportSize),
        "initial navigation size must fit the actual VTK viewport");
    const auto initialOverlaySize = overlay->size();
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
    const QSize availableSize(overlay->parentWidget()->width() - overlay->x(),
        overlay->parentWidget()->height() - overlay->y());
    require(overlay->width() == std::min(initialOverlaySize.width() + 100, availableSize.width())
            && overlay->height() == std::min(initialOverlaySize.height() + 200, availableSize.height()),
        "resize grip must grow instruments within the actual VTK viewport");
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
        if (id == QStringLiteral("420") || id == QStringLiteral("folkeboot")) {
            require(force.contract == QStringLiteral("4"), "movement model not V4");
            require(force.contract == QStringLiteral("4"), "movement model not V4");
            require(!last.value(QStringLiteral("bewegungen")).toArray().isEmpty()
                    && !last.value(QStringLiteral("elementtransformationen_modell")).toArray().isEmpty(),
                "movement result omitted confirmed settings or element poses");
        } else {
            require(force.contract == QStringLiteral("3"), "static model test-drive contract changed");
        }
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
        if (app.arguments().size() == 4 && app.arguments()[1] == QStringLiteral("--inspection-only")) {
            QDir evidence(app.arguments()[3]);
            require(evidence.mkpath(QStringLiteral(".")), "inspection evidence directory");
            QJsonObject report;
            testBoatInspection(app.arguments()[2], evidence, report);
            require(renderErrors->count == 0, "VTK error in inspection test");
            std::cout << QJsonDocument(report).toJson().toStdString();
            return 0;
        }
        if (app.arguments().size() == 4 && app.arguments()[1] == QStringLiteral("--main-window-only")) {
            QDir evidence(app.arguments()[3]);
            require(evidence.mkpath(QStringLiteral(".")), "main-window evidence directory");
            QJsonObject report;
            testMainWindow(app.arguments()[2], evidence, report);
            require(renderErrors->count == 0, "VTK error in focused main-window test");
            std::cout << QJsonDocument(report).toJson().toStdString();
            return 0;
        }
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
        scene.renderNow();
        const auto firstImage = scene.captureImage();
        require(scene.setState(simulation.states.back(),
            forcedisplay::parseForceState(simulation.states.back()), &error), "animation last state");
        scene.renderNow();
        require(simulation.states.back().value(QStringLiteral("zeit_s")).toDouble()
                > referenceState.value(QStringLiteral("zeit_s")).toDouble(),
            "animation fixture must advance actual Core time");
        require(digest(scene.captureImage()) != digest(firstImage), "advanced Core state renders identically");
        require(scene.setState(referenceState, forceState, &error), "animation replay first state");
        scene.renderNow();
        const auto replayImage = scene.captureImage();
        const auto replayDifference = differingPixels(firstImage, replayImage);
        const auto replayTolerance = std::max<qsizetype>(100, firstImage.width() * firstImage.height() / 10000);
        require(replayDifference <= replayTolerance,
            "same simulation time produced a visibly different frame");
        report.insert(QStringLiteral("animationReplayDifferentPixels"),
            static_cast<qint64>(replayDifference));
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
        require(digest(scene.captureImage()) == digest(firstImage),
            "model reload retained stale material or render state");
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
