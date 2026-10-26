#include "main_window.hpp"

#include "core_adapter.hpp"
#include "vtk_scene.hpp"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMetaObject>
#include <QMessageBox>
#include <QJsonArray>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

QString runtimeText(double seconds)
{
    if (seconds < 1.0) {
        return QStringLiteral("%1 ms").arg(seconds * 1000.0, 0, 'f', 2);
    }
    return QStringLiteral("%1 s").arg(seconds, 0, 'f', 3);
}

double vectorMagnitude(const QJsonValue& value)
{
    if (!value.isArray() || value.toArray().size() != 3) {
        return 0.0;
    }
    const auto vector = value.toArray();
    double sum = 0.0;
    for (const auto& component : vector) {
        if (!component.isDouble() || !std::isfinite(component.toDouble())) {
            return 0.0;
        }
        sum += component.toDouble() * component.toDouble();
    }
    return std::sqrt(sum);
}

QString forceName(const QString& id)
{
    if (id.endsWith(QStringLiteral(":gewicht"))) {
        return QStringLiteral("Gewicht");
    }
    if (id.endsWith(QStringLiteral(":auftrieb"))) {
        return QStringLiteral("Auftrieb");
    }
    return id.isEmpty() ? QStringLiteral("Kraft") : id;
}

const QString folderSettingKey = QStringLiteral("modellordner");
const QString modelSettingKey = QStringLiteral("modell");
constexpr int liveIntervalMs = 33;
const QColor rejectedModelColor(0x8a, 0x9b, 0xab);

