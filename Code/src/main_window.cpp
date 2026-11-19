#include "main_window.hpp"

#include "vtk_scene.hpp"
#include "graphics_dialog.hpp"
#include "navigation.hpp"
#include "boat_inspection.hpp"
#include "filter_header.hpp"

#include <segelphysik/defaults.hpp>
#include <segelphysik/hydrostatik.hpp>
#include <segelphysik/modell_fingerprints.hpp>
#include <segelphysik/sailforce.hpp>
#include <segelphysik/modell.hpp>
#include <segelphysik/pose.hpp>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QElapsedTimer>
#include <thread>
#include <QJsonDocument>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QMetaObject>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QStyleOptionTab>
#include <QStackedWidget>
#include <QTabBar>
#include <QThread>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <utility>
#include <thread>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

using forcedisplay::Contribution;

namespace {

constexpr int staticRawValueRole = Qt::UserRole + 10;

void waitForDisplayDeadline(std::chrono::steady_clock::time_point deadline)
{
#ifdef Q_OS_WIN
    // Keep display publication independent of the coarse Windows Sleep timer.
    const auto remaining = std::chrono::duration_cast<std::chrono::nanoseconds>(
        deadline - std::chrono::steady_clock::now());
    if (remaining.count() <= 0) return;
    const HANDLE timer = CreateWaitableTimerExW(
        nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (timer) {
        const auto closeTimer = qScopeGuard([timer] { CloseHandle(timer); });
        LARGE_INTEGER due{};
        due.QuadPart = -std::max<LONGLONG>(1, remaining.count() / 100);
        if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE)
            && WaitForSingleObject(timer, INFINITE) == WAIT_OBJECT_0) return;
    }
#endif
    std::this_thread::sleep_until(deadline);
}

class StaticValueItem final : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem& other) const override
    {
        const int column = treeWidget()->sortColumn();
        const auto a = data(column, staticRawValueRole);
        const auto b = other.data(column, staticRawValueRole);
        if (column >= 3 && column <= 6 && (a.isValid() || b.isValid())) {
            if (a.isValid() != b.isValid()) return a.isValid();
            return a.toDouble() < b.toDouble();
        }
        return QString::localeAwareCompare(text(column), other.text(column)) < 0;
    }
};

class StaticNavigationColumn final : public QScrollArea {
public:
    StaticNavigationColumn()
    {
        setWidgetResizable(true);
        setFixedWidth(200);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFrameShape(QFrame::NoFrame);
    }
protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QScrollArea::resizeEvent(event);
        if (!widget()) return;
        auto* speed = widget()->findChild<QWidget*>(QStringLiteral("navigationSpeed"));
        if (!speed) return;
        const int instrumentHeight = std::max(100, (height() - speed->sizeHint().height() - 18) / 3);
        setFixedWidth(instrumentHeight);
        widget()->setFixedWidth(instrumentHeight);
    }
};

class StaticViewsColumn final : public QWidget {
public:
    StaticViewsColumn(std::array<VtkSceneWidget*, 3>& scenes, std::array<QWidget*, 3>& viewSlots)
    {
        setObjectName(QStringLiteral("staticViewsColumn"));
        const std::array<QString, 3> names{
            QStringLiteral("Seite"), QStringLiteral("Heck"), QStringLiteral("Draufsicht")};
        const std::array<VtkSceneWidget::OrthographicView, 3> views{
            VtkSceneWidget::OrthographicView::Port, VtkSceneWidget::OrthographicView::Stern,
            VtkSceneWidget::OrthographicView::Top};
        for (std::size_t i = 0; i < scenes.size(); ++i) {
            viewSlots[i] = new QWidget(this);
            viewSlots[i]->setObjectName(QStringLiteral("staticViewSlot%1").arg(i));
            auto* layout = new QVBoxLayout(viewSlots[i]);
            layout->setContentsMargins(0, 0, 0, 0);
            scenes[i] = new VtkSceneWidget(viewSlots[i]);
            layout->addWidget(scenes[i]);
            scenes[i]->setObjectName(QStringLiteral("staticScene%1").arg(i));
            scenes[i]->setNavigationVisible(false);
            scenes[i]->setOrthographicView(views[i]);
            scenes[i]->setViewName(names[i]);
        }
        slots_ = viewSlots;
        setFixedWidth(150);
    }

protected:
    bool event(QEvent* event) override
    {
        const bool handled = QWidget::event(event);
        if (event->type() == QEvent::LayoutRequest) updateViewLayout();
        return handled;
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        updateViewLayout();
    }

private:
    void updateViewLayout()
    {
        const int gap = 6;
        int minimumImageHeight = 1;
        for (auto* slot : slots_) {
            const auto minimum = slot->layout()->minimumSize();
            minimumImageHeight = std::max(minimumImageHeight,
                std::max(minimum.height(), static_cast<int>(
                    std::ceil(4.0 * minimum.width() / 3.0))));
        }
        setMinimumHeight(3 * minimumImageHeight + 2 * gap);
        const int imageHeight = std::max(minimumImageHeight, (height() - 2*gap) / 3);
        const int imageWidth = qRound(3.0 * imageHeight / 4.0);
        setFixedWidth(imageWidth);
        for (std::size_t i = 0; i < slots_.size(); ++i) {
            const int y = static_cast<int>(i) * (imageHeight + gap);
            slots_[i]->setGeometry(0, y, imageWidth, imageHeight);
        }
    }

   std::array<QWidget*, 3> slots_{};
};

struct WorkspaceTheme {
    const char* label;
    const char* background;
    const char* panel;
    const char* border;
    const char* input;
    const char* button;
    const char* hover;
    const char* primary;
    const char* accent;
    const char* muted;
    const char* caption;
    const char* alternate;
};

const std::array<WorkspaceTheme, 3> workspaceThemes{{
    {"Bootsauswahl", "#1c1118", "#2b1923", "#65404d", "#21131b",
        "#422632", "#593342", "#85334b", "#e7a0b3", "#c5a6b0", "#d4a0af", "#321e28"},
    {"Statisches Modell", "#0b1420", "#111e2c", "#344c61", "#0c1722",
        "#1b2b3d", "#263d52", "#244e7a", "#91bde9", "#a5bacb", "#88a3b9", "#162636"},
    {"Dynamisches Modell", "#0d1b16", "#142a20", "#3b6250", "#102219",
        "#203c2e", "#2b503c", "#256344", "#9dd9b5", "#a1c0ae", "#94bfa6", "#1b3427"}
}};

class WorkspaceTabBar final : public QTabBar {
public:
    explicit WorkspaceTabBar(QWidget* parent)
        : QTabBar(parent)
    {
        setObjectName(QStringLiteral("workspaceTabs"));
        setAccessibleName(QStringLiteral("Arbeitsbereich"));
        setExpanding(false);
        setDrawBase(false);
        for (const auto& theme : workspaceThemes) {
            addTab(QString::fromUtf8(theme.label));
        }
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        for (int index = 0; index < count(); ++index) {
            const auto& theme = workspaceThemes.at(index);
            QStyleOptionTab option;
            initStyleOption(&option, index);
            const auto rect = tabRect(index).adjusted(0, 0, -6, -1);
            const bool selected = index == currentIndex();
            painter.setBrush(QColor(selected ? theme.primary : theme.panel));
            painter.setPen(QPen(QColor(selected ? theme.accent : theme.border), 1));
            painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 7, 7);
            painter.fillRect(rect.adjusted(7, 2, -7, -rect.height() + 5),
                QColor(theme.primary));
            painter.setPen(QColor(selected ? "#ffffff" : theme.accent));
            painter.drawText(rect.adjusted(15, 4, -15, -4), Qt::AlignCenter, option.text);
            if (selected) {
                painter.fillRect(QRect(rect.left() + 7, rect.bottom() - 3,
                    rect.width() - 14, 3), QColor(theme.accent));
            }
            if (option.state & QStyle::State_HasFocus) {
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(theme.accent), 1, Qt::DotLine));
                painter.drawRoundedRect(rect.adjusted(5, 5, -5, -5), 4, 4);
            }
        }
    }
};

QString runtimeText(double seconds)
{
    if (seconds < 1.0) {
        return QStringLiteral("%1 ms").arg(seconds * 1000.0, 0, 'f', 2);
    }
    return QStringLiteral("%1 s").arg(seconds, 0, 'f', 3);
}

QString movementValueText(double requested, double confirmed, const QString& unit)
{
    const auto suffix = unit == QStringLiteral("deg") ? QStringLiteral("°")
        : unit == QStringLiteral("%") ? QStringLiteral(" %")
        : QStringLiteral(" ") + unit;
    return QStringLiteral("Eingabe %1%2 · Core %3%2")
        .arg(requested, 0, 'f', 1).arg(suffix).arg(confirmed, 0, 'f', 1);
}

const QString folderSettingKey = QStringLiteral("modellordner");
const QString modelSettingKey = QStringLiteral("modell_id");
const QString legacyModelSettingKey = QStringLiteral("modell");
constexpr int liveIntervalMs = 16;
constexpr double gravity = 9.81;
const QColor rejectedModelColor(0x8a, 0x9b, 0xab);
const QChar warningSign(0x26A0);

enum Column { ColName, ColMagnitude, ColX, ColY, ColZ, ColState, ColScale, ColumnCount };

QString samePath(const QString& path)
{
    return QFileInfo(path).absoluteFilePath();
}

QString cacheKey(const QString& id, const QString& version)
{
    return id + QLatin1Char('\n') + version;
}

QString vectorText(const std::optional<forcedisplay::Vec3>& v, const QString& unit)
{
    if (!v) {
        return QStringLiteral("nicht geliefert");
    }
    return QStringLiteral("(%1; %2; %3)")
        .arg(forcedisplay::formatQuantity((*v)[0], unit), forcedisplay::formatQuantity((*v)[1], unit),
            forcedisplay::formatQuantity((*v)[2], unit));
}

QString scaleText(const std::optional<forcedisplay::ScaledMagnitude>& scaled)
{
    if (!scaled) {
        return QStringLiteral("–");
    }
    if (scaled->overScale) {
        return QStringLiteral("über Skala");
    }
    QString text = QStringLiteral("%1 %").arg(scaled->u * 100.0, 0, 'f', 0);
    if (scaled->smallValue) {
        text += QStringLiteral(" · Kleinwert");
    }
    return text;
}

QString referenceSettingKey(const QString& modelId, const QString& kind)
{
    return QStringLiteral("referenzen/%1/%2").arg(modelId, kind);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , scene_(new VtkSceneWidget(this))
{
    scene_->setObjectName(QStringLiteral("simulationScene"));
    setWindowTitle(QStringLiteral("SegelPhysik – Hydrostatik"));
    resize(1500, 940);
    setMinimumSize(1024, 680);
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks);
    auto* settingsMenu = menuBar()->addMenu(QStringLiteral("Settings"));
    airSettingsAction_ = settingsMenu->addAction(QStringLiteral("Lufteigenschaften"));
    connect(airSettingsAction_, &QAction::triggered, this, [this] { setupAirProperties(); });

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(12);

    auto* workspaceTabs = new WorkspaceTabBar(central);
    connect(workspaceTabs, &QTabBar::currentChanged, this, &MainWindow::updateWorkspace);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("SEGELPHYSIK"));
    title->setObjectName(QStringLiteral("title"));
    auto* subtitle = new QLabel(QStringLiteral("Hydrostatik · Gleichgewicht · Bewegung"));
    subtitle->setObjectName(QStringLiteral("subtitle"));
    auto* titleBox = new QVBoxLayout;
    titleBox->setSpacing(2);
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);
    header->addLayout(titleBox);
    header->addStretch();
    chooseFolderButton_ = new QPushButton(QStringLiteral("Modellordner wählen"));
    chooseFolderButton_->setObjectName(QStringLiteral("primaryButton"));
    refreshModelsButton_ = new QPushButton(QStringLiteral("Modelle aktualisieren"));
    refreshModelsButton_->setObjectName(QStringLiteral("refreshModelsButton"));
    root->addLayout(header);
    auto* workspaceRow = new QHBoxLayout;
    workspaceRow->addWidget(workspaceTabs);
    workspaceRow->addStretch();
    modelLabel_ = new QLabel(QStringLiteral("Kein Modell ausgewählt"));
    modelLabel_->setObjectName(QStringLiteral("modelName"));
    modelLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    modelLabel_->setTextFormat(Qt::PlainText);
    workspaceRow->addWidget(modelLabel_);
    root->addLayout(workspaceRow);

    auto* mainArea = new QHBoxLayout;
    mainArea_ = mainArea;
    mainArea->setSpacing(14);

    auto* catalogPanel = new QWidget;
    catalogPanel_ = catalogPanel;
    catalogPanel->setObjectName(QStringLiteral("panel"));
    catalogPanel->setMinimumWidth(210);
    catalogPanel->setMaximumWidth(260);
    auto* catalogLayout = new QVBoxLayout(catalogPanel);
    catalogLayout->setContentsMargins(14, 18, 14, 14);
    catalogLayout->setSpacing(10);
    auto* cameraCaption = new QLabel(QStringLiteral("KAMERA"));
    cameraCaption->setObjectName(QStringLiteral("caption"));
    inspectionCameraCombo_ = new QComboBox;
    inspectionCameraCombo_->setObjectName(QStringLiteral("inspectionCameraPreset"));
    inspectionCameraCombo_->addItem(QStringLiteral("Freie Ansicht (Maus)"), QString());
    for (const auto& preset : navigation::cameraPresets())
        inspectionCameraCombo_->addItem(QString::fromUtf8(preset.label),
            QString::fromLatin1(preset.id));
    inspectionCameraCombo_->setEnabled(false);
    inspectionCameraCombo_->setToolTip(QStringLiteral(
        "Sechs achsenfeste Ansichten sind orthografisch und erlauben die Segelbearbeitung. "
        "Schräge Ansichten dienen nur der Betrachtung."));
    auto* catalogCaption = new QLabel(QStringLiteral("MODELLE"));
    catalogCaption->setObjectName(QStringLiteral("caption"));
    folderLabel_ = new QLabel(QStringLiteral("Kein Modellordner"));
    folderLabel_->setObjectName(QStringLiteral("time"));
    folderLabel_->setWordWrap(true);
    modelList_ = new QListWidget;
    modelList_->setObjectName(QStringLiteral("modelList"));
    modelList_->setSelectionMode(QAbstractItemView::SingleSelection);
    modelList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    modelList_->setTextElideMode(Qt::ElideRight);
    modelList_->setContextMenuPolicy(Qt::CustomContextMenu);
    catalogLayout->addWidget(cameraCaption);
    catalogLayout->addWidget(inspectionCameraCombo_);
    catalogLayout->addWidget(catalogCaption);
    catalogLayout->addWidget(folderLabel_);
    catalogLayout->addWidget(modelList_, 1);
    catalogLayout->addWidget(refreshModelsButton_);
    catalogLayout->addWidget(chooseFolderButton_);
    mainArea->addWidget(catalogPanel);
    workspaceStack_ = new QStackedWidget;
    workspaceStack_->setObjectName(QStringLiteral("workspaceStack"));
    inspection_ = new BoatInspection;
    inspection_->onSailConfigurationSaving = [this] {
        sailSaveInProgress_ = true;
        setBusy(true);
    };
    inspection_->onManualModeChanged = [this](bool) {
        setBusy(openThread_ != nullptr || simulationThread_ != nullptr);
    };
    inspection_->onSailConfigurationFinished = [this](const QString& id, bool saved,
        const QString& revision, std::shared_ptr<const segelphysik::Modellkatalog> updatedCatalog,
        const QString& catalogError) {
        sailSaveInProgress_ = false;
        if (!saved && inspection_->manualModeActive()) {
            closeAfterSailSave_ = false;
            auto* tabs = findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
            if (tabs && tabs->currentIndex() != 0) tabs->setCurrentIndex(0);
            statusLabel_->setText(QStringLiteral(
                "Segeländerungen nicht gespeichert; die Vorschau bleibt erhalten."));
            statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
            setBusy(openThread_ != nullptr || simulationThread_ != nullptr);
            return;
        }
        if (saved && closeAfterSailSave_) {
            closeAfterSailSave_ = false;
            QTimer::singleShot(0, this, [this] { close(); });
            return;
        }
        closeAfterSailSave_ = false;
        modelCache_.clear();
        if (saved && id == modelId_ && updatedCatalog && !catalogReloadPending_) {
            auto updated = catalog_;
            updated.catalog = std::move(updatedCatalog);
            for (auto& entry : updated.entries)
                if (entry.id == id) entry.version = revision;
            reopenAfterCatalog_ = true;
            applyCatalog(updated);
            return;
        }
        if (saved && !catalogError.isEmpty())
            showError(QStringLiteral("Segel gespeichert, Modellaktualisierung fehlgeschlagen"),
                catalogError);
        if (id == modelId_) reopenAfterCatalog_ = true;
        if (!saved) {
            statusLabel_->setText(QStringLiteral("Segelkonfiguration nicht bestätigt; Modell wird geprüft."));
            statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
        }
        reloadCatalog();
        setBusy(openThread_ != nullptr || simulationThread_ != nullptr);
    };
    workspaceStack_->addWidget(inspection_);
    connect(inspectionCameraCombo_, &QComboBox::activated, this, [this] {
        QString error;
        if (!inspection_->setCameraPreset(inspectionCameraCombo_->currentData().toString(), &error))
            showError(QStringLiteral("Kameraansicht"), error);
    });
    auto* simulationPanel = new QWidget;
    auto* simulationLayout = new QHBoxLayout(simulationPanel);
    simulationLayout->setContentsMargins(0, 0, 0, 0);
    simulationLayout->setSpacing(14);
    sceneStack_ = new QStackedWidget;
    sceneStack_->setObjectName(QStringLiteral("sceneStack"));
    sceneStack_->addWidget(scene_);
    largeScene_ = scene_;
    staticNavigationColumn_ = new StaticNavigationColumn;
    staticNavigationColumn_->setObjectName(QStringLiteral("staticNavigationColumn"));
    simulationLayout->addWidget(staticNavigationColumn_);
    staticNavigationColumn_->hide();
    simulationLayout->addWidget(sceneStack_, 1);
    staticViewsColumn_ = new StaticViewsColumn(staticScenes_, staticViewSlots_);
    for (auto* view : staticScenes_)
        view->onDoubleClicked = [this, view] { swapStaticView(view); };
    simulationLayout->addWidget(staticViewsColumn_);
    staticViewsColumn_->hide();
    workspaceStack_->addWidget(simulationPanel);
    mainArea->addWidget(workspaceStack_, 1);

    auto* panel = new QWidget;
    panel->setObjectName(QStringLiteral("panel"));
    auto* panelLayout = new QVBoxLayout(panel);
    simulationControlsLayout_ = panelLayout;
    panelLayout->setContentsMargins(18, 18, 18, 18);
    panelLayout->setSpacing(10);

    auto* modeCaption = new QLabel(QStringLiteral("SIMULATION"));
    modeCaption->setObjectName(QStringLiteral("caption"));
    modeCombo_ = new QComboBox;
    modeCombo_->setObjectName(QStringLiteral("simulationMode"));
    modeCombo_->addItem(QStringLiteral("Statisches Gleichgewicht"), false);
    modeCombo_->addItem(QStringLiteral("Dynamischer Verlauf"), true);
    panelLayout->addWidget(modeCaption);
    panelLayout->addWidget(modeCombo_);

    parameterForm_ = new QFormLayout;
    durationSpin_ = new QDoubleSpinBox;
    durationSpin_->setObjectName(QStringLiteral("simulationDuration"));
    durationSpin_->setRange(0.0, 3600.0);
    durationSpin_->setDecimals(3);
    durationSpin_->setSingleStep(0.1);
    durationSpin_->setValue(10.0);
    durationSpin_->setSuffix(QStringLiteral(" s"));
    durationSpin_->setSpecialValueText(QStringLiteral("bis Abbruch"));
    stepSpin_ = new QDoubleSpinBox;
    stepSpin_->setObjectName(QStringLiteral("simulationStep"));
    stepSpin_->setRange(0.0001, 1.0);
    stepSpin_->setDecimals(4);
    stepSpin_->setSingleStep(0.01);
    stepSpin_->setValue(0.01);
    stepSpin_->setSuffix(QStringLiteral(" s"));
    durationSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    stepSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    parameterForm_->addRow(QStringLiteral("Dauer"), durationSpin_);
    parameterForm_->addRow(QStringLiteral("Schrittweite"), stepSpin_);
    realtimeCheck_ = new QCheckBox(QStringLiteral("Echtzeit (1 s = 1 s)"));
    realtimeCheck_->setObjectName(QStringLiteral("realtime"));
    realtimeCheck_->setChecked(true);
    realtimeCheck_->setToolTip(QStringLiteral(
        "Ein: Der Core rechnet jeden Schritt erst, wenn seine Simulationszeit "
        "erreicht ist.\nAus: Der Core rechnet ungebremst; die Anzeige folgt "
        "so schnell wie sinnvoll."));
    parameterForm_->addRow(QStringLiteral("Takt"), realtimeCheck_);
    panelLayout->addLayout(parameterForm_);

    runButton_ = new QPushButton(QStringLiteral("Core-Lauf starten"));
    runButton_->setObjectName(QStringLiteral("primaryButton"));
    runButton_->setProperty("sailforceRunButton", true);
    cancelButton_ = new QPushButton(QStringLiteral("Lauf stoppen"));
    cancelButton_->setObjectName(QStringLiteral("stopSimulation"));
    cancelButton_->setEnabled(false);
    panelLayout->addWidget(runButton_);
    panelLayout->addWidget(cancelButton_);

    stationaryForcePanel_ = new QGroupBox(QStringLiteral("Eine Fremdkraft"));
    stationaryForcePanel_->setObjectName(QStringLiteral("stationaryForcePanel"));
    auto* stationaryForm = new QFormLayout(stationaryForcePanel_);
    stationaryForceActive_ = new QCheckBox(QStringLiteral("Fremdkraft aktiv"));
    stationaryForceActive_->setObjectName(QStringLiteral("stationaryForceActive"));
    stationaryForm->addRow(stationaryForceActive_);
    auto* forceVector = new QGroupBox(QStringLiteral("Kraftvektor (X, Y, Z)"));
    auto* forceVectorForm = new QFormLayout(forceVector);
    auto* applicationPoint = new QGroupBox(QStringLiteral("Gemeinsamer Ansatzpunkt (Modell)"));
    auto* applicationPointForm = new QFormLayout(applicationPoint);
    stationaryForm->addRow(forceVector);
    stationaryForm->addRow(applicationPoint);
    for (int axis = 0; axis < 3; ++axis) {
        const auto axisName = QString::fromLatin1("XYZ").mid(axis,1);
        auto* force = new QDoubleSpinBox;
        force->setObjectName(QStringLiteral("stationaryForce%1").arg(axisName));
        force->setRange(-1e8,1e8);
        force->setDecimals(3);
        force->setKeyboardTracking(false);
        force->setSuffix(QStringLiteral(" N"));
        stationaryForce_[axis] = force;
        forceVectorForm->addRow(axisName, force);
        auto* point = new QDoubleSpinBox;
        point->setObjectName(QStringLiteral("stationaryPoint%1").arg(axisName));
        point->setRange(-1e6,1e6);
        point->setDecimals(4);
        point->setKeyboardTracking(false);
        point->setSuffix(QStringLiteral(" m"));
        stationaryPoint_[axis] = point;
        applicationPointForm->addRow(axisName, point);
    }
    panelLayout->addWidget(stationaryForcePanel_);

    windPanel_ = new QGroupBox(QStringLiteral("Wahrer Wind"));
    windPanel_->setObjectName(QStringLiteral("windPanel"));
    auto* windForm = new QFormLayout(windPanel_);
    windDirectionEdit_ = new QLineEdit(QStringLiteral("0,0"));
    windDirectionEdit_->setObjectName(QStringLiteral("windDirection"));
    windSpeedEdit_ = new QLineEdit(QStringLiteral("5,0"));
    windSpeedEdit_->setObjectName(QStringLiteral("windSpeed"));
    windHeightEdit_ = new QLineEdit(QStringLiteral("10,0"));
    windHeightEdit_->setObjectName(QStringLiteral("windHeight"));
    windMode_ = new QComboBox;
    windMode_->setObjectName(QStringLiteral("windMode"));
    windMode_->addItem(QStringLiteral("Weltfest"), false);
    windMode_->addItem(QStringLiteral("Mit Boot mitgeführt"), true);
    windStatus_ = new QLabel;
    windStatus_->setWordWrap(true);
    windForm->addRow(QStringLiteral("Herkunft (°)"), windDirectionEdit_);
    windForm->addRow(QStringLiteral("v0 (m/s)"), windSpeedEdit_);
    windForm->addRow(QStringLiteral("h0 (m)"), windHeightEdit_);
    windForm->addRow(QStringLiteral("Dynamik"), windMode_);
    windForm->addRow(windStatus_);
    panelLayout->addWidget(windPanel_);
    sailPanel_ = new QGroupBox(QStringLiteral("Segelphysik"));
    sailLayout_ = new QFormLayout(sailPanel_);
    sailStatus_ = new QLabel(QStringLiteral("Keine explizite Segelmodellzuordnung."));
    sailStatus_->setWordWrap(true);
    panelLayout->addWidget(sailPanel_);
    const auto windInput = [this](QLineEdit* edit, double& stored, bool direction, bool height) {
        if (!edit->isModified()) return;
        bool ok = false;
        auto raw = edit->text().trimmed();
        raw.replace(QLatin1Char(','), QLatin1Char('.'));
        const double parsed = raw.toDouble(&ok);
        const double before = stored;
        if (!ok || !std::isfinite(parsed)) {
            windStatus_->setText(QStringLiteral("Ungültige Zahl; vorheriger Wert bleibt erhalten."));
        } else {
            double value = parsed;
            if (direction) {
                value = std::fmod(value, 360.);
                if (value < 0.) value += 360.;
            } else {
                value = std::clamp(value, .01, 100.);
            }
            QString error;
            if (live_ && simulationThread_ && lastRunDynamic_.value_or(false) &&
                !height && !live_->setWind(direction ? std::optional<double>(value) : std::nullopt,
                    direction ? std::nullopt : std::optional<double>(value), {}, &error)) {
                windStatus_->setText(error);
            } else {
                stored = value;
                windStatus_->setText(value != parsed && !direction
                    ? QStringLiteral("Wert auf den zulässigen Bereich begrenzt.")
                    : live_ && simulationThread_
                        ? QStringLiteral("Änderung für den nächsten Schritt angefordert.")
                        : QString());
            }
        }
        if (stored == before && (!ok || !std::isfinite(parsed)))
            edit->setToolTip(QStringLiteral("Vorheriger gültiger Wert bleibt erhalten."));
        else edit->setToolTip(QString());
        edit->setText(QString::number(stored, 'f', 1).replace(QLatin1Char('.'), QLatin1Char(',')));
    };
    connect(windDirectionEdit_, &QLineEdit::editingFinished, this,
        [this, windInput] { windInput(windDirectionEdit_, windDirection_, true, false); });
    connect(windSpeedEdit_, &QLineEdit::editingFinished, this,
        [this, windInput] { windInput(windSpeedEdit_, windSpeed_, false, false); });
    connect(windHeightEdit_, &QLineEdit::editingFinished, this,
        [this, windInput] { windInput(windHeightEdit_, windHeight_, false, true); });
    connect(windMode_, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (!live_ || !simulationThread_ || !lastRunDynamic_.value_or(false)) return;
        QString error;
        if (!live_->setWind({}, {}, windMode_->itemData(index).toBool(), &error)) {
            const QSignalBlocker blocker(windMode_);
            windMode_->setCurrentIndex(index == 0 ? 1 : 0);
            windStatus_->setText(error);
        } else windStatus_->setText(QStringLiteral("Moduswechsel für den nächsten Schritt angefordert."));
    });

    testDrivePanel_ = new QGroupBox(QStringLiteral("Testantrieb (temporaer)"));
    testDrivePanel_->setObjectName(QStringLiteral("testDrivePanel"));
    auto* testLayout = new QFormLayout(testDrivePanel_);
    testDriveCheck_ = new QCheckBox(QStringLiteral("Testantrieb aktiv"));
    testDriveCheck_->setObjectName(QStringLiteral("testDriveActive"));
    testDriveCheck_->setToolTip(QStringLiteral(
        "Wirkt am naechsten unberechneten Schritt des laufenden Versuchs, auch im Rueckblick. "
        "Aus entfernt Schub UND Geschwindigkeitsbegrenzer, stoppt aber nicht das Boot."));
    testForceSpin_ = new QDoubleSpinBox;
    testForceSpin_->setObjectName(QStringLiteral("testDriveForce"));
    testForceSpin_->setRange(0.0, 100000.0);
    testForceSpin_->setDecimals(2);
    testForceSpin_->setValue(200.0);
    testForceSpin_->setSuffix(QStringLiteral(" N"));
    testSpeedSpin_ = new QDoubleSpinBox;
    testSpeedSpin_->setObjectName(QStringLiteral("testDriveSpeed"));
    testSpeedSpin_->setDecimals(8);
    testSpeedSpin_->setRange(0.01, 20.0);
    testSpeedSpin_->setValue(testSpeedLimit_);
    testSpeedSpin_->setSuffix(QStringLiteral(" m/s"));
    testSpeedSpin_->setToolTip(QStringLiteral("Startwert intern exakt 10 kn = 463/90 m/s."));
    testDriveStatus_ = new QLabel(QStringLiteral("Nur Testfunktion, keine Widerstandsphysik."));
    testDriveStatus_->setWordWrap(true);
    testLayout->addRow(testDriveCheck_);
    cameraFrameCombo_ = new QComboBox;
    cameraFrameCombo_->setObjectName(QStringLiteral("dynamicCameraFrame"));
    cameraFrameCombo_->addItem(QStringLiteral("Körpermodell"), true);
    cameraFrameCombo_->addItem(QStringLiteral("Weltmodell"), false);
    cameraFrameCombo_->setToolTip(QStringLiteral(
        "Kamerabezug der freien Ansicht: Körpermodell folgt der Bootspose; "
        "Weltmodell folgt der Bootsposition mit weltorientierter Kamera. Keine Änderung der Antriebsphysik "
        "oder der modellfesten Tabellenkoordinaten."));
    cameraFramePanel_ = new QWidget;
    auto* frameLayout = new QFormLayout(cameraFramePanel_);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->addRow(QStringLiteral("Kamerabezug"), cameraFrameCombo_);
    connect(cameraFrameCombo_, &QComboBox::currentIndexChanged, this, [this] {
        scene_->setBodyFixedCamera(workspaceIndex_ != 2 || cameraFrameCombo_->currentData().toBool());
    });
    testLayout->addRow(QStringLiteral("Testkraft"), testForceSpin_);
    testLayout->addRow(QStringLiteral("Maximale Fahrt"), testSpeedSpin_);
    testLayout->addRow(testDriveStatus_);
    panelLayout->addWidget(testDrivePanel_);
    movementPanel_ = new QGroupBox(QStringLiteral("Ruder- und Schwertstellung (Core)"));
    movementPanel_->setObjectName(QStringLiteral("movementPanel"));
    movementLayout_ = new QFormLayout(movementPanel_);
    movementPanel_->setVisible(false);
    panelLayout->addWidget(movementPanel_);
    connect(testSpeedSpin_, &QDoubleSpinBox::valueChanged, this, [this](double value) { testSpeedLimit_ = value; });
    connect(testDriveCheck_, &QCheckBox::toggled, this, [this](bool active) {
        if (live_ && simulationThread_) {
            QString error;
            if (!live_->setTestDriveActive(active, &error)) {
                const QSignalBlocker blocker(testDriveCheck_);
                testDriveCheck_->setChecked(!active);
                showError(QStringLiteral("Testantrieb"), error);
            } else {
                testDriveStatus_->setText(QStringLiteral("Uebernahme am naechsten unberechneten Schritt angefordert."));
            }
        }
    });

    auto* statusCaption = new QLabel(QStringLiteral("LAUFSTATUS"));
    statusCaption->setObjectName(QStringLiteral("caption"));
    statusLabel_ = new QLabel(QStringLiteral("Modell laden, um zu beginnen"));
    statusLabel_->setWordWrap(true);
    statusLabel_->setObjectName(QStringLiteral("status"));
    timeLabel_ = new QLabel(QStringLiteral("Simulationszeit: –  ·  Rechenzeit: –"));
    timeLabel_->setWordWrap(true);
    timeLabel_->setObjectName(QStringLiteral("time"));
    panelLayout->addWidget(statusCaption);
    panelLayout->addWidget(statusLabel_);
    panelLayout->addWidget(timeLabel_);
    const auto stationaryInputChanged = [this] {
        if (live_ && simulationThread_ && lastRunDynamic_.value_or(false)) {
            std::array<double, 3> force{}, point{};
            for (std::size_t axis = 0; axis < force.size(); ++axis) {
                force[axis] = stationaryForce_[axis]->value();
                point[axis] = stationaryPoint_[axis]->value();
            }
            QString error;
            if (!live_->setDynamicForce(stationaryForceActive_->isChecked(), force, point, &error))
                showError(QStringLiteral("Fremdkraft"), error);
            else
                statusLabel_->setText(QStringLiteral(
                    "Fremdkraft angefordert: gilt ab dem nächsten Simulationsschritt; Historie unverändert."));
            return;
        }
        if (lastRunDynamic_ && !*lastRunDynamic_ && displayedIndex_ >= 0)
            statusLabel_->setText(QStringLiteral(
                "Eingaben geändert – angezeigtes Ergebnis gehört zum vorherigen Rechenfall."));
    };
    connect(stationaryForceActive_, &QCheckBox::toggled, this, stationaryInputChanged);
    for (auto* value : stationaryForce_)
        connect(value, &QDoubleSpinBox::valueChanged, this, stationaryInputChanged);
    for (auto* value : stationaryPoint_)
        connect(value, &QDoubleSpinBox::valueChanged, this, stationaryInputChanged);
    displayPanel_ = buildDisplayPanel();
    displayPanel_->setObjectName(QStringLiteral("displayControls"));
    panelLayout->addWidget(displayPanel_);
    panelLayout->addStretch(1);

    auto* panelScroll = new QScrollArea;
    panelScroll->setObjectName(QStringLiteral("panelScroll"));
    panelScroll->setWidget(panel);
    panelScroll->setWidgetResizable(true);
    panelScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    panelScroll->setFrameShape(QFrame::NoFrame);
    panelScroll->setMinimumWidth(320);
    panelScroll->setMaximumWidth(380);
    simulationLayout->addWidget(panelScroll);
    staticDisplaySidebar_ = new QScrollArea;
    staticDisplaySidebar_->setObjectName(QStringLiteral("staticDisplaySidebar"));
    staticDisplaySidebar_->setWidgetResizable(true);
    staticDisplaySidebar_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    staticDisplaySidebar_->setFrameShape(QFrame::NoFrame);
    staticDisplaySidebar_->setMinimumWidth(320);
    staticDisplaySidebar_->setMaximumWidth(380);
    auto* displayFrame = new QWidget;
    displayFrame->setObjectName(QStringLiteral("panel"));
    auto* displayLayout = new QVBoxLayout(displayFrame);
    displayLayout->setContentsMargins(18, 18, 18, 18);
    displayLayout->setSpacing(10);
    displayLayout->addStretch();
    staticDisplaySidebar_->setWidget(displayFrame);
    mainArea->insertWidget(0, staticDisplaySidebar_);
    staticDisplaySidebar_->hide();
    root->addLayout(mainArea, 1);

    playbackPanel_ = new QWidget;
    auto* playback = new QHBoxLayout(playbackPanel_);
    playback->setContentsMargins(0, 0, 0, 0);
    stateLabel_ = new QLabel(QStringLiteral("Zustand 0 / 0"));
    stateSlider_ = new QSlider(Qt::Horizontal);
    stateSlider_->setRange(0, 0);
    playback->addWidget(new QLabel(QStringLiteral("WIEDERGABE")));
    playback->addWidget(stateSlider_, 1);
    playback->addWidget(stateLabel_);
    root->addWidget(playbackPanel_);

    setCentralWidget(central);
    valuesDock_ = buildValuesDock();
    auto* simulationValues = valuesDock_->widget();
    valuesStack_ = new QStackedWidget;
    valuesStack_->addWidget(inspection_->valuesWidget());
    valuesStack_->addWidget(simulationValues);
    valuesDock_->setWidget(valuesStack_);
    addDockWidget(Qt::BottomDockWidgetArea, valuesDock_);

    liveTimer_ = new QTimer(this);
    liveTimer_->setTimerType(Qt::PreciseTimer);
    liveTimer_->setInterval(liveIntervalMs);

    connect(chooseFolderButton_, &QPushButton::clicked, this, [this] { chooseModelFolder(); });
    connect(refreshModelsButton_, &QPushButton::clicked, this, [this] { refreshModels(); });
    connect(modelList_, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* current, QListWidgetItem*) {
            if (current == nullptr) {
                return;
            }
            const auto id = current->data(Qt::UserRole).toString();
            if (id != modelId_) {
                openModel(id);
            }
        });
    connect(modelList_, &QListWidget::customContextMenuRequested, this,
        [this](const QPoint& position) {
            auto* item = modelList_->itemAt(position);
            if (!item) return;
            const auto id = item->data(Qt::UserRole).toString();
            const auto* entry = entryFor(id);
            if (!entry || !entry->schemaValid || entry->visualizationHashCurrent) return;
            QMenu menu(this);
            auto* update = menu.addAction(QStringLiteral("Visualisation-Hash erneuern"));
            update->setEnabled(fingerprintThread_ == nullptr && simulationThread_ == nullptr
                && openThread_ == nullptr && catalogThread_ == nullptr);
            connect(update, &QAction::triggered, this,
                [this, id] { updateVisualizationFingerprint(id); });
            menu.exec(modelList_->viewport()->mapToGlobal(position));
        });
    connect(liveTimer_, &QTimer::timeout, this, [this] { updateLiveProgress(); });
    connect(runButton_, &QPushButton::clicked, this, [this] { startSimulation(); });
    connect(cancelButton_, &QPushButton::clicked, this, [this] { cancellation_.store(true); });
    connect(modeCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
        [this](int) { updateModeControls(); });
    connect(stateSlider_, &QSlider::valueChanged, this, [this](int value) {
        if (!programmaticSlider_) {
            followLive_ = value >= stateSlider_->maximum();
        }
        displayState(value);
    });

    scene_->onPicked = [this](const QString& id) { selectContribution(id); };
    scene_->onFramePresented = [this] {
        awaitingPresentation_ = false;
        if (pendingPresentation_) QTimer::singleShot(0, this, [this] {
            auto present = std::move(pendingPresentation_);
            pendingPresentation_ = {};
            if (present) present();
        });
    };
    scene_->onReferencesChanged = [this] {
        storeReferences();
        updateReferenceLabel();
        updateValues();
    };

    loadSettings();
    simulationValuesVisible_ = !valuesDock_->isHidden();
    updateModeControls();
    setBusy(false);
    updateWorkspace(workspaceTabs->currentIndex());
}

