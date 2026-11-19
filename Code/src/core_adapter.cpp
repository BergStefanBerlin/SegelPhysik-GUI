#include "core_adapter.hpp"

#include "segelphysik/aufbereitung.hpp"
#include "segelphysik/inspektion.hpp"
#include "segelphysik/modell.hpp"
#include "segelphysik/simulation.hpp"
#include "segelphysik/sailforce.hpp"
#include "segelphysik/wind.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {

double currentThreadCpuSeconds()
{
#ifdef Q_OS_WIN
    FILETIME created{}, exited{}, kernel{}, user{};
    if (GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user)) {
        ULARGE_INTEGER k{}, u{};
        k.LowPart = kernel.dwLowDateTime;
        k.HighPart = kernel.dwHighDateTime;
        u.LowPart = user.dwLowDateTime;
        u.HighPart = user.dwHighDateTime;
        return static_cast<double>(k.QuadPart + u.QuadPart) * 1e-7;
    }
#endif
    return -1.;
}

quint64 currentThreadCpuCycles()
{
#ifdef Q_OS_WIN
    ULONG64 cycles = 0;
    if (QueryThreadCycleTime(GetCurrentThread(), &cycles)) return cycles;
#endif
    return 0;
}

std::filesystem::path nativePath(const QString& path)
{
    return std::filesystem::path(path.toStdWString());
}

QJsonObject parseObject(const QByteArray& json, const QString& context)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError
        || !document.isObject()) {
        throw std::runtime_error(
            QStringLiteral("%1: %2")
                .arg(context, parseError.errorString())
                .toStdString());
    }
    return document.object();
}

bool hasFiniteNumber(const QJsonObject& object, const QString& field)
{
    const auto value = object.value(field);
    return value.isDouble() && std::isfinite(value.toDouble());
}

QString formatDiagnostics(const QJsonObject& object)
{
    QStringList messages;
    const auto failures = object.value(QStringLiteral("fehler")).toArray();
    for (const auto& value : failures) {
        const auto failure = value.toObject();
        const auto id = failure.value(QStringLiteral("id")).toString();
        const auto details = failure.value(QStringLiteral("details")).toString();
        const auto entry = details.isEmpty()
            ? id
            : QStringLiteral("%1: %2").arg(id, details);
        if (!entry.isEmpty()) {
            messages.push_back(entry);
        }
    }
    return messages.join(QLatin1Char('\n'));
}

template<std::size_t N>
std::array<double, N> finiteArray(const QJsonObject& state, const QString& field)
{
    const auto values = state.value(field).toArray();
    if (values.size() != static_cast<qsizetype>(N)) {
        throw std::runtime_error(QStringLiteral("Startzustand ohne gültiges Feld %1.")
                                     .arg(field).toStdString());
    }
    std::array<double, N> result{};
    for (std::size_t i = 0; i < N; ++i) {
        const auto value = values.at(static_cast<qsizetype>(i));
        if (!value.isDouble() || !std::isfinite(value.toDouble())) {
            throw std::runtime_error(QStringLiteral("Startzustand ohne gültiges Feld %1.")
                                         .arg(field).toStdString());
        }
        result[i] = value.toDouble();
    }
    return result;
}

// Übernimmt einen angezeigten Core-Zustand als Startwert eines neuen Laufs.
segelphysik::Zustand startState(const QJsonObject& state, bool keepMotion)
{
    segelphysik::Zustand result;
    if (state.isEmpty()) {
        return result;
    }
    result.pose = segelphysik::Pose(finiteArray<3>(state, QStringLiteral("position_m")),
        finiteArray<4>(state, QStringLiteral("orientierung_xyzw")));
    if (keepMotion) {
        result.geschwindigkeit_welt_m_s =
            finiteArray<3>(state, QStringLiteral("geschwindigkeit_welt_m_s"));
        result.omega_modell_rad_s = finiteArray<3>(state, QStringLiteral("omega_modell_rad_s"));
    }
    for (const auto& id : state.value(QStringLiteral("geflutete_elemente")).toArray()) {
        result.geflutet.push_back(id.toString().toStdString());
    }
    const auto movements = state.value(QStringLiteral("bewegungen")).toArray();
    for (const auto& value : movements) {
        const auto movement = value.toObject();
        const auto id = movement.value(QStringLiteral("element_id")).toString();
        const auto setting = movement.value(QStringLiteral("wert"));
        if (id.isEmpty() || !setting.isDouble() || !std::isfinite(setting.toDouble()))
            throw std::runtime_error("Startzustand ohne gueltige Core-Bewegungsstellung.");
        result.bewegungsstellungen.push_back({id.toStdString(), setting.toDouble()});
    }
    const auto lastEvent = state.value(QStringLiteral("letzte_bewegungsereigniskennung"));
    if (lastEvent.isDouble() && lastEvent.toDouble() >= 0.0)
        result.letzte_bewegungsereigniskennung = static_cast<std::size_t>(lastEvent.toDouble());
    return result;
}

std::optional<segelphysik::DynamischeFremdkraftKonfiguration> dynamicForceConfiguration(
    const QJsonObject& start, segelphysik::Zustand& state)
{
    const auto key = QStringLiteral("dynamische_fremdkraft");
    if (!start.contains(key)) return std::nullopt;
    const auto value = start.value(key);
    if (!value.isObject())
        throw std::invalid_argument("Dynamische Fremdkraft muss ein Objekt sein.");
    const auto configuration = value.toObject();
    if (!configuration.value(QStringLiteral("aktiv")).isBool()
        || !configuration.value(QStringLiteral("stellungen")).isArray()
        || (configuration.contains(QStringLiteral("modellfest"))
            && (!configuration.value(QStringLiteral("modellfest")).isBool()
                || !configuration.value(QStringLiteral("modellfest")).toBool())))
        throw std::invalid_argument("Dynamische Fremdkraft ohne explizite modellfeste Kraft-/Stellungsdefinition.");
    segelphysik::DynamischeFremdkraftKonfiguration result;
    result.aktiv = configuration.value(QStringLiteral("aktiv")).toBool();
    result.kraft_modellfest_n = finiteArray<3>(configuration, QStringLiteral("kraft_n"));
    result.angriffspunkt_modell_m =
        finiteArray<3>(configuration, QStringLiteral("angriffspunkt_modell_m"));
    decltype(state.bewegungsstellungen) settings;
    QSet<QString> ids;
    for (const auto& entry : configuration.value(QStringLiteral("stellungen")).toArray()) {
        const auto setting = entry.toObject();
        const auto id = setting.value(QStringLiteral("element_id")).toString();
        const auto number = setting.value(QStringLiteral("wert"));
        if (id.isEmpty() || ids.contains(id) || !number.isDouble()
            || !std::isfinite(number.toDouble()))
            throw std::invalid_argument("Dynamische Fremdkraft ohne eindeutige gueltige Core-Bewegungsstellung.");
        ids.insert(id);
        settings.push_back({id.toStdString(), number.toDouble()});
    }
    state.bewegungsstellungen = std::move(settings);
    return result;
}

