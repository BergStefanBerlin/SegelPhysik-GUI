#include "main_window.hpp"

#include "vtk_scene.hpp"
#include "graphics_dialog.hpp"
#include "navigation.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QMetaObject>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSlider>
#include <QThread>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <limits>

using forcedisplay::Contribution;

namespace {

QString runtimeText(double seconds)
{
    if (seconds < 1.0) {
        return QStringLiteral("%1 ms").arg(seconds * 1000.0, 0, 'f', 2);
    }
    return QStringLiteral("%1 s").arg(seconds, 0, 'f', 3);
}

QString movementValueText(double requested, double confirmed, bool rudder)
{
    return rudder
        ? QStringLiteral("Eingabe %1° · Core %2°")
            .arg(requested, 0, 'f', 1).arg(confirmed, 0, 'f', 1)
        : QStringLiteral("Eingabe %1 % · Core %2 %")
            .arg(requested, 0, 'f', 1).arg(confirmed, 0, 'f', 1);
}

const QString folderSettingKey = QStringLiteral("modellordner");
const QString modelSettingKey = QStringLiteral("modell_id");
const QString legacyModelSettingKey = QStringLiteral("modell");
constexpr int liveIntervalMs = 33;
constexpr double gravity = 9.81;
const QColor rejectedModelColor(0x8a, 0x9b, 0xab);
const QChar warningSign(0x26A0);

enum Column { ColName, ColMagnitude, ColX, ColY, ColZ, ColState, ColScale, ColumnCount };

QString defaultModelFolder()
{
    const QString besideExecutable =
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("modelle"));
    if (QDir(besideExecutable).exists()) {
        return besideExecutable;
    }
#ifdef SEGELPHYSIK_DEFAULT_MODEL_DIR
    return QString::fromUtf8(SEGELPHYSIK_DEFAULT_MODEL_DIR);
#else
    return {};
#endif
}

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
    setWindowTitle(QStringLiteral("SegelPhysik – Hydrostatik"));
    resize(1500, 940);
    setMinimumSize(1024, 680);
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(12);

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
    header->addWidget(chooseFolderButton_);
    root->addLayout(header);

    auto* mainArea = new QHBoxLayout;
    mainArea->setSpacing(14);

    auto* catalogPanel = new QWidget;
    catalogPanel->setObjectName(QStringLiteral("panel"));
    catalogPanel->setMinimumWidth(210);
    catalogPanel->setMaximumWidth(260);
    auto* catalogLayout = new QVBoxLayout(catalogPanel);
    catalogLayout->setContentsMargins(14, 18, 14, 14);
    catalogLayout->setSpacing(10);
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
    catalogLayout->addWidget(catalogCaption);
    catalogLayout->addWidget(folderLabel_);
    catalogLayout->addWidget(modelList_, 1);
    mainArea->addWidget(catalogPanel);
    mainArea->addWidget(scene_, 1);

    auto* panel = new QWidget;
    panel->setObjectName(QStringLiteral("panel"));
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(18, 18, 18, 18);
    panelLayout->setSpacing(10);

    auto* modelCaption = new QLabel(QStringLiteral("AKTIVES MODELL"));
    modelCaption->setObjectName(QStringLiteral("caption"));
    modelLabel_ = new QLabel(QStringLiteral("Kein Modell ausgewählt"));
    modelLabel_->setWordWrap(true);
    modelLabel_->setObjectName(QStringLiteral("modelName"));
    panelLayout->addWidget(modelCaption);
    panelLayout->addWidget(modelLabel_);

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
    realtimeCheck_->setChecked(true);
    realtimeCheck_->setToolTip(QStringLiteral(
        "Ein: Der Core rechnet jeden Schritt erst, wenn seine Simulationszeit "
        "erreicht ist.\nAus: Der Core rechnet ungebremst; die Anzeige folgt "
        "so schnell wie sinnvoll."));
    parameterForm_->addRow(QStringLiteral("Takt"), realtimeCheck_);
    panelLayout->addLayout(parameterForm_);

    runButton_ = new QPushButton(QStringLiteral("Core-Lauf starten"));
    runButton_->setObjectName(QStringLiteral("primaryButton"));
    cancelButton_ = new QPushButton(QStringLiteral("Lauf stoppen"));
    cancelButton_->setObjectName(QStringLiteral("stopSimulation"));
    cancelButton_->setEnabled(false);
    panelLayout->addWidget(runButton_);
    panelLayout->addWidget(cancelButton_);

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
    panelLayout->addWidget(buildDisplayPanel());
    panelLayout->addStretch(1);

    auto* panelScroll = new QScrollArea;
    panelScroll->setObjectName(QStringLiteral("panelScroll"));
    panelScroll->setWidget(panel);
    panelScroll->setWidgetResizable(true);
    panelScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    panelScroll->setFrameShape(QFrame::NoFrame);
    panelScroll->setMinimumWidth(320);
    panelScroll->setMaximumWidth(380);
    mainArea->addWidget(panelScroll);
    root->addLayout(mainArea, 1);

    auto* playback = new QHBoxLayout;
    stateLabel_ = new QLabel(QStringLiteral("Zustand 0 / 0"));
    stateSlider_ = new QSlider(Qt::Horizontal);
    stateSlider_->setRange(0, 0);
    playback->addWidget(new QLabel(QStringLiteral("WIEDERGABE")));
    playback->addWidget(stateSlider_, 1);
    playback->addWidget(stateLabel_);
    root->addLayout(playback);

    setCentralWidget(central);
    valuesDock_ = buildValuesDock();
    addDockWidget(Qt::BottomDockWidgetArea, valuesDock_);

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background:#0b1420; color:#e8eef5; "
        "font-family:'Segoe UI'; font-size:10pt; }"
        "QLabel, QCheckBox { background:transparent; }"
        "QCheckBox::indicator { width:15px; height:15px; border:1px solid "
        "#4a6680; border-radius:4px; background:#0c1722; }"
        "QCheckBox::indicator:checked { background:#36c1d2; border-color:#5fd9e6; }"
        "QLabel#title { font-size:19pt; font-weight:700; letter-spacing:2px; }"
        "QLabel#subtitle { color:#8ca4b8; font-size:10pt; }"
        "QLabel#caption { color:#88a3b9; font-size:8pt; font-weight:700; "
        "letter-spacing:1.5px; }"
        "QLabel#modelName { font-size:13pt; font-weight:600; }"
        "QLabel#status { color:#b8e7c3; font-size:11pt; }"
        "QLabel#time, QLabel#legend { color:#a5bacb; font-size:9pt; }"
        "QLabel#balance { background:#111e2c; border:1px solid #23384b; "
        "border-radius:8px; padding:8px; }"
        "QLabel#detail { color:#c9d6e2; font-size:9pt; }"
        "QWidget#panel { background:#111e2c; border:1px solid #23384b; "
        "border-radius:12px; }"
        "QScrollArea#panelScroll { background:transparent; }"
        "QPushButton { background:#1b2b3d; border:1px solid #344c61; "
        "border-radius:7px; padding:9px 13px; font-weight:600; }"
        "QPushButton:hover { background:#263d52; }"
        "QPushButton:disabled { color:#637587; background:#17222d; }"
        "QPushButton#primaryButton { background:#137c91; border-color:#189ab1; }"
        "QPushButton#primaryButton:hover { background:#1795aa; }"
        "QPushButton#smallButton { padding:5px 9px; }"
        "QComboBox, QDoubleSpinBox { background:#0c1722; border:1px solid "
        "#344c61; border-radius:6px; padding:6px; }"
        "QListWidget#modelList { background:#0c1722; border:1px solid "
        "#263b4d; border-radius:6px; padding:3px; }"
        "QListWidget#modelList::item { padding:4px 6px; border-radius:4px; }"
        "QListWidget#modelList::item:selected { background:#137c91; "
        "color:#ffffff; }"
        "QTreeWidget { background:#0c1722; alternate-background-color:#101e2b; "
        "border:1px solid #263b4d; }"
        "QTreeWidget::item:selected { background:#5c4a12; color:#ffd166; }"
        "QHeaderView::section { background:#162636; color:#9fb3c3; padding:5px; "
        "border:0; }"
        "QDockWidget { color:#88a3b9; font-weight:700; }"
        "QDockWidget::title { background:#111e2c; padding:6px; }"
        "QSlider::groove:horizontal { height:5px; background:#263b4d; "
        "border-radius:2px; }"
        "QScrollBar:vertical { background:#0b1420; width:10px; margin:0; }"
        "QScrollBar::handle:vertical { background:#2c4459; border-radius:5px; "
        "min-height:24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }"
        "QSlider::handle:horizontal { background:#36c1d2; width:15px; "
        "margin:-6px 0; border-radius:7px; }"));

    folderWatcher_ = new QFileSystemWatcher(this);
    rescanTimer_ = new QTimer(this);
    rescanTimer_->setSingleShot(true);
    rescanTimer_->setInterval(400);
    liveTimer_ = new QTimer(this);
    liveTimer_->setInterval(liveIntervalMs);

    connect(chooseFolderButton_, &QPushButton::clicked, this, [this] { chooseModelFolder(); });
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
    connect(folderWatcher_, &QFileSystemWatcher::directoryChanged, this,
        [this](const QString&) { scheduleFolderRescan(); });
    connect(folderWatcher_, &QFileSystemWatcher::fileChanged, this,
        [this](const QString&) { scheduleFolderRescan(); });
    connect(rescanTimer_, &QTimer::timeout, this, [this] { reloadCatalog(); });
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
    scene_->onReferencesChanged = [this] {
        storeReferences();
        updateReferenceLabel();
        updateValues();
    };

    auto* setupMenu = menuBar()->addMenu(QStringLiteral("Setup"));
    setupMenu->addAction(QStringLiteral("Grafik …"), this, &MainWindow::setupGraphics);
    setupMenu->addAction(QStringLiteral("Materialvorschau …"), this, &MainWindow::setupMaterialPreview);
    loadSettings();
    updateModeControls();
    setBusy(false);
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

    forcesCheck_ = new QCheckBox(QStringLiteral("Vektoren anzeigen"));
    forcesCheck_->setObjectName(QStringLiteral("displayVectors"));
    forcesCheck_->setChecked(true);
    layout->addWidget(forcesCheck_);

    auto* form = new QFormLayout;
    form->setSpacing(6);
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
    form->addRow(QStringLiteral("Kamera"), cameraCombo_);
    navigationCheck_ = new QCheckBox(QStringLiteral("Kompass und Lageanzeigen"));
    navigationCheck_->setObjectName(QStringLiteral("displayNavigation"));
    navigationCheck_->setChecked(true);
    connect(navigationCheck_, &QCheckBox::toggled, scene_, &VtkSceneWidget::setNavigationVisible);
    form->addRow(navigationCheck_);
    auto scaleCombo = [] {
        auto* combo = new QComboBox;
        combo->addItem(QStringLiteral("logarithmisch"),
            static_cast<int>(forcedisplay::ScaleMode::Logarithmic));
        combo->addItem(QStringLiteral("linear"), static_cast<int>(forcedisplay::ScaleMode::Linear));
        return combo;
    };
    forceScaleCombo_ = scaleCombo();
    momentScaleCombo_ = scaleCombo();
    visibilityCombo_ = new QComboBox;
    visibilityCombo_->setObjectName(QStringLiteral("graphicsProfile"));
    visibilityCombo_->addItem(QStringLiteral("Technik – durchsichtig"), 0);
    visibilityCombo_->addItem(QStringLiteral("Optik – grafisch"), 1);
    form->addRow(QStringLiteral("Skala Kraft"), forceScaleCombo_);
    form->addRow(QStringLiteral("Skala Moment"), momentScaleCombo_);
    form->addRow(QStringLiteral("Darstellung"), visibilityCombo_);
    layout->addLayout(form);
    graphicsStatus_ = new QLabel;
    graphicsStatus_->setWordWrap(true);
    graphicsStatus_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(graphicsStatus_);

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
    lateralCheck_->setChecked(true);
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
    layout->addWidget(lateralCheck_);

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

    valuesCheck_ = new QCheckBox(QStringLiteral("Wertefenster anzeigen"));
    valuesCheck_->setChecked(true);
    layout->addWidget(valuesCheck_);

    QList<QCheckBox*> displayChecks{forcesCheck_, leverCheck_, lateralCheck_};
    for (auto* check : forceChecks_) {
        displayChecks << check;
    }
    for (auto* check : momentChecks_) {
        displayChecks << check;
    }
    for (auto* check : displayChecks) {
        connect(check, &QCheckBox::toggled, this, [this](bool) { applyDisplayOptions(); });
    }
    for (auto* combo : {forceScaleCombo_, momentScaleCombo_, visibilityCombo_}) {
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
    balanceLabel_->setMinimumWidth(260);
    balanceLabel_->setMaximumWidth(360);
    balanceLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    stepInfoLabel_ = new QLabel(QStringLiteral("Zeit: –  ·  Schritt: –"));
    stepInfoLabel_->setObjectName(QStringLiteral("time"));
    auto* copyButton = new QPushButton(QStringLiteral("Werte kopieren"));
    copyButton->setObjectName(QStringLiteral("smallButton"));
    copyButton->setToolTip(QStringLiteral("Bilanz und Tabelle als Text (Strg+C)"));
    left->addWidget(balanceLabel_, 1);
    left->addWidget(stepInfoLabel_);
    left->addWidget(copyButton);

    auto* right = new QVBoxLayout;
    valuesTree_ = new QTreeWidget;
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

    layout->addLayout(left);
    layout->addLayout(right, 1);
    dock->setWidget(content);

    connect(copyButton, &QPushButton::clicked, this, [this] { copyValues(); });
    auto* shortcut = new QShortcut(QKeySequence::Copy, content);
    shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut, &QShortcut::activated, this, [this] { copyValues(); });
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
    shuttingDown_.store(true);
    cancellation_.store(true);
    for (const auto& thread : {openThread_, simulationThread_, catalogThread_}) {
        if (thread != nullptr) {
            thread->wait();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings;
    settings.setValue(QStringLiteral("fenster/geometrie"), saveGeometry());
    settings.setValue(QStringLiteral("fenster/zustand"), saveState());
    saveDisplaySettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::loadSettings()
{
    QSettings settings;
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
    scene_->setNavigationVisible(navigationCheck_->isChecked());
    for (std::size_t i = 0; i < forceChecks_.size(); ++i) {
        restoreCheck(forceChecks_[i], QStringLiteral("darstellung/kraft_%1").arg(i));
        restoreCheck(momentChecks_[i], QStringLiteral("darstellung/moment_%1").arg(i));
    }
    restoreCheck(leverCheck_, QStringLiteral("darstellung/hebelarm"));
    restoreCheck(lateralCheck_, QStringLiteral("darstellung/lateraldruckpunkt"));
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
    if (!graphics::load(settings, &graphics_, &graphicsError)) {
        QTimer::singleShot(0, this, [this, graphicsError] {
            showError(QStringLiteral("Grafikeinstellungen"), graphicsError);
        });
    }
    {
        const QSignalBlocker blocker(visibilityCombo_);
        visibilityCombo_->setCurrentIndex(graphics_.active);
    }
    graphicsReady_ = true;
    applyDisplayOptions();
}

void MainWindow::saveDisplaySettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("darstellung/navigation"), navigationCheck_->isChecked());
    settings.setValue(QStringLiteral("darstellung/pfeile"), forcesCheck_->isChecked());
    for (std::size_t i = 0; i < forceChecks_.size(); ++i) {
        settings.setValue(QStringLiteral("darstellung/kraft_%1").arg(i), forceChecks_[i]->isChecked());
        settings.setValue(QStringLiteral("darstellung/moment_%1").arg(i), momentChecks_[i]->isChecked());
    }
    settings.setValue(QStringLiteral("darstellung/hebelarm"), leverCheck_->isChecked());
    settings.setValue(QStringLiteral("darstellung/lateraldruckpunkt"), lateralCheck_->isChecked());
    settings.setValue(QStringLiteral("darstellung/skala_kraft"), forceScaleCombo_->currentData());
    settings.setValue(QStringLiteral("darstellung/skala_moment"), momentScaleCombo_->currentData());
}

