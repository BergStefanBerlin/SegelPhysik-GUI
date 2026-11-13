#pragma once

#include "core_adapter.hpp"
#include "display_geometry.hpp"
#include "force_display.hpp"
#include "model_catalog.hpp"
#include "graphics_settings.hpp"

#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QPointer>
#include <QSet>
#include <QString>

#include <array>
#include <atomic>
#include <memory>
#include <optional>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QDockWidget;
class QDoubleSpinBox;
class QFileSystemWatcher;
class QFormLayout;
class QGroupBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSlider;
class QThread;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class VtkSceneWidget;
class BoatInspection;
class QStackedWidget;
class QHBoxLayout;
class QVBoxLayout;
class QScrollArea;
class QLineEdit;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void setModelFolder(const QString& folder);
    // Modell-ID oder Pfad einer modells_information.json; wird nach dem Katalogabruf
    // ausgewertet. Leer: zuletzt gewähltes oder erstes verwendbares Modell.
    void selectInitialModel(const QString& preferred);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    struct OpenedModel {
        ModelDisplayData data;
        QVector<DisplayPart> parts;
    };

    QWidget* buildDisplayPanel();
    QDockWidget* buildValuesDock();
    void applyWorkspaceTheme(int index);
    void updateWorkspace(int index);
    void swapStaticView(VtkSceneWidget* view);
    void updateSceneActivity(bool synchronize = true);
    void chooseModelFolder();
    void reloadCatalog();
    void updateVisualizationFingerprint(const QString& modelId);
    void applyCatalog(const CatalogSnapshot& snapshot);
    void scheduleFolderRescan();
    void applyPendingSelection();
    const ModelEntry* entryFor(const QString& id) const;
    void openModel(const QString& id);
    void modelDisplayOpened(const ModelEntry& entry,
        const std::shared_ptr<OpenedModel>& opened);
    void modelOpened(const ModelEntry& entry, const std::shared_ptr<OpenedModel>& opened,
        const ModelValidation& validation);
    void startSimulation();
    void startLiveSimulation(double duration, double step, bool paced, const QJsonObject& start);
    void updateLiveProgress();
    void finishLiveSimulation();
    void simulationFinished();
    void updateSliderRange(qsizetype count);
    qsizetype stateCount() const;
    QJsonObject stateAt(qsizetype index, QString* error) const;
    void displayState(qsizetype index, bool force = false);
    void displaySimulationResult(const SimulationResult& result);
    void resetResults();
    void setBusy(bool busy);
    void updateModeControls();
    void configureMovementControls(const ModelDisplayData& data);
    void updateMovementControls(const QJsonObject& state);
    void showError(const QString& title, const QString& message);

    void applyDisplayOptions(bool restoreHistory = true);
    void updateStaticForceMenu();
    void setupGraphics();
    void closeGraphics();
    void setupMaterialPreview();
    void applyGraphicsProfile(bool restoreHistory = true);
    void loadSettings();
    void saveDisplaySettings();
    void restoreModeDisplaySettings();
    void initialReferences(const ModelDisplayData& data);
    void storeReferences() const;
    void updateReferenceLabel();
    void clearValues();
    void showModelProblems(const QVector<ModelProblem>& problems);
    void updateValues();
    void updateStaticValues();
    void filterStaticValues();
    void updateDetail();
    void selectContribution(const QString& id);
    void copyValues() const;

    VtkSceneWidget* scene_ = nullptr;
    std::array<VtkSceneWidget*, 3> staticScenes_{};
    std::array<QWidget*, 3> staticViewSlots_{};
    VtkSceneWidget* largeScene_ = nullptr;
    QWidget* staticViewsColumn_ = nullptr;
    QList<QWidget*> vectorControls_;
    QWidget* staticForceMenu_ = nullptr;
    std::array<QCheckBox*, forcedisplay::staticForceCount> staticForceChecks_{};
    std::array<QPushButton*, forcedisplay::staticForceCount> staticColorButtons_{};
    std::array<forcedisplay::StaticForceStyle, forcedisplay::staticForceCount> staticForceStyles_ =
        forcedisplay::staticForceDefaults();
    QHash<QString, QString> hydrodynamicRoles_;
    BoatInspection* inspection_ = nullptr;
    QStackedWidget* workspaceStack_ = nullptr;
    QStackedWidget* sceneStack_ = nullptr;
    QWidget* graphicsPage_ = nullptr;
    QStackedWidget* valuesStack_ = nullptr;
    QWidget* playbackPanel_ = nullptr;
    bool simulationValuesVisible_ = true;
    int workspaceIndex_ = -1;
    int simulationGeneration_ = 0;
    QWidget* catalogPanel_ = nullptr;
    QHBoxLayout* mainArea_ = nullptr;
    QWidget* displayPanel_ = nullptr;
    QScrollArea* staticDisplaySidebar_ = nullptr;
    QScrollArea* staticNavigationColumn_ = nullptr;
    QVBoxLayout* simulationControlsLayout_ = nullptr;
    graphics::Settings graphics_ = graphics::defaults();
    graphics::Settings staticGraphics_ = graphics::defaults();
    bool graphicsReady_ = false;
    QLabel* graphicsStatus_ = nullptr;
    QPushButton* chooseFolderButton_ = nullptr;
    QListWidget* modelList_ = nullptr;
    QLabel* folderLabel_ = nullptr;
    QComboBox* inspectionCameraCombo_ = nullptr;
    QCheckBox* realtimeCheck_ = nullptr;
    QFileSystemWatcher* folderWatcher_ = nullptr;
    QTimer* rescanTimer_ = nullptr;
    QTimer* liveTimer_ = nullptr;
    QPushButton* runButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
    QComboBox* modeCombo_ = nullptr;
    QDoubleSpinBox* durationSpin_ = nullptr;
    QDoubleSpinBox* stepSpin_ = nullptr;
    QCheckBox* testDriveCheck_ = nullptr;
    QComboBox* cameraFrameCombo_ = nullptr;
    QWidget* cameraFramePanel_ = nullptr;
    QDoubleSpinBox* testForceSpin_ = nullptr;
    QDoubleSpinBox* testSpeedSpin_ = nullptr;
    QLabel* testDriveStatus_ = nullptr;
    QWidget* testDrivePanel_ = nullptr;
    QGroupBox* movementPanel_ = nullptr;
    QGroupBox* stationaryForcePanel_ = nullptr;
    QGroupBox* windPanel_ = nullptr;
    QLineEdit* windDirectionEdit_ = nullptr;
    QLineEdit* windSpeedEdit_ = nullptr;
    QLineEdit* windHeightEdit_ = nullptr;
    QComboBox* windMode_ = nullptr;
    QLabel* windStatus_ = nullptr;
    double windDirection_ = 0.;
    double windSpeed_ = 5.;
    double windHeight_ = 10.;
    QCheckBox* stationaryForceActive_ = nullptr;
    std::array<QDoubleSpinBox*, 3> stationaryForce_{};
    std::array<QDoubleSpinBox*, 3> stationaryPoint_{};
    QHash<QString, QDoubleSpinBox*> stationaryMovements_;
    QFormLayout* movementLayout_ = nullptr;
    QHash<QString, QSlider*> movementSliders_;
    QHash<QString, QLabel*> movementValueLabels_;
    QHash<QString, bool> movementIsRudder_;
    QHash<QString, double> movementConfirmedValues_;
    QHash<QString, bool> movementInputPending_;
    double testSpeedLimit_ = 463.0 / 90.0;
    QFormLayout* parameterForm_ = nullptr;
    QSlider* stateSlider_ = nullptr;
    QLabel* modelLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* timeLabel_ = nullptr;
    QLabel* stateLabel_ = nullptr;

    // Darstellung (A-G-00011, A-G-00014, A-G-00018)
    QCheckBox* forcesCheck_ = nullptr;
    QComboBox* forceScaleCombo_ = nullptr;
    QComboBox* momentScaleCombo_ = nullptr;
    QPushButton* graphicsButton_ = nullptr;
    QWidget* dynamicMotionPanel_ = nullptr;
    std::array<QCheckBox*, 4> dynamicMotionChecks_{};
    QComboBox* cameraCombo_ = nullptr;
    QCheckBox* navigationCheck_ = nullptr;
    QCheckBox* attitudeViewsCheck_ = nullptr;
    // Je Rubrik (Kräfte, Momente): Gewicht gesamt/Teile, Auftrieb gesamt/Teile, Summe.
    std::array<QCheckBox*, 5> forceChecks_{};
    std::array<QCheckBox*, 5> momentChecks_{};
    QCheckBox* leverCheck_ = nullptr;
    QCheckBox* lateralCheck_ = nullptr;
    QCheckBox* sailCenterCheck_ = nullptr;
    QCheckBox* windArrowsCheck_ = nullptr;
    QLabel* pressureStatus_ = nullptr;
    QCheckBox* valuesCheck_ = nullptr;
    QPushButton* refitButton_ = nullptr;
    QLabel* referenceLabel_ = nullptr;

    // Wertefenster (A-G-00016, A-G-00017)
    QDockWidget* valuesDock_ = nullptr;
    QLabel* balanceLabel_ = nullptr;
    QLabel* stepInfoLabel_ = nullptr;
    QTreeWidget* valuesTree_ = nullptr;
    QWidget* legacyValues_ = nullptr;
    QTreeWidget* staticValues_ = nullptr;
    std::array<QLineEdit*, 10> staticValueFilters_{};
    QLabel* detailLabel_ = nullptr;
    QTreeWidgetItem* rootItem_ = nullptr;
    QTreeWidgetItem* stationaryItem_ = nullptr;
    QTreeWidgetItem* courseRateItem_ = nullptr;
    QHash<QString, QTreeWidgetItem*> groupItems_;
    QHash<QString, QTreeWidgetItem*> contributionItems_;
    forcedisplay::ForceState currentForces_;
    bool hasForces_ = false;
    // Modellfehler im Wertefenster, solange keine Kraftrechnung möglich ist.
    QVector<ModelProblem> problems_;

    CatalogSnapshot catalog_;
    QHash<QString, std::shared_ptr<OpenedModel>> modelCache_;
    QString modelId_;
    QString modelVersion_;
    QString modelPath_;
    QString modelName_;
    ModelValidation validation_;
    // Modus des letzten Laufs fuer die Kennzeichnung veralteter Eingaben.
    std::optional<bool> lastRunDynamic_;
    QVector<QJsonObject> states_;
    QString runtimeInfo_;
    QString modelFolder_;
    bool useConfiguredModelFolder_ = false;
    bool defaultsReloadPending_ = false;
    bool fingerprintWatcherSuppressed_ = false;
    QString pendingSelection_;
    bool selectionPending_ = false;
    bool reopenAfterCatalog_ = false;
    bool catalogReloadPending_ = false;
    int openGeneration_ = 0;
    qsizetype displayedIndex_ = -1;
    std::shared_ptr<LiveSimulation> live_;
    bool followLive_ = true;
    bool programmaticSlider_ = false;
    std::atomic_bool cancellation_{false};
    std::atomic_bool shuttingDown_{false};
    QPointer<QThread> openThread_;
    QSet<QThread*> openThreads_;
    QPointer<QThread> simulationThread_;
    QPointer<QThread> catalogThread_;
    QPointer<QThread> fingerprintThread_;
};