segelphysik::Zustand startWind(const QJsonObject& input,
    const segelphysik::Modell& model, const segelphysik::Zustand& state)
{
    const auto value = input.value(QStringLiteral("wind_eingabe"));
    if (value.isUndefined()) return state;
    if (!value.isObject()) throw std::invalid_argument("Windvorgaben fehlen.");
    const auto wind = value.toObject();
    for (const auto& field : {QStringLiteral("richtung_grad"), QStringLiteral("v0_m_s"),
                              QStringLiteral("h0_m")})
        if (!hasFiniteNumber(wind, field))
            throw std::invalid_argument("Windvorgaben enthalten ungueltige Zahlen.");
    if (!wind.value(QStringLiteral("bootmitgefuehrt")).isBool())
        throw std::invalid_argument("Windvorgaben ohne Richtungsmodus.");
    segelphysik::WindKonfiguration config;
    config.herkunft_welt_grad = wind.value(QStringLiteral("richtung_grad")).toDouble();
    config.referenzgeschwindigkeit_m_s = wind.value(QStringLiteral("v0_m_s")).toDouble();
    config.referenzhoehe_m = wind.value(QStringLiteral("h0_m")).toDouble();
    config.modus = wind.value(QStringLiteral("bootmitgefuehrt")).toBool()
        ? segelphysik::WindModus::bootmitgefuehrt : segelphysik::WindModus::weltfest;
    std::optional<segelphysik::Vektor3> point;
    const auto pointValue = wind.value(QStringLiteral("mastspitze_modell_m"));
    if (!pointValue.isUndefined() && !pointValue.isNull() && !pointValue.isArray())
        throw std::invalid_argument("Mastspitze muss ein Vektor sein.");
    if (pointValue.isArray())
        point = finiteArray<3>(wind, QStringLiteral("mastspitze_modell_m"));
    const auto element = wind.value(QStringLiteral("mast_element_id")).toString().toStdString();
    auto diagnosis = wind.value(QStringLiteral("mast_diagnose")).toString().toStdString();
    if (!point && diagnosis.empty())
        diagnosis = "Keine Mastreferenz uebergeben; Punktwind bleibt verfuegbar.";
    const auto source = wind.value(QStringLiteral("mast_quelle"));
    if (!source.isUndefined() && !source.isString())
        throw std::invalid_argument("Mastquelle muss eine Zeichenkette sein.");
    if (point && source.isString() && source.toString().isEmpty())
        throw std::invalid_argument("Mastquelle darf nicht leer sein.");
    auto withMast = state;
    if (point && source.isString() && !source.toString().isEmpty())
        withMast.mast = segelphysik::MastReferenz{
            point, element, source.toString().toStdString(), diagnosis, model.fassung};
    return segelphysik::wind_start(model, withMast, config, point, element, diagnosis);
}

std::optional<segelphysik::SailforceKonfiguration> sailforceInput(const QJsonObject& input)
{
    const auto value = input.value(QStringLiteral("sailforce_eingabe"));
    if (value.isUndefined()) return std::nullopt;
    if (!value.isObject()) throw std::invalid_argument("Sailforce-Eingabe muss ein Objekt sein.");
    const auto object = value.toObject();
    const auto air = object.value(QStringLiteral("luft"));
    const auto sails = object.value(QStringLiteral("segel"));
    if (!air.isObject() || !sails.isArray())
        throw std::invalid_argument("Sailforce-Eingabe benoetigt Luft und Segel.");
    const auto luft = air.toObject();
    if (!hasFiniteNumber(luft, QStringLiteral("dichte_kg_m3"))
        || !hasFiniteNumber(luft, QStringLiteral("kinematische_viskositaet_m2_s")))
        throw std::invalid_argument("Lufteigenschaften muessen endlich sein.");
    segelphysik::SailforceKonfiguration config;
    config.luft.dichte_kg_m3 = luft.value(QStringLiteral("dichte_kg_m3")).toDouble();
    config.luft.kinematische_viskositaet_m2_s =
        luft.value(QStringLiteral("kinematische_viskositaet_m2_s")).toDouble();
    if (config.luft.dichte_kg_m3 <= 0. || config.luft.kinematische_viskositaet_m2_s <= 0.)
        throw std::invalid_argument("Lufteigenschaften muessen positiv sein.");
    QSet<QString> ids;
    for (const auto& entry : sails.toArray()) {
        const auto sail = entry.toObject();
        const auto id = sail.value(QStringLiteral("element_id")).toString();
        if (!entry.isObject() || id.isEmpty() || ids.contains(id)
            || !sail.value(QStringLiteral("aktiv")).isBool()
            || !hasFiniteNumber(sail, QStringLiteral("stellwinkel_grad")))
            throw std::invalid_argument("Sailforce-Segelwahl ist ungueltig oder doppelt.");
        ids.insert(id);
        config.segel.push_back({id.toStdString(), sail.value(QStringLiteral("aktiv")).toBool(),
            sail.value(QStringLiteral("stellwinkel_grad")).toDouble()});
    }
    return config;
}

} // namespace

QString validateSailforceInput(const QString& modelPath, const QJsonObject& input,
    std::shared_ptr<const segelphysik::ModelInspection> inspection)
{
    try {
        const auto config = sailforceInput(input);
        if (!config) return QStringLiteral("Sailforce-Eingabe fehlt.");
        const auto model = inspection ? segelphysik::modellAusInspektion(*inspection)
                                      : segelphysik::lade_modell(nativePath(modelPath));
        (void)segelphysik::sailforce_start(model, {}, nativePath(modelPath), *config);
        return {};
    } catch (const std::exception& exception) {
        return QString::fromUtf8(exception.what());
    }
}

