#include "core_adapter.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>

#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

double number(const QJsonObject& object, const QString& key)
{
    const auto value = object.value(key);
    require(value.isDouble(), "Required numeric field is missing.");
    return value.toDouble();
}

void testValidation(const QString& modelPath, ModelValidation& validation)
{
    validation = validateModel(modelPath);
    require(validation.valid, "Core rejected the reference model.");
    require(!validation.modelId.isEmpty(), "Core returned no model ID.");
    require(!validation.fingerprint.isEmpty(),
        "Core returned no model fingerprint.");

    const auto invalid = validateModel(modelPath + QStringLiteral(".missing"));
    require(!invalid.valid, "Core accepted a missing model file.");
    require(!invalid.diagnostic.isEmpty(),
        "Missing-model error was not surfaced.");
    require(invalid.diagnostic.contains(QStringLiteral("CORE-MOD-0005")),
        "Core error-catalog ID was not surfaced.");
}

void testStaticRun(const QString& modelPath,
    const ModelValidation& validation)
{
    std::atomic_bool cancellation{false};
    const auto result = runSimulation(
        modelPath, validation, false, 0.0, 0.01, &cancellation);
    require(result.completed, "Static Core run did not converge.");
    require(result.status == QStringLiteral("konvergiert"),
        "Static run returned an unexpected status.");
    require(!result.states.isEmpty(), "Static run returned no states.");
    require(std::isfinite(result.runtimeSeconds) && result.runtimeSeconds >= 0.0,
        "Static run returned an invalid real runtime.");

    const auto state = result.states.constLast();
    require(number(state, QStringLiteral("zeit_s")) == 0.0,
        "Static state has a nonzero simulation time.");
    const auto vectors = state.value(QStringLiteral("vektoren")).toArray();
    bool hasWeight = false;
    bool hasBuoyancy = false;
    for (const auto& value : vectors) {
        const auto item = value.toObject();
        if (item.value(QStringLiteral("art")).toString()
            != QStringLiteral("kraft")) {
            continue;
        }
        const auto id = item.value(QStringLiteral("id")).toString();
        hasWeight = hasWeight || id.endsWith(QStringLiteral(":gewicht"));
        hasBuoyancy = hasBuoyancy || id.endsWith(QStringLiteral(":auftrieb"));
    }
    require(hasWeight && hasBuoyancy,
        "Static state omitted weight or buoyancy force contributions.");
    require(number(state, QStringLiteral("volumen_m3")) > 0.0,
        "Static reference model has no displacement volume.");
}

void testDynamicRun(const QString& modelPath,
    const ModelValidation& validation)
{
    std::atomic_bool cancellation{false};
    const auto result = runSimulation(
        modelPath, validation, true, 0.1, 0.01, &cancellation);
    require(result.completed, "Dynamic Core run did not complete.");
    require(result.status == QStringLiteral("abgeschlossen"),
        "Dynamic run returned an unexpected status.");
    require(result.states.size() >= 2,
        "Dynamic run returned too few states.");
    require(std::isfinite(result.runtimeSeconds) && result.runtimeSeconds >= 0.0,
        "Dynamic run returned an invalid real runtime.");

    double previousTime = -1.0;
    for (const auto& state : result.states) {
        const double time = number(state, QStringLiteral("zeit_s"));
        require(time >= previousTime,
            "Dynamic simulation time is not monotonic.");
        previousTime = time;
        require(!state.value(QStringLiteral("vektoren")).toArray().isEmpty(),
            "Dynamic state omitted hydrostatic force contributions.");
    }
    require(std::abs(previousTime - 0.1) < 1e-9,
        "Dynamic run did not reach its requested end time.");
}

