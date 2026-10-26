#pragma once

#include "core_adapter.hpp"
#include "model_catalog.hpp"

#include <QMainWindow>
#include <QPointer>
#include <QString>

#include <atomic>
#include <memory>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFileSystemWatcher;
class QFormLayout;
class QLabel;
class QListWidget;
class QPushButton;
class QSlider;
class QTableWidget;
class QThread;
class QTimer;
class VtkSceneWidget;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void openModel(const QString& path);
    void setModelFolder(const QString& folder);
    void selectInitialModel(const QString& preferredPath);

private:
    void chooseModelFolder();
    void rescanModelFolder();
    void scheduleFolderRescan(const QString& changedPath);
    void validateCatalog(const QVector<ModelEntry>& entries);
    void validateSelectedModel(const QString& path);
    void startSimulation();
    void startLiveSimulation(double duration, double step, bool paced);
    void updateLiveProgress();
    void finishLiveSimulation();
    void updateSliderRange(qsizetype count);
    qsizetype stateCount() const;
    QJsonObject stateAt(qsizetype index, QString* error) const;
    void displayState(qsizetype index);
    void displaySimulationResult(const SimulationResult& result);
    void setBusy(bool busy);
    void updateModeControls();
    void updateForceTable(const QJsonObject& state);
    void showError(const QString& title, const QString& message);

    VtkSceneWidget* scene_ = nullptr;
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
    QFormLayout* parameterForm_ = nullptr;
    QCheckBox* forcesCheck_ = nullptr;
    QSlider* stateSlider_ = nullptr;
    QLabel* modelLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* timeLabel_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    QTableWidget* forceTable_ = nullptr;

    QString modelPath_;
    ModelValidation validation_;
    QVector<QJsonObject> states_;
    QString runtimeInfo_;
    QString modelFolder_;
    QString modelName_;
    bool reloadCurrentModel_ = false;
    int catalogGeneration_ = 0;
    std::shared_ptr<LiveSimulation> live_;
    bool followLive_ = true;
    bool programmaticSlider_ = false;
    std::atomic_bool cancellation_{false};
    std::atomic_bool shuttingDown_{false};
    QPointer<QThread> validationThread_;
    QPointer<QThread> simulationThread_;
    QPointer<QThread> catalogThread_;
};