ModelValidation validateModel(const QString& modelPath)
{
    ModelValidation result;
    try {
        bool valid = false;
        const auto json = segelphysik::pruefung_json(nativePath(modelPath), valid);
        const auto object = parseObject(
            QByteArray::fromStdString(json), QStringLiteral("Modellprüfung"));
        result.valid = valid && object.value(QStringLiteral("gueltig")).toBool();
        result.modelId = object.value(QStringLiteral("modell_id")).toString();
        result.fingerprint =
            object.value(QStringLiteral("modell_fassung")).toString();
        result.diagnostic = object.value(QStringLiteral("diagnose")).toString();
        if (result.diagnostic.isEmpty()) {
            result.diagnostic = formatDiagnostics(object);
        }
        if (!result.valid && result.diagnostic.isEmpty()) {
            result.diagnostic = QStringLiteral(
                "Der Core hat das Modell nicht zur Simulation freigegeben.");
        }
        if (result.valid
            && (result.modelId.isEmpty() || result.fingerprint.isEmpty())) {
            result.valid = false;
            result.diagnostic = QStringLiteral(
                "Die Core-Prüfung lieferte keine vollständige Modellidentität.");
        }
    } catch (const std::exception& error) {
        result.valid = false;
        result.diagnostic = QString::fromUtf8(error.what());
    }
    return result;
}

