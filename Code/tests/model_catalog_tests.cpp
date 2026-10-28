#include "model_catalog.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>

#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void copyTree(const QString& source, const QString& target)
{
    require(QDir().mkpath(target), "mkdir failed.");
    QDirIterator it(source, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString file = it.next();
        const QString relative = QDir(source).relativeFilePath(file);
        const QString destination = QDir(target).filePath(relative);
        require(QDir().mkpath(QFileInfo(destination).absolutePath()), "mkdir failed.");
        require(QFile::copy(file, destination), "Cannot copy reference model.");
    }
}

const ModelEntry* find(const CatalogSnapshot& snapshot, const QString& id)
{
    for (const auto& entry : snapshot.entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

void testReferenceFolder(const QString& folder)
{
    const auto snapshot = loadCatalog(folder);
    require(snapshot.error.isEmpty(), "Reference catalog reported an error.");
    require(snapshot.catalog != nullptr, "Reference catalog missing.");
    require(snapshot.entries.size() >= 2, "Reference folder lists too few models.");
    const auto* quader = find(snapshot, QStringLiteral("quader"));
    require(quader != nullptr, "Reference folder omitted the quader model.");
    require(quader->usable, "quader is not usable.");
    require(!quader->name.isEmpty(), "quader has no display name.");
    require(!quader->version.isEmpty(), "quader has no model version.");
    require(QFile::exists(quader->path), "Resolved model file does not exist.");
    for (const auto& id : {QStringLiteral("optimist"), QStringLiteral("420"),
             QStringLiteral("folkeboot")}) {
        const auto* boat = find(snapshot, id);
        require(boat != nullptr, "Reference folder omitted boat model.");
        if (!boat->usable || boat->errorCount != 0) {
            throw std::runtime_error(
                "Core rejected boat model in GUI catalog: " + id.toStdString());
        }
    }
    require(watchedModelPaths(folder).size() > snapshot.entries.size(),
        "Folder watch list misses model directories or files.");

    // Struktur auf Abruf (A-G-00021): Masse und Hüllquader vom Core.
    const auto data = loadDisplayData(*snapshot.catalog, quader->id, quader->version);
    require(data.totalMassKg && *data.totalMassKg > 0.0, "Core delivered no total mass.");
    require(!data.geometry.teile.empty(), "Core delivered no display geometry.");
}

void testFolderChanges(const QString& referenceFolder)
{
    QTemporaryDir root;
    require(root.isValid(), "Temporary folder unavailable.");
    const QDir dir(root.path());
    require(loadCatalog(root.path()).entries.isEmpty(), "Empty folder lists models.");

    copyTree(QDir(referenceFolder).filePath(QStringLiteral("quader")),
        dir.filePath(QStringLiteral("quader")));
    require(dir.mkpath(QStringLiteral("kaputt")), "mkdir failed.");
    {
        QFile file(dir.filePath(QStringLiteral("kaputt/modell.json")));
        require(file.open(QIODevice::WriteOnly), "Cannot write broken model.");
        file.write("{}");
    }

    auto snapshot = loadCatalog(root.path());
    require(snapshot.entries.size() == 2, "Catalog does not list both models.");
    const auto* quader = find(snapshot, QStringLiteral("quader"));
    require(quader != nullptr && quader->usable, "Copied quader is not usable.");
    const ModelEntry* broken = nullptr;
    for (const auto& entry : snapshot.entries) {
        if (!entry.usable) {
            broken = &entry;
        }
    }
    require(broken != nullptr, "Broken model is not listed as faulty (A-G-00020).");
    require(broken->errorCount > 0, "Broken model has no error count.");
    require(!broken->diagnostics.isEmpty(), "Broken model has no diagnostics.");
    require(!broken->problems.isEmpty() && broken->problems.front().code.startsWith(QStringLiteral("CORE-")),
        "Broken model has no structured problems.");
    require(!broken->problems.front().remedy.isEmpty(),
        "Problem lacks the remedy from the Core error catalog.");

    // Geänderte Modelldatei: alte Fassung wird abgelehnt (A-G-00022).
    const QString oldVersion = quader->version;
    {
        QFile file(dir.filePath(QStringLiteral("quader/modell.json")));
        require(file.open(QIODevice::Append), "Cannot modify model.");
        file.write("\n");
    }
    bool rejected = false;
    try {
        static_cast<void>(loadDisplayData(*snapshot.catalog, QStringLiteral("quader"), oldVersion));
    } catch (const std::exception& error) {
        rejected = isOutdatedVersionError(QString::fromUtf8(error.what()));
    }
    require(rejected, "Outdated model version was not reported as CORE-KAT-0004.");
    snapshot = loadCatalog(root.path());
    quader = find(snapshot, QStringLiteral("quader"));
    require(quader != nullptr && quader->version != oldVersion,
        "Reloaded catalog did not report a new model version.");
    static_cast<void>(loadDisplayData(*snapshot.catalog, quader->id, quader->version));

    require(QFile::remove(dir.filePath(QStringLiteral("kaputt/modell.json"))),
        "Cannot remove test model.");
    require(loadCatalog(root.path()).entries.size() == 1, "Removed model is still listed.");
}

} // namespace

void testProblemParsing()
{
    const auto problems = problemsFromText(QStringLiteral(
        "CORE-MOD-0007 [modell.json, Gruppe rigg, Element Spriet]: Ueberschneidung; Vergleich mit Mast\n"
        "freier Text ohne Kennung"));
    require(problems.size() == 2, "Two problems expected.");
    require(problems[0].code == QStringLiteral("CORE-MOD-0007"), "Code not parsed.");
    require(problems[0].location == QStringLiteral("modell.json, Gruppe rigg, Element Spriet"),
        "Location not parsed.");
    require(problems[0].message.startsWith(QStringLiteral("Ueberschneidung")), "Message not parsed.");
    require(!problems[0].summary.isEmpty() && !problems[0].remedy.isEmpty(),
        "Error catalog help missing.");
    require(problems[1].code.isEmpty() && problems[1].message == QStringLiteral("freier Text ohne Kennung"),
        "Free text not kept.");
}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (argc != 2) {
        std::cerr << "Expected the reference model folder.\n";
        return 2;
    }
    try {
        const QString folder = QString::fromLocal8Bit(argv[1]);
        testProblemParsing();
        testReferenceFolder(folder);
        testFolderChanges(folder);
        std::cout << "Model catalog via core passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