QString defaultModelFolder()
{
    const QString besideExecutable =
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QStringLiteral("modelle"));
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

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , scene_(new VtkSceneWidget(this))
{
    setWindowTitle(QStringLiteral("SegelPhysik – Hydrostatik"));
    resize(1440, 900);
    setMinimumSize(1024, 680);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(12);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("SEGELPHYSIK"));
    title->setObjectName(QStringLiteral("title"));
    auto* subtitle = new QLabel(
        QStringLiteral("Hydrostatik · Gleichgewicht · Bewegung"));
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
    mainArea->addWidget(scene_, 1);

    auto* panel = new QWidget;
    panel->setObjectName(QStringLiteral("panel"));
    panel->setMinimumWidth(300);
    panel->setMaximumWidth(360);
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(18, 18, 18, 18);
    panelLayout->setSpacing(12);

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
    mainArea->insertWidget(0, catalogPanel);

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
    modeCombo_->addItem(QStringLiteral("Statisches Gleichgewicht"), false);
    modeCombo_->addItem(QStringLiteral("Dynamischer Verlauf"), true);
    panelLayout->addWidget(modeCaption);
    panelLayout->addWidget(modeCombo_);

    auto* parameterForm = new QFormLayout;
    parameterForm_ = parameterForm;
    durationSpin_ = new QDoubleSpinBox;
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
    parameterForm->addRow(QStringLiteral("Dauer"), durationSpin_);
    parameterForm->addRow(QStringLiteral("Schrittweite"), stepSpin_);
    realtimeCheck_ = new QCheckBox(QStringLiteral("Echtzeit (1 s = 1 s)"));
    realtimeCheck_->setChecked(true);
    realtimeCheck_->setToolTip(QStringLiteral(
        "Ein: Der Core rechnet jeden Schritt erst, wenn seine Simulationszeit "
        "erreicht ist.\nAus: Der Core rechnet ungebremst; die Anzeige folgt "
        "so schnell wie sinnvoll."));
    parameterForm->addRow(QStringLiteral("Takt"), realtimeCheck_);
    durationSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    stepSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    stepSpin_->setSuffix(QStringLiteral(" s"));
    panelLayout->addLayout(parameterForm);

    runButton_ = new QPushButton(QStringLiteral("Core-Lauf starten"));
    runButton_->setObjectName(QStringLiteral("primaryButton"));
    cancelButton_ = new QPushButton(QStringLiteral("Lauf stoppen"));
    cancelButton_->setEnabled(false);
    panelLayout->addWidget(runButton_);
    panelLayout->addWidget(cancelButton_);

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

    auto* forceCaption = new QLabel(QStringLiteral("KRAFTBEITRÄGE"));
    forceCaption->setObjectName(QStringLiteral("caption"));
    forcesCheck_ = new QCheckBox(QStringLiteral("Pfeile in 3D anzeigen"));
    forcesCheck_->setChecked(true);
    forceTable_ = new QTableWidget(0, 2);
    forceTable_->setHorizontalHeaderLabels(
        {QStringLiteral("Beitrag"), QStringLiteral("Betrag")});
    forceTable_->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    forceTable_->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    forceTable_->verticalHeader()->hide();
    forceTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    forceTable_->setSelectionMode(QAbstractItemView::NoSelection);
    forceTable_->setMinimumHeight(120);
    forceTable_->setObjectName(QStringLiteral("forceTable"));
    panelLayout->addWidget(forceCaption);
    panelLayout->addWidget(forcesCheck_);
    panelLayout->addWidget(forceTable_, 1);
    auto* legend = new QLabel(
        QStringLiteral("<span style='color:#ff745f'>● Gewicht</span>  "
                       "<span style='color:#42d5ff'>● Auftrieb</span><br>"
                       "Pfeile relativ skaliert; Werte bleiben SI."));
    legend->setObjectName(QStringLiteral("legend"));
    panelLayout->addWidget(legend);

    mainArea->addWidget(panel);
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
        "QWidget#panel { background:#111e2c; border:1px solid #23384b; "
        "border-radius:12px; }"
        "QPushButton { background:#1b2b3d; border:1px solid #344c61; "
        "border-radius:7px; padding:9px 13px; font-weight:600; }"
        "QPushButton:hover { background:#263d52; }"
        "QPushButton:disabled { color:#637587; background:#17222d; }"
        "QPushButton#primaryButton { background:#137c91; border-color:#189ab1; }"
        "QPushButton#primaryButton:hover { background:#1795aa; }"
        "QComboBox, QDoubleSpinBox { background:#0c1722; border:1px solid "
        "#344c61; border-radius:6px; padding:7px; }"
        "QListWidget#modelList { background:#0c1722; border:1px solid "
        "#263b4d; border-radius:6px; padding:3px; }"
        "QListWidget#modelList::item { padding:4px 6px; border-radius:4px; }"
        "QListWidget#modelList::item:selected { background:#137c91; "
        "color:#ffffff; }"
        "QTableWidget { background:#0c1722; alternate-background-color:#101e2b; "
        "gridline-color:#263b4d; border:1px solid #263b4d; }"
        "QHeaderView::section { background:#162636; color:#9fb3c3; padding:6px; "
        "border:0; }"
        "QSlider::groove:horizontal { height:5px; background:#263b4d; "
        "border-radius:2px; }"
        "QSlider::handle:horizontal { background:#36c1d2; width:15px; "
        "margin:-6px 0; border-radius:7px; }"));

    folderWatcher_ = new QFileSystemWatcher(this);
    rescanTimer_ = new QTimer(this);
    rescanTimer_->setSingleShot(true);
    rescanTimer_->setInterval(400);
    liveTimer_ = new QTimer(this);
    liveTimer_->setInterval(liveIntervalMs);

    connect(chooseFolderButton_, &QPushButton::clicked,
        this, [this] { chooseModelFolder(); });
    connect(modelList_, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* current, QListWidgetItem*) {
            if (current == nullptr) {
                return;
            }
            const auto path = current->data(Qt::UserRole).toString();
            if (samePath(path) != samePath(modelPath_)) {
                openModel(path);
            }
        });
    connect(folderWatcher_, &QFileSystemWatcher::directoryChanged,
        this, [this](const QString& path) { scheduleFolderRescan(path); });
    connect(folderWatcher_, &QFileSystemWatcher::fileChanged,
        this, [this](const QString& path) { scheduleFolderRescan(path); });
    connect(rescanTimer_, &QTimer::timeout,
        this, [this] { rescanModelFolder(); });
    connect(liveTimer_, &QTimer::timeout,
        this, [this] { updateLiveProgress(); });
    connect(runButton_, &QPushButton::clicked,
        this, [this] { startSimulation(); });
    connect(cancelButton_, &QPushButton::clicked,
        this, [this] { cancellation_.store(true); });
    connect(modeCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
        this, [this](int) { updateModeControls(); });
    connect(stateSlider_, &QSlider::valueChanged, this, [this](int value) {
        if (!programmaticSlider_) {
            followLive_ = value >= stateSlider_->maximum();
        }
        displayState(value);
    });
    connect(forcesCheck_, &QCheckBox::toggled,
        scene_, &VtkSceneWidget::setForceVisibility);

    updateModeControls();
    setBusy(false);
}