void MainWindow::updateWorkspace(int index)
{
    if (workspaceIndex_ == 0 && index != 0 && inspection_->manualModeActive()) {
        const bool pending = inspection_->finishManualMode();
        if (pending && !sailSaveInProgress_) {
            auto* tabs = findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
            if (tabs) {
                const QSignalBlocker blocker(tabs);
                tabs->setCurrentIndex(workspaceIndex_);
            }
            return;
        }
    }
    if (workspaceIndex_ >= 0 && workspaceIndex_ != index) {
        saveDisplaySettings();
        closeGraphics();
        if (workspaceIndex_ > 0) {
            swapStaticView(scene_);
            for (std::size_t i = 0; i < staticScenes_.size(); ++i) {
                if (staticViewSlots_[i]->isAncestorOf(staticScenes_[i])) continue;
                for (auto* slot : staticViewSlots_) {
                    if (!slot->isAncestorOf(staticScenes_[i])) continue;
                    auto* occupant = qobject_cast<QVBoxLayout*>(staticViewSlots_[i]->layout())->itemAt(0)->widget();
                    slot->layout()->removeWidget(staticScenes_[i]);
                    staticViewSlots_[i]->layout()->removeWidget(occupant);
                    slot->layout()->addWidget(occupant);
                    staticViewSlots_[i]->layout()->addWidget(staticScenes_[i]);
                    occupant->show();
                    staticScenes_[i]->show();
                    break;
                }
            }
        }
        cancellation_.store(true);
        ++simulationGeneration_;
        liveTimer_->stop();
        resetResults();
        lastRunDynamic_.reset();
        statusLabel_->setText(QStringLiteral("Reiter gewechselt – Simulationszustand zurückgesetzt."));
        statusLabel_->setToolTip(QString());
        statusLabel_->setStyleSheet(QString());
        timeLabel_->setText(QStringLiteral("Simulationszeit: 0.000 s"));
    }
    workspaceIndex_ = index;
    restoreModeDisplaySettings();
    applyWorkspaceTheme(index);
    const bool inspection = index == 0;
    const bool staticModel = index == 1;
    const bool simulation = index > 0;
    scene_->dockNavigation(simulation ? staticNavigationColumn_ : nullptr);
    staticNavigationColumn_->setVisible(simulation && navigationCheck_->isChecked());
    if (simulation) scene_->onDoubleClicked = [this] { swapStaticView(scene_); };
    else scene_->onDoubleClicked = {};
    scene_->setViewName(simulation ? QStringLiteral("Frei") : QString());
    if (inspection) inspection_->setCatalogWidget(catalogPanel_);
    else mainArea_->insertWidget(0, catalogPanel_);
    catalogPanel_->setVisible(!simulation);
    if (simulation) {
        auto* displayLayout = qobject_cast<QVBoxLayout*>(staticDisplaySidebar_->widget()->layout());
        displayLayout->insertWidget(0, displayPanel_);
    } else
        simulationControlsLayout_->insertWidget(
            simulationControlsLayout_->count()-1, displayPanel_);
    displayPanel_->show();
    staticDisplaySidebar_->setVisible(simulation);
    staticViewsColumn_->setVisible(simulation && attitudeViewsCheck_->isChecked());
    attitudeViewsCheck_->setVisible(simulation);
    for (auto* control : vectorControls_) control->setVisible(!simulation);
    staticForceMenu_->setVisible(simulation);
    dynamicMotionPanel_->setVisible(index == 2);
    graphicsStatus_->setVisible(!simulation);
    pressureStatus_->setVisible(!simulation);
    legacyValues_->setVisible(!simulation);
    staticValues_->setVisible(simulation);
    scene_->setBodyFixedCamera(staticModel || cameraFrameCombo_->currentData().toBool());
    cameraCombo_->ensurePolished();
    cameraCombo_->setMinimumHeight(cameraCombo_->sizeHint().height());
    if (inspection && workspaceStack_->currentIndex() != 0)
        simulationValuesVisible_ = !valuesDock_->isHidden();
    workspaceStack_->setCurrentIndex(inspection ? 0 : 1);
    valuesStack_->setCurrentIndex(inspection ? 0 : 1);
    chooseFolderButton_->setVisible(inspection);
    playbackPanel_->setVisible(index == 2);
    valuesDock_->setVisible(inspection || simulationValuesVisible_);
    if (!inspection) {
        const QSignalBlocker blocker(modeCombo_);
        modeCombo_->setCurrentIndex(staticModel ? 0 : 1);
    }
    modeCombo_->setVisible(false);
    updateModeControls();
    graphicsButton_->setVisible(simulation);
    updateSceneActivity(false);
    applyDisplayOptions();
}

void MainWindow::swapStaticView(VtkSceneWidget* view)
{
    if (workspaceIndex_ <= 0 || graphicsPage_ || view == largeScene_) return;
    for (auto* slot : staticViewSlots_) {
        if (!slot->isAncestorOf(view)) continue;
        auto* previous = largeScene_;
        sceneStack_->removeWidget(previous);
        slot->layout()->removeWidget(view);
        sceneStack_->addWidget(view);
        slot->layout()->addWidget(previous);
        largeScene_ = view;
        sceneStack_->setCurrentWidget(view);
        previous->show();
        view->show();
        updateSceneActivity();
        return;
    }
}

void MainWindow::updateSceneActivity(bool synchronize)
{
    if (!graphicsReady_) return;
    auto update = [this](VtkSceneWidget* view) {
        const bool active = workspaceIndex_ <= 0 ? view == scene_
            : view == largeScene_ || attitudeViewsCheck_->isChecked();
        view->setUpdatesActive(active);
    };
    update(scene_);
    for (auto* view : staticScenes_) update(view);
    if (synchronize) {
        applyDisplayOptions(false);
        if (displayedIndex_ >= 0) displayState(displayedIndex_, true);
    }
}

void MainWindow::applyWorkspaceTheme(int index)
{
    const auto& theme = workspaceThemes.at(index);
    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background:%1; color:#e8eef5; "
        "font-family:'Segoe UI'; font-size:10pt; }"
        "QLabel, QCheckBox { background:transparent; }"
        "QCheckBox::indicator { width:15px; height:15px; border:1px solid "
        "%3; border-radius:4px; background:%4; }"
        "QCheckBox::indicator:checked { background:%7; border-color:%8; "
        "image:url(:/controls/checkmark.svg); }"
        "QTabBar::tab { padding:12px 20px; font-weight:600; }"
        "QLabel#title { font-size:19pt; font-weight:700; letter-spacing:2px; }"
        "QLabel#subtitle { color:%9; font-size:10pt; }"
        "QLabel#caption { color:%10; font-size:8pt; font-weight:700; "
        "letter-spacing:1.5px; }"
        "QLabel#modelName { font-size:19pt; font-weight:700; letter-spacing:2px; }"
        "QLabel#status { color:#b8e7c3; font-size:11pt; }"
        "QLabel#time, QLabel#legend { color:%9; font-size:9pt; }"
        "QLabel#balance { background:%2; border:1px solid %3; "
        "border-radius:8px; padding:8px; }"
        "QLabel#detail { color:#c9d6e2; font-size:9pt; }"
        "QWidget#panel { background:%2; border:1px solid %3; "
        "border-radius:12px; }"
        "QWidget#simulationScene { border:1px solid %3; border-radius:12px; }"
        "QScrollArea#panelScroll, QScrollArea#staticDisplaySidebar { background:transparent; }"
        "QPushButton { background:%5; border:1px solid %3; "
        "border-radius:7px; padding:9px 13px; font-weight:600; }"
        "QPushButton:hover { background:%6; }"
        "QPushButton:disabled { color:%9; background:%2; }"
        "QPushButton#primaryButton { background:%7; border-color:%8; }"
        "QPushButton#primaryButton:hover { background:%6; }"
        "QPushButton#smallButton { padding:5px 9px; }"
        "QComboBox, QDoubleSpinBox { background:%4; border:1px solid "
        "%3; border-radius:6px; padding:6px; }"
        "QListWidget#modelList { background:%4; border:1px solid "
        "%3; border-radius:12px; padding:3px; }"
        "QListWidget#modelList::item { padding:4px 6px; border-radius:4px; }"
        "QListWidget#modelList::item:selected { background:%7; "
        "color:#ffffff; }"
        "QTreeWidget { background:%4; alternate-background-color:%11; "
        "border:1px solid %3; border-radius:12px; }"
        "QTreeWidget::item:selected { background:#5c4a12; color:#ffd166; }"
        "QHeaderView::section { background:%11; color:%9; padding:5px; "
        "border:0; }"
        "QDockWidget { color:%10; font-weight:700; }"
        "QDockWidget::title { background:%2; padding:6px; }"
        "QMenu::item:selected, QMenuBar::item:selected { background:%7; }"
        "QSlider::groove:horizontal { height:5px; background:%3; "
        "border-radius:2px; }"
        "QScrollBar:vertical { background:%1; width:10px; margin:0; }"
        "QScrollBar::handle:vertical { background:%3; border-radius:5px; "
        "min-height:24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }"
        "QSlider::handle:horizontal { background:%8; width:15px; "
        "margin:-6px 0; border-radius:7px; }")
        .arg(QString::fromLatin1(theme.background))
        .arg(QString::fromLatin1(theme.panel))
        .arg(QString::fromLatin1(theme.border))
        .arg(QString::fromLatin1(theme.input))
        .arg(QString::fromLatin1(theme.button))
        .arg(QString::fromLatin1(theme.hover))
        .arg(QString::fromLatin1(theme.primary))
        .arg(QString::fromLatin1(theme.accent))
        .arg(QString::fromLatin1(theme.muted))
        .arg(QString::fromLatin1(theme.caption))
        .arg(QString::fromLatin1(theme.alternate)));
}