void MainWindow::setupGraphics()
{
    editGraphicsSettings(this, graphics_, scene_->graphicsStatus(),
        [this](const graphics::Settings& proposed) {
            QSettings storage;
            QString error;
            if (!graphics::save(storage, proposed, &error)) {
                showError(QStringLiteral("Grafikeinstellungen"), error);
                return false;
            }
            graphics_ = proposed;
            applyDisplayOptions();
            return true;
        }, [this] { return graphicsStatus_->text(); });
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

void MainWindow::applyGraphicsProfile()
{
    auto profile = graphics_.profiles[static_cast<std::size_t>(graphics_.active)];
    const bool reduced = profile.liveReduction && live_ && !live_->progress().finished;
    if (reduced) {
        for (auto feature : {graphics::Feature::SSAO, graphics::Feature::Shadows,
                 graphics::Feature::Reflection}) {
            profile.enabled[static_cast<std::size_t>(feature)] = false;
        }
    }
    scene_->setGraphicsProfile(profile);
    if (scene_->motionStateCount() == 0 && displayedIndex_ >= 0 && displayedIndex_ < stateCount()) {
        displayState(displayedIndex_, true);
    }
    const QString prefix = reduced
        ? QStringLiteral("Live-Reduktion aktiv: SSAO, Schatten und Bootsspiegelung aus.\n")
        : QString();
    graphicsStatus_->setText(prefix + scene_->graphicsStatus());
}

void MainWindow::applyDisplayOptions()
{
    if (!graphicsReady_) return;
    const int active = visibilityCombo_->currentData().toInt();
    if (active != graphics_.active) {
        auto proposed = graphics_;
        proposed.active = active;
        QSettings storage;
        QString error;
        if (!graphics::save(storage, proposed, &error)) {
            const QSignalBlocker blocker(visibilityCombo_);
            visibilityCombo_->setCurrentIndex(graphics_.active);
            showError(QStringLiteral("Grafikeinstellungen"), error);
            return;
        }
        graphics_ = proposed;
    }
    const auto& profile = graphics_.profiles[static_cast<std::size_t>(graphics_.active)];
    applyGraphicsProfile();
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
    saveDisplaySettings();
    // Ausblenden ändert nur die Grafik; die Skala-Spalte folgt dem Skalenmodus.
    updateValues();
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
    modelFolder_ = usable ? QDir(folder).absolutePath() : defaultModelFolder();
    if (usable) {
        QSettings().setValue(folderSettingKey, modelFolder_);
    }
    modelCache_.clear();
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

void MainWindow::scheduleFolderRescan()
{
    rescanTimer_->start();
}

void MainWindow::reloadCatalog()
{
    if (catalogThread_ != nullptr) {
        catalogReloadPending_ = true;
        return;
    }
    catalogReloadPending_ = false;
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
                tip << QStringLiteral("Core-Katalog: fehlerhaft (%1 Fehler) – nur Ansicht")
                           .arg(entry.errorCount);
                tip << entry.diagnostics;
            }
            item->setToolTip(tip.join(QLatin1Char('\n')));
            if (entry.id == modelId_) {
                modelList_->setCurrentItem(item);
            }
        }
    }

    const auto watched = folderWatcher_->files() + folderWatcher_->directories();
    if (!watched.isEmpty()) {
        folderWatcher_->removePaths(watched);
    }
    const auto paths = watchedModelPaths(modelFolder_);
    if (!paths.isEmpty()) {
        folderWatcher_->addPaths(paths);
    }

    if (selectionPending_) {
        applyPendingSelection();
        return;
    }
    const auto* current = entryFor(modelId_);
    const bool running = simulationThread_ != nullptr;
    const bool changed = current == nullptr || current->version != previousVersion;
    if (!modelId_.isEmpty() && (changed || reopenAfterCatalog_)) {
        if (running) {
            // Laufende Simulationen bleiben unberührt (A-G-00022).
            modelLabel_->setText(QStringLiteral("%1\n(geänderte Modellfassung im Ordner)").arg(modelName_));
        } else if (current != nullptr) {
            const QString id = modelId_;
            modelId_.clear();
            openModel(id);
        }
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

void MainWindow::resetResults()
{
    live_.reset();
    for (auto it = movementSliders_.cbegin(); it != movementSliders_.cend(); ++it) {
        const auto id = it.key();
        const bool rudder = movementIsRudder_.value(id, false);
        const double initial = rudder ? 0.0 : 100.0;
        const QSignalBlocker blocker(it.value());
        it.value()->setValue(rudder ? 900 : 1000);
        movementConfirmedValues_[id] = initial;
        movementInputPending_[id] = false;
        if (auto* label = movementValueLabels_.value(id, nullptr))
            label->setText(movementValueText(initial, initial, rudder));
    }
    states_.clear();
    runtimeInfo_.clear();
    displayedIndex_ = -1;
    updateSliderRange(0);
    stateLabel_->setText(QStringLiteral("Zustand 0 / 0"));
    scene_->clearState();
    clearValues();
}

void MainWindow::openModel(const QString& id)
{
    if (simulationThread_ != nullptr) {
        return;
    }
    const auto* entry = entryFor(id);
    if (entry == nullptr) {
        return;
    }
    modelId_ = entry->id;
    cameraCombo_->setEnabled(false);
    modelVersion_ = entry->version;
    modelPath_ = entry->path;
    modelName_ = entry->name.isEmpty() ? entry->id : entry->name;
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
            if (!opened) {
                if (!catalog) {
                    throw std::runtime_error("Kein Modellkatalog verfügbar.");
                }
                auto fresh = std::make_shared<OpenedModel>();
                fresh->data = loadDisplayData(*catalog, selected.id, selected.version);
                fresh->parts = toDisplayParts(fresh->data.geometry);
                opened = fresh;
            }
            if (selected.usable) {
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
                    scene_->clearModel();
                    showModelProblems(selected.problems + problemsFromText(error));
                    return;
                }
                modelCache_.insert(cacheKey(selected.id, selected.version), opened);
                modelOpened(selected, opened, validation);
            },
            Qt::QueuedConnection);
    });
    openThread_ = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread] {
        if (openThread_ == thread) {
            openThread_ = nullptr;
        }
        setBusy(simulationThread_ != nullptr);
    });
    thread->start();
}