MainWindow::~MainWindow()
{
    shuttingDown_.store(true);
    cancellation_.store(true);
    if (validationThread_ != nullptr) {
        validationThread_->wait();
    }
    if (simulationThread_ != nullptr) {
        simulationThread_->wait();
    }
    if (catalogThread_ != nullptr) {
        catalogThread_->wait();
    }
}

void MainWindow::chooseModelFolder()
{
    const auto folder = QFileDialog::getExistingDirectory(this,
        QStringLiteral("Modellordner auswählen"), modelFolder_);
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
    rescanModelFolder();
}

void MainWindow::selectInitialModel(const QString& preferredPath)
{
    if (!preferredPath.isEmpty()) {
        openModel(preferredPath);
        return;
    }
    const auto remembered = QSettings().value(modelSettingKey).toString();
    QListWidgetItem* target = nullptr;
    for (int row = 0; row < modelList_->count(); ++row) {
        auto* item = modelList_->item(row);
        if (samePath(item->data(Qt::UserRole).toString()) == samePath(remembered)) {
            target = item;
            break;
        }
    }
    if (target == nullptr && modelList_->count() > 0) {
        target = modelList_->item(0);
    }
    if (target != nullptr) {
        modelList_->setCurrentItem(target);
    }
}

void MainWindow::scheduleFolderRescan(const QString& changedPath)
{
    if (!modelPath_.isEmpty()
        && QFileInfo(changedPath).absolutePath()
            == QFileInfo(modelPath_).absolutePath()) {
        reloadCurrentModel_ = true;
    }
    rescanTimer_->start();
}

