#include "model_catalog.hpp"

#include <segelphysik/core.hpp>

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <exception>
#include <filesystem>

namespace {

const QString modelFileName = QStringLiteral("modell.json");

std::filesystem::path nativePath(const QString& path)
{
    return std::filesystem::path(path.toStdWString());
}

QString text(const std::string& value)
{
    return QString::fromStdString(value);
}

QString diagnosticText(const segelphysik::KatalogDiagnose& diagnostic)
{
    return diagnostic.fundstelle.empty()
        ? QStringLiteral("%1: %2").arg(text(diagnostic.fehler_id), text(diagnostic.text))
        : QStringLiteral("%1: %2 (%3)")
              .arg(text(diagnostic.fehler_id), text(diagnostic.text),
                  text(diagnostic.fundstelle));
}

QFileInfoList modelDirectories(const QDir& dir)
{
    return dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name | QDir::IgnoreCase);
}

} // namespace

QVector<ModelProblem> problemsFromText(const QString& text)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*(CORE-[A-Z]+-\d+)(?:\s*\[([^\]]*)\])?\s*:?\s*(.*)$)"));
    QVector<ModelProblem> problems;
    for (const auto& line : text.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const auto match = pattern.match(line);
        ModelProblem problem;
        if (match.hasMatch()) {
            problem.code = match.captured(1);
            problem.location = match.captured(2).trimmed();
            problem.message = match.captured(3).trimmed();
        } else {
            problem.message = line.trimmed();
        }
        if (!problem.message.isEmpty() || !problem.code.isEmpty()) {
            problems.push_back(withErrorHelp(problem));
        }
    }
    return problems;
}

ModelProblem withErrorHelp(ModelProblem problem)
{
    struct Help {
        QString summary;
        QString remedy;
    };
    static const QHash<QString, Help> help = [] {
        QHash<QString, Help> result;
        const auto json = segelphysik::fehlerdokumentation_json();
        const auto document = QJsonDocument::fromJson(
            QByteArray(json.data(), static_cast<qsizetype>(json.size())));
        for (const auto& value : document.object().value(QStringLiteral("fehler")).toArray()) {
            const auto entry = value.toObject();
            result.insert(entry.value(QStringLiteral("id")).toString(),
                {entry.value(QStringLiteral("kurztext")).toString(),
                    entry.value(QStringLiteral("handlungsanweisung")).toString()});
        }
        return result;
    }();
    const auto it = help.constFind(problem.code);
    if (it != help.constEnd()) {
        problem.summary = it->summary;
        problem.remedy = it->remedy;
    }
    return problem;
}

CatalogSnapshot loadCatalog(const QString& folder)
{
    CatalogSnapshot snapshot;
    snapshot.folder = folder;
    if (folder.isEmpty()) {
        snapshot.error = QStringLiteral("Kein Modellordner gewählt.");
        return snapshot;
    }
    try {
        auto catalog = std::make_shared<segelphysik::Modellkatalog>(
            segelphysik::modellkatalog(nativePath(folder)));
        for (const auto& item : catalog->eintraege()) {
            ModelEntry entry;
            entry.id = text(item.modell_id);
            entry.name = item.name.empty() ? entry.id : text(item.name);
            entry.description = item.beschreibung ? text(*item.beschreibung) : QString();
            entry.version = text(item.modell_fassung);
            entry.usable = item.status == "verwendbar";
            entry.errorCount = static_cast<int>(item.fehleranzahl);
            try {
                entry.path = QString::fromStdWString(segelphysik::katalog_datei(
                    *catalog, item.modell_id, item.modell_fassung).wstring());
            } catch (const std::exception& error) {
                entry.diagnostics << QString::fromUtf8(error.what());
                entry.problems += problemsFromText(QString::fromUtf8(error.what()));
            }
            if (!entry.usable) {
                try {
                    for (const auto& diagnostic : segelphysik::modelldiagnose(
                             *catalog, item.modell_id, item.modell_fassung)) {
                        entry.diagnostics << diagnosticText(diagnostic);
                        entry.problems.push_back(withErrorHelp({text(diagnostic.fehler_id),
                            text(diagnostic.text), text(diagnostic.fundstelle), {}, {}}));
                    }
                } catch (const std::exception& error) {
                    entry.diagnostics << QString::fromUtf8(error.what());
                    entry.problems += problemsFromText(QString::fromUtf8(error.what()));
                }
            }
            entry.diagnostics.removeDuplicates();
            snapshot.entries.push_back(entry);
        }
        snapshot.catalog = std::move(catalog);
    } catch (const std::exception& error) {
        snapshot.error = QString::fromUtf8(error.what());
    }
    return snapshot;
}

QStringList watchedModelPaths(const QString& folder)
{
    QStringList paths;
    const QDir dir(folder);
    if (folder.isEmpty() || !dir.exists()) {
        return paths;
    }
    paths << dir.absolutePath();
    if (dir.exists(modelFileName)) {
        paths << dir.absoluteFilePath(modelFileName);
    }
    for (const auto& sub : modelDirectories(dir)) {
        const QDir subDir(sub.absoluteFilePath());
        if (!subDir.exists(modelFileName)) {
            continue;
        }
        paths << subDir.absolutePath();
        for (const auto& file : subDir.entryInfoList(QDir::Files)) {
            paths << file.absoluteFilePath();
        }
    }
    return paths;
}

ModelDisplayData loadDisplayData(const segelphysik::Modellkatalog& catalog,
    const QString& modelId, const QString& version)
{
    const auto id = modelId.toStdString();
    const auto fassung = version.toStdString();
    ModelDisplayData data;
    data.modelId = modelId;
    data.version = version;
    data.path = QString::fromStdWString(
        segelphysik::katalog_datei(catalog, id, fassung).wstring());
    data.geometry = segelphysik::darstellungsgeometrie(catalog, id, fassung);
    for (const auto& diagnostic : data.geometry.diagnosen) {
        data.partDiagnostics << diagnosticText(diagnostic);
    }
    try {
        const auto structure = segelphysik::modellstruktur(catalog, id, fassung);
        data.totalMassKg = structure.gesamtmasse_kg;
        data.bounds = structure.huellquader_m;
        for (const auto& group : structure.gruppen) {
            for (const auto& element : group.elemente) {
                data.elementGroups.insert(QString::fromStdString(element.id),
                    QString::fromStdString(group.id));
            }
        }
    } catch (const std::exception& error) {
        // Fehlerhafte Modelle bleiben ansehbar; dann fehlt nur die Kraftreferenz.
        data.partDiagnostics << QString::fromUtf8(error.what());
    }
    return data;
}

bool isOutdatedVersionError(const QString& message)
{
    return message.contains(QStringLiteral("CORE-KAT-0004"));
}
