#include "core_adapter.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

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

void testStationaryRun(const QString& modelPath, const ModelValidation& validation)
{
    std::atomic_bool cancellation{false};
    QJsonObject configuration{
        {QStringLiteral("aktiv"), false},
        {QStringLiteral("modellfest"), false},
        {QStringLiteral("kraft_n"), QJsonArray{0.,0.,0.}},
        {QStringLiteral("angriffspunkt_modell_m"), QJsonArray{0.,0.,0.}},
        {QStringLiteral("stellungen"), QJsonArray{}}};
    const auto result = runSimulation(modelPath, validation, false, 0., .01,
        &cancellation, QJsonObject{{QStringLiteral("stationaer"), configuration}});
    require(result.error.isEmpty() && result.completed && !result.states.isEmpty(),
        "Stationary zero-load reference did not converge through GUI adapter.");
    const auto state = result.states.constLast();
    require(state.value(QStringLiteral("stationaer")).isObject(),
        "Stationary result omitted its explicit Core contract.");
    const auto stationary = state.value(QStringLiteral("stationaer")).toObject();
    const auto checkResidual = [](const QJsonObject& context, const QString& key) {
        const auto residual = context.value(key).toArray();
        require(residual.size() == 3, "Stationary residual must have exactly three components.");
        for (const auto& value : residual)
            require(value.isDouble() && std::isfinite(value.toDouble())
                    && std::abs(value.toDouble()) <= .1,
                "Stationary component exceeded physical acceptance threshold.");
    };
    checkResidual(stationary, QStringLiteral("residuum_kraft_welt_n"));
    checkResidual(stationary, QStringLiteral("residuum_moment_modell_nm"));
    require(stationary.value(QStringLiteral("referenzkurs_verwendet")).toBool(),
        "Zero-load free yaw must be marked as a reference, not a solved absolute course.");
    require(number(stationary, QStringLiteral("krafttoleranz_n")) <= .1
            && number(stationary, QStringLiteral("momenttoleranz_nm")) <= .1,
        "Core tolerances exceed agreed upper bounds.");
    std::cout << "Stationary zero-load evidence: "
              << QJsonDocument(state.value(QStringLiteral("stationaer")).toObject())
                     .toJson(QJsonDocument::Compact).constData() << '\n';
    auto modelFrame = configuration;
    modelFrame.insert(QStringLiteral("modellfest"), true);
    modelFrame.insert(QStringLiteral("aktiv"), true);
    modelFrame.insert(QStringLiteral("angriffspunkt_modell_m"), QJsonArray{1.,2.,3.});
    const auto framed = runSimulation(modelPath, validation, false, 0., .01,
        &cancellation, QJsonObject{{QStringLiteral("stationaer"), modelFrame}});
    require(framed.completed && framed.error.isEmpty() && !framed.states.isEmpty(),
        "Model-fixed zero-load run was rejected.");
    const auto framedContext = framed.states.constLast().value(QStringLiteral("stationaer")).toObject();
    require(framedContext.value(QStringLiteral("modellfest")).toBool()
            && framedContext.value(QStringLiteral("aktiv")).toBool()
            && framedContext.value(QStringLiteral("angriffspunkt_modell_m"))
                == modelFrame.value(QStringLiteral("angriffspunkt_modell_m"))
            && !stationary.value(QStringLiteral("modellfest")).toBool()
            && !stationary.value(QStringLiteral("aktiv")).toBool(),
        "Force reference/application point or immutable previous snapshot lost.");
    checkResidual(framedContext, QStringLiteral("residuum_kraft_welt_n"));
    checkResidual(framedContext, QStringLiteral("residuum_moment_modell_nm"));
    const auto movementPath = QDir(QFileInfo(modelPath).absolutePath())
        .filePath(QStringLiteral("../420/modell.json"));
    const auto movementValidation = validateModel(movementPath);
    require(movementValidation.valid, "Stationary movement reference model is invalid.");
    for (bool active : {false, true}) {
        auto reference = configuration;
        reference.insert(QStringLiteral("aktiv"), active);
        reference.insert(QStringLiteral("modellfest"), true);
        reference.insert(QStringLiteral("kraft_n"), QJsonArray{100., 0., 0.});
        reference.insert(QStringLiteral("angriffspunkt_modell_m"), QJsonArray{1., 0., 0.});
        reference.insert(QStringLiteral("stellungen"), QJsonArray{
            QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Ruder")}, {QStringLiteral("wert"), 0.}},
            QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Schwert")}, {QStringLiteral("wert"), 100.}}});
        const auto run = runSimulation(movementPath, movementValidation, false, 0., .01,
            &cancellation, QJsonObject{{QStringLiteral("stationaer"), reference}});
        std::cout << "420 reference active=" << active << " runtime=" << run.runtimeSeconds
                  << " status=" << run.status.toStdString() << " error=" << run.error.toStdString()
                  << " result=" << (run.states.isEmpty() ? QByteArray{} :
                    QJsonDocument(run.states.constLast().value(QStringLiteral("stationaer")).toObject())
                        .toJson(QJsonDocument::Compact)).constData() << '\n';
        require(run.error.isEmpty() && !run.states.isEmpty(),
            "420 screenshot reference failed without a physical result.");
        const auto context = run.states.constLast().value(QStringLiteral("stationaer")).toObject();
        if (active) {
            auto trial = reference;
            trial.insert(QStringLiteral("stellungen"), QJsonArray{
                QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Ruder")}, {QStringLiteral("wert"), .0114}},
                QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Schwert")}, {QStringLiteral("wert"), 100.}}});
            const auto steered = runSimulation(movementPath, movementValidation, false, 0., .01,
                &cancellation, QJsonObject{{QStringLiteral("stationaer"), trial}});
            require(steered.completed && !steered.states.isEmpty(),
                "Independently balanced 420 rudder reference must converge.");
            const auto balanced = steered.states.constLast().value(QStringLiteral("stationaer")).toObject();
            checkResidual(balanced, QStringLiteral("residuum_kraft_welt_n"));
            checkResidual(balanced, QStringLiteral("residuum_moment_modell_nm"));
            std::cout << "420 steering diagnostic 0.0114: "
                      << QJsonDocument(balanced).toJson(QJsonDocument::Compact).constData() << '\n';
        }
        require(context.value(QStringLiteral("referenzkurs_verwendet")).toBool(),
            "Model-fixed force must not determine an absolute heading.");
        if (!active) {
            require(run.completed, "Unloaded 420 reference must converge.");
        } else {
            require(context.value(QStringLiteral("iterationen")).toInt() <= 20,
                "Free-heading 420 case wasted its iteration budget.");
            require(std::abs(number(context, QStringLiteral("geschwindigkeit_relativ_wasser_betrag_m_s"))
                    - 1.9871) <= .1,
                "420 trial speed moved outside the observed 0.1 m/s band.");
            require(run.completed || (context.value(QStringLiteral("diagnose")).toString()
                    .contains(QStringLiteral("stagniert"))
                    && !context.value(QStringLiteral("konvergiert")).toBool()),
                "420 unsatisfied balance must remain explicitly non-converged.");
        }
    }
    auto movedConfiguration = configuration;
    const QJsonArray settings{
        QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Ruder")}, {QStringLiteral("wert"), 20.}},
        QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Schwert")}, {QStringLiteral("wert"), 50.}}};
    movedConfiguration.insert(QStringLiteral("stellungen"), settings);
    const auto moved = runSimulation(movementPath, movementValidation, false, 0., .01,
        &cancellation, QJsonObject{{QStringLiteral("stationaer"), movedConfiguration}});
    require(moved.completed && moved.error.isEmpty() && !moved.states.isEmpty(),
        "Fixed rudder/centreboard equilibrium failed through adapter.");
    const auto movedContext = moved.states.constLast().value(QStringLiteral("stationaer")).toObject();
    const auto confirmed = movedContext.value(QStringLiteral("stellungen")).toArray();
    require(confirmed.size() == settings.size(), "Fixed movement snapshot lost elements.");
    for (qsizetype i = 0; i < settings.size(); ++i)
        require(confirmed.at(i).toObject().value(QStringLiteral("element_id"))
                    == settings.at(i).toObject().value(QStringLiteral("element_id"))
                && confirmed.at(i).toObject().value(QStringLiteral("wert"))
                    == settings.at(i).toObject().value(QStringLiteral("wert")),
            "Fixed rudder/centreboard input was not preserved.");
    checkResidual(movedContext, QStringLiteral("residuum_kraft_welt_n"));
    checkResidual(movedContext, QStringLiteral("residuum_moment_modell_nm"));
    require(!moved.states.constLast().value(QStringLiteral("elementtransformationen_modell")).toArray().isEmpty(),
        "Stationary moved geometry not supplied for rendering.");
    auto invalid = configuration;
    for (double rudder : {0., 1., -1., 24., -24.}) {
        auto turning = configuration;
        turning.insert(QStringLiteral("kurvenfahrt"), true);
        turning.insert(QStringLiteral("aktiv"), true);
        turning.insert(QStringLiteral("modellfest"), true);
        turning.insert(QStringLiteral("kraft_n"), QJsonArray{100.,0.,0.});
        turning.insert(QStringLiteral("angriffspunkt_modell_m"), QJsonArray{1.,0.,0.});
        if (std::abs(rudder) > 1.) {
            turning.insert(QStringLiteral("kraft_n"), QJsonArray{150.,10.,0.});
            turning.insert(QStringLiteral("angriffspunkt_modell_m"), QJsonArray{-.05,0.,3.});
        }
        turning.insert(QStringLiteral("stellungen"), QJsonArray{
            QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Ruder")},
                {QStringLiteral("wert"), rudder}},
            QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Schwert")},
                {QStringLiteral("wert"), 100.}}});
        const auto circle = runSimulation(movementPath, movementValidation, false, 0., .01,
            &cancellation, QJsonObject{{QStringLiteral("stationaer"), turning}});
        require(circle.completed && circle.error.isEmpty() && !circle.states.isEmpty(),
            "Real 420 turning run from rest must converge without changing fixed settings.");
        const auto output = circle.states.constLast();
        const auto pressure = output.value(QStringLiteral("druckpunkte")).toObject();
        const auto lateral = pressure.value(QStringLiteral("lateral")).toObject();
        const auto replacement = pressure.value(QStringLiteral("ersatz_segel")).toObject();
        require(lateral.value(QStringLiteral("auswertung_version")).toInt() == 2
                && replacement.value(QStringLiteral("status")).toString() == QStringLiteral("gueltig")
                && replacement.value(QStringLiteral("quelle")).toString() == QStringLiteral("fremdkraft")
                && replacement.value(QStringLiteral("position_modell_m"))
                    == turning.value(QStringLiteral("angriffspunkt_modell_m")),
            "Real Core turning output must deliver corrected LDP and frozen replacement SDP.");
        const auto context = output.value(QStringLiteral("stationaer")).toObject();
        checkResidual(context, QStringLiteral("residuum_kraft_welt_n"));
        checkResidual(context, QStringLiteral("residuum_moment_modell_nm"));
        const double rate = number(output, QStringLiteral("kursdrehrate_rad_s"));
        require(context.value(QStringLiteral("kurvenfahrt")).toBool()
                && context.value(QStringLiteral("kraft_n")) == turning.value(QStringLiteral("kraft_n"))
                && context.value(QStringLiteral("angriffspunkt_modell_m"))
                    == turning.value(QStringLiteral("angriffspunkt_modell_m"))
                && context.value(QStringLiteral("stellungen")) == turning.value(QStringLiteral("stellungen")),
            "Turning run must preserve its frozen force and rudder/centreboard settings.");
        if (rudder != 0.)
            require(rate * rudder < 0., "Opposite fixed rudders must produce opposite signed course rates.");
        if (rudder == 24.)
            require(std::abs(rate + .30153) < .001
                    && std::abs(number(context, QStringLiteral("geschwindigkeit_relativ_wasser_betrag_m_s"))
                        - 2.1244) < .01,
                "Screenshot case must reach the independently found turning branch.");
        require(context.value(QStringLiteral("iterationen")).toInt() < 30,
            "Real 420 reference must converge without exhausting the 100-iteration budget.");
        std::cout << "420 turning from rest rudder=" << rudder
                  << " rate_rad_s=" << rate << " runtime=" << circle.runtimeSeconds
                  << " iterations=" << context.value(QStringLiteral("iterationen")).toInt() << '\n';
    }
    invalid.insert(QStringLiteral("kraft_n"), QJsonArray{0.,0.});
    const auto rejected = runSimulation(modelPath, validation, false, 0., .01,
        &cancellation, QJsonObject{{QStringLiteral("stationaer"), invalid}});
    require(!rejected.error.isEmpty(), "Invalid stationary force vector was silently accepted.");
    auto invalidTurning = configuration;
    invalidTurning.insert(QStringLiteral("kurvenfahrt"), QStringLiteral("true"));
    const auto rejectedTurning = runSimulation(modelPath, validation, false, 0., .01,
        &cancellation, QJsonObject{{QStringLiteral("stationaer"), invalidTurning}});
    require(!rejectedTurning.error.isEmpty() && !rejectedTurning.completed,
        "Turning mode must be an explicit boolean, not coerced from a string.");
    const auto wrongMode = runSimulation(modelPath, validation, true, .1, .01,
        &cancellation, QJsonObject{{QStringLiteral("stationaer"), configuration}});
    require(!wrongMode.error.isEmpty() && !wrongMode.completed,
        "Dynamic run silently accepted a stationary configuration.");
    auto unknownElement = configuration;
    unknownElement.insert(QStringLiteral("stellungen"), QJsonArray{
        QJsonObject{{QStringLiteral("element_id"), QStringLiteral("unbekanntes_element")},
            {QStringLiteral("wert"), 50.}}});
    const auto rejectedElement = runSimulation(modelPath, validation, false, 0., .01,
        &cancellation, QJsonObject{{QStringLiteral("stationaer"), unknownElement}});
    require(!rejectedElement.completed && (!rejectedElement.error.isEmpty()
            || (!rejectedElement.states.isEmpty()
                && !rejectedElement.states.constLast().value(QStringLiteral("stationaer")).toObject()
                        .value(QStringLiteral("diagnose")).toString().isEmpty())),
        "Unknown stationary movement element must be rejected with an explicit diagnostic.");
    auto impossible = configuration;
    impossible.insert(QStringLiteral("aktiv"), true);
    impossible.insert(QStringLiteral("kraft_n"), QJsonArray{0.,0.,1e8});
    const auto unsolved = runSimulation(modelPath, validation, false, 0., .01,
        &cancellation, QJsonObject{{QStringLiteral("stationaer"), impossible}});
    std::cout << "Unbalanced load status: " << unsolved.status.toStdString()
              << "; diagnostic: " << unsolved.error.toStdString()
              << "; states: " << unsolved.states.size() << '\n';
    require(unsolved.error.isEmpty() && !unsolved.completed && !unsolved.states.isEmpty(),
        "Excessive upward load must yield an explicit unsuccessful physical result.");
    const auto unsolvedContext = unsolved.states.constLast().value(QStringLiteral("stationaer")).toObject();
    require(!unsolvedContext.value(QStringLiteral("konvergiert")).toBool()
            && !unsolvedContext.value(QStringLiteral("diagnose")).toString().isEmpty(),
        "Non-convergence lost its diagnostic or was presented as equilibrium.");
    cancellation.store(true);
    const auto stopped = runSimulation(modelPath, validation, false, 0., .01,
        &cancellation, QJsonObject{{QStringLiteral("stationaer"), configuration}});
    require(!stopped.completed && stopped.status == QStringLiteral("abgebrochen"),
        "Stationary adapter did not preserve cancellation status.");
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

void testAnalyticalStationaryRun(const QString& modelPath)
    {
        const auto validation = validateModel(modelPath);
        require(validation.valid, "Analytical stationary fixture rejected by adapter.");
        std::atomic_bool cancellation{false};
        QJsonObject configuration{
            {QStringLiteral("aktiv"), true},
            {QStringLiteral("modellfest"), false},
            {QStringLiteral("kraft_n"), QJsonArray{1500., 0., 0.}},
            {QStringLiteral("angriffspunkt_modell_m"), QJsonArray{0., 0., -.4}},
            {QStringLiteral("stellungen"), QJsonArray{}}};
        const auto result = runSimulation(modelPath, validation, false, 0., .01,
            &cancellation, QJsonObject{{QStringLiteral("stationaer"), configuration}});
        require(result.completed && result.error.isEmpty() && !result.states.isEmpty(),
            "Analytical stationary force case did not converge through GUI adapter.");
        const auto state = result.states.constLast();
        const auto context = state.value(QStringLiteral("stationaer")).toObject();
        const double speed = number(context, QStringLiteral("geschwindigkeit_relativ_wasser_betrag_m_s"));
        require(std::abs(speed - 1.118033989) <= .1,
            "Analytical stationary speed exceeds 0.1 m/s acceptance error.");
        for (const auto& key : {QStringLiteral("residuum_kraft_welt_n"),
                 QStringLiteral("residuum_moment_modell_nm")}) {
            const auto residual = context.value(key).toArray();
            require(residual.size() == 3, "Analytical case omitted residual components.");
            for (const auto& component : residual)
                require(component.isDouble() && std::isfinite(component.toDouble())
                        && std::abs(component.toDouble()) <= .1,
                    "Analytical case exceeded force/moment component acceptance.");
        }
        require(context.value(QStringLiteral("kraft_n")) == configuration.value(QStringLiteral("kraft_n"))
                && context.value(QStringLiteral("angriffspunkt_modell_m"))
                    == configuration.value(QStringLiteral("angriffspunkt_modell_m")),
            "Analytical load was not preserved in the frozen result.");
        std::cout << "Analytical stationary adapter evidence: "
                  << QJsonDocument(context).toJson(QJsonDocument::Compact).constData() << '\n';
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

void testTurningReference(const QString& straightModel)
    {
        const QDir fixture(QDir(QFileInfo(straightModel).absolutePath())
            .filePath(QStringLiteral("../stationaere_kurvenreferenz")));
        QFile input(fixture.filePath(QStringLiteral("kreisfall.json")));
        require(input.open(QIODevice::ReadOnly), "Shared Core circle fixture must be readable.");
        const auto reference = QJsonDocument::fromJson(input.readAll()).object();
        require(!reference.isEmpty(), "Shared Core circle reference must be valid JSON.");
        const auto modelPath = fixture.filePath(reference.value(QStringLiteral("modell")).toString());
        const auto validation = validateModel(modelPath);
        require(validation.valid, "Shared Core circle model rejected.");
        auto configuration = reference.value(QStringLiteral("stationaer")).toObject();
        configuration.insert(QStringLiteral("kraft_n"), configuration.take(QStringLiteral("kraft_welt_n")));
        auto start = reference.value(QStringLiteral("start")).toObject();
        start.insert(QStringLiteral("stationaer"), configuration);
        std::atomic_bool cancellation{false};
        const auto result = runSimulation(modelPath, validation, false, 0., .01, &cancellation, start);
        std::cout << "Core circle adapter status=" << result.status.toStdString()
                  << " error=" << result.error.toStdString() << '\n';
        require(result.completed && result.error.isEmpty() && !result.states.isEmpty(),
            "Shared Core circle equilibrium must pass through the real GUI adapter.");
        const auto state = result.states.constLast();
        const auto context = state.value(QStringLiteral("stationaer")).toObject();
        const auto expected = reference.value(QStringLiteral("erwartet")).toObject();
        require(state.value(QStringLiteral("vertragsversion")) == QJsonValue(QStringLiteral("6"))
                && state.value(QStringLiteral("kursdrehrate_status")) == QJsonValue(QStringLiteral("gueltig")),
            "Circle result must have the explicit V6 rate contract.");
        require(std::abs(number(state, QStringLiteral("kursdrehrate_rad_s"))
                    - number(expected, QStringLiteral("kursdrehrate_rad_s"))) * 180. / std::acos(-1.) <= .01,
            "Circle reference heading-rate error exceeds 0.01 degree/s.");
        require(context.value(QStringLiteral("loesungstyp")) == QJsonValue(QStringLiteral("kurvenfahrt"))
                && context.value(QStringLiteral("kurvenfahrt")).toBool(),
            "Turning mode must not be silently reduced to the old straight-line contract.");
        require(std::abs(number(context, QStringLiteral("radius_m")) - 5.) <= .1,
            "Circle reference radius was not preserved.");
        require(std::abs(number(context, QStringLiteral("geschwindigkeit_relativ_wasser_betrag_m_s")) - 2.) <= .1,
            "Circle reference speed error exceeds 0.1 m/s.");
        for (const auto& key : {QStringLiteral("residuum_kraft_welt_n"),
                 QStringLiteral("residuum_moment_modell_nm")}) {
            const auto values = context.value(key).toArray();
            require(values.size() == 3, "Circle motion residual requires exactly three components.");
            for (const auto& component : values)
                require(component.isDouble() && std::isfinite(component.toDouble())
                        && std::abs(component.toDouble()) <= .1,
                    "Circle motion residual exceeds 0.1 N/Nm.");
        }
        const auto external = context.value(QStringLiteral("aeussere_kraft_welt_n")).toArray();
        require(external.size() == 3 && std::abs(external[1].toDouble()) > 1000.,
            "Converged circle must retain its nonzero external centripetal force.");
        const auto omega = context.value(QStringLiteral("omega_modell_rad_s")).toArray();
        require(omega.size() == 3 && std::abs(omega[2].toDouble()
                    - number(state, QStringLiteral("kursdrehrate_rad_s"))) > .001,
            "At heel, body omega-Z must not replace the world course rate.");
        std::cout << "Shared Core circle adapter evidence: "
                  << QJsonDocument(context).toJson(QJsonDocument::Compact).constData() << '\n';
        auto spinStart = start;
        auto spinConfiguration = configuration;
        spinConfiguration.insert(QStringLiteral("aktiv"), false);
        spinStart.insert(QStringLiteral("position_m"), QJsonArray{0.,0.,0.});
        spinStart.insert(QStringLiteral("orientierung_xyzw"), QJsonArray{0.,0.,0.,1.});
        spinStart.insert(QStringLiteral("geschwindigkeit_welt_m_s"), QJsonArray{0.,0.,0.});
        spinStart.insert(QStringLiteral("omega_modell_rad_s"), QJsonArray{0.,0.,-.4});
        spinStart.insert(QStringLiteral("stationaer"), spinConfiguration);
        const auto spin = runSimulation(modelPath, validation, false, 0., .01,
            &cancellation, spinStart);
        require(spin.completed && spin.error.isEmpty() && !spin.states.isEmpty(),
            "Pure-spin Core reference must work through the same GUI adapter.");
        const auto spinContext = spin.states.constLast().value(QStringLiteral("stationaer")).toObject();
        require(spinContext.value(QStringLiteral("loesungstyp")) == QJsonValue(QStringLiteral("reine_drehung"))
                && spinContext.value(QStringLiteral("radius_m")).isNull()
                && spinContext.value(QStringLiteral("radius_status")) == QJsonValue(QStringLiteral("reine_drehung")),
            "Pure spin must be distinct from straight-line/circle, without a fabricated radius.");
        require(std::abs(number(spin.states.constLast(), QStringLiteral("kursdrehrate_rad_s")) + .4) <= 1e-8,
            "Negative pure-spin rate must be preserved.");
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
    require(historical.value(QStringLiteral("vertragsversion")).toString() == QStringLiteral("6")
        && historical.value(QStringLiteral("testantrieb")).isObject(), "V6 test context missing");
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
    require(result.value(QStringLiteral("vertragsversion")).toString() == QStringLiteral("6"),
        "Movement simulation did not return result contract V6.");
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

void testDynamicForce(const QString& referenceModelPath)
{
    const auto modelPath = QDir(QFileInfo(referenceModelPath).absolutePath())
        .filePath(QStringLiteral("../420/modell.json"));
    const auto validation = validateModel(modelPath);
    require(validation.valid, "Dynamic force movement model rejected.");
    QJsonObject configuration{
        {QStringLiteral("aktiv"), true},
        {QStringLiteral("modellfest"), true},
        {QStringLiteral("kraft_n"), QJsonArray{150., 10., -2.}},
        {QStringLiteral("angriffspunkt_modell_m"), QJsonArray{1., 0., 3.}},
        {QStringLiteral("stellungen"), QJsonArray{
            QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Ruder")},
                {QStringLiteral("wert"), 5.}},
            QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Schwert")},
                {QStringLiteral("wert"), 50.}}}}};
    const auto start = QJsonObject{{QStringLiteral("dynamische_fremdkraft"), configuration}};
    QString error;
    auto live = LiveSimulation::create(modelPath, validation, .03, .01, false, &error, start);
    if (!live) std::cerr << error.toStdString() << '\n';
    require(live != nullptr, "Dynamic force start rejected.");
    std::atomic_bool cancellation{false};
    live->run(cancellation);
    require(live->progress().status == QStringLiteral("abgeschlossen"),
        "Dynamic force live run failed.");
    const auto frozen = live->stateJson(0, &error);
    bool foundForce = false;
    for (const auto& value : frozen.value(QStringLiteral("vektoren")).toArray()) {
        const auto vector = value.toObject();
        if (vector.value(QStringLiteral("id")).toString().contains(QStringLiteral("fremdkraft"))
            && vector.value(QStringLiteral("art")) == QJsonValue(QStringLiteral("kraft"))) {
            require(vector.value(QStringLiteral("vektor")) == configuration.value(QStringLiteral("kraft_n"))
                    && vector.value(QStringLiteral("punkt_m"))
                        == configuration.value(QStringLiteral("angriffspunkt_modell_m")),
                "Dynamic initial XYZ force/application point differs from GUI inputs.");
            const auto reference = frozen.value(QStringLiteral("moment_bezug_modell_m")).toArray();
            const auto moment = vector.value(QStringLiteral("moment_s_nm")).toArray();
            require(reference.size() == 3 && moment.size() == 3,
                "Dynamic force must expose its physical moment about the moved CG.");
            const auto point = configuration.value(QStringLiteral("angriffspunkt_modell_m")).toArray();
            const auto force = configuration.value(QStringLiteral("kraft_n")).toArray();
            for (int axis = 0; axis < 3; ++axis) {
                const int a = (axis + 1) % 3, b = (axis + 2) % 3;
                const double expected = (point[a].toDouble() - reference[a].toDouble()) * force[b].toDouble()
                    - (point[b].toDouble() - reference[b].toDouble()) * force[a].toDouble();
                require(std::abs(moment[axis].toDouble() - expected) < 1e-10,
                    "Dynamic force moment differs from model-point lever cross force.");
            }
            foundForce = true;
        }
    }
    require(foundForce, "Dynamic initial state omitted actual external force.");
    for (qsizetype i = 0; i < live->progress().states; ++i) {
        const auto state = live->stateJson(i, &error);
        const auto movements = state.value(QStringLiteral("bewegungen")).toArray();
        for (const auto& entry : configuration.value(QStringLiteral("stellungen")).toArray()) {
            bool found = false;
            for (const auto& movement : movements)
                found = found || (movement.toObject().value(QStringLiteral("element_id"))
                        == entry.toObject().value(QStringLiteral("element_id"))
                    && movement.toObject().value(QStringLiteral("wert"))
                        == entry.toObject().value(QStringLiteral("wert")));
            require(found, "Fixed Core movement missing or changed during dynamic run.");
        }
    }
    const auto batch = runSimulation(modelPath, validation, true, .03, .01, &cancellation, start);
    if (!batch.completed || !batch.error.isEmpty() || batch.states.size() != live->progress().states)
        std::cerr << "Dynamic batch: " << batch.status.toStdString()
                  << " error=" << batch.error.toStdString() << " states=" << batch.states.size()
                  << " live=" << live->progress().states << '\n';
    require(batch.completed && batch.error.isEmpty()
            && batch.states.size() == live->progress().states,
        "Dynamic force batch/live contract differs.");
    const auto last = live->stateJson(live->progress().states - 1, &error);
    for (const auto& key : {QStringLiteral("position_m"), QStringLiteral("omega_modell_rad_s")}) {
        const auto actual = last.value(key).toArray();
        const auto expected = batch.states.constLast().value(key).toArray();
        require(actual.size() == 3 && expected.size() == 3, "Dynamic output shape invalid.");
        for (int axis = 0; axis < 3; ++axis)
            require(std::abs(actual[axis].toDouble() - expected[axis].toDouble()) < 1e-12,
                "Dynamic force batch/live physics diverges.");
    }
    auto invalid = configuration;
    invalid.insert(QStringLiteral("modellfest"), false);
    require(!LiveSimulation::create(modelPath, validation, .03, .01, false, &error,
                {{QStringLiteral("dynamische_fremdkraft"), invalid}}) && !error.isEmpty(),
        "Unsupported dynamic world-fixed force was silently accepted.");
    invalid = configuration;
    invalid.insert(QStringLiteral("stellungen"), QJsonArray{
        QJsonObject{{QStringLiteral("element_id"), QStringLiteral("Ruder")},
            {QStringLiteral("wert"), 91.}}});
    require(!LiveSimulation::create(modelPath, validation, .03, .01, false, &error,
                {{QStringLiteral("dynamische_fremdkraft"), invalid}}) && !error.isEmpty(),
        "Invalid dynamic movement setting was silently accepted.");
    invalid = configuration;
    invalid.insert(QStringLiteral("aktiv"), QStringLiteral("true"));
    require(!LiveSimulation::create(modelPath, validation, .03, .01, false, &error,
                {{QStringLiteral("dynamische_fremdkraft"), invalid}}) && !error.isEmpty(),
        "Malformed dynamic activation was silently accepted.");
    require(live->stateJson(0, &error) == frozen,
        "Later runs or invalid requests changed a historical force snapshot.");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (argc != 3) {
        std::cerr << "Expected the Core reference-model and stationary fixture paths.\n";
        return 2;
    }
    try {
        const auto modelPath = QString::fromLocal8Bit(argv[1]);
        if (QString::fromLocal8Bit(argv[2]) == QStringLiteral("--dynamic-force-only")) {
            testDynamicForce(modelPath);
            std::cout << "Dynamic force adapter contract passed.\n";
            return 0;
        }
        ModelValidation validation;
        testValidation(modelPath, validation);
        testStaticRun(modelPath, validation);
        testStationaryRun(modelPath, validation);
        testAnalyticalStationaryRun(QString::fromLocal8Bit(argv[2]));
        testTurningReference(QString::fromLocal8Bit(argv[2]));
        testDynamicRun(modelPath, validation);
        testLiveRun(modelPath, validation);
        testStartState(modelPath, validation);
        testDrive(modelPath, validation);
        testMovementInput(modelPath);
        testDynamicForce(modelPath);
        std::cout << "Core adapter validation, static hydrostatics and "
                     "dynamic run passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