void MainWindow::rescanModelFolder()
{
    const auto entries = scanModelFolder(modelFolder_);
    folderLabel_->setText(modelFolder_.isEmpty()
        ? QStringLiteral("Kein Modellordner gewählt")
        : QStringLiteral("%1 · %2 Modelle")
            .arg(QDir(modelFolder_).dirName())
            .arg(entries.size()));
    folderLabel_->setToolTip(QDir::toNativeSeparators(modelFolder_));

    {
        const QSignalBlocker blocker(modelList_);
        modelList_->clear();
        for (const auto& entry : entries) {
            auto* item = new QListWidgetItem(entry.name, modelList_);
            item->setData(Qt::UserRole, entry.path);
            item->setData(Qt::UserRole + 1, entry.name);
            item->setToolTip(QDir::toNativeSeparators(entry.path));
            if (samePath(entry.path) == samePath(modelPath_)) {
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
    validateCatalog(entries);

    const bool running = simulationThread_ != nullptr;
    if (reloadCurrentModel_ && !running && !modelPath_.isEmpty()) {
        reloadCurrentModel_ = false;
        if (QFileInfo::exists(modelPath_)) {
            openModel(modelPath_);
        }
    }
}

void MainWindow::validateCatalog(const QVector<ModelEntry>& entries)
{
    const int generation = ++catalogGeneration_;
    if (catalogThread_ != nullptr) {
        return;
    }
    auto* thread = QThread::create([this, entries, generation] {
        QVector<ModelValidation> results;
        for (const auto& entry : entries) {
            if (shuttingDown_.load()) {
                break;
            }
            results.push_back(validateModel(entry.path));
        }
        QMetaObject::invokeMethod(this,
            [this, entries, results, generation] {
                if (generation != catalogGeneration_) {
                    return;
                }
                for (int row = 0; row < modelList_->count(); ++row) {
                    auto* item = modelList_->item(row);
                    const auto index = row < entries.size() ? row : -1;
                    if (index < 0 || index >= results.size()
                        || item->data(Qt::UserRole).toString()
                            != entries.at(index).path) {
                        continue;
                    }
                    const auto& result = results.at(index);
                    item->setToolTip(QStringLiteral("%1\n%2").arg(
                        QDir::toNativeSeparators(entries.at(index).path),
                        result.valid
                            ? QStringLiteral("Core-Prüfung: simulierbar")
                            : QStringLiteral("Core-Prüfung: abgelehnt – %1")
                                .arg(result.diagnostic)));
                    item->setText(result.valid
                        ? entries.at(index).name
                        : QStringLiteral("%1  (nur Ansicht)")
                            .arg(entries.at(index).name));
                    if (!result.valid) {
                        item->setForeground(rejectedModelColor);
                    }
                }
            },
            Qt::QueuedConnection);
    });
    catalogThread_ = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread, generation] {
        if (catalogThread_ == thread) {
            catalogThread_ = nullptr;
        }
        if (generation != catalogGeneration_) {
            validateCatalog(scanModelFolder(modelFolder_));
        }
    });
    thread->start();
}

void MainWindow::openModel(const QString& path)
{
    if (simulationThread_ != nullptr) {
        return;
    }
    modelPath_ = path;
    validation_ = {};
    live_.reset();
    states_.clear();
    runtimeInfo_.clear();
    stateSlider_->setRange(0, 0);
    stateLabel_->setText(QStringLiteral("Zustand 0 / 0"));
    forceTable_->setRowCount(0);
    QSettings().setValue(modelSettingKey, samePath(path));
    {
        const QSignalBlocker blocker(modelList_);
        modelList_->setCurrentItem(nullptr);
        for (int row = 0; row < modelList_->count(); ++row) {
            auto* item = modelList_->item(row);
            if (samePath(item->data(Qt::UserRole).toString()) == samePath(path)) {
                modelList_->setCurrentItem(item);
                modelList_->scrollToItem(item);
                break;
            }
        }
    }
    const auto* current = modelList_->currentItem();
    modelName_ = current != nullptr
        ? current->data(Qt::UserRole + 1).toString()
        : QFileInfo(QFileInfo(path).absolutePath()).fileName();
    modelLabel_->setText(modelName_);
    modelLabel_->setToolTip(QDir::toNativeSeparators(path));
    statusLabel_->setText(QStringLiteral("Modellgeometrie wird geladen …"));
    statusLabel_->setToolTip(QString());
    timeLabel_->setText(QStringLiteral("Simulationszeit: –  ·  Rechenzeit: –"));
    setBusy(true);
    statusLabel_->setStyleSheet(QString());

    QString error;
    if (!scene_->setModel(path, &error)) {
        setBusy(false);
        statusLabel_->setText(
            QStringLiteral("Darstellung nicht verfügbar: %1").arg(error));
        statusLabel_->setToolTip(error);
        statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
        return;
    }
    validateSelectedModel(path);
}

void MainWindow::validateSelectedModel(const QString& path)
{
    auto* thread = QThread::create([this, path] {
        const auto result = validateModel(path);
        QMetaObject::invokeMethod(this,
            [this, path, result] {
                if (path != modelPath_) {
                    return;
                }
                validation_ = result;
                setBusy(false);
                statusLabel_->setStyleSheet(QString());
                if (result.valid) {
                    statusLabel_->setText(QStringLiteral("Core-Prüfung erfolgreich"));
                    modelLabel_->setToolTip(QStringLiteral("%1\n%2 · %3")
                            .arg(QDir::toNativeSeparators(path),
                                result.modelId, result.fingerprint));
                } else {
                    const auto code = QRegularExpression(
                        QStringLiteral("CORE-[A-Z]+-\\d+"))
                                          .match(result.diagnostic)
                                          .captured();
                    statusLabel_->setText(QStringLiteral(
                        "Nur Ansicht – vom Core nicht zur Simulation "
                        "freigegeben%1 (Details: Maus über diesen Text)")
                            .arg(code.isEmpty() ? QString()
                                                : QStringLiteral(": ") + code));
                    statusLabel_->setToolTip(result.diagnostic);
                    statusLabel_->setStyleSheet(QStringLiteral("color:#ffbf69;"));
                }
                setBusy(validationThread_ != nullptr
                    && validationThread_->isRunning());
            },
            Qt::QueuedConnection);
    });
    validationThread_ = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread] {
        if (validationThread_ == thread) {
            validationThread_ = nullptr;
        }
        setBusy(simulationThread_ != nullptr
            && simulationThread_->isRunning());
    });
    thread->start();
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
    cancellation_.store(false);
    live_.reset();
    states_.clear();
    runtimeInfo_.clear();
    stateSlider_->setRange(0, 0);
    forceTable_->setRowCount(0);
    statusLabel_->setToolTip(QString());
    statusLabel_->setStyleSheet(QString());
    if (dynamic) {
        startLiveSimulation(duration, step, realtimeCheck_->isChecked());
        return;
    }

    const QString path = modelPath_;
    const auto validation = validation_;
    statusLabel_->setText(QStringLiteral("Statischer Core-Lauf läuft …"));
    timeLabel_->setText(QStringLiteral("Simulationszeit: –  ·  Rechenzeit: läuft"));
    setBusy(true);

    auto* thread = QThread::create([this, path, validation] {
        const auto result = runSimulation(
            path, validation, false, 0.0, 0.01, &cancellation_);
        QMetaObject::invokeMethod(this,
            [this, path, result] {
                if (path != modelPath_) {
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
        setBusy(validationThread_ != nullptr
            && validationThread_->isRunning());
    });
    thread->start();
    cancelButton_->setEnabled(true);
}

void MainWindow::startLiveSimulation(double duration, double step, bool paced)
{
    QString error;
    std::shared_ptr<LiveSimulation> live = LiveSimulation::create(
        modelPath_, validation_, duration, step, paced, &error);
    if (!live) {
        statusLabel_->setText(QStringLiteral("Core-Lauf nicht gestartet"));
        showError(QStringLiteral("Core-Simulation"), error);
        return;
    }
    live_ = live;
    followLive_ = true;
    statusLabel_->setText(paced
        ? QStringLiteral("Dynamik läuft in Echtzeit …")
        : QStringLiteral("Dynamik läuft ungebremst …"));
    setBusy(true);

    auto* thread = QThread::create([this, live] { live->run(cancellation_); });
    simulationThread_ = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread, live] {
        if (simulationThread_ == thread) {
            simulationThread_ = nullptr;
        }
        if (live_ == live) {
            finishLiveSimulation();
        }
        setBusy(validationThread_ != nullptr
            && validationThread_->isRunning());
    });
    thread->start();
    cancelButton_->setEnabled(true);
    liveTimer_->start();
    updateLiveProgress();
}