SimulationResult runSimulation(const QString& modelPath,
    const ModelValidation& validation, bool dynamic, double durationSeconds,
    double stepSeconds, std::atomic_bool* cancellation, const QJsonObject& start,
    std::shared_ptr<const segelphysik::ModelInspection> inspection)
{
    SimulationResult result;
    if (!validation.valid) {
        result.error = QStringLiteral("Das Modell wurde nicht erfolgreich geprüft.");
        return result;
    }
    if (dynamic && (!std::isfinite(durationSeconds)
                       || durationSeconds < 0.0
                       || !std::isfinite(stepSeconds)
                       || stepSeconds <= 0.0)) {
        result.error = QStringLiteral(
            "Dauer muss endlich und nicht negativ, Schrittweite endlich und "
            "positiv sein.");
        return result;
    }
    if (cancellation == nullptr) {
        result.error = QStringLiteral(
            "Der Lauf benötigt ein gültiges Abbruch-Flag.");
        return result;
    }

    const auto outputPath = QDir::temp().filePath(
        QStringLiteral("segelphysik-gui-%1.jsonl")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    try {
        const auto model = inspection ? segelphysik::modellAusInspektion(*inspection)
                                      : segelphysik::lade_modell(nativePath(modelPath));
        if (model.id != validation.modelId.toStdString() ||
            model.fassung != validation.fingerprint.toStdString())
            throw std::runtime_error("Modellfassung hat sich seit der Prüfung geändert.");
        if (start.contains(QStringLiteral("stationaer"))) {
            const auto value = start.value(QStringLiteral("stationaer"));
            if (dynamic || !value.isObject())
                throw std::invalid_argument("Stationaerer Rechenfall ist nur als Objekt im statischen Lauf erlaubt.");
            const auto stationary = value.toObject();
            if (!stationary.value(QStringLiteral("aktiv")).isBool()
                || !stationary.value(QStringLiteral("modellfest")).isBool()
                || (stationary.contains(QStringLiteral("kurvenfahrt"))
                    && !stationary.value(QStringLiteral("kurvenfahrt")).isBool())
                || !stationary.value(QStringLiteral("stellungen")).isArray())
                throw std::invalid_argument("Stationaerer Rechenfall ohne explizite Kraft-/Stellungsdefinition.");
            finiteArray<3>(stationary, QStringLiteral("kraft_n"));
            finiteArray<3>(stationary, QStringLiteral("angriffspunkt_modell_m"));
            for (const auto& entry : stationary.value(QStringLiteral("stellungen")).toArray()) {
                const auto setting = entry.toObject();
                if (!entry.isObject() || setting.value(QStringLiteral("element_id")).toString().isEmpty()
                    || !setting.value(QStringLiteral("wert")).isDouble()
                    || !std::isfinite(setting.value(QStringLiteral("wert")).toDouble()))
                    throw std::invalid_argument("Stationaerer Rechenfall mit ungueltiger Elementstellung.");
            }
        }
        segelphysik::Laufoptionen options;
        options.ergebnis_json_format = segelphysik::ErgebnisJsonFormat::V6;
        options.dynamisch = dynamic;
        options.dauer_s = durationSeconds;
        options.dt_s = stepSeconds;
        options.jsonl = true;
        options.abbruch = cancellation;
        auto physicalStart = start;
        physicalStart.remove(QStringLiteral("stationaer"));
        physicalStart.remove(QStringLiteral("dynamische_fremdkraft"));
        physicalStart.remove(QStringLiteral("wind_eingabe"));
        physicalStart.remove(QStringLiteral("sailforce_eingabe"));
        options.start = startState(physicalStart, dynamic || start.contains(QStringLiteral("stationaer")));
        if (start.contains(QStringLiteral("wind_eingabe")))
            options.start = startWind(start, model, options.start);
        if (dynamic || start.contains(QStringLiteral("stationaer")))
            options.sailforce = sailforceInput(start);
        if (!dynamic && start.contains(QStringLiteral("dynamische_fremdkraft")))
            throw std::invalid_argument("Dynamische Fremdkraft ist nur im dynamischen Lauf erlaubt.");
        options.dynamische_fremdkraft = dynamicForceConfiguration(start, options.start);
        if (start.contains(QStringLiteral("stationaer"))) {
            const auto stationary = start.value(QStringLiteral("stationaer")).toObject();
            segelphysik::StationaereGleichgewichtKonfiguration configuration;
            configuration.aktiv = stationary.value(QStringLiteral("aktiv")).toBool();
            configuration.modellfest = stationary.value(QStringLiteral("modellfest")).toBool();
            configuration.kurvenfahrt = stationary.value(QStringLiteral("kurvenfahrt")).toBool(false);
            configuration.kraft_welt_n = finiteArray<3>(stationary, QStringLiteral("kraft_n"));
            configuration.angriffspunkt_modell_m =
                finiteArray<3>(stationary, QStringLiteral("angriffspunkt_modell_m"));
            std::size_t sequence = 0;
            for (const auto& entry : stationary.value(QStringLiteral("stellungen")).toArray()) {
                const auto setting = entry.toObject();
                configuration.stellungen.push_back({++sequence,
                    setting.value(QStringLiteral("element_id")).toString().toStdString(),
                    setting.value(QStringLiteral("wert")).toDouble()});
            }
            options.stationaer = configuration;
        }

        const auto report = segelphysik::simulieren(
            model, nativePath(modelPath), nativePath(outputPath), options);
        result.status = QString::fromStdString(report.status);
        result.diagnostic = QString::fromStdString(report.diagnose);
        result.runtimeSeconds = report.gesamtlaufzeit_s;
        result.runtimeAvailable = true;

        QFile output(outputPath);
        if (!output.open(QIODevice::ReadOnly | QIODevice::Text)) {
            throw std::runtime_error(
                QStringLiteral("Core-Ergebnisdatei kann nicht gelesen werden: %1")
                    .arg(output.errorString())
                    .toStdString());
        }
        while (!output.atEnd()) {
            const auto line = output.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }
            const auto state = parseObject(line, QStringLiteral("Core-Zustand"));
            const auto contract =
                state.value(QStringLiteral("vertragsversion")).toString();
            if ((contract != QStringLiteral("1") && contract != QStringLiteral("2")
                    && contract != QStringLiteral("3") && contract != QStringLiteral("4")
                    && contract != QStringLiteral("5") && contract != QStringLiteral("6")
                    && contract != QStringLiteral("7") && contract != QStringLiteral("8"))
                || state.value(QStringLiteral("modell_id")).toString()
                    != validation.modelId
                || state.value(QStringLiteral("modell_fassung")).toString()
                    != validation.fingerprint
                || !hasFiniteNumber(state, QStringLiteral("zeit_s"))) {
                throw std::runtime_error(
                    "Core-Zustand hat eine inkompatible Vertragsversion, "
                    "Modellzuordnung oder Simulationszeit.");
            }
            if (contract == QStringLiteral("5")
                && !state.value(QStringLiteral("stationaer")).isObject())
                throw std::runtime_error(
                    "Core-Zustand V5 ohne stationaeren Ergebnisvertrag.");
            if (contract == QStringLiteral("7")) {
                if (!state.value(QStringLiteral("dynamische_fremdkraft")).isObject())
                    throw std::runtime_error("Core-Zustand V7 ohne dynamischen Fremdkraftvertrag.");
                const auto force = state.value(QStringLiteral("dynamische_fremdkraft")).toObject();
                if (!force.value(QStringLiteral("aktiv")).isBool()
                    || force.value(QStringLiteral("modellfest")) != QJsonValue(true)
                    || !hasFiniteNumber(force, QStringLiteral("auswertung_schritt"))
                    || force.value(QStringLiteral("auswertung_schritt")).toDouble() < 0.
                    || std::floor(force.value(QStringLiteral("auswertung_schritt")).toDouble())
                        != force.value(QStringLiteral("auswertung_schritt")).toDouble()
                    || !hasFiniteNumber(force, QStringLiteral("auswertung_zeit_s"))
                    || force.value(QStringLiteral("modell_fassung")) != QJsonValue(validation.fingerprint)
                    || force.value(QStringLiteral("auswertung_schritt")) != state.value(QStringLiteral("schritt"))
                    || force.value(QStringLiteral("auswertung_zeit_s")) != state.value(QStringLiteral("zeit_s")))
                    throw std::runtime_error("Core-Zustand V7 mit inkonsistentem dynamischem Fremdkraftvertrag.");
                finiteArray<3>(force, QStringLiteral("kraft_modellfest_n"));
                finiteArray<3>(force, QStringLiteral("angriffspunkt_modell_m"));
            }
            if (contract == QStringLiteral("6") || contract == QStringLiteral("7")) {
                const auto rate = state.value(QStringLiteral("kursdrehrate_rad_s"));
                const auto rateStatus = state.value(QStringLiteral("kursdrehrate_status")).toString();
                if (!((rateStatus == QStringLiteral("gueltig") && rate.isDouble()
                            && std::isfinite(rate.toDouble()))
                        || (rateStatus == QStringLiteral("nicht_bestimmbar") && rate.isNull())))
                    throw std::runtime_error("Core-Zustand V6 mit ungültiger Kursdrehrate oder Status.");
                if (options.stationaer && !state.value(QStringLiteral("stationaer")).isObject())
                    throw std::runtime_error("Stationärer Core-Zustand V6 ohne Ergebnisvertrag.");
            }
            result.states.push_back(state);
        }
        output.close();
        QFile::remove(outputPath);

        if (!std::isfinite(result.runtimeSeconds)
            || result.runtimeSeconds < 0.0) {
            throw std::runtime_error(
                "Core lieferte keine endliche, nichtnegative Gesamtlaufzeit.");
        }
        result.completed = dynamic
            ? result.status == QStringLiteral("abgeschlossen")
            : result.status == QStringLiteral("konvergiert");
        if (result.states.isEmpty() && result.error.isEmpty()) {
            result.error = QStringLiteral(
                "Core-Lauf hat keine vollständigen Ergebniszustände geliefert.");
            result.completed = false;
        }
    } catch (const std::exception& error) {
        QFile::remove(outputPath);
        result.error = QString::fromUtf8(error.what());
        result.completed = false;
    }
    return result;
}

struct LiveSimulation::Impl {
    struct WindChange {
        std::optional<double> direction;
        std::optional<double> speed;
        std::optional<bool> boatFixed;
    };
    std::unique_ptr<segelphysik::Modellaufbereitung> workerPreparation;
    std::unique_ptr<segelphysik::Modellaufbereitung> displayPreparation;
    QString modelId;
    QString fingerprint;
    double durationSeconds = 0.0;
    double stepSeconds = 0.01;
    bool realtimePacing = true;
    segelphysik::Zustand start;
    QJsonObject initialJson;
    std::shared_ptr<const segelphysik::ErgebnisSnapshot> initialSnapshot;
    bool hasTestDrive = false;
    std::vector<segelphysik::TestantriebEreignis> pendingTestEvents;
    std::size_t nextTestEvent = 1;
    std::vector<segelphysik::Bewegungsereignis> pendingMovementEvents;
    std::size_t nextMovementEvent = 1;
    std::optional<segelphysik::DynamischeFremdkraftKonfiguration> pendingForce;
    std::optional<WindChange> pendingWind;
    std::map<std::string, double> pendingSailAngles;
    QString testDriveStatus;