QWidget* MainWindow::buildDisplayPanel()
{
    auto* box = new QWidget;
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 6, 0, 0);
    layout->setSpacing(7);
    auto* caption = new QLabel(QStringLiteral("DARSTELLUNG"));
    caption->setObjectName(QStringLiteral("caption"));
    layout->addWidget(caption);
    layout->addWidget(cameraFramePanel_);

    forcesCheck_ = new QCheckBox(QStringLiteral("Vektoren anzeigen"));
    forcesCheck_->setObjectName(QStringLiteral("displayVectors"));
    forcesCheck_->setChecked(true);
    layout->addWidget(forcesCheck_);

    auto* cameraRow = new QHBoxLayout;
    cameraRow->setSpacing(6);
    cameraCombo_ = new QComboBox;
    cameraCombo_->setObjectName(QStringLiteral("cameraPreset"));
    cameraCombo_->setEnabled(false);
    cameraCombo_->addItem(QStringLiteral("Freie Ansicht (Maus)"), QString());
    for (const auto& preset : navigation::cameraPresets())
        cameraCombo_->addItem(QString::fromUtf8(preset.label), QString::fromLatin1(preset.id));
    cameraCombo_->setToolTip(QStringLiteral(
        "Ansicht relativ zur Yacht setzen; danach mit der Maus frei drehen.\n"
        "Seiten, Bug und Heck: beim Anwählen Wasserhöhe Z=0 und horizontaler Blick (0°).\n"
        "Blickrichtung, Abstand und Kamera-Oben bleiben bei Bewegung körperfest."));
    connect(cameraCombo_, &QComboBox::activated, this, [this] {
        const auto id = cameraCombo_->currentData().toString();
        if (id.isEmpty()) return;
        QString error;
        if (!scene_->setCameraPreset(id, &error))
            showError(QStringLiteral("Kameraansicht"), error);
    });
    cameraRow->addWidget(new QLabel(QStringLiteral("Kamera")));
    cameraRow->addWidget(cameraCombo_, 1);
    layout->addLayout(cameraRow);
    navigationCheck_ = new QCheckBox(QStringLiteral("Kompass und Lageanzeigen"));
    navigationCheck_->setObjectName(QStringLiteral("displayNavigation"));
    navigationCheck_->setChecked(true);
    connect(navigationCheck_, &QCheckBox::toggled, this, [this](bool visible) {
        scene_->setNavigationVisible(visible);
        staticNavigationColumn_->setVisible(workspaceIndex_ > 0 && visible);
        saveDisplaySettings();
    });
    layout->addWidget(navigationCheck_);
    attitudeViewsCheck_ = new QCheckBox(QStringLiteral("Analyseansichten"));
    attitudeViewsCheck_->setObjectName(QStringLiteral("displayAttitudeViews"));
    attitudeViewsCheck_->setChecked(true);
    attitudeViewsCheck_->setToolTip(QStringLiteral("Rechte Spalte mit Seite, Heck und Draufsicht anzeigen."));
    layout->addWidget(attitudeViewsCheck_);
    connect(attitudeViewsCheck_, &QCheckBox::toggled, this, [this](bool visible) {
        staticViewsColumn_->setVisible(workspaceIndex_ > 0 && visible);
        updateSceneActivity();
        saveDisplaySettings();
    });
    auto scaleCombo = [] {
        auto* combo = new QComboBox;
        combo->addItem(QStringLiteral("logarithmisch"),
            static_cast<int>(forcedisplay::ScaleMode::Logarithmic));
        combo->addItem(QStringLiteral("linear"), static_cast<int>(forcedisplay::ScaleMode::Linear));
        return combo;
    };
    forceScaleCombo_ = scaleCombo();
    momentScaleCombo_ = scaleCombo();
    graphicsButton_ = new QPushButton(QStringLiteral("Darstellung"));
    graphicsButton_->setObjectName(QStringLiteral("configureGraphics"));
    connect(graphicsButton_, &QPushButton::clicked, this, &MainWindow::setupGraphics);
    layout->addWidget(graphicsButton_);
    dynamicMotionPanel_ = new QGroupBox(QStringLiteral("Fahrdynamikdarstellung"));
    dynamicMotionPanel_->setObjectName(QStringLiteral("dynamicMotionPanel"));
    auto* motionLayout = new QVBoxLayout(dynamicMotionPanel_);
    const std::array<graphics::Feature, 4> motionFeatures{graphics::Feature::Wake,
        graphics::Feature::WaterTrack, graphics::Feature::DistanceMarks, graphics::Feature::Trail};
    for (std::size_t i = 0; i < motionFeatures.size(); ++i) {
        const auto feature = motionFeatures[i];
        const auto& info = graphics::features()[static_cast<std::size_t>(feature)];
        auto* check = new QCheckBox(feature == graphics::Feature::Wake
            ? QStringLiteral("Bug-/Heckwelle") : QString::fromUtf8(info.label));
        check->setObjectName(QStringLiteral("dynamicMotion_%1").arg(QString::fromLatin1(info.key)));
        check->setToolTip(QString::fromUtf8(info.explanation));
        dynamicMotionChecks_[i] = check;
        motionLayout->addWidget(check);
        connect(check, &QCheckBox::toggled, this, [this, feature](bool enabled) {
            auto next = graphics_;
            next.profiles[0].enabled[static_cast<std::size_t>(feature)] = enabled;
            QSettings storage;
            QString error;
            if (!graphics::save(storage, next, &error)) {
                showError(QStringLiteral("Fahrdynamikdarstellung"), error);
                applyGraphicsProfile(false);
                return;
            }
            graphics_ = next;
            applyGraphicsProfile(false);
        });
    }
    auto* motionParameters = new QFormLayout;
    const auto motionNumber = [this, motionParameters](const QString& name, const QString& label,
        double graphics::Profile::* member, double minimum, double maximum) {
        auto* field = new QDoubleSpinBox;
        field->setObjectName(name);
        field->setDecimals(5);
        field->setRange(minimum, maximum);
        field->setKeyboardTracking(false);
        motionParameters->addRow(label, field);
        connect(field, &QDoubleSpinBox::valueChanged, this, [this, field, member](double value) {
            auto next = graphics_;
            next.profiles[0].*member = value;
            QSettings storage;
            QString error;
            if (!graphics::save(storage, next, &error)) {
                showError(QStringLiteral("Fahrdynamikdarstellung"), error);
                const QSignalBlocker blocker(field);
                field->setValue(graphics_.profiles[0].*member);
                return;
            }
            graphics_ = next;
            applyGraphicsProfile(false);
        });
    };
    motionNumber(QStringLiteral("dynamicTrackAge"), QStringLiteral("Spuralter [s]"),
        &graphics::Profile::trackAge, .001, 86400);
    motionNumber(QStringLiteral("dynamicTrackSpacing"), QStringLiteral("Spurabtastung [m]"),
        &graphics::Profile::trackSpacing, .00001, 10000);
    motionNumber(QStringLiteral("dynamicMarkSpacing"), QStringLiteral("Distanzmarken [m]"),
        &graphics::Profile::markSpacing, .001, 10000);
    auto* motionBudget = new QSpinBox;
    motionBudget->setObjectName(QStringLiteral("dynamicTrackBudget"));
    motionBudget->setRange(2, 65536);
    motionBudget->setKeyboardTracking(false);
    motionParameters->addRow(QStringLiteral("Max. Spurpunkte"), motionBudget);
    connect(motionBudget, &QSpinBox::valueChanged, this, [this, motionBudget](int value) {
        auto next = graphics_;
        next.profiles[0].trackBudget = value;
        QSettings storage;
        QString error;
        if (!graphics::save(storage, next, &error)) {
            showError(QStringLiteral("Fahrdynamikdarstellung"), error);
            const QSignalBlocker blocker(motionBudget);
            motionBudget->setValue(graphics_.profiles[0].trackBudget);
            return;
        }
        graphics_ = next;
        applyGraphicsProfile(false);
    });
    motionLayout->addLayout(motionParameters);
    layout->addWidget(dynamicMotionPanel_);
    auto* vectorScales = new QWidget;
    auto* scaleForm = new QFormLayout(vectorScales);
    scaleForm->setContentsMargins(0, 0, 0, 0);
    scaleForm->setSpacing(6);
    scaleForm->addRow(QStringLiteral("Skala Kraft"), forceScaleCombo_);
    scaleForm->addRow(QStringLiteral("Skala Moment"), momentScaleCombo_);
    layout->addWidget(vectorScales);
    graphicsStatus_ = new QLabel;
    graphicsStatus_->setWordWrap(true);
    graphicsStatus_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(graphicsStatus_);

    staticForceMenu_ = new QWidget;
    staticForceMenu_->setObjectName(QStringLiteral("staticForceMenu"));
    auto* staticMenuLayout = new QVBoxLayout(staticForceMenu_);
    staticMenuLayout->setContentsMargins(0, 0, 0, 0);
    for (std::size_t rowIndex = 0; rowIndex < staticForceStyles_.size(); ++rowIndex) {
        const auto i = rowIndex < 2 ? 1 - rowIndex : rowIndex;
        if (rowIndex == 5)
            staticMenuLayout->addSpacing(fontMetrics().lineSpacing() / 2);
        if (rowIndex == 0 || rowIndex == 5)
            staticMenuLayout->addWidget(new QLabel(rowIndex == 0
                ? QStringLiteral("<b>Kraftpfeile (einfach)</b>")
                : QStringLiteral("<b>Kraftpfeile (vollständig)</b>")));
        const auto& style = staticForceStyles_[i];
        auto* row = new QHBoxLayout;
        auto* check = new QCheckBox(style.name);
        check->setObjectName(QStringLiteral("staticForce_%1").arg(style.key));
        check->setChecked(style.enabled);
        check->setToolTip(style.explanation);
        auto* color = new QPushButton;
        color->setObjectName(QStringLiteral("staticColor_%1").arg(style.key));
        color->setFixedSize(26, 22);
        color->setAccessibleName(QStringLiteral("Farbe %1").arg(style.name));
        staticForceChecks_[i] = check;
        staticColorButtons_[i] = color;
        row->addWidget(check, 1);
        row->addWidget(color, 0, Qt::AlignRight);
        staticMenuLayout->addLayout(row);
        connect(check, &QCheckBox::toggled, this, [this, i](bool on) {
            staticForceStyles_[i].enabled = on;
            applyDisplayOptions();
        });
        connect(color, &QPushButton::clicked, this, [this, i] {
            const auto chosen = QColorDialog::getColor(QColor(staticForceStyles_[i].color),
                this, QStringLiteral("Farbe: %1").arg(staticForceStyles_[i].name));
            if (!chosen.isValid()) return;
            staticForceStyles_[i].color = chosen.name(QColor::HexRgb);
            applyDisplayOptions();
        });
    }
    auto* defaultColors = new QPushButton(QStringLiteral("Default Farben"));
    defaultColors->setObjectName(QStringLiteral("staticDefaultColors"));
    staticMenuLayout->addWidget(defaultColors);
    connect(defaultColors, &QPushButton::clicked, this, [this] {
        const auto defaults = forcedisplay::staticForceDefaults();
        for (std::size_t i = 0; i < defaults.size(); ++i)
            staticForceStyles_[i].color = defaults[i].color;
        applyDisplayOptions();
    });
    layout->addWidget(staticForceMenu_);
    staticForceMenu_->hide();

    auto* referenceRow = new QHBoxLayout;
    referenceLabel_ = new QLabel(QStringLiteral("R_F: –  ·  R_M: –"));
    referenceLabel_->setObjectName(QStringLiteral("time"));
    referenceLabel_->setWordWrap(true);
    referenceLabel_->setToolTip(QStringLiteral(
        "Skalenreferenzen: ein Beitrag mit diesem Betrag erhält die volle Pfeillänge.\n"
        "Während eines Laufs fest; „Neu einpassen“ setzt sie auf das 1,1-Fache des\n"
        "aktuellen Maximums."));
    refitButton_ = new QPushButton(QStringLiteral("Neu einpassen"));
    refitButton_->setObjectName(QStringLiteral("smallButton"));
    referenceRow->addWidget(referenceLabel_, 1);
    referenceRow->addWidget(refitButton_);
    layout->addLayout(referenceRow);

    leverCheck_ = new QCheckBox(QStringLiteral("Hebelarm der Auswahl"));
    leverCheck_->setChecked(true);
    lateralCheck_ = new QCheckBox(QStringLiteral("Lateraldruckpunkt (LDP)"));
    lateralCheck_->setObjectName(QStringLiteral("displayLateralCenter"));
    lateralCheck_->setChecked(true);
    sailCenterCheck_ = new QCheckBox(QStringLiteral("SDP (Ersatz)"));
    sailCenterCheck_->setObjectName(QStringLiteral("displaySailCenter"));
    sailCenterCheck_->setChecked(true);
    // Zwei Rubriken in einem Raster: Zeilen Gewicht/Auftrieb/Summe, Spalten gesamt/Teile.
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(3);
    auto addSelection = [grid](int row, const QString& title, const QString& sumText,
                            std::array<QCheckBox*, 5>& checks,
                            const std::array<bool, 5>& defaults, const QStringList& tips) {
        grid->addWidget(new QLabel(QStringLiteral("<b>%1</b>").arg(title)), row, 0);
        grid->addWidget(new QLabel(QStringLiteral("gesamt")), row, 1, Qt::AlignHCenter);
        grid->addWidget(new QLabel(QStringLiteral("Teile")), row, 2, Qt::AlignHCenter);
        for (std::size_t i = 0; i < checks.size(); ++i) {
            checks[i] = new QCheckBox;
            checks[i]->setChecked(defaults[i]);
            checks[i]->setToolTip(tips.at(static_cast<int>(i)));
        }
        grid->addWidget(new QLabel(QStringLiteral("Gewicht")), row + 1, 0);
        grid->addWidget(checks[0], row + 1, 1, Qt::AlignHCenter);
        grid->addWidget(checks[1], row + 1, 2, Qt::AlignHCenter);
        grid->addWidget(new QLabel(QStringLiteral("Auftrieb")), row + 2, 0);
        grid->addWidget(checks[2], row + 2, 1, Qt::AlignHCenter);
        grid->addWidget(checks[3], row + 2, 2, Qt::AlignHCenter);
        grid->addWidget(new QLabel(QStringLiteral("Summe %1").arg(sumText)), row + 3, 0);
        grid->addWidget(checks[4], row + 3, 1, Qt::AlignHCenter);
    };
    addSelection(0, QStringLiteral("Kräfte"), QStringLiteral("ΣF"), forceChecks_,
        {false, true, false, true, true},
        {QStringLiteral("Gesamtgewicht im Schwerpunkt"), QStringLiteral("Gewicht je Element"),
            QStringLiteral("Gesamtauftrieb im Auftriebspunkt"), QStringLiteral("Auftrieb je Element"),
            QStringLiteral("Resultierende Kraft")});
    grid->setRowMinimumHeight(4, 6);
    addSelection(5, QStringLiteral("Momente"), QStringLiteral("ΣM"), momentChecks_,
        {false, false, false, false, true},
        {QStringLiteral("Moment des Gesamtgewichts um S"),
            QStringLiteral("Moment des Gewichts je Element um S"),
            QStringLiteral("Moment des Gesamtauftriebs um S"),
            QStringLiteral("Moment des Auftriebs je Element um S"),
            QStringLiteral("Resultierendes Moment um S")});
    grid->setColumnStretch(3, 1);
    layout->addLayout(grid);
    layout->addWidget(leverCheck_);
    layout->addWidget(new QLabel(QStringLiteral("Druckpunkte")));
    layout->addWidget(lateralCheck_);
    layout->addWidget(sailCenterCheck_);
    aeroCenterCheck_ = new QCheckBox(QStringLiteral("SDP Segel gesamt"));
    aeroCenterCheck_->setObjectName(QStringLiteral("displayAeroCenter"));
    aeroCenterCheck_->setChecked(true);
    aeroIndividualPanel_ = new QWidget;
    aeroIndividualLayout_ = new QVBoxLayout(aeroIndividualPanel_);
    aeroIndividualLayout_->setContentsMargins(0, 0, 0, 0);
    aeroBoundariesCheck_ = new QCheckBox(QStringLiteral("Streifengrenzen"));
    aeroBoundariesCheck_->setObjectName(QStringLiteral("displayAeroBoundaries"));
    layout->addWidget(aeroCenterCheck_);
    layout->addWidget(aeroIndividualPanel_);
    layout->addWidget(aeroBoundariesCheck_);
    auto* aeroGrid = new QGridLayout;
    aeroGrid->addWidget(new QLabel(QStringLiteral("Segellasten")), 0, 0);
    const std::array<QString, 3> aeroNames{QStringLiteral("Streifen"), QStringLiteral("je Segel"), QStringLiteral("gesamt")};
    for (int i = 0; i < 3; ++i) {
        aeroGrid->addWidget(new QLabel(aeroNames[static_cast<std::size_t>(i)]), 0, i + 1);
        sailForceChecks_[static_cast<std::size_t>(i)] = new QCheckBox;
        sailMomentChecks_[static_cast<std::size_t>(i)] = new QCheckBox;
        sailForceChecks_[static_cast<std::size_t>(i)]->setChecked(i != 1);
        sailMomentChecks_[static_cast<std::size_t>(i)]->setChecked(i == 2);
        aeroGrid->addWidget(sailForceChecks_[static_cast<std::size_t>(i)], 1, i + 1);
        aeroGrid->addWidget(sailMomentChecks_[static_cast<std::size_t>(i)], 2, i + 1);
    }
    aeroGrid->addWidget(new QLabel(QStringLiteral("Kraft")), 1, 0);
    aeroGrid->addWidget(new QLabel(QStringLiteral("Moment um S")), 2, 0);
    layout->addLayout(aeroGrid);
    windArrowsCheck_ = new QCheckBox(QStringLiteral("Windpfeile anzeigen"));
    windArrowsCheck_->setObjectName(QStringLiteral("displayWindArrows"));
    windArrowsCheck_->setChecked(true);
    layout->addWidget(windArrowsCheck_);
    auto* windLegend = new QLabel(QStringLiteral(
        "<span style='color:#57b6ff'>━ Wahrer Wind</span>  "
        "<span style='color:#ff9a32'>━ Scheinbarer Wind</span>"));
    windLegend->setWordWrap(true);
    layout->addWidget(windLegend);
    pressureStatus_ = new QLabel;
    pressureStatus_->setObjectName(QStringLiteral("pressureStatus"));
    pressureStatus_->setWordWrap(true);
    layout->addWidget(pressureStatus_);

    auto* legend = new QLabel(QStringLiteral(
        "<span style='color:%1'>━ Gewicht</span>  "
        "<span style='color:%2'>╍ Auftrieb</span>  "
        "<span style='color:%5'>━ gesamt</span>  "
        "<span style='color:%3'>━ ΣF</span>  "
        "<span style='color:%4'>↻ ΣM</span><br>"
        "Momente um S. Länge relativ zur Referenz. Klick wählt aus.")
            .arg(forcedisplay::colorFor(forcedisplay::Category::Weight),
                forcedisplay::colorFor(forcedisplay::Category::Buoyancy),
                forcedisplay::colorFor(forcedisplay::Category::ForceSum),
                forcedisplay::colorFor(forcedisplay::Category::MomentSum),
                QStringLiteral("#67a9cf")));
    legend->setObjectName(QStringLiteral("legend"));
    legend->setWordWrap(true);
    layout->addWidget(legend);
    vectorControls_ = {forcesCheck_, referenceLabel_, refitButton_, leverCheck_,
        lateralCheck_, sailCenterCheck_, legend, vectorScales};
    for (int i = 0; i < grid->count(); ++i)
        if (auto* widget = grid->itemAt(i)->widget()) vectorControls_ << widget;

    valuesCheck_ = new QCheckBox(QStringLiteral("Wertefenster anzeigen"));
    valuesCheck_->setChecked(true);
    layout->addWidget(valuesCheck_);

    QList<QCheckBox*> displayChecks{forcesCheck_, leverCheck_, lateralCheck_, sailCenterCheck_, windArrowsCheck_,
        aeroCenterCheck_, aeroBoundariesCheck_};
    for (auto* check : sailForceChecks_) displayChecks << check;
    for (auto* check : sailMomentChecks_) displayChecks << check;
    for (auto* check : forceChecks_) {
        displayChecks << check;
    }
    for (auto* check : momentChecks_) {
        displayChecks << check;
    }
    for (auto* check : displayChecks) {
        connect(check, &QCheckBox::toggled, this, [this](bool) { applyDisplayOptions(); });
    }
    for (auto* combo : {forceScaleCombo_, momentScaleCombo_}) {
        connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { applyDisplayOptions(); });
    }
    connect(refitButton_, &QPushButton::clicked, this, [this] { scene_->refitReferences(); });
    connect(valuesCheck_, &QCheckBox::toggled, this, [this](bool on) {
        if (valuesDock_ != nullptr) {
            valuesDock_->setVisible(on);
        }
    });
    return box;
}

QDockWidget* MainWindow::buildValuesDock()
{
    auto* dock = new QDockWidget(QStringLiteral("WERTE"), this);
    dock->setObjectName(QStringLiteral("werteDock"));
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
        | QDockWidget::DockWidgetClosable);
    auto* content = new QWidget;
    auto* layout = new QHBoxLayout(content);
    layout->setContentsMargins(10, 6, 10, 8);
    layout->setSpacing(12);

    auto* left = new QVBoxLayout;
    balanceLabel_ = new QLabel(QStringLiteral("Keine Daten"));
    balanceLabel_->setObjectName(QStringLiteral("balance"));
    balanceLabel_->setWordWrap(true);
    balanceLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    balanceLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    auto* balanceScroll = new QScrollArea;
    balanceScroll->setObjectName(QStringLiteral("balanceScroll"));
    balanceScroll->setWidgetResizable(true);
    balanceScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    balanceScroll->setMinimumWidth(260);
    balanceScroll->setMaximumWidth(360);
    balanceScroll->setWidget(balanceLabel_);
    stepInfoLabel_ = new QLabel(QStringLiteral("Zeit: –  ·  Schritt: –"));
    stepInfoLabel_->setObjectName(QStringLiteral("time"));
    auto* copyButton = new QPushButton(QStringLiteral("Werte kopieren"));
    copyButton->setObjectName(QStringLiteral("smallButton"));
    copyButton->setToolTip(QStringLiteral("Bilanz und Tabelle als Text (Strg+C)"));
    left->addWidget(balanceScroll, 1);
    left->addWidget(stepInfoLabel_);
    left->addWidget(copyButton);

    auto* right = new QVBoxLayout;
    valuesTree_ = new QTreeWidget;
    valuesTree_->setObjectName(QStringLiteral("simulationValues"));
    valuesTree_->setColumnCount(ColumnCount);
    valuesTree_->setHeaderLabels({QStringLiteral("Beitrag"), QStringLiteral("Betrag"),
        QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z"), QStringLiteral("Zustand"),
        QStringLiteral("Skala")});
    valuesTree_->setAlternatingRowColors(true);
    valuesTree_->setUniformRowHeights(true);
    valuesTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    valuesTree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    valuesTree_->header()->setSectionResizeMode(ColName, QHeaderView::Stretch);
    for (int column = ColMagnitude; column < ColumnCount; ++column) {
        valuesTree_->header()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }
    valuesTree_->header()->setStretchLastSection(false);
    detailLabel_ = new QLabel(QStringLiteral("Beitrag in Tabelle oder 3D wählen."));
    detailLabel_->setObjectName(QStringLiteral("detail"));
    detailLabel_->setWordWrap(true);
    detailLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    right->addWidget(valuesTree_, 1);
    right->addWidget(detailLabel_);

    legacyValues_ = new QWidget;
    auto* legacyLayout = new QHBoxLayout(legacyValues_);
    legacyLayout->setContentsMargins(0, 0, 0, 0);
    legacyLayout->addLayout(left);
    legacyLayout->addLayout(right, 1);
    layout->addWidget(legacyValues_, 1);
    staticValues_ = new QTreeWidget;
    staticValues_->setObjectName(QStringLiteral("staticForceValues"));
    auto* header = new FilterHeader(staticValues_);
    staticValues_->setHeader(header);
    staticValues_->setHeaderLabels({QStringLiteral("Objekt/Pfad"), QStringLiteral("Art"),
        QStringLiteral("Bezeichnung"), QStringLiteral("Betrag"), QStringLiteral("X"),
        QStringLiteral("Y"), QStringLiteral("Z"), QStringLiteral("Einheit"),
        QStringLiteral("Bezugspunkt"), QStringLiteral("Status")});
    staticValues_->setToolTip(QStringLiteral(
        "Kräfte und Momente im Modellkoordinatensystem; Momente um S. "
        "Spaltenfilter kombinieren sich. Die Pfeilauswahl beeinflusst diese Tabelle nicht."));
    staticValues_->setRootIsDecorated(false);
    staticValues_->setAlternatingRowColors(true);
    staticValues_->setUniformRowHeights(true);
    staticValues_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    staticValues_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(false);
    header->setMinimumSectionSize(45);
    const std::array<int, 10> widths{180, 85, 230, 110, 100, 100, 100, 75, 220, 160};
    for (int i = 0; i < 10; ++i) {
        staticValues_->setColumnWidth(i, widths[i]);
        auto* filter = new QLineEdit(header->viewport());
        staticValueFilters_[i] = filter;
        filter->setObjectName(QStringLiteral("staticValueFilter%1").arg(i));
        filter->setPlaceholderText(QStringLiteral("*"));
        filter->setClearButtonEnabled(true);
        filter->setToolTip(staticValues_->headerItem()->text(i) + QStringLiteral(" filtern"));
        connect(filter, &QLineEdit::textChanged, this, [this] { filterStaticValues(); });
    }
    header->setFilters(staticValueFilters_);
    staticValues_->setSortingEnabled(true);
    staticValues_->sortItems(0, Qt::AscendingOrder);
    layout->addWidget(staticValues_, 1);
    staticValues_->hide();
    dock->setWidget(content);

    connect(copyButton, &QPushButton::clicked, this, [this] { copyValues(); });
    auto* shortcut = new QShortcut(QKeySequence::Copy, content);
    shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut, &QShortcut::activated, this, [this] { copyValues(); });
    connect(staticValues_, &QTreeWidget::itemSelectionChanged, this, [this] {
        const auto selected = staticValues_->selectedItems();
        const auto id = selected.isEmpty() ? QString()
            : selected.front()->data(0, Qt::UserRole).toString();
        scene_->setSelected(id);
    });
    connect(valuesTree_, &QTreeWidget::itemSelectionChanged, this, [this] {
        const auto items = valuesTree_->selectedItems();
        const QString id =
            items.isEmpty() ? QString() : items.front()->data(ColName, Qt::UserRole).toString();
        scene_->setSelected(id);
        updateDetail();
    });
    connect(dock, &QDockWidget::visibilityChanged, this, [this](bool) {
        if (valuesCheck_ != nullptr && valuesDock_ != nullptr) {
            const QSignalBlocker blocker(valuesCheck_);
            valuesCheck_->setChecked(!valuesDock_->isHidden());
        }
    });
    return dock;
}