void MainWindow::updateLiveProgress()
{
    if (!live_) {
        liveTimer_->stop();
        return;
    }
    const auto progress = live_->progress();
    const double load = progress.wallSeconds > 0.0
        ? 100.0 * progress.coreSeconds / progress.wallSeconds
        : 0.0;
    runtimeInfo_ = QStringLiteral("Echtzeit: %1 s  ·  Core-Rechenzeit: %2 (%3 %)")
        .arg(progress.wallSeconds, 0, 'f', 2)
        .arg(runtimeText(progress.coreSeconds))
        .arg(load, 0, 'f', 1);
    updateSliderRange(progress.states);
    if (progress.states == 0) {
        return;
    }
    const auto last = std::min<qsizetype>(
        progress.states - 1, std::numeric_limits<int>::max());
    if (followLive_ && stateSlider_->value() != static_cast<int>(last)) {
        programmaticSlider_ = true;
        stateSlider_->setValue(static_cast<int>(last));
        programmaticSlider_ = false;
    } else {
        displayState(stateSlider_->value());
    }
}

void MainWindow::finishLiveSimulation()
{
    liveTimer_->stop();
    updateLiveProgress();
    const auto progress = live_->progress();
    const QString diagnosis = progress.diagnostic.isEmpty()
        ? QString()
        : QStringLiteral(" · %1").arg(progress.diagnostic);
    statusLabel_->setText(QStringLiteral("%1 · %2 Zustände%3")
            .arg(progress.status)
            .arg(progress.states)
            .arg(diagnosis));
    statusLabel_->setToolTip(progress.diagnostic);
    const bool completed = progress.status == QStringLiteral("abgeschlossen")
        || (progress.status == QStringLiteral("abgebrochen")
            && progress.diagnostic.isEmpty());
    statusLabel_->setStyleSheet(completed
        ? QStringLiteral("color:#b8e7c3;")
        : QStringLiteral("color:#ffbf69;"));
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
            ? QStringLiteral("Reale Core-Gesamtlaufzeit: %1")
                .arg(runtimeText(result.runtimeSeconds))
            : QStringLiteral("Reale Core-Gesamtlaufzeit: nicht verfügbar"));
        showError(QStringLiteral("Core-Simulation"), result.error);
        return;
    }

    states_ = result.states;
    updateSliderRange(states_.size());
    const QString diagnosis = result.diagnostic.isEmpty()
        ? QString()
        : QStringLiteral(" · %1").arg(result.diagnostic);
    statusLabel_->setText(
        QStringLiteral("%1 · %2 Zustände%3")
            .arg(result.status)
            .arg(states_.size())
            .arg(diagnosis));
    statusLabel_->setStyleSheet(result.completed
        ? QStringLiteral("color:#b8e7c3;")
        : QStringLiteral("color:#ffbf69;"));
    if (!states_.isEmpty()) {
        displayState(0);
        scene_->frameStates(states_);
    }
    if (!result.completed) {
        showError(QStringLiteral("Simulation nicht abgeschlossen"),
            result.diagnostic.isEmpty()
                ? result.status
                : QStringLiteral("%1\n%2").arg(result.status, result.diagnostic));
    }
}

