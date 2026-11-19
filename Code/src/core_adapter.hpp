#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

#include <atomic>
#include <array>
#include <memory>
#include <optional>

namespace segelphysik { class ModelInspection; }

struct TestDriveOptions {
    bool active = false;
    double forceN = 200.0;
    double speedLimit = 463.0 / 90.0;
};

struct ModelValidation {
    bool valid = false;
    QString modelId;
    QString fingerprint;
    QString diagnostic;
};

struct SimulationResult {
    bool completed = false;
    QString status;
    QString diagnostic;
    QString error;
    bool runtimeAvailable = false;
    double runtimeSeconds = 0.0;
    QVector<QJsonObject> states;
};

ModelValidation validateModel(const QString& modelPath);
QString validateSailforceInput(const QString& modelPath, const QJsonObject& input,
    std::shared_ptr<const segelphysik::ModelInspection> inspection);
// start: Core-Ergebniszustand, von dem der Lauf ausgeht (Pose,
// Geschwindigkeiten, geflutete Elemente; Zeit beginnt neu bei 0).
// Leer bedeutet Initialwerte des Core. Die Hydrostatik verwirft Geschwindigkeiten.
// start.stationaer: eingefrorene Fremdkraft und Elementstellungen fuer den
// stationaeren Core-Vertrag; vorhandene Geschwindigkeiten sind nur Startwerte.
SimulationResult runSimulation(const QString& modelPath,
    const ModelValidation& validation, bool dynamic, double durationSeconds,
    double stepSeconds, std::atomic_bool* cancellation,
    const QJsonObject& start = {},
    std::shared_ptr<const segelphysik::ModelInspection> inspection = {});

// Dynamischer Lauf ohne Vorausrechnung: run() rechnet im Worker-Thread
// Schritt für Schritt und legt Rohzustände sowie maximal 64 Ergebnis-Snapshots ab.
// Die GUI liest den Fortschritt und erzeugt JSON nur für angezeigte Zustände.
// Worker und Anzeige besitzen getrennte langlebige Core-Aufbereitungen;
// historische JSON-Anfragen blockieren keinen Integrationsschritt.
class LiveSimulation {
public:
    struct Progress {
        qsizetype states = 0;
        bool finished = false;
        QString status;
        QString diagnostic;
        double simulationSeconds = 0.0;
        double wallSeconds = 0.0;
        double coreSeconds = 0.0;
        double physicsStepSeconds = 0.0;
        double pacingSeconds = 0.0;
        double historyMutexWaitSeconds = 0.0;
        double workerCpuSeconds = -1.;
        quint64 workerCpuCycles = 0;
        double snapshotCaptureSeconds = 0.0;
        double snapshotCaptureMaximumMs = 0.0;
        quint64 snapshotCaptures = 0;
        quint64 snapshotReaderHits = 0;
        quint64 snapshotReaderMisses = 0;
        QString testDriveStatus;
        quint64 sailforceRevision = 0;
    };

    // durationSeconds == 0 bedeutet: laufen bis zum Abbruch.
    static std::unique_ptr<LiveSimulation> create(const QString& modelPath,
        const ModelValidation& validation, double durationSeconds,
        double stepSeconds, bool realtimePacing, QString* error,
        const QJsonObject& start = {}, const std::optional<TestDriveOptions>& testDrive = std::nullopt,
        std::shared_ptr<const segelphysik::ModelInspection> inspection = {});
    ~LiveSimulation();
    LiveSimulation(const LiveSimulation&) = delete;
    LiveSimulation& operator=(const LiveSimulation&) = delete;

    void run(const std::atomic_bool& cancellation);
    Progress progress() const;
    QJsonObject initialStateJson() const;
    struct JsonTimings {
        bool snapshotHit = false;
        double snapshotCaptureMs = 0.;
        double coreMs = 0.;
        double decodeMs = 0.;
        double motionMs = 0.;
        double motionReadMs = 0.;
        double motionPrepareMs = 0.;
        qsizetype motionStates = 0;
        double forceParseMs = 0.;
        double fetchMs = 0.;
        double guiWaitMs = 0.;
        qsizetype bytes = 0;
    };
    QJsonObject stateJson(qsizetype index, QString* error, JsonTimings* timings = nullptr) const;
    QVector<QJsonObject> motionStates(qsizetype begin, qsizetype end, QString* error) const;
    bool setTestDriveActive(bool active, QString* error);
    bool setMovementValue(const QString& elementId, double value, QString* error);
    bool setDynamicForce(bool active, const std::array<double, 3>& force,
        const std::array<double, 3>& point, QString* error);
    bool setWind(std::optional<double> direction, std::optional<double> speed,
        std::optional<bool> boatFixed, QString* error);
    bool setSailAngle(const QString& elementId, double angle, QString* error);

    static constexpr qsizetype maximumStates = 2'000'000;

private:
    LiveSimulation();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