void testLiveRun(const QString& modelPath, const ModelValidation& validation)
{
    std::atomic_bool cancellation{false};
    const auto reference = runSimulation(
        modelPath, validation, true, 0.1, 0.01, &cancellation);

    QString error;
    auto live = LiveSimulation::create(
        modelPath, validation, 0.1, 0.01, false, &error);
    require(live != nullptr, "Live simulation could not be created.");
    live->run(cancellation);
    const auto progress = live->progress();
    require(progress.finished, "Live run did not finish.");
    require(progress.status == QStringLiteral("abgeschlossen"),
        "Live run returned an unexpected status.");
    require(progress.states == reference.states.size(),
        "Live run recorded a different number of states.");
    require(progress.coreSeconds >= 0.0
            && progress.coreSeconds <= progress.wallSeconds + 1e-9,
        "Live run reported implausible core or wall time.");

    const auto last = live->stateJson(progress.states - 1, &error);
    require(!last.isEmpty(), "Live state JSON is not available.");
    require(last.value(QStringLiteral("status")).toString()
            == QStringLiteral("abgeschlossen"),
        "Final live state does not carry the final status.");
    const auto expected = reference.states.constLast();
    require(std::abs(number(last, QStringLiteral("zeit_s")) - 0.1) < 1e-9,
        "Live run did not reach its end time.");
    const auto livePosition = last.value(QStringLiteral("position_m")).toArray();
    const auto refPosition = expected.value(QStringLiteral("position_m")).toArray();
    for (int axis = 0; axis < 3; ++axis) {
        require(std::abs(livePosition.at(axis).toDouble()
                    - refPosition.at(axis).toDouble()) < 1e-12,
            "Live run deviates from the file-based Core run.");
    }
    require(live->stateJson(progress.states, &error).isEmpty(),
        "Out-of-range state was accepted.");

    auto paced = LiveSimulation::create(
        modelPath, validation, 0.2, 0.01, true, &error);
    require(paced != nullptr, "Paced live simulation could not be created.");
    const auto start = std::chrono::steady_clock::now();
    paced->run(cancellation);
    const double wall = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    require(wall >= 0.19, "Paced live run ran ahead of real time.");
    require(paced->progress().states == 21, "Paced run lost states.");

    auto endless = LiveSimulation::create(
        modelPath, validation, 0.0, 0.01, true, &error);
    require(endless != nullptr, "Open-ended live simulation was rejected.");
    std::atomic_bool stop{false};
    std::thread worker([&] { endless->run(stop); });
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    stop.store(true);
    worker.join();
    const auto stopped = endless->progress();
    require(stopped.finished && stopped.status == QStringLiteral("abgebrochen"),
        "Open-ended live run did not stop on request.");
    require(stopped.states >= 5 && stopped.simulationSeconds < 0.5,
        "Open-ended paced run did not follow real time.");
}

// Moduswechsel: der neue Lauf übernimmt den angezeigten Zustand.
void testStartState(const QString& modelPath, const ModelValidation& validation)
{
    std::atomic_bool cancellation{false};
    const auto statik = runSimulation(modelPath, validation, false, 0.0, 0.01, &cancellation);
    require(statik.completed, "Static run for start-state test failed.");
    const auto ruhe = statik.states.constLast();

    QString error;
    auto live = LiveSimulation::create(modelPath, validation, 0.05, 0.01, false, &error, ruhe);
    require(live != nullptr, "Live run from static state could not be created.");
    live->run(cancellation);
    const auto first = live->stateJson(0, &error);
    const auto a = first.value(QStringLiteral("position_m")).toArray();
    const auto b = ruhe.value(QStringLiteral("position_m")).toArray();
    for (int axis = 0; axis < 3; ++axis) {
        require(a.at(axis).toDouble() == b.at(axis).toDouble(),
            "Live run did not start from the static state.");
    }
    require(number(first, QStringLiteral("zeit_s")) == 0.0, "Start state kept its old time.");
    const auto last = live->stateJson(live->progress().states - 1, &error);
    require(std::abs(last.value(QStringLiteral("position_m")).toArray().at(2).toDouble()
                - b.at(2).toDouble()) < 1e-3,
        "Live run from equilibrium drifted away.");

    auto moved = ruhe;
    moved.insert(QStringLiteral("geschwindigkeit_welt_m_s"), QJsonArray{0.0, 0.0, 1.0});
    moved.insert(QStringLiteral("geflutete_elemente"), QJsonArray{QStringLiteral("x")});
    const auto again = runSimulation(modelPath, validation, false, 0.0, 0.01, &cancellation, moved);
    require(again.completed, "Static run from a moving state failed.");
    const auto s = again.states.constLast();
    require(s.value(QStringLiteral("geschwindigkeit_welt_m_s")).toArray().at(2).toDouble() == 0.0,
        "Static run kept the start velocity.");
    require(s.value(QStringLiteral("geflutete_elemente")).toArray().size() == 1,
        "Static run lost the flooding state.");

    moved.insert(QStringLiteral("position_m"), QJsonArray{0.0, 0.0});
    require(!LiveSimulation::create(modelPath, validation, 0.05, 0.01, false, &error, moved),
        "Broken start state was accepted.");
}