    mutable std::mutex mutex;
    std::vector<segelphysik::Zustand> states;
    struct SnapshotEntry {
        qsizetype index = -1;
        double captureMs = 0.;
        std::shared_ptr<const segelphysik::ErgebnisSnapshot> snapshot;
    };
    std::array<SnapshotEntry, 64> snapshots;
    bool finished = false;
    std::string status = "laufend";
    std::string diagnostic;
    double wallSeconds = 0.0;
    double coreSeconds = 0.0;
    double physicsStepSeconds = 0.0;
    double pacingSeconds = 0.0;
    double historyMutexWaitSeconds = 0.0;
    double workerCpuStart = -1.;
    double workerCpuSeconds = -1.;
    quint64 workerCpuCyclesStart = 0;
    quint64 workerCpuCycles = 0;
    double snapshotCaptureSeconds = 0.0;
    double snapshotCaptureMaximumMs = 0.0;
    quint64 snapshotCaptures = 0;
    mutable quint64 snapshotReaderHits = 0;
    mutable quint64 snapshotReaderMisses = 0;
    std::chrono::steady_clock::time_point started;
    bool startedValid = false;

    void publish(const segelphysik::Zustand& state, double stepCoreSeconds,
        std::shared_ptr<const segelphysik::ErgebnisSnapshot> snapshot = {},
        double physicsSeconds = 0., double waitSeconds = 0., double commandMutexWaitSeconds = 0.)
    {
        const auto captureStart = std::chrono::steady_clock::now();
        const bool capture = !snapshot;
        if (capture) snapshot = workerPreparation->snapshot(state);
        const auto captureSeconds = capture ? std::chrono::duration<double>(
            std::chrono::steady_clock::now() - captureStart).count() : 0.;
        std::shared_ptr<const segelphysik::ErgebnisSnapshot> retired;
        const auto lockStart = std::chrono::steady_clock::now();
        {
            const std::lock_guard<std::mutex> lock(mutex);
            historyMutexWaitSeconds += commandMutexWaitSeconds + std::chrono::duration<double>(
                std::chrono::steady_clock::now() - lockStart).count();
            physicsStepSeconds += physicsSeconds;
            pacingSeconds += waitSeconds;
            const auto index = states.size();
            states.push_back(state);
            auto& entry = snapshots[index % snapshots.size()];
            retired = std::move(entry.snapshot);
            entry.index = static_cast<qsizetype>(index);
            entry.captureMs = captureSeconds * 1000.;
            entry.snapshot = std::move(snapshot);
            coreSeconds += stepCoreSeconds + captureSeconds;
            snapshotCaptureSeconds += captureSeconds;
            snapshotCaptureMaximumMs = std::max(snapshotCaptureMaximumMs, entry.captureMs);
            if (capture) ++snapshotCaptures;
        }
    }

    void finish(const std::string& finalStatus, const std::string& detail)
    {
        const auto cpuEnd = currentThreadCpuSeconds();
        const auto cyclesEnd = currentThreadCpuCycles();
        const std::lock_guard<std::mutex> lock(mutex);
        workerCpuSeconds = cpuEnd >= 0. && workerCpuStart >= 0. ? cpuEnd - workerCpuStart : -1.;
        workerCpuCycles = cyclesEnd >= workerCpuCyclesStart ? cyclesEnd - workerCpuCyclesStart : 0;
        if (!pendingTestEvents.empty()) {
            testDriveStatus = QStringLiteral("Lauf beendet: %1 angeforderte Flagwechsel wurden nicht mehr angewandt.")
                .arg(static_cast<qulonglong>(pendingTestEvents.size()));
            pendingTestEvents.clear();
        }
        if (!pendingMovementEvents.empty()) {
            testDriveStatus = QStringLiteral("Lauf beendet: %1 Stellereignisse wurden nicht mehr angewandt.")
                .arg(static_cast<qulonglong>(pendingMovementEvents.size()));
            pendingMovementEvents.clear();
        }
        if (pendingForce) {
            testDriveStatus += QStringLiteral("\nLauf beendet: Fremdkraftaenderung wurde nicht mehr angewandt.");
            pendingForce.reset();
        }
        if (pendingWind) {
            testDriveStatus += QStringLiteral("\nLauf beendet: Windaenderung wurde nicht mehr angewandt.");
            pendingWind.reset();
        }
        if (!pendingSailAngles.empty()) {
            testDriveStatus += QStringLiteral("\nLauf beendet: Segelstellungen wurden nicht mehr angewandt.");
            pendingSailAngles.clear();
        }
        status = finalStatus;
        diagnostic = detail;
        finished = true;
        wallSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
    }
};

LiveSimulation::LiveSimulation()
    : impl_(std::make_unique<Impl>())
{
}

LiveSimulation::~LiveSimulation() = default;