MainWindow::~MainWindow()
{
    scene_->dockNavigation(nullptr);
    shuttingDown_.store(true);
    cancellation_.store(true);
    if (stateFetchThread_) stateFetchThread_->quit();
    for (auto* thread : openThreads_) thread->wait();
    for (const auto& thread : {openThread_, simulationThread_, stateFetchThread_, catalogThread_,
             fingerprintThread_}) {
        if (thread != nullptr) {
            thread->wait();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (inspection_->sailSaveInProgress() || inspection_->finishManualMode()) {
        closeAfterSailSave_ = sailSaveInProgress_;
        event->ignore();
        return;
    }
    QSettings settings;
    settings.setValue(QStringLiteral("fenster/geometrie"), saveGeometry());
    settings.setValue(QStringLiteral("fenster/zustand"), saveState());
    saveDisplaySettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    const double density = settings.value(QStringLiteral("luft/dichte_kg_m3"), 1.225).toDouble();
    const double viscosity = settings.value(QStringLiteral("luft/kinematische_viskositaet_m2_s"), 1.5e-5).toDouble();
    if (std::isfinite(density) && density > 0.) airDensity_ = density;
    if (std::isfinite(viscosity) && viscosity > 0.) airViscosity_ = viscosity;
    const auto geometry = settings.value(QStringLiteral("fenster/geometrie")).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    const auto state = settings.value(QStringLiteral("fenster/zustand")).toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }
    {
        const QSignalBlocker b(valuesCheck_);
        valuesCheck_->setChecked(!valuesDock_->isHidden());
    }
    auto restoreCheck = [&settings](QCheckBox* check, const QString& key) {
        const QSignalBlocker blocker(check);
        check->setChecked(settings.value(key, check->isChecked()).toBool());
    };
    restoreCheck(forcesCheck_, QStringLiteral("darstellung/pfeile"));
    restoreCheck(navigationCheck_, QStringLiteral("darstellung/navigation"));
    restoreCheck(attitudeViewsCheck_, QStringLiteral("statisch/lageansichten"));
    scene_->setNavigationVisible(navigationCheck_->isChecked());
    for (std::size_t i = 0; i < forceChecks_.size(); ++i) {
        restoreCheck(forceChecks_[i], QStringLiteral("darstellung/kraft_%1").arg(i));
        restoreCheck(momentChecks_[i], QStringLiteral("darstellung/moment_%1").arg(i));
    }
    restoreCheck(leverCheck_, QStringLiteral("darstellung/hebelarm"));
    restoreCheck(lateralCheck_, QStringLiteral("darstellung/lateraldruckpunkt"));
    restoreCheck(sailCenterCheck_, QStringLiteral("darstellung/ersatz_sdp"));
    restoreCheck(aeroCenterCheck_, QStringLiteral("darstellung/sailforce_sdp_gesamt"));
    restoreCheck(aeroBoundariesCheck_, QStringLiteral("darstellung/sailforce_grenzen"));
    for (std::size_t i = 0; i < sailForceChecks_.size(); ++i) {
        restoreCheck(sailForceChecks_[i], QStringLiteral("darstellung/sailforce_kraft_%1").arg(i));
        restoreCheck(sailMomentChecks_[i], QStringLiteral("darstellung/sailforce_moment_%1").arg(i));
    }
    for (std::size_t i = 0; i < staticForceStyles_.size(); ++i) {
        auto& style = staticForceStyles_[i];
        const QString key = QStringLiteral("statisch/kraftmenue/%1/").arg(style.key);
        const auto stored = settings.value(key + QStringLiteral("farbe"), style.color).toString();
        if (QColor(stored).isValid()) {
            style.color = stored;
        } else {
            QTimer::singleShot(0, this, [this, stored] {
                showError(QStringLiteral("Statische Kraftfarben"),
                    QStringLiteral("Ungültige gespeicherte Farbe: %1. Bitte neu auswählen.").arg(stored));
            });
        }
        style.enabled = settings.value(key + QStringLiteral("ein"), style.enabled).toBool();
        const QSignalBlocker blocker(staticForceChecks_[i]);
        staticForceChecks_[i]->setChecked(style.enabled);
    }
    auto restoreCombo = [&settings](QComboBox* combo, const QString& key) {
        const QSignalBlocker blocker(combo);
        const int index = combo->findData(settings.value(key, combo->currentData()).toInt());
        if (index >= 0) {
            combo->setCurrentIndex(index);
        }
    };
    restoreCombo(forceScaleCombo_, QStringLiteral("darstellung/skala_kraft"));
    restoreCombo(momentScaleCombo_, QStringLiteral("darstellung/skala_moment"));
    QString graphicsError;
    if (!settings.contains(QStringLiteral("grafik/profile_v1"))
        && !graphics::save(settings, graphics::dynamicDefaults(), &graphicsError)) {
        QTimer::singleShot(0, this, [this, graphicsError] {
            showError(QStringLiteral("Grafikeinstellungen"), graphicsError);
        });
    }
    if (!graphics::load(settings, &graphics_, &graphicsError)) {
        QTimer::singleShot(0, this, [this, graphicsError] {
            showError(QStringLiteral("Grafikeinstellungen"), graphicsError);
        });
    }
    settings.beginGroup(QStringLiteral("statisch"));
    if (!settings.contains(QStringLiteral("grafik/profile_v1"))) {
        staticGraphics_ = graphics::staticDefaults();
        if (!graphics::save(settings, staticGraphics_, &graphicsError)) {
            QTimer::singleShot(0, this, [this, graphicsError] {
                showError(QStringLiteral("Statische Grafikeinstellungen"), graphicsError);
            });
        }
    } else if (!graphics::load(settings, &staticGraphics_, &graphicsError)) {
        QTimer::singleShot(0, this, [this, graphicsError] {
            showError(QStringLiteral("Statische Grafikeinstellungen"), graphicsError);
        });
    }
    settings.endGroup();
    graphicsReady_ = true;
    applyDisplayOptions();
}

void MainWindow::setupAirProperties()
{
    if (simulationThread_ != nullptr) return;
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Lufteigenschaften"));
    auto* form = new QFormLayout(&dialog);
    auto* density = new QLineEdit(QString::number(airDensity_, 'g', 17), &dialog);
    auto* viscosity = new QLineEdit(QString::number(airViscosity_, 'g', 17), &dialog);
    density->setObjectName(QStringLiteral("airDensity"));
    viscosity->setObjectName(QStringLiteral("airViscosity"));
    form->addRow(QStringLiteral("Dichte (kg/m³)"), density);
    form->addRow(QStringLiteral("Kinematische Viskosität (m²/s)"), viscosity);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        auto parse = [](QString text, bool* ok) {
            text.replace(QLatin1Char(','), QLatin1Char('.'));
            return text.toDouble(ok);
        };
        bool densityOk = false, viscosityOk = false;
        const double rho = parse(density->text(), &densityOk);
        const double nu = parse(viscosity->text(), &viscosityOk);
        if (!densityOk || !viscosityOk || !std::isfinite(rho) || !std::isfinite(nu)
            || rho <= 0. || nu <= 0.) {
            QMessageBox::warning(&dialog, QStringLiteral("Lufteigenschaften"),
                QStringLiteral("Beide Werte müssen endliche positive Zahlen sein."));
            return;
        }
        if (validation_.valid && !modelPath_.isEmpty()) {
            QJsonArray choices;
            for (auto it = sailActive_.cbegin(); it != sailActive_.cend(); ++it)
                choices.append(QJsonObject{{QStringLiteral("element_id"), it.key()},
                    {QStringLiteral("aktiv"), it.value()->isChecked()},
                    {QStringLiteral("stellwinkel_grad"), sailAngles_.value(it.key())->value()}});
            const QJsonObject input{{QStringLiteral("sailforce_eingabe"), QJsonObject{
                {QStringLiteral("luft"), QJsonObject{{QStringLiteral("dichte_kg_m3"), rho},
                    {QStringLiteral("kinematische_viskositaet_m2_s"), nu}}},
                {QStringLiteral("segel"), choices}}}};
            const auto opened = modelCache_.value(cacheKey(modelId_, modelVersion_));
            const auto error = validateSailforceInput(modelPath_, input,
                opened ? opened->data.inspection : nullptr);
            if (!error.isEmpty()) {
                QMessageBox::warning(&dialog, QStringLiteral("Lufteigenschaften"), error);
                return;
            }
        }
        airDensity_ = rho;
        airViscosity_ = nu;
        QSettings storage;
        storage.setValue(QStringLiteral("luft/dichte_kg_m3"), rho);
        storage.setValue(QStringLiteral("luft/kinematische_viskositaet_m2_s"), nu);
        dialog.accept();
    });
    dialog.exec();
}

void MainWindow::saveDisplaySettings()
{
    if (!graphicsReady_ || workspaceIndex_ <= 0) return;
    QSettings settings;
    const QString mode = workspaceIndex_ == 2 ? QStringLiteral("dynamisch/") : QStringLiteral("statisch/");
    settings.setValue(mode + QStringLiteral("navigation"), navigationCheck_->isChecked());
    settings.setValue(mode + QStringLiteral("lageansichten"), attitudeViewsCheck_->isChecked());
    settings.setValue(QStringLiteral("darstellung/pfeile"), forcesCheck_->isChecked());
    for (std::size_t i = 0; i < forceChecks_.size(); ++i) {
        settings.setValue(QStringLiteral("darstellung/kraft_%1").arg(i), forceChecks_[i]->isChecked());
        settings.setValue(QStringLiteral("darstellung/moment_%1").arg(i), momentChecks_[i]->isChecked());
    }
    settings.setValue(QStringLiteral("darstellung/hebelarm"), leverCheck_->isChecked());
    settings.setValue(QStringLiteral("darstellung/lateraldruckpunkt"), lateralCheck_->isChecked());
    settings.setValue(QStringLiteral("darstellung/ersatz_sdp"), sailCenterCheck_->isChecked());
    settings.setValue(QStringLiteral("darstellung/sailforce_sdp_gesamt"), aeroCenterCheck_->isChecked());
    settings.setValue(QStringLiteral("darstellung/sailforce_grenzen"), aeroBoundariesCheck_->isChecked());
    for (std::size_t i = 0; i < sailForceChecks_.size(); ++i) {
        settings.setValue(QStringLiteral("darstellung/sailforce_kraft_%1").arg(i), sailForceChecks_[i]->isChecked());
        settings.setValue(QStringLiteral("darstellung/sailforce_moment_%1").arg(i), sailMomentChecks_[i]->isChecked());
    }
    settings.setValue(QStringLiteral("darstellung/skala_kraft"), forceScaleCombo_->currentData());
    settings.setValue(QStringLiteral("darstellung/skala_moment"), momentScaleCombo_->currentData());
    for (const auto& style : staticForceStyles_) {
        const QString key = mode + QStringLiteral("kraftmenue/%1/").arg(style.key);
        settings.setValue(key + QStringLiteral("farbe"), style.color);
        settings.setValue(key + QStringLiteral("ein"), style.enabled);
    }
    settings.sync();
    if (settings.status() != QSettings::NoError)
        showError(QStringLiteral("Darstellung speichern"),
            QStringLiteral("Darstellungseinstellungen konnten nicht gespeichert werden."));
}

void MainWindow::restoreModeDisplaySettings()
{
    if (!graphicsReady_ || workspaceIndex_ <= 0) return;
    QSettings settings;
    const QString mode = workspaceIndex_ == 2 ? QStringLiteral("dynamisch/") : QStringLiteral("statisch/");
    {
        const QSignalBlocker navigation(navigationCheck_), attitude(attitudeViewsCheck_);
        navigationCheck_->setChecked(settings.value(mode + QStringLiteral("navigation"),
            workspaceIndex_ == 2).toBool());
        attitudeViewsCheck_->setChecked(settings.value(mode + QStringLiteral("lageansichten"),
            workspaceIndex_ == 1).toBool());
    }
    staticForceStyles_ = forcedisplay::staticForceDefaults();
    for (std::size_t i = 0; i < staticForceStyles_.size(); ++i) {
        auto& style = staticForceStyles_[i];
        const QString key = mode + QStringLiteral("kraftmenue/%1/").arg(style.key);
        const QString color = settings.value(key + QStringLiteral("farbe"), style.color).toString();
        if (QColor(color).isValid()) style.color = color;
        else showError(QStringLiteral("Darstellungsfarbe"),
            QStringLiteral("Ungültige gespeicherte Farbe für %1: %2").arg(style.name, color));
        style.enabled = settings.value(key + QStringLiteral("ein"), style.enabled).toBool();
        const QSignalBlocker blocker(staticForceChecks_[i]);
        staticForceChecks_[i]->setChecked(style.enabled);
    }
    scene_->setNavigationVisible(navigationCheck_->isChecked());
}

void MainWindow::setupGraphics()
{
    if (workspaceIndex_ <= 0) return;
    if (graphicsPage_) {
        sceneStack_->setCurrentWidget(graphicsPage_);
        return;
    }
    const bool dynamic = workspaceIndex_ == 2;
    graphicsPage_ = createGraphicsSettingsPage(sceneStack_, dynamic ? graphics_ : staticGraphics_, scene_->graphicsStatus(),
        [this, dynamic](const graphics::Settings& proposed) {
            QSettings storage;
            if (!dynamic) storage.beginGroup(QStringLiteral("statisch"));
            QString error;
            if (!graphics::save(storage, proposed, &error)) {
                showError(QStringLiteral("Grafikeinstellungen"), error);
                return false;
            }
            if (dynamic) graphics_ = proposed;
            else staticGraphics_ = proposed;
            applyDisplayOptions();
            return true;
        }, [this] { closeGraphics(); },
        [this](graphics::Feature feature) { return scene_->graphicsFeatureStatus(feature); }, !dynamic);
    sceneStack_->addWidget(graphicsPage_);
    sceneStack_->setCurrentWidget(graphicsPage_);
}

void MainWindow::closeGraphics()
{
    if (!graphicsPage_) return;
    sceneStack_->setCurrentWidget(largeScene_);
    sceneStack_->removeWidget(graphicsPage_);
    graphicsPage_->deleteLater();
    graphicsPage_ = nullptr;
}

void MainWindow::setupMaterialPreview()
{
    const auto model = modelCache_.value(cacheKey(modelId_, modelVersion_));
    if (!model) {
        showError(QStringLiteral("Materialvorschau"), QStringLiteral("Bitte zuerst ein Modell laden."));
        return;
    }
    QStringList elements;
    for (const auto& part : model->parts) {
        if (part.surface != DisplayPart::Surface::Invisible && !elements.contains(part.elementId)) {
            elements.append(part.elementId);
        }
    }
    if (elements.isEmpty()) {
        showError(QStringLiteral("Materialvorschau"), QStringLiteral("Das Modell hat keine sichtbaren Darstellungsteile."));
        return;
    }
    bool accepted = false;
    const QString element = QInputDialog::getItem(this, QStringLiteral("Materialvorschau"),
        QStringLiteral("Darstellungsteil (nur Vorschau, keine Aenderung der Modell-/Physikdaten)"),
        elements, 0, false, &accepted);
    if (!accepted) return;
    const QStringList styles{QStringLiteral("Original"), QStringLiteral("Lack"),
        QStringLiteral("Segelstoff"), QStringLiteral("Holz / Teak"), QStringLiteral("Gebuerstetes Metall")};
    const QString style = QInputDialog::getItem(this, QStringLiteral("Materialvorschau"),
        QStringLiteral("Material fuer diesen Teil; gilt bis zum naechsten Modellladen"),
        styles, 0, false, &accepted);
    if (!accepted) return;
    scene_->setMaterialPreview(element, static_cast<int>(styles.indexOf(style)));
    applyGraphicsProfile();
}

void MainWindow::applyGraphicsProfile(bool restoreHistory)
{
    const auto& dynamicProfile = graphics_.profiles[0];
    for (const auto& entry : {std::make_pair("dynamicTrackAge", dynamicProfile.trackAge),
             std::make_pair("dynamicTrackSpacing", dynamicProfile.trackSpacing),
             std::make_pair("dynamicMarkSpacing", dynamicProfile.markSpacing)}) {
        auto* field = dynamicMotionPanel_->findChild<QDoubleSpinBox*>(QString::fromLatin1(entry.first));
        const QSignalBlocker blocker(field);
        field->setValue(entry.second);
    }
    auto* motionBudget = dynamicMotionPanel_->findChild<QSpinBox*>(QStringLiteral("dynamicTrackBudget"));
    const QSignalBlocker blockBudget(motionBudget);
    motionBudget->setValue(dynamicProfile.trackBudget);
    const std::array<graphics::Feature, 4> motionFeatures{graphics::Feature::Wake,
        graphics::Feature::WaterTrack, graphics::Feature::DistanceMarks, graphics::Feature::Trail};
    for (std::size_t i = 0; i < motionFeatures.size(); ++i) {
        const QSignalBlocker blocker(dynamicMotionChecks_[i]);
        dynamicMotionChecks_[i]->setChecked(graphics_.profiles[0].on(motionFeatures[i]));
        dynamicMotionChecks_[i]->setEnabled(motionFeatures[i] != graphics::Feature::DistanceMarks
            || graphics_.profiles[0].on(graphics::Feature::WaterTrack));
    }
    auto profile = workspaceIndex_ == 1 ? staticGraphics_.profiles[0]
        : graphics_.profiles[0];
    const bool reduced = profile.liveReduction && live_ && !live_->progress().finished;
    if (reduced) {
        for (auto feature : {graphics::Feature::SSAO, graphics::Feature::Shadows,
                 graphics::Feature::Reflection}) {
            profile.enabled[static_cast<std::size_t>(feature)] = false;
        }
    }
    scene_->setGraphicsProfile(profile);
    if (workspaceIndex_ > 0)
        for (auto* view : staticScenes_) view->setGraphicsProfile(profile);
    if (restoreHistory && scene_->updatesActive() && scene_->motionStateCount() == 0
        && displayedIndex_ >= 0 && displayedIndex_ < stateCount()) {
        displayState(displayedIndex_, true);
    }
    const QString prefix = reduced
        ? QStringLiteral("Live-Reduktion aktiv: SSAO, Schatten und Bootsspiegelung aus.\n")
        : QString();
    graphicsStatus_->setText(prefix + scene_->graphicsStatus());
}

void MainWindow::applyDisplayOptions(bool restoreHistory)
{
    if (!graphicsReady_) return;
    graphicsButton_->setToolTip(QStringLiteral("Technikdarstellung für Frei. Seite, Heck und Draufsicht bleiben fest schematisch. Änderungen werden sofort gespeichert."));
    const auto& profile = workspaceIndex_ == 1 ? staticGraphics_.profiles[0]
        : graphics_.profiles[0];
    applyGraphicsProfile(restoreHistory);
    VtkSceneWidget::DisplayOptions options;
    options.forces = forcesCheck_->isChecked();
    auto selection = [](const std::array<QCheckBox*, 5>& checks) {
        return VtkSceneWidget::VectorSelection{checks[0]->isChecked(), checks[1]->isChecked(),
            checks[2]->isChecked(), checks[3]->isChecked(), checks[4]->isChecked()};
    };
    options.force = selection(forceChecks_);
    options.moment = selection(momentChecks_);
    options.leverArm = leverCheck_->isChecked();
    options.lateralCenter = lateralCheck_->isChecked();
    options.sailCenter = sailCenterCheck_->isChecked();
    options.aeroCenter = aeroCenterCheck_->isChecked();
    for (auto it = aeroIndividualChecks_.cbegin(); it != aeroIndividualChecks_.cend(); ++it)
        if (it.value()->isChecked()) options.aeroIndividualCenters.insert(it.key());
    options.aeroBoundaries = aeroBoundariesCheck_->isChecked();
    for (std::size_t i = 0; i < sailForceChecks_.size(); ++i) {
        options.sailForce[i] = sailForceChecks_[i]->isChecked();
        options.sailMoment[i] = sailMomentChecks_[i]->isChecked();
    }
    options.windArrows = windArrowsCheck_->isChecked();
    if (workspaceIndex_ > 0) {
        options.forces = false;
        options.staticForces = true;
        options.staticStyles = staticForceStyles_;
        options.leverArm = false;
        options.lateralCenter = staticForceStyles_[static_cast<std::size_t>(forcedisplay::StaticForce::Lateral)].enabled;
        options.sailCenter = staticForceStyles_[static_cast<std::size_t>(forcedisplay::StaticForce::Sail)].enabled;
        options.courseArrow = staticForceStyles_[static_cast<std::size_t>(forcedisplay::StaticForce::Direction)].enabled;
    }
    options.visibility = profile.throughBody ? VtkSceneWidget::Visibility::ThroughBody
                                            : VtkSceneWidget::Visibility::Spatial;
    options.forceMode =
        static_cast<forcedisplay::ScaleMode>(forceScaleCombo_->currentData().toInt());
    options.momentMode =
        static_cast<forcedisplay::ScaleMode>(momentScaleCombo_->currentData().toInt());
    QList<QWidget*> dependent{leverCheck_, forceScaleCombo_, momentScaleCombo_,
        refitButton_};
    for (auto* check : forceChecks_) {
        dependent << check;
    }
    for (auto* check : momentChecks_) {
        dependent << check;
    }
    for (auto* widget : dependent) {
        widget->setEnabled(options.forces);
    }
    scene_->setOptions(options);
    for (auto* view : staticScenes_) view->setOptions(options);
    updateStaticForceMenu();
    saveDisplaySettings();
    // Ausblenden ändert nur die Grafik; die Skala-Spalte folgt dem Skalenmodus.
    updateValues();
}

void MainWindow::updateStaticForceMenu()
{
    const auto sums = forcedisplay::staticResultants(currentForces_,
        largeScene_->statePose().value_or(forcedisplay::Pose{}), hydrodynamicRoles_);
    for (std::size_t i = 0; i < staticForceStyles_.size(); ++i) {
        const auto& style = staticForceStyles_[i];
        auto* check = staticForceChecks_[i];
        const auto checkStyle = QStringLiteral("QCheckBox { color: %1; }").arg(style.color);
        if (check->styleSheet() != checkStyle) check->setStyleSheet(checkStyle);
        check->setEnabled(hasForces_ && sums[i].available);
        check->setToolTip(style.explanation + QStringLiteral("\n")
            + (i >= 5 ? QStringLiteral(
                "Resultierende, Bezug S: kein nachgewiesener Angriffspunkt und keine vollständige Momentenäquivalenz.\n") : QString())
            + (hasForces_ ? sums[i].diagnostic : QStringLiteral("Noch kein Core-Ergebnis")));
        const auto buttonStyle = QStringLiteral(
            "QPushButton { background-color: %1; border: 1px solid #8794a2; }").arg(style.color);
        if (staticColorButtons_[i]->styleSheet() != buttonStyle)
            staticColorButtons_[i]->setStyleSheet(buttonStyle);
        staticColorButtons_[i]->setToolTip(QStringLiteral("%1: %2").arg(style.name, style.color));
    }
}

void MainWindow::initialReferences(const ModelDisplayData& model)
{
    QSettings settings;
    double force = settings.value(referenceSettingKey(model.modelId, QStringLiteral("kraft")), 0.0)
                       .toDouble();
    const double moment =
        settings.value(referenceSettingKey(model.modelId, QStringLiteral("moment")), 0.0).toDouble();
    if (!forcedisplay::isValidReference(force) && model.totalMassKg && *model.totalMassKg > 0.0) {
        force = std::max(*model.totalMassKg * gravity, 1.0);
    }
    scene_->setReferences(force, moment);
    for (auto* view : staticScenes_) view->setReferences(force, moment);
    storeReferences();
    updateReferenceLabel();
}

void MainWindow::storeReferences() const
{
    if (modelId_.isEmpty()) {
        return;
    }
    QSettings settings;
    if (forcedisplay::isValidReference(scene_->forceReference())) {
        settings.setValue(
            referenceSettingKey(modelId_, QStringLiteral("kraft")), scene_->forceReference());
    }
    if (forcedisplay::isValidReference(scene_->momentReference())) {
        settings.setValue(
            referenceSettingKey(modelId_, QStringLiteral("moment")), scene_->momentReference());
    }
}

void MainWindow::updateReferenceLabel()
{
    auto text = [](double value, const QString& unit) {
        return forcedisplay::isValidReference(value) ? forcedisplay::formatQuantity(value, unit)
                                                     : QStringLiteral("–");
    };
    referenceLabel_->setText(QStringLiteral("R_F: %1  ·  R_M: %2")
            .arg(text(scene_->forceReference(), QStringLiteral("N")),
                text(scene_->momentReference(), QStringLiteral("N·m"))));
}

void MainWindow::chooseModelFolder()
{
    const auto folder = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Modellordner auswählen"), modelFolder_);
    if (folder.isEmpty()) {
        return;
    }
    setModelFolder(folder);
    selectInitialModel({});
}

void MainWindow::setModelFolder(const QString& folder)
{
    const bool usable = !folder.isEmpty() && QDir(folder).exists();
    if (!folder.isEmpty() && !usable) {
        showError(QStringLiteral("Modellordner"),
            QStringLiteral("Der ausgewählte Modellordner ist nicht verfügbar: %1")
                .arg(QDir::toNativeSeparators(folder)));
        return;
    }
    useConfiguredModelFolder_ = folder.isEmpty();
    if (usable) {
        modelFolder_ = QDir(folder).absolutePath();
    } else {
        try {
            modelFolder_ = QString::fromStdWString(
                segelphysik::modellordner_standard().wstring());
        } catch (const std::exception& error) {
            modelFolder_.clear();
            showError(QStringLiteral("Modellpfad-Konfiguration"),
                QString::fromUtf8(error.what()));
        }
    }
    if (usable) {
        QSettings().setValue(folderSettingKey, modelFolder_);
    }
    ++openGeneration_;
    modelId_.clear();
    modelVersion_.clear();
    modelPath_.clear();
    validation_ = {};
    modelCache_.clear();
    resetResults();
    scene_->clearModel();
    for (auto* view : staticScenes_) view->clearModel();
    cameraCombo_->setEnabled(false);
    inspectionCameraCombo_->setEnabled(false);
    inspection_->clear(QStringLiteral("Modellordner wird eingelesen …"));
    setBusy(true);
    reloadCatalog();
}