void testDrive(const QString& modelPath, const ModelValidation& validation)
{
    QString error;
    auto invalid = TestDriveOptions{};
    invalid.speedLimit = 0;
    require(!LiveSimulation::create(modelPath, validation, 0.1, 0.01, false, &error, {}, invalid),
        "invalid test drive accepted");
    error.clear();
    auto live = LiveSimulation::create(modelPath, validation, 0.3, 0.01, true, &error, {},
        TestDriveOptions{false, 100000, 0.01});
    require(live != nullptr, "test drive initialization failed");
    require(live->setTestDriveActive(true, &error), "queued activation rejected");
    std::atomic_bool stop{false};
    std::thread worker([&] { live->run(stop); });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (live->progress().states < 6 && !live->progress().finished
        && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const auto historical = live->stateJson(2, &error);
    const bool switched = live->setTestDriveActive(false, &error);
    worker.join();
    require(switched && !historical.isEmpty(), "live test switch failed");
    const auto progress = live->progress();
    require(progress.status == QStringLiteral("abgeschlossen"), "test drive run failed");
    require(live->stateJson(2, &error) == historical, "later flag changed historical state");
    require(historical.value(QStringLiteral("vertragsversion")).toString() == QStringLiteral("3")
        && historical.value(QStringLiteral("testantrieb")).isObject(), "V3 test context missing");
    const auto speed = historical.value(QStringLiteral("geschwindigkeit_welt_m_s")).toArray();
    require(std::hypot(speed[0].toDouble(), speed[1].toDouble()) <= 0.0100000001,
        "active live cap not enforced");
    const auto last = live->stateJson(progress.states - 1, &error);
    bool disabled = false;
    for (const auto& value : last.value(QStringLiteral("vektoren")).toArray()) {
        const auto c = value.toObject();
        if (c.value(QStringLiteral("quelle")).toString() == QStringLiteral("testantrieb")) {
            disabled = c.value(QStringLiteral("zustand")).toString() == QStringLiteral("deaktiviert");
        }
    }
    require(disabled, "live switch did not disable test force");
    require(!progress.testDriveStatus.isEmpty(), "effective switch confirmation absent");
    require(!live->setTestDriveActive(true, &error), "event accepted after end");
    error.clear();
    const auto motion = live->motionStates(0, progress.states, &error);
    require(motion.size() == progress.states && error.isEmpty(), "raw motion states incomplete");
    require(motion[2].value(QStringLiteral("position_m")) == historical.value(QStringLiteral("position_m")),
        "lightweight history does not match actual physics");
}

void testMovementInput(const QString& referenceModelPath)
{
    const auto modelPath = QDir(QFileInfo(referenceModelPath).absolutePath())
        .filePath(QStringLiteral("../420/modell.json"));
    const auto validation = validateModel(modelPath);
    require(validation.valid, "420 movement model was rejected by Core.");

    QString error;
    auto live = LiveSimulation::create(modelPath, validation, 0.5, 0.01, true, &error);
    require(live != nullptr, "Movement simulation could not be created.");
    std::atomic_bool cancellation{false};
    std::thread worker([&] { live->run(cancellation); });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (live->progress().states < 2 && !live->progress().finished
        && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    bool invalidRejected = false;
    bool rudderAccepted = false;
    bool swordAccepted = false;
    if (!live->progress().finished) {
        invalidRejected = !live->setMovementValue(QStringLiteral("Ruder"), 91.0, &error);
        error.clear();
        rudderAccepted = live->setMovementValue(QStringLiteral("Ruder"), 20.0, &error);
        swordAccepted = live->setMovementValue(QStringLiteral("Schwert"), 50.0, &error);
    }
    worker.join();
    require(invalidRejected, "Out-of-range rudder input was accepted.");
    require(rudderAccepted, "Valid rudder input was rejected.");
    require(swordAccepted, "Valid sword input was rejected.");
    const auto progress = live->progress();
    require(progress.status == QStringLiteral("abgeschlossen"),
        "Movement simulation did not complete.");
    const auto result = live->stateJson(progress.states - 1, &error);
    require(result.value(QStringLiteral("vertragsversion")).toString() == QStringLiteral("4"),
        "Movement simulation did not return result contract V4.");
    require(result.value(QStringLiteral("letzte_bewegungsereigniskennung")).toDouble() == 2.0,
        "Core did not acknowledge both movement events.");

    bool rudderConfirmed = false;
    bool swordConfirmed = false;
    for (const auto& value : result.value(QStringLiteral("bewegungen")).toArray()) {
        const auto movement = value.toObject();
        const auto id = movement.value(QStringLiteral("element_id")).toString();
        const double setting = movement.value(QStringLiteral("wert")).toDouble();
        rudderConfirmed = rudderConfirmed || (id == QStringLiteral("Ruder") && setting == 20.0);
        swordConfirmed = swordConfirmed || (id == QStringLiteral("Schwert") && setting == 50.0);
    }
    require(rudderConfirmed && swordConfirmed,
        "Core did not confirm the requested rudder and sword settings.");
    require(!result.value(QStringLiteral("elementtransformationen_modell")).toArray().isEmpty(),
        "Core omitted movement poses needed for rendering.");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (argc != 2) {
        std::cerr << "Expected the Core reference-model path.\n";
        return 2;
    }
    try {
        const auto modelPath = QString::fromLocal8Bit(argv[1]);
        ModelValidation validation;
        testValidation(modelPath, validation);
        testStaticRun(modelPath, validation);
        testDynamicRun(modelPath, validation);
        testLiveRun(modelPath, validation);
        testStartState(modelPath, validation);
        testDrive(modelPath, validation);
        testMovementInput(modelPath);
        std::cout << "Core adapter validation, static hydrostatics and "
                     "dynamic run passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