std::unique_ptr<LiveSimulation> LiveSimulation::create(const QString& modelPath,
    const ModelValidation& validation, double durationSeconds,
    double stepSeconds, bool realtimePacing, QString* error, const QJsonObject& start,
    const std::optional<TestDriveOptions>& testDrive,
    std::shared_ptr<const segelphysik::ModelInspection> inspection)
{
    auto fail = [error](const QString& message) {
        if (error != nullptr) {
            *error = message;
        }
        return std::unique_ptr<LiveSimulation>();
    };
    if (!validation.valid) {
        return fail(QStringLiteral("Das Modell wurde nicht erfolgreich geprüft."));
    }
    if (!std::isfinite(durationSeconds) || durationSeconds < 0.0
        || !std::isfinite(stepSeconds) || stepSeconds <= 0.0) {
        return fail(QStringLiteral(
            "Dauer muss endlich und nicht negativ, Schrittweite endlich und "
            "positiv sein."));
    }
    if (durationSeconds > 0.0
        && durationSeconds / stepSeconds
            > static_cast<double>(maximumStates - 1)) {
        return fail(QStringLiteral(
            "Dauer und Schrittweite ergeben mehr als %1 Zustände.")
                .arg(maximumStates));
    }
    std::unique_ptr<LiveSimulation> live(new LiveSimulation);
    try {
        auto physicalStart = start;
        physicalStart.remove(QStringLiteral("dynamische_fremdkraft"));
        physicalStart.remove(QStringLiteral("wind_eingabe"));
        physicalStart.remove(QStringLiteral("sailforce_eingabe"));
        live->impl_->start = startState(physicalStart, true);
        const auto force = dynamicForceConfiguration(start, live->impl_->start);
        if (live->impl_->start.letzte_bewegungsereigniskennung
            == std::numeric_limits<std::size_t>::max())
            return fail(QStringLiteral("Bewegungs-Ereigniskennung erschoepft."));
        live->impl_->nextMovementEvent =
            live->impl_->start.letzte_bewegungsereigniskennung + 1;
        live->impl_->workerPreparation = std::make_unique<segelphysik::Modellaufbereitung>(
            inspection ? segelphysik::modellAusInspektion(*inspection)
                       : segelphysik::lade_modell(nativePath(modelPath)));
        const auto& model = live->impl_->workerPreparation->modell();
        if (model.id != validation.modelId.toStdString() ||
            model.fassung != validation.fingerprint.toStdString())
            return fail(QStringLiteral("Modellfassung hat sich seit der Prüfung geändert."));
        live->impl_->start = startWind(start, model, live->impl_->start);
        if (const auto config = sailforceInput(start))
            live->impl_->start = segelphysik::sailforce_start(
                model, live->impl_->start, nativePath(modelPath), *config);
        if (live->impl_->start.sailforce && !live->impl_->start.wind)
            live->impl_->start = segelphysik::wind_start(model, live->impl_->start,
                segelphysik::WindKonfiguration{}, std::nullopt, {},
                "Keine Mastreferenz; Punktwind fuer sailforce verfuegbar.");
        live->impl_->displayPreparation =
            std::make_unique<segelphysik::Modellaufbereitung>(model);
        if (testDrive) {
            live->impl_->start = segelphysik::testantrieb_start(model,
                live->impl_->start,
                segelphysik::TestantriebKonfiguration{testDrive->forceN, testDrive->speedLimit},
                testDrive->active);
            live->impl_->hasTestDrive = true;
            live->impl_->testDriveStatus = testDrive->active
                ? QStringLiteral("Testantrieb zum Laufstart aktiv.")
                : QStringLiteral("Testantrieb aus; im laufenden Versuch schaltbar.");
        }
        if (force)
            live->impl_->start = segelphysik::dynamische_fremdkraft_start(
                model, live->impl_->start, *force);
        const bool hasHydrodynamicHull = std::any_of(
            model.elemente.cbegin(),
            model.elemente.cend(),
            [](const segelphysik::Element& element) {
                return element.hydrodynamik_rolle == segelphysik::HydrodynamikRolle::rumpf;
            });
        if (hasHydrodynamicHull) {
            live->impl_->start = segelphysik::hydrodynamik_start(
                model, live->impl_->start,
                segelphysik::HydrodynamikKonfiguration{});
        }
        live->impl_->initialSnapshot =
            live->impl_->workerPreparation->snapshot(live->impl_->start);
        live->impl_->initialJson = parseObject(QByteArray::fromStdString(
            segelphysik::snapshot_json(*live->impl_->initialSnapshot, "laufend", {},
                segelphysik::ErgebnisJsonFormat::V6)),
            QStringLiteral("Vorbereiteter Core-Startzustand"));
    } catch (const std::exception& exception) {
        return fail(QString::fromUtf8(exception.what()));
    }
    if (QString::fromStdString(live->impl_->workerPreparation->modell().id) != validation.modelId
        || QString::fromStdString(live->impl_->workerPreparation->modell().fassung)
            != validation.fingerprint) {
        return fail(QStringLiteral(
            "Die Modelldatei wurde seit der Core-Prüfung verändert."));
    }
    live->impl_->modelId = validation.modelId;
    live->impl_->fingerprint = validation.fingerprint;
    live->impl_->durationSeconds = durationSeconds;
    live->impl_->stepSeconds = stepSeconds;
    live->impl_->realtimePacing = realtimePacing;
    return live;
}

void LiveSimulation::run(const std::atomic_bool& cancellation)
{
    using clock = std::chrono::steady_clock;
    auto& d = *impl_;
    {
        const std::lock_guard<std::mutex> lock(d.mutex);
        d.started = clock::now();
        d.startedValid = true;
        d.workerCpuStart = currentThreadCpuSeconds();
        d.workerCpuCyclesStart = currentThreadCpuCycles();
    }
    const bool endless = d.durationSeconds == 0.0;
    try {
        segelphysik::Zustand state = d.start;
        d.publish(state, 0.0, std::move(d.initialSnapshot));
        const double begin = state.zeit_s;
        const double end = begin + d.durationSeconds;
        std::size_t tick = 0;
        while (endless || state.zeit_s < end) {
            if (cancellation.load()) {
                d.finish("abgebrochen", {});
                return;
            }
            ++tick;
            const double target = endless
                ? begin + static_cast<double>(tick) * d.stepSeconds
                : std::min(end, begin + static_cast<double>(tick) * d.stepSeconds);
            const auto pacingStart = clock::now();
            if (d.realtimePacing) {
                // Erst rechnen, wenn die Wanduhr den Zielzeitpunkt erreicht:
                // keine Vorausrechnung; Abbruch bleibt reaktionsschnell.
                const auto due = d.started
                    + std::chrono::duration_cast<clock::duration>(
                        std::chrono::duration<double>(target - begin));
                while (clock::now() < due && !cancellation.load()) {
                    std::this_thread::sleep_until(std::min(
                        due, clock::now() + std::chrono::milliseconds(20)));
                }
                if (cancellation.load()) {
                    continue;
                }
            }
            const auto computeStart = clock::now();
            const double stepPacingSeconds = std::chrono::duration<double>(computeStart - pacingStart).count();
            std::vector<segelphysik::TestantriebEreignis> events;
            std::vector<segelphysik::Bewegungsereignis> movementEvents;
            std::optional<segelphysik::DynamischeFremdkraftKonfiguration> forceChange;
            std::optional<Impl::WindChange> windChange;
            std::map<std::string, double> sailAngles;
            double commandMutexWaitSeconds = 0.;
            const auto commandLockStart = clock::now();
            {
                const std::lock_guard<std::mutex> lock(d.mutex);
                commandMutexWaitSeconds = std::chrono::duration<double>(clock::now() - commandLockStart).count();
                events.swap(d.pendingTestEvents);
                movementEvents.swap(d.pendingMovementEvents);
                forceChange.swap(d.pendingForce);
                windChange.swap(d.pendingWind);
                sailAngles.swap(d.pendingSailAngles);
            }
            auto stepStart = forceChange
                ? segelphysik::dynamische_fremdkraft_aendern(
                    d.workerPreparation->modell(), state, *forceChange) : state;
            if (windChange) {
                std::optional<segelphysik::WindModus> mode;
                if (windChange->boatFixed)
                    mode = *windChange->boatFixed ? segelphysik::WindModus::bootmitgefuehrt
                                                  : segelphysik::WindModus::weltfest;
                stepStart = segelphysik::wind_aendern(stepStart, windChange->direction,
                    windChange->speed, mode);
            }
            for (const auto& angle : sailAngles)
                stepStart = segelphysik::sailforce_stellwinkel(stepStart, angle.first, angle.second);
            const auto physicsStart = clock::now();
            auto next = d.workerPreparation->schritt(stepStart,
                target - state.zeit_s, events, movementEvents);
            const double physicsSeconds = std::chrono::duration<double>(clock::now() - physicsStart).count();
            if (forceChange) {
                const std::lock_guard<std::mutex> lock(d.mutex);
                d.testDriveStatus = QStringLiteral("Fremdkraft am Uebergang %1 → %2 uebernommen.")
                    .arg(static_cast<qulonglong>(state.schritt))
                    .arg(static_cast<qulonglong>(next.schritt));
            }
            if (!events.empty()) {
                const std::lock_guard<std::mutex> lock(d.mutex);
                d.testDriveStatus = QStringLiteral("Testantrieb %1: Ereignis %2 am Uebergang %3 → %4 uebernommen.")
                    .arg(events.back().aktiv ? QStringLiteral("ein") : QStringLiteral("aus"))
                    .arg(static_cast<qulonglong>(events.back().kennung))
                    .arg(static_cast<qulonglong>(state.schritt))
                    .arg(static_cast<qulonglong>(next.schritt));
            }
            if (!(next.zeit_s > state.zeit_s)) {
                throw std::runtime_error("CORE-SIM-0001: Kein Zeitfortschritt.");
            }
            next.zeit_s = target;
            state = next;
            d.publish(state, std::chrono::duration<double>(
                clock::now() - computeStart).count(), {}, physicsSeconds,
                stepPacingSeconds, commandMutexWaitSeconds);
            if (static_cast<qsizetype>(tick) + 1 >= maximumStates) {
                d.finish("abgebrochen",
                    "Speichergrenze für aufgezeichnete Zustände erreicht.");
                return;
            }
        }
        d.finish("abgeschlossen", {});
    } catch (const std::exception& exception) {
        d.finish("fehlgeschlagen", exception.what());
    }
}