void MainWindow::selectInitialModel(const QString& preferred)
{
    pendingSelection_ = preferred;
    selectionPending_ = true;
    if (catalogThread_ == nullptr && !catalogReloadPending_
        && catalog_.folder == modelFolder_) {
        applyPendingSelection();
    }
}

void MainWindow::refreshModels()
{
    if (fingerprintThread_ != nullptr || sailSaveInProgress_ || simulationThread_ != nullptr
        || inspection_->manualModeActive()) return;
    modelCache_.clear();
    ++openGeneration_;
    validation_ = {};
    if (simulationThread_ == nullptr) {
        resetResults();
        scene_->clearModel();
        for (auto* view : staticScenes_) view->clearModel();
        inspection_->clear(QStringLiteral("Modell wird erneut geprüft …"));
        cameraCombo_->setEnabled(false);
        inspectionCameraCombo_->setEnabled(false);
        statusLabel_->setText(QStringLiteral("Modell wird erneut geprüft …"));
        statusLabel_->setToolTip(QString());
        statusLabel_->setStyleSheet(QString());
    }
    setBusy(true);
    reopenAfterCatalog_ = true;
    reloadCatalog();
}

void MainWindow::reloadCatalog()
{
    if (catalogThread_ != nullptr) {
        catalogReloadPending_ = true;
        return;
    }
    catalogReloadPending_ = false;
    if (useConfiguredModelFolder_) {
        try {
            const auto configured = QString::fromStdWString(
                segelphysik::modellordner_standard().wstring());
            if (configured != modelFolder_) {
                if (simulationThread_ != nullptr) {
                    defaultsReloadPending_ = true;
                } else {
                    defaultsReloadPending_ = false;
                    setModelFolder({});
                    selectInitialModel({});
                    return;
                }
            }
        } catch (const std::exception& error) {
            if (simulationThread_ == nullptr) modelFolder_.clear();
            else defaultsReloadPending_ = true;
            showError(QStringLiteral("Modellpfad-Konfiguration"),
                QString::fromUtf8(error.what()));
        }
    }
    const QString folder = modelFolder_;
    folderLabel_->setText(QStringLiteral("Katalog wird geladen …"));
    auto* thread = QThread::create([this, folder] {
        auto snapshot = loadCatalog(folder);
        QMetaObject::invokeMethod(
            this, [this, snapshot] { applyCatalog(snapshot); }, Qt::QueuedConnection);
    });
    catalogThread_ = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread] {
        if (catalogThread_ == thread) {
            catalogThread_ = nullptr;
        }
        if (catalogReloadPending_ && !shuttingDown_.load()) {
            reloadCatalog();
        }
    });
    thread->start();
}

void MainWindow::applyCatalog(const CatalogSnapshot& snapshot)
{
    if (snapshot.folder != modelFolder_ || catalogReloadPending_) {
        return;
    }
    const auto previous = entryFor(modelId_);
    const QString previousVersion = previous != nullptr ? previous->version : modelVersion_;
    catalog_ = snapshot;
    const int usable = static_cast<int>(std::count_if(catalog_.entries.cbegin(),
        catalog_.entries.cend(), [](const ModelEntry& e) { return e.usable; }));
    if (modelFolder_.isEmpty()) {
        folderLabel_->setText(QStringLiteral("Kein Modellordner gewählt"));
    } else if (!catalog_.error.isEmpty()) {
        folderLabel_->setText(QStringLiteral("%1 · Katalog nicht lesbar").arg(QDir(modelFolder_).dirName()));
    } else {
        folderLabel_->setText(QStringLiteral("%1 · %2 Modelle (%3 verwendbar)")
                .arg(QDir(modelFolder_).dirName())
                .arg(catalog_.entries.size())
                .arg(usable));
    }
    folderLabel_->setToolTip(catalog_.error.isEmpty()
            ? QDir::toNativeSeparators(modelFolder_)
            : QStringLiteral("%1\n%2").arg(QDir::toNativeSeparators(modelFolder_), catalog_.error));

    {
        const QSignalBlocker blocker(modelList_);
        modelList_->clear();
        for (const auto& entry : catalog_.entries) {
            const QString name = entry.name.isEmpty() ? entry.id : entry.name;
            auto* item = new QListWidgetItem(
                entry.usable ? name : QStringLiteral("%1 %2").arg(warningSign).arg(name), modelList_);
            item->setData(Qt::UserRole, entry.id);
            item->setData(Qt::UserRole + 1, name);
            QStringList tip{QStringLiteral("%1 · Fassung %2").arg(entry.id, entry.version)};
            if (!entry.description.isEmpty()) {
                tip << entry.description;
            }
            if (!entry.path.isEmpty()) {
                tip << QDir::toNativeSeparators(entry.path);
            }
            if (entry.usable) {
                tip << QStringLiteral("Core-Katalog: verwendbar");
            } else {
                item->setForeground(rejectedModelColor);
                tip << QStringLiteral("Core-Katalog: fehlerhaft (%1 Fehler)")
                           .arg(entry.errorCount);
                if (!entry.schemaValid || !entry.visualizationHashCurrent)
                    tip << QStringLiteral("Modellansicht durch Core-Prüfung gesperrt");
                else
                    tip << QStringLiteral("Nur Ansicht; Simulation gesperrt");
                tip << entry.diagnostics;
            }
            item->setToolTip(tip.join(QLatin1Char('\n')));
            if (entry.id == modelId_) {
                modelList_->setCurrentItem(item);
            }
        }
    }

    if (selectionPending_) {
        applyPendingSelection();
        return;
    }
    const auto* current = entryFor(modelId_);
    const bool running = simulationThread_ != nullptr;
    const bool changed = current == nullptr || current->version != previousVersion;
    if (!modelId_.isEmpty() && (changed || reopenAfterCatalog_)) {
        const auto opened = modelCache_.value(cacheKey(modelId_, modelVersion_));
        if (opened && opened->data.inspection &&
            QFileInfo(modelPath_).fileName() != QStringLiteral("modells_data.json")) {
            modelLabel_->setText(QStringLiteral("%1\n(geänderte Modellfassung im Ordner; Inspektion unverändert)")
                .arg(modelName_));
            reopenAfterCatalog_ = false;
            return;
        }
        if (running) {
            // Laufende Simulationen bleiben unberührt (A-G-00022).
            modelLabel_->setText(QStringLiteral("%1\n(geänderte Modellfassung im Ordner)").arg(modelName_));
        } else if (current != nullptr) {
            const QString id = modelId_;
            openModel(id);
        } else {
            modelId_.clear();
            modelVersion_.clear();
            modelPath_.clear();
            modelName_.clear();
            modelLabel_->setText(QStringLiteral("Kein Modell verfügbar"));
            statusLabel_->setText(QStringLiteral("Das gewählte Modell ist nicht mehr verfügbar."));
            setBusy(openThread_ != nullptr);
        }
    } else if (modelId_.isEmpty() && !running && openThread_ == nullptr) {
        setBusy(false);
    }
    reopenAfterCatalog_ = false;
}

void MainWindow::applyPendingSelection()
{
    selectionPending_ = false;
    QString preferred = pendingSelection_;
    pendingSelection_.clear();
    if (preferred.isEmpty()) {
        preferred = QSettings().value(modelSettingKey).toString();
        if (entryFor(preferred) == nullptr) {
            preferred = QSettings().value(legacyModelSettingKey).toString();
        }
    }
    QListWidgetItem* target = nullptr;
    QListWidgetItem* firstUsable = nullptr;
    for (int row = 0; row < modelList_->count(); ++row) {
        auto* item = modelList_->item(row);
        const auto* entry = entryFor(item->data(Qt::UserRole).toString());
        if (entry == nullptr) {
            continue;
        }
        if (firstUsable == nullptr && entry->usable) {
            firstUsable = item;
        }
        if (!preferred.isEmpty()
            && (entry->id == preferred
                || (!entry->path.isEmpty() && samePath(entry->path) == samePath(preferred))
                || (!entry->path.isEmpty()
                    && samePath(QFileInfo(entry->path).absolutePath()) == samePath(preferred)))) {
            target = item;
        }
    }
    if (target == nullptr) {
        target = firstUsable != nullptr ? firstUsable : modelList_->item(0);
    }
    if (target != nullptr) {
        if (modelList_->currentItem() == target) {
            const QString id = target->data(Qt::UserRole).toString();
            if (id != modelId_) {
                openModel(id);
            }
        } else {
            modelList_->setCurrentItem(target);
        }
    } else {
        setBusy(simulationThread_ != nullptr || openThread_ != nullptr);
    }
}

const ModelEntry* MainWindow::entryFor(const QString& id) const
{
    for (const auto& entry : catalog_.entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

void MainWindow::updateVisualizationFingerprint(const QString& modelId)
{
    if (fingerprintThread_ != nullptr || simulationThread_ != nullptr
        || openThread_ != nullptr || catalogThread_ != nullptr) return;
    const auto* entry = entryFor(modelId);
    if (!entry || !entry->schemaValid || entry->visualizationHashCurrent
        || QFileInfo(entry->path).fileName() != QStringLiteral("modells_data.json")) {
        return;
    }

    const auto modelFolder = QFileInfo(entry->path).absolutePath();
    statusLabel_->setText(QStringLiteral("Visualisation-Fingerprint wird aktualisiert …"));
    statusLabel_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
    const auto catalogFolder = modelFolder_;
    auto* thread = QThread::create([this, modelFolder, catalogFolder] {
        QString error;
        try {
            segelphysik::aktualisiere_modell_fingerprints(
                std::filesystem::path(modelFolder.toStdWString()),
                {segelphysik::ModellFingerprintBereich::visualisation});
        } catch (const std::exception& exception) {
            error = QString::fromUtf8(exception.what());
        }
        QMetaObject::invokeMethod(this, [this, catalogFolder, error] {
            if (modelFolder_ != catalogFolder) return;
            if (!error.isEmpty()) {
                fingerprintThread_.clear();
                statusLabel_->setText(QStringLiteral("Fingerprint-Aktualisierung fehlgeschlagen"));
                statusLabel_->setToolTip(error);
                statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
                showModelProblems(problemsFromText(error));
                return;
            }
            reopenAfterCatalog_ = true;
            modelCache_.clear();
            reloadCatalog();
        }, Qt::QueuedConnection);
    });
    fingerprintThread_ = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread] {
        if (fingerprintThread_ == thread) fingerprintThread_ = nullptr;
    });
    thread->start();
}

void MainWindow::resetResults()
{
    renderedSailVersions_.clear();
    live_.reset();
    for (auto it = movementSliders_.cbegin(); it != movementSliders_.cend(); ++it) {
        const auto id = it.key();
        const double initial = stationaryMovements_.value(id)->value();
        const auto range = movementSliderRanges_.value(id);
        const double fraction = range[1] > range[0]
            ? (initial - range[0]) / (range[1] - range[0]) : 0.0;
        const QSignalBlocker blocker(it.value());
        it.value()->setValue(static_cast<int>(std::lround(
            std::clamp(fraction, 0.0, 1.0) * it.value()->maximum())));
        movementConfirmedValues_[id] = initial;
        movementInputPending_[id] = false;
        if (auto* label = movementValueLabels_.value(id, nullptr))
            label->setText(movementValueText(initial, initial, movementValueUnits_.value(id)));
    }
    states_.clear();
    runtimeInfo_.clear();
    displayedIndex_ = -1;
    desiredStateIndex_ = -1;
    updateSliderRange(0);
    stateLabel_->setText(QStringLiteral("Zustand 0 / 0"));
    scene_->clearState();
    for (auto* view : staticScenes_) view->clearState();
    clearValues();
}

void MainWindow::openModel(const QString& id)
{
    if (simulationThread_ != nullptr || sailSaveInProgress_ || inspection_->manualModeActive()) {
        return;
    }
    const auto* entry = entryFor(id);
    if (entry == nullptr) {
        return;
    }
    const bool sameModel = modelId_ == id;
    closeGraphics();
    modelId_ = entry->id;
    cameraCombo_->setEnabled(false);
    inspectionCameraCombo_->setEnabled(false);
    modelVersion_ = entry->version;
    modelPath_ = entry->path;
    modelName_ = entry->name.isEmpty() ? entry->id : entry->name;
    if (!sameModel)
        inspection_->clear(QStringLiteral("Modell wird vom Core geladen …"));
    validation_ = {};
    lastRunDynamic_.reset();
    resetResults();
    QSettings().setValue(modelSettingKey, modelId_);
    {
        const QSignalBlocker blocker(modelList_);
        for (int row = 0; row < modelList_->count(); ++row) {
            auto* item = modelList_->item(row);
            if (item->data(Qt::UserRole).toString() == id) {
                modelList_->setCurrentItem(item);
                modelList_->scrollToItem(item);
                break;
            }
        }
    }
    modelLabel_->setText(modelName_);
    modelLabel_->setToolTip(QStringLiteral("%1 · Fassung %2").arg(modelId_, modelVersion_));
    statusLabel_->setText(QStringLiteral("Modell wird vom Core geladen …"));
    statusLabel_->setToolTip(QString());
    statusLabel_->setStyleSheet(QString());
    timeLabel_->setText(QStringLiteral("Simulationszeit: –  ·  Rechenzeit: –"));

    const ModelEntry selected = *entry;
    const auto catalog = catalog_.catalog;
    const auto cached = modelCache_.value(cacheKey(selected.id, selected.version));
    const int generation = ++openGeneration_;
    setBusy(true);
    auto* thread = QThread::create([this, selected, catalog, cached, generation] {
        std::shared_ptr<OpenedModel> opened = cached;
        QString error;
        ModelValidation validation;
        try {
            if (!catalog)
                throw std::runtime_error("Kein Modellkatalog verfügbar.");
            (void)segelphysik::katalog_datei(*catalog, selected.id.toStdString(),
                selected.version.toStdString());
            if (!opened) {
                auto fresh = std::make_shared<OpenedModel>();
                fresh->data = loadDisplayData(*catalog, selected.id, selected.version);
                if (!fresh->data.geometry.teile.empty())
                    fresh->parts = toDisplayParts(fresh->data.geometry);
                opened = fresh;
                if (opened->data.modelFormat03) {
                    const auto display = std::make_shared<OpenedModel>(*opened);
                    QMetaObject::invokeMethod(this,
                        [this, selected, display, generation] {
                            if (generation == openGeneration_ && selected.id == modelId_)
                                modelDisplayOpened(selected, display);
                        },
                        Qt::QueuedConnection);
                }
            }
            if (opened->data.modelFormat03) {
                if (selected.usable && !opened->data.inspection) {
                    if (!catalog)
                        throw std::runtime_error("Kein Modellkatalog verfügbar.");
                    loadSelectedModelPhysics(opened->data, *catalog);
                }
                if (opened->data.inspection) {
                    if (!opened->sailConfiguration && catalog)
                        opened->sailConfiguration = segelphysik::leseSegelKonfiguration(
                            *catalog, *opened->data.inspection);
                    const auto& info = opened->data.inspection->info();
                    validation.modelId = QString::fromStdString(info.model_id);
                    validation.fingerprint = QString::fromStdString(info.model_version);
                    validation.valid = info.status == "verwendbar";
                    QStringList diagnostics;
                    for (const auto& diagnostic : info.diagnostics)
                        diagnostics << QStringLiteral("%1: %2")
                            .arg(QString::fromStdString(diagnostic.fehler_id),
                                QString::fromStdString(diagnostic.text));
                    validation.diagnostic = diagnostics.join(QLatin1Char('\n'));
                    if (validation.valid) {
                        try {
                            const auto model = segelphysik::modellAusInspektion(*opened->data.inspection);
                            const auto prepared = segelphysik::sailforce_start(model, {},
                                std::filesystem::path(selected.path.toStdWString()));
                            opened->sailIds.clear();
                            opened->sailDiagnostic.clear();
                            if (prepared.sailforce)
                                for (const auto& sail : prepared.sailforce->konfiguration.segel)
                                    opened->sailIds.push_back(QString::fromStdString(sail.element_id));
                        } catch (const std::exception& exception) {
                            opened->sailDiagnostic = QString::fromUtf8(exception.what());
                        }
                    }
                } else {
                    validation.diagnostic = selected.diagnostics.join(QLatin1Char('\n'));
                }
            } else if (selected.usable) {
                validation = validateModel(selected.path);
            } else {
                validation.diagnostic = selected.diagnostics.join(QLatin1Char('\n'));
            }
        } catch (const std::exception& exception) {
            error = QString::fromUtf8(exception.what());
        }
        QMetaObject::invokeMethod(this,
            [this, selected, opened, validation, error, generation] {
                if (generation != openGeneration_ || selected.id != modelId_) {
                    return;
                }
                if (!error.isEmpty()) {
                    setBusy(false);
                    if (isOutdatedVersionError(error)) {
                        statusLabel_->setText(QStringLiteral(
                            "Modell wurde im Ordner geändert – Katalog wird neu geladen."));
                        statusLabel_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
                        reopenAfterCatalog_ = true;
                        modelCache_.clear();
                        reloadCatalog();
                        return;
                    }
                    statusLabel_->setText(
                        QStringLiteral("Darstellung nicht verfügbar: %1").arg(error));
                    statusLabel_->setToolTip(error);
                    statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
                    if (!opened || !opened->data.modelFormat03) {
                        scene_->clearModel();
                        for (auto* view : staticScenes_) view->clearModel();
                        inspection_->clear(error);
                    }
                    showModelProblems(selected.problems + problemsFromText(error));
                    return;
                }
                if (opened->data.modelFormat03 && !opened->data.inspection) {
                    inspection_->clear(validation.diagnostic);
                    statusLabel_->setText(QStringLiteral(
                        "%1 Ansicht geladen – Physik nicht freigegeben")
                            .arg(warningSign));
                    statusLabel_->setToolTip(validation.diagnostic);
                    statusLabel_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
                    showModelProblems(selected.problems +
                        problemsFromText(validation.diagnostic));
                    setBusy(false);
                    return;
                }
                modelCache_.insert(cacheKey(selected.id, selected.version), opened);
                modelOpened(selected, opened, validation);
            },
            Qt::QueuedConnection);
    });
    openThread_ = thread;
    openThreads_.insert(thread);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread] {
        openThreads_.remove(thread);
        if (openThread_ == thread) {
            openThread_ = nullptr;
        }
        setBusy(simulationThread_ != nullptr || openThread_ != nullptr);
    });
    thread->start();
}

void MainWindow::modelDisplayOpened(const ModelEntry& entry,
    const std::shared_ptr<OpenedModel>& opened)
{
    QString error;
    inspection_->setStructure(opened->data.structure);
    scene_->setElementGroups(opened->data.elementGroups);
    for (auto* view : staticScenes_) view->setElementGroups(opened->data.elementGroups);
    configureMovementControls(opened->data);
    configureSailControls(*opened);
    if (opened->parts.isEmpty()) {
        scene_->clearModel();
        for (auto* view : staticScenes_) view->clearModel();
    } else if (!scene_->setModel(opened->parts, &error)) {
        statusLabel_->setText(QStringLiteral("Darstellung nicht verfügbar: %1").arg(error));
        statusLabel_->setToolTip(error);
        statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
        showModelProblems(entry.problems + problemsFromText(error));
        return;
    }
    for (auto* view : staticScenes_) {
        if (!opened->parts.isEmpty() && !view->setModel(opened->parts, &error)) {
            for (auto* other : staticScenes_) other->clearModel();
            statusLabel_->setText(QStringLiteral("Zusatzansicht nicht verfügbar: %1").arg(error));
            showModelProblems(entry.problems + problemsFromText(error));
            return;
        }
    }
    cameraCombo_->setEnabled(true);
    inspectionCameraCombo_->setEnabled(!opened->parts.isEmpty());
    {
        const QSignalBlocker blocker(cameraCombo_);
        cameraCombo_->setCurrentIndex(0);
    }
    initialReferences(opened->data);
    applyDisplayOptions();
    clearValues();
    statusLabel_->setText(QStringLiteral("Objektbaum und Darstellung geladen"));
    statusLabel_->setToolTip(QStringLiteral(
        "Physikdaten werden im Core im Hintergrund geladen; Aerodynamik ist vorläufig ausgenommen."));
    statusLabel_->setStyleSheet(QString());
    modelLabel_->setToolTip(QStringLiteral("%1 · Fassung %2")
        .arg(entry.id, entry.version));
    setBusy(false);
}

void MainWindow::configureSailControls(const OpenedModel& model)
{
    while (auto* item = aeroIndividualLayout_->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    aeroIndividualChecks_.clear();
    while (auto* item = sailLayout_->takeAt(0)) {
        if (item->widget() && item->widget() != sailStatus_) delete item->widget();
        delete item;
    }
    sailActive_.clear();
    sailAngles_.clear();
    sailPending_.clear();
    sailPendingRevision_.clear();
    for (const auto& id : model.sailIds) {
        auto* pointCheck = new QCheckBox(QStringLiteral("SDP %1").arg(id), aeroIndividualPanel_);
        const QString pointKey = QStringLiteral("darstellung/sailforce_sdp/%1/%2").arg(modelId_, id);
        pointCheck->setChecked(QSettings().value(pointKey, false).toBool());
        aeroIndividualChecks_.insert(id, pointCheck);
        aeroIndividualLayout_->addWidget(pointCheck);
        connect(pointCheck, &QCheckBox::toggled, this, [this, pointKey](bool on) {
            QSettings().setValue(pointKey, on);
            applyDisplayOptions();
        });
        auto* row = new QWidget(sailPanel_);
        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        auto* active = new QCheckBox(QStringLiteral("aktiv"), row);
        const QString key = QStringLiteral("sailforce/%1/%2/aktiv").arg(modelId_, id);
        active->setChecked(QSettings().value(key, true).toBool());
        auto* angle = new QDoubleSpinBox(row);
        angle->setRange(-180., 180.);
        angle->setDecimals(2);
        angle->setSuffix(QStringLiteral("°"));
        layout->addWidget(active);
        layout->addWidget(angle);
        sailActive_.insert(id, active);
        sailAngles_.insert(id, angle);
        sailLayout_->addRow(id, row);
        connect(active, &QCheckBox::toggled, this, [key](bool checked) {
            QSettings().setValue(key, checked);
        });
        connect(angle, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, id](double value) {
            if (!live_ || simulationThread_ == nullptr || !modeCombo_->currentData().toBool()) return;
            QString error;
            const auto priorRevision = live_->progress().sailforceRevision;
            if (!live_->setSailAngle(id, value, &error)) {
                sailStatus_->setText(error);
                return;
            }
            sailPending_.insert(id, value);
            sailPendingRevision_.insert(id, priorRevision);
            sailStatus_->setText(QStringLiteral("%1: Stellung ausstehend bis zur nächsten Schrittgrenze.").arg(id));
        });
    }
    sailStatus_->setText(!model.sailDiagnostic.isEmpty() ? model.sailDiagnostic
        : model.sailIds.isEmpty() ? QStringLiteral("Keine explizite Segelmodellzuordnung.")
        : QStringLiteral("Aktivierung gilt für den nächsten Lauf."));
    sailLayout_->addRow(sailStatus_);
}

void MainWindow::modelOpened(const ModelEntry& entry, const std::shared_ptr<OpenedModel>& opened,
    const ModelValidation& validation)
{
    QString error;
    renderedSailVersions_.clear();
    inspection_->setSnapshot(opened->data.inspection, catalog_.catalog, true,
        opened->sailConfiguration);
    inspection_->setEnabled(true);
    scene_->setElementGroups(opened->data.elementGroups);
    hydrodynamicRoles_ = opened->data.hydrodynamicRoles;
    scene_->setHydrodynamicRoles(hydrodynamicRoles_);
    for (auto* view : staticScenes_) view->setHydrodynamicRoles(hydrodynamicRoles_);
    for (auto* view : staticScenes_) view->setElementGroups(opened->data.elementGroups);
    configureMovementControls(opened->data);
    configureSailControls(*opened);
    if (opened->parts.isEmpty()) {
        scene_->clearModel();
        for (auto* view : staticScenes_) view->clearModel();
    } else if (!scene_->setModel(opened->parts, &error)) {
        statusLabel_->setText(QStringLiteral("Darstellung nicht verfügbar: %1").arg(error));
        statusLabel_->setToolTip(error);
        statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
        showModelProblems(entry.problems + problemsFromText(error));
        setBusy(false);
        return;
    }
    for (auto* view : staticScenes_) {
        if (!opened->parts.isEmpty() && !view->setModel(opened->parts, &error)) {
            for (auto* other : staticScenes_) other->clearModel();
            statusLabel_->setText(QStringLiteral("Zusatzansicht nicht verfügbar: %1").arg(error));
            showModelProblems(entry.problems + problemsFromText(error));
            setBusy(false);
            return;
        }
    }
    cameraCombo_->setEnabled(true);
    inspectionCameraCombo_->setEnabled(!opened->parts.isEmpty());
    {
        const QSignalBlocker blocker(cameraCombo_);
        cameraCombo_->setCurrentIndex(0);
    }
    {
        const QSignalBlocker blocker(inspectionCameraCombo_);
        const int inspectionPreset = inspectionCameraCombo_->findData(QStringLiteral("backbord"));
        inspectionCameraCombo_->setCurrentIndex(inspectionPreset < 0 ? 0 : inspectionPreset);
    }
    if (!opened->parts.isEmpty()
        && !inspection_->setCameraPreset(QStringLiteral("backbord"), &error)) {
        showError(QStringLiteral("Kameraansicht"), error);
    }
    initialReferences(opened->data);
    applyDisplayOptions();
    clearValues();
    validation_ = validation;
    QStringList tooltip{QStringLiteral("%1 · Fassung %2").arg(entry.id, entry.version),
        QDir::toNativeSeparators(entry.path)};
    if (opened->data.totalMassKg) {
        tooltip << QStringLiteral("Gesamtmasse: %1")
                       .arg(forcedisplay::formatQuantity(*opened->data.totalMassKg, QStringLiteral("kg")));
    }
    tooltip << opened->data.partDiagnostics;
    modelLabel_->setToolTip(tooltip.join(QLatin1Char('\n')));
    statusLabel_->setStyleSheet(QString());
    if (validation.valid) {
        statusLabel_->setText(QStringLiteral("Core-Prüfung erfolgreich"));
        statusLabel_->setToolTip(QStringLiteral("%1 · %2").arg(validation.modelId, validation.fingerprint));
    } else {
        const auto code = QRegularExpression(QStringLiteral("CORE-[A-Z]+-\\d+"))
                              .match(validation.diagnostic)
                              .captured();
        statusLabel_->setText(QStringLiteral(
            "%1 Nur Ansicht – vom Core nicht zur Simulation freigegeben%2 "
            "(Details im Wertefenster)")
                .arg(warningSign)
                .arg(code.isEmpty() ? QString() : QStringLiteral(": ") + code));
        statusLabel_->setToolTip(validation.diagnostic);
        statusLabel_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
        showModelProblems(!entry.problems.isEmpty() ? entry.problems
                                                    : problemsFromText(validation.diagnostic));
    }
    setBusy(openThread_ != nullptr && openThread_->isRunning());
}