void MainWindow::modelOpened(const ModelEntry& entry, const std::shared_ptr<OpenedModel>& opened,
    const ModelValidation& validation)
{
    QString error;
    scene_->setElementGroups(opened->data.elementGroups);
    configureMovementControls(opened->data);
    if (!scene_->setModel(opened->parts, &error)) {
        statusLabel_->setText(QStringLiteral("Darstellung nicht verfügbar: %1").arg(error));
        statusLabel_->setToolTip(error);
        statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
        showModelProblems(entry.problems + problemsFromText(error));
        setBusy(false);
        return;
    }
    cameraCombo_->setEnabled(true);
    {
        const QSignalBlocker blocker(cameraCombo_);
        cameraCombo_->setCurrentIndex(0);
    }
    initialReferences(opened->data);
    applyGraphicsProfile();
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
    movementIsRudder_.clear();
    movementConfirmedValues_.clear();
    movementInputPending_.clear();
    auto ids = displayData.elementMovements.keys();
    ids.sort();
    for (const auto& id : ids) {
        const auto definition = displayData.elementMovements.value(id);
        const bool rudder = definition.ruderwinkel;
        auto* slider = new QSlider(Qt::Horizontal, movementPanel_);
        slider->setObjectName(QStringLiteral("movement_%1").arg(id));
        slider->setRange(0, rudder ? 1800 : 1000);
        slider->setValue(rudder ? 900 : 1000);
        slider->setEnabled(false);
        auto* valueLabel = new QLabel(movementValueText(rudder ? 0.0 : 100.0,
            rudder ? 0.0 : 100.0, rudder), movementPanel_);
        valueLabel->setObjectName(QStringLiteral("movementValue_%1").arg(id));
        valueLabel->setMinimumWidth(125);
        auto* row = new QWidget(movementPanel_);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->addWidget(slider, 1);
        rowLayout->addWidget(valueLabel);
        movementLayout_->addRow(id, row);
        movementSliders_.insert(id, slider);
        movementValueLabels_.insert(id, valueLabel);
        movementIsRudder_.insert(id, rudder);
        movementConfirmedValues_.insert(id, rudder ? 0.0 : 100.0);
        movementInputPending_.insert(id, false);
        connect(slider, &QSlider::valueChanged, this, [this, id, slider, valueLabel, rudder](int position) {
            const double value = rudder ? (position - 900) / 10.0 : position / 10.0;
            movementInputPending_[id] = true;
            valueLabel->setText(movementValueText(value,
                movementConfirmedValues_.value(id, rudder ? 0.0 : 100.0), rudder));
            if (!live_ || simulationThread_ == nullptr) return;
            QString error;
            if (!live_->setMovementValue(id, value, &error)) {
                movementInputPending_[id] = false;
                const double confirmed = movementConfirmedValues_.value(id, rudder ? 0.0 : 100.0);
                const QSignalBlocker blocker(slider);
                slider->setValue(rudder
                    ? static_cast<int>(std::lround((confirmed + 90.0) * 10.0))
                    : static_cast<int>(std::lround(confirmed * 10.0)));
                valueLabel->setText(movementValueText(confirmed, confirmed, rudder));
                showError(QStringLiteral("Ruder-/Schwertstellung"), error);
            }
        });
    }
    movementPanel_->setVisible(!ids.isEmpty() && modeCombo_->currentData().toBool());
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
        const bool rudder = movementIsRudder_.value(id, false);
        const double setting = value.toDouble();
        movementConfirmedValues_[id] = setting;
        const double requested = rudder
            ? (slider->value() - 900) / 10.0 : slider->value() / 10.0;
        const bool pending = movementInputPending_.value(id, false);
        if (pending && std::abs(requested - setting) < 1e-9)
            movementInputPending_[id] = false;
        if (!movementInputPending_.value(id, false)) {
            const QSignalBlocker blocker(slider);
            slider->setValue(rudder
                ? static_cast<int>(std::lround((setting + 90.0) * 10.0))
                : static_cast<int>(std::lround(setting * 10.0)));
        }
        const double displayedRequest = movementInputPending_.value(id, false)
            ? requested : setting;
        label->setText(movementValueText(displayedRequest, setting, rudder));
    }
}