LiveSimulation::Progress LiveSimulation::progress() const
{
    const auto& d = *impl_;
    const std::lock_guard<std::mutex> lock(d.mutex);
    Progress result;
    result.states = static_cast<qsizetype>(d.states.size());
    result.finished = d.finished;
    result.status = QString::fromStdString(d.status);
    result.diagnostic = QString::fromStdString(d.diagnostic);
    result.testDriveStatus = d.testDriveStatus;
    const auto& latest = d.states.empty() ? d.start : d.states.back();
    if (latest.sailforce) result.sailforceRevision = static_cast<quint64>(latest.sailforce->revision);
    result.simulationSeconds = d.states.empty() ? 0.0 : d.states.back().zeit_s;
    result.coreSeconds = d.coreSeconds;
    result.physicsStepSeconds = d.physicsStepSeconds;
    result.pacingSeconds = d.pacingSeconds;
    result.historyMutexWaitSeconds = d.historyMutexWaitSeconds;
    result.workerCpuSeconds = d.workerCpuSeconds;
    result.workerCpuCycles = d.workerCpuCycles;
    result.snapshotCaptureSeconds = d.snapshotCaptureSeconds;
    result.snapshotCaptureMaximumMs = d.snapshotCaptureMaximumMs;
    result.snapshotCaptures = d.snapshotCaptures;
    result.snapshotReaderHits = d.snapshotReaderHits;
    result.snapshotReaderMisses = d.snapshotReaderMisses;
    result.wallSeconds = d.finished
        ? d.wallSeconds
        : (d.startedValid
            ? std::chrono::duration<double>(
                std::chrono::steady_clock::now() - d.started).count()
            : 0.0);
    return result;
}

QJsonObject LiveSimulation::initialStateJson() const
{
    return impl_->initialJson;
}

bool LiveSimulation::setTestDriveActive(bool active, QString* error)
{
    auto& d = *impl_;
    const std::lock_guard<std::mutex> lock(d.mutex);
    if (!d.hasTestDrive || d.finished) {
        if (error) *error = QStringLiteral("Kein laufender Versuch mit Testantriebskontext.");
        return false;
    }
    if (d.nextTestEvent == std::numeric_limits<std::size_t>::max()) {
        if (error) *error = QStringLiteral("Testantriebs-Ereigniskennung erschoepft.");
        return false;
    }
    d.pendingTestEvents.push_back({d.nextTestEvent++, active});
    return true;
}

bool LiveSimulation::setDynamicForce(bool active, const std::array<double, 3>& force,
    const std::array<double, 3>& point, QString* error)
{
    auto& d = *impl_;
    const std::lock_guard<std::mutex> lock(d.mutex);
    if (d.finished || !d.start.dynamische_fremdkraft) {
        if (error) *error = QStringLiteral("Keine laufende Simulation mit dynamischer Fremdkraft.");
        return false;
    }
    if (!std::all_of(force.begin(), force.end(), [](double v) { return std::isfinite(v); })
        || !std::all_of(point.begin(), point.end(), [](double v) { return std::isfinite(v); })) {
        if (error) *error = QStringLiteral("Fremdkraft und Ansatzpunkt muessen endlich sein.");
        return false;
    }
    if (active && d.hasTestDrive) {
        if (error) *error = QStringLiteral("Fremdkraft und Testantrieb duerfen nicht gleichzeitig aktiv sein.");
        return false;
    }
    d.pendingForce = segelphysik::DynamischeFremdkraftKonfiguration{active, force, point};
    return true;
}