void MainWindow::configureMovementControls(const ModelDisplayData& displayData)
{
    while (auto* item = movementLayout_->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    movementSliders_.clear();
    movementValueLabels_.clear();
    movementSliderRanges_.clear();
    movementCoreRanges_.clear();
    movementValueUnits_.clear();
    movementConfirmedValues_.clear();
    movementInputPending_.clear();
    stationaryMovements_.clear();
    auto ids = displayData.elementMovements.keys();
    ids.sort();
    for (const auto& id : ids) {
        const auto definition = displayData.elementMovements.value(id);
        const bool rudder = definition.ruderwinkel;
        const auto groupId = displayData.movementGroups.value(id);
        const bool groupMovement = !groupId.isEmpty();
        const auto groupPreview = groupMovement && displayData.inspection
            ? segelphysik::getGroupMovementPreview(*displayData.inspection,
                groupId.toStdString())
            : std::nullopt;
        auto* slider = new QSlider(Qt::Horizontal, movementPanel_);
        slider->setObjectName(QStringLiteral("movement_%1").arg(id));
        slider->setRange(0, rudder ? 1800 : 1000);
        slider->setEnabled(false);
        const auto preview = !groupMovement && displayData.inspection
            ? segelphysik::getMovementPreview(*displayData.inspection, id.toStdString())
            : std::nullopt;
        const bool movementAvailable = (groupMovement && groupPreview.has_value())
            || preview.has_value();
        const double defaultValue = groupMovement && groupPreview
            ? groupPreview->reference_value
            : preview ? preview->reference_value : (rudder ? 0.0 : 100.0);
        const std::array<double, 2> sliderRange{
            groupMovement && groupPreview && groupPreview->minimum_value
                ? *groupPreview->minimum_value : rudder ? -90.0 : 0.0,
            groupMovement && groupPreview && groupPreview->maximum_value
                ? *groupPreview->maximum_value : rudder ? 90.0 : 100.0};
        const QString unit = groupMovement && groupPreview
            ? QString::fromStdString(groupPreview->value_unit)
            : preview ? QString::fromStdString(preview->value_unit)
                      : rudder ? QStringLiteral("deg") : QStringLiteral("%");
        auto coreRange = sliderRange;
        if (groupMovement && !rudder && groupPreview &&
            definition.art == segelphysik::Bewegungsart::drehen && unit == QStringLiteral("deg")) {
            for (auto& value : coreRange)
                value = 100.0 - (value - groupPreview->reference_value) / 0.9;
        }
        movementSliderRanges_.insert(id, sliderRange);
        movementCoreRanges_.insert(id, coreRange);
        movementValueUnits_.insert(id, unit);
        const int sliderMaximum = slider->maximum();
        const double sliderFraction = sliderRange[1] > sliderRange[0]
            ? (defaultValue - sliderRange[0]) / (sliderRange[1] - sliderRange[0]) : 0.0;
        slider->setValue(static_cast<int>(std::lround(
            std::clamp(sliderFraction, 0.0, 1.0) * sliderMaximum)));
        auto* valueLabel = new QLabel(movementValueText(defaultValue, defaultValue, unit), movementPanel_);
        valueLabel->setObjectName(QStringLiteral("movementValue_%1").arg(id));
        valueLabel->setMinimumWidth(125);
        auto* row = new QWidget(movementPanel_);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->addWidget(slider, 1);
        rowLayout->addWidget(valueLabel);
        movementLayout_->addRow(id, row);
        auto* fixed = new QDoubleSpinBox(movementPanel_);
        fixed->setObjectName(QStringLiteral("stationaryMovement_%1").arg(id));
        fixed->setDecimals(3);
        fixed->setKeyboardTracking(false);
        const auto minimum = preview && preview->minimum_value ? *preview->minimum_value
            : groupMovement && groupPreview && groupPreview->minimum_value
                ? *groupPreview->minimum_value : rudder ? -1e6 : 0.0;
        const auto maximum = preview && preview->maximum_value ? *preview->maximum_value
            : groupMovement && groupPreview && groupPreview->maximum_value
                ? *groupPreview->maximum_value : rudder ? 1e6 : 100.0;
        fixed->setRange(minimum, maximum);
        fixed->setProperty("coreMovementAvailable", movementAvailable);
        if (movementAvailable) {
            fixed->setValue(defaultValue);
            fixed->setSuffix(QStringLiteral(" ") + unit);
        } else {
            fixed->setSpecialValueText(QStringLiteral("Core-Bewegungsmetadaten fehlen"));
            fixed->setValue(fixed->minimum());
        }
        const bool bounded = movementAvailable &&
            ((preview && preview->minimum_value && preview->maximum_value) ||
             (groupPreview && groupPreview->minimum_value && groupPreview->maximum_value));
        fixed->setToolTip(!movementAvailable
            ? QStringLiteral("Statischer Lauf gesperrt: Core-Bewegungsmetadaten für dieses Element fehlen.")
            : (rudder
            ? QStringLiteral("Core-Ruderwinkel in Grad.")
            : QStringLiteral("Core-Bewegungsstellung in Prozent."))
            + (bounded
                ? QStringLiteral(" Grenzen aus der Core-Bewegungsdefinition.")
                : QStringLiteral(" Fehlende physikalische Grenzen werden nicht erfunden; Core validiert.")));
        movementLayout_->addRow(id, fixed);
        stationaryMovements_.insert(id, fixed);
        connect(fixed, &QDoubleSpinBox::valueChanged, this, [this, id, fixed] {
            if (live_ && simulationThread_ && lastRunDynamic_.value_or(false)) {
                QString error;
                if (!live_->setMovementValue(id, coreMovementValue(id, fixed->value()), &error)) {
                    showError(QStringLiteral("Ruder-/Schwertstellung"), error);
                    return;
                }
                statusLabel_->setText(QStringLiteral(
                    "Stellung angefordert: gilt ab dem nächsten Simulationsschritt; Historie unverändert."));
                return;
            }
            if (lastRunDynamic_ && !*lastRunDynamic_ && displayedIndex_ >= 0)
                statusLabel_->setText(QStringLiteral(
                    "Stellung geändert – angezeigtes Ergebnis gehört zum vorherigen Rechenfall."));
        });
        movementSliders_.insert(id, slider);
        movementValueLabels_.insert(id, valueLabel);
        movementConfirmedValues_.insert(id, defaultValue);
        movementInputPending_.insert(id, false);
        connect(slider, &QSlider::valueChanged, this, [this, id, slider, valueLabel, rudder](int position) {
            const auto range = movementSliderRanges_.value(id);
            const double fraction = static_cast<double>(position) / slider->maximum();
            const double value = range[0] + fraction * (range[1] - range[0]);
            movementInputPending_[id] = true;
            valueLabel->setText(movementValueText(value,
                movementConfirmedValues_.value(id, rudder ? 0.0 : 100.0),
                movementValueUnits_.value(id)));
            if (!live_ || simulationThread_ == nullptr) return;
            QString error;
            if (!live_->setMovementValue(id, coreMovementValue(id, value), &error)) {
                movementInputPending_[id] = false;
                const double confirmed = movementConfirmedValues_.value(id, rudder ? 0.0 : 100.0);
                const auto sliderRange = movementSliderRanges_.value(id);
                const double confirmedFraction = sliderRange[1] > sliderRange[0]
                    ? (confirmed - sliderRange[0]) / (sliderRange[1] - sliderRange[0]) : 0.0;
                const QSignalBlocker blocker(slider);
                slider->setValue(static_cast<int>(std::lround(
                    std::clamp(confirmedFraction, 0.0, 1.0) * slider->maximum())));
                valueLabel->setText(movementValueText(confirmed, confirmed,
                    movementValueUnits_.value(id)));
                showError(QStringLiteral("Ruder-/Schwertstellung"), error);
            }
        });
    }
    movementPanel_->setVisible(!ids.isEmpty());
}

void MainWindow::updateMovementControls(const QJsonObject& state)
{
    for (const auto& item : state.value(QStringLiteral("bewegungen")).toArray()) {
        const auto movement = item.toObject();
        const auto id = movement.value(QStringLiteral("element_id")).toString();
        const auto value = movement.value(QStringLiteral("wert"));
        auto* slider = movementSliders_.value(id, nullptr);
        auto* label = movementValueLabels_.value(id, nullptr);
        if (slider == nullptr || label == nullptr || !value.isDouble()) continue;
        const double setting = displayMovementValue(id, value.toDouble());
        movementConfirmedValues_[id] = setting;
        const auto range = movementSliderRanges_.value(id);
        const double requested = range[0] +
            static_cast<double>(slider->value()) / slider->maximum() *
            (range[1] - range[0]);
        const bool pending = movementInputPending_.value(id, false);
        if (pending && std::abs(requested - setting) < 1e-9)
            movementInputPending_[id] = false;
        if (!movementInputPending_.value(id, false)) {
            const QSignalBlocker blocker(slider);
            const double fraction = range[1] > range[0]
                ? (setting - range[0]) / (range[1] - range[0]) : 0.0;
            slider->setValue(static_cast<int>(std::lround(
                std::clamp(fraction, 0.0, 1.0) * slider->maximum())));
        }
        const double displayedRequest = movementInputPending_.value(id, false)
            ? requested : setting;
        label->setText(movementValueText(displayedRequest, setting,
            movementValueUnits_.value(id)));
    }
}

double MainWindow::coreMovementValue(const QString& elementId, double displayValue) const
{
    const auto displayRange = movementSliderRanges_.value(elementId);
    const auto coreRange = movementCoreRanges_.value(elementId, displayRange);
    const double span = displayRange[1] - displayRange[0];
    if (!(span > 0.0)) return coreRange[0];
    const double fraction = (displayValue - displayRange[0]) / span;
    return coreRange[0] + fraction * (coreRange[1] - coreRange[0]);
}

double MainWindow::displayMovementValue(const QString& elementId, double coreValue) const
{
    const auto displayRange = movementSliderRanges_.value(elementId);
    const auto coreRange = movementCoreRanges_.value(elementId, displayRange);
    const double span = coreRange[1] - coreRange[0];
    if (std::abs(span) <= 1e-12) return displayRange[0];
    const double fraction = (coreValue - coreRange[0]) / span;
    return displayRange[0] + fraction * (displayRange[1] - displayRange[0]);
}

void MainWindow::startSimulation()
{
    if (!validation_.valid || modelPath_.isEmpty()) {
        showError(QStringLiteral("Kein gültiges Modell"),
            QStringLiteral("Bitte zuerst ein vom Core akzeptiertes Modell laden."));
        return;
    }
    if (simulationThread_ != nullptr) {
        return;
    }
    startValidatedSimulation();
}

void MainWindow::startValidatedSimulation()
{
    const auto* workspace = findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
    const bool dynamic = workspace && workspace->currentIndex() == 2;
    for (auto it = stationaryMovements_.cbegin(); it != stationaryMovements_.cend(); ++it) {
        if (!it.value()->property("coreMovementAvailable").toBool()) {
            showError(QStringLiteral("Core-Bewegungsdefinition fehlt"),
                QStringLiteral("Kein Core-Lauf: Bewegungsmetadaten für %1 fehlen.").arg(it.key()));
            return;
        }
    }
    double duration = durationSpin_->value();
    double step = stepSpin_->value();
    if (dynamic) {
        QDialog dialog(this);
        dialog.setObjectName(QStringLiteral("dynamicRunDialog"));
        dialog.setWindowTitle(QStringLiteral("Dynamischen Core-Lauf starten"));
        auto* layout = new QFormLayout(&dialog);
        auto* runDuration = new QDoubleSpinBox(&dialog);
        runDuration->setObjectName(QStringLiteral("runDuration"));
        runDuration->setRange(durationSpin_->minimum(), durationSpin_->maximum());
        runDuration->setDecimals(3);
        runDuration->setSuffix(QStringLiteral(" s"));
        runDuration->setSpecialValueText(QStringLiteral("Bis Abbruch"));
        runDuration->setValue(duration);
        auto* runStep = new QDoubleSpinBox(&dialog);
        runStep->setObjectName(QStringLiteral("runStep"));
        runStep->setRange(stepSpin_->minimum(), stepSpin_->maximum());
        runStep->setDecimals(4);
        runStep->setSuffix(QStringLiteral(" s"));
        runStep->setValue(step);
        auto* paced = new QCheckBox(QStringLiteral("Echtzeit (1 s = 1 s)"), &dialog);
        paced->setObjectName(QStringLiteral("runRealtime"));
        paced->setChecked(realtimeCheck_->isChecked());
        layout->addRow(QStringLiteral("Dauer"), runDuration);
        layout->addRow(QStringLiteral("Schrittweite"), runStep);
        layout->addRow(QStringLiteral("Takt"), paced);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Starten"));
        layout->addRow(buttons);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
            if (runDuration->value() > 0.
                && runDuration->value() / runStep->value() > LiveSimulation::maximumStates - 1) {
                showError(QStringLiteral("Laufparameter"),
                    QStringLiteral("Dauer und Schrittweite ergeben zu viele Zustände (maximal %1).")
                        .arg(LiveSimulation::maximumStates));
                return;
            }
            dialog.accept();
        });
        if (dialog.exec() != QDialog::Accepted) return;
        duration = runDuration->value();
        step = runStep->value();
        durationSpin_->setValue(duration);
        stepSpin_->setValue(step);
        realtimeCheck_->setChecked(paced->isChecked());
    }
    QJsonObject start = nextRunInitialState_;
    nextRunInitialState_ = {};
    lastRunDynamic_ = dynamic;
    cancellation_.store(false);
    resetResults();
    statusLabel_->setToolTip(QString());
    statusLabel_->setStyleSheet(QString());
    QJsonObject stationary;
    const auto opened = modelCache_.value(cacheKey(modelId_, modelVersion_));
    segelphysik::MastspitzenErgebnis mast;
    if (opened && opened->data.inspection)
        mast = segelphysik::bestimmeMastspitze(*opened->data.inspection);
    else mast.diagnose = "Modellinspektion fuer Mastspitze nicht verfuegbar";
    QJsonObject wind{{QStringLiteral("richtung_grad"), windDirection_},
        {QStringLiteral("v0_m_s"), windSpeed_},
        {QStringLiteral("h0_m"), windHeight_},
        {QStringLiteral("bootmitgefuehrt"), !dynamic || windMode_->currentData().toBool()},
        {QStringLiteral("mast_element_id"), QString::fromStdString(mast.element_id)},
        {QStringLiteral("mast_quelle"), QString::fromStdString(mast.quelle)},
        {QStringLiteral("mast_diagnose"), QString::fromStdString(mast.diagnose)}};
    if (mast.position_modell_m) {
        wind.insert(QStringLiteral("mastspitze_modell_m"),
            QJsonArray{(*mast.position_modell_m)[0], (*mast.position_modell_m)[1],
                       (*mast.position_modell_m)[2]});
        windStatus_->clear();
    } else {
        windStatus_->setText(QString::fromStdString(mast.diagnose));
    }
    start.insert(QStringLiteral("wind_eingabe"), wind);
    QJsonArray sailChoices;
    for (auto it = sailActive_.cbegin(); it != sailActive_.cend(); ++it)
        sailChoices.append(QJsonObject{{QStringLiteral("element_id"), it.key()},
            {QStringLiteral("aktiv"), it.value()->isChecked()},
            {QStringLiteral("stellwinkel_grad"), sailAngles_.value(it.key())->value()}});
    start.insert(QStringLiteral("sailforce_eingabe"), QJsonObject{
        {QStringLiteral("luft"), QJsonObject{
            {QStringLiteral("dichte_kg_m3"), airDensity_},
            {QStringLiteral("kinematische_viskositaet_m2_s"), airViscosity_}}},
        {QStringLiteral("segel"), sailChoices}});
    stationary.insert(QStringLiteral("aktiv"), stationaryForceActive_->isChecked());
    stationary.insert(QStringLiteral("kurvenfahrt"), true);
    stationary.insert(QStringLiteral("modellfest"), true);
    QJsonArray force, point, settings;
    for (int axis = 0; axis < 3; ++axis) {
        force.append(stationaryForce_[axis]->value());
        point.append(stationaryPoint_[axis]->value());
    }
    stationary.insert(QStringLiteral("kraft_n"), force);
    stationary.insert(QStringLiteral("angriffspunkt_modell_m"), point);
    for (auto it = stationaryMovements_.cbegin(); it != stationaryMovements_.cend(); ++it)
        settings.append(QJsonObject{{QStringLiteral("element_id"), it.key()},
            {QStringLiteral("wert"), coreMovementValue(it.key(), it.value()->value())}});
    stationary.insert(QStringLiteral("stellungen"), settings);
    if (dynamic) {
        start.insert(QStringLiteral("dynamische_fremdkraft"), stationary);
        lastRunInput_ = start;
        startLiveSimulation(duration, step, realtimeCheck_->isChecked(), start);
        return;
    }
    start.insert(QStringLiteral("stationaer"), stationary);

    const QString path = modelPath_;
    const QString id = modelId_;
    const auto validation = validation_;
    const auto inspection = opened ? opened->data.inspection : nullptr;
    statusLabel_->setText(QStringLiteral("Statischer Core-Lauf läuft …"));
    timeLabel_->setText(QStringLiteral("Simulationszeit: –  ·  Rechenzeit: läuft"));
    setBusy(true);

    const int generation = simulationGeneration_;
    auto* thread = QThread::create([this, path, id, validation, start, generation, inspection] {
        const auto result = runSimulation(path, validation, false, 0.0, 0.01,
            &cancellation_, start, inspection);
        QMetaObject::invokeMethod(this,
            [this, id, result, generation] {
                if (id != modelId_ || generation != simulationGeneration_) {
                    return;
                }
                displaySimulationResult(result);
            },
            Qt::QueuedConnection);
    });
    simulationThread_ = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread] {
        if (simulationThread_ == thread) {
            simulationThread_ = nullptr;
        }
        simulationFinished();
    });
    thread->start();
    cancelButton_->setEnabled(true);
}

void MainWindow::startLiveSimulation(double duration, double step, bool paced, const QJsonObject& start)
{
    performanceSamples_ = {};
    lastFetchTimings_ = {};
    nextStateFetch_ = {};
    pendingPresentation_ = {};
    awaitingPresentation_ = false;
    const auto path = modelPath_;
    const auto validation = validation_;
    const auto opened = modelCache_.value(cacheKey(modelId_, modelVersion_));
    const auto inspection = opened ? opened->data.inspection : nullptr;
    const int generation = simulationGeneration_;
    statusLabel_->setText(QStringLiteral("Dynamischer Core-Lauf wird vorbereitet …"));
    setBusy(true);
    auto live = std::make_shared<std::shared_ptr<LiveSimulation>>();
    auto error = std::make_shared<QString>();
    auto* thread = QThread::create([path, validation, duration, step, paced, start, live, error,
                                    inspection] {
        *live = LiveSimulation::create(path, validation, duration, step, paced, error.get(),
            start, std::nullopt, inspection);
    });
    simulationThread_ = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread, path, generation, paced, live, error] {
        if (simulationThread_ == thread) simulationThread_ = nullptr;
        if (shuttingDown_.load()) return;
        if (generation != simulationGeneration_ || path != modelPath_) {
            setBusy(openThread_ != nullptr || simulationThread_ != nullptr);
            return;
        }
        startPreparedLiveSimulation(*live, paced, *error);
    });
    thread->start();
}

void MainWindow::startPreparedLiveSimulation(
    const std::shared_ptr<LiveSimulation>& live, bool paced, const QString& error)
{
    if (!live) {
        statusLabel_->setText(QStringLiteral("Core-Lauf nicht gestartet"));
        showError(QStringLiteral("Core-Simulation"), error);
        setBusy(false);
        return;
    }
    live_ = live;
    followLive_ = true;
    QString initialError;
    const auto initial = live->initialStateJson();
    if (scene_->updatesActive() && !scene_->appendMotionStates({initial}, &initialError)) {
        showError(QStringLiteral("Core-Startzustand"), initialError);
        setBusy(false);
        return;
    }
    displayFetchedState(0, initial, initialError);
    if (cancellation_.load()) {
        setBusy(false);
        return;
    }
    statusLabel_->setText(paced ? QStringLiteral("Dynamik läuft in Echtzeit …")
                                : QStringLiteral("Dynamik läuft ungebremst …"));

    auto* thread = QThread::create([this, live] { live->run(cancellation_); });
    simulationThread_ = thread;
    setBusy(true);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread, live] {
        if (simulationThread_ == thread) {
            simulationThread_ = nullptr;
        }
        if (live_ == live) {
            finishLiveSimulation();
        }
        simulationFinished();
    });
    thread->start(QThread::HighPriority);
    cancelButton_->setEnabled(true);
    liveTimer_->start();
    updateLiveProgress();
}

void MainWindow::simulationFinished()
{
    if (defaultsReloadPending_) {
        defaultsReloadPending_ = false;
        refreshModels();
        return;
    }
    const auto* tabs = findChild<QTabBar*>(QStringLiteral("workspaceTabs"));
    if (tabs && tabs->currentIndex() != 0) {
        const QSignalBlocker blocker(modeCombo_);
        modeCombo_->setCurrentIndex(tabs->currentIndex() == 2 ? 1 : 0);
    }
    setBusy(openThread_ != nullptr && openThread_->isRunning());
    const auto* current = entryFor(modelId_);
    if (current != nullptr && current->version != modelVersion_) {
        statusLabel_->setText(statusLabel_->text()
            + QStringLiteral("\nModell im Ordner geändert – zum Aktualisieren neu wählen."));
    }
}

void MainWindow::updateLiveProgress()
{
    if (!live_) {
        liveTimer_->stop();
        return;
    }
    const auto progress = live_->progress();
    if (!progress.testDriveStatus.isEmpty()) {
        testDriveStatus_->setText(progress.testDriveStatus);
        statusLabel_->setText(progress.testDriveStatus);
    }
    applyGraphicsProfile();
    const double load =
        progress.wallSeconds > 0.0 ? 100.0 * progress.coreSeconds / progress.wallSeconds : 0.0;
    runtimeInfo_ = QStringLiteral("Echtzeit: %1 s  ·  Core-Rechenzeit: %2 (%3 %)")
                       .arg(progress.wallSeconds, 0, 'f', 2)
                       .arg(runtimeText(progress.coreSeconds))
                       .arg(load, 0, 'f', 1);
    updateSliderRange(progress.states);
    if (progress.states == 0) {
        return;
    }
    setBusy(simulationThread_ != nullptr);
    const auto last = std::min<qsizetype>(progress.states - 1, std::numeric_limits<int>::max());
    if (followLive_ && stateSlider_->value() != static_cast<int>(last)) {
        programmaticSlider_ = true;
        stateSlider_->setValue(static_cast<int>(last));
        programmaticSlider_ = false;
    } else {
        // Rückblick: gleicher Zustand wird nicht erneut aufgebaut (A-G-00018).
        displayState(stateSlider_->value());
    }
}

void MainWindow::finishLiveSimulation()
{
    liveTimer_->stop();
    updateLiveProgress();
    const auto progress = live_->progress();
    const QString diagnosis =
        progress.diagnostic.isEmpty() ? QString() : QStringLiteral(" · %1").arg(progress.diagnostic);
    statusLabel_->setText(
        QStringLiteral("%1 · %2 Zustände%3%4").arg(progress.status).arg(progress.states).arg(diagnosis)
            .arg(progress.testDriveStatus.isEmpty() ? QString()
                : QStringLiteral("\n") + progress.testDriveStatus));
    statusLabel_->setToolTip(progress.diagnostic);
    const bool completed = progress.status == QStringLiteral("abgeschlossen")
        || (progress.status == QStringLiteral("abgebrochen") && progress.diagnostic.isEmpty());
    statusLabel_->setStyleSheet(
        completed ? QStringLiteral("color:#b8e7c3;") : QStringLiteral("color:#ffbf69;"));
    displayState(stateSlider_->value(), true);
    if (progress.status == QStringLiteral("fehlgeschlagen")) {
        showError(QStringLiteral("Core-Simulation"), progress.diagnostic);
    }
}