void MainWindow::startSimulation()
{
    if (!validation_.valid || modelPath_.isEmpty()) {
        showError(QStringLiteral("Kein gültiges Modell"),
            QStringLiteral("Bitte zuerst ein vom Core akzeptiertes Modell laden."));
        return;
    }
    if (simulationThread_ != nullptr && simulationThread_->isRunning()) {
        return;
    }

    const bool dynamic = modeCombo_->currentData().toBool();
    const double duration = durationSpin_->value();
    const double step = stepSpin_->value();
    // Moduswechsel: aktuelle Werte übernehmen, nicht die Initialwerte.
    QJsonObject start;
    QString startNote;
    if (lastRunDynamic_ && *lastRunDynamic_ != dynamic && displayedIndex_ >= 0) {
        QString error;
        start = stateAt(displayedIndex_, &error);
        if (!start.isEmpty()) {
            startNote = QStringLiteral(" (Start aus Zustand bei t = %1 s)")
                            .arg(start.value(QStringLiteral("zeit_s")).toDouble(), 0, 'f', 2);
        }
    }
    lastRunDynamic_ = dynamic;
    cancellation_.store(false);
    resetResults();
    statusLabel_->setToolTip(QString());
    statusLabel_->setStyleSheet(QString());
    if (dynamic) {
        startLiveSimulation(duration, step, realtimeCheck_->isChecked(), start);
        if (live_) {
            statusLabel_->setText(statusLabel_->text() + startNote);
        }
        return;
    }

    const QString path = modelPath_;
    const QString id = modelId_;
    const auto validation = validation_;
    statusLabel_->setText(QStringLiteral("Statischer Core-Lauf läuft …") + startNote);
    timeLabel_->setText(QStringLiteral("Simulationszeit: –  ·  Rechenzeit: läuft"));
    setBusy(true);

    auto* thread = QThread::create([this, path, id, validation, start] {
        const auto result = runSimulation(path, validation, false, 0.0, 0.01, &cancellation_, start);
        QMetaObject::invokeMethod(this,
            [this, id, result] {
                if (id != modelId_) {
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
    QString error;
    std::shared_ptr<LiveSimulation> live =
        LiveSimulation::create(modelPath_, validation_, duration, step, paced, &error, start,
            TestDriveOptions{testDriveCheck_->isChecked(), testForceSpin_->value(), testSpeedLimit_});
    if (!live) {
        statusLabel_->setText(QStringLiteral("Core-Lauf nicht gestartet"));
        showError(QStringLiteral("Core-Simulation"), error);
        return;
    }
    live_ = live;
    followLive_ = true;
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
    thread->start();
    cancelButton_->setEnabled(true);
    liveTimer_->start();
    updateLiveProgress();
}

void MainWindow::simulationFinished()
{
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
    if (!progress.testDriveStatus.isEmpty()) testDriveStatus_->setText(progress.testDriveStatus);
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
        QStringLiteral("%1 · %2 Zustände%3").arg(progress.status).arg(progress.states).arg(diagnosis));
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
        showError(QStringLiteral("Simulation nicht abgeschlossen"),
            result.diagnostic.isEmpty() ? result.status
                                        : QStringLiteral("%1\n%2").arg(result.status, result.diagnostic));
    }
}

void MainWindow::displayState(qsizetype index, bool force)
{
    const auto count = stateCount();
    if (index < 0 || index >= count) {
        return;
    }
    const bool live = live_ && simulationThread_ != nullptr;
    const QString suffix = live
        ? (followLive_ ? QStringLiteral("  · live") : QStringLiteral("  · Rückblick"))
        : QString();
    stateLabel_->setText(QStringLiteral("Zustand %1 / %2%3").arg(index + 1).arg(count).arg(suffix));
    if (index == displayedIndex_ && !force) {
        return;
    }
    QString error;
    const auto state = stateAt(index, &error);
    forcedisplay::ForceState forces;
    try {
        if (state.isEmpty()) {
            throw std::runtime_error(error.toStdString());
        }
        forces = forcedisplay::parseForceState(state);
    } catch (const std::exception& exception) {
        error = QString::fromUtf8(exception.what());
    }
    if (error.isEmpty()) {
        QString motionError;
        if (scene_->motionStateCount() > index) scene_->resetMotionHistory();
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
    if (!error.isEmpty() || !scene_->setState(state, forces, &error)) {
        cancellation_.store(true);
        statusLabel_->setText(QStringLiteral("Darstellung des Zustands fehlgeschlagen"));
        showError(QStringLiteral("Core-Zustand"), error);
        return;
    }
    displayedIndex_ = index;
    applyGraphicsProfile();
    currentForces_ = forces;
    hasForces_ = true;
    updateMovementControls(state);
    updateValues();
    const auto time = state.value(QStringLiteral("zeit_s")).toDouble();
    timeLabel_->setText(
        QStringLiteral("Simulationszeit: %1 s  ·  %2").arg(time, 0, 'f', 3).arg(runtimeInfo_));
}

void MainWindow::setBusy(bool busy)
{
    const bool hasModel = validation_.valid && !modelPath_.isEmpty();
    const bool dynamic = modeCombo_->currentData().toBool();
    const bool running = simulationThread_ != nullptr;
    const bool movementReady = live_ != nullptr && live_->progress().states > 0;
    testDrivePanel_->setVisible(dynamic);
    movementPanel_->setVisible(dynamic && !movementSliders_.isEmpty());
    for (auto* slider : movementSliders_)
        slider->setEnabled(hasModel && dynamic && running && movementReady);
    testDriveCheck_->setEnabled(hasModel && dynamic && (!busy || running));
    testForceSpin_->setEnabled(!busy && dynamic);
    testSpeedSpin_->setEnabled(!busy && dynamic);
    chooseFolderButton_->setEnabled(!running);
    modelList_->setEnabled(!running);
    modeCombo_->setEnabled(!busy && hasModel);
    runButton_->setEnabled(!busy && hasModel);
    cancelButton_->setEnabled(running);
    durationSpin_->setEnabled(!busy && dynamic);
    stepSpin_->setEnabled(!busy && dynamic);
    realtimeCheck_->setEnabled(!busy && dynamic);
}

void MainWindow::updateModeControls()
{
    const bool dynamic = modeCombo_->currentData().toBool();
    parameterForm_->setRowVisible(durationSpin_, dynamic);
    parameterForm_->setRowVisible(stepSpin_, dynamic);
    parameterForm_->setRowVisible(realtimeCheck_, dynamic);
    setBusy(simulationThread_ != nullptr || openThread_ != nullptr);
}

void MainWindow::clearValues()
{
    const QSignalBlocker blocker(valuesTree_);
    valuesTree_->clear();
    rootItem_ = nullptr;
    groupItems_.clear();
    contributionItems_.clear();
    currentForces_ = {};
    hasForces_ = false;
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

void MainWindow::updateValues()
{
    if (!hasForces_) {
        return;
    }
    const auto& forces = currentForces_;
    const auto summary = forcedisplay::balanceSummary(forces);
    QString card = QStringLiteral("<b>%1</b>").arg(summary.title.toHtmlEscaped());
    if (!forces.flooded.isEmpty()) {
        card += QStringLiteral("<br><span style=\"color:#ff8a7a;\">Vollgelaufen: %1</span>")
                    .arg(forces.flooded.join(QStringLiteral(", ")).toHtmlEscaped());
    }
    for (const auto& line : summary.lines) {
        card += QStringLiteral("<br>") + line.toHtmlEscaped();
    }
    balanceLabel_->setText(card);
    balanceLabel_->setStyleSheet(summary.complete ? QStringLiteral("color:#b8e7c3;")
                                                  : QStringLiteral("color:#ffd8a0;"));
    stepInfoLabel_->setText(QStringLiteral("Zeit: %1 s  ·  Schritt: %2  ·  Vertrag V%3")
            .arg(forcedisplay::formatFixed(forces.time, 2))
            .arg(forces.step)
            .arg(forces.contract));

    const QSignalBlocker blocker(valuesTree_);
    if (rootItem_ == nullptr) {
        rootItem_ = new QTreeWidgetItem(valuesTree_);
        rootItem_->setExpanded(true);
        rootItem_->setFlags(Qt::ItemIsEnabled);
    }
    rootItem_->setText(ColName, modelName_.isEmpty() ? forces.modelId : modelName_);
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