void MainWindow::displayState(qsizetype index)
{
    const auto count = stateCount();
    if (index < 0 || index >= count) {
        return;
    }
    QString error;
    const auto state = stateAt(index, &error);
    if (state.isEmpty() || !scene_->setState(state, forcesCheck_->isChecked(), &error)) {
        cancellation_.store(true);
        statusLabel_->setText(QStringLiteral("Darstellung des Zustands fehlgeschlagen"));
        showError(QStringLiteral("Core-Zustand"), error);
        return;
    }
    updateForceTable(state);
    const auto time = state.value(QStringLiteral("zeit_s")).toDouble();
    timeLabel_->setText(
        QStringLiteral("Simulationszeit: %1 s  ·  %2")
            .arg(time, 0, 'f', 3)
            .arg(runtimeInfo_));
    stateLabel_->setText(
        QStringLiteral("Zustand %1 / %2%3")
            .arg(index + 1)
            .arg(count)
            .arg(live_ && simulationThread_ != nullptr
                    ? (followLive_ ? QStringLiteral("  · live")
                                   : QStringLiteral("  · Rückblick"))
                    : QString()));
}

void MainWindow::setBusy(bool busy)
{
    const bool hasModel = validation_.valid && !modelPath_.isEmpty();
    const bool dynamic = modeCombo_->currentData().toBool();
    chooseFolderButton_->setEnabled(!busy);
    modelList_->setEnabled(!busy);
    modeCombo_->setEnabled(!busy && hasModel);
    runButton_->setEnabled(!busy && hasModel);
    cancelButton_->setEnabled(busy && simulationThread_ != nullptr);
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
    setBusy(simulationThread_ != nullptr || validationThread_ != nullptr);
}
void MainWindow::updateForceTable(const QJsonObject& state)
{
    forceTable_->setRowCount(0);
    const auto contributions = state.value(QStringLiteral("vektoren")).toArray();
    for (const auto& value : contributions) {
        const auto item = value.toObject();
        if (item.value(QStringLiteral("art")).toString()
            != QStringLiteral("kraft")) {
            continue;
        }
        const auto row = forceTable_->rowCount();
        forceTable_->insertRow(row);
        auto name = forceName(item.value(QStringLiteral("id")).toString());
        const auto elementId = item.value(QStringLiteral("element_id")).toString();
        if (!elementId.isEmpty()) {
            name = QStringLiteral("%1 · %2").arg(elementId, name);
        }
        const double magnitude =
            vectorMagnitude(item.value(QStringLiteral("vektor")));
        forceTable_->setItem(row, 0, new QTableWidgetItem(name));
        forceTable_->setItem(row, 1, new QTableWidgetItem(
            QStringLiteral("%1 N").arg(magnitude, 0, 'g', 5)));
    }
}

void MainWindow::showError(const QString& title, const QString& message)
{
    statusLabel_->setToolTip(message);
    statusLabel_->setStyleSheet(QStringLiteral("color:#ff8a7a;"));
    QMessageBox::warning(this, title, message);
}
