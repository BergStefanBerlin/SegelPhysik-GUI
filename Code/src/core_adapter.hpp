#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

#include <atomic>
#include <memory>
#include <optional>

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
// start: Core-Ergebniszustand, von dem der Lauf ausgeht (Pose,
// Geschwindigkeiten, geflutete Elemente; Zeit beginnt neu bei 0).
// Leer bedeutet Initialwerte des Core. Die Statik verwirft Geschwindigkeiten.
SimulationResult runSimulation(const QString& modelPath,
    const ModelValidation& validation, bool dynamic, double durationSeconds,
    double stepSeconds, std::atomic_bool* cancellation,
    const QJsonObject& start = {});

// Dynamischer Lauf ohne Vorausrechnung: run() rechnet im Worker-Thread
// Schritt für Schritt und legt nur Rohzustände ab. Die GUI liest jederzeit
// den Fortschritt und erzeugt Ergebnis-JSON nur für angezeigte Zustände.
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
        QString testDriveStatus;
    };

    // durationSeconds == 0 bedeutet: laufen bis zum Abbruch.
    static std::unique_ptr<LiveSimulation> create(const QString& modelPath,
        const ModelValidation& validation, double durationSeconds,
        double stepSeconds, bool realtimePacing, QString* error,
        const QJsonObject& start = {}, const std::optional<TestDriveOptions>& testDrive = std::nullopt);
    ~LiveSimulation();
    LiveSimulation(const LiveSimulation&) = delete;
    LiveSimulation& operator=(const LiveSimulation&) = delete;

    void run(const std::atomic_bool& cancellation);
    Progress progress() const;
    QJsonObject stateJson(qsizetype index, QString* error) const;
    QVector<QJsonObject> motionStates(qsizetype begin, qsizetype end, QString* error) const;
    bool setTestDriveActive(bool active, QString* error);
    bool setMovementValue(const QString& elementId, double value, QString* error);

    static constexpr qsizetype maximumStates = 2'000'000;

private:
    LiveSimulation();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