void MainWindow::updateSliderRange(qsizetype count)
{
    const auto last = std::min<qsizetype>(
        std::max<qsizetype>(0, count - 1), std::numeric_limits<int>::max());
    programmaticSlider_ = true;
    stateSlider_->setRange(0, static_cast<int>(last));
    programmaticSlider_ = false;
    stateSlider_->setVisible(count > 1 || live_ != nullptr);
    stateLabel_->setVisible(count > 1 || live_ != nullptr);
}

qsizetype MainWindow::stateCount() const
{
    return live_ ? live_->progress().states : states_.size();
}

QJsonObject MainWindow::stateAt(qsizetype index, QString* error) const
{
    if (live_) {
        return live_->stateJson(index, error);
    }
    if (index < 0 || index >= states_.size()) {
        if (error != nullptr) {
            *error = QStringLiteral("Zustand %1 ist nicht vorhanden.").arg(index);
        }
        return {};
    }
    return states_.at(index);
}

void MainWindow::displaySimulationResult(const SimulationResult& result)
{
    runtimeInfo_ = result.runtimeAvailable
        ? QStringLiteral("Rechenzeit: %1").arg(runtimeText(result.runtimeSeconds))
        : QStringLiteral("Rechenzeit: nicht verfügbar");
    if (!result.error.isEmpty()) {
        statusLabel_->setText(QStringLiteral("Core-Lauf fehlgeschlagen"));
        statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
        timeLabel_->setText(result.runtimeAvailable
                ? QStringLiteral("Reale Core-Gesamtlaufzeit: %1").arg(runtimeText(result.runtimeSeconds))
                : QStringLiteral("Reale Core-Gesamtlaufzeit: nicht verfügbar"));
        showError(QStringLiteral("Core-Simulation"), result.error);
        return;
    }

    states_ = result.states;
    updateSliderRange(states_.size());
    const QString diagnosis =
        result.diagnostic.isEmpty() ? QString() : QStringLiteral(" · %1").arg(result.diagnostic);
    statusLabel_->setText(
        QStringLiteral("%1 · %2 Zustände%3").arg(result.status).arg(states_.size()).arg(diagnosis));
    statusLabel_->setStyleSheet(
        result.completed ? QStringLiteral("color:#b8e7c3;") : QStringLiteral("color:#ffbf69;"));
    if (!states_.isEmpty()) {
        displayState(0, true);
        scene_->frameStates(states_);
    }
    if (!result.completed) {
        QString message = result.diagnostic.isEmpty() ? result.status
            : QStringLiteral("%1\n%2").arg(result.status, result.diagnostic);
        const auto stationary = states_.isEmpty() ? QJsonObject{}
            : states_.constLast().value(QStringLiteral("stationaer")).toObject();
        if (!stationary.isEmpty()) {
            const auto components = [&stationary](const QString& key) {
                QStringList values;
                for (const auto& value : stationary.value(key).toArray())
                    values << QString::number(value.toDouble(), 'g', 6);
                return values.join(QStringLiteral("; "));
            };
            message += QStringLiteral(
                "\n\nNur Näherungszustand, kein bestätigtes Gleichgewicht."
                "\nRestkraft Welt (Bewegungsresiduum X; Y; Z): [%1] N"
                "\nRestmoment Modell (Bewegungsresiduum X; Y; Z): [%2] Nm"
                "\nToleranzen je Komponente: %3 N / %4 Nm."
                "\nEine nahezu unveränderte Fahrt allein genügt nicht."
                "\nFremdkraft und Ruder-/Schwertstellungen sind während des Laufs änderbar.")
                .arg(components(QStringLiteral("residuum_kraft_welt_n")),
                    components(QStringLiteral("residuum_moment_modell_nm")))
                .arg(stationary.value(QStringLiteral("krafttoleranz_n")).toDouble(), 0, 'g', 4)
                .arg(stationary.value(QStringLiteral("momenttoleranz_nm")).toDouble(), 0, 'g', 4);
        }
        showError(QStringLiteral("Simulation nicht abgeschlossen"),
            message);
    }
}

void MainWindow::displayState(qsizetype index, bool force)
{
    const auto count = stateCount();
    if (index < 0 || index >= count) {
        return;
    }
    const bool isLive = live_ && simulationThread_ != nullptr;
    const QString suffix = isLive
        ? (followLive_ ? QStringLiteral("  · live") : QStringLiteral("  · Rückblick"))
        : QString();
    stateLabel_->setText(QStringLiteral("Zustand %1 / %2%3").arg(index + 1).arg(count).arg(suffix));
    if (live_) desiredStateIndex_ = index;
    if (index == displayedIndex_ && !force) {
        return;
    }
    if (live_) {
        desiredStateIndex_ = index;
        if (pendingPresentation_ && (!isLive || !scene_->updatesActive())) {
            auto present = std::move(pendingPresentation_);
            pendingPresentation_ = {};
            awaitingPresentation_ = false;
            present();
            return;
        }
        if (stateFetchBusy_ || (isLive && pendingPresentation_)) return;
        const auto live = live_;
        const int generation = simulationGeneration_;
        auto state = std::make_shared<QJsonObject>();
        auto error = std::make_shared<QString>();
        auto timings = std::make_shared<LiveSimulation::JsonTimings>();
        auto fetchedIndex = std::make_shared<qsizetype>(index);
        const bool fetchLatest = followLive_ && simulationThread_ != nullptr;
        const auto now = std::chrono::steady_clock::now();
        auto readyTime = std::make_shared<std::chrono::steady_clock::time_point>();
        const auto due = fetchLatest ? std::max(now, nextStateFetch_) : now;
        if (fetchLatest) nextStateFetch_ = due + std::chrono::milliseconds(24);
        const auto motionBegin = scene_->motionStateCount();
        const auto prepareMotion = scene_->motionInputPreparer();
        const auto prepareWaterlines = scene_->waterlinePreparer();
        auto waterlines = std::make_shared<std::shared_ptr<const PreparedWaterlines>>();
        auto motion = std::make_shared<QVector<watermotion::Input>>();
        auto preparedForces = std::make_shared<forcedisplay::ForceState>();
        const bool fetchMotion = scene_->updatesActive();
        auto work = [live, state, error, timings, fetchedIndex, fetchLatest, due,
                                         motionBegin, prepareMotion, motion, fetchMotion, preparedForces,
                                         now, readyTime, prepareWaterlines, waterlines] {
            waitForDisplayDeadline(due);
            if (fetchLatest) *fetchedIndex = std::max<qsizetype>(0, live->progress().states - 1);
            *state = live->stateJson(*fetchedIndex, error.get(), timings.get());
            if (error->isEmpty()) {
                try {
                    const auto begin = std::chrono::steady_clock::now();
                    *preparedForces = forcedisplay::parseForceState(*state);
                    timings->forceParseMs = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - begin).count();
                } catch (const std::exception& exception) {
                    *error = QString::fromUtf8(exception.what());
                }
            }
            if (fetchMotion && error->isEmpty() && motionBegin <= *fetchedIndex) {
                try {
                    const auto begin = std::chrono::steady_clock::now();
                    const auto rawMotion = live->motionStates(motionBegin, *fetchedIndex + 1, error.get());
                    const auto readEnd = std::chrono::steady_clock::now();
                    timings->motionReadMs = std::chrono::duration<double, std::milli>(readEnd - begin).count();
                    timings->motionStates = rawMotion.size();
                    if (error->isEmpty()) *motion = prepareMotion(rawMotion);
                    timings->motionPrepareMs = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - readEnd).count();
                    timings->motionMs = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - begin).count();
                } catch (const std::exception& exception) {
                    *error = QString::fromUtf8(exception.what());
                }
            }
            if (fetchMotion && error->isEmpty()) {
                try {
                    *waterlines = prepareWaterlines(*state);
                } catch (const std::exception& exception) {
                    *error = QString::fromUtf8(exception.what());
                }
            }
            *readyTime = std::chrono::steady_clock::now();
            timings->fetchMs = std::chrono::duration<double, std::milli>(*readyTime - now).count();
        };
        auto onReady = [this, live, generation, state, error, timings,
                                                  fetchedIndex, motionBegin, motion, preparedForces, readyTime,
                                                  waterlines] {
            stateFetchBusy_ = false;
            if (shuttingDown_.load() || generation != simulationGeneration_ || live_ != live) return;
            auto present = [this, live, generation, state, error, timings, fetchedIndex,
                               motionBegin, motion, preparedForces, readyTime, waterlines] {
                if (shuttingDown_.load() || generation != simulationGeneration_ || live_ != live) return;
                timings->guiWaitMs = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - *readyTime).count();
                lastFetchTimings_ = *timings;
                const auto readyIndex = *fetchedIndex;
                if ((followLive_ || desiredStateIndex_ == readyIndex) && error->isEmpty()
                    && scene_->motionStateCount() >= motionBegin
                    && scene_->motionStateCount() <= readyIndex) {
                    const auto skip = scene_->motionStateCount() - motionBegin;
                    scene_->appendMotionInputs(motion->mid(skip), error.get());
                }
                if (followLive_ && simulationThread_ != nullptr) {
                    // Prefetch while the GUI renders this complete immutable snapshot.
                    desiredStateIndex_ = std::max(readyIndex, live->progress().states - 1);
                    displayState(desiredStateIndex_, true);
                    if (readyIndex > displayedIndex_)
                        displayFetchedState(readyIndex, *state, *error, preparedForces.get(), waterlines->get());
                } else if (desiredStateIndex_ == readyIndex)
                    displayFetchedState(readyIndex, *state, *error, preparedForces.get(), waterlines->get());
                else if (desiredStateIndex_ >= 0) displayState(desiredStateIndex_, true);
            };
            if (followLive_ && simulationThread_ && scene_->updatesActive() && awaitingPresentation_)
                pendingPresentation_ = std::move(present);
            else present();
        };
        if (!stateFetchThread_) {
            stateFetchThread_ = new QThread(this);
            stateReader_ = new QObject;
            stateReader_->moveToThread(stateFetchThread_);
            connect(stateFetchThread_, &QThread::finished, stateReader_, &QObject::deleteLater);
            stateFetchThread_->start(QThread::LowPriority);
        }
        stateFetchBusy_ = true;
        QMetaObject::invokeMethod(stateReader_,
            [this, work = std::move(work), onReady = std::move(onReady)]() mutable {
                work();
                QMetaObject::invokeMethod(this, std::move(onReady), Qt::QueuedConnection);
            }, Qt::QueuedConnection);
        return;
    }
    QString error;
    const auto state = stateAt(index, &error);
    displayFetchedState(index, state, error);
}

void MainWindow::displayFetchedState(qsizetype index, const QJsonObject& state, QString error,
    const forcedisplay::ForceState* preparedForces, const PreparedWaterlines* waterlines)
{
    QElapsedTimer profileTimer;
    profileTimer.start();
    const auto renderTotals = [this] {
        std::array<double, 6> totals{scene_->workCounters().renderMs, scene_->workCounters().waterCutsMs,
            scene_->workCounters().overlaysMs, scene_->workCounters().environmentMs,
            scene_->workCounters().opaqueMs, scene_->workCounters().translucentMs};
        for (auto* view : staticScenes_) {
            totals[0] += view->workCounters().renderMs;
            totals[1] += view->workCounters().waterCutsMs;
            totals[2] += view->workCounters().overlaysMs;
            totals[3] += view->workCounters().environmentMs;
            totals[4] += view->workCounters().opaqueMs;
            totals[5] += view->workCounters().translucentMs;
        }
        return totals;
    };
    const auto beforeRender = renderTotals();
    const double mainRenderBefore = scene_->workCounters().renderMs;
    const auto layersBefore = scene_->workCounters();
    forcedisplay::ForceState forces;
    try {
        if (state.isEmpty()) {
            throw std::runtime_error(error.toStdString());
        }
        forces = preparedForces ? *preparedForces : forcedisplay::parseForceState(state);
    } catch (const std::exception& exception) {
        error = QString::fromUtf8(exception.what());
    }
    const double parseMs = profileTimer.nsecsElapsed() / 1e6;
    if (error.isEmpty() && forces.aero && !forces.aero->sails.isEmpty()) {
        const auto opened = modelCache_.value(cacheKey(modelId_, modelVersion_));
        if (!opened || !opened->sailConfiguration
            || opened->sailConfiguration->modell_fassung != validation_.fingerprint.toStdString()) {
            error = QStringLiteral("Bestätigte Segelgeometrie zur Modellfassung fehlt.");
        } else {
            for (const auto& sail : forces.aero->sails) {
                const QString version = sail.sourceVersion + QLatin1Char(':')
                    + QString::number(sail.angle, 'g', 17) + QLatin1Char(':')
                    + QString::number(sail.mirrored);
                if (renderedSailVersions_.value(sail.elementId) == version) continue;
                try {
                    const auto key = validation_.fingerprint + QLatin1Char(':')
                        + sail.elementId + QLatin1Char(':') + version;
                    auto* parts = confirmedSailGeometry_.object(key);
                    if (!parts) {
                        const auto groupId = opened->data.elementGroups.value(sail.elementId);
                        if (!opened->data.inspection || groupId.isEmpty()) {
                            error = QStringLiteral("Bestätigte Segelstellung kann nicht dargestellt werden: %1").arg(sail.elementId);
                            break;
                        }
                        const auto groupPreview = segelphysik::getGroupMovementPreview(
                            *opened->data.inspection, groupId.toStdString());
                        if (!groupPreview) {
                            renderedSailVersions_.insert(sail.elementId, version);
                            continue;
                        }
                        const auto requestedValue = groupPreview->reference_value + sail.angle;
                        const auto posedGroup = segelphysik::getGroupMovementPreview(
                            *opened->data.inspection, groupId.toStdString(), requestedValue);
                        if (!posedGroup) {
                            error = QStringLiteral("JSON-Gruppenstellung kann nicht dargestellt werden: %1").arg(groupId);
                            break;
                        }
                        const auto requestedPose = std::find_if(posedGroup->poses.begin(),
                            posedGroup->poses.end(),
                            [](const auto& pose) { return pose.label == "requested"; });
                        if (requestedPose == posedGroup->poses.end()) {
                            error = QStringLiteral("JSON-Gruppenstellung kann nicht dargestellt werden: %1").arg(groupId);
                            break;
                        }
                        parts = new QVector<DisplayPart>(toDisplayParts(requestedPose->meshes));
                        confirmedSailGeometry_.insert(key, parts);
                    }
                    QSet<QString> posedElements;
                    for (const auto& part : *parts) posedElements.insert(part.elementId);
                    for (const auto& elementId : posedElements) {
                        if (!scene_->setSailGeometry(elementId, *parts, &error)) break;
                        for (auto* view : staticScenes_)
                            if (!view->setSailGeometry(elementId, *parts, &error)) break;
                        if (!error.isEmpty()) break;
                    }
                } catch (const std::exception& exception) {
                    error = QString::fromUtf8(exception.what());
                }
                if (!error.isEmpty()) break;
                renderedSailVersions_.insert(sail.elementId, version);
            }
        }
    }
    const double geometryMs = profileTimer.nsecsElapsed() / 1e6 - parseMs;
    if (error.isEmpty() && scene_->updatesActive()) {
        QString motionError;
        if (scene_->motionStateCount() > index + 1) scene_->resetMotionHistory();
        for (auto begin = scene_->motionStateCount(); begin <= index && motionError.isEmpty();) {
            const auto end = std::min(index + 1, begin + 256);
            QVector<QJsonObject> motion;
            if (live_) {
                motion = live_->motionStates(begin, end, &motionError);
            } else {
                for (qsizetype i = begin; i < end; ++i) motion.push_back(states_[i]);
            }
            if (motionError.isEmpty() && !scene_->appendMotionStates(motion, &motionError) && motionError.isEmpty()) {
                motionError = QStringLiteral("Fahrthistorie konnte nicht aufgebaut werden.");
            }
            begin = end;
        }
        if (!motionError.isEmpty()) scene_->invalidateMotionHistory(index + 1, motionError);
    }
    const double sceneBeginMs = profileTimer.nsecsElapsed() / 1e6;
    if (error.isEmpty() && scene_->updatesActive()) awaitingPresentation_ = true;
    bool rendered = error.isEmpty()
        && (!scene_->updatesActive() || scene_->setState(state, forces, &error, waterlines));
    if (rendered && workspaceIndex_ > 0)
        for (auto* view : staticScenes_)
            if (view->updatesActive() && !view->setState(state, forces, &error)) {
                rendered = false;
                break;
            }
    if (rendered && !scene_->updatesActive() && navigationCheck_->isChecked()) {
        try {
            scene_->updateDockedNavigation(state, forces);
        } catch (const std::exception& exception) {
            rendered = false;
            error = QString::fromUtf8(exception.what());
        }
    }
    const double sceneEndMs = profileTimer.nsecsElapsed() / 1e6;
    if (!rendered) {
        renderedSailVersions_.clear();
        scene_->clearState();
        for (auto* view : staticScenes_) view->clearState();
        cancellation_.store(true);
        statusLabel_->setText(QStringLiteral("Darstellung des Zustands fehlgeschlagen"));
        showError(QStringLiteral("Core-Zustand"), error);
        return;
    }
    displayedIndex_ = index;
    applyGraphicsProfile();
    currentForces_ = forces;
    hasForces_ = true;
    updateStaticForceMenu();
    pressureStatus_->setText(QStringLiteral("LDP: %1\nSDP (Ersatz): %2")
        .arg(forces.lateral ? (forces.lateral->evaluationVersion == 2
                ? forces.lateral->status : QStringLiteral("Altstand, nicht korrigiert"))
            : QStringLiteral("nicht verfügbar"),
            forces.sailCenter ? forces.sailCenter->status : QStringLiteral("keine Fremdkraft geliefert")));
    pressureStatus_->setToolTip(QStringLiteral(
        "LDP: äquivalenter Angriffspunkt der Seitenkraft, darf bei Gegenkräften außerhalb liegen. "
        "SDP (Ersatz): eingefrorener Ansatzpunkt der Fremdkraft, keine berechnete Segelphysik. "
        "Bei Näherungszuständen sind auch die Druckpunkte nur Näherungswerte."));
    if (forces.aero) {
        pressureStatus_->setText(pressureStatus_->text() + QStringLiteral("\nSDP gesamt: %1")
            .arg(forces.aero->center.status));
        for (const auto& sail : forces.aero->sails) {
            pressureStatus_->setText(pressureStatus_->text() + QStringLiteral("\nSDP %1: %2")
                .arg(sail.elementId, sail.center.status));
            const auto pending = sailPending_.constFind(sail.elementId);
            if (followLive_ && pending != sailPending_.cend() && *pending == sail.angle
                && static_cast<quint64>(forces.aero->revision) > sailPendingRevision_.value(sail.elementId)) {
                sailPending_.remove(sail.elementId);
                sailPendingRevision_.remove(sail.elementId);
                if (sailPending_.isEmpty())
                    sailStatus_->setText(QStringLiteral("Segelstellung im angezeigten Core-Zustand bestätigt."));
            }
        }
        pressureStatus_->setToolTip(pressureStatus_->toolTip() + QStringLiteral(
            "\nAerodynamischer SDP: projizierter Seitenkraft-Ersatzpunkt; kein Angriffspunkt der gesamten Segellast."));
    } else pressureStatus_->setText(pressureStatus_->text() + QStringLiteral("\nSegel-SDP: nicht verfügbar (Altstand oder keine Zuordnung)"));
    updateMovementControls(state);
    updateValues();
    const auto time = state.value(QStringLiteral("zeit_s")).toDouble();
    timeLabel_->setText(
        QStringLiteral("Simulationszeit: %1 s  ·  %2").arg(time, 0, 'f', 3).arg(runtimeInfo_));
    if (performanceDiagnostics_) {
        const auto afterRender = renderTotals();
        performanceSamples_.append(QJsonObject{
            {QStringLiteral("index"), static_cast<double>(index)},
            {QStringLiteral("core_json_ms"), lastFetchTimings_.coreMs},
            {QStringLiteral("snapshot_cache_treffer"), lastFetchTimings_.snapshotHit},
            {QStringLiteral("snapshot_capture_ms"), lastFetchTimings_.snapshotCaptureMs},
            {QStringLiteral("fetch_ms"), lastFetchTimings_.fetchMs},
            {QStringLiteral("gui_wait_ms"), lastFetchTimings_.guiWaitMs},
            {QStringLiteral("follow_live"), followLive_},
            {QStringLiteral("json_decode_ms"), lastFetchTimings_.decodeMs},
            {QStringLiteral("worker_motion_ms"), lastFetchTimings_.motionMs},
            {QStringLiteral("worker_motion_read_ms"), lastFetchTimings_.motionReadMs},
            {QStringLiteral("worker_motion_prepare_ms"), lastFetchTimings_.motionPrepareMs},
            {QStringLiteral("worker_motion_zustaende"), static_cast<double>(lastFetchTimings_.motionStates)},
            {QStringLiteral("worker_force_parse_ms"), lastFetchTimings_.forceParseMs},
            {QStringLiteral("json_bytes"), static_cast<double>(lastFetchTimings_.bytes)},
            {QStringLiteral("force_parse_ms"), parseMs},
            {QStringLiteral("sail_geometry_ms"), geometryMs},
            {QStringLiteral("motion_ms"), sceneBeginMs - parseMs - geometryMs},
            {QStringLiteral("scene_ms"), sceneEndMs - sceneBeginMs},
            {QStringLiteral("render_ms"), afterRender[0] - beforeRender[0]},
            {QStringLiteral("main_render_ms"), scene_->workCounters().renderMs - mainRenderBefore},
            {QStringLiteral("main_layer_ms"), scene_->workCounters().mainLayerMs - layersBefore.mainLayerMs},
            {QStringLiteral("force_layer_ms"), scene_->workCounters().forceLayerMs - layersBefore.forceLayerMs},
            {QStringLiteral("axes_layer_ms"), scene_->workCounters().axesLayerMs - layersBefore.axesLayerMs},
            {QStringLiteral("water_cuts_ms"), afterRender[1] - beforeRender[1]},
            {QStringLiteral("overlays_ms"), afterRender[2] - beforeRender[2]},
            {QStringLiteral("environment_ms"), afterRender[3] - beforeRender[3]},
            {QStringLiteral("opaque_ms"), afterRender[4] - beforeRender[4]},
            {QStringLiteral("translucent_ms"), afterRender[5] - beforeRender[5]},
            {QStringLiteral("ui_ms"), profileTimer.nsecsElapsed() / 1e6 - sceneEndMs}});
    }
}

void MainWindow::setBusy(bool busy)
{
    busy = busy || sailSaveInProgress_;
    const bool hasModel = validation_.valid && !modelPath_.isEmpty();
    const bool dynamic = modeCombo_->currentData().toBool();
    const bool running = simulationThread_ != nullptr;
    airSettingsAction_->setEnabled(!busy && !running);
    const bool editing = inspection_->manualModeActive();
    testDrivePanel_->hide();
    cameraFramePanel_->setVisible(dynamic);
    stationaryForcePanel_->setVisible(true);
    stationaryForcePanel_->setEnabled(hasModel && (!busy || (dynamic && running)));
    windPanel_->setEnabled(hasModel);
    windDirectionEdit_->setEnabled(!busy || (dynamic && running));
    windSpeedEdit_->setEnabled(!busy || (dynamic && running));
    windHeightEdit_->setEnabled(!busy);
    windMode_->setEnabled(dynamic && (!busy || running));
    for (auto* active : sailActive_) active->setEnabled(hasModel && !busy);
    for (auto* angle : sailAngles_) angle->setEnabled(hasModel && (!busy || (dynamic && running)));
    movementPanel_->setVisible(!movementSliders_.isEmpty());
    for (auto* fixed : stationaryMovements_) {
        movementLayout_->setRowVisible(fixed, true);
        fixed->setEnabled(hasModel && (!busy || (dynamic && running))
            && fixed->property("coreMovementAvailable").toBool());
    }
    for (auto* slider : movementSliders_)
        movementLayout_->setRowVisible(slider->parentWidget(), false);
    for (auto* slider : movementSliders_)
        slider->setEnabled(false);
    testDriveCheck_->setEnabled(hasModel && dynamic && (!busy || running));
    testForceSpin_->setEnabled(!busy && dynamic);
    testSpeedSpin_->setEnabled(!busy && dynamic);
    chooseFolderButton_->setEnabled(!busy && !running && !editing);
    refreshModelsButton_->setEnabled(!busy && !running && !editing && fingerprintThread_ == nullptr);
    modelList_->setEnabled(!busy && !running && !editing);
    modeCombo_->setEnabled(!busy && hasModel);
    runButton_->setEnabled(!busy && hasModel && !editing);
    cancelButton_->setEnabled(running);
    durationSpin_->setEnabled(!busy && dynamic);
    stepSpin_->setEnabled(!busy && dynamic);
    realtimeCheck_->setEnabled(!busy && dynamic);
}

void MainWindow::updateModeControls()
{
    parameterForm_->setRowVisible(durationSpin_, false);
    parameterForm_->setRowVisible(stepSpin_, false);
    parameterForm_->setRowVisible(realtimeCheck_, false);
    if (!modeCombo_->currentData().toBool()) {
        const QSignalBlocker blocker(windMode_);
        windMode_->setCurrentIndex(1);
    }
    setBusy(simulationThread_ != nullptr || openThread_ != nullptr);
}

