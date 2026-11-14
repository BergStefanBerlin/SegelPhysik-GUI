#include "display_geometry.hpp"
#include "core_adapter.hpp"
#include "graphics_settings.hpp"
#include "vtk_scene.hpp"
#include "main_window.hpp"
#include "navigation.hpp"
#include "boat_inspection.hpp"

#include <segelphysik/katalog.hpp>
#include <segelphysik/defaults.hpp>
#include <segelphysik/modell.hpp>
#include <segelphysik/modell_fingerprints.hpp>
#include <segelphysik/segelkonfigurator.hpp>
#include <segelphysik/wind.hpp>
#include <QApplication>
#include <QTabWidget>
#include <QStackedWidget>
#include <QAction>
#include <QCheckBox>
#include <QStyle>
#include <QStyleOptionButton>
#include <QComboBox>
#include <QColorDialog>
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
#include <QContextMenuEvent>
#include <QWheelEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QScopeGuard>
#include <QSizeGrip>
#include <QTimer>
#include <QTabBar>
#include <QTemporaryDir>
#include <QMap>
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
#include <vtkRenderWindowInteractor.h>
#include <vtkRendererCollection.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <future>
#include <functional>
#include <iostream>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <thread>
#include <tuple>
#ifdef _WIN32
#include <Windows.h>
#undef small
#endif

namespace {
int testInspectionCameraAndPointerRouting();

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
void waitUntil(const std::function<bool()>& ready, const char* message);

QTreeWidgetItem* sailTreeItem(QTreeWidget* tree, const QString& id)
{
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        const auto ids = (*it)->data(0, Qt::UserRole + 1).toStringList();
        if ((*it)->data(0, Qt::UserRole + 4).toString() == QStringLiteral("element")
            && ids.size() == 1 && ids.front() == id) return *it;
    }
    return nullptr;
}

QAction* menuAction(QMenu* menu, const QString& label)
{
    if (!menu) return nullptr;
    for (auto* action : menu->actions()) {
        if (action->text() == label) return action;
        if (auto* found = menuAction(action->menu(), label)) return found;
    }
    return nullptr;
}

enum class SailDialogTestAction { Accept, Cancel, RejectInvalid };

void toggleMovementContextAction(QTreeWidget* tree, QTreeWidgetItem* item, bool checked)
{
    tree->scrollToItem(item);
    QApplication::processEvents();
    const auto rect = tree->visualItemRect(item);
    require(rect.isValid() && !rect.isEmpty(), "movement owner is visible");
    bool triggered = false;
    bool invalidState = false;
    QTimer driver;
    driver.setInterval(10);
    QObject::connect(&driver, &QTimer::timeout, &driver, [&] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            auto* menu = qobject_cast<QMenu*>(widget);
            if (!menu || !menu->isVisible()) continue;
            auto* action = menuAction(menu, QStringLiteral("Bewegungsvorschau"));
            invalidState = action && action->isEnabled()
                && (!action->isCheckable() || action->isChecked() == checked);
            if (action && action->isEnabled() && !invalidState) {
                triggered = true;
                action->trigger();
            }
            menu->close();
            driver.stop();
            return;
        }
    });
    driver.start();
    const auto local = rect.center();
    QContextMenuEvent context(QContextMenuEvent::Mouse, local,
        tree->viewport()->mapToGlobal(local));
    QApplication::sendEvent(tree->viewport(), &context);
    require(!invalidState, "movement context action reflects current state");
    waitUntil([&] { return triggered; }, "movement context action not available");
}

void invokeSailContextAction(QTreeWidget* tree, const QString& elementId,
    const QString& actionText, const QString& dialogValue = {},
    SailDialogTestAction dialogAction = SailDialogTestAction::Accept,
    std::optional<bool> desiredChecked = std::nullopt)
{
    QApplication::processEvents();
    QTreeWidgetItem* item = nullptr;
    const auto rowError = "420 sail has an exact element row in inspection tree: "
        + elementId.toStdString() + " (" + actionText.toStdString() + ")";
    waitUntil([&] {
        item = sailTreeItem(tree, elementId);
        return item != nullptr;
    }, rowError.c_str());
    tree->scrollToItem(item);
    QApplication::processEvents();
    const auto rect = tree->visualItemRect(item);
    require(rect.isValid() && !rect.isEmpty(), "420 sail row is visible for context menu");
    const QPoint local = rect.center();
    const QPoint global = tree->viewport()->mapToGlobal(local);
    bool actionTriggered = false;
    bool actionUnavailable = false;
    bool dialogAccepted = dialogValue.isEmpty();
    QTimer driver;
    driver.setInterval(10);
    QObject::connect(&driver, &QTimer::timeout, &driver, [&] {
        if (!actionTriggered) {
            for (auto* widget : QApplication::topLevelWidgets()) {
                auto* menu = qobject_cast<QMenu*>(widget);
                if (!menu || !menu->isVisible()) continue;
                auto* action = menuAction(menu, actionText);
                if (!action || !action->isEnabled()) {
                    actionUnavailable = true;
                    menu->close();
                    driver.stop();
                    break;
                }
                actionTriggered = true;
                if (!dialogValue.isEmpty()) {
                    QTimer::singleShot(100, tree, [&] {
                        for (auto* widget : QApplication::topLevelWidgets()) {
                            auto* dialog = qobject_cast<QDialog*>(widget);
                            if (!dialog || !dialog->isVisible()) continue;
                            auto* input = dialog->findChild<QLineEdit*>(
                                QStringLiteral("fockOffsetCm"));
                            auto* buttons = dialog->findChild<QDialogButtonBox*>();
                            if (!input || !buttons || !buttons->button(QDialogButtonBox::Ok))
                                continue;
                            if (dialogAction == SailDialogTestAction::Cancel) {
                                buttons->button(QDialogButtonBox::Cancel)->click();
                                dialogAccepted = true;
                            } else if (dialogAction == SailDialogTestAction::RejectInvalid) {
                                input->setText(QStringLiteral("not-a-number"));
                                QTimer::singleShot(100, tree, [tree, &dialogAccepted] {
                                    for (auto* warningWidget : QApplication::topLevelWidgets()) {
                                        auto* warning = qobject_cast<QMessageBox*>(warningWidget);
                                        if (warning && warning->isVisible()) {
                                            warning->accept();
                                            break;
                                        }
                                    }
                                    QTimer::singleShot(100, tree, [tree, &dialogAccepted] {
                                        for (auto* dialogWidget : QApplication::topLevelWidgets()) {
                                            auto* dialog = qobject_cast<QDialog*>(dialogWidget);
                                            if (!dialog || !dialog->isVisible()
                                                || !dialog->findChild<QLineEdit*>(
                                                    QStringLiteral("fockOffsetCm"))) continue;
                                            auto* dialogButtons =
                                                dialog->findChild<QDialogButtonBox*>();
                                            if (!dialogButtons) continue;
                                            dialogAccepted = true;
                                            dialogButtons->button(QDialogButtonBox::Cancel)->click();
                                            return;
                                        }
                                    });
                                });
                                buttons->button(QDialogButtonBox::Ok)->click();
                            } else {
                                input->setText(dialogValue);
                                buttons->button(QDialogButtonBox::Ok)->click();
                                dialogAccepted = true;
                            }
                            return;
                        }
                    });
                }
                if (!desiredChecked || action->isChecked() != *desiredChecked)
                    action->trigger();
                menu->close();
                break;
            }
        }
        if (!dialogValue.isEmpty() && actionTriggered && !dialogAccepted
            && dialogAction != SailDialogTestAction::RejectInvalid) {
            for (auto* widget : QApplication::topLevelWidgets()) {
                auto* dialog = qobject_cast<QDialog*>(widget);
                if (!dialog || !dialog->isVisible()) continue;
                auto* input = dialog->findChild<QLineEdit*>(QStringLiteral("fockOffsetCm"));
                if (!input) continue;
                auto* buttons = dialog->findChild<QDialogButtonBox*>();
                require(buttons && buttons->button(QDialogButtonBox::Cancel),
                    "Fock offset dialog exposes its cancel action");
                dialogAccepted = true;
                buttons->button(QDialogButtonBox::Cancel)->click();
                break;
            }
        }
        if (actionTriggered && dialogAccepted) driver.stop();
    });
    driver.start();
    QContextMenuEvent context(QContextMenuEvent::Mouse, local, global);
    QApplication::sendEvent(tree->viewport(), &context);
    if (actionUnavailable) {
        const auto* diagnostic = tree->window()->findChild<QLabel*>(
            QStringLiteral("inspectionDiagnostics"));
        throw std::runtime_error(("GUI context action unavailable: "
            + elementId.toStdString() + " / " + actionText.toStdString()
            + (diagnostic ? " / " + diagnostic->text().toStdString() : "")).c_str());
    }
    try {
        waitUntil([&] { return actionTriggered && dialogAccepted; },
            "GUI context action or Fock dialog timed out");
    } catch (const std::exception&) {
        const auto* diagnostic = tree->window()->findChild<QLabel*>(
            QStringLiteral("inspectionDiagnostics"));
        throw std::runtime_error(("GUI context action timed out: "
            + elementId.toStdString() + " / " + actionText.toStdString()
            + (diagnostic ? " / " + diagnostic->text().toStdString() : "")).c_str());
    }
    driver.stop();
    QApplication::processEvents();
}

std::optional<segelphysik::SegelLage> sailPose(
    const segelphysik::SegelKonfigurationSnapshot& configuration, const QString& id)
{
    for (std::size_t i = 0; i < configuration.referenzen.size()
        && i < configuration.lagen.size(); ++i)
        if (QString::fromStdString(configuration.referenzen[i].element_id) == id)
            return configuration.lagen[i];
    return std::nullopt;
}

std::optional<segelphysik::SegelReferenz> sailReference(
    const segelphysik::SegelKonfigurationSnapshot& configuration, const QString& id)
{
    for (const auto& reference : configuration.referenzen)
        if (QString::fromStdString(reference.element_id) == id) return reference;
    return std::nullopt;
}

forcedisplay::Vec3 rotateSailPoint(const std::array<double, 4>& q,
    const forcedisplay::Vec3& point)
{
    const forcedisplay::Vec3 v{q[0], q[1], q[2]};
    const auto cross1 = forcedisplay::cross(v, point);
    const auto cross2 = forcedisplay::cross(v, cross1);
    return forcedisplay::add(point, forcedisplay::add(
        forcedisplay::scale(cross1, 2.0 * q[3]), forcedisplay::scale(cross2, 2.0)));
}

forcedisplay::Vec3 posedSailPoint(const segelphysik::SegelReferenz& reference,
    const segelphysik::SegelLage& pose, const segelphysik::Vektor3& corner)
{
    Q_UNUSED(reference);
    return forcedisplay::add(rotateSailPoint(pose.rotation_xyzw,
        {corner[0], corner[1], corner[2]}),
        {pose.translation_m[0], pose.translation_m[1], pose.translation_m[2]});
}

