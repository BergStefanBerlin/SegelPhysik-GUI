#include "core_adapter.hpp"

#include "segelphysik/modell.hpp"
#include "segelphysik/simulation.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStringList>
#include <QUuid>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

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

} // namespace

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
    double stepSeconds, std::atomic_bool* cancellation)
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
        segelphysik::Laufoptionen options;
        options.dynamisch = dynamic;
        options.dauer_s = durationSeconds;
        options.dt_s = stepSeconds;
        options.jsonl = true;
        options.abbruch = cancellation;

        const auto report = segelphysik::simulieren(
            nativePath(modelPath), nativePath(outputPath), options);
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
            if (state.value(QStringLiteral("vertragsversion")).toString()
                    != QStringLiteral("1")
                || state.value(QStringLiteral("modell_id")).toString()
                    != validation.modelId
                || state.value(QStringLiteral("modell_fassung")).toString()
                    != validation.fingerprint
                || !hasFiniteNumber(state, QStringLiteral("zeit_s"))) {
                throw std::runtime_error(
                    "Core-Zustand hat eine inkompatible Vertragsversion, "
                    "Modellzuordnung oder Simulationszeit.");
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
    segelphysik::Modell workerModel;
    segelphysik::Modell displayModel;
    QString modelId;
    QString fingerprint;
    double durationSeconds = 0.0;
    double stepSeconds = 0.01;
    bool realtimePacing = true;

    mutable std::mutex mutex;
    std::vector<segelphysik::Zustand> states;
    bool finished = false;
    std::string status = "laufend";
    std::string diagnostic;
    double wallSeconds = 0.0;
    double coreSeconds = 0.0;
    std::chrono::steady_clock::time_point started;
    bool startedValid = false;

    void publish(const segelphysik::Zustand& state, double stepCoreSeconds)
    {
        const std::lock_guard<std::mutex> lock(mutex);
        states.push_back(state);
        coreSeconds += stepCoreSeconds;
    }

    void finish(const std::string& finalStatus, const std::string& detail)
    {
        const std::lock_guard<std::mutex> lock(mutex);
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
    double stepSeconds, bool realtimePacing, QString* error)
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
        live->impl_->workerModel = segelphysik::lade_modell(nativePath(modelPath));
        live->impl_->displayModel = live->impl_->workerModel;
    } catch (const std::exception& exception) {
        return fail(QString::fromUtf8(exception.what()));
    }
    if (QString::fromStdString(live->impl_->workerModel.id) != validation.modelId
        || QString::fromStdString(live->impl_->workerModel.fassung)
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
    }
    const bool endless = d.durationSeconds == 0.0;
    try {
        segelphysik::Zustand state;
        d.publish(state, 0.0);
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
            auto next = segelphysik::dynamikschritt(
                d.workerModel, state, target - state.zeit_s);
            if (!(next.zeit_s > state.zeit_s)) {
                throw std::runtime_error("CORE-SIM-0001: Kein Zeitfortschritt.");
            }
            next.zeit_s = target;
            state = next;
            d.publish(state, std::chrono::duration<double>(
                clock::now() - computeStart).count());
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
    result.simulationSeconds = d.states.empty() ? 0.0 : d.states.back().zeit_s;
    result.coreSeconds = d.coreSeconds;
    result.wallSeconds = d.finished
        ? d.wallSeconds
        : (d.startedValid
            ? std::chrono::duration<double>(
                std::chrono::steady_clock::now() - d.started).count()
            : 0.0);
    return result;
}

QJsonObject LiveSimulation::stateJson(qsizetype index, QString* error) const
{
    const auto& d = *impl_;
    segelphysik::Zustand state;
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
        state = d.states[static_cast<std::size_t>(index)];
        if (d.finished && index + 1 == static_cast<qsizetype>(d.states.size())) {
            status = d.status;
            diagnostic = d.diagnostic;
        }
    }
    try {
        return parseObject(QByteArray::fromStdString(segelphysik::zustand_json(
                               d.displayModel, state, status, diagnostic)),
            QStringLiteral("Core-Zustand"));
    } catch (const std::exception& exception) {
        if (error != nullptr) {
            *error = QString::fromUtf8(exception.what());
        }
        return {};
    }
}