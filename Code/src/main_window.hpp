#pragma once

#include "core_adapter.hpp"
#include "display_geometry.hpp"
#include "force_display.hpp"
#include "model_catalog.hpp"
#include "graphics_settings.hpp"

#include <QHash>
#include <QMainWindow>
#include <QPointer>
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

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void setModelFolder(const QString& folder);
    // Modell-ID oder Pfad einer modell.json; wird nach dem Katalogabruf
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
    void chooseModelFolder();
    void reloadCatalog();
    void applyCatalog(const CatalogSnapshot& snapshot);
    void scheduleFolderRescan();
    void applyPendingSelection();
    const ModelEntry* entryFor(const QString& id) const;
    void openModel(const QString& id);
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

    void applyDisplayOptions();
    void setupGraphics();
    void setupMaterialPreview();
    void applyGraphicsProfile();
    void loadSettings();
    void saveDisplaySettings() const;
    void initialReferences(const ModelDisplayData& data);
    void storeReferences() const;
    void updateReferenceLabel();
    void clearValues();
    void showModelProblems(const QVector<ModelProblem>& problems);
    void updateValues();
    void updateDetail();
    void selectContribution(const QString& id);
    void copyValues() const;

    VtkSceneWidget* scene_ = nullptr;
    graphics::Settings graphics_ = graphics::defaults();
    bool graphicsReady_ = false;
    QLabel* graphicsStatus_ = nullptr;
    QPushButton* chooseFolderButton_ = nullptr;
    QListWidget* modelList_ = nullptr;
    QLabel* folderLabel_ = nullptr;
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
    QDoubleSpinBox* testForceSpin_ = nullptr;
    QDoubleSpinBox* testSpeedSpin_ = nullptr;
    QLabel* testDriveStatus_ = nullptr;
    QWidget* testDrivePanel_ = nullptr;
    QGroupBox* movementPanel_ = nullptr;
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
    QComboBox* visibilityCombo_ = nullptr;
    QComboBox* cameraCombo_ = nullptr;
    QCheckBox* navigationCheck_ = nullptr;
    // Je Rubrik (Kräfte, Momente): Gewicht gesamt/Teile, Auftrieb gesamt/Teile, Summe.
    std::array<QCheckBox*, 5> forceChecks_{};
    std::array<QCheckBox*, 5> momentChecks_{};
    QCheckBox* leverCheck_ = nullptr;
    QCheckBox* lateralCheck_ = nullptr;
    QCheckBox* valuesCheck_ = nullptr;
    QPushButton* refitButton_ = nullptr;
    QLabel* referenceLabel_ = nullptr;

    // Wertefenster (A-G-00016, A-G-00017)
    QDockWidget* valuesDock_ = nullptr;
    QLabel* balanceLabel_ = nullptr;
    QLabel* stepInfoLabel_ = nullptr;
    QTreeWidget* valuesTree_ = nullptr;
    QLabel* detailLabel_ = nullptr;
    QTreeWidgetItem* rootItem_ = nullptr;
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
    // Modus des letzten Laufs: Beim Moduswechsel startet der neue Lauf vom
    // angezeigten Zustand statt von den Initialwerten.
    std::optional<bool> lastRunDynamic_;
    QVector<QJsonObject> states_;
    QString runtimeInfo_;
    QString modelFolder_;
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
    QPointer<QThread> simulationThread_;
    QPointer<QThread> catalogThread_;
};