bool LiveSimulation::setWind(std::optional<double> direction, std::optional<double> speed,
    std::optional<bool> boatFixed, QString* error)
{
    auto& d = *impl_;
    const std::lock_guard<std::mutex> lock(d.mutex);
    if (d.finished || !d.start.wind) {
        if (error) *error = QStringLiteral("Kein laufender Versuch mit Windkontext.");
        return false;
    }
    auto change = d.pendingWind.value_or(Impl::WindChange{});
    if (direction) change.direction = direction;
    if (speed) change.speed = speed;
    if (boatFixed) change.boatFixed = boatFixed;
    try {
        std::optional<segelphysik::WindModus> mode;
        if (change.boatFixed)
            mode = *change.boatFixed ? segelphysik::WindModus::bootmitgefuehrt
                                     : segelphysik::WindModus::weltfest;
        const auto& current = d.states.empty() ? d.start : d.states.back();
        (void)segelphysik::wind_aendern(current, change.direction, change.speed, mode);
    } catch (const std::invalid_argument& exception) {
        if (error) *error = QString::fromUtf8(exception.what());
        return false;
    }
    d.pendingWind = change;
    return true;
}

bool LiveSimulation::setSailAngle(const QString& elementId, double angle, QString* error)
{
    auto& d = *impl_;
    const std::lock_guard<std::mutex> lock(d.mutex);
    if (d.finished || !d.start.sailforce || !std::isfinite(angle)) {
        if (error) *error = QStringLiteral("Keine laufende Simulation mit gueltiger Segelstellung.");
        return false;
    }
    try {
        const auto& current = d.states.empty() ? d.start : d.states.back();
        (void)segelphysik::sailforce_stellwinkel(current, elementId.toStdString(), angle);
    } catch (const std::exception& exception) {
        if (error) *error = QString::fromUtf8(exception.what());
        return false;
    }
    d.pendingSailAngles[elementId.toStdString()] = angle;
    return true;
}

bool LiveSimulation::setMovementValue(const QString& elementId, double value, QString* error)
{
    auto& d = *impl_;
    const std::lock_guard<std::mutex> lock(d.mutex);
    if (d.finished) {
        if (error) *error = QStringLiteral("Keine laufende dynamische Simulation.");
        return false;
    }
    const auto id = elementId.toStdString();
    const auto& elements = d.workerPreparation->modell().elemente;
    const auto element = std::find_if(elements.begin(), elements.end(),
        [&](const segelphysik::Element& candidate) { return candidate.id == id; });
    if (element == elements.end() || !element->bewegung) {
        if (error) *error = QStringLiteral("Element besitzt keine gueltige Bewegungsdefinition.");
        return false;
    }
    const double minimum = element->bewegung->ruderwinkel ? -90.0 : 0.0;
    const double maximum = element->bewegung->ruderwinkel ? 90.0 : 100.0;
    if (!std::isfinite(value) || value < minimum || value > maximum) {
        if (error) *error = QStringLiteral("Stellwert liegt ausserhalb des zulaessigen Bereichs.");
        return false;
    }
    if (d.nextMovementEvent == std::numeric_limits<std::size_t>::max()) {
        if (error) *error = QStringLiteral("Bewegungs-Ereigniskennung erschoepft.");
        return false;
    }
    d.pendingMovementEvents.push_back(
        {d.nextMovementEvent++, id, value});
    return true;
}

QJsonObject LiveSimulation::stateJson(qsizetype index, QString* error, JsonTimings* timings) const
{
    if (timings) *timings = {};
    const auto& d = *impl_;
    segelphysik::Zustand state;
    std::shared_ptr<const segelphysik::ErgebnisSnapshot> snapshot;
    std::string status = "laufend";
    std::string diagnostic;
    {
        const std::lock_guard<std::mutex> lock(d.mutex);
        if (index < 0 || index >= static_cast<qsizetype>(d.states.size())) {
            if (error != nullptr) {
                *error = QStringLiteral("Zustand %1 ist nicht vorhanden.").arg(index);
            }

            return {};
        }
        const auto& entry = d.snapshots[static_cast<std::size_t>(index) % d.snapshots.size()];
        if (entry.index == index) snapshot = entry.snapshot;
        if (snapshot) ++d.snapshotReaderHits;
        else ++d.snapshotReaderMisses;
        if (snapshot && timings) timings->snapshotCaptureMs = entry.captureMs;
        if (!snapshot) state = d.states[static_cast<std::size_t>(index)];
        if (d.finished && index + 1 == static_cast<qsizetype>(d.states.size())) {
            status = d.status;
            diagnostic = d.diagnostic;
        }
    }

    try {
        const auto start = std::chrono::steady_clock::now();
        const auto json = snapshot
            ? segelphysik::snapshot_json(*snapshot, status, diagnostic,
                segelphysik::ErgebnisJsonFormat::V6)
            : d.displayPreparation->json(state, status, diagnostic, nullptr, nullptr,
                segelphysik::ErgebnisJsonFormat::V6);
        const auto serialized = std::chrono::steady_clock::now();
        auto result = parseObject(QByteArray::fromStdString(json), QStringLiteral("Core-Zustand"));
        if (timings) {
            timings->snapshotHit = static_cast<bool>(snapshot);
            timings->coreMs = std::chrono::duration<double, std::milli>(serialized - start).count();
            timings->decodeMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - serialized).count();
            timings->bytes = static_cast<qsizetype>(json.size());
        }
        return result;
    } catch (const std::exception& exception) {
        if (error != nullptr) {
            *error = QString::fromUtf8(exception.what());
        }
        return {};
    }
}

QVector<QJsonObject> LiveSimulation::motionStates(qsizetype begin, qsizetype end, QString* error) const
{
    const auto& d = *impl_;
    const std::lock_guard<std::mutex> lock(d.mutex);
    if (begin < 0 || end < begin || static_cast<std::size_t>(end) > d.states.size()) {
        if (error) *error = QStringLiteral("Ungueltiger Bereich fuer Fahrthistorie.");
        return {};
    }
    QVector<QJsonObject> result;
    result.reserve(end - begin);
    auto array = [](const auto& values) {
        QJsonArray a;
        for (double value : values) a.append(value);
        return a;
    };
    for (qsizetype i = begin; i < end; ++i) {
        const auto& s = d.states[static_cast<std::size_t>(i)];
        result.push_back({{QStringLiteral("zeit_s"), s.zeit_s},
            {QStringLiteral("position_m"), array(s.pose.position_m())},
            {QStringLiteral("orientierung_xyzw"), array(s.pose.orientierung_xyzw())},
            {QStringLiteral("geschwindigkeit_welt_m_s"), array(s.geschwindigkeit_welt_m_s)},
            {QStringLiteral("moment_bezug_modell_m"),
                array(d.displayPreparation->modell().schwerpunkt_m)}});
    }
    return result;
}