void MainWindow::clearValues()
{
    staticValues_->clear();
    pressureStatus_->setText(QStringLiteral("Druckpunkte: kein Ergebnis"));
    const QSignalBlocker blocker(valuesTree_);
    valuesTree_->clear();
    rootItem_ = nullptr;
    stationaryItem_ = nullptr;
    courseRateItem_ = nullptr;
    groupItems_.clear();
    contributionItems_.clear();
    currentForces_ = {};
    hasForces_ = false;
    updateStaticForceMenu();
    problems_.clear();
    valuesTree_->headerItem()->setText(ColName, QStringLiteral("Beitrag"));
    balanceLabel_->setText(QStringLiteral("Keine Daten – Core-Lauf starten."));
    balanceLabel_->setStyleSheet(QString());
    stepInfoLabel_->setText(QStringLiteral("Zeit: –  ·  Schritt: –"));
    detailLabel_->setText(QStringLiteral("Beitrag in Tabelle oder 3D wählen."));
}

// Ohne gültiges Modell gibt es keine Kraftrechnung; das Wertefenster zeigt
// stattdessen die Modellfehler mit Fundstelle und Abhilfe.
void MainWindow::showModelProblems(const QVector<ModelProblem>& problems)
{
    clearValues();
    problems_ = problems;
    if (problems_.isEmpty()) {
        problems_.push_back({{}, QStringLiteral("Der Core hat das Modell abgelehnt, aber keine "
                                                "Einzelmeldung geliefert."), {}, {}, {}});
    }
    const QSignalBlocker blocker(valuesTree_);
    valuesTree_->headerItem()->setText(ColName, QStringLiteral("Modellfehler"));
    auto* root = new QTreeWidgetItem(valuesTree_);
    root->setText(ColName, QStringLiteral("%1 – %2 Fehler").arg(modelName_).arg(problems_.size()));
    root->setFlags(Qt::ItemIsEnabled);
    root->setFirstColumnSpanned(true);
    for (qsizetype i = 0; i < problems_.size(); ++i) {
        const auto& p = problems_.at(i);
        auto* item = new QTreeWidgetItem(root);
        const QString title = p.summary.isEmpty() ? p.message : p.summary;
        item->setText(ColName, p.code.isEmpty() ? title : QStringLiteral("%1 · %2").arg(p.code, title));
        item->setForeground(ColName, QColor(QStringLiteral("#ffbf69")));
        item->setData(ColName, Qt::UserRole + 1, static_cast<int>(i));
        item->setFirstColumnSpanned(true);
        const auto addLine = [item, i](const QString& label, const QString& text) {
            if (text.isEmpty()) {
                return;
            }
            auto* line = new QTreeWidgetItem(item);
            line->setText(ColName, QStringLiteral("%1: %2").arg(label, text));
            line->setToolTip(ColName, text);
            line->setData(ColName, Qt::UserRole + 1, static_cast<int>(i));
            line->setFirstColumnSpanned(true);
        };
        addLine(QStringLiteral("Fundstelle"), p.location);
        if (!p.summary.isEmpty()) {
            addLine(QStringLiteral("Meldung"), p.message);
        }
        addLine(QStringLiteral("Abhilfe"), p.remedy);
        item->setExpanded(true);
    }
    root->setExpanded(true);
    balanceLabel_->setText(QStringLiteral(
        "<b>Modell fehlerhaft</b><br>Keine Kraftrechnung möglich.<br>%1 Fehler vom Core gemeldet."
        "<br>Nach dem Korrigieren der Modelldateien lädt die GUI das Modell automatisch neu.")
            .arg(problems_.size()));
    balanceLabel_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
    stepInfoLabel_->setText(QStringLiteral("Modell: %1  ·  Fassung %2").arg(modelId_, modelVersion_.left(12)));
    detailLabel_->setText(QStringLiteral("Fehler in der Tabelle wählen, um alle Angaben zu sehen."));
    if (valuesCheck_ != nullptr && !valuesCheck_->isChecked()) {
        valuesCheck_->setChecked(true);
    }
}

void MainWindow::filterStaticValues()
{
    for (int i = 0; i < staticValues_->topLevelItemCount(); ++i) {
        auto* row = staticValues_->topLevelItem(i);
        bool matches = true;
        for (int c = 0; c < staticValues_->columnCount(); ++c)
            matches = matches && row->text(c).contains(
                staticValueFilters_[c]->text(), Qt::CaseInsensitive);
        row->setHidden(!matches);
    }
}

void MainWindow::updateStaticValues()
{
    const QSignalBlocker blocker(staticValues_);
    QHash<QString, QTreeWidgetItem*> rows;
    for (int i = 0; i < staticValues_->topLevelItemCount(); ++i) {
        auto* row = staticValues_->topLevelItem(i);
        rows.insert(row->data(0, Qt::UserRole).toString(), row);
    }
    const auto pose = largeScene_->statePose();
    forcedisplay::Pose inverse;
    if (pose) {
        inverse.quaternionXyzw = pose->quaternionXyzw;
        for (int i = 0; i < 3; ++i) inverse.quaternionXyzw[i] *= -1.;
    }
    auto contributions = currentForces_.contributions;
    auto derived = forcedisplay::derivedContributions(currentForces_);
    const auto sums = forcedisplay::staticResultants(currentForces_,
        pose.value_or(forcedisplay::Pose{}), hydrodynamicRoles_);
    for (auto& c : derived) {
        const auto type = c.category == forcedisplay::Category::Weight
            ? forcedisplay::StaticForce::Weight : forcedisplay::StaticForce::Buoyancy;
        const auto& sum = sums[static_cast<std::size_t>(type)];
        if (c.kind == forcedisplay::Kind::Moment) {
            // moment_s_nm ist bereits auf den Modellbezug S bezogen.
            c.frame = QStringLiteral("modell");
            c.point = currentForces_.reference;
        } else if (c.total) {
            c.frame = QStringLiteral("modell");
            c.vector = pose && sum.forceWorld
                ? std::optional<forcedisplay::Vec3>(
                    forcedisplay::toWorldVector(inverse, *sum.forceWorld)) : std::nullopt;
            c.point = pose && sum.pointWorld
                ? std::optional<forcedisplay::Vec3>(forcedisplay::toWorldVector(inverse,
                    forcedisplay::sub(*sum.pointWorld, pose->position))) : std::nullopt;
        }
        if ((c.total && !sum.forceWorld) || !c.vector)
            c.state = QStringLiteral("unvollstaendig");
        contributions.push_back(c);
    }
    const auto pointText = [](const forcedisplay::Vec3& point) {
        return QStringLiteral("[%1; %2; %3] m")
            .arg(forcedisplay::formatFixed(point[0], 2),
                forcedisplay::formatFixed(point[1], 2),
                forcedisplay::formatFixed(point[2], 2));
    };
    for (const auto& c : contributions) {
        auto* row = rows.take(c.id);
        if (!row) {
            row = new StaticValueItem(staticValues_);
            row->setData(0, Qt::UserRole, c.id);
        }
        row->setText(0, c.elementId.isEmpty() ? modelName_
            : modelName_ + QStringLiteral("/") + c.elementId);
        row->setText(1, c.kind == forcedisplay::Kind::Force
            ? QStringLiteral("Kraft") : QStringLiteral("Moment"));
        row->setText(2, forcedisplay::contributionLabel(c));
        row->setToolTip(2, c.id);
        row->setForeground(2, QColor(forcedisplay::colorForContribution(c)));
        const bool knownFrame = c.frame == QStringLiteral("modell")
            || (c.frame == QStringLiteral("welt") && pose.has_value());
        const bool valid = forcedisplay::showsValues(c) && knownFrame;
        if (valid) {
            const auto vector = c.frame == QStringLiteral("welt")
                ? forcedisplay::toWorldVector(inverse, *c.vector) : *c.vector;
            const std::array<double, 4> numbers{
                forcedisplay::norm(vector), vector[0], vector[1], vector[2]};
            for (int i = 0; i < 4; ++i) {
                row->setText(i + 3, forcedisplay::formatValue(numbers[i], c.unit));
                row->setData(i + 3, staticRawValueRole, numbers[i]);
                row->setTextAlignment(i + 3, Qt::AlignRight | Qt::AlignVCenter);
            }
        } else {
            for (int i = 3; i <= 6; ++i) {
                row->setText(i, QStringLiteral("—"));
                row->setData(i, staticRawValueRole, QVariant());
            }
        }
        row->setText(7, c.unit);
        QString reference = QStringLiteral("nicht bestimmbar");
        if (c.kind == forcedisplay::Kind::Moment || c.fromSums) {
            if (currentForces_.reference)
                reference = QStringLiteral("S ") + pointText(*currentForces_.reference);
        } else if (c.point && knownFrame) {
            const auto point = c.frame == QStringLiteral("welt")
                ? forcedisplay::toWorldVector(inverse, forcedisplay::sub(*c.point, pose->position))
                : *c.point;
            reference = (c.total ? (c.category == forcedisplay::Category::Weight
                ? QStringLiteral("S ") : QStringLiteral("A ")) : QString()) + pointText(point);
        }
        row->setText(8, reference);
        row->setText(9, knownFrame ? forcedisplay::stateText(c.state)
            : QStringLiteral("Koordinatenbezug nicht verfügbar"));
        if (!c.errorId.isEmpty()) row->setToolTip(9, c.errorId);
    }
    for (auto* stale : rows) delete stale;
    filterStaticValues();
}

void MainWindow::updateValues()
{
    if (!hasForces_) {
        return;
    }
    const auto& forces = currentForces_;
    updateStaticValues();
    if (workspaceIndex_ > 0) return;
    const auto summary = forcedisplay::balanceSummary(forces);
    QString card = QStringLiteral("<b>%1</b>").arg(summary.title.toHtmlEscaped());
    if (!forces.flooded.isEmpty()) {
        card += QStringLiteral("<br><span style=\"color:#ff8a7a;\">Vollgelaufen: %1</span>")
                    .arg(forces.flooded.join(QStringLiteral(", ")).toHtmlEscaped());
    }
    for (const auto& line : summary.lines) {
        card += QStringLiteral("<br>") + line.toHtmlEscaped();
    }
    QJsonObject stationaryContext;
    if (!live_ && displayedIndex_ >= 0 && displayedIndex_ < states_.size())
        stationaryContext = states_.at(displayedIndex_).value(QStringLiteral("stationaer")).toObject();
    if (!stationaryContext.isEmpty()) {
        card += QStringLiteral("<br><b>Stationäres Fahrtgleichgewicht</b><br>Core: %1")
                    .arg(stationaryContext.value(QStringLiteral("status")).toString().toHtmlEscaped());
        if (!stationaryContext.value(QStringLiteral("konvergiert")).toBool())
            card += QStringLiteral("<br><b>Fahrt und Lage sind nur Näherungswerte.</b>");
        if (stationaryContext.value(QStringLiteral("referenzkurs_verwendet")).toBool())
            card += QStringLiteral("<br>Absoluter Kurs unbestimmt: Startkurs ist Referenz.");
        if (stationaryContext.contains(QStringLiteral("loesungstyp")))
            card += QStringLiteral("<br>Lösungstyp: %1")
                .arg(stationaryContext.value(QStringLiteral("loesungstyp")).toString().toHtmlEscaped());
        const auto radius = stationaryContext.value(QStringLiteral("radius_m"));
        if (radius.isDouble() && std::isfinite(radius.toDouble()))
            card += QStringLiteral("<br>Kreisradius relativ zum Wasser: %1 m")
                .arg(radius.toDouble(), 0, 'g', 8);
        else if (stationaryContext.contains(QStringLiteral("radius_status")))
            card += QStringLiteral("<br>Kreisradius relativ zum Wasser: %1")
                .arg(stationaryContext.value(QStringLiteral("radius_status")).toString().toHtmlEscaped());
        card += QStringLiteral("<br>Keine Aussage über dynamische Stabilität oder Eindeutigkeit.");
    }
    balanceLabel_->setText(card);
    const bool stationaryConverged = stationaryContext.isEmpty()
        || stationaryContext.value(QStringLiteral("konvergiert")).toBool();
    const auto balanceStyle = summary.complete && stationaryConverged
        ? QStringLiteral("color:#b8e7c3;") : QStringLiteral("color:#ffd8a0;");
    if (balanceLabel_->styleSheet() != balanceStyle) balanceLabel_->setStyleSheet(balanceStyle);
    stepInfoLabel_->setText(stationaryContext.isEmpty()
        ? QStringLiteral("Zeit: %1 s  ·  Schritt: %2  ·  Vertrag V%3")
            .arg(forcedisplay::formatFixed(forces.time, 2))
            .arg(forces.step)
            .arg(forces.contract)
        : QStringLiteral("Stationär · %1 Iterationen · Vertrag V%2")
            .arg(stationaryContext.value(QStringLiteral("iterationen")).toInt())
            .arg(forces.contract));

    const QSignalBlocker blocker(valuesTree_);
    const bool valuesUpdates = valuesTree_->updatesEnabled();
    valuesTree_->setUpdatesEnabled(false);
    if (rootItem_ == nullptr) {
        rootItem_ = new QTreeWidgetItem(valuesTree_);
        rootItem_->setExpanded(true);
        rootItem_->setFlags(Qt::ItemIsEnabled);
    }
    rootItem_->setText(ColName, modelName_.isEmpty() ? forces.modelId : modelName_);
    if (!courseRateItem_)
        courseRateItem_ = new QTreeWidgetItem(rootItem_, {QStringLiteral("Kursdrehrate (Core)")});
    courseRateItem_->setText(ColMagnitude, forces.courseRateRadS
        ? QStringLiteral("%1 rad/s").arg(*forces.courseRateRadS, 0, 'g', 17)
        : QStringLiteral("nicht verfügbar"));
    courseRateItem_->setText(ColState, forces.courseRateStatus);
    courseRateItem_->setToolTip(ColMagnitude, QStringLiteral(
        "Signierte Kursdrehrate um Welt-Z, nicht Modell-Z oder Betrag von omega."));
    QSet<QString> seen;
    for (const auto& c : forces.contributions) {
        const QString groupName = forcedisplay::groupLabel(c);
        auto* group = groupItems_.value(groupName);
        if (group == nullptr) {
            group = new QTreeWidgetItem(rootItem_);
            group->setText(ColName, groupName);
            group->setFlags(Qt::ItemIsEnabled);
            group->setExpanded(true);
            groupItems_.insert(groupName, group);
        }
        auto* item = contributionItems_.value(c.id);
        if (item == nullptr || item->parent() != group) {
            delete item;
            item = new QTreeWidgetItem(group);
            item->setData(ColName, Qt::UserRole, c.id);
            contributionItems_.insert(c.id, item);
        }
        seen.insert(c.id);
        item->setText(ColName, forcedisplay::contributionLabel(c));
        item->setForeground(ColName, QColor(forcedisplay::colorFor(c.category, c.group)));
        if (forcedisplay::showsValues(c)) {
            const auto& v = *c.vector;
            item->setText(ColMagnitude, forcedisplay::formatQuantity(forcedisplay::norm(v), c.unit));
            item->setText(ColX, forcedisplay::formatQuantity(v[0], c.unit));
            item->setText(ColY, forcedisplay::formatQuantity(v[1], c.unit));
            item->setText(ColZ, forcedisplay::formatQuantity(v[2], c.unit));
        } else {
            // Zustände erscheinen als Text, nie als Leerzelle oder 0 (A-G-00017).
            const QString text = forcedisplay::stateText(c.state);
            for (int column : {ColMagnitude, ColX, ColY, ColZ}) {
                item->setText(column, text);
            }
        }
        item->setText(ColState, forcedisplay::stateText(c.state));
        item->setText(ColScale, scaleText(scene_->scaleInfo(c.id)));
        for (int column = ColMagnitude; column < ColumnCount; ++column) {
            item->setTextAlignment(column, Qt::AlignRight | Qt::AlignVCenter);
        }
    }
    for (auto it = contributionItems_.begin(); it != contributionItems_.end();) {
        if (!seen.contains(it.key())) {
            if (valuesTree_->currentItem() == it.value() || it.value()->isSelected()) {
                valuesTree_->setCurrentItem(nullptr);
                scene_->setSelected({});
            }
            delete it.value();
            it = contributionItems_.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = groupItems_.begin(); it != groupItems_.end();) {
        if (it.value()->childCount() == 0) {
            delete it.value();
            it = groupItems_.erase(it);
        } else {
            ++it;
        }
    }
    delete stationaryItem_;
    stationaryItem_ = nullptr;
    if (!live_ && displayedIndex_ >= 0 && displayedIndex_ < states_.size()) {
        const auto state = states_.at(displayedIndex_);
        const auto stationary = state.value(QStringLiteral("stationaer"));
        if (stationary.isObject()) {
            stationaryItem_ = new QTreeWidgetItem(rootItem_,
                {QStringLiteral("Stationäre Auswertung (Core)")});
            stationaryItem_->setExpanded(true);
            const auto append = [](const auto& self, QTreeWidgetItem* parent,
                                   const QString& name, const QJsonValue& value) -> void {
                auto* item = new QTreeWidgetItem(parent, {name});
                if (value.isObject()) {
                    item->setExpanded(true);
                    const auto object = value.toObject();
                    for (auto it = object.begin(); it != object.end(); ++it)
                        self(self,item,it.key(),it.value());
                } else {
                    auto encoded = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
                    encoded = encoded.mid(1,encoded.size()-2);
                    item->setText(ColMagnitude,QString::fromUtf8(encoded));
                    item->setToolTip(ColMagnitude,QString::fromUtf8(encoded));
                }
            };
            const auto object = stationary.toObject();
            for (auto it = object.begin(); it != object.end(); ++it)
                append(append, stationaryItem_, it.key(), it.value());
            append(append, stationaryItem_, QStringLiteral("Position [m]; X/Y Referenz"),
                state.value(QStringLiteral("position_m")));
            append(append, stationaryItem_, QStringLiteral("Orientierung Quaternion XYZW"),
                state.value(QStringLiteral("orientierung_xyzw")));
            append(append, stationaryItem_, QStringLiteral("Geschwindigkeit Welt [m/s]"),
                state.value(QStringLiteral("geschwindigkeit_welt_m_s")));
            const auto orientation = navigation::derive(forcedisplay::readPose(state), {1.,0.,0.});
            const auto angle = [](const std::optional<double>& value) {
                return value ? QJsonValue(*value) : QJsonValue(QJsonValue::Null);
            };
            append(append, stationaryItem_, QStringLiteral("Rollen [Grad]"), angle(orientation.roll));
            append(append, stationaryItem_, QStringLiteral("Stampfen [Grad]"), angle(orientation.pitch));
            append(append, stationaryItem_, QStringLiteral("Kurs / Gierwinkel [Grad]; Referenzstatus siehe Core"),
                angle(orientation.heading));
        }
    }
    valuesTree_->setUpdatesEnabled(valuesUpdates);
    updateDetail();
}

void MainWindow::updateDetail()
{
    if (!problems_.isEmpty()) {
        const auto items = valuesTree_->selectedItems();
        const QVariant index = items.isEmpty() ? QVariant() : items.front()->data(ColName, Qt::UserRole + 1);
        if (!index.isValid() || index.toInt() < 0 || index.toInt() >= problems_.size()) {
            detailLabel_->setText(QStringLiteral("Fehler in der Tabelle wählen, um alle Angaben zu sehen."));
            return;
        }
        const auto& p = problems_.at(index.toInt());
        QStringList lines;
        lines << QStringLiteral("<b>%1</b> %2").arg(p.code.toHtmlEscaped(), p.summary.toHtmlEscaped());
        if (!p.location.isEmpty()) {
            lines << QStringLiteral("Fundstelle: %1").arg(p.location.toHtmlEscaped());
        }
        lines << QStringLiteral("Meldung: %1").arg(p.message.toHtmlEscaped());
        if (!p.remedy.isEmpty()) {
            lines << QStringLiteral("Abhilfe: %1").arg(p.remedy.toHtmlEscaped());
        }
        detailLabel_->setText(lines.join(QStringLiteral("<br>")));
        return;
    }
    const QString id = scene_->selected();
    if (id.isEmpty() || !hasForces_) {
        detailLabel_->setText(QStringLiteral("Beitrag in Tabelle oder 3D wählen."));
        return;
    }
    auto all = currentForces_.contributions;
    all += forcedisplay::derivedContributions(currentForces_);
    for (const auto& c : all) {
        if (c.id != id) {
            continue;
        }
        QStringList lines;
        lines << QStringLiteral("<b>%1</b>  <span style='color:#8ca4b8'>(%2)</span>")
                     .arg(forcedisplay::contributionLabel(c).toHtmlEscaped(), c.id.toHtmlEscaped());
        const bool derived = c.total
            || (c.kind == forcedisplay::Kind::Moment
                && (c.category == forcedisplay::Category::Weight
                    || c.category == forcedisplay::Category::Buoyancy));
        if (derived) {
            lines << QStringLiteral("Anzeigewert der GUI: Summe bzw. Moment um S aus den Core-Beiträgen.");
        }
        const QString pointName = c.kind == forcedisplay::Kind::Moment ? QStringLiteral("Bezugspunkt")
                                                                      : QStringLiteral("Angriffspunkt");
        lines << QStringLiteral("%1: %2 · Frame: %3%4")
                     .arg(pointName, vectorText(c.point, QStringLiteral("m")),
                         c.frame.isEmpty() ? QStringLiteral("–") : c.frame,
                         c.pointKind.isEmpty() ? QString() : QStringLiteral(" · Punktart: %1").arg(c.pointKind));
        lines << QStringLiteral("Vektor: %1").arg(forcedisplay::showsValues(c)
                    ? vectorText(c.vector, c.unit)
                    : forcedisplay::stateText(c.state));
        if (c.momentS) {
            lines << QStringLiteral("Moment um S: %1").arg(vectorText(c.momentS, QStringLiteral("N·m")));
        }
        QStringList meta{QStringLiteral("Zustand: %1").arg(forcedisplay::stateText(c.state))};
        if (!c.source.isEmpty()) {
            meta << QStringLiteral("Quelle: %1").arg(c.source);
        }
        if (!c.role.isEmpty()) {
            meta << QStringLiteral("Rolle: %1").arg(c.role);
        }
        if (!c.errorId.isEmpty()) {
            meta << QStringLiteral("Fehler: %1").arg(c.errorId);
        }
        lines << meta.join(QStringLiteral(" · "));
        detailLabel_->setText(lines.join(QStringLiteral("<br>")));
        return;
    }
    detailLabel_->setText(QStringLiteral("Auswahl im aktuellen Zustand nicht vorhanden."));
}

void MainWindow::selectContribution(const QString& id)
{
    const QSignalBlocker blocker(valuesTree_);
    auto* item = contributionItems_.value(id);
    valuesTree_->setCurrentItem(item);
    if (item != nullptr) {
        valuesTree_->scrollToItem(item);
    } else {
        valuesTree_->clearSelection();
    }
    updateDetail();
}

void MainWindow::copyValues() const
{
    if (workspaceIndex_ > 0) {
        QStringList lines;
        QStringList header;
        for (int c = 0; c < staticValues_->columnCount(); ++c)
            header << staticValues_->headerItem()->text(c);
        lines << header.join(QLatin1Char('\t'));
        const bool selectedOnly = !staticValues_->selectedItems().isEmpty();
        for (int i = 0; i < staticValues_->topLevelItemCount(); ++i) {
            const auto* row = staticValues_->topLevelItem(i);
            if (row->isHidden() || (selectedOnly && !row->isSelected())) continue;
            QStringList cells;
            for (int c = 0; c < staticValues_->columnCount(); ++c) cells << row->text(c);
            lines << cells.join(QLatin1Char('\t'));
        }
        QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
        return;
    }
    if (!hasForces_ && !problems_.isEmpty()) {
        QStringList lines{QStringLiteral("Modell\t%1\t%2").arg(modelName_, modelId_),
            QStringLiteral("Fassung\t%1").arg(modelVersion_), QString(),
            QStringLiteral("Fehler-ID\tKurztext\tFundstelle\tMeldung\tAbhilfe")};
        for (const auto& p : problems_) {
            lines << QStringList{p.code, p.summary, p.location, p.message, p.remedy}.join(QLatin1Char('\t'));
        }
        QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
        return;
    }
    if (!hasForces_) {
        return;
    }
    QStringList lines;
    const auto summary = forcedisplay::balanceSummary(currentForces_);
    lines << summary.title;
    lines << summary.lines;
    lines << QStringLiteral("Zeit\t%1 s\tSchritt\t%2")
                 .arg(forcedisplay::formatFixed(currentForces_.time, 2))
                 .arg(currentForces_.step);
    lines << QString();
    QStringList header;
    for (int column = 0; column < ColumnCount; ++column) {
        header << valuesTree_->headerItem()->text(column);
    }
    lines << QStringLiteral("Gruppe\t") + header.join(QLatin1Char('\t'));
    for (int g = 0; rootItem_ != nullptr && g < rootItem_->childCount(); ++g) {
        const auto* group = rootItem_->child(g);
        for (int i = 0; i < group->childCount(); ++i) {
            const auto* item = group->child(i);
            QStringList cells{group->text(ColName)};
            for (int column = 0; column < ColumnCount; ++column) {
                cells << item->text(column);
            }
            lines << cells.join(QLatin1Char('\t'));
        }
    }
    QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

void MainWindow::showError(const QString& title, const QString& message)
{
    statusLabel_->setToolTip(message);
    statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
    QMessageBox::warning(this, title, message);
}