bool dragInspectionPoint(VtkSceneWidget* scene, const QPointF& from,
    const QPointF& to, const std::function<void()>& afterMove = {})
{
    auto* viewport = scene->findChild<QVTKOpenGLNativeWidget*>();
    if (!viewport) return false;
    const auto global = [&](const QPointF& point) {
        return QPointF(viewport->mapToGlobal(point.toPoint()));
    };
    const auto startGlobal = global(from);
    QMouseEvent press(QEvent::MouseButtonPress, from, startGlobal,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent move(QEvent::MouseMove, to, global(to),
        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, to, global(to),
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &press);
    QApplication::sendEvent(viewport, &move);
    QApplication::processEvents();
    scene->renderNow();
    if (afterMove) afterMove();
    QApplication::sendEvent(viewport, &release);
    QApplication::processEvents();
    return true;
}

std::vector<std::array<double, 6>> inspectionGeometryBounds(VtkSceneWidget* scene)
{
    std::vector<std::array<double, 6>> result;
    auto* viewport = scene->findChild<QVTKOpenGLNativeWidget*>();
    if (!viewport || !viewport->renderWindow()) return result;
    auto* renderer = viewport->renderWindow()->GetRenderers()->GetFirstRenderer();
    if (!renderer) return result;
    auto* props = renderer->GetViewProps();
    props->InitTraversal();
    while (auto* prop = props->GetNextProp()) {
        auto* actor = vtkActor::SafeDownCast(prop);
        if (!actor || !actor->GetVisibility() || !actor->GetUseBounds()) continue;
        auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
        if (!mapper || !mapper->GetInput()) continue;
        double bounds[6];
        actor->GetBounds(bounds);
        std::array<double, 6> entry{};
        bool finite = true;
        for (int i = 0; i < 6; ++i) {
            entry[static_cast<std::size_t>(i)] = bounds[i];
            finite = finite && std::isfinite(bounds[i]);
        }
        if (finite) result.push_back(entry);
    }
    std::sort(result.begin(), result.end());
    return result;
}

void runUnifiedMontageIntegration(const QString& modelRoot)
{
    const auto source = std::filesystem::path(modelRoot.toStdWString());
    const auto mutableModelInputHashes = [&] {
        QMap<QString, QByteArray> hashes;
        const auto addFile = [&](const std::filesystem::path& file) {
            const auto path = QString::fromStdWString(file.wstring());
            QFile input(path);
            require(input.open(QIODevice::ReadOnly),
                "mutable model input is readable for checksum");
            hashes.insert(QString::fromStdString(file.lexically_relative(source).generic_string()),
                QCryptographicHash::hash(input.readAll(), QCryptographicHash::Sha256));
        };
        for (const auto& model : std::filesystem::directory_iterator(source)) {
            if (!model.is_directory()
                || !std::filesystem::exists(model.path() / L"modells_information.json")) continue;
            const auto modelData = model.path() / L"modells_data.json";
            if (std::filesystem::is_regular_file(modelData)) addFile(modelData);
            const auto visualisation = model.path() / L"modells" / L"visualisation";
            if (!std::filesystem::is_directory(visualisation)) continue;
            for (const auto& file : std::filesystem::recursive_directory_iterator(visualisation)) {
                if (file.is_regular_file()) addFile(file.path());
            }
        }
        return hashes;
    };
    const auto originalMutableInputHashes = mutableModelInputHashes();
    auto catalog = std::make_shared<segelphysik::Modellkatalog>(
        segelphysik::modellkatalog(source));
    int boats = 0;
    int sails = 0;
    int renderedModels = 0;
    for (const auto& entry : catalog->eintraege()) {
        std::cerr << "Unified GUI montage: loading " << entry.modell_id << '\n';
        auto snapshot = segelphysik::readModelInspection(
            *catalog, entry.modell_id, entry.modell_fassung);
        auto configuration = segelphysik::leseSegelKonfiguration(*catalog, *snapshot);
        if (configuration.status == segelphysik::SegelKonfigurationsStatus::Fehler) {
            std::string diagnostics = entry.modell_id;
            for (const auto& diagnostic : configuration.diagnosen)
                diagnostics += " / " + diagnostic.code + ": " + diagnostic.nachricht;
            throw std::runtime_error(diagnostics);
        }
        BoatInspection widget;
        widget.setSnapshot(snapshot, catalog);
        widget.resize(1400, 900);
        widget.show();
        widget.scene()->renderNow();
        require(!inspectionGeometryBounds(widget.scene()).empty(),
            "every current Core model renders visible GUI geometry");
        ++renderedModels;
        std::cerr << "Unified GUI montage: rendered " << entry.modell_id << '\n';
        auto* tree = widget.findChild<QTreeWidget*>(QStringLiteral("inspectionObjectTree"));
        require(tree != nullptr, "canonical object tree exists");
        QSet<QString> ids;
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            const auto* item = *it;
            if (item->data(0, Qt::UserRole + 2).isValid())
                require(!item->toolTip(0).isEmpty(),
                    "all models expose geometry provenance in the object tree");
            if (item->data(0, Qt::UserRole + 4).toString() != QStringLiteral("element")) continue;
            const auto elementIds = item->data(0, Qt::UserRole + 1).toStringList();
            require(elementIds.size() == 1 && !ids.contains(elementIds.front()),
                "all models retain exactly one reference per object");
            ids.insert(elementIds.front());
        }
        if (configuration.referenzen.empty()) {
            widget.close();
            continue;
        }
        QString cameraError;
        require(widget.setCameraPreset(QStringLiteral("backbord"), &cameraError),
            "all boats support axis-fixed manual editing");
        ++boats;
        const auto references = configuration.referenzen;
        for (const auto& reference : references) {
            const auto id = QString::fromStdString(reference.element_id);
            std::cerr << "Unified GUI montage: editing " << entry.modell_id
                      << '/' << reference.element_id << '\n';
            invokeSailContextAction(tree, id, reference.art == segelphysik::SegelArt::Fock
                ? QStringLiteral("Fock automatisch setzen")
                : QStringLiteral("Großsegel automatisch setzen"));
            invokeSailContextAction(tree, id, QStringLiteral("Manuelles Verschieben"), {},
                SailDialogTestAction::Accept, true);
            if (reference.art == segelphysik::SegelArt::Fock)
                invokeSailContextAction(tree, id,
                    QStringLiteral("Höhenversatz entlang Vorliek (cm)…"), QStringLiteral("10"));
            auto saved = segelphysik::leseSegelKonfiguration(*catalog, *snapshot);
            require(saved.status == segelphysik::SegelKonfigurationsStatus::Bereit
                    && saved.revision != configuration.revision,
                "every boat GUI action persists through Core");
            configuration = saved;
            bool rotated = false;
            for (const auto& view : {QStringLiteral("backbord"), QStringLiteral("steuerbord"),
                    QStringLiteral("bug"), QStringLiteral("heck"),
                    QStringLiteral("oben"), QStringLiteral("unten")}) {
                require(widget.setCameraPreset(view, &cameraError), "manual axis view exists");
                widget.scene()->renderNow();
                const auto currentReference = sailReference(configuration, id);
                const auto currentPose = sailPose(configuration, id);
                require(currentReference && currentPose, "current object pose and corners retained");
                const auto handle = widget.scene()->inspectionScreenPointForWorld(
                    posedSailPoint(*currentReference, *currentPose, currentReference->ecken.schothorn_m));
                const auto pivot = widget.scene()->inspectionScreenPointForWorld(
                    posedSailPoint(*currentReference, *currentPose, currentReference->ecken.hals_m));
                const auto lever = handle - pivot;
                const auto length = std::hypot(lever.x(), lever.y());
                if (length < 30 || !widget.scene()->rect().adjusted(10, 10, -10, -10)
                        .contains(handle.toPoint())) continue;
                const auto revision = configuration.revision;
                dragInspectionPoint(widget.scene(), handle,
                    handle + QPointF(-lever.y() / length * 12, lever.x() / length * 12));
                saved = segelphysik::leseSegelKonfiguration(*catalog, *snapshot);
                if (saved.revision == revision) continue;
                const auto after = sailPose(saved, id);
                require(after && after->rotation_xyzw != currentPose->rotation_xyzw,
                    "each boat supports real pointer-driven rigid sail rotation");
                configuration = saved;
                rotated = true;
                break;
            }
            require(rotated, "each sail has a usable semantic rotation handle in an axis view");
            ++sails;
        }
        widget.close();
        BoatInspection reopened;
        reopened.setSnapshot(snapshot, catalog);
        require(reopened.findChild<QLabel*>(QStringLiteral("inspectionDiagnostics"))->text()
                != QStringLiteral("Core-Konfiguration fehlt oder ordnet dem Segel keine eindeutige Referenz zu."),
            "every boat reopens its confirmed configuration");
    }
    require(renderedModels == static_cast<int>(catalog->eintraege().size()),
        "GUI rendered every current model in the catalog");
    require(boats == 3 && sails == 5, "all three current boats and five sails exercised");
    require(mutableModelInputHashes() == originalMutableInputHashes,
        "montage leaves visualisation folders and modells_data.json unchanged");
    std::cout << "Unified GUI montage: " << renderedModels << " models, "
              << boats << " boats, " << sails << " sails\n";
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

bool sameStableImage(const QImage& first, const QImage& second)
{
    if (first.size() != second.size()) return false;
    const auto a = first.convertToFormat(QImage::Format_ARGB32);
    const auto b = second.convertToFormat(QImage::Format_ARGB32);
    qsizetype changed = 0;
    qsizetype changedTwoLevels = 0;
    // Unchanged GPU frames can differ by one level, rarely two, in isolated pixels.
    const auto limit = std::max<qsizetype>(100, a.width() * a.height() / 10000);
    for (int y = 0; y < a.height(); ++y) {
        const auto* rowA = reinterpret_cast<const QRgb*>(a.constScanLine(y));
        const auto* rowB = reinterpret_cast<const QRgb*>(b.constScanLine(y));
        for (int x = 0; x < a.width(); ++x) {
            if (rowA[x] == rowB[x]) continue;
            const int difference = std::max({std::abs(qRed(rowA[x]) - qRed(rowB[x])),
                std::abs(qGreen(rowA[x]) - qGreen(rowB[x])),
                std::abs(qBlue(rowA[x]) - qBlue(rowB[x])),
                std::abs(qAlpha(rowA[x]) - qAlpha(rowB[x]))});
            if (++changed > limit || difference > 2 ||
                (difference == 2 && ++changedTwoLevels > std::max<qsizetype>(2, limit / 20)))
                return false;
        }
    }
    return true;
}

void testStableImageComparison()
{
    QImage baseline(20, 20, QImage::Format_ARGB32);
    baseline.fill(qRgb(100, 100, 100));
    auto changed = baseline;
    for (int x = 0; x < 5; ++x) changed.setPixel(x, 0, qRgb(102, 100, 100));
    require(sameStableImage(baseline, changed), "sparse measured GPU differences rejected");
    changed.setPixel(5, 0, qRgb(102, 100, 100));
    require(!sameStableImage(baseline, changed), "too many two-level differences accepted");
    changed = baseline;
    changed.setPixel(0, 0, qRgb(103, 100, 100));
    require(!sameStableImage(baseline, changed), "larger pixel difference accepted");
    changed = baseline;
    for (int i = 0; i < 101; ++i) changed.setPixel(i % 20, i / 20, qRgb(101, 100, 100));
    require(!sameStableImage(baseline, changed), "widespread pixel differences accepted");
}

void waitUntil(const std::function<bool()>& ready, const char* message)
{
    QElapsedTimer timeout;
    timeout.start();
    bool satisfied = ready();
    while (!satisfied && timeout.elapsed() < 15000) {
        QApplication::processEvents();
        satisfied = ready();
    }
    require(satisfied, message);
}

void confirmDynamicRun(MainWindow& window, double seconds, bool realtime = true)
{
    auto* timer = new QTimer(&window);
    timer->setInterval(10);
    QObject::connect(timer, &QTimer::timeout, &window, [&window, seconds, realtime, timer] {
        auto* dialog = window.findChild<QDialog*>(QStringLiteral("dynamicRunDialog"));
        if (!dialog || !dialog->isVisible()) return;
        timer->stop();
        timer->deleteLater();
        dialog->findChild<QDoubleSpinBox*>(QStringLiteral("runDuration"))->setValue(seconds);
        dialog->findChild<QDoubleSpinBox*>(QStringLiteral("runStep"))->setValue(.01);
        dialog->findChild<QCheckBox*>(QStringLiteral("runRealtime"))->setChecked(realtime);
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    });
    timer->start();
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
    const auto modelPath = QDir(models).filePath(QStringLiteral("420/modells_information.json"));
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
                    auto* renderer = scene.findChild<QVTKOpenGLNativeWidget*>()
                        ->renderWindow()->GetRenderers()->GetFirstRenderer();
                    const std::array<double, 2> clippingBefore{
                        renderer->GetActiveCamera()->GetClippingRange()[0],
                        renderer->GetActiveCamera()->GetClippingRange()[1]};
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
                    const auto restored = scene.captureImage();
                    if (digest(restored) != digest(withoutTrail)) {
                        withoutTrail.save(evidence.filePath(prefix + QStringLiteral("_trail_before.bmp")), "BMP");
                        restored.save(evidence.filePath(prefix + QStringLiteral("_trail_restored.bmp")), "BMP");
                        std::cerr << "Trail restore profile " << profileIndex << ": "
                            << differingPixels(withoutTrail, restored) << " differing pixels; clipping "
                            << clippingBefore[0] << ',' << clippingBefore[1] << " -> "
                            << renderer->GetActiveCamera()->GetClippingRange()[0] << ','
                            << renderer->GetActiveCamera()->GetClippingRange()[1] << '\n';
                    }
                    require(digest(restored) == digest(withoutTrail),
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

void testStaticViews(MainWindow& window, VtkSceneWidget& scene)
{
    using namespace forcedisplay;
    auto* column = window.findChild<QWidget*>(QStringLiteral("staticViewsColumn"));
    require(column && column->isVisible(), "static auxiliary column must be visible");
    const auto mainPosition = scene.mapToGlobal(QPoint());
    const auto columnPosition = column->mapToGlobal(QPoint());
    const auto controlPosition = window.findChild<QGroupBox*>(
        QStringLiteral("stationaryForcePanel"))->mapToGlobal(QPoint());
    require(mainPosition.x() + scene.width() <= columnPosition.x()
            && columnPosition.x() + column->width() <= controlPosition.x(),
        "auxiliary column must sit between main viewport and calculation controls");
    int previousBottom = -1;
    int firstHeight = 0;
    for (int i = 0; i < 3; ++i) {
        auto* view = dynamic_cast<VtkSceneWidget*>(
            window.findChild<QWidget*>(QStringLiteral("staticScene%1").arg(i)));
        require(view && view->isVisible(), "all three static views must exist");
        view->renderNow();
        auto* viewport = view->findChild<QVTKOpenGLNativeWidget*>();
        const double ratio = viewport->devicePixelRatioF();
        const auto image = view->captureImage();
        require(std::abs(image.width() - 0.75 * image.height()) <= 1.0,
            "actual auxiliary image must be portrait 4:3 within one device pixel");
        auto* slot = window.findChild<QWidget*>(QStringLiteral("staticViewSlot%1").arg(i));
        require(viewport->height() > 0 && viewport->width() > 0
                && slot->y() > previousBottom,
            "auxiliary views must not overlap");
        if (i == 0) firstHeight = viewport->height();
        require(viewport->height() == firstHeight && ratio > 0.,
            "auxiliary image heights must match");
        previousBottom = slot->y() + slot->height() - 1;
        require(view->parallelProjection() && !view->options().forces
                && !view->options().leverArm && view->options().staticForces
                && (i == 2 || !view->courseArrowGeometry()),
            "Auxiliary views must use shared resultants; only top may have a direction arrow.");
        const auto camera = view->cameraView();
        const auto direction = sub(camera.position, camera.focalPoint);
        if (i < 2) {
            require(camera.position[2] == 0. && camera.focalPoint[2] == 0.
                    && camera.viewUp == Vec3{0., 0., 1.},
                "side and stern cameras must remain exactly at water level with world up");
        } else {
            require(direction[0] == 0. && direction[1] == 0. && direction[2] > 0.
                    && camera.viewUp[2] == 0.,
                "top view must look vertically down with horizontal bow-up");
        }
        require(view->statePose().has_value() == scene.statePose().has_value(),
            "all four scenes must share state availability");
        if (const auto pose = scene.statePose()) {
            require(view->statePose()->position == pose->position
                    && view->statePose()->quaternionXyzw == pose->quaternionXyzw,
                "full Core pose must match without levelling heel, pitch or immersion");
            auto forward = toWorldVector(*pose, {1., 0., 0.});
            forward[2] = 0.;
            forward = scale(forward, 1. / norm(forward));
            const Vec3 expected = i == 0 ? Vec3{-forward[1], forward[0], 0.}
                : i == 1 ? scale(forward, -1.) : Vec3{0., 0., 1.};
            require(norm(cross(direction, expected)) < 1e-8 * norm(direction)
                    && dot(direction, expected) > 0.,
                "cameras must use actual horizontal Backbord/stern directions");
            if (i == 2)
                require(norm(sub(camera.viewUp, forward)) < 1e-10,
                    "top camera must keep the actual bow up");
        }
        for (bool sail : {false, true}) {
            require(view->pressurePoint(sail) == scene.pressurePoint(sail),
                "all views must share exact Core pressure points or their absence");
        }
        auto hostileProfile = graphics::defaults(1);
        hostileProfile.bodyOpacity = 0.05;
        hostileProfile.throughBody = true;
        hostileProfile.environmentPath = QStringLiteral("must-not-load-for-bound-view.hdr");
        view->setGraphicsProfile(hostileProfile);
        for (std::size_t feature = 0; feature < graphics::featureCount; ++feature) {
            const auto id = static_cast<graphics::Feature>(feature);
            require(view->graphicsProfile().enabled[feature]
                    == (id == graphics::Feature::Water || id == graphics::Feature::Waterline || id == graphics::Feature::Smooth
                        || id == graphics::Feature::FXAA),
                "bound views must enforce fixed schematic features independently of user settings");
        }
        require(view->graphicsProfile().bodyOpacity == 1.
                && view->graphicsProfile().environmentPath.isEmpty()
                && view->graphicsProfile().waterOpacity == 0.14
                && view->options().visibility == VtkSceneWidget::Visibility::ThroughBody
                && view->motionStateCount() == 0,
            "bound views must keep solid bodies, light water and unoccluded markers without environment or motion history");
        auto* renderers = viewport->renderWindow()->GetRenderers();
        renderers->InitTraversal();
        while (auto* layer = renderers->GetNextItem())
            if (layer->GetLayer() == 1)
                require(!layer->GetPreserveDepthBuffer(),
                    "bound pressure markers and labels must ignore scene occlusion");
        auto* renderer = viewport->renderWindow()->GetRenderers()->GetFirstRenderer();
        require(!renderer->GetUseImageBasedLighting() && !renderer->GetUseSSAO()
                && renderer->GetUseFXAA() && !renderer->GetGradientBackground(),
            "schematic renderer must have no environment lighting or decorative passes");
        auto* actors = renderer->GetActors();
        actors->InitTraversal();
        int solidParts = 0;
        while (auto* actor = actors->GetNextActor()) {
            auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
            if (!actor->GetVisibility() || !actor->GetUseBounds() || !mapper) continue;
            auto* mesh = mapper->GetInput();
            if (!mesh || mesh->GetNumberOfPolys() == 0) continue;
            ++solidParts;
            auto* property = actor->GetProperty();
            require(property->GetOpacity() == 1. && property->GetInterpolation() != VTK_PBR
                    && property->GetSpecular() == 0.,
                "every visible yacht surface must be solid and matte, including glass and cloth");
        }
        require(solidParts > 0, "schematic view must contain actual visible yacht geometry");
    }
}

void testStaticForceMenu(MainWindow& window, VtkSceneWidget& freeScene, const QDir& evidence)
{
    using namespace forcedisplay;
    auto* table = window.findChild<QTreeWidget*>(QStringLiteral("staticForceValues"));
    auto* navigationColumn = window.findChild<QScrollArea*>(QStringLiteral("staticNavigationColumn"));
    auto* navigation = navigationColumn->widget();
    require(!navigation->findChild<QLabel*>(QStringLiteral("navigationDragHandle"))->isVisible(),
        "Docked navigation must not show a Navigation heading.");
    auto* speedPanel = navigation->findChild<QWidget*>(QStringLiteral("navigationSpeed"));
    auto* speedRow = speedPanel->parentWidget();
    const int speedHeight = speedPanel->height();
    const auto previousSize = window.size();
    window.resize(window.width(), window.height() + 120);
    QApplication::processEvents();
    require(speedPanel->height() == speedHeight,
        "Resizing the static column must preserve speed-panel height.");
    auto* compass = navigation->findChild<QWidget*>(QStringLiteral("navigation_compass"));
    auto* roll = navigation->findChild<QWidget*>(QStringLiteral("navigation_roll"));
    auto* pitch = navigation->findChild<QWidget*>(QStringLiteral("navigation_pitch"));
    require(compass->y() == 0
            && compass->mapToGlobal(QPoint()).y() == freeScene.mapToGlobal(QPoint()).y(),
        "Visible compass card must start flush with the neighbouring main scene.");
    require(speedPanel->styleSheet().contains(QStringLiteral("border-radius:12px")),
        "Speed card must share the twelve-pixel panel rounding.");
    require(roll->y() - (compass->y() + compass->height()) == 6
            && pitch->y() - (roll->y() + roll->height()) == 6
            && speedRow->y() - (pitch->y() + pitch->height()) == 6
            && speedRow->y() + speedRow->height() == navigation->height(),
        "Instruments must fill remaining height with six-pixel gaps and speed panel at bottom.");
    window.resize(previousSize);
    QApplication::processEvents();
    auto* attitudeViews = window.findChild<QCheckBox*>(QStringLiteral("displayAttitudeViews"));
    auto* boundViews = window.findChild<QWidget*>(QStringLiteral("staticViewsColumn"));
    require(attitudeViews->isVisible() && attitudeViews->text() == QStringLiteral("Analyseansichten"),
        "Static view-column checkbox must be available under navigation.");
    attitudeViews->setChecked(false);
    require(!boundViews->isVisible(), "Analyseansichten must hide the complete right-hand view column.");
    attitudeViews->setChecked(true);
    require(boundViews->isVisible(), "Analyseansichten must restore the right-hand view column.");
    for (auto* label : window.findChildren<QLabel*>())
        if (label->text() == QStringLiteral("WIEDERGABE"))
            require(!label->isVisible(), "Static model must not show the playback row.");
    require(window.findChild<QCheckBox*>(QStringLiteral("staticForce_weight"))->y()
            < window.findChild<QCheckBox*>(QStringLiteral("staticForce_buoyancy"))->y(),
        "Simple selection must list weight before buoyancy.");
    require(table && table->isVisible() && table->columnCount() == 10
            && table->topLevelItemCount() > 0
            && table->selectionMode() == QAbstractItemView::ExtendedSelection,
        "Static values must use a full-width ten-column multiselect table.");
    require(!window.findChild<QWidget*>(QStringLiteral("balanceScroll"))->isVisible()
            && !window.findChild<QLabel*>(QStringLiteral("time"))->isVisible(),
        "Static values must remove the old left summary and time block.");
    for (auto* button : window.findChildren<QPushButton*>())
        if (button->text() == QStringLiteral("Werte kopieren"))
            require(!button->isVisible(), "Static values must remove the copy button.");
    bool hasForce = false, hasMoment = false;
    const int forceRowCount = table->topLevelItemCount();
    for (int i = 0; i < table->topLevelItemCount(); ++i) {
        const auto* row = table->topLevelItem(i);
        hasForce = hasForce || row->text(1) == QStringLiteral("Kraft");
        hasMoment = hasMoment || row->text(1) == QStringLiteral("Moment");
        if (row->text(1) == QStringLiteral("Moment"))
            require(row->text(8).startsWith(QStringLiteral("S ")),
                "Moment rows must explicitly name Core reference S.");
        const auto magnitude = row->data(3, Qt::UserRole + 10);
        if (magnitude.isValid()) {
            double squared = 0.;
            for (int axis = 4; axis <= 6; ++axis) {
                const double value = row->data(axis, Qt::UserRole + 10).toDouble();
                squared += value * value;
            }
            require(std::abs(magnitude.toDouble() - std::sqrt(squared)) < 1e-8,
                "Numeric table magnitude must match its displayed-frame vector components.");
        }
    }
    require(hasForce && hasMoment, "Table must include forces and moments regardless of arrow choices.");
    auto* kindFilter = window.findChild<QLineEdit*>(QStringLiteral("staticValueFilter1"));
    kindFilter->setText(QStringLiteral("Moment"));
    int visibleMoments = 0;
    for (int i = 0; i < table->topLevelItemCount(); ++i) {
        const auto* row = table->topLevelItem(i);
        if (row->isHidden()) continue;
        ++visibleMoments;
        require(row->text(1) == QStringLiteral("Moment"), "Kind filter must hide force rows.");
    }
    require(visibleMoments > 0, "Moment filter must retain matching rows.");
    auto* nameFilter = window.findChild<QLineEdit*>(QStringLiteral("staticValueFilter2"));
    nameFilter->setText(QStringLiteral("no-such-force"));
    for (int i = 0; i < table->topLevelItemCount(); ++i)
        require(table->topLevelItem(i)->isHidden(), "Column filters must combine with AND.");
    nameFilter->clear();
    kindFilter->clear();
    table->sortItems(3, Qt::AscendingOrder);
    std::optional<double> last;
    for (int i = 0; i < table->topLevelItemCount(); ++i) {
        const auto value = table->topLevelItem(i)->data(3, Qt::UserRole + 10);
        if (!value.isValid()) continue;
        require(!last || *last <= value.toDouble(), "Magnitude sort must be numeric, not lexicographic.");
        last = value.toDouble();
    }
    const int priorWidth = table->columnWidth(2);
    table->setColumnWidth(2, priorWidth + 40);
    require(table->columnWidth(2) == priorWidth + 40, "Static columns must be resizable.");
    table->topLevelItem(0)->setSelected(true);
    table->topLevelItem(1)->setSelected(true);
    require(table->selectedItems().size() == 2, "Multiple table rows must be selectable.");
    table->clearSelection();
    table->sortItems(0, Qt::AscendingOrder);
    std::array<VtkSceneWidget*, 4> views{&freeScene};
    for (int i = 0; i < 3; ++i)
        views[i + 1] = dynamic_cast<VtkSceneWidget*>(
            window.findChild<QWidget*>(QStringLiteral("staticScene%1").arg(i)));
    auto* sail = window.findChild<QCheckBox*>(QStringLiteral("staticForce_sail"));
    sail->setChecked(false);
    require(table->topLevelItemCount() == forceRowCount,
        "Graphical force selection must not remove numerical force or moment rows.");
    for (auto* view : views) {
        view->renderNow();
        require(!view->pressurePoint(true) && view->pressurePoint(false),
            "One shared checkbox must remove both SDP point and arrow in all four views.");
    }
    sail->setChecked(true);
    for (auto* view : views) {
        view->renderNow();
        require(view->pressurePoint(true).has_value(), "SDP must return in all four views.");
    }
    auto* direction = window.findChild<QCheckBox*>(QStringLiteral("staticForce_direction"));
    direction->setChecked(false);
    for (auto* view : views) {
        view->renderNow();
        require(!view->courseArrowGeometry(), "Shared direction toggle must affect every view.");
    }
    direction->setChecked(true);
    for (std::size_t i = 0; i < views.size(); ++i) {
        views[i]->renderNow();
        require(views[i]->courseArrowGeometry().has_value() == (i == 0 || i == 3),
            "Direction arrow must occur in Frei and top, never side or stern.");
    }
    for (const auto* key : {"friction", "hull", "rudder", "centreboard", "heave", "total"}) {
        auto* check = window.findChild<QCheckBox*>(
            QStringLiteral("staticForce_") + QString::fromLatin1(key));
        require(check && check->isEnabled() && !check->isChecked(),
            "Real 420 must offer separate complete resultants, all initially off.");
        check->setChecked(true);
    }
    const QColor chosen(QStringLiteral("#c842e1"));
    bool pickerSeen = false;
    QTimer::singleShot(0, [&] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (auto* picker = qobject_cast<QColorDialog*>(widget)) {
                pickerSeen = true;
                picker->setCurrentColor(chosen);
                picker->accept();
            }
        }
    });
    window.findChild<QPushButton*>(QStringLiteral("staticColor_sail"))->click();
    require(pickerSeen, "Color box must open an actual color picker.");
    for (auto* view : views) {
        require(QColor(view->options().staticStyles[static_cast<std::size_t>(StaticForce::Sail)].color) == chosen,
            "Chosen color must immediately reach all four views.");
        view->renderNow();
        bool coloredPoint = false, coloredArrow = false;
        auto* renderers = view->findChild<QVTKOpenGLNativeWidget*>()->renderWindow()->GetRenderers();
        renderers->InitTraversal();
        while (auto* layer = renderers->GetNextItem()) {
            if (layer->GetLayer() != 1) continue;
            auto* actors = layer->GetActors();
            actors->InitTraversal();
            while (auto* actor = actors->GetNextActor()) {
                if (!actor->GetVisibility()) continue;
                const auto* rgb = actor->GetProperty()->GetColor();
                if (std::abs(rgb[0] - chosen.redF()) > .001
                    || std::abs(rgb[1] - chosen.greenF()) > .001
                    || std::abs(rgb[2] - chosen.blueF()) > .001) continue;
                auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
                if (!mapper || !mapper->GetInput()) continue;
                coloredPoint = coloredPoint || mapper->GetInput()->GetNumberOfVerts() > 0;
                coloredArrow = coloredArrow || mapper->GetInput()->GetNumberOfLines() > 0;
            }
        }
        require(coloredPoint && coloredArrow,
            "Actual SDP point and resultant arrow actors must both use the picked color in every view.");
    }
    MainWindow reopened;
    require(QColor(reopened.findChild<QPushButton*>(QStringLiteral("staticColor_sail"))->toolTip()
                .section(QStringLiteral(": "), -1)) == chosen
            && reopened.findChild<QCheckBox*>(QStringLiteral("staticForce_rudder"))->isChecked(),
        "Static colors and selections must survive a new window.");
    reopened.close();
    window.findChild<QPushButton*>(QStringLiteral("staticDefaultColors"))->click();
    const auto defaults = staticForceDefaults();
    for (auto* view : views) {
        for (std::size_t i = 0; i < defaults.size(); ++i)
            require(QColor(view->options().staticStyles[i].color) == QColor(defaults[i].color),
                "Default Farben must reset all shared colors.");
        require(view->options().staticStyles[static_cast<std::size_t>(StaticForce::Rudder)].enabled,
            "Reset colors must not change force selections.");
        view->renderNow();
    }
    require(window.grab().save(evidence.filePath(QStringLiteral("static-force-menu.bmp")), "BMP"),
        "Shared force-menu screenshot must be written.");
    for (std::size_t i = 5; i < defaults.size(); ++i)
        window.findChild<QCheckBox*>(QStringLiteral("staticForce_") + defaults[i].key)->setChecked(false);
}

void testViewInteraction(MainWindow& window, VtkSceneWidget& freeScene, const QDir& evidence)
{
    using namespace forcedisplay;
    auto* stack = window.findChild<QStackedWidget*>(QStringLiteral("sceneStack"));
    std::array<VtkSceneWidget*, 4> views{&freeScene};
    for (int i = 0; i < 3; ++i)
        views[i+1] = dynamic_cast<VtkSceneWidget*>(
            window.findChild<QWidget*>(QStringLiteral("staticScene%1").arg(i)));
    const auto viewport = [](VtkSceneWidget* view) {
        return view->findChild<QVTKOpenGLNativeWidget*>();
    };
    const auto drag = [&](VtkSceneWidget* view, Qt::MouseButton button,
                          Qt::KeyboardModifiers modifiers, const QPointF& delta) {
        auto* target = viewport(view);
        const QPointF start(target->width() * .45, target->height() * .55);
        const QPointF global = target->mapToGlobal(start.toPoint());
        QMouseEvent press(QEvent::MouseButtonPress, start, global, button, button, modifiers);
        QMouseEvent move(QEvent::MouseMove, start + delta, global + delta,
            Qt::NoButton, button, modifiers);
        QMouseEvent release(QEvent::MouseButtonRelease, start + delta, global + delta,
            button, Qt::NoButton, modifiers);
        QApplication::sendEvent(target, &press);
        QApplication::sendEvent(target, &move);
        QApplication::sendEvent(target, &release);
        QApplication::processEvents();
        view->renderNow();
    };
    const auto wheel = [&](VtkSceneWidget* view) {
        auto* target = viewport(view);
        const QPointF point(target->width() * .5, target->height() * .5);
        QWheelEvent event(point, target->mapToGlobal(point.toPoint()), QPoint(), QPoint(0,120),
            Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(target, &event);
        view->renderNow();
    };
    const auto doubleClick = [&](VtkSceneWidget* view) {
        auto* target = viewport(view);
        const QPointF point(target->width() * .5, target->height() * .5);
        const QPointF global = target->mapToGlobal(point.toPoint());
        // Dispatch to the real child under the pointer, including navigation instruments.
        auto* receiver = target->childAt(point.toPoint());
        if (!receiver) receiver = target;
        const QPointF local = receiver->mapFromGlobal(global.toPoint());
        QMouseEvent event(QEvent::MouseButtonDblClick, local, global,
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease, local, global,
            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(receiver, &event);
        QApplication::sendEvent(receiver, &release);
        QApplication::processEvents();
    };
    const auto sameCamera = [](const VtkSceneWidget::CameraView& a,
                               const VtkSceneWidget::CameraView& b) {
        return norm(sub(a.position,b.position)) < 1e-8
            && norm(sub(a.focalPoint,b.focalPoint)) < 1e-8
            && norm(sub(a.viewUp,b.viewUp)) < 1e-10;
    };
    const std::array<QString, 4> names{QStringLiteral("Frei"), QStringLiteral("Seite"),
        QStringLiteral("Heck"), QStringLiteral("Draufsicht")};
    const auto labels = [&] {
        for (std::size_t i = 0; i < views.size(); ++i) {
            auto* target = viewport(views[i]);
            auto* name = views[i]->findChild<QLabel*>(QStringLiteral("viewName"));
            auto* button = views[i]->findChild<QPushButton*>(QStringLiteral("defaultView"));
            require(name && name->isVisible() && name->text() == names[i]
                    && target->rect().contains(name->geometry())
                    && std::abs(target->width() - name->geometry().right() - 1 - 4) <= 1
                    && std::abs(target->height() - name->geometry().bottom() - 1 - 4) <= 1,
                "every view must keep its exact name inside its bottom-right corner");
            require(button && button->isVisible() == (i != 0),
                "default button must belong only to each bound view, regardless of slot");
            if (i != 0) {
                require(!button->geometry().intersects(name->geometry()),
                    "default button must not overlap view name");
                require(button->width() >= button->fontMetrics().horizontalAdvance(button->text()) + 6,
                    "default button must leave enough room for its full text");
            }
            if (stack->currentWidget() != views[i]) {
                const auto size = target->size() * target->devicePixelRatioF();
                require(std::abs(size.width() - .75 * size.height()) <= 1.,
                    "small slot keeps portrait image aspect even when showing Frei");
            }
            require(views[i]->statePose().has_value() == freeScene.statePose().has_value()
                    && views[i]->pressurePoint(false) == freeScene.pressurePoint(false)
                    && views[i]->pressurePoint(true) == freeScene.pressurePoint(true),
                "swapped views must preserve shared data");
        }
    };
    labels();
    require(!freeScene.findChild<QLabel*>(QStringLiteral("drivingMomentLegend")),
        "removed moment legend must not remain");
    for (int i = 1; i < 4; ++i) {
        views[i]->renderNow();
        require(!views[i]->findChild<QLabel*>(QStringLiteral("drivingMomentLegend")),
            "bound views must not retain moment legends");
    }
    doubleClick(&freeScene);
    require(stack->currentWidget() == &freeScene, "double click large Frei must do nothing");
    const auto freeCamera = freeScene.cameraView();
    for (int i = 1; i < 4; ++i) {
        auto* view = views[i];
        auto* button = view->findChild<QPushButton*>(QStringLiteral("defaultView"));
        button->click();
        const auto initial = view->cameraView();
        const double initialScale = view->parallelScale();
        wheel(view);
        require(view->parallelScale() < initialScale, "wheel must actually zoom a bound camera");
        const double wheelScale = view->parallelScale();
        drag(view, Qt::RightButton, Qt::NoModifier, {0., 8.});
        require(view->parallelScale() < wheelScale, "right drag must zoom bound camera");
        drag(view, Qt::LeftButton, Qt::ShiftModifier, {4., 3.});
        drag(view, Qt::MiddleButton, Qt::NoModifier, {7., 9.});
        require(norm(sub(view->cameraView().focalPoint, initial.focalPoint)) > 1e-6,
            "middle drag must actually pan bound camera");
        const auto beforeLeftPan = view->cameraView();
        const double beforeLeftScale = view->parallelScale();
        if (i < 3)
            require(std::abs(beforeLeftPan.position[2] - beforeLeftPan.focalPoint[2]) < 1e-12
                    && beforeLeftPan.position[2] != 0.,
                "vertical side/stern pan must move both heights, not tilt the camera");
        else
            require(std::abs(beforeLeftPan.position[0] - beforeLeftPan.focalPoint[0]) < 1e-12
                    && std::abs(beforeLeftPan.position[1] - beforeLeftPan.focalPoint[1]) < 1e-12,
                "top pan must preserve vertical projection");
        drag(view, Qt::LeftButton, Qt::NoModifier, {10., 8.});
        const auto panned = view->cameraView();
        require(norm(sub(panned.focalPoint, beforeLeftPan.focalPoint)) > 1e-6
                && norm(sub(sub(panned.position, panned.focalPoint),
                    sub(beforeLeftPan.position, beforeLeftPan.focalPoint))) < 1e-8
                && norm(sub(panned.viewUp, beforeLeftPan.viewUp)) < 1e-10
                && view->parallelScale() == beforeLeftScale,
            "plain left drag must pan while preserving exact view direction, up and zoom");
        const double zoom = view->parallelScale();
        const auto beforeFree = freeScene.cameraView();
        doubleClick(view);
        require(stack->currentWidget() == view, "small view double click must enlarge that view");
        require(sameCamera(panned, view->cameraView()) && view->parallelScale() == zoom
                && sameCamera(beforeFree, freeScene.cameraView()),
            "swap and resize must preserve both camera states");
        labels();
        doubleClick(view);
        require(stack->currentWidget() == view, "double click large view must do nothing");
        const auto originalSize = window.size();
        window.resize(originalSize + QSize(40, 30));
        QApplication::processEvents();
        view->renderNow();
        require(sameCamera(panned, view->cameraView()) && view->parallelScale() == zoom,
            "resize must not silently overwrite bound user camera");
        window.resize(originalSize);
        QApplication::processEvents();
        button->click();
        require(stack->currentWidget() == view && view->parallelScale() > 0.
                && !sameCamera(panned, view->cameraView()),
            "default must reset only camera, not window arrangement");
        const auto reset = view->cameraView();
        if (i < 3) require(reset.position[2] == 0. && reset.focalPoint[2] == 0.,
            "default must restore exact side/stern waterline eye height");
        drag(view, Qt::LeftButton, Qt::NoModifier, {-11., -7.});
        const auto largePan = view->cameraView();
        require(norm(sub(largePan.focalPoint, reset.focalPoint)) > 1e-6
                && norm(sub(sub(largePan.position, largePan.focalPoint),
                    sub(reset.position, reset.focalPoint))) < 1e-8
                && largePan.viewUp == reset.viewUp,
            "plain left pan must also preserve the bound direction in the large slot");
        wheel(view);
        const double afterDefaultZoom = view->parallelScale();
        doubleClick(&freeScene);
        require(stack->currentWidget() == &freeScene && view->parallelScale() == afterDefaultZoom,
            "Frei in a small slot must swap back without resetting other camera");
        button->click();
    }
    require(sameCamera(freeCamera, freeScene.cameraView()),
        "bound interactions must leave free camera unchanged");
    // Frei remains a full free camera even in a small slot.
    doubleClick(views[1]);
    const auto beforeRotation = freeScene.cameraView();
    drag(&freeScene, Qt::LeftButton, Qt::NoModifier, {12., 10.});
    require(!sameCamera(beforeRotation, freeScene.cameraView()),
        "Frei must still rotate while in a small slot");
    require(freeScene.setCameraView(freeCamera), "restore free camera");
    doubleClick(views[2]);
    require(stack->currentWidget() == views[2], "swap must target current large view, not fixed Frei");
    labels();
    window.findChild<QPushButton*>(QStringLiteral("configureGraphics"))->click();
    stack->currentWidget()->findChild<QPushButton*>(QStringLiteral("closeGraphics"))->click();
    require(stack->currentWidget() == views[2], "graphics Back must restore currently enlarged view");
    require(window.grab().save(evidence.filePath(QStringLiteral("swapped_views.bmp")), "BMP"),
        "view swapping screenshot");
    doubleClick(&freeScene);
    // Restore the two permuted small views using only the actual double-click path.
    doubleClick(views[1]);
    doubleClick(views[2]);
    doubleClick(&freeScene);
    labels();
    require(stack->currentWidget() == &freeScene, "free view restored");
    testStaticViews(window, freeScene);
    // Leaving static mode must restore the real free scene for dynamic mode.
    doubleClick(views[1]);
    doubleClick(views[3]);
    auto* tabs = window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
    tabs->setCurrentIndex(2);
    QApplication::processEvents();
    require(stack->currentWidget() == &freeScene
            && freeScene.findChild<QLabel*>(QStringLiteral("viewName"))->isVisible(),
        "dynamic workspace must restore free scene and retain its view name");
    tabs->setCurrentIndex(1);
    QApplication::processEvents();
    testStaticViews(window, freeScene);
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
    const auto duration = window.findChild<QDoubleSpinBox*>(
        QStringLiteral("simulationDuration"))->value();
    auto* models = window.findChild<QListWidget*>(QStringLiteral("modelList"));
    require(models != nullptr, "model list for workspace identity check");
    const auto selectedModel = models->currentItem()->data(Qt::UserRole).toString();
    auto* vectors = window.findChild<QCheckBox*>(QStringLiteral("displayVectors"));
    auto* displayControls = window.findChild<QWidget*>(QStringLiteral("displayControls"));
    auto* staticSidebar = window.findChild<QScrollArea*>(QStringLiteral("staticDisplaySidebar"));
    auto* modelName = window.findChild<QLabel*>(QStringLiteral("modelName"));
    auto* title = window.findChild<QLabel*>(QStringLiteral("title"));
    require(vectors && displayControls && staticSidebar && modelName && title,
        "workspace display controls and external active-model title exist");
    const bool initialVectors = vectors->isChecked();
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
        require(models->isVisible() == (selected == 0)
            && staticSidebar->isVisible() == (selected > 0),
            "both simulation workspaces replace model list with display sidebar");
        require(staticSidebar->isAncestorOf(displayControls) == (selected > 0),
            "same complete display controls occupy the left in both simulation workspaces");
        require(modelName->font().pixelSize() == title->font().pixelSize()
            && modelName->font().pointSizeF() == title->font().pointSizeF()
            && modelName->alignment().testFlag(Qt::AlignRight),
            "active-model title matches application title size and is right aligned");
        const auto modelRect = QRect(modelName->mapToGlobal(QPoint()), modelName->size());
        const auto tabRect = QRect(tabs->mapToGlobal(QPoint()), tabs->size());
        require(std::abs(modelRect.center().y()-tabRect.center().y()) <= 1
            && modelRect.bottom() < scene.mapToGlobal(QPoint()).y(),
            "active-model title is outside panels on the workspace-tab row");
        require(vectors->isChecked() == initialVectors,
            "moving display controls preserves vector visibility selection");
        if (selected == 1) {
            auto* navigationColumn = window.findChild<QScrollArea*>(QStringLiteral("staticNavigationColumn"));
            auto* navigationToggle = window.findChild<QCheckBox*>(QStringLiteral("displayNavigation"));
            require(navigationToggle && !navigationToggle->isChecked()
                    && navigationColumn && !navigationColumn->isVisible(),
                "Static navigation defaults off.");
            navigationToggle->setChecked(true);
            QApplication::processEvents();
            require(navigationColumn && navigationColumn->isVisible()
                    && navigationColumn->widget()
                    && navigationColumn->widget()->objectName() == QStringLiteral("navigationOverlay")
                    && staticSidebar->mapToGlobal(QPoint()).x() + staticSidebar->width()
                        <= navigationColumn->mapToGlobal(QPoint()).x()
                    && navigationColumn->mapToGlobal(QPoint()).x() + navigationColumn->width()
                        <= scene.mapToGlobal(QPoint()).x(),
                "Static navigation must be a separate column between selection and main scene.");
            auto* turning = window.findChild<QCheckBox*>(QStringLiteral("stationaryTurning"));
            require(!turning && !window.findChild<QGroupBox*>(QStringLiteral("stationaryForcePanel"))
                    ->findChild<QComboBox*>(),
                "Stationary turning and model-fixed force are unconditional, not user switches.");
            auto* staticMenu = window.findChild<QWidget*>(QStringLiteral("staticForceMenu"));
            require(staticMenu && staticMenu->isVisible(),
                "Static resultant selection must be visible.");
            const auto staticDefaults = forcedisplay::staticForceDefaults();
            for (const auto& entry : staticDefaults) {
                auto* check = window.findChild<QCheckBox*>(QStringLiteral("staticForce_") + entry.key);
                require(check && check->isChecked() == entry.enabled,
                    "Static menu defaults must enable only the simple entries.");
            }
            require(staticSidebar->mapToGlobal(QPoint()).x() < scene.mapToGlobal(QPoint()).x()
                && displayControls->isVisible(),
                "static display controls occupy the left of the rendering viewport");
            vectors->setChecked(!initialVectors);
            require(!vectors->isVisible() && !scene.options().forces
                    && !scene.options().leverArm && scene.options().courseArrow
                    && scene.options().staticForces,
                "Static resultants must be independent of hidden dynamic vector settings.");
            vectors->setChecked(initialVectors);
            auto* graphicsButton = window.findChild<QPushButton*>(QStringLiteral("configureGraphics"));
            require(graphicsButton->height() >= graphicsButton->sizeHint().height(),
                "static graphics button text must not be clipped after hiding vector rows");
            auto* cameraCombo = window.findChild<QComboBox*>(QStringLiteral("cameraPreset"));
            require(cameraCombo->height() >= cameraCombo->sizeHint().height()
                    && cameraCombo->height() >= cameraCombo->fontMetrics().height() + 12,
                "camera preset must remain readable after reducing static controls");
            auto* navigation = window.findChild<QCheckBox*>(QStringLiteral("displayNavigation"));
            require(navigation->mapToGlobal(QPoint()).y() >=
                    cameraCombo->mapToGlobal(QPoint()).y() + cameraCombo->height(),
                "remaining static display controls must not overlap");
            const auto priorSize = window.size();
            for (const auto size : {QSize(1500, 940), QSize(1200, 800), QSize(1024, 680)}) {
                window.resize(size);
                QApplication::processEvents();
                testStaticViews(window, scene);
            }
            window.resize(priorSize);
            QApplication::processEvents();
        } else if (selected == 0) {
            require(!window.findChild<QWidget*>(QStringLiteral("staticViewsColumn"))->isVisible(),
                "auxiliary views must not appear in boat selection");
        } else {
            require(!window.findChild<QWidget*>(QStringLiteral("staticViewsColumn"))->isVisible()
                    && window.findChild<QCheckBox*>(QStringLiteral("displayNavigation"))->isChecked()
                    && !vectors->isVisible() && scene.options().staticForces,
                "dynamic workspace defaults to navigation on and analysis views off");
        }
        const auto* mode = window.findChild<QComboBox*>(QStringLiteral("simulationMode"));
        require(!mode->isVisible() && (selected == 0 || mode->currentData().toBool() == (selected == 2)),
            "workspace determines simulation mode without dropdown");
        auto* forcePanel = window.findChild<QGroupBox*>(QStringLiteral("stationaryForcePanel"));
        require(forcePanel && forcePanel->isVisible() == (selected > 0),
            "shared force configuration exists in both simulation workspaces");
        if (selected > 0) {
            auto* stop = window.findChild<QPushButton*>(QStringLiteral("stopSimulation"));
            require(forcePanel->mapToGlobal(QPoint()).y() >=
                stop->mapToGlobal(QPoint()).y()+stop->height(),
                "external force settings follow stop button");
            auto* movements = window.findChild<QGroupBox*>(QStringLiteral("movementPanel"));
            const auto fixed = window.findChildren<QDoubleSpinBox*>(
                QRegularExpression(QStringLiteral("^stationaryMovement_")));
            if (!fixed.isEmpty()) {
                require(movements && movements->isVisible()
                    && movements->mapToGlobal(QPoint()).y() >=
                        forcePanel->mapToGlobal(QPoint()).y()+forcePanel->height(),
                    "fixed rudder and centreboard settings follow external force");
                for (auto* setting : fixed)
                    require(setting->isVisible() && setting->isEnabled(),
                        "every stationary element setting is editable before run");
            }
        }
        require(window.findChild<QDoubleSpinBox*>(QStringLiteral("simulationDuration"))->value() == duration
            && models->currentItem()
            && models->currentItem()->data(Qt::UserRole).toString() == selectedModel,
            "workspace switch changed model or simulation inputs");
        const auto currentCamera = scene.cameraView();
        require(camera.position == currentCamera.position && camera.focalPoint == currentCamera.focalPoint
            && camera.viewUp == currentCamera.viewUp, "workspace switch changed camera");
        require(central->palette().color(QPalette::Window) == backgrounds[selected],
            "entire workspace must use selected theme");
        if (selected != 0) {
            auto* nav = window.findChild<QScrollArea*>(QStringLiteral("staticNavigationColumn"))->widget();
            auto* speed = nav->findChild<QWidget*>(QStringLiteral("navigationSpeed"));
            auto* magnitude = nav->findChild<QLabel*>(QStringLiteral("navigationSpeedMagnitude"));
            auto* rate = nav->findChild<QLabel*>(QStringLiteral("navigationCourseRate"));
            auto* axisX = nav->findChild<QLabel*>(QStringLiteral("navigationSpeedComponent0"));
            auto* axisZ = nav->findChild<QLabel*>(QStringLiteral("navigationSpeedComponent2"));
            require(speed && magnitude && rate && axisX && axisZ
                    && magnitude->y() < rate->y() && rate->y() < axisX->y()
                    && speed->rect().contains(rate->geometry())
                    && speed->rect().contains(axisZ->geometry()),
                "DPI layout: course rate below magnitude and above every unclipped world row");
            auto* viewport = scene.findChild<QVTKOpenGLNativeWidget*>();
            require(viewport && !viewport->mask().isEmpty()
                    && !viewport->mask().contains(QPoint(0, 0))
                    && viewport->mask().contains(viewport->rect().center()),
                "simulation viewport must share rounded inspection clipping");
            auto* forceX = window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"));
            auto* pointX = window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryPointX"));
            require(forceX && pointX && forceX->parentWidget() != pointX->parentWidget(),
                "single force vector and shared application point must form distinct groups");
            for (const auto& axis : {QStringLiteral("Y"), QStringLiteral("Z")}) {
                require(window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForce") + axis)
                            ->parentWidget() == forceX->parentWidget()
                        && window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryPoint") + axis)
                            ->parentWidget() == pointX->parentWidget(),
                    "XYZ components must belong to one vector and one point");
            }
            const bool wasChecked = vectors->isChecked();
            const auto whitePixels = [vectors] {
                QStyleOptionButton option;
                option.initFrom(vectors);
                option.state |= vectors->isChecked() ? QStyle::State_On : QStyle::State_Off;
                const auto rect = vectors->style()->subElementRect(
                    QStyle::SE_CheckBoxIndicator, &option, vectors);
                const auto image = vectors->grab(rect).toImage();
                int count = 0;
                for (int y = 0; y < image.height(); ++y)
                    for (int x = 0; x < image.width(); ++x) {
                        const auto color = image.pixelColor(x, y);
                        if (color.red() > 230 && color.green() > 230 && color.blue() > 230)
                            ++count;
                    }
                return count;
            };
            vectors->setChecked(false);
            const int uncheckedWhite = whitePixels();
            vectors->setChecked(true);
            require(whitePixels() > uncheckedWhite + 5,
                "checked checkbox must render a white checkmark, not only a colored fill");
            vectors->setChecked(wasChecked);
        }
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

int testInspectionCameraAndPointerRouting()
{
    VtkSceneWidget scene;
    scene.setInspectionMode();
    DisplayPart sailPart;
    sailPart.elementId = QStringLiteral("Großsegel");
    sailPart.mesh = vtkSmartPointer<vtkPolyData>::New();
    vtkNew<vtkPoints> points;
    points->InsertNextPoint(-1.0, -1.0, 0.0);
    points->InsertNextPoint(1.0, -1.0, 0.0);
    points->InsertNextPoint(0.0, 1.0, 0.0);
    vtkNew<vtkCellArray> triangles;
    const vtkIdType triangle[3] = {0, 1, 2};
    triangles->InsertNextCell(3, triangle);
    sailPart.mesh->SetPoints(points);
    sailPart.mesh->SetPolys(triangles);
    QString error;
    require(scene.setModel({sailPart}, &error),
        "synthetic triangular sail enters normal inspection rendering");
    scene.resize(620, 420);
    scene.show();
    QApplication::processEvents();
    scene.renderNow();
    const std::array<std::pair<QString, std::size_t>, 6> orthographicViews{{
        {QStringLiteral("backbord"), 1}, {QStringLiteral("steuerbord"), 1},
        {QStringLiteral("bug"), 0}, {QStringLiteral("heck"), 0},
        {QStringLiteral("oben"), 2}, {QStringLiteral("unten"), 2}}};
    std::array<bool, 3> translatedPlanes{};
    for (const auto& [view, normalAxis] : orthographicViews) {
        require(scene.setInspectionCameraPreset(view, nullptr)
            && scene.inspectionEditView() && scene.parallelProjection(),
            "each of the six model-axis inspection views is orthographic and editable");
        const auto horizontal = scene.inspectionWorldVectorForScreenDelta({20.0, 0.0});
        const auto vertical = scene.inspectionWorldVectorForScreenDelta({0.0, 15.0});
        require(std::abs(horizontal[normalAxis]) < 1e-10
                && std::abs(vertical[normalAxis]) < 1e-10,
            "axis-view pointer translation excludes its model-plane normal");
        require(forcedisplay::norm(horizontal) > 1e-10
                && forcedisplay::norm(vertical) > 1e-10
                && forcedisplay::norm(forcedisplay::cross(horizontal, vertical)) > 1e-10,
            "synthetic triangle pointer translation spans the active camera plane");
        translatedPlanes[normalAxis] = true;
        auto* panViewport = scene.findChild<QVTKOpenGLNativeWidget*>();
        require(panViewport, "inspection camera pan viewport exists");
        scene.renderNow();
        const forcedisplay::Vec3 referencePoint{0.0, 0.0, 0.0};
        const auto beforePan = scene.inspectionScreenPointForWorld(referencePoint);
        const QPointF start{30.0, 30.0};
        const QPointF movement{20.0, 15.0};
        const QPointF global = panViewport->mapToGlobal(start.toPoint());
        QMouseEvent panPress(QEvent::MouseButtonPress, start, global,
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent panMove(QEvent::MouseMove, start + movement, global + movement,
            Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent panRelease(QEvent::MouseButtonRelease, start + movement, global + movement,
            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(panViewport, &panPress);
        QApplication::sendEvent(panViewport, &panMove);
        QApplication::sendEvent(panViewport, &panRelease);
        scene.renderNow();
        const auto screenMovement =
            scene.inspectionScreenPointForWorld(referencePoint) - beforePan;
        require(std::abs(screenMovement.x() - movement.x()) < 0.1
                && std::abs(screenMovement.y() - movement.y()) < 0.1,
            "inspection background pan must follow the mouse horizontally and vertically");
    }
    require(scene.setInspectionCameraPreset(QStringLiteral("oben_backbord_bug"), nullptr)
        && !scene.inspectionEditView() && !scene.parallelProjection(),
        "oblique inspection presets remain view-only perspective views");
    require(scene.setInspectionCameraPreset(QString(), nullptr)
        && !scene.inspectionEditView(), "free camera remains view-only");
    require(scene.setInspectionCameraPreset(QStringLiteral("oben"), nullptr),
        "return to top-axis inspection view");
    auto* viewport = scene.findChild<QVTKOpenGLNativeWidget*>();
    require(viewport, "inspection interaction viewport exists");
    scene.setInspectionEditTarget(QStringLiteral("Großsegel"));
    scene.setInspectionHandles({{QStringLiteral("hals"), {0.0, 0.0, 0.0}}});
    scene.renderNow();
    const auto handlePoint = scene.inspectionScreenPointForWorld({0.0, 0.0, 0.0});
    const QPointF handleGlobal = viewport->mapToGlobal(handlePoint.toPoint());
    QVector<VtkSceneWidget::InspectionDragPhase> dragPhases;
    QString draggedElement, draggedHandle, contextElement, interactionHint;
    scene.onInspectionDrag = [&](const QString& element, const QString& handle,
        const QPointF&, VtkSceneWidget::InspectionDragPhase phase) {
        draggedElement = element;
        draggedHandle = handle;
        dragPhases.push_back(phase);
    };
    scene.onInspectionContextMenu = [&](const QString& element, const QPoint&) {
        contextElement = element;
    };
    scene.onInspectionInteractionHint = [&](const QString& hint) {
        interactionHint = hint;
    };
    QMouseEvent dragPress(QEvent::MouseButtonPress, handlePoint, handleGlobal,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent dragMove(QEvent::MouseMove, handlePoint + QPointF(25.0, 15.0),
        handleGlobal + QPointF(25.0, 15.0), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent dragRelease(QEvent::MouseButtonRelease, handlePoint + QPointF(25.0, 15.0),
        handleGlobal + QPointF(25.0, 15.0), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &dragPress);
    QApplication::sendEvent(viewport, &dragMove);
    QApplication::sendEvent(viewport, &dragRelease);
    require(dragPhases.size() == 3
        && dragPhases[0] == VtkSceneWidget::InspectionDragPhase::Start
        && dragPhases[1] == VtkSceneWidget::InspectionDragPhase::Move
        && dragPhases[2] == VtkSceneWidget::InspectionDragPhase::Finish
        && draggedElement == QStringLiteral("Großsegel")
        && draggedHandle == QStringLiteral("hals"),
        "DPI-aware handle routing emits one start/move/finish transaction");
    const std::array<std::pair<QString, forcedisplay::Vec3>, 4> syntheticCorners{{
        {QStringLiteral("hals"), {-0.75, -0.65, 0.0}},
        {QStringLiteral("schothorn"), {0.75, -0.65, 0.0}},
        {QStringLiteral("kopf_vorliek"), {0.0, 0.7, 0.0}},
        {QStringLiteral("kopf_achterliek"), {0.65, 0.45, 0.0}}}};
    for (std::size_t corner = 0; corner < syntheticCorners.size(); ++corner) {
        scene.setInspectionHandles({{syntheticCorners[corner].first,
            syntheticCorners[corner].second}});
        scene.renderNow();
        const auto cornerPoint = scene.inspectionScreenPointForWorld(
            syntheticCorners[corner].second);
        const auto cornerGlobal = viewport->mapToGlobal(cornerPoint.toPoint());
        dragPhases.clear();
        QMouseEvent cornerPress(QEvent::MouseButtonPress, cornerPoint, cornerGlobal,
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent cornerMove(QEvent::MouseMove, cornerPoint + QPointF(13.0, 9.0),
            cornerGlobal + QPointF(13.0, 9.0), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent cornerRelease(QEvent::MouseButtonRelease,
            cornerPoint + QPointF(13.0, 9.0), cornerGlobal + QPointF(13.0, 9.0),
            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(viewport, &cornerPress);
        QApplication::sendEvent(viewport, &cornerMove);
        QApplication::sendEvent(viewport, &cornerRelease);
        require(dragPhases.size() == 3 && draggedHandle == syntheticCorners[corner].first
                && dragPhases.front() == VtkSceneWidget::InspectionDragPhase::Start
                && dragPhases.back() == VtkSceneWidget::InspectionDragPhase::Finish,
            corner < 3
                ? "synthetic triangular sail dispatches each semantic corner handle"
                : "synthetic quadrilateral sail dispatches its fourth semantic corner");
    }
    scene.setInspectionHandles({});
    int interactionCancellations = 0;
    scene.onInspectionInteractionCancelled = [&] { ++interactionCancellations; };
    dragPhases.clear();
    const QPointF cancelPoint = scene.inspectionScreenPointForWorld({0.1, 0.1, 0.0});
    const QPointF cancelGlobal = viewport->mapToGlobal(cancelPoint.toPoint());
    QMouseEvent cancelPress(QEvent::MouseButtonPress, cancelPoint, cancelGlobal,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent cancelMove(QEvent::MouseMove, cancelPoint + QPointF(20.0, 12.0),
        cancelGlobal + QPointF(20.0, 12.0), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &cancelPress);
    QApplication::sendEvent(viewport, &cancelMove);
    QEvent ungrab(QEvent::UngrabMouse);
    QApplication::sendEvent(viewport, &ungrab);
    QMouseEvent cancelledRelease(QEvent::MouseButtonRelease,
        cancelPoint + QPointF(20.0, 12.0), cancelGlobal + QPointF(20.0, 12.0),
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &cancelledRelease);
    require(interactionCancellations == 1
        && dragPhases.size() == 2
        && dragPhases.back() == VtkSceneWidget::InspectionDragPhase::Move,
        "mouse ungrab cancels the in-progress GUI drag without a later Finish phase");

    QMouseEvent hidePress(QEvent::MouseButtonPress, cancelPoint, cancelGlobal,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent hideMove(QEvent::MouseMove, cancelPoint + QPointF(18.0, 11.0),
        cancelGlobal + QPointF(18.0, 11.0), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &hidePress);
    QApplication::sendEvent(viewport, &hideMove);
    viewport->hide();
    QApplication::processEvents();
    QMouseEvent hiddenRelease(QEvent::MouseButtonRelease,
        cancelPoint + QPointF(18.0, 11.0), cancelGlobal + QPointF(18.0, 11.0),
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &hiddenRelease);
    require(interactionCancellations == 2
        && dragPhases.back() == VtkSceneWidget::InspectionDragPhase::Move,
        "hiding the inspection viewport cancels without committing the preview");
    viewport->show();
    QApplication::processEvents();
    scene.renderNow();
    scene.setInspectionHandles({
        {QStringLiteral("hals"), {0.0, 0.0, 0.0}},
        {QStringLiteral("schothorn"), {0.0, 0.0, 0.0}}});
    dragPhases.clear();
    const QPointF ambiguousPoint = scene.inspectionScreenPointForWorld({0.0, 0.0, 0.0});
    const QPointF ambiguousGlobal = viewport->mapToGlobal(ambiguousPoint.toPoint());
    QMouseEvent ambiguousPress(QEvent::MouseButtonPress, ambiguousPoint, ambiguousGlobal,
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent ambiguousRelease(QEvent::MouseButtonRelease, ambiguousPoint, ambiguousGlobal,
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &ambiguousPress);
    QApplication::sendEvent(viewport, &ambiguousRelease);
    require(dragPhases.isEmpty()
        && interactionHint.contains(QStringLiteral("andere Achsansicht")),
        "overlapping projected handles are rejected with an alternate-view hint");
    const QPointF bodyPoint = scene.inspectionScreenPointForWorld({0.1, 0.1, 0.0});
    const QPointF bodyGlobal = viewport->mapToGlobal(bodyPoint.toPoint());
    QMouseEvent menuPress(QEvent::MouseButtonPress, bodyPoint, bodyGlobal,
        Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QMouseEvent menuRelease(QEvent::MouseButtonRelease, bodyPoint, bodyGlobal,
        Qt::RightButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &menuPress);
    QApplication::sendEvent(viewport, &menuRelease);
    require(contextElement == QStringLiteral("Großsegel"),
        "right-click picking reports the exact displayed sail ID");
    scene.setInspectionEditTarget({});
    scene.setInspectionHandles({});
    scene.hide();
    return static_cast<int>(std::count(translatedPlanes.begin(), translatedPlanes.end(), true));
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
    auto* inspectionCameraCombo = window.findChild<QComboBox*>(
        QStringLiteral("inspectionCameraPreset"));
    require(inspectionCameraCombo && inspectionCameraCombo->isEnabled(),
        "boot inspection exposes its camera selector above model selection");
    testInspectionCameraAndPointerRouting();
    report.insert(QStringLiteral("inspectionCameraAxesAndPointerRouting"), true);
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
    if (!(visible > 0 && visible < rootValues)) {
        int movementRows = 0;
        QStringList areas;
        for (int i = 0; i < values->topLevelItemCount(); ++i) {
            const auto area = values->topLevelItem(i)->text(1);
            if (area == QStringLiteral("Bewegung")) ++movementRows;
            if (!areas.contains(area)) areas.push_back(area);
        }
        throw std::runtime_error(("filter changes actual visible rows: total="
            + std::to_string(rootValues) + ", visible=" + std::to_string(visible)
            + ", Bewegung=" + std::to_string(movementRows)
            + ", property areas=" + areas.join(QStringLiteral(", ")).toStdString()).c_str());
    }
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
    QSet<QString> elementIds;
    QSet<QString> sourceIds;
    int geometryNodes = 0;
    int movementPreviews = 0;
    bool derivedGeometry = false;
    QTreeWidgetItem* firstGeometry = nullptr;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        auto* item = *it;
        if (item->text(0) == QStringLiteral("Abgeleitete Daten") && item->childCount() > 0)
            derivedGeometry = true;
        if (item->data(0, Qt::UserRole + 2).isValid()) {
            ++geometryNodes;
            require(!item->toolTip(0).isEmpty(), "geometry provenance is visible");
            if (!firstGeometry) firstGeometry = item;
            if (item->text(0).startsWith(QStringLiteral("Quellmesh"))) {
                const auto key = item->parent()->text(0) + item->text(0);
                require(!sourceIds.contains(key), "source reused by roles is listed only once");
                sourceIds.insert(key);
            }
        }
        if (item->data(0, Qt::UserRole + 4).toString() != QStringLiteral("element")) continue;
        const auto ids = item->data(0, Qt::UserRole + 1).toStringList();
        require(ids.size() == 1 && !elementIds.contains(ids.front()),
            "each existing object appears exactly once");
        elementIds.insert(ids.front());
        tree->clearSelection();
        item->setSelected(true);
        QApplication::processEvents();
        const auto camera = inspectionScene->cameraView();
        require(camera.position == inspectionCamera.position
                && camera.focalPoint == inspectionCamera.focalPoint,
            "current object selection preserves camera");
    }
    require(!elementIds.isEmpty(), "canonical object tree retains model elements");
    require(geometryNodes > 0 && !sourceIds.isEmpty(),
        "source geometry nodes are visible under their model elements");
    require(derivedGeometry, "derived geometry appears in its own element branch");
    require(window.grab().save(evidence.filePath(QStringLiteral("boat_unified_source.bmp")), "BMP"),
        "current object model screenshot");
    tree->clearSelection();
    require(firstGeometry != nullptr, "selectable geometry node exists");
    firstGeometry->setSelected(true);
    QApplication::processEvents();
    inspectionScene->renderNow();
    require(inspectionScene->property("hasInspectionMesh").toBool(),
        "selected source geometry remains renderable");
    const auto geometryCamera = inspectionScene->cameraView();
    require(geometryCamera.position == inspectionCamera.position
            && geometryCamera.focalPoint == inspectionCamera.focalPoint,
        "geometry selection preserves camera");
    firstGeometry->setCheckState(0, Qt::Checked);
    require(inspectionScene->property("hasInspectionMesh").toBool(),
        "checked geometry remains renderable without a selected node");
    firstGeometry->setCheckState(0, Qt::Unchecked);
    tree->clearSelection();
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        const auto id = (*it)->data(0, Qt::UserRole + 6).toString();
        if (id.isEmpty()) continue;
        ++movementPreviews;
        const auto previewCamera = inspectionScene->cameraView();
        toggleMovementContextAction(tree, *it, true);
        inspectionScene->renderNow();
        require(inspectionScene->property("inspectionMovementPreviewCount").toInt() > 0,
            "checked movement preview is visible without changing simulation");
        require(inspectionScene->cameraView().position == previewCamera.position
                && inspectionScene->cameraView().focalPoint == previewCamera.focalPoint,
            "movement preview preserves inspection camera");
        toggleMovementContextAction(tree, *it, false);
        require(inspectionScene->property("inspectionMovementPreviewCount").toInt() == 0,
            "unchecking preview removes only temporary movement graphics");
    }
    require(movementPreviews > 0, "moving elements expose a preview in the common object tree");
    const auto simulationCamera = simulationScene->cameraView();
    testWorkspaceTabs(window, *simulationScene, evidence, report);
    tabs->setCurrentIndex(0);
    require(simulationScene->cameraView().position == simulationCamera.position,
        "inspection never changes simulation camera");
    const auto treeBefore = tree->topLevelItemCount();
    QFile changed(copy.filePath(QStringLiteral("modells_information.json")));
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
    report.insert(QStringLiteral("boatInspectionTreesMeshesValuesFixedSnapshotAndLayout"), true);
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
    auto* profiles = window.findChild<QPushButton*>(QStringLiteral("configureGraphics"));
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
    auto* navigationColumn = window.findChild<QScrollArea*>(QStringLiteral("staticNavigationColumn"));
    auto* overlay = navigationColumn->findChild<QWidget*>(QStringLiteral("navigationOverlay"));
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
    require(!navigationColumn->isVisible(), "Hiding static navigation must release its entire column.");
    navigationCheck->setChecked(true);
    require(overlay->isVisible(), "navigation visibility on");
    require(navigationColumn->isVisible(), "Navigation switch must restore its static column.");
    vectors->setChecked(false);
    require(profiles->isEnabled(), "hidden vectors disable profile selection");
    vectors->setChecked(true);
    auto* fixedForcePanel = window.findChild<QGroupBox*>(QStringLiteral("stationaryForcePanel"));
    require(fixedForcePanel && fixedForcePanel->isVisible(), "static run force panel visible");
    run->click();
    require(!fixedForcePanel->isEnabled(), "stationary inputs locked immediately for immutable run");
    waitUntil([&] { return !stop->isEnabled() || unexpectedDialog; }, "stationary GUI result timeout");
    require(!unexpectedDialog, "unexpected stationary GUI error");
    QTreeWidgetItem* stationaryResultItem = nullptr;
    for (QTreeWidgetItemIterator it(values); *it; ++it)
        if ((*it)->text(0) == QStringLiteral("Stationäre Auswertung (Core)"))
            stationaryResultItem = *it;
    require(stationaryResultItem, "real stationary Core result displayed through V5 GUI force parser");
    auto* courseRateLabel = overlay->findChild<QLabel*>(QStringLiteral("navigationCourseRate"));
    require(courseRateLabel && courseRateLabel->text() == QStringLiteral("0.0 °/s"),
        "stationary V6 zero course rate must be shown as numeric zero");
    scene->renderNow();
    require(scene->courseArrowGeometry().has_value(),
        "Zero-rate static result must render the straight direction arrow like Frei.");
    testStaticViews(window, *scene);
    navigationCheck->setChecked(false);
    scene->renderNow();
    require(scene->courseArrowGeometry().has_value(),
        "Static direction arrow must be independent of navigation visibility.");
    navigationCheck->setChecked(true);
    scene->renderNow();
    require(scene->courseArrowGeometry().has_value(),
        "Navigation visibility must not change the static direction arrow.");
    const QStringList stationaryFields{
        QStringLiteral("residuum_kraft_welt_n"), QStringLiteral("residuum_moment_modell_nm"),
        QStringLiteral("krafttoleranz_n"), QStringLiteral("momenttoleranz_nm"),
        QStringLiteral("referenzkurs_verwendet"), QStringLiteral("geschwindigkeit_relativ_wasser_m_s"),
        QStringLiteral("abdriftwinkel_rad"), QStringLiteral("stellungen"), QStringLiteral("kraft_n"),
        QStringLiteral("kurvenfahrt"), QStringLiteral("loesungstyp"), QStringLiteral("omega_modell_rad_s"),
        QStringLiteral("aeussere_kraft_welt_n"), QStringLiteral("aeusseres_moment_modell_nm"),
        QStringLiteral("radius_status")};
    for (const auto& name : stationaryFields) {
        bool present = false;
        for (int i = 0; i < stationaryResultItem->childCount(); ++i)
            present = present || stationaryResultItem->child(i)->text(0) == name;
        require(present, "stationary output omitted a required Core field");
    }
    values->scrollToItem(stationaryResultItem, QAbstractItemView::PositionAtTop);
    scene->renderNow();
    require(window.grab().save(evidence.filePath(QStringLiteral("stationary_result.bmp")), "BMP"),
        "stationary UI result screenshot");
    auto* fixedForceX = window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"));
    require(fixedForceX && fixedForcePanel->isEnabled(), "stationary inputs unlock after result");
    fixedForceX->setValue(1.);
    require(status->text().contains(QStringLiteral("vorherigen Rechenfall")),
        "editing stationary inputs marks previous result stale");
    fixedForceX->setValue(0.);
    window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"))->setCurrentIndex(2);
    duration->setValue(2.0);
    waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "GUI mode change readiness");
    confirmDynamicRun(window, 2.);
    run->click();
    waitUntil([&] { return slider->maximum() > 2 || unexpectedDialog; }, "GUI live states absent");
    require(!unexpectedDialog, "unexpected live error");
    require(!courseRateLabel->text().startsWith(QStringLiteral("--")),
        "dynamic V6 must supply the same course-rate display contract");
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
        require(slider->value() == 0 && slider->maximum() == 0
                && !scene->courseArrowGeometry() && scene->motionStateCount() == 0,
            "workspace switch must clear playback, heading arrow and motion history");
        if (index != 0)
            require(mode->currentIndex() == (index == 2 ? 1 : 0),
                "workspace must select its own simulation mode");
    }
    workspaceTabs->setCurrentIndex(2);
    waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "tab switch cancellation timeout");
    require(!unexpectedDialog && slider->maximum() == 0 && values->topLevelItemCount() == 0,
        "cancelled live run must not repopulate the new workspace");
    confirmDynamicRun(window, 2.);
    run->click();
    waitUntil([&] { return slider->maximum() > 2 || unexpectedDialog; }, "fresh live run after tab reset");
    require(!unexpectedDialog, "fresh dynamic run failed");
    slider->setValue(0);
    const auto camera = scene->cameraView();
    auto* compassWidget = navigationColumn->findChild<QWidget*>(QStringLiteral("navigation_compass"));
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
    values = window.findChild<QTreeWidget*>(QStringLiteral("staticForceValues"));
    QStringList before;
    for (QTreeWidgetItemIterator item(values); *item; ++item) {
        for (int column = 0; column < values->columnCount(); ++column) before << (*item)->text(column);
    }
    require(!before.isEmpty(), "GUI force table empty");
    require(profiles->isVisible(), "Dynamic settings must use the shared display configuration entry.");
    profiles->click();
    auto* settingsStack = window.findChild<QStackedWidget*>(QStringLiteral("sceneStack"));
    require(settingsStack->currentWidget() != scene, "Dynamic must open its own graphics configuration.");
    settingsStack->currentWidget()->findChild<QPushButton*>(QStringLiteral("closeGraphics"))->click();
    require(settingsStack->currentWidget() == scene, "Dynamic settings must return to the large view.");
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
    for (auto* action : window.menuBar()->actions()) {
        if (action->text() == QStringLiteral("Setup") && action->menu()) {
            for (auto* entry : action->menu()->actions())
                require(!entry->text().startsWith(QStringLiteral("Grafik"))
                    && !entry->text().startsWith(QStringLiteral("Materialvorschau")),
                    "Graphics must no longer be reachable through Setup.");
        }
    }
    stop->click();
    waitUntil([&] { return !stop->isEnabled() || unexpectedDialog; }, "GUI cancellation timeout");
    if (unexpectedDialog || !status->text().contains(QStringLiteral("abgebrochen")))
        throw std::runtime_error("GUI cancellation did not preserve partial status: "
            + status->text().toStdString());
    require(slider->maximum() > 0, "GUI cancellation lost recorded states");
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
    const auto savedGraphics = QSettings().value(QStringLiteral("grafik/profile_v1"));
    const auto dynamicProfile = scene->graphicsProfile();
    workspaceTabs->setCurrentIndex(1);
    require(profiles->isVisible(), "Static workspace must expose its configuration.");
    profiles->click();
    auto* staticPage = settingsStack->currentWidget();
    require(staticPage->objectName() == QStringLiteral("graphicsSettingsPage")
        && !scene->isVisible() && !staticPage->findChild<QTabWidget*>(),
        "Only technique table must replace the static boat.");
    auto* staticWater = staticPage->findChild<QCheckBox*>(QStringLiteral("profile0_water"));
    const bool savedWater = !staticWater->isChecked();
    staticWater->toggle();
    require(scene->graphicsProfile().on(graphics::Feature::Water) == savedWater,
        "Static checkbox must immediately change the actual scene profile.");
    const auto staticData = QSettings().value(QStringLiteral("statisch/grafik/profile_v1"));
    require(staticData.isValid() && QSettings().value(QStringLiteral("grafik/profile_v1")) == savedGraphics,
        "Static immediate saving must preserve dynamic data.");
    const auto originalSize = window.size();
    window.resize(1280, 720);
    QApplication::processEvents();
    auto* back = staticPage->findChild<QPushButton*>(QStringLiteral("closeGraphics"));
    require(back->isVisible()
        && staticPage->rect().contains(QRect(back->mapTo(staticPage, QPoint()), back->size())),
        "Back button must remain reachable at compact size and every DPI.");
    window.resize(originalSize);
    QApplication::processEvents();
    require(window.grab().save(evidence.filePath(QStringLiteral("graphics_configuration.bmp")), "BMP"),
        "static table screenshot");
    workspaceTabs->setCurrentIndex(2);
    require(settingsStack->currentWidget() == scene
        && QSettings().value(QStringLiteral("grafik/profile_v1")) == savedGraphics
        && QSettings().value(QStringLiteral("statisch/grafik/profile_v1")) == staticData,
        "Workspace switch must retain immediately saved static settings and preserve dynamic.");
    require(scene->graphicsProfile().enabled == dynamicProfile.enabled
        && scene->graphicsProfile().bodyOpacity == dynamicProfile.bodyOpacity
        && scene->graphicsProfile().throughBody == dynamicProfile.throughBody,
        "Dynamic rendering must restore its unchanged previous profile.");
    workspaceTabs->setCurrentIndex(1);
    profiles->click();
    window.selectInitialModel(QStringLiteral("420"));
    require(settingsStack->currentWidget() == scene
        && QSettings().value(QStringLiteral("grafik/profile_v1")) == savedGraphics,
        "Model switch must close configuration without changing dynamic settings.");
    auto* persistedSail = window.findChild<QCheckBox*>(QStringLiteral("displaySailCenter"));
    auto* persistedLateral = window.findChild<QCheckBox*>(QStringLiteral("displayLateralCenter"));
    persistedSail->setChecked(false);
    persistedLateral->setChecked(false);
    navigationCheck->setChecked(false);
    window.close();
    require(QSettings().value(QStringLiteral("statisch/navigation")).toBool() == false,
        "navigation visibility must be saved for the active workspace");
    MainWindow reopened;
    reopened.findChild<QTabBar*>(QStringLiteral("workspaceTabs"))->setCurrentIndex(1);
    auto* restoredNavigation = reopened.findChild<QCheckBox*>(QStringLiteral("displayNavigation"));
    require(restoredNavigation && !restoredNavigation->isChecked(),
        "navigation visibility must be restored in the next window");
    require(!reopened.findChild<QCheckBox*>(QStringLiteral("displaySailCenter"))->isChecked()
        && !reopened.findChild<QCheckBox*>(QStringLiteral("displayLateralCenter"))->isChecked(),
        "Both pressure-point visibility switches must persist independently.");
    reopened.findChild<QTabBar*>(QStringLiteral("workspaceTabs"))->setCurrentIndex(1);
    reopened.findChild<QPushButton*>(QStringLiteral("configureGraphics"))->click();
    auto* restoredSettings = reopened.findChild<QStackedWidget*>(QStringLiteral("sceneStack"))->currentWidget();
    require(restoredSettings->findChild<QCheckBox*>(QStringLiteral("profile0_water"))->isChecked() == savedWater
        && QSettings().value(QStringLiteral("grafik/profile_v1")) == savedGraphics,
        "Reopening must restore immediate static settings without touching dynamic.");
    restoredSettings->findChild<QCheckBox*>(QStringLiteral("profile0_water"))->setChecked(true);
    restoredSettings->findChild<QPushButton*>(QStringLiteral("closeGraphics"))->click();
    reopened.close();

    MainWindow movementWindow;
    movementWindow.findChild<QCheckBox*>(QStringLiteral("displaySailCenter"))->setChecked(true);
    movementWindow.findChild<QCheckBox*>(QStringLiteral("displayLateralCenter"))->setChecked(true);
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
    auto* fixedRudderInput = movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryMovement_Ruder"));
    auto* fixedSwordInput = movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryMovement_Schwert"));
    fixedRudderInput->setValue(20.);
    fixedSwordInput->setValue(50.);
    confirmDynamicRun(movementWindow, 0.);
    movementRun->click();
    waitUntil([&] { return movementStop->isEnabled() || unexpectedDialog; },
        "dynamic run did not enable the stop control");
    require(!unexpectedDialog, "unexpected dynamic start error");
    auto* movementTimeline = movementWindow.findChild<QSlider*>();
    waitUntil([&] { return movementTimeline->maximum() > 0 || unexpectedDialog; },
        "fixed-movement run did not publish its first Core states");
    require(!unexpectedDialog, "unexpected movement initialization error");
    require(!rudder->isVisible() && !sword->isVisible()
            && fixedRudderInput->isEnabled() && fixedSwordInput->isEnabled()
            && fixedRudderInput->value() == 20. && fixedSwordInput->value() == 50.,
        "dynamic numeric settings must remain editable and legacy live sliders hidden");
    movementStop->click();
    waitUntil([&] { return !movementStop->isEnabled() || unexpectedDialog; },
        "live movement simulation cancellation timeout");
    require(!unexpectedDialog, "unexpected movement cancellation error");
    movementWindow.findChild<QTabBar*>(QStringLiteral("workspaceTabs"))->setCurrentIndex(1);
    movementWindow.findChild<QCheckBox*>(QStringLiteral("stationaryForceActive"))->setChecked(true);
    movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"))->setValue(100.);
    movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryPointX"))->setValue(1.);
    movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryMovement_Ruder"))->setValue(0.);
    movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryMovement_Schwert"))->setValue(100.);
    auto* movementScene = dynamic_cast<VtkSceneWidget*>(
        movementWindow.findChild<QWidget*>(QStringLiteral("simulationScene")));
    require(movementScene, "420 simulation scene required");
    auto* turningRate = movementWindow.findChild<QScrollArea*>(QStringLiteral("staticNavigationColumn"))
        ->widget()->findChild<QLabel*>(QStringLiteral("navigationCourseRate"));
    require(turningRate, "Heading-rate display must exist with unconditional turning.");
    movementWindow.findChild<QCheckBox*>(QStringLiteral("displayNavigation"))->setChecked(true);
    require(turningRate->isVisible(), "turning screenshot must show the actual navigation rate");
    for (double fixedRudder : {1., -1., 24.}) {
        if (fixedRudder == 24.) {
            movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"))->setValue(150.);
            movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceY"))->setValue(10.);
            movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryPointX"))->setValue(-.05);
            movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryPointZ"))->setValue(3.);
        }
        movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryMovement_Ruder"))->setValue(fixedRudder);
        movementRun->click();
        require(!movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"))->isEnabled(),
            "Model-fixed force is part of the immutable running case.");
        waitUntil([&] { return !movementStop->isEnabled() || unexpectedDialog; },
            "420 turning GUI solve timeout");
        require(!unexpectedDialog, "real 420 turning GUI case must converge without a warning");
        if (fixedRudder == 1.) {
            const auto priorCamera = movementScene->cameraView();
            auto* resultValues = movementWindow.findChild<QTreeWidget*>(QStringLiteral("simulationValues"));
            const auto readValues = [resultValues] {
                QStringList result;
                for (QTreeWidgetItemIterator item(resultValues); *item; ++item)
                    for (int column = 0; column < resultValues->columnCount(); ++column)
                        result << (*item)->text(column);
                return result;
            };
            const auto priorValues = readValues();
            const auto priorRate = turningRate->text();
            const auto bodyStatus = movementScene->graphicsFeatureStatus(graphics::Feature::BodyTransparency);
            require(bodyStatus == QStringLiteral("Ausgeschaltet."),
                "The opaque static default must report analysis transparency as off.");
            require(movementScene->graphicsFeatureStatus(graphics::Feature::Reflection)
                    == QStringLiteral("Ausgeschaltet."),
                "Disabled effects must be reported as off, not active.");
            const auto dynamicData = QSettings().value(QStringLiteral("grafik/profile_v1"));
            movementWindow.findChild<QPushButton*>(QStringLiteral("configureGraphics"))->click();
            auto* stack = movementWindow.findChild<QStackedWidget*>(QStringLiteral("sceneStack"));
            auto* page = stack->currentWidget();
            require(!page->findChild<QCheckBox*>(QStringLiteral("profile0_throughBody")),
                "static graphics page must not retain hidden force/vector configuration");
            auto* transparency = page->findChild<QCheckBox*>(QStringLiteral("profile0_bodyTransparency"));
            require(transparency && !transparency->isChecked(),
                "The static default must not enable analysis transparency.");
            transparency->setChecked(true);
            require(movementScene->graphicsFeatureStatus(graphics::Feature::BodyTransparency)
                    .contains(QStringLiteral("Körperdeckkraft")),
                "Enabled analysis transparency must explicitly report its applied opacity.");
            transparency->setChecked(false);
            auto* grid = page->findChild<QCheckBox*>(QStringLiteral("profile0_grid"));
            const bool wasGrid = grid->isChecked();
            grid->toggle();
            require(movementScene->graphicsProfile().on(graphics::Feature::Grid) != wasGrid,
                "Immediate change must update the rendered static profile.");
            testStaticViews(movementWindow, *movementScene);
            require(page->findChild<QLabel*>(QStringLiteral("profile0_status_grid"))->text()
                .contains(wasGrid ? QStringLiteral("Ausgeschaltet.")
                                  : QStringLiteral("Aktiviert;")),
                "Table status must match the renderer immediately after each setting.");
            grid->toggle();
            require(movementScene->graphicsProfile().on(graphics::Feature::Grid) == wasGrid
                && readValues() == priorValues && turningRate->text() == priorRate
                && movementScene->cameraView().position == priorCamera.position
                && movementScene->cameraView().focalPoint == priorCamera.focalPoint
                && QSettings().value(QStringLiteral("grafik/profile_v1")) == dynamicData,
                "Static auto-save must preserve result, pose/camera and unrelated dynamic settings.");
            require(movementWindow.grab().save(evidence.filePath(QStringLiteral("graphics_configuration.bmp")), "BMP"),
                "real static result with technique table screenshot");
            page->findChild<QPushButton*>(QStringLiteral("closeGraphics"))->click();
            require(stack->currentWidget() == movementScene,
                "Back must return to the same static result.");
        }
        require(turningRate->text().startsWith(fixedRudder > 0. ? QLatin1Char('-') : QLatin1Char('+'))
                && !turningRate->text().contains(QStringLiteral("≈")),
            "actual positive/negative 420 course rates must be signed and converged");
        movementScene->renderNow();
        require(movementScene->courseArrowGeometry().has_value(),
            "Static Frei must show the shared direction arrow.");
        if (fixedRudder == 24.)
            require(turningRate->text().contains(QStringLiteral("-17."))
                    && movementWindow.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryMovement_Ruder"))
                        ->value() == 24.,
                "24-degree screenshot case must converge on the correct branch without adjusting rudder");
        require(movementWindow.findChild<QLabel*>(QStringLiteral("balance"))->text()
                .contains(QStringLiteral("Bewegungsresiduum Welt")),
            "turning result must display motion residuals, not merely external force sums");
        movementScene->renderNow();
        testStaticViews(movementWindow, *movementScene);
        require(movementScene->pressurePoint(true).has_value(),
            "Active real Core force must render its replacement SDP.");
        require(movementScene->pressurePoint(false).has_value(),
            "Real 420 turning side force must render the corrected LDP.");
        auto* navigationToggle = movementWindow.findChild<QCheckBox*>(QStringLiteral("displayNavigation"));
        navigationToggle->setChecked(false);
        movementScene->renderNow();
        require(movementScene->pressurePoint(true) && movementScene->pressurePoint(false),
            "Pressure points must be independent of navigation visibility.");
        navigationToggle->setChecked(true);
        auto* sailToggle = movementWindow.findChild<QCheckBox*>(QStringLiteral("displaySailCenter"));
        sailToggle->setChecked(false);
        movementScene->renderNow();
        require(!sailToggle->isVisible()
                && movementScene->pressurePoint(true) && movementScene->pressurePoint(false),
            "static pressure markers must not inherit hidden dynamic SDP settings");
        sailToggle->setChecked(true);
        auto* lateralToggle = movementWindow.findChild<QCheckBox*>(QStringLiteral("displayLateralCenter"));
        lateralToggle->setChecked(false);
        movementScene->renderNow();
        require(!lateralToggle->isVisible()
                && movementScene->pressurePoint(true) && movementScene->pressurePoint(false),
            "static pressure markers must not inherit hidden dynamic LDP settings");
        lateralToggle->setChecked(true);
        movementScene->renderNow();
        const auto pressureImage = movementScene->captureImage();
        int lightGreenPixels = 0, darkGreenPixels = 0;
        for (int y = 0; y < pressureImage.height(); ++y)
            for (int x = 0; x < pressureImage.width(); ++x) {
                const auto color = pressureImage.pixelColor(x, y);
                if (color.green() > 150 && color.red() < 140 && color.blue() < 150) ++lightGreenPixels;
                if (color.green() > 65 && color.green() < 140 && color.red() < 65
                    && color.blue() < 100) ++darkGreenPixels;
            }
        require(lightGreenPixels > 10 && darkGreenPixels > 10,
            "Both pressure markers/labels must actually be visible in the analysis raster.");
        require(movementWindow.grab().save(evidence.filePath(
                fixedRudder == 24. ? QStringLiteral("turning_rudder_24.bmp")
                    : fixedRudder > 0. ? QStringLiteral("turning_positive_rudder.bmp")
                            : QStringLiteral("turning_negative_rudder.bmp")), "BMP"),
            "actual signed turning display screenshot");
        if (fixedRudder == 24.) {
            testStaticForceMenu(movementWindow, *movementScene, evidence);
            testViewInteraction(movementWindow, *movementScene, evidence);
        }
    }
    errors.stop();
    movementRun->click();
    auto* movementTabs = movementWindow.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
    movementTabs->setCurrentIndex(0);
    movementTabs->setCurrentIndex(1);
    waitUntil([&] { return movementRun->isEnabled() || unexpectedDialog; },
        "stationary calculation did not stop after workspace change");
    require(!unexpectedDialog && !movementScene->courseArrowGeometry()
            && !movementScene->pressurePoint(true) && !movementScene->pressurePoint(false)
            && turningRate->text() == QStringLiteral("-- °/s"),
        "late stationary result must not restore a discarded state");
    testStaticViews(movementWindow, *movementScene);
    QJsonObject markerState{
        {QStringLiteral("position_m"), QJsonArray{2., -3., .4}},
        {QStringLiteral("orientierung_xyzw"), QJsonArray{.25, -.15, .3, std::sqrt(.825)}},
        {QStringLiteral("geschwindigkeit_welt_m_s"), QJsonArray{0., 0., 0.}}};
    forcedisplay::ForceState markerForces;
    markerForces.lateral = forcedisplay::LateralCenter{
        QStringLiteral("gueltig"), forcedisplay::Vec3{9., 0., -.6}, 2};
    markerForces.sailCenter = forcedisplay::SailCenter{
        QStringLiteral("gueltig"), forcedisplay::Vec3{.5, .2, 3.}};
    markerForces.reference = forcedisplay::Vec3{0., 0., 0.};
    markerForces.stationary.insert(QStringLiteral("konvergiert"), true);
    markerForces.balanceStatus = QStringLiteral("vollstaendig");
    markerForces.total = forcedisplay::GroupSum{};
    markerForces.total->moment = forcedisplay::Vec3{-8., 17., -26.};
    markerForces.lateral->transverseForce = -80.;
    forcedisplay::Contribution drive;
    drive.id = QStringLiteral("fixture:external");
    drive.source = QStringLiteral("externe_kraft");
    drive.frame = QStringLiteral("modell");
    drive.state = QStringLiteral("gueltig");
    drive.vector = forcedisplay::Vec3{1., 2., 3.};
    drive.momentS = forcedisplay::Vec3{12., -23., 34.};
    markerForces.contributions.push_back(drive);
    auto counter = drive;
    counter.id = QStringLiteral("fixture:counter");
    counter.source = QStringLiteral("hydrostatik");
    counter.momentS = forcedisplay::Vec3{10000., 20000., 30000.};
    markerForces.contributions.push_back(counter);
    auto vertical = drive;
    vertical.frame = QStringLiteral("welt");
    vertical.point = forcedisplay::Vec3{2., -3., .4};
    vertical.id = QStringLiteral("fixture:weight");
    vertical.source = QStringLiteral("schwerkraft");
    vertical.category = forcedisplay::Category::Weight;
    vertical.vector = forcedisplay::Vec3{0., 0., -500.};
    markerForces.contributions.push_back(vertical);
    vertical.id = QStringLiteral("fixture:buoyancy");
    vertical.source = QStringLiteral("hydrostatik");
    vertical.category = forcedisplay::Category::Buoyancy;
    vertical.point = forcedisplay::Vec3{2.1, -3., .2};
    vertical.vector = forcedisplay::Vec3{0., 0., 500.};
    markerForces.contributions.push_back(vertical);
    QString markerError;
    const auto setMarkerState = [&] {
        require(movementScene->setState(markerState, markerForces, &markerError),
            "marker pose fixture failed");
        for (int i = 0; i < 3; ++i) {
            auto* view = dynamic_cast<VtkSceneWidget*>(movementWindow.findChild<QWidget*>(
                QStringLiteral("staticScene%1").arg(i)));
            require(view->setState(markerState, markerForces, &markerError),
                "auxiliary marker pose fixture failed");
            view->renderNow();
            auto* renderers = view->findChild<QVTKOpenGLNativeWidget*>()->renderWindow()->GetRenderers();
            renderers->InitTraversal();
            bool lateralArrow = false, sailArrow = false;
            bool weightArrow = false, buoyancyArrow = false, buoyancyPoint = false, centerPoint = false;
            while (auto* layer = renderers->GetNextItem()) {
                if (layer->GetLayer() != 1) continue;
                auto* actors = layer->GetActors();
                actors->InitTraversal();
                while (auto* actor = actors->GetNextActor()) {
                    if (!actor->GetVisibility()) continue;
                    auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
                    if (!mapper || !mapper->GetInput()) continue;
                    const auto* color = actor->GetProperty()->GetColor();
                    const bool lines = mapper->GetInput()->GetNumberOfLines() > 0;
                    if (std::abs(color[0] - 99./255.) < .001 && std::abs(color[2] - 121./255.) < .001 && lines)
                        lateralArrow = true;
                    if (std::abs(color[0] - 23./255.) < .001 && std::abs(color[2] - 56./255.) < .001 && lines)
                        sailArrow = true;
                    if (std::abs(color[0] - 26./255.) < .001 && std::abs(color[2] - 204./255.) < .001) {
                        weightArrow = weightArrow || lines;
                        centerPoint = centerPoint || mapper->GetInput()->GetNumberOfVerts() > 0;
                    }
                    if (std::abs(color[0] - 85./255.) < .001 && std::abs(color[2] - 240./255.) < .001) {
                        buoyancyArrow = buoyancyArrow || lines;
                        buoyancyPoint = buoyancyPoint || mapper->GetInput()->GetNumberOfVerts() > 0;
                    }
                }
                require(!layer->GetPreserveDepthBuffer(), "markers and arrows must ignore body occlusion");
            }
            require(lateralArrow == view->pressurePoint(false).has_value()
                    && sailArrow == view->pressurePoint(true).has_value(),
                "nonzero projected LDP and SDP arrows must be visible in exact marker colors in all three views");
            require(centerPoint && buoyancyPoint && weightArrow == (i != 2) && buoyancyArrow == (i != 2),
                "S and A must remain visible, with blue vertical resultant arrows only in side/stern projections");
        }
        testStaticViews(movementWindow, *movementScene);
    };
    setMarkerState();
    const auto markerPose = forcedisplay::readPose(markerState);
    require(movementScene->pressurePoint(false) && movementScene->pressurePoint(true)
        && forcedisplay::norm(forcedisplay::sub(*movementScene->pressurePoint(false),
            forcedisplay::toWorldPoint(markerPose, *markerForces.lateral->position))) < 1e-10
        && forcedisplay::norm(forcedisplay::sub(*movementScene->pressurePoint(true),
            forcedisplay::toWorldPoint(markerPose, *markerForces.sailCenter->position))) < 1e-10,
        "Pressure points must preserve exact XYZ with roll/translation, even outside the boat.");
    markerForces.lateral->status = QStringLiteral("geometrisch");
    markerForces.sailCenter.reset();
    setMarkerState();
    require(!movementScene->pressurePoint(false) && !movementScene->pressurePoint(true),
        "Geometric points must not masquerade as LDP; missing force must not invent SDP.");
    markerForces.lateral->status = QStringLiteral("gueltig");
    markerForces.lateral->evaluationVersion = 0;
    setMarkerState();
    require(!movementScene->pressurePoint(false), "Legacy LDP must not claim corrected placement.");
    movementWindow.close();
    report.insert(QStringLiteral("stationaryTurningAndSignedCourseRate"), true);
    report.insert(QStringLiteral("liveMovementOverlay"), true);
}

void testWindInputPrecision(const QString& models)
{
    QTemporaryDir settings;
    require(settings.isValid(), "Wind input settings directory");
    QCoreApplication::setOrganizationName(QStringLiteral("SegelPhysikTests"));
    QCoreApplication::setApplicationName(QStringLiteral("WindInputPrecision"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    MainWindow window;
    window.setModelFolder(models);
    window.selectInitialModel(QStringLiteral("folkeboot"));
    window.show();
    QPushButton* run = nullptr;
    for (auto* button : window.findChildren<QPushButton*>())
        if (button->text() == QStringLiteral("Core-Lauf starten")) run = button;
    auto* stop = window.findChild<QPushButton*>(QStringLiteral("stopSimulation"));
    auto* speed = window.findChild<QLineEdit*>(QStringLiteral("windSpeed"));
    auto* height = window.findChild<QLineEdit*>(QStringLiteral("windHeight"));
    auto* direction = window.findChild<QLineEdit*>(QStringLiteral("windDirection"));
    auto* mode = window.findChild<QComboBox*>(QStringLiteral("windMode"));
    auto* windPanel = window.findChild<QGroupBox*>(QStringLiteral("windPanel"));
    auto* workspace = window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
    auto* scene = dynamic_cast<VtkSceneWidget*>(
        window.findChild<QWidget*>(QStringLiteral("simulationScene")));
    require(run && stop && speed && height && direction && mode && windPanel && workspace && scene,
        "Wind input controls are missing.");
    waitUntil([&] { return run->isEnabled(); }, "Wind input model load timeout");
    workspace->setCurrentIndex(1);
    const auto edit = [](QLineEdit* field, const QString& text) {
        field->setText(text);
        field->setModified(true);
        require(QMetaObject::invokeMethod(field, "editingFinished"), "Finish wind input");
    };
    require(direction->text() == QStringLiteral("0,0") && speed->text() == QStringLiteral("5,0") &&
        height->text() == QStringLiteral("10,0") && !mode->currentData().toBool(),
        "default wind direction, speed, height, or mode");
    const auto hasWindMessage = [windPanel](const QString& text) {
        for (auto* label : windPanel->findChildren<QLabel*>())
            if (label->text().contains(text)) return true;
        return false;
    };
    for (auto* field : {speed, height}) {
        edit(field, QStringLiteral("0"));
        require(field->text() == QStringLiteral("0,0") &&
            hasWindMessage(QStringLiteral("begrenzt")), "lower wind limit must show its correction");
        edit(field, QStringLiteral("0,01"));
        require(field->text() == QStringLiteral("0,0") &&
            !hasWindMessage(QStringLiteral("begrenzt")), "minimum wind value must be accepted");
        edit(field, QStringLiteral("100"));
        require(field->text() == QStringLiteral("100,0") &&
            !hasWindMessage(QStringLiteral("begrenzt")), "maximum wind value must be accepted");
        edit(field, QStringLiteral("101"));
        require(field->text() == QStringLiteral("100,0") &&
            hasWindMessage(QStringLiteral("begrenzt")), "upper wind limit must show its correction");
        edit(field, QStringLiteral("nicht numerisch"));
        require(field->text() == QStringLiteral("100,0") && !field->toolTip().isEmpty() &&
            hasWindMessage(QStringLiteral("Ungültige Zahl")), "invalid wind input must retain its value");
    }
    edit(height, QStringLiteral("0,01"));
    edit(speed, QStringLiteral("0"));
    require(speed->text() == QStringLiteral("0,0"), "Minimum wind must be rounded for display");
    require(QMetaObject::invokeMethod(speed, "editingFinished"), "Repeat minimum wind input");
    const auto catalog = loadCatalog(models);
    const auto entry = std::find_if(catalog.entries.cbegin(), catalog.entries.cend(),
        [](const auto& candidate) { return candidate.id == QStringLiteral("folkeboot"); });
    require(catalog.catalog && entry != catalog.entries.cend(), "Wind test model catalog");
    const auto inspection = segelphysik::readModelInspection(
        *catalog.catalog, entry->id.toStdString(), entry->version.toStdString());
    require(inspection != nullptr, "Wind test model inspection");
    const auto mast = segelphysik::bestimmeMastspitze(*inspection);
    require(mast.position_modell_m.has_value(), "Wind test mast position");
    auto* navigationColumn = window.findChild<QWidget*>(QStringLiteral("staticNavigationColumn"));
    auto* overlay = navigationColumn
        ? dynamic_cast<navigation::Overlay*>(
            navigationColumn->findChild<QWidget*>(QStringLiteral("navigationOverlay"))) : nullptr;
    auto* profile = overlay ? overlay->findChild<QWidget*>(QStringLiteral("windProfile")) : nullptr;
    require(overlay && profile && profile->width() > 0 && profile->height() > 0,
        "Wind profile widget missing or empty");
    for (int runIndex = 0; runIndex < 2; ++runIndex) {
        waitUntil([&] { return run->isEnabled(); }, "Minimum wind model not ready");
        run->click();
        require(stop->isEnabled(), "Minimum wind static run did not start");
        waitUntil([&] { return !stop->isEnabled(); }, "Minimum wind static run timeout");
        scene->renderNow();
        const auto pose = scene->statePose();
        require(pose.has_value(), "Minimum wind static result is missing");
        const auto mastWorld = forcedisplay::toWorldPoint(*pose, *mast.position_modell_m);
        const double upper = (std::floor(mastWorld[2] / 5.) + 1.) * 5.;
        segelphysik::WindKonfiguration config;
        config.referenzgeschwindigkeit_m_s = .01;
        config.referenzhoehe_m = .01;
        navigation::Values expectedValues;
        expectedValues.windReference = std::array<double, 2>{.01, .01};
        for (int i = 0; i <= 40; ++i) {
            const double h = .0002 + (upper - .0002) * i / 40.;
            expectedValues.windProfile.push_back({h, segelphysik::windprofil(config, h)});
        }
        QImage actual(profile->size(), QImage::Format_ARGB32_Premultiplied);
        QImage expected(profile->size(), QImage::Format_ARGB32_Premultiplied);
        actual.fill(Qt::transparent);
        expected.fill(Qt::transparent);
        profile->render(&actual);
        overlay->setValues(expectedValues);
        profile->render(&expected);
        require(actual == expected, "Minimum wind must survive unmodified input and restart");
    }
    edit(speed, QStringLiteral("0,04"));
    require(speed->text() == QStringLiteral("0,0"), "Wind value must be rounded for display.");
    require(QMetaObject::invokeMethod(speed, "editingFinished"), "Repeat unmodified wind input");
    waitUntil([&] { return run->isEnabled(); }, "Wind input model not ready after workspace switch");
    run->click();
    require(stop->isEnabled(), "Wind precision static run did not start.");
    waitUntil([&] { return !stop->isEnabled(); }, "Wind precision static run timeout");
    scene->renderNow();
    require(scene->statePose().has_value(), "Wind precision static result is missing.");
    require(scene->options().windArrows, "Wind arrows are disabled.");
    require(scene->windArrowGeometry()[0].has_value(),
        "Unchanged rounded wind input must preserve its 0.04 m/s reference value.");

    edit(speed, QStringLiteral("5"));
    edit(height, QStringLiteral("10"));
    workspace->setCurrentIndex(2);
    waitUntil([&] { return run->isEnabled(); }, "Dynamic wind model not ready");
    auto* slider = window.findChild<QSlider*>();
    require(slider, "Dynamic wind playback slider missing");
    confirmDynamicRun(window, 10.);
    run->click();
    waitUntil([&] { return stop->isEnabled() && slider->maximum() >= 1; },
        "Dynamic wind run did not start");
    require(!height->isEnabled() && speed->isEnabled() && direction->isEnabled() && mode->isEnabled(),
        "Live wind inputs must remain editable except reference height");
    edit(direction, QStringLiteral("90"));
    edit(speed, QStringLiteral("8"));
    mode->setCurrentIndex(1);
    const auto showingReference = [&](const QString& value) {
        scene->renderNow();
        return profile->toolTip().contains(value);
    };
    waitUntil([&] { return showingReference(QStringLiteral("8.0 m/s")); },
        "Live wind change did not reach the displayed state");
    slider->setValue(0);
    waitUntil([&] { return showingReference(QStringLiteral("5.0 m/s")); },
        "Live playback did not display the historic wind snapshot");
    require(stop->isEnabled(), "Historic playback must not stop the live run");
    slider->setValue(slider->maximum());
    waitUntil([&] { return showingReference(QStringLiteral("8.0 m/s")); },
        "Returning to live playback did not restore the changed wind snapshot");
    stop->click();
    waitUntil([&] { return !stop->isEnabled(); }, "Dynamic wind stop timeout");
    slider->setValue(0);
    waitUntil([&] { return showingReference(QStringLiteral("5.0 m/s")); },
        "Historic wind view used current reference instead of its own snapshot");
    slider->setValue(slider->maximum());
    waitUntil([&] { return showingReference(QStringLiteral("8.0 m/s")); },
        "Playback did not restore the changed wind snapshot");
    window.close();
}

void testBodyFixedCamera(const QString& models, const QString& evidencePath)
{
    using namespace forcedisplay;
    const auto catalog = segelphysik::modellkatalog(std::filesystem::path(models.toStdWString()));
    const auto* entry = catalog.eintrag("420");
    require(entry, "camera test requires 420");
    const auto parts = toDisplayParts(segelphysik::darstellungsgeometrie(
        catalog, entry->modell_id, entry->modell_fassung));
    const auto path = QDir(models).filePath(QStringLiteral("420/modells_information.json"));
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
    auto inconsistentWind = state;
    inconsistentWind.insert(QStringLiteral("wind"), QJsonObject{
        {QStringLiteral("zeit_s"), state.value(QStringLiteral("zeit_s"))},
        {QStringLiteral("schritt"), state.value(QStringLiteral("schritt"))},
        {QStringLiteral("modell_fassung"), QStringLiteral("andere-fassung")}});
    require(!scene.setState(inconsistentWind, parseForceState(inconsistentWind), &error)
            && !error.isEmpty(),
        "wind from another model snapshot must be rejected before rendering");
    auto windState = state;
    windState.insert(QStringLiteral("referenzpunkte"), QJsonArray{QJsonObject{
        {QStringLiteral("id"), QStringLiteral("mastspitze")},
        {QStringLiteral("positionsstatus"), QStringLiteral("gueltig")},
        {QStringLiteral("position_welt_m"), QJsonArray{2., 3., 10.}}}});
    auto windVector = [](const QJsonArray& vector, bool visible) {
        return QJsonObject{{QStringLiteral("stroemung_welt_m_s"), vector},
            {QStringLiteral("pfeil_sichtbar"), visible}};
    };
    QJsonObject wind{{QStringLiteral("zeit_s"), state.value(QStringLiteral("zeit_s"))},
        {QStringLiteral("schritt"), state.value(QStringLiteral("schritt"))},
        {QStringLiteral("modell_fassung"), state.value(QStringLiteral("modell_fassung"))},
        {QStringLiteral("referenzgeschwindigkeit_m_s"), 5.},
        {QStringLiteral("referenzhoehe_m"), 10.},
        {QStringLiteral("wahr"), windVector(QJsonArray{-5., 0., 0.}, true)},
        {QStringLiteral("scheinbar"), windVector(QJsonArray{0., 0., 10.}, true)}};
    windState.insert(QStringLiteral("wind"), wind);
    auto windOptions = scene.options();
    windOptions.windArrows = true;
    scene.setOptions(windOptions);
    require(scene.setState(windState, parseForceState(state), &error), "wind arrow state");
    auto* actualProfile = overlay->findChild<QWidget*>(QStringLiteral("windProfile"));
    require(actualProfile && actualProfile->width() > 0 && actualProfile->height() > 0,
        "wind profile widget missing or empty");
    auto* navigationOverlay = dynamic_cast<navigation::Overlay*>(overlay);
    require(navigationOverlay, "wind profile overlay missing");
    segelphysik::WindKonfiguration profileConfig;
    profileConfig.referenzgeschwindigkeit_m_s = 5.;
    profileConfig.referenzhoehe_m = 10.;
    for (const auto& test : std::array<std::pair<double, double>, 4>{{
             {2.4, 5.}, {8., 10.}, {10., 15.}, {10.5, 15.}}}) {
        windState.insert(QStringLiteral("referenzpunkte"), QJsonArray{QJsonObject{
            {QStringLiteral("id"), QStringLiteral("mastspitze")},
            {QStringLiteral("positionsstatus"), QStringLiteral("gueltig")},
            {QStringLiteral("position_welt_m"), QJsonArray{2., 3., test.first}}}});
        require(scene.setState(windState, parseForceState(state), &error), "profile height state");
        scene.renderNow();
        navigation::Values expectedValues;
        expectedValues.windReference = std::array<double, 2>{10., 5.};
        for (int i = 0; i <= 40; ++i) {
            const double height = .0002 + (test.second - .0002) * i / 40.;
            expectedValues.windProfile.push_back({height, segelphysik::windprofil(profileConfig, height)});
        }
        QImage actual(actualProfile->size(), QImage::Format_ARGB32_Premultiplied);
        QImage expected(actualProfile->size(), QImage::Format_ARGB32_Premultiplied);
        actual.fill(Qt::transparent);
        expected.fill(Qt::transparent);
        actualProfile->render(&actual);
        navigationOverlay->setValues(expectedValues);
        actualProfile->render(&expected);
        if (actual != expected) {
            actual.save(QDir(evidencePath).filePath(QStringLiteral("profile_actual.bmp")), "BMP");
            expected.save(QDir(evidencePath).filePath(QStringLiteral("profile_expected.bmp")), "BMP");
            std::cerr << "Profile height " << test.first << ", size "
                      << actual.width() << 'x' << actual.height() << ", expected "
                      << expected.width() << 'x' << expected.height() << ", differing pixels "
                      << differingPixels(actual, expected) << '\n';
        }
        require(actual == expected, "wind profile must use Core values and the strictly higher five-metre bound");
    }
    windState.insert(QStringLiteral("referenzpunkte"), QJsonArray{QJsonObject{
        {QStringLiteral("id"), QStringLiteral("mastspitze")},
        {QStringLiteral("positionsstatus"), QStringLiteral("gueltig")},
        {QStringLiteral("position_welt_m"), QJsonArray{2., 3., 10.}}}});
    require(scene.setState(windState, parseForceState(state), &error), "restore wind arrow height");
    scene.renderNow();
    auto arrows = scene.windArrowGeometry();
    require(arrows[0] && arrows[1] && arrows[0]->start == Vec3{2., 3., 10.} &&
            arrows[1]->start == arrows[0]->start &&
            std::abs(arrows[0]->length - .5) < 1e-10 &&
            std::abs(arrows[1]->length - 1.) < 1e-10 &&
            arrows[0]->direction[0] < -.999 && arrows[1]->direction[2] > .999,
        "wind arrows must start at the mast and follow the scaled flow vectors");
    windOptions.windArrows = false;
    scene.setOptions(windOptions);
    scene.renderNow();
    require(!scene.windArrowGeometry()[0] && !scene.windArrowGeometry()[1],
        "wind arrow switch must hide both arrows");
    windOptions.windArrows = true;
    scene.setOptions(windOptions);
    wind.insert(QStringLiteral("wahr"), windVector(QJsonArray{.1, 0., 0.}, false));
    wind.insert(QStringLiteral("scheinbar"), windVector(QJsonArray{0., 0., .10001}, true));
    windState.insert(QStringLiteral("wind"), wind);
    require(scene.setState(windState, parseForceState(state), &error), "threshold wind state");
    scene.renderNow();
    require(!scene.windArrowGeometry()[0] && scene.windArrowGeometry()[1],
        "wind arrow thresholds must be independent, including vertical apparent wind");
    windState.insert(QStringLiteral("referenzpunkte"), QJsonArray{});
    require(scene.setState(windState, parseForceState(state), &error), "missing mast wind state");
    scene.renderNow();
    require(!scene.windArrowGeometry()[0] && !scene.windArrowGeometry()[1],
        "missing mast must not create wind arrows");
    require(scene.setState(state, parseForceState(state), &error), "restore camera state");
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

void captureWaterReflection(const QString& models, const QString& evidencePath)
{
    QDir evidence(evidencePath);
    require(evidence.mkpath(QStringLiteral(".")), "water reflection evidence directory");
    const auto catalog = segelphysik::modellkatalog(std::filesystem::path(models.toStdWString()));
    for (const QString& id : {QStringLiteral("420"), QStringLiteral("optimist")}) {
        const auto* entry = catalog.eintrag(id.toStdString());
        require(entry != nullptr, "water reflection model absent");
        const auto parts = toDisplayParts(segelphysik::darstellungsgeometrie(
            catalog, entry->modell_id, entry->modell_fassung));
        const QDir modelDir(QDir(models).filePath(id));
        const auto path = modelDir.filePath(modelDir.exists(QStringLiteral("modells_data.json"))
            ? QStringLiteral("modells_data.json") : QStringLiteral("modells_information.json"));
        QString error;
        auto live = LiveSimulation::create(path, validateModel(path), 0.0, 0.01, true,
            &error, {}, TestDriveOptions{true, 200, 0.1});
        require(live != nullptr, "water reflection simulation initialization");
        VtkSceneWidget scene;
        scene.resize(1000, 740);
        require(scene.setModel(parts, &error), "water reflection model load");
        scene.show();
        QApplication::processEvents();
        scene.setNavigationVisible(false);
        auto options = scene.options();
        options.forces = false;
        options.courseArrow = false;
        scene.setOptions(options);
        auto camera = scene.cameraView();
        camera.focalPoint = {0., 0., 0.};
        camera.viewUp = {0., 0., 1.};
        std::atomic_bool cancellation{false};
        auto worker = std::async(std::launch::async, [&] { live->run(cancellation); });
        const auto stopOnExit = qScopeGuard([&] { cancellation.store(true); });
        waitUntil([&] { const auto progress = live->progress(); return progress.states >= 2 || progress.finished; },
            "water reflection live state timeout");
        double firstLiveTime = -1.;
        for (const bool running : {true, false}) {
            if (!running) {
                require(live->progress().simulationSeconds > firstLiveTime,
                    "simulation must advance while live reflection views are captured");
                cancellation.store(true);
                worker.get();
            }
            for (const auto& view : std::array<std::pair<QString, forcedisplay::Vec3>, 3>{{
                    {QStringLiteral("steep"), {6., -8., 12.}},
                    {QStringLiteral("low"), {8., -10., 1.}},
                    {QStringLiteral("underwater"), {8., -10., -4.}}}}) {
                const auto progress = live->progress();
                require(progress.states >= 2 && (running ? !progress.finished :
                    progress.finished && progress.status == QStringLiteral("abgebrochen")),
                    "water reflection live or stopped simulation state");
                if (running && firstLiveTime < 0.) firstLiveTime = progress.simulationSeconds;
                const auto state = live->stateJson(progress.states - 1, &error);
                require(error.isEmpty() && scene.setState(state, forcedisplay::parseForceState(state), &error),
                    "water reflection state");
                camera.position = view.second;
                require(scene.setCameraView(camera), "water reflection camera");
                QImage withoutReflection;
                for (const bool reflection : {false, true}) {
                    auto profile = graphics::defaults(1);
                    profile.enabled[static_cast<std::size_t>(graphics::Feature::Reflection)] = reflection;
                    scene.setGraphicsProfile(profile);
                    scene.renderNow();
                    const auto image = scene.captureImage();
                    const auto phase = running ? QStringLiteral("_live_") : QStringLiteral("_");
                    require(image.save(evidence.filePath(id + phase + view.first
                        + (reflection ? QStringLiteral("_on.bmp") : QStringLiteral("_off.bmp"))), "BMP"),
                        "water reflection image");
                    if (!running) {
                        scene.renderNow();
                        require(sameStableImage(image, scene.captureImage()),
                            "stopped simulation reflection must remain stable");
                    }
                    if (!reflection) {
                        withoutReflection = image;
                    } else if (view.first == QStringLiteral("steep")) {
                        require(differingPixels(withoutReflection, image) > 100,
                            "Surface reflection must remain visible.");
                        require(differingPixels(withoutReflection.copy(300, 500, 400, 238),
                            image.copy(300, 500, 400, 238)) < 40,
                            "Reflection must not form deep detached geometry.");
                    } else if (view.first == QStringLiteral("underwater")) {
                        require(differingPixels(withoutReflection, image) < 40,
                            "Reflection geometry must not appear below the water surface.");
                    }
                }
            }
        }
    }
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
        const auto path = QDir(models).filePath(id + QStringLiteral("/modells_information.json"));
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
        require(force.contract == QStringLiteral("6"), "live result must use the V6 contract");
        require(!last.value(QStringLiteral("bewegungen")).toArray().isEmpty()
                && !last.value(QStringLiteral("elementtransformationen_modell")).toArray().isEmpty(),
            "movement result omitted confirmed settings or element poses");
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
            s.remove(QStringLiteral("wind"));
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
    window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"))->setCurrentIndex(2);
    duration->setValue(2.0);
    active->setChecked(true);
    waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "UI dynamic readiness");
    require(!unexpectedDialog, "UI dynamic initialization error");
    auto* sharedForce = window.findChild<QCheckBox*>(QStringLiteral("stationaryForceActive"));
    sharedForce->setChecked(true);
    window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"))->setValue(200.);
    confirmDynamicRun(window, 2.);
    run->click();
    waitUntil([&] { return slider->maximum() >= 4 || unexpectedDialog; }, "UI live history");
    require(!unexpectedDialog && !active->isVisible() && !force->isVisible() && !speed->isVisible()
            && sharedForce->isEnabled(),
        "legacy drive stays hidden and the shared dynamic force remains editable");
    slider->setValue(0);
    waitUntil([&] { return !stop->isEnabled() || unexpectedDialog; }, "UI live completion");
    require(!unexpectedDialog && sharedForce->isEnabled(), "UI force completion");
    window.close();
    report.insert(QStringLiteral("sharedDynamicForceEditableAndRewind"), true);
    QFile file(evidence.filePath(QStringLiteral("report.json")));
    require(file.open(QIODevice::WriteOnly), "motion evidence file");
    const auto bytes = QJsonDocument(report).toJson();
    require(file.write(bytes) == bytes.size(), "motion evidence write");
    std::cout << bytes.constData();
}

void runGroupMovementInspectionTests(const QString& sourceRoot)
{
    QTemporaryDir fixture;
    require(fixture.isValid(), "isolated group preview fixture");
    const auto source = std::filesystem::path(sourceRoot.toStdWString());
    const auto root = std::filesystem::path(fixture.path().toStdWString());
    const auto target = root / "420";
    std::filesystem::create_directories(target / "modells");
    for (const auto& file : {"schema_data.json", "schema_validation.json"})
        std::filesystem::copy_file(source / file, root / file);
    for (const auto& file : {"modells_data.json", "modells_validation.json"})
        std::filesystem::copy_file(source / "420" / file, target / file);
    for (const auto& folder : {"visualisation", "mass", "hydrostatics"})
        std::filesystem::copy(source / "420" / "modells" / folder,
            target / "modells" / folder, std::filesystem::copy_options::recursive);
    const auto dataPath = target / "modells_data.json";
    std::ifstream input(dataPath, std::ios::binary);
    auto data = nlohmann::json::parse(input);
    input.close();
    for (auto& group : data.at("groups"))
        for (auto& element : group.at("elements"))
            if (element.contains("physics")) {
                element["physics"].erase("aerodynamic");
                if (element["physics"].empty()) element.erase("physics");
            }
    data["groups"]["mainsail"]["elements"][0]["placement"]["originalTack"] = "port";
    data["groups"]["mainsail"]["elements"][0]["placement"]["translation_m"] =
        {0.8507067519354837, 0., 1.35};
    data["groups"]["mainsail"]["elements"][0]["placement"]["rotation_xyzw"] =
        {0., 0., 0., 1.};
    {
        std::ofstream output(dataPath, std::ios::binary | std::ios::trunc);
        output << data.dump();
        require(output.good(), "write isolated group preview fixture");
    }
    const auto previousSchema = qgetenv("SEGELPHYSIK_TEST_SCHEMA_ROOT");
    const bool hadSchema = qEnvironmentVariableIsSet("SEGELPHYSIK_TEST_SCHEMA_ROOT");
    struct RestoreSchema {
        QByteArray value;
        bool existed;
        ~RestoreSchema() {
            if (existed) qputenv("SEGELPHYSIK_TEST_SCHEMA_ROOT", value);
            else qunsetenv("SEGELPHYSIK_TEST_SCHEMA_ROOT");
        }
    } restore{previousSchema, hadSchema};
    qputenv("SEGELPHYSIK_TEST_SCHEMA_ROOT", fixture.path().toLocal8Bit());
    segelphysik::aktualisiere_modell_fingerprints(target,
        {segelphysik::ModellFingerprintBereich::modelData,
         segelphysik::ModellFingerprintBereich::visualisation,
         segelphysik::ModellFingerprintBereich::mass,
         segelphysik::ModellFingerprintBereich::hydrostatics,
         segelphysik::ModellFingerprintBereich::aerodynamics});
    const auto catalog = std::make_shared<const segelphysik::Modellkatalog>(
        segelphysik::modellkatalog(root));
    const auto* entry = catalog->eintrag("420");
    require(entry != nullptr && entry->schema_valide, "isolated 420 catalog entry");
    (void)segelphysik::lade_modell(dataPath);
    const auto snapshot = segelphysik::readModelInspection(*catalog, "420", entry->modell_fassung);
    const auto before = segelphysik::groupMovementPreviewJson(*snapshot, "mainsail");
    BoatInspection widget;
    widget.resize(1000, 700);
    widget.show();
    widget.setSnapshot(snapshot, catalog, false);
    QApplication::processEvents();
    auto* tree = widget.findChild<QTreeWidget*>(QStringLiteral("inspectionObjectTree"));
    auto* values = widget.findChild<QTreeWidget*>(QStringLiteral("inspectionValues"));
    require(tree && values, "group preview tree and table exist");
    QTreeWidgetItem* group = nullptr;
    QTreeWidgetItem* sail = nullptr;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        const auto id = (*it)->data(0, Qt::UserRole).toString();
        const auto kind = (*it)->data(0, Qt::UserRole + 4).toString();
        if (id == QStringLiteral("/gruppen/mainsail") && kind == QStringLiteral("gruppe"))
            group = *it;
        if (id == QStringLiteral("/gruppen/mainsail/elemente/Mainsail") &&
            kind == QStringLiteral("element")) sail = *it;
    }
    require(group && sail, "group and sail tree nodes exist");
    const auto* diagnostics = widget.findChild<QLabel*>(QStringLiteral("inspectionDiagnostics"));
    const auto movementError = QStringLiteral("group owns movement preview: %1")
        .arg(diagnostics ? diagnostics->text() : QString()).toStdString();
    require(!group->data(0, Qt::UserRole + 6).toString().isEmpty(), movementError.c_str());
    for (int i = 0; i < group->childCount(); ++i)
        require(group->child(i)->data(0, Qt::UserRole + 4).toString()
                == QStringLiteral("element"),
            "group contains objects rather than a preview action");
    for (int i = 0; i < sail->childCount(); ++i)
        require(sail->child(i)->data(0, Qt::UserRole + 5).toString().isEmpty(),
            "group movement is not attached to sail element");
    const auto camera = widget.scene()->cameraView();
    toggleMovementContextAction(tree, group, true);
    QApplication::processEvents();
    const auto previewMessage = QStringLiteral("group preview pose count %1: %2")
        .arg(widget.scene()->property("inspectionMovementPoseCount").toInt())
        .arg(diagnostics ? diagnostics->text() : QString()).toStdString();
    require(widget.scene()->property("inspectionMovementPoseCount").toInt() == 3,
        previewMessage.c_str());
    require(widget.scene()->property("inspectionPivotCount").toInt() == 1,
        "group preview displays the Core axis");
    require(widget.scene()->cameraView().position == camera.position &&
        widget.scene()->cameraView().focalPoint == camera.focalPoint,
        "group preview preserves the inspection camera");
    group->setSelected(true);
    QApplication::processEvents();
    QTreeWidgetItem* port = nullptr;
    QTreeWidgetItem* starboard = nullptr;
    int missingBounds = 0;
    for (int i = 0; i < values->topLevelItemCount(); ++i) {
        auto* row = values->topLevelItem(i);
        const auto action = row->data(0, Qt::UserRole + 13).toString();
        if (action == QStringLiteral("backbord")) port = row;
        if (action == QStringLiteral("steuerbord")) starboard = row;
        if ((row->text(2) == QStringLiteral("Vorschau Minimum") ||
             row->text(2) == QStringLiteral("Vorschau Maximum")) &&
            row->text(5) == QStringLiteral("nicht_vorhanden")) ++missingBounds;
    }
    require(port && starboard && port->text(3).toDouble() == 90. &&
        starboard->text(3).toDouble() == -90. && missingBounds == 2,
        "both signed sides and absent model bounds are shown");
    port->setSelected(true);
    QApplication::processEvents();
    require(widget.scene()->property("inspectionMovementPoseCount").toInt() == 1,
        "selecting one sail variant shows only that Core pose");
    values->clearSelection();
    toggleMovementContextAction(tree, group, false);
    tree->clearSelection();
    QApplication::processEvents();
    require(widget.scene()->property("inspectionMovementPreviewCount").toInt() == 0,
        "disabling group preview clears only temporary graphics");
    require(segelphysik::groupMovementPreviewJson(*snapshot, "mainsail") == before,
        "GUI preview does not mutate Core snapshot");
    widget.setSnapshot(snapshot, catalog, true);
    QString cameraError;
    require(widget.setCameraPreset(QStringLiteral("backbord"), &cameraError),
        "axis view available for 0.3 sail configuration");
    int savedActions = 0;
    bool saveFailed = false;
    std::shared_ptr<const segelphysik::Modellkatalog> currentCatalog = catalog;
    std::shared_ptr<const segelphysik::ModelInspection> currentSnapshot = snapshot;
    widget.onSailConfigurationFinished = [&](const QString& id, bool saved, const QString&,
        std::shared_ptr<const segelphysik::Modellkatalog>, const QString&) {
        if (!saved) { saveFailed = true; return; }
        require(id == QStringLiteral("420"), "GUI reports saved model identity");
        currentCatalog = std::make_shared<const segelphysik::Modellkatalog>(
            segelphysik::modellkatalog(root));
        const auto* current = currentCatalog->eintrag("420");
        require(current && current->fingerprints_aktuell, "saved GUI model is valid");
        currentSnapshot = segelphysik::readModelInspection(
            *currentCatalog, "420", current->modell_fassung);
        widget.setSnapshot(currentSnapshot, currentCatalog, true);
        widget.setEnabled(true);
        ++savedActions;
    };
    const auto waitForSave = [&](int expected) {
        const auto message = QStringLiteral("0.3 GUI sail action %1 was not saved").arg(expected)
            .toStdString();
        waitUntil([&] { return savedActions == expected || saveFailed; },
            message.c_str());
        require(!saveFailed, "0.3 GUI sail action failed");
        QApplication::processEvents();
    };
    invokeSailContextAction(tree, QStringLiteral("Mainsail"),
        QStringLiteral("Großsegel automatisch setzen"));
    waitForSave(1);
    invokeSailContextAction(tree, QStringLiteral("Jib"),
        QStringLiteral("Fock automatisch setzen"));
    waitForSave(2);
    invokeSailContextAction(tree, QStringLiteral("Mainsail"),
        QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, true);
    invokeSailContextAction(tree, QStringLiteral("Mainsail"),
        QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, false);
    invokeSailContextAction(tree, QStringLiteral("Jib"),
        QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, true);
    const auto modelBytes = [&] {
        std::ifstream input(dataPath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    };
    const auto beforeManual = modelBytes();
    invokeSailContextAction(tree, QStringLiteral("Jib"),
        QStringLiteral("Höhenversatz entlang Vorliek (cm)…"), QStringLiteral("10"));
    invokeSailContextAction(tree, QStringLiteral("Jib"), QStringLiteral("Backbordbug"));
    require(savedActions == 2 && modelBytes() == beforeManual,
        "manual offset and tack remain in memory until the mode is left");
    invokeSailContextAction(tree, QStringLiteral("Jib"),
        QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, false);
    waitForSave(3);
    const auto savedConfig = segelphysik::leseSegelKonfiguration(*currentCatalog, *currentSnapshot);
    const auto jibPose = sailPose(savedConfig, QStringLiteral("Jib"));
    require(jibPose && std::abs(jibPose->fock_versatz_cm - 10.) < 1e-7 &&
        jibPose->original_buglage == segelphysik::SegelBuglage::Backbord,
        "GUI retains jib offset and original tack after reload");
    const auto jibReference = sailReference(savedConfig, QStringLiteral("Jib"));
    require(jibReference.has_value(), "jib corner reference retained after reload");
    invokeSailContextAction(tree, QStringLiteral("Jib"),
        QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, true);
    const auto beforeDrag = modelBytes();
    bool dragged = false;
    for (const auto& view : {QStringLiteral("backbord"), QStringLiteral("steuerbord"),
                             QStringLiteral("bug"), QStringLiteral("heck"),
                             QStringLiteral("oben"), QStringLiteral("unten")}) {
        require(widget.setCameraPreset(view, &cameraError), "manual sail camera available");
        widget.scene()->renderNow();
        const auto handle = widget.scene()->inspectionScreenPointForWorld(
            posedSailPoint(*jibReference, *jibPose, jibReference->ecken.schothorn_m));
        const auto pivot = widget.scene()->inspectionScreenPointForWorld(
            posedSailPoint(*jibReference, *jibPose, jibReference->ecken.hals_m));
        const auto lever = handle - pivot;
        const auto length = std::hypot(lever.x(), lever.y());
        if (length < 30 || !widget.scene()->rect().adjusted(10, 10, -10, -10)
                .contains(handle.toPoint())) continue;
        const auto preparations = widget.scene()->workCounters().meshPreparations;
        const auto boundsBefore = inspectionGeometryBounds(widget.scene());
        dragInspectionPoint(widget.scene(), handle,
            handle + QPointF(-lever.y() / length * 12, lever.x() / length * 12));
        const auto drag = widget.scene()->onInspectionDrag;
        require(static_cast<bool>(drag), "manual drag handler exists");
        const QPointF start(200., 200.);
        drag(QStringLiteral("Jib"), {}, start, VtkSceneWidget::InspectionDragPhase::Start);
        drag(QStringLiteral("Jib"), {}, start + QPointF(20., 10.),
            VtkSceneWidget::InspectionDragPhase::Move);
        drag(QStringLiteral("Jib"), {}, start + QPointF(20., 10.),
            VtkSceneWidget::InspectionDragPhase::Finish);
        require(savedActions == 3 && modelBytes() == beforeDrag,
            "multiple manual gestures do not write model data");
        require(widget.scene()->workCounters().meshPreparations == preparations,
            "manual drag does not rebuild the model meshes");
        require(inspectionGeometryBounds(widget.scene()) != boundsBefore,
            "manual drag visibly updates the sail before saving");
        invokeSailContextAction(tree, QStringLiteral("Jib"),
            QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, false);
        waitForSave(4);
        const auto after = segelphysik::leseSegelKonfiguration(*currentCatalog, *currentSnapshot);
        const auto rotated = sailPose(after, QStringLiteral("Jib"));
        require(rotated && rotated->rotation_xyzw != jibPose->rotation_xyzw &&
            rotated->translation_m != jibPose->translation_m,
            "leaving manual mode saves both rotation and translation");
        dragged = true;
        break;
    }
    require(dragged, "jib corner can be rotated in an axis view");
    invokeSailContextAction(tree, QStringLiteral("Jib"),
        QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, true);
    invokeSailContextAction(tree, QStringLiteral("Jib"),
        QStringLiteral("Höhenversatz entlang Vorliek (cm)…"), QStringLiteral("12"));
    const auto beforeForeign = modelBytes();
    {
        std::ofstream output(dataPath, std::ios::binary | std::ios::app);
        output << " ";
        require(output.good(), "write isolated external model change");
    }
    const auto foreignData = modelBytes();
    invokeSailContextAction(tree, QStringLiteral("Jib"),
        QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, false);
    waitUntil([&] { return saveFailed; }, "conflicting manual save was not rejected");
    require(widget.manualModeActive() && modelBytes() == foreignData,
        "failed manual save retains the draft and does not overwrite external changes");
    invokeSailContextAction(tree, QStringLiteral("Jib"),
        QStringLiteral("Ungespeicherte Änderungen verwerfen"));
    require(!widget.manualModeActive(), "discard exits the retained manual draft");
    widget.close();
    {
        std::ofstream output(dataPath, std::ios::binary | std::ios::trunc);
        output << beforeForeign;
        require(output.good(), "restore isolated model after conflict");
    }

    {
        QTemporaryDir settings;
        require(settings.isValid(), "workspace-exit settings fixture");
        QCoreApplication::setOrganizationName(QStringLiteral("SegelPhysikManualExitTests"));
        QCoreApplication::setApplicationName(QStringLiteral("ManualExit"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
        bool unexpectedDialog = false;
        QTimer errors;
        QObject::connect(&errors, &QTimer::timeout, [&] {
            for (auto* top : QApplication::topLevelWidgets()) {
                auto* message = qobject_cast<QMessageBox*>(top);
                if (!message || !message->isVisible()) continue;
                unexpectedDialog = true;
                message->reject();
            }
        });
        errors.start(20);
        MainWindow window;
        window.setModelFolder(fixture.path());
        window.selectInitialModel(QStringLiteral("420"));
        window.show();
        auto* run = window.findChild<QPushButton*>(QStringLiteral("primaryButton"));
        auto* workspace = window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
        auto* mainTree = window.findChild<QTreeWidget*>(QStringLiteral("inspectionObjectTree"));
        auto* modelName = window.findChild<QLabel*>(QStringLiteral("modelName"));
        auto* cameraCombo = window.findChild<QComboBox*>(QStringLiteral("inspectionCameraPreset"));
        BoatInspection* mainInspection = nullptr;
        for (auto* parent = mainTree ? mainTree->parentWidget() : nullptr; parent;
             parent = parent->parentWidget()) {
            if ((mainInspection = dynamic_cast<BoatInspection*>(parent))) break;
        }
        require(run && workspace && mainTree && modelName && cameraCombo,
            "manual workspace-exit controls");
        require(mainInspection, "manual workspace-exit inspection");
        waitUntil([&] { return run->isEnabled() || unexpectedDialog; },
            "workspace-exit model load timeout");
        require(!unexpectedDialog, "workspace-exit model load failed");
#ifdef _WIN32
        struct ResourceLocks {
            std::vector<HANDLE> handles;
            std::vector<std::filesystem::path> paths;
            ~ResourceLocks() {
                for (const auto handle : handles) CloseHandle(handle);
                for (const auto& path : paths) {
                    std::error_code error;
                    std::filesystem::remove(path, error);
                }
            }
        } resourceLocks;
        for (const auto* folder : {"visualisation", "mass", "hydrostatics", "aerodynamics"}) {
            const auto path = target / "modells" / folder / "locked-test.dat";
            std::filesystem::create_directories(path.parent_path());
            {
                std::ofstream output(path, std::ios::binary);
                output << "unreferenced data";
                require(output.good(), "write isolated resource");
            }
            resourceLocks.paths.push_back(path);
            const auto handle = CreateFileW(path.c_str(), GENERIC_READ, 0, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            require(handle != INVALID_HANDLE_VALUE, "lock isolated resource");
            resourceLocks.handles.push_back(handle);
        }
#endif
        const auto axisIndex = cameraCombo->findData(QStringLiteral("backbord"));
        require(axisIndex >= 0, "manual workspace-exit axis camera available");
        cameraCombo->setCurrentIndex(axisIndex);
        require(QMetaObject::invokeMethod(cameraCombo, "activated", Q_ARG(int, axisIndex)),
            "manual workspace-exit axis camera selected");
        invokeSailContextAction(mainTree, QStringLiteral("Jib"),
            QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, true);
        require(mainInspection->manualModeActive(), "manual workspace-exit mode entered");
        invokeSailContextAction(mainTree, QStringLiteral("Jib"),
            QStringLiteral("Höhenversatz entlang Vorliek (cm)…"), QStringLiteral("13"));
        const auto initialVersion = modelName->toolTip();
        const auto initialBytes = modelBytes();
        workspace->setCurrentIndex(1);
        try {
            waitUntil([&] {
                return (!mainInspection->manualModeActive()
                    && modelName->toolTip() != initialVersion && run->isEnabled())
                    || unexpectedDialog;
            }, "leaving the workspace did not save and reload the manual draft");
        } catch (const std::exception&) {
            const auto* message = window.findChild<QLabel*>(QStringLiteral("inspectionDiagnostics"));
            const auto detail = QStringLiteral(
                "workspace exit: changed=%1, version=%2, ready=%3, manual=%4, saving=%5, diagnostic=%6")
                .arg(modelBytes() != initialBytes).arg(modelName->toolTip() != initialVersion)
                .arg(run->isEnabled()).arg(mainInspection->manualModeActive())
                .arg(mainInspection->sailSaveInProgress()).arg(message ? message->text() : QString());
            throw std::runtime_error(detail.toStdString());
        }
        require(!unexpectedDialog, "workspace-exit save failed");
        require(modelBytes() != initialBytes, "workspace-exit persisted the manual draft");
        workspace->setCurrentIndex(0);
        invokeSailContextAction(mainTree, QStringLiteral("Jib"),
            QStringLiteral("Manuelles Verschieben"), {}, SailDialogTestAction::Accept, true);
        invokeSailContextAction(mainTree, QStringLiteral("Jib"),
            QStringLiteral("Höhenversatz entlang Vorliek (cm)…"), QStringLiteral("14"));
        const auto beforeClose = modelBytes();
        window.close();
        waitUntil([&] { return (!window.isVisible() && modelBytes() != beforeClose)
                || unexpectedDialog; }, "closing did not save the manual draft");
        require(!unexpectedDialog, "closing save failed");
        errors.stop();
    }

    data["groups"]["mainsail"]["elements"][0]["placement"]["originalTack"] = "unknown";
    {
        std::ofstream output(dataPath, std::ios::binary | std::ios::trunc);
        output << data.dump();
        require(output.good(), "write unknown tack fixture");
    }
    segelphysik::aktualisiere_modell_fingerprints(target,
        {segelphysik::ModellFingerprintBereich::modelData});
    const auto unknownCatalog = std::make_shared<const segelphysik::Modellkatalog>(
        segelphysik::modellkatalog(root));
    const auto* unknownEntry = unknownCatalog->eintrag("420");
    require(unknownEntry != nullptr && unknownEntry->schema_valide, "unknown tack catalog entry");
    const auto unknownSnapshot = segelphysik::readModelInspection(
        *unknownCatalog, "420", unknownEntry->modell_fassung);
    widget.setSnapshot(unknownSnapshot, unknownCatalog, false);
    widget.show();
    QApplication::processEvents();
    group = nullptr;
    sail = nullptr;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        const auto id = (*it)->data(0, Qt::UserRole).toString();
        if (id == QStringLiteral("/gruppen/mainsail") &&
            (*it)->data(0, Qt::UserRole + 4).toString() == QStringLiteral("gruppe")) group = *it;
        if (id == QStringLiteral("/gruppen/mainsail/elemente/Mainsail") &&
            (*it)->data(0, Qt::UserRole + 4).toString() == QStringLiteral("element")) sail = *it;
    }
    require(group && sail, "unknown tack tree nodes");
    group->setSelected(true);
    QApplication::processEvents();
    bool diagnostic = false;
    for (int i = 0; i < values->topLevelItemCount(); ++i) {
        const auto* row = values->topLevelItem(i);
        if (row->text(2) == QStringLiteral("Vorschau-Diagnose") &&
            row->text(3).contains(QStringLiteral("Originalbuglage")) &&
            row->text(3).contains(QStringLiteral("Mainsail"))) diagnostic = true;
        require(row->data(0, Qt::UserRole + 13).toString() != QStringLiteral("backbord") &&
            row->data(0, Qt::UserRole + 13).toString() != QStringLiteral("steuerbord"),
            "unknown tack must not offer guessed side poses");
    }
    require(diagnostic, "unknown tack diagnosis is visible at group");
    tree->clearSelection();
    sail->setSelected(true);
    QApplication::processEvents();
    for (int i = 0; i < values->topLevelItemCount(); ++i)
        require(values->topLevelItem(i)->text(2) != QStringLiteral("Vorschau-Diagnose"),
            "group movement is not offered at the sail element");
    widget.close();

    data["groups"]["mainsail"]["elements"][1]["display"]["hidden"] = true;
    {
        std::ofstream output(dataPath, std::ios::binary | std::ios::trunc);
        output << data.dump();
        require(output.good(), "write hidden group member fixture");
    }
    segelphysik::aktualisiere_modell_fingerprints(target,
        {segelphysik::ModellFingerprintBereich::modelData});
    const auto hiddenCatalog = std::make_shared<const segelphysik::Modellkatalog>(
        segelphysik::modellkatalog(root));
    const auto* hiddenEntry = hiddenCatalog->eintrag("420");
    require(hiddenEntry != nullptr && hiddenEntry->schema_valide, "hidden member catalog entry");
    const auto hiddenSnapshot = segelphysik::readModelInspection(
        *hiddenCatalog, "420", hiddenEntry->modell_fassung);
    widget.setSnapshot(hiddenSnapshot, hiddenCatalog, false);
    widget.show();
    QApplication::processEvents();
    group = nullptr;
    for (QTreeWidgetItemIterator it(tree); *it; ++it)
        if ((*it)->data(0, Qt::UserRole).toString() == QStringLiteral("/gruppen/mainsail") &&
            (*it)->data(0, Qt::UserRole + 4).toString() == QStringLiteral("gruppe")) group = *it;
    require(group != nullptr, "hidden member group node");
    group->setSelected(true);
    QApplication::processEvents();
    bool unavailableNormal = false;
    bool missingMember = false;
    for (int i = 0; i < values->topLevelItemCount(); ++i) {
        const auto* row = values->topLevelItem(i);
        if (row->text(2) == QStringLiteral("Normalstellung")) {
            unavailableNormal = row->text(5) == QStringLiteral("nicht_bestimmbar") &&
                row->data(0, Qt::UserRole + 13).toString().isEmpty();
        }
        if (row->text(2) == QStringLiteral("Vorschau-Diagnose") &&
            row->text(3).contains(QStringLiteral("Boom"))) missingMember = true;
    }
    require(unavailableNormal && missingMember,
        "incomplete group geometry is diagnosed and unavailable normal pose is not offered");
    widget.close();
}

void runModelFormat03InteractionTests(const QString& sourceRoot)
{
    struct EnvironmentValue {
        QByteArray name;
        bool existed = false;
        QByteArray value;
        EnvironmentValue(const char* key, const QByteArray& replacement)
            : name(key), existed(qEnvironmentVariableIsSet(key)), value(qgetenv(key))
        {
            qputenv(name.constData(), replacement);
        }
        ~EnvironmentValue()
        {
            if (existed) qputenv(name.constData(), value);
            else qunsetenv(name.constData());
        }
        void set(const std::optional<QByteArray>& replacement)
        {
            if (replacement) qputenv(name.constData(), *replacement);
            else qunsetenv(name.constData());
        }
    };

    QTemporaryDir fixture;
    QTemporaryDir settings;
    require(fixture.isValid() && settings.isValid(), "isolated GUI 0.3 fixture directories");
    const auto source = std::filesystem::path(sourceRoot.toStdWString());
    const auto destination = std::filesystem::path(fixture.path().toStdWString());
    std::filesystem::copy(source / "schema_data.json", destination / "schema_data.json");
    std::filesystem::copy(source / "schema_validation.json",
        destination / "schema_validation.json");
    std::filesystem::copy(source / "bleistift", destination / "bleistift",
        std::filesystem::copy_options::recursive);

    EnvironmentValue schemaOverride("SEGELPHYSIK_TEST_SCHEMA_ROOT",
        fixture.path().toLocal8Bit());
    EnvironmentValue defaultsOverride("SEGELPHYSIK_TEST_DEFAULTS_FILE",
        QDir(settings.path()).filePath(QStringLiteral("defaults.json")).toLocal8Bit());
    EnvironmentValue testModelRoot("SEGELPHYSIK_TEST_MODEL_ROOT", {});
    EnvironmentValue modelRoot("SEGELPHYSIK_MODEL_ROOT", {});
    EnvironmentValue schemaRoot("SEGELPHYSIK_SCHEMA_ROOT", {});
    QCoreApplication::setOrganizationName(QStringLiteral("SegelPhysikGateTests"));
    QCoreApplication::setApplicationName(QStringLiteral("ModelFormat03"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());

    const auto validationPath = destination / "bleistift" / "modells_validation.json";
    auto validation = nlohmann::json::parse([&] {
        std::ifstream input(validationPath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }());
    validation["model"]["fingerprints"]["visualisation"]["hash"] = std::string(64, '0');
    {
        std::ofstream output(validationPath, std::ios::binary | std::ios::trunc);
        output << validation.dump();
        require(output.good(), "write isolated stale visualization fingerprint");
    }
    const auto staleBytes = [&] {
        std::ifstream input(validationPath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }();
    const auto schemaDataPath = destination / "schema_data.json";
    const auto schemaDataBytes = [&] {
        std::ifstream input(schemaDataPath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }();
    const auto schemaValidationPath = destination / "schema_validation.json";
    const auto schemaValidationBytes = [&] {
        std::ifstream input(schemaValidationPath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }();

    MainWindow window;
    window.setModelFolder(fixture.path());
    window.selectInitialModel(QStringLiteral("bleistift"));
    window.show();
    auto* modelList = window.findChild<QListWidget*>(QStringLiteral("modelList"));
    auto* run = window.findChild<QPushButton*>(QStringLiteral("primaryButton"));
    auto* status = window.findChild<QLabel*>(QStringLiteral("status"));
    auto* refresh = window.findChild<QPushButton*>(QStringLiteral("refreshModelsButton"));
    require(modelList && run && status && refresh, "MainWindow exposes model list, status and actions");
    const auto refreshCatalog = [&] {
        waitUntil([&] { return refresh->isEnabled(); }, "manual refresh not available");
        refresh->click();
    };
    try {
        waitUntil([&] {
            return modelList->count() == 1
                && status->toolTip().contains(QStringLiteral("CORE-"));
        }, "stale 0.3 model reports Core diagnostic in the real UI");
    } catch (const std::exception& error) {
        throw std::runtime_error((std::string(error.what()) + "; model count="
            + std::to_string(modelList->count())
            + ", status=" + status->text().toStdString()
            + ", tooltip=" + status->toolTip().toStdString()).c_str());
    }
    require(!run->isEnabled(), "stale visualization fingerprint blocks simulation");
    QList<VtkSceneWidget*> scenes;
    for (auto* widget : window.findChildren<QWidget*>())
        if (auto* scene = dynamic_cast<VtkSceneWidget*>(widget)) scenes.push_back(scene);
    require(!scenes.isEmpty(), "MainWindow exposes its VTK views");
    for (const auto* scene : scenes)
        require(scene->workCounters().meshPreparations == 0,
            "stale visualization fingerprint keeps geometry out of every view");

    const auto invokeContext = [&](bool expectAction) {
        auto* item = modelList->item(0);
        modelList->scrollToItem(item);
        QApplication::processEvents();
        const auto rect = modelList->visualItemRect(item);
        require(rect.isValid() && !rect.isEmpty(), "0.3 catalog row is visible");
        bool triggered = false;
        bool actionEnabled = false;
        bool actionActivated = false;
        QTimer driver;
        driver.setInterval(10);
        QObject::connect(&driver, &QTimer::timeout, &driver, [&] {
            for (auto* widget : QApplication::topLevelWidgets()) {
                auto* menu = qobject_cast<QMenu*>(widget);
                if (!menu || !menu->isVisible()) continue;
                for (auto* action : menu->actions()) {
                    if (action->text() != QStringLiteral("Visualisation-Hash erneuern"))
                        continue;
                    triggered = true;
                    actionEnabled = action->isEnabled();
                    QObject::connect(action, &QAction::triggered, menu,
                        [&actionActivated] { actionActivated = true; });
                    if (!actionEnabled) {
                        menu->close();
                        return;
                    }
                    action->trigger();
                    menu->close();
                    return;
                }
            }
        });
        driver.start();
        const QPoint local = rect.center();
        const QPoint global = modelList->viewport()->mapToGlobal(local);
        QContextMenuEvent event(QContextMenuEvent::Mouse, local, global);
        QApplication::sendEvent(modelList->viewport(), &event);
        driver.stop();
        QApplication::processEvents();
        require(triggered == expectAction,
            expectAction ? "stale, schema-valid model offers its hash action"
                         : "schema-invalid model does not offer its hash action");
        if (expectAction) {
            if (!actionEnabled) return false;
            require(actionActivated,
                "stale visualization hash action is activated from its context menu");
            return true;
        }
        return false;
    };

#ifdef _WIN32
    struct FileHandle {
        HANDLE value = INVALID_HANDLE_VALUE;
        ~FileHandle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
        void close()
        {
            if (value != INVALID_HANDLE_VALUE) {
                CloseHandle(value);
                value = INVALID_HANDLE_VALUE;
            }
        }
    } validationLock;
    validationLock.value = CreateFileW(validationPath.c_str(), GENERIC_READ,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    require(validationLock.value != INVALID_HANDLE_VALUE,
        "lock temporary validation file against atomic replacement");
    waitUntil([&] { return invokeContext(true); },
        "hash action did not become available after model loading");
    try {
        waitUntil([&] {
            return status->text().contains(QStringLiteral("fehlgeschlagen"))
                && status->toolTip().contains(QStringLiteral("CORE-FPR-0003"));
        }, "failed hash worker reports its Core error in the UI");
    } catch (const std::exception& error) {
        QStringList dialogs;
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (const auto* dialog = qobject_cast<QMessageBox*>(widget))
                dialogs << dialog->text();
        }
        throw std::runtime_error((std::string(error.what()) + "; status="
            + status->text().toStdString() + "; tooltip="
            + status->toolTip().toStdString() + "; dialogs="
            + dialogs.join(QStringLiteral(" | ")).toStdString() + "; model="
            + modelList->item(0)->toolTip().toStdString()).c_str());
    }
    require(!run->isEnabled()
        && scenes.front()->workCounters().meshPreparations == 0,
        "failed hash update keeps simulation and geometry blocked");
    require(staleBytes == [&] {
        std::ifstream input(validationPath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }(), "failed UI update leaves validation bytes unchanged");
    validationLock.close();
#endif

    waitUntil([&] { return invokeContext(true); },
        "hash action did not become available after releasing validation file");
    try {
        waitUntil([&] { return run->isEnabled(); },
            "fingerprint worker updates isolated model and catalog reload restores simulation");
    } catch (const std::exception& error) {
        throw std::runtime_error((std::string(error.what()) + "; post-update status="
            + status->text().toStdString()
            + ", tooltip=" + status->toolTip().toStdString()).c_str());
    }
    require(staleBytes != [&] {
        std::ifstream input(validationPath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }(), "successful UI action updates only the temporary validation file");
    require(std::any_of(scenes.cbegin(), scenes.cend(), [](const auto* scene) {
        return scene->workCounters().meshPreparations > 0;
    }), "successful hash refresh reloads visualization into the VTK views");
    auto dataPath = destination / "bleistift" / "modells_data.json";
    const auto checkedCatalog = loadCatalog(fixture.path());
    const auto* checkedEntry = checkedCatalog.catalog
        ? checkedCatalog.catalog->eintrag("bleistift") : nullptr;
    require(checkedEntry && checkedEntry->status == "verwendbar",
        "snapshot catalog ready before external change");
    const auto checkedInspection = segelphysik::readModelInspection(
        *checkedCatalog.catalog, "bleistift", checkedEntry->modell_fassung);
    const auto checkedValidation = validateModel(QString::fromStdWString(dataPath.wstring()));
    require(checkedValidation.valid, "snapshot model valid before external change");
    auto data = nlohmann::json::parse([&] {
        std::ifstream input(dataPath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }());
    data["identity"]["name"] = 42;
    {
        std::ofstream output(dataPath, std::ios::binary | std::ios::trunc);
        output << data.dump();
        require(output.good(), "write schema-invalid isolated model data");
    }
    require(run->isEnabled(), "external edits are not watched automatically");
    std::atomic_bool snapshotCancelled{false};
    const auto snapshotRun = runSimulation(QString::fromStdWString(dataPath.wstring()),
        checkedValidation, false, 0., 0.01, &snapshotCancelled, {}, checkedInspection);
    require(snapshotRun.completed && !snapshotRun.states.isEmpty(),
        "simulation reuses checked snapshot without rereading an externally changed model");
    QString snapshotError;
    require(LiveSimulation::create(QString::fromStdWString(dataPath.wstring()),
        checkedValidation, 0.1, 0.01, false, &snapshotError, {}, std::nullopt, checkedInspection)
        != nullptr, "dynamic preparation reuses checked snapshot");
    refreshCatalog();
    bool eventLoopResponsive = false;
    QTimer::singleShot(0, [&] { eventLoopResponsive = true; });
    waitUntil([&] { return eventLoopResponsive; },
        "GUI event loop remains responsive while the Core reloads the model");
    waitUntil([&] {
        return !run->isEnabled()
            && status->toolTip().contains(QStringLiteral("CORE-MOD"));
    }, "manual reload blocks a schema-invalid changed model");
    invokeContext(false);
    {
        std::ifstream input(source / "bleistift" / "modells_data.json", std::ios::binary);
        const auto originalData = std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
        std::ofstream output(dataPath, std::ios::binary | std::ios::trunc);
        output << originalData;
        require(output.good(), "restore original isolated model data");
    }
    refreshCatalog();
    waitUntil([&] { return run->isEnabled(); },
        "manual refresh revalidates model after repair");
    auto oldValidation = validateModel(QString::fromStdWString(dataPath.wstring()));
    require(oldValidation.valid, "restored temporary model is valid");
    std::atomic_bool cancelled{false};
    const auto validRun = runSimulation(QString::fromStdWString(dataPath.wstring()),
        oldValidation, false, 0., 0.01, &cancelled);
    require(validRun.completed && !validRun.states.isEmpty(),
        "valid static simulation completes after the Core revision check");
    oldValidation.fingerprint = QStringLiteral("outdated");
    QString liveError;
    require(!LiveSimulation::create(QString::fromStdWString(dataPath.wstring()),
        oldValidation, 0.1, 0.01, false, &liveError) &&
        liveError.contains(QStringLiteral("Modellfassung")),
        "dynamic preparation rejects an outdated model revision");
    const auto staleRun = runSimulation(QString::fromStdWString(dataPath.wstring()),
        oldValidation, false, 0., 0.01, &cancelled);
    require(!staleRun.completed && staleRun.error.contains(QStringLiteral("Modellfassung")),
        "static simulation rejects an outdated model revision");

    auto* workspace = window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
    require(workspace, "workspace tabs available for simulation preparation");
    workspace->setCurrentIndex(2);
    confirmDynamicRun(window, .1, false);
    run->click();
    workspace->setCurrentIndex(1);
    waitUntil([&] { return run->isEnabled(); },
        "workspace change during preparation releases the controls");
    require(status->text().contains(QStringLiteral("Reiter gewechselt")),
        "cancelled preparation must not start a simulation in another workspace");
    workspace->setCurrentIndex(2);
    bool simulationError = false;
    QTimer errorDriver;
    QObject::connect(&errorDriver, &QTimer::timeout, [&] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            auto* message = qobject_cast<QMessageBox*>(widget);
            if (message && message->isVisible()) {
                simulationError = true;
                message->reject();
            }
        }
    });
    errorDriver.start(10);
    confirmDynamicRun(window, .1, false);
    run->click();
    waitUntil([&] { return run->isEnabled() || simulationError; },
        "valid dynamic simulation after asynchronous preflight");
    errorDriver.stop();
    require(!simulationError && status->text().contains(QStringLiteral("abgeschlossen")),
        "valid dynamic simulation completes after the Core preflight");
    workspace->setCurrentIndex(0);

    const auto resourceRelative = data["groups"]["body"]["elements"][0]
        ["display"]["geometry"].get<std::string>();
    const auto resourcePath = destination / "bleistift"
        / std::filesystem::u8path(resourceRelative);
    const auto resourceBytes = [&] {
        std::ifstream input(resourcePath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }();
    require(std::filesystem::remove(resourcePath),
        "remove isolated geometry resource to exercise watcher");
    refreshCatalog();
    waitUntil([&] {
        return !run->isEnabled()
            && status->toolTip().contains(QStringLiteral("CORE-MOD"));
    }, "resource deletion triggers Core revalidation");
    {
        std::ofstream output(resourcePath, std::ios::binary);
        output << resourceBytes;
        require(output.good(), "restore isolated geometry resource");
    }
    refreshCatalog();
    waitUntil([&] { return run->isEnabled(); },
        "resource restoration triggers Core revalidation");

    {
        std::ofstream output(schemaDataPath, std::ios::binary | std::ios::trunc);
        output << "{";
        require(output.good(), "corrupt isolated watched schema file");
    }
    refreshCatalog();
    waitUntil([&] {
        return !run->isEnabled()
            && status->toolTip().contains(QStringLiteral("CORE-MOD"));
    }, "schema file change triggers Core revalidation");
    {
        std::ofstream output(schemaDataPath, std::ios::binary | std::ios::trunc);
        output << schemaDataBytes;
        require(output.good(), "restore isolated schema file");
    }
    refreshCatalog();
    waitUntil([&] { return run->isEnabled(); },
        "schema restoration triggers Core revalidation");

    {
        std::ofstream output(schemaValidationPath, std::ios::binary | std::ios::trunc);
        output << "{";
        require(output.good(), "corrupt isolated validation schema file");
    }
    refreshCatalog();
    waitUntil([&] {
        return !run->isEnabled()
            && status->toolTip().contains(QStringLiteral("CORE-MOD"));
    }, "validation schema change triggers Core revalidation");
    {
        std::ofstream output(schemaValidationPath, std::ios::binary | std::ios::trunc);
        output << schemaValidationBytes;
        require(output.good(), "restore isolated validation schema file");
    }
    refreshCatalog();
    waitUntil([&] { return run->isEnabled(); },
        "validation schema restoration triggers Core revalidation");

    auto validationData = nlohmann::json::parse([&] {
        std::ifstream input(validationPath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
    }());
    const auto validValidationBytes = validationData.dump();
    validationData["identity"]["id"] = "different";
    {
        std::ofstream output(validationPath, std::ios::binary | std::ios::trunc);
        output << validationData.dump();
        require(output.good(), "corrupt isolated watched validation file");
    }
    refreshCatalog();
    waitUntil([&] {
        return !run->isEnabled()
            && status->toolTip().contains(QStringLiteral("CORE-MOD"));
    }, "validation file change triggers Core revalidation");
    {
        std::ofstream output(validationPath, std::ios::binary | std::ios::trunc);
        output << validValidationBytes;
        require(output.good(), "restore isolated validation file");
    }
    refreshCatalog();
    waitUntil([&] { return run->isEnabled(); },
        "validation restoration triggers Core revalidation");
    window.close();

    const auto defaultRoot = destination / "default-models";
    const auto alternateRoot = destination / "alternate-models";
    std::filesystem::create_directories(defaultRoot);
    std::filesystem::create_directories(alternateRoot);
    std::filesystem::copy(source / "bleistift", defaultRoot / "bleistift",
        std::filesystem::copy_options::recursive);

    const auto defaultsPath = QDir(settings.path()).filePath(QStringLiteral("defaults.json"));
    const auto writeDefaults = [&](const QString& modelPath) {
        const nlohmann::json defaults{
            {"modelRoot", modelPath.toStdString()},
            {"schemaRoot", fixture.path().toStdString()},
            {"defaults", {{"preserved", true}}}};
        std::ofstream output(defaultsPath.toStdString(), std::ios::binary | std::ios::trunc);
        output << defaults.dump();
        require(output.good(), "write isolated GUI defaults file");
    };
    const auto testOverridePath = fixture.path() + QStringLiteral("/test-override");
    const auto environmentPath = fixture.path() + QStringLiteral("/environment");
    const auto explicitPath = fixture.path() + QStringLiteral("/explicit");
    writeDefaults(QString::fromStdWString(defaultRoot.wstring()));
    testModelRoot.set(testOverridePath.toLocal8Bit());
    modelRoot.set(environmentPath.toLocal8Bit());
    const auto testSchemaPath = fixture.path() + QStringLiteral("/schema-test");
    const auto environmentSchemaPath = fixture.path() + QStringLiteral("/schema-env");
    schemaOverride.set(testSchemaPath.toLocal8Bit());
    schemaRoot.set(environmentSchemaPath.toLocal8Bit());
    require(segelphysik::modellordner_standard() ==
            std::filesystem::path(testOverridePath.toStdWString()),
        "test model-root override outranks environment and defaults");
    require(segelphysik::schemaordner_fuer_modellordner(defaultRoot) ==
            std::filesystem::path(testSchemaPath.toStdWString()),
        "test schema-root override outranks environment and defaults");
    require(segelphysik::modellordner_standard(
            std::filesystem::path(explicitPath.toStdWString())) ==
            std::filesystem::path(explicitPath.toStdWString()),
        "explicit model root outranks all configured paths");
    testModelRoot.set(std::nullopt);
    schemaOverride.set(std::nullopt);
    require(segelphysik::modellordner_standard() ==
            std::filesystem::path(environmentPath.toStdWString()),
        "model-root environment variable outranks defaults");
    require(segelphysik::schemaordner_fuer_modellordner(defaultRoot) ==
            std::filesystem::path(environmentSchemaPath.toStdWString()),
        "schema-root environment variable outranks defaults");
    modelRoot.set(std::nullopt);
    schemaRoot.set(std::nullopt);
    require(segelphysik::modellordner_standard() == defaultRoot,
        "defaults model root is used when higher-priority paths are absent");
    require(segelphysik::schemaordner_fuer_modellordner(defaultRoot) ==
            std::filesystem::path(fixture.path().toStdWString()),
        "defaults schema root is used when higher-priority paths are absent");

    MainWindow defaultsWindow;
    defaultsWindow.setModelFolder({});
    defaultsWindow.selectInitialModel(QStringLiteral("bleistift"));
    defaultsWindow.show();
    auto* defaultsList = defaultsWindow.findChild<QListWidget*>(QStringLiteral("modelList"));
    auto* defaultsRefresh = defaultsWindow.findChild<QPushButton*>(QStringLiteral("refreshModelsButton"));
    require(defaultsList && defaultsRefresh, "defaults-backed MainWindow model list and refresh");
    const auto refreshDefaults = [&] {
        waitUntil([&] { return defaultsRefresh->isEnabled(); }, "defaults refresh not available");
        defaultsRefresh->click();
    };
    waitUntil([&] { return defaultsList->count() == 1; },
        "GUI loads the model root from defaults.json");
    writeDefaults(QString::fromStdWString(alternateRoot.wstring()));
    refreshDefaults();
    waitUntil([&] { return defaultsList->count() == 0; },
        "defaults file watcher switches the configured catalog root");
    QString defaultsDialogText;
    QTimer dialogDriver;
    dialogDriver.setInterval(10);
    QObject::connect(&dialogDriver, &QTimer::timeout, &dialogDriver, [&] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            auto* dialog = qobject_cast<QMessageBox*>(widget);
            if (!dialog || !dialog->isVisible()) continue;
            defaultsDialogText = dialog->text();
            dialog->accept();
        }
    });
    dialogDriver.start();
    {
        std::ofstream output(defaultsPath.toStdString(),
            std::ios::binary | std::ios::trunc);
        output << "{";
        require(output.good(), "write invalid isolated defaults JSON");
    }
    refreshDefaults();
    waitUntil([&] {
        return !defaultsDialogText.isEmpty()
            && defaultsDialogText.contains(QStringLiteral("defaults.json"));
    }, "invalid defaults changes are surfaced instead of falling back silently");
    dialogDriver.stop();
    writeDefaults(QString::fromStdWString(defaultRoot.wstring()));
    refreshDefaults();
    waitUntil([&] { return defaultsList->count() == 1; },
        "valid defaults restore the configured GUI catalog");
    const auto showsFolder = [&](const QString& folderName) {
        for (const auto* label : defaultsWindow.findChildren<QLabel*>())
            if (label->text().contains(folderName)) return true;
        return false;
    };
    modelRoot.set(QString::fromStdWString(alternateRoot.wstring()).toLocal8Bit());
    defaultsWindow.setModelFolder(QString::fromStdWString(defaultRoot.wstring()));
    try {
        waitUntil([&] {
            return defaultsList->count() == 1
                && showsFolder(QStringLiteral("default-models"));
        }, "explicit GUI folder selection loads despite environment override");
    } catch (const std::exception& error) {
        throw std::runtime_error((std::string(error.what()) + "; entries="
            + std::to_string(defaultsList->count())
            + ", default folder label found="
            + (showsFolder(QStringLiteral("default-models")) ? "yes" : "no")).c_str());
    }
    writeDefaults(QString::fromStdWString(alternateRoot.wstring()));
    waitUntil([&] {
        return defaultsList->count() == 1
            && showsFolder(QStringLiteral("default-models"));
    }, "explicit GUI folder selection outranks changed defaults and environment");
    modelRoot.set(std::nullopt);
    defaultsWindow.setModelFolder({});
    waitUntil([&] {
        return defaultsList->count() == 0
            && showsFolder(QStringLiteral("alternate-models"));
    }, "configured GUI folder follows defaults after explicit selection is cleared");
    defaultsDialogText.clear();
    dialogDriver.start();
    require(std::filesystem::remove(std::filesystem::path(defaultsPath.toStdWString())),
        "remove isolated defaults file to exercise watcher recovery");
    refreshDefaults();
    waitUntil([&] {
        return !defaultsDialogText.isEmpty()
            && defaultsDialogText.contains(QStringLiteral("defaults.json"));
    }, "missing defaults file is surfaced through the watched parent directory");
    dialogDriver.stop();
    writeDefaults(QString::fromStdWString(defaultRoot.wstring()));
    refreshDefaults();
    waitUntil([&] { return defaultsList->count() == 1; },
        "recreated defaults file restores configured GUI catalog");
#ifdef _WIN32
    struct DefaultsReadLock {
        HANDLE value = INVALID_HANDLE_VALUE;
        ~DefaultsReadLock() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
        void close()
        {
            if (value != INVALID_HANDLE_VALUE) {
                CloseHandle(value);
                value = INVALID_HANDLE_VALUE;
            }
        }
    } defaultsReadLock;
    defaultsReadLock.value = CreateFileW(
        std::filesystem::path(defaultsPath.toStdWString()).c_str(), GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    require(defaultsReadLock.value != INVALID_HANDLE_VALUE,
        "lock temporary defaults file against GUI reads");
    defaultsDialogText.clear();
    dialogDriver.start();
    writeDefaults(QString::fromStdWString(alternateRoot.wstring()));
    refreshDefaults();
    waitUntil([&] {
        return !defaultsDialogText.isEmpty()
            && defaultsDialogText.contains(QStringLiteral("defaults.json"));
    }, "unreadable defaults change is reported instead of falling back");
    dialogDriver.stop();
    defaultsReadLock.close();
    writeDefaults(QString::fromStdWString(defaultRoot.wstring()));
    refreshDefaults();
    waitUntil([&] { return defaultsList->count() == 1; },
        "readable defaults restore GUI configuration after access error");
#endif
    defaultsWindow.close();
    std::cout << "GUI 0.3 UI-/Worker-/Watcher-Gates bestanden; nur temporaere Modellkopie geaendert.\n";
}
}

int main(int argc, char** argv)
{
    vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_WARNING);
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    QElapsedTimer startup;
    startup.start();
    vtkNew<RenderErrors> renderErrors;
    vtkOutputWindow::SetInstance(renderErrors);
    try {
        if (app.arguments().size() == 3
            && app.arguments()[1] == QStringLiteral("--group-movement-only")) {
            runGroupMovementInspectionTests(app.arguments()[2]);
            return 0;
        }
        if (app.arguments().size() == 3
            && app.arguments()[1] == QStringLiteral("--model-format-03-gates")) {
            runModelFormat03InteractionTests(app.arguments()[2]);
            return 0;
        }
        if (app.arguments().size() == 4 && app.arguments()[1] == QStringLiteral("--performance-regression-only")) {
            QTemporaryDir settings;
            require(settings.isValid(), "Performance settings directory");
            QCoreApplication::setOrganizationName(QStringLiteral("SegelPhysikPerformanceTest"));
            QCoreApplication::setApplicationName(QStringLiteral("Performance"));
            QSettings::setDefaultFormat(QSettings::IniFormat);
            QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
            QDir evidence(app.arguments()[3]);
            require(evidence.mkpath(QStringLiteral(".")), "Performance evidence directory");
            const auto catalog = segelphysik::modellkatalog(
                std::filesystem::path(app.arguments()[2].toStdWString()));
            const auto* entry = catalog.eintrag("folkeboot");
            require(entry, "Performance model exists");
            const auto parts = toDisplayParts(segelphysik::darstellungsgeometrie(
                catalog, entry->modell_id, entry->modell_fassung));
            const auto path = QDir(app.arguments()[2]).filePath(QStringLiteral("folkeboot/modells_data.json"));
            QString error;
            auto live = LiveSimulation::create(path, validateModel(path), .1, .01, false, &error);
            require(live != nullptr, "Performance reference live create");
            std::atomic_bool stop{false};
            live->run(stop);
            const auto state = live->stateJson(5, &error);
            require(error.isEmpty() && !state.isEmpty(), "Performance reference state");
            const auto forces = forcedisplay::parseForceState(state);
            VtkSceneWidget first, second;
            first.resize(640, 480);
            second.resize(240, 180);
            second.setOrthographicView(VtkSceneWidget::OrthographicView::Port);
            first.show();
            second.show();
            require(first.setModel(parts, &error) && second.setModel(parts, &error), "Shared model load");
            require(second.workCounters().meshPreparations == 0, "Base mesh preparation shared between scenes");
            require(first.setState(state, forces, &error), "First prepared state");
            const auto prepared = first.workCounters().meshPreparations;
            require(second.setState(state, forces, &error), "Second prepared state");
            require(second.workCounters().meshPreparations == 0, "Transformed meshes shared between scenes");
            require(first.setState(state, forces, &error)
                    && first.workCounters().meshPreparations == prepared,
                "Unchanged matrices do not rebuild meshes");
            const auto profileWork = second.workCounters();
            second.setGraphicsProfile(graphics::defaults(0));
            second.setGraphicsProfile(graphics::defaults(1));
            require(second.workCounters().profiles == profileWork.profiles
                    && second.workCounters().renders == profileWork.renders,
                "Identical effective orthographic profile has no render");
            second.setUpdatesActive(false);
            const auto suspended = second.workCounters();
            second.setOptions({});
            second.setGraphicsProfile(graphics::defaults(0));
            second.setState(state, forces, &error);
            second.renderNow();
            second.frameStates({state});
            second.findChild<QVTKOpenGLNativeWidget*>()->renderWindow()->GetInteractor()->Render();
            QApplication::processEvents();
            require(second.workCounters().states == suspended.states
                    && second.workCounters().profiles == suspended.profiles
                    && second.workCounters().renders == suspended.renders
                    && second.workCounters().meshPreparations == suspended.meshPreparations,
                "Suspended scene does no work");
            second.setUpdatesActive(true);
            require(second.setState(state, forces, &error), "Resume selected state");
            require(second.statePose()->position == first.statePose()->position, "Resumed pose matches");
            first.clearState();
            require(first.setState(state, forces, &error)
                    && first.workCounters().meshPreparations == prepared,
                "Clear state restores immutable base without rebuilding meshes");
            auto moved = state;
            auto matrices = moved.value(QStringLiteral("elementtransformationen_modell")).toArray();
            require(!matrices.isEmpty(), "Movement matrices available");
            int matrixIndex = -1;
            for (int i = 0; i < matrices.size() && matrixIndex < 0; ++i) {
                const auto id = matrices[i].toObject().value(QStringLiteral("element_id")).toString();
                for (const auto& part : parts)
                    if (part.elementId == id) {
                        matrixIndex = i;
                        break;
                    }
            }
            require(matrixIndex >= 0, "Display part has a movement matrix");
            auto element = matrices[matrixIndex].toObject();
            auto matrix = element.value(QStringLiteral("matrix_zeilenweise")).toArray();
            matrix[3] = matrix[3].toDouble() + .125;
            element.insert(QStringLiteral("matrix_zeilenweise"), matrix);
            matrices[matrixIndex] = element;
            moved.insert(QStringLiteral("elementtransformationen_modell"), matrices);
            require(first.setState(moved, forces, &error), "Changed matrix state");
            const auto changed = first.workCounters().meshPreparations;
            require(changed > prepared, "Changed matrix invalidates prepared data");
            require(second.setState(moved, forces, &error)
                    && second.workCounters().meshPreparations == 0, "Changed preparation shared");
            require(first.setState(state, forces, &error), "Historical matrix restored");
            require(first.workCounters().meshPreparations > changed, "Historical matrix invalidates current cache");
            const auto beforeSourceChange = first.workCounters().meshPreparations;
            parts.front().mesh->Modified();
            require(first.setModel(parts, &error) && second.setModel(parts, &error)
                    && first.workCounters().meshPreparations > beforeSourceChange,
                "Changed source MTime invalidates base preparation");
            require(first.setState(state, forces, &error) && second.setState(state, forces, &error)
                    && second.workCounters().meshPreparations == 0,
                "Replaced preparation is shared without stale transformed data");
            first.close();
            second.close();

            bool unexpectedDialog = false;
            QTimer errors;
            QObject::connect(&errors, &QTimer::timeout, [&] {
                for (auto* widget : QApplication::topLevelWidgets())
                    if (auto* message = qobject_cast<QMessageBox*>(widget)) {
                        std::cerr << message->text().toStdString() << '\n';
                        unexpectedDialog = true;
                        message->reject();
                    }
            });
            errors.start(50);
            MainWindow window;
            window.resize(1600, 1000);
            window.setModelFolder(app.arguments()[2]);
            window.selectInitialModel(QStringLiteral("folkeboot"));
            window.show();
            auto* run = window.findChild<QPushButton*>(QStringLiteral("primaryButton"));
            auto* tabs = window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
            waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "Performance window load");
            require(!unexpectedDialog, "Performance model loaded");
            tabs->setCurrentIndex(2);
            window.findChild<QCheckBox*>(QStringLiteral("stationaryForceActive"))->setChecked(true);
            window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"))->setValue(150.);
            window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceY"))->setValue(10.);
            window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryPointZ"))->setValue(3.);
            auto* views = window.findChild<QCheckBox*>(QStringLiteral("displayAttitudeViews"));
            auto* column = window.findChild<QWidget*>(QStringLiteral("staticViewsColumn"));
            std::vector<VtkSceneWidget*> auxiliary;
            for (auto* widget : column->findChildren<QWidget*>())
                if (auto* scene = dynamic_cast<VtkSceneWidget*>(widget)) auxiliary.push_back(scene);
            require(auxiliary.size() == 3, "Three auxiliary scenes");
            views->setChecked(false);
            std::vector<VtkSceneWidget::WorkCounters> before;
            for (auto* scene : auxiliary) before.push_back(scene->workCounters());
            confirmDynamicRun(window, .3, false);
            run->click();
            auto* cancel = window.findChild<QPushButton*>(QStringLiteral("stopSimulation"));
            require(cancel, "Performance stop control");
            waitUntil([&] { return !cancel->isEnabled() || unexpectedDialog; }, "Performance live completion");
            require(!unexpectedDialog, "Performance live completed");
            for (std::size_t i = 0; i < auxiliary.size(); ++i) {
                const auto& after = auxiliary[i]->workCounters();
                require(after.states == before[i].states && after.profiles == before[i].profiles
                        && after.renders == before[i].renders
                        && after.meshPreparations == before[i].meshPreparations,
                    "Real live ticks leave switched-off scenes untouched");
            }
            auto* slider = window.findChild<QSlider*>();
            require(slider, "Performance history control");
            auto* free = dynamic_cast<VtkSceneWidget*>(
                window.findChild<QWidget*>(QStringLiteral("simulationScene")));
            const auto priorMotionInputs = free->workCounters().motionInputs;
            views->setChecked(true);
            const auto coloredArrows = [](VtkSceneWidget* scene, const QColor& color) {
                int count = 0;
                auto* layers = scene->findChild<QVTKOpenGLNativeWidget*>()->renderWindow()->GetRenderers();
                layers->InitTraversal();
                while (auto* layer = layers->GetNextItem()) {
                    if (layer->GetLayer() != 1) continue;
                    auto* actors = layer->GetActors();
                    actors->InitTraversal();
                    while (auto* actor = actors->GetNextActor()) {
                        const auto* rgb = actor->GetProperty()->GetColor();
                        auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
                        if (actor->GetVisibility() && mapper && mapper->GetInput()
                            && mapper->GetInput()->GetNumberOfLines() > 0
                            && std::abs(rgb[0] - color.redF()) < .001
                            && std::abs(rgb[1] - color.greenF()) < .001
                            && std::abs(rgb[2] - color.blueF()) < .001)
                            ++count;
                    }
                }
                return count;
            };
            const auto defaults = forcedisplay::staticForceDefaults();
            for (const auto& style : defaults)
                window.findChild<QCheckBox*>(QStringLiteral("staticForce_") + style.key)->setChecked(false);
            for (const auto type : {forcedisplay::StaticForce::Weight, forcedisplay::StaticForce::Buoyancy,
                     forcedisplay::StaticForce::Sail}) {
                const auto& style = defaults[static_cast<std::size_t>(type)];
                auto* choice = window.findChild<QCheckBox*>(QStringLiteral("staticForce_") + style.key);
                require(choice->isEnabled(), "Weight, buoyancy and external force must be available");
                choice->setChecked(true);
                free->renderNow();
                if (coloredArrows(free, QColor(style.color)) == 0) {
                    std::cerr << "Missing arrow: " << style.key.toStdString()
                              << ", selected=" << free->options().staticStyles[static_cast<std::size_t>(type)].enabled
                              << ", active=" << free->updatesActive()
                              << ", tooltip=" << choice->toolTip().toStdString() << '\n';
                }
                require(coloredArrows(free, QColor(style.color)) > 0,
                    "Selected weight/buoyancy/external force must produce an actual free-view arrow");
                const auto enabledImage = digest(free->captureImage());
                choice->setChecked(false);
                free->renderNow();
                require(coloredArrows(free, QColor(style.color)) == 0,
                    "Deselected force must remove its actual free-view arrow");
                require(digest(free->captureImage()) != enabledImage,
                    "Force selection must change the rendered free-view image");
            }
            views->setChecked(false);
            require(free->workCounters().motionInputs == priorMotionInputs,
                "Changing views at the same snapshot must not replay the complete motion history");
            for (std::size_t i = 0; i < auxiliary.size(); ++i)
                before[i] = auxiliary[i]->workCounters();
            slider->setValue(0);
            views->setChecked(true);
            for (std::size_t i = 0; i < auxiliary.size(); ++i)
                require(auxiliary[i]->workCounters().states == before[i].states + 1
                        && auxiliary[i]->statePose()->position == free->statePose()->position,
                    "Resume only selected historical snapshot once");
            auto* viewport = auxiliary.front()->findChild<QVTKOpenGLNativeWidget*>();
            const QPointF local(viewport->rect().center());
            QMouseEvent doubleClick(QEvent::MouseButtonDblClick, local,
                QPointF(viewport->mapToGlobal(local.toPoint())),
                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(viewport, &doubleClick);
            QApplication::processEvents();
            views->setChecked(false);
            require(!free->updatesActive() && auxiliary.front()->updatesActive(),
                "Swapped large view stays active when column is off");
            slider->setValue(slider->maximum());
            require(!unexpectedDialog, "Swapped active view keeps state and navigation valid");
            free->resetMotionHistory();
            const auto freeBeforeResume = free->workCounters().states;
            views->setChecked(true);
            require(free->workCounters().states == freeBeforeResume + 1,
                "Empty free-view history resumes without duplicate state replay");
            const auto selectView = [](VtkSceneWidget* scene) {
                auto* target = scene->findChild<QVTKOpenGLNativeWidget*>();
                const QPointF point(target->rect().center());
                const QPointF global(target->mapToGlobal(point.toPoint()));
                for (auto type : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease,
                         QEvent::MouseButtonDblClick, QEvent::MouseButtonRelease}) {
                    QMouseEvent mouse(type, point, global, Qt::LeftButton,
                        type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                        Qt::NoModifier);
                    QApplication::sendEvent(target, &mouse);
                }
                QApplication::processEvents();
            };
            auto* stack = window.findChild<QStackedWidget*>(QStringLiteral("sceneStack"));
            auto* weight = window.findChild<QCheckBox*>(QStringLiteral("staticForce_weight"));
            const auto historyBeforeSwaps = free->workCounters().motionInputs;
            selectView(free);
            for (int repeat = 0; repeat < 3; ++repeat)
                for (auto* view : auxiliary) {
                    selectView(view);
                    require(stack->currentWidget() == view, "Real double click enlarges selected schematic view");
                    views->setChecked(false);
                    weight->setChecked(false);
                    weight->setChecked(true);
                    require(view->options().staticStyles[static_cast<std::size_t>(
                                forcedisplay::StaticForce::Weight)].enabled,
                        "Visible schematic view follows force selection while free view is suspended");
                    views->setChecked(true);
                    selectView(free);
                    require(stack->currentWidget() == free && free->updatesActive()
                            && free->statePose()->position == view->statePose()->position,
                        "Real double click restores free view with selected snapshot");
                    require(coloredArrows(free, QColor(defaults[static_cast<std::size_t>(
                                forcedisplay::StaticForce::Weight)].color)) > 0,
                        "Free-view force arrow returns after schematic swap and hidden selection change");
                }
            require(free->workCounters().motionInputs == historyBeforeSwaps,
                "Repeated actual free/schematic swaps must not replay old motion inputs");
            for (std::size_t i = 0; i < defaults.size(); ++i)
                window.findChild<QCheckBox*>(QStringLiteral("staticForce_") + defaults[i].key)
                    ->setChecked(defaults[i].enabled);
            const auto image = window.grab().toImage();
            require(image.save(evidence.filePath(QStringLiteral("performance-window.bmp")), "BMP"),
                "Performance image evidence");
            views->setChecked(false);
            window.selectInitialModel(QStringLiteral("420"));
            waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "Model change while views off");
            require(!unexpectedDialog, "Model replacement succeeded");
            views->setChecked(true);
            require(!free->statePose().has_value(), "Model replacement invalidates historical geometry");
            for (auto* view : auxiliary)
                require(!view->statePose().has_value(), "No stale auxiliary state after model replacement");
            window.close();
            QFile report(evidence.filePath(QStringLiteral("report.json")));
            require(report.open(QIODevice::WriteOnly), "Performance report");
            report.write(QJsonDocument(QJsonObject{{"sharedMeshes", true}, {"invalidatedMatrices", true},
                {"orthographicProfileIdempotent", true}, {"switchedOffSceneWork", 0},
                {"liveAndHistoricalResume", true}, {"swappedView", true},
                {"sourceMTimeInvalidation", true}, {"modelReplacement", true},
                {"interactorSuspended", true}, {"forceArrowActorAndPixelSelection", true},
                {"realFreeSchematicSwaps", 18}, {"sameSnapshotHistoryReplayInputs", 0}}).toJson());
            require(renderErrors->count == 0, "Performance VTK errors");
            return 0;
        }
        if (app.arguments().size() == 4 && app.arguments()[1] == QStringLiteral("--performance-audit-only")) {
            QDir evidence(app.arguments()[3]);
            require(evidence.mkpath(QStringLiteral(".")), "Performance evidence directory required.");
            const auto catalog = segelphysik::modellkatalog(
                std::filesystem::path(app.arguments()[2].toStdWString()));
            QJsonObject report;
            report.insert(QStringLiteral("duration_s"), .5);
            report.insert(QStringLiteral("step_s"), .01);
            report.insert(QStringLiteral("repeats"), 3);
            report.insert(QStringLiteral("rendered_snapshots_per_repeat"), 8);
            for (const QString& id : {QStringLiteral("420"), QStringLiteral("folkeboot")}) {
                const auto* entry = catalog.eintrag(id.toStdString());
                require(entry, "Performance reference model missing.");
                const auto path = QDir(app.arguments()[2]).filePath(id + QStringLiteral("/modells_information.json"));
                const auto validation = validateModel(path);
                require(validation.valid, "Performance reference model invalid.");
                const auto parts = toDisplayParts(segelphysik::darstellungsgeometrie(
                    catalog, entry->modell_id, entry->modell_fassung));
                QJsonArray runs;
                for (int repeat = 0; repeat < 3; ++repeat) {
                    QString error;
                    auto live = LiveSimulation::create(path, validation, .5, .01, false, &error, {},
                        TestDriveOptions{true, 200., 463. / 90.});
                    require(live != nullptr, "Performance Core run creation failed.");
                    QElapsedTimer timer;
                    std::atomic_bool cancelled{false};
                    timer.start();
                    live->run(cancelled);
                    const double wallMs = timer.nsecsElapsed() / 1e6;
                    const auto progress = live->progress();
                    require(progress.status == QStringLiteral("abgeschlossen") && progress.states == 51,
                        "Performance Core run must finish all 50 steps.");
                    QVector<QJsonObject> snapshots;
                    timer.restart();
                    for (int i = 1; i <= 8; ++i) {
                        auto state = live->stateJson(i * 6, &error);
                        require(error.isEmpty() && !state.isEmpty(), "Performance JSON snapshot failed.");
                        snapshots.push_back(state);
                    }
                    const double jsonMs = timer.nsecsElapsed() / 1e6;
                    QWidget panel;
                    auto* layout = new QHBoxLayout(&panel);
                    auto* free = new VtkSceneWidget;
                    free->setMinimumSize(720, 480);
                    layout->addWidget(free);
                    std::array<VtkSceneWidget*, 3> auxiliary{};
                    auto* column = new QVBoxLayout;
                    layout->addLayout(column);
                    for (int i = 0; i < 3; ++i) {
                        auxiliary[i] = new VtkSceneWidget;
                        auxiliary[i]->setMinimumSize(140, 160);
                        auxiliary[i]->setOrthographicView(
                            static_cast<VtkSceneWidget::OrthographicView>(i + 1));
                        column->addWidget(auxiliary[i]);
                    }
                    std::array<VtkSceneWidget*, 4> scenes{free, auxiliary[0], auxiliary[1], auxiliary[2]};
                    VtkSceneWidget::DisplayOptions options;
                    options.forces = false;
                    options.staticForces = true;
                    options.staticStyles = forcedisplay::staticForceDefaults();
                    graphics::Profile profile;
                    for (auto* scene : scenes) {
                        require(scene->setModel(parts, &error), "Performance scene model failed.");
                        scene->setOptions(options);
                        scene->setGraphicsProfile(profile);
                    }
                    panel.show();
                    QApplication::processEvents();
                    const auto firstForces = forcedisplay::parseForceState(snapshots.front());
                    for (auto* scene : scenes)
                        require(scene->setState(snapshots.front(), firstForces, &error),
                            "Performance warmup failed.");
                    double freeMs = 0., auxiliaryMs = 0., profileMs = 0.;
                    for (const auto& state : snapshots) {
                        const auto forces = forcedisplay::parseForceState(state);
                        timer.restart();
                        require(free->setState(state, forces, &error), "Performance free state failed.");
                        freeMs += timer.nsecsElapsed() / 1e6;
                        timer.restart();
                        for (auto* scene : auxiliary)
                            require(scene->setState(state, forces, &error), "Performance auxiliary state failed.");
                        auxiliaryMs += timer.nsecsElapsed() / 1e6;
                        timer.restart();
                        // Current MainWindow reapplies an unchanged profile during each progress/state update.
                        for (auto* scene : scenes) scene->setGraphicsProfile(profile);
                        profileMs += timer.nsecsElapsed() / 1e6;
                    }
                    runs.append(QJsonObject{
                        {QStringLiteral("core_wall_ms"), wallMs},
                        {QStringLiteral("core_compute_ms"), progress.coreSeconds * 1000.},
                        {QStringLiteral("snapshot_json_8_ms"), jsonMs},
                        {QStringLiteral("free_state_mean_ms"), freeMs / 8.},
                        {QStringLiteral("three_auxiliary_state_mean_ms"), auxiliaryMs / 8.},
                        {QStringLiteral("unchanged_profile_four_views_mean_ms"), profileMs / 8.},
                        {QStringLiteral("four_views_and_profile_mean_ms"), (freeMs + auxiliaryMs + profileMs) / 8.}});
                    panel.close();
                }
                report.insert(id, runs);
            }
            require(renderErrors->count == 0, "VTK error in performance audit.");
            const auto output = QJsonDocument(report).toJson();
            QFile file(evidence.filePath(QStringLiteral("performance-audit.json")));
            require(file.open(QIODevice::WriteOnly) && file.write(output) == output.size(),
                "Performance report persistence failed.");
            std::cout << output.toStdString();
            return 0;
        }
        if (app.arguments().size() == 4 && app.arguments()[1] == QStringLiteral("--dynamic-layout-only")) {
            QTemporaryDir settings;
            require(settings.isValid(), "Temporary dynamic-test settings directory required.");
            QCoreApplication::setOrganizationName(QStringLiteral("SegelPhysikDynamicLayoutTest"));
            QCoreApplication::setApplicationName(QStringLiteral("DynamicLayout"));
            QSettings::setDefaultFormat(QSettings::IniFormat);
            QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
            QDir evidence(app.arguments()[3]);
            require(evidence.mkpath(QStringLiteral(".")), "Dynamic evidence directory required.");
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
            window.resize(1600, 1000);
            window.setModelFolder(app.arguments()[2]);
            window.selectInitialModel(QStringLiteral("folkeboot"));
            window.show();
            auto* tabs = window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
            auto* run = window.findChild<QPushButton*>(QStringLiteral("primaryButton"));
            require(tabs && run, "Dynamic workspace controls required.");
            waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "Dynamic model load timeout.");
            require(!unexpectedDialog, "Dynamic model load failed.");
            tabs->setCurrentIndex(1);
            auto* activeForce = window.findChild<QCheckBox*>(QStringLiteral("stationaryForceActive"));
            activeForce->setChecked(true);
            window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"))->setValue(150.);
            window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceY"))->setValue(10.);
            window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryPointZ"))->setValue(3.);
            const auto fixedInputs = window.findChildren<QDoubleSpinBox*>(
                QRegularExpression(QStringLiteral("^stationaryMovement_")));
            require(!fixedInputs.isEmpty(), "Reference model needs fixed movement input.");
            fixedInputs.front()->setValue(fixedInputs.front()->value() + 5.);
            const double fixedValue = fixedInputs.front()->value();
            auto* staticWeight = window.findChild<QCheckBox*>(QStringLiteral("staticForce_weight"));
            staticWeight->setChecked(false);
            tabs->setCurrentIndex(2);
            QApplication::processEvents();
            auto* sidebar = window.findChild<QWidget*>(QStringLiteral("staticDisplaySidebar"));
            auto* navigation = window.findChild<QScrollArea*>(QStringLiteral("staticNavigationColumn"));
            auto* views = window.findChild<QWidget*>(QStringLiteral("staticViewsColumn"));
            auto* table = window.findChild<QTreeWidget*>(QStringLiteral("staticForceValues"));
            auto* frame = window.findChild<QComboBox*>(QStringLiteral("dynamicCameraFrame"));
            auto* drive = window.findChild<QCheckBox*>(QStringLiteral("testDriveActive"));
            auto* duration = window.findChild<QDoubleSpinBox*>(QStringLiteral("simulationDuration"));
            auto* step = window.findChild<QDoubleSpinBox*>(QStringLiteral("simulationStep"));
            auto* realtime = window.findChild<QCheckBox*>(QStringLiteral("realtime"));
            auto* scene = dynamic_cast<VtkSceneWidget*>(
                window.findChild<QWidget*>(QStringLiteral("simulationScene")));
            require(sidebar && navigation && views && table && frame && drive && duration
                    && step && realtime && scene, "Shared dynamic layout and parameters required.");
            require(sidebar->isVisible() && navigation->isVisible() && !views->isVisible()
                    && table->isVisible() && frame->isVisible() && !duration->isVisible()
                    && !step->isVisible() && !realtime->isVisible() && !drive->isVisible()
                    && activeForce->isVisible() && fixedInputs.front()->isVisible()
                    && fixedInputs.front()->value() == fixedValue,
                "Dynamic workspace must expose shared fixed inputs, not legacy test drive/run parameters.");
            require(frame->count() == 2 && frame->itemText(0) == QStringLiteral("Körpermodell")
                    && frame->itemText(1) == QStringLiteral("Weltmodell")
                    && frame->isVisible(),
                "Body/world selector must remain separately available.");
            require(staticWeight->isChecked(), "Dynamic force selection must not inherit changed static selection.");
            auto* configure = window.findChild<QPushButton*>(QStringLiteral("configureGraphics"));
            configure->click();
            auto* graphicsStack = window.findChild<QStackedWidget*>(QStringLiteral("sceneStack"));
            auto* opacity = graphicsStack->currentWidget()->findChild<QDoubleSpinBox*>(
                QStringLiteral("profile0_waterOpacity"));
            require(opacity, "Dynamic graphics settings must match static controls.");
            opacity->setValue(.37);
            graphicsStack->currentWidget()->findChild<QPushButton*>(QStringLiteral("closeGraphics"))->click();
            tabs->setCurrentIndex(1);
            require(!staticWeight->isChecked(), "Static selection must restore separately.");
            configure->click();
            opacity = graphicsStack->currentWidget()->findChild<QDoubleSpinBox*>(QStringLiteral("profile0_waterOpacity"));
            require(opacity && std::abs(opacity->value() - .37) > .001,
                "Dynamic graphics change must not alter static graphics.");
            graphicsStack->currentWidget()->findChild<QPushButton*>(QStringLiteral("closeGraphics"))->click();
            tabs->setCurrentIndex(2);
            require(staticWeight->isChecked(), "Dynamic selection restores independently.");
            require(!window.findChild<QTreeWidget*>(QStringLiteral("simulationValues"))->isVisible()
                    && !window.findChild<QWidget*>(QStringLiteral("staticForceMenu"))->isHidden()
                    && scene->options().staticForces && !scene->options().forces,
                "Dynamic workspace must replace legacy values/vectors with shared resultants.");
            auto* attitude = window.findChild<QCheckBox*>(QStringLiteral("displayAttitudeViews"));
            attitude->setChecked(false);
            require(!views->isVisible(), "Dynamic attitude toggle must hide the right views.");
            attitude->setChecked(true);
            bool dialogAnswered = false;
            const auto answerRunDialog = [&](bool accept, double seconds) {
                dialogAnswered = false;
                auto* timer = new QTimer(&window);
                timer->setInterval(10);
                QObject::connect(timer, &QTimer::timeout, &window, [&, accept, seconds, timer] {
                    auto* dialog = window.findChild<QDialog*>(QStringLiteral("dynamicRunDialog"));
                    if (!dialog || !dialog->isVisible()) return;
                    timer->stop();
                    timer->deleteLater();
                    dialog->findChild<QDoubleSpinBox*>(QStringLiteral("runDuration"))->setValue(seconds);
                    dialog->findChild<QDoubleSpinBox*>(QStringLiteral("runStep"))->setValue(.01);
                    dialog->findChild<QCheckBox*>(QStringLiteral("runRealtime"))->setChecked(true);
                    dialogAnswered = true;
                    if (accept)
                        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
                    else dialog->reject();
                });
                timer->start();
            };
            answerRunDialog(false, .7);
            run->click();
            waitUntil([&] { return (dialogAnswered && run->isEnabled()) || unexpectedDialog; },
                "Cancelled dynamic start timeout.");
            require(!unexpectedDialog && duration->value() == 10. && table->topLevelItemCount() == 0,
                "Cancelled start must preserve parameters and not run.");
            answerRunDialog(true, .2);
            bool liveInputsEnabled = false;
            QTimer liveInputTimer;
            QObject::connect(&liveInputTimer, &QTimer::timeout, [&] {
                auto* status = window.findChild<QLabel*>(QStringLiteral("status"));
                if (!status || !status->text().startsWith(QStringLiteral("Dynamik läuft"))) return;
                liveInputTimer.stop();
                liveInputsEnabled = activeForce->isEnabled() && fixedInputs.front()->isEnabled();
                if (!liveInputsEnabled) return;
                window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"))->setValue(175.);
                window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryPointZ"))->setValue(2.);
                fixedInputs.front()->setValue(fixedValue + 1.);
            });
            liveInputTimer.start(10);
            run->click();
            waitUntil([&] { return run->isEnabled() || unexpectedDialog; }, "Short dynamic run timeout.");
            require(!unexpectedDialog && liveInputsEnabled && scene->statePose() && table->topLevelItemCount() > 0,
                "Actual dynamic simulation must populate the shared force/moment table.");
            const auto rowCount = table->topLevelItemCount();
            auto* weight = window.findChild<QCheckBox*>(QStringLiteral("staticForce_weight"));
            weight->setChecked(false);
            require(table->topLevelItemCount() == rowCount,
                "Dynamic numerical rows must be independent of graphical selection.");
            weight->setChecked(true);
            auto* filter = window.findChild<QLineEdit*>(QStringLiteral("staticValueFilter2"));
            filter->setText(QStringLiteral("fremdkraft"));
            int visibleRows = 0;
            for (int i = 0; i < rowCount; ++i) {
                auto* row = table->topLevelItem(i);
                if (row->isHidden()) continue;
                ++visibleRows;
                require(row->text(2).contains(QStringLiteral("fremdkraft"), Qt::CaseInsensitive),
                    "Dynamic table filter must hide nonmatching numerical rows.");
            }
            require(visibleRows > 0, "Actual dynamic external-force contribution must be present.");
            filter->clear();
            answerRunDialog(false, .8);
            run->click();
            waitUntil([&] { return (dialogAnswered && run->isEnabled()) || unexpectedDialog; },
                "Later cancelled dynamic start timeout.");
            require(!unexpectedDialog && table->topLevelItemCount() == rowCount && scene->statePose()
                    && duration->value() == .2,
                "Cancelling a later start must retain prior results and confirmed run parameters.");
            const auto pose = scene->statePose();
            for (int i = 0; i < 3; ++i) {
                auto* view = dynamic_cast<VtkSceneWidget*>(window.findChild<QWidget*>(
                    QStringLiteral("staticScene%1").arg(i)));
                require(view && view->statePose() && view->statePose()->position == pose->position
                        && view->options().staticForces,
                    "All dynamic views must receive the displayed Core snapshot.");
            }
            auto* stack = window.findChild<QStackedWidget*>(QStringLiteral("sceneStack"));
            auto* small = dynamic_cast<VtkSceneWidget*>(
                window.findChild<QWidget*>(QStringLiteral("staticScene0")));
            small->onDoubleClicked();
            require(stack->currentWidget() == small, "Dynamic small view must enlarge.");
            scene->onDoubleClicked();
            require(stack->currentWidget() == scene, "Dynamic free view must restore.");
            require(window.grab().save(evidence.filePath(QStringLiteral("dynamic-layout.bmp")), "BMP"),
                "Dynamic layout screenshot.");
            frame->setCurrentIndex(1);
            const auto before = scene->cameraView();
            QJsonObject moved{{QStringLiteral("position_m"), QJsonArray{10., 4., .2}},
                {QStringLiteral("orientierung_xyzw"), QJsonArray{0., 0., 0., 1.}},
                {QStringLiteral("geschwindigkeit_welt_m_s"), QJsonArray{1., 0., 0.}}};
            QString error;
            const auto previousPose = scene->statePose()->position;
            require(scene->setState(moved, {}, &error) && !scene->bodyFixedCamera()
                    && std::abs(scene->cameraView().position[0] - before.position[0]
                        - (10. - previousPose[0])) < 1e-8,
                "World-oriented camera must follow boat translation to keep it centered.");
            const auto worldCamera = scene->cameraView();
            moved.insert(QStringLiteral("orientierung_xyzw"),
                QJsonArray{0., 0., std::sqrt(.5), std::sqrt(.5)});
            require(scene->setState(moved, {}, &error)
                    && scene->cameraView().position == worldCamera.position
                    && scene->cameraView().focalPoint == worldCamera.focalPoint
                    && scene->cameraView().viewUp == worldCamera.viewUp,
                "World-oriented camera must not rotate with the boat.");
            frame->setCurrentIndex(0);
            const auto followed = scene->cameraView();
            moved.insert(QStringLiteral("position_m"), QJsonArray{12., 4., .2});
            require(scene->setState(moved, {}, &error) && scene->bodyFixedCamera()
                    && std::abs(scene->cameraView().position[0] - followed.position[0] - 2.) < 1e-8,
                "Body camera must resume following without a jump.");
            tabs->setCurrentIndex(1);
            require(!duration->isVisible() && !frame->isVisible() && scene->bodyFixedCamera()
                    && table->topLevelItemCount() == 0,
                "Static workspace must retain its controls and clear dynamic results.");
            tabs->setCurrentIndex(2);
            require(duration->value() == .2 && frame->currentIndex() == 0 && table->isVisible(),
                "Dynamic inputs must survive workspace switches.");
            require(activeForce->isChecked()
                    && window.findChild<QDoubleSpinBox*>(QStringLiteral("stationaryForceX"))->value() == 175.
                    && fixedInputs.front()->value() == fixedValue + 1.,
                "Shared physical inputs survive both workspace directions.");
            require(!unexpectedDialog && renderErrors->count == 0, "Dynamic layout must not raise errors.");
            window.close();
            MainWindow restored;
            restored.setModelFolder(app.arguments()[2]);
            restored.selectInitialModel(QStringLiteral("folkeboot"));
            restored.show();
            auto* restoredRun = restored.findChild<QPushButton*>(QStringLiteral("primaryButton"));
            waitUntil([&] { return restoredRun->isEnabled() || unexpectedDialog; }, "Restart model load.");
            auto* restoredTabs = restored.findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
            auto* restoredScene = dynamic_cast<VtkSceneWidget*>(
                restored.findChild<QWidget*>(QStringLiteral("simulationScene")));
            restoredTabs->setCurrentIndex(1);
            require(!restored.findChild<QCheckBox*>(QStringLiteral("staticForce_weight"))->isChecked()
                    && std::abs(restoredScene->graphicsProfile().waterOpacity - .37) > 1e-6,
                "Restart must restore static display without dynamic edits.");
            restoredTabs->setCurrentIndex(2);
            require(restored.findChild<QCheckBox*>(QStringLiteral("staticForce_weight"))->isChecked()
                    && std::abs(restoredScene->graphicsProfile().waterOpacity - .37) < 1e-6,
                "Restart must restore dynamic graphics and force selection independently.");
            require(!unexpectedDialog && renderErrors->count == 0, "Restart display restoration failed.");
            restored.close();
            std::cout << "Dynamic layout, live Core snapshots, table, view swaps and body/world camera passed.\n";
            return 0;
        }
        if (app.arguments().contains(QStringLiteral("--column-boundaries-only"))) {
            QTemporaryDir settings;
            require(settings.isValid(), "Temporary column-test settings directory required.");
            QCoreApplication::setOrganizationName(QStringLiteral("SegelPhysikColumnTest"));
            QCoreApplication::setApplicationName(QStringLiteral("ColumnBoundaries"));
            QSettings::setDefaultFormat(QSettings::IniFormat);
            QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
            MainWindow window;
            window.show();
            for (const int workspace : {0, 1, 2}) {
                window.findChild<QTabBar*>(QStringLiteral("workspaceTabs"))->setCurrentIndex(workspace);
                auto* table = window.findChild<QTreeWidget*>(workspace == 0
                    ? QStringLiteral("inspectionValues") : QStringLiteral("staticForceValues"));
                // Bootsauswahl zeigt die Wertetabelle normalerweise erst nach Objektauswahl.
                for (auto* parent = table->parentWidget(); parent && parent != &window;
                     parent = parent->parentWidget()) parent->show();
                table->show();
                QApplication::processEvents();
                auto* header = table->header();
                auto* viewport = header->viewport();
                require(viewport->width() > 0 && viewport->height() > 0,
                    "Column header must be visible for raster/cursor checks.");
                for (int section = 0; section < 2; ++section) {
                    const int edge = header->sectionViewportPosition(section) + header->sectionSize(section);
                    const auto image = viewport->grab().toImage();
                    int bright = 0;
                    for (int y = 2; y < image.height() - 2; ++y) {
                        const auto pixel = image.pixelColor(edge - 1, y);
                        if (pixel.red() > 140 && pixel.green() > 160 && pixel.blue() > 170) ++bright;
                    }
                    require(bright >= image.height() - 6,
                        "Light vertical separator must span both header title and filter row.");
                    for (const int y : {5, viewport->height() - 5}) {
                        const QPointF point(edge - 1, y);
                        QMouseEvent move(QEvent::MouseMove, point,
                            viewport->mapToGlobal(point.toPoint()), Qt::NoButton,
                            Qt::NoButton, Qt::NoModifier);
                        QApplication::sendEvent(viewport, &move);
                        require(viewport->cursor().shape() == Qt::SizeHorCursor
                                && header->cursor().shape() == Qt::SizeHorCursor,
                            "Resizable boundary must show the horizontal double-arrow cursor.");
                    }
                    const QPointF centre(header->sectionViewportPosition(section) + 10, 5);
                    QMouseEvent move(QEvent::MouseMove, centre,
                        viewport->mapToGlobal(centre.toPoint()), Qt::NoButton,
                        Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(viewport, &move);
                    require(viewport->cursor().shape() != Qt::SizeHorCursor
                            && header->cursor().shape() != Qt::SizeHorCursor,
                        "Header away from a boundary must not show resize cursor.");
                }
            }
            window.close();
            std::cout << "Column separators and resize cursor passed in all three workspaces.\n";
            return 0;
        }
        if (app.arguments().size() == 2
            && app.arguments()[1] == QStringLiteral("--inspection-pointer-only")) {
            testInspectionCameraAndPointerRouting();
            require(renderErrors->count == 0, "VTK error in inspection pointer test");
            std::cout << "{\"inspectionCameraAxesAndPointerRouting\":true}\n";
            return 0;
        }
        if (app.arguments().size() == 3
            && app.arguments()[1] == QStringLiteral("--unified-montage-integration")) {
            runUnifiedMontageIntegration(app.arguments()[2]);
            require(renderErrors->count == 0, "VTK error in unified object montage");
            return 0;
        }
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
        if (app.arguments().size() == 4 && app.arguments()[1] == QStringLiteral("--water-reflection-only")) {
            captureWaterReflection(app.arguments()[2], app.arguments()[3]);
            require(renderErrors->count == 0, "VTK error in water reflection capture");
            return 0;
        }
        if (app.arguments().size() == 4 && app.arguments()[1] == QStringLiteral("--water-motion-only")) {
            testWaterMotion(app.arguments()[2], app.arguments()[3]);
            require(renderErrors->count == 0, "VTK error in focused water-motion test");
            return 0;
        }
        if (app.arguments().size() == 3 && app.arguments()[1] == QStringLiteral("--wind-input-only")) {
            testWindInputPrecision(app.arguments()[2]);
            require(renderErrors->count == 0, "VTK error in wind input test");
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
        const auto modelPath = QDir(app.arguments()[1]).filePath(QStringLiteral("folkeboot/modells_information.json"));
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
        const auto naturalImage = scene.captureImage();
        const auto naturalMaterial = digest(naturalImage);
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
        const auto restoredImage = scene.captureImage();
        testStableImageComparison();
        if (!sameStableImage(naturalImage, restoredImage)) {
            naturalImage.save(output.filePath(QStringLiteral("material_before_reset.bmp")), "BMP");
            restoredImage.save(output.filePath(QStringLiteral("material_after_reset.bmp")), "BMP");
            std::cerr << "Material restore differing pixels: "
                      << differingPixels(naturalImage, restoredImage) << '\n';
        }
        require(sameStableImage(naturalImage, restoredImage),
            "preview reset did not restore actual surface materials");
        require(scene.captureImage().save(output.filePath(QStringLiteral("natural_materials_optik.bmp")), "BMP"),
            "natural material screenshot");
        report.insert(QStringLiteral("materialDemonstration"),
            QStringLiteral("Persistent semantic materials captured without preview; four all-part previews reset within sparse pixel differences"));
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
        const auto pausedImage = scene.captureImage();
        if (!sameStableImage(frozen, pausedImage)) {
            frozen.save(output.filePath(QStringLiteral("paused_before.bmp")), "BMP");
            pausedImage.save(output.filePath(QStringLiteral("paused_after.bmp")), "BMP");
            std::cerr << "Paused water differing pixels: "
                      << differingPixels(frozen, pausedImage) << '\n';
        }
        require(sameStableImage(frozen, pausedImage), "paused water animation changed");
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
        const auto staticPreview = scene.captureImage();
        scene.renderNow();
        const auto staticPreviewAfter = scene.captureImage();
        if (!sameStableImage(staticPreview, staticPreviewAfter)) {
            staticPreview.save(output.filePath(QStringLiteral("static_before.bmp")), "BMP");
            staticPreviewAfter.save(output.filePath(QStringLiteral("static_after.bmp")), "BMP");
            std::cerr << "Static preview differing pixels: "
                      << differingPixels(staticPreview, staticPreviewAfter) << '\n';
        }
        require(sameStableImage(staticPreview, staticPreviewAfter), "no-state water preview is not static");
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
        const auto reloadedImage = scene.captureImage();
        if (!sameStableImage(firstImage, reloadedImage)) {
            firstImage.save(output.filePath(QStringLiteral("reload_before.bmp")), "BMP");
            reloadedImage.save(output.filePath(QStringLiteral("reload_after.bmp")), "BMP");
            std::cerr << "Model reload differing pixels: "
                      << differingPixels(firstImage, reloadedImage) << '\n';
        }
        require(sameStableImage(firstImage, reloadedImage),
            "model reload retained stale material or render state");
        require(renderErrors->count == 0, "VTK errors in heeled/static/model-switch cases");
        report.insert(QStringLiteral("staticNoStatePreview"), true);
        report.insert(QStringLiteral("modelSwitch"), QStringLiteral("folkeboot -> quader -> folkeboot; image restored within sparse pixel differences"));
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
