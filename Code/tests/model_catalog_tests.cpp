#include "model_catalog.hpp"
#include <segelphysik/modell.hpp>
#include <segelphysik/modell_fingerprints.hpp>
#include <segelphysik/segelkonfigurator.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QSet>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>

#include <algorithm>
#include <limits>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <stdexcept>
#include <utility>

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

    // Legacy selection loads its Core inspection as before.
    const auto data = loadDisplayData(*snapshot.catalog, quader->id, quader->version);
    require(data.inspection != nullptr, "Display data must retain its Core inspection snapshot.");
    require(data.totalMassKg && *data.totalMassKg > 0.0, "Core delivered no total mass.");
    require(!data.geometry.teile.empty(), "Core delivered no display geometry.");
}

void test420Model02(const QString& folder)
{
    const auto snapshot = loadCatalog(folder);
    require(snapshot.error.isEmpty() && snapshot.catalog != nullptr,
        "420 model catalog could not be loaded.");
    const auto* boat = find(snapshot, QStringLiteral("420"));
    require(boat != nullptr && boat->usable && boat->errorCount == 0,
        "Core rejected the 420 schema 0.2 model in the GUI catalog.");
    require(QFileInfo(boat->path).fileName() == QStringLiteral("modells_information.json"),
        "GUI did not resolve the 420 schema 0.2 source file.");
    const auto data = loadDisplayData(*snapshot.catalog, boat->id, boat->version);
    require(data.inspection != nullptr && !data.geometry.teile.empty(),
        "GUI received no 420 inspection snapshot or display geometry.");
    require(data.totalMassKg && *data.totalMassKg > 0.0,
        "GUI received no valid 420 mass projection.");
    require(data.elementGroups.value(QStringLiteral("Ruder")) == QStringLiteral("ruder"),
        "GUI lost the 420 rudder group.");
    require(data.elementMovements.contains(QStringLiteral("Schwert"))
            && data.elementMovements.contains(QStringLiteral("Ruder")),
        "GUI lost explicit movement definitions from the 420 model.");
}

void testSimpleModels02(const QString& folder)
{
    const auto snapshot = loadCatalog(folder);
    require(snapshot.error.isEmpty() && snapshot.catalog != nullptr,
        "Simple model catalog could not be loaded.");
    for (const auto& id : {QStringLiteral("bleistift"), QStringLiteral("kegel"),
             QStringLiteral("kugel"), QStringLiteral("platte"), QStringLiteral("pyramide"),
             QStringLiteral("quader"), QStringLiteral("rolle"), QStringLiteral("unregelmaessig")}) {
        const auto* entry = find(snapshot, id);
        require(entry != nullptr && entry->usable && entry->errorCount == 0,
            "Core rejected a simple schema 0.2 model in the GUI catalog.");
        require(QFileInfo(entry->path).fileName() == QStringLiteral("modells_information.json"),
            "GUI did not resolve a simple schema 0.2 source file.");
        const auto data = loadDisplayData(*snapshot.catalog, entry->id, entry->version);
        require(data.inspection != nullptr && !data.geometry.teile.empty(),
            "GUI received no inspection snapshot or display geometry for a simple model.");
    }
}

void testModelFormat03(const QString& folder, qsizetype expectedModels)
{
    const auto snapshot = loadCatalog(folder);
    require(snapshot.error.isEmpty() && snapshot.catalog != nullptr,
        "0.3 model catalog could not be loaded.");
    require(snapshot.entries.size() == expectedModels, "Unexpected number of 0.3 models.");
    for (const auto& entry : snapshot.entries) {
        require(entry.schemaValid && entry.fingerprintsCurrent && entry.usable
                && entry.errorCount == 0,
            "Core did not fully validate and release a 0.3 model.");
        require(QFileInfo(entry.path).fileName() == QStringLiteral("modells_data.json"),
            "Core did not resolve the 0.3 model data file.");
    }

    const auto* boat = find(snapshot, QStringLiteral("420"));
    require(boat != nullptr, "0.3 catalog omitted the 420.");
    const auto display = loadDisplayData(*snapshot.catalog, boat->id, boat->version);
    require(display.modelFormat03 && display.inspection == nullptr
            && !display.structure.gruppen.empty() && !display.geometry.teile.empty(),
        "0.3 model selection did not immediately provide tree and display data.");
    const auto inspection = segelphysik::readModelInspection(
        *snapshot.catalog, boat->id.toStdString(), boat->version.toStdString());
    const auto configuration = segelphysik::leseSegelKonfiguration(*snapshot.catalog, *inspection);
    const auto configured = segelphysik::leseKonfigurierteSegelgeometrie(configuration);
    require(configured.status == segelphysik::SegelErgebnisStatus::Vorschau,
        "Core could not provide configured sail geometry.");
    for (const auto& sail : {QStringLiteral("Mainsail"), QStringLiteral("Jib")}) {
        const auto part = std::find_if(display.geometry.teile.begin(), display.geometry.teile.end(),
            [&sail](const auto& candidate) {
                return QString::fromStdString(candidate.element_id) == sail;
            });
        require(part != display.geometry.teile.end(),
            "420 display geometry omitted a sail.");
        const auto mesh = std::find_if(configured.meshes.begin(), configured.meshes.end(),
            [&sail](const auto& candidate) {
                return QString::fromStdString(candidate.element_id) == sail;
            });
        require(mesh != configured.meshes.end() && !mesh->points_m.empty()
                && mesh->points_m.size() == part->punkte_m.size(),
            "420 configured sail geometry is missing or incomplete.");
        for (std::size_t i = 0; i < mesh->points_m.size(); ++i)
            for (std::size_t axis = 0; axis < 3; ++axis)
                require(std::abs(mesh->points_m[i][axis] - part->punkte_m[i][axis]) < 1e-8,
                    "420 display geometry omitted its configured placement.");
    }
    require(!display.totalMassKg, "0.3 display phase precalculated physics.");
    bool hasHydrodynamicReference = false;
    bool hasAerodynamicReference = false;
    for (const auto& group : display.structure.gruppen)
        for (const auto& element : group.elemente) {
            hasHydrodynamicReference = hasHydrodynamicReference ||
                element.referenz_hydrodynamik;
            hasAerodynamicReference = hasAerodynamicReference ||
                element.referenz_aerodynamik;
        }
    require(hasHydrodynamicReference && hasAerodynamicReference,
        "Object tree lost hydrodynamic or aerodynamic reference markers.");
    auto selected = display;
    loadSelectedModelPhysics(selected, *snapshot.catalog);
    require(selected.inspection != nullptr && selected.totalMassKg
            && *selected.totalMassKg > 0.0,
        "Selected yacht did not load physics through the Core.");

    const auto* source = find(snapshot, QStringLiteral("bleistift"));
    require(source != nullptr && source->schemaValid, "bleistift must be schema-valid.");
    QTemporaryDir temporary;
    require(temporary.isValid(), "Temporary 0.3 model folder unavailable.");
    for (const auto& schema : {QStringLiteral("schema_data.json"),
             QStringLiteral("schema_validation.json")}) {
        require(QFile::copy(QDir(folder).filePath(schema),
                    QDir(temporary.path()).filePath(schema)),
            "Cannot copy 0.3 schema.");
    }
    const auto copiedModel = QDir(temporary.path()).filePath(QStringLiteral("bleistift"));
    copyTree(QFileInfo(source->path).absolutePath(), copiedModel);
    const auto aeroFolder = QDir(copiedModel).filePath(
        QStringLiteral("modells/aerodynamics"));
    require(QDir().mkpath(aeroFolder), "Cannot create aerodynamics fixture.");
    QFile deferredAero(QDir(aeroFolder).filePath(QStringLiteral("unparsed.json")));
    require(deferredAero.open(QIODevice::WriteOnly), "Cannot create deferred aerodynamics data.");
    require(deferredAero.write("{ invalid and intentionally unparsed") > 0,
        "Cannot write deferred aerodynamics data.");
    deferredAero.close();
    const auto deferredCatalog = loadCatalog(temporary.path());
    const auto* deferredEntry = find(deferredCatalog, QStringLiteral("bleistift"));
    require(deferredEntry && deferredEntry->usable && deferredEntry->fingerprintsCurrent,
        "Deferred aerodynamics data must not prevent use of the model.");
    require(QDir(aeroFolder).removeRecursively(), "Cannot remove deferred aerodynamics fixture.");
    const auto previousSchemaRoot = qgetenv("SEGELPHYSIK_TEST_SCHEMA_ROOT");
    const auto previousDefaultsFile = qgetenv("SEGELPHYSIK_TEST_DEFAULTS_FILE");
    const auto restoreEnvironment = qScopeGuard([&] {
        qputenv("SEGELPHYSIK_TEST_SCHEMA_ROOT", previousSchemaRoot);
        qputenv("SEGELPHYSIK_TEST_DEFAULTS_FILE", previousDefaultsFile);
    });
    require(qputenv("SEGELPHYSIK_TEST_SCHEMA_ROOT", temporary.path().toUtf8()),
        "Cannot isolate 0.3 schema root.");
    require(qputenv("SEGELPHYSIK_TEST_DEFAULTS_FILE",
        QDir(temporary.path()).filePath(QStringLiteral("defaults.json")).toUtf8()),
        "Cannot isolate defaults file.");
    segelphysik::aktualisiere_modell_fingerprints(
        std::filesystem::path(copiedModel.toStdWString()),
        {segelphysik::ModellFingerprintBereich::visualisation,
         segelphysik::ModellFingerprintBereich::mass,
         segelphysik::ModellFingerprintBereich::hydrostatics,
         segelphysik::ModellFingerprintBereich::aerodynamics,
         segelphysik::ModellFingerprintBereich::modelData});
    QFile validation(QDir(copiedModel).filePath(QStringLiteral("modells_validation.json")));
    require(validation.open(QIODevice::ReadOnly), "Cannot read 0.3 validation data.");
    QJsonParseError parseError;
    auto document = QJsonDocument::fromJson(validation.readAll(), &parseError);
    validation.close();
    require(parseError.error == QJsonParseError::NoError && document.isObject(),
        "Cannot parse 0.3 validation data.");
    auto root = document.object();
    auto model = root.value(QStringLiteral("model")).toObject();
    auto fingerprints = model.value(QStringLiteral("fingerprints")).toObject();
    auto modelData = fingerprints.value(QStringLiteral("modelData")).toObject();
    modelData.insert(QStringLiteral("hash"), QString(64, QLatin1Char('0')));
    fingerprints.insert(QStringLiteral("modelData"), modelData);
    model.insert(QStringLiteral("fingerprints"), fingerprints);
    root.insert(QStringLiteral("model"), model);
    require(validation.open(QIODevice::WriteOnly | QIODevice::Truncate),
        "Cannot write 0.3 validation data.");
    require(validation.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) >= 0,
        "Cannot update 0.3 validation fingerprint.");
    validation.close();

    const auto stale = loadCatalog(temporary.path());
    const auto* entry = find(stale, QStringLiteral("bleistift"));
    require(entry && entry->schemaValid && entry->visualizationHashCurrent
            && !entry->fingerprintsCurrent && !entry->usable,
        "Stale physics fingerprint did not separate view and simulation status.");
    const auto staleDisplay = loadDisplayData(*stale.catalog, entry->id, entry->version);
    require(!staleDisplay.inspection && !staleDisplay.geometry.teile.empty(),
        "Stale physics fingerprint incorrectly blocked model inspection.");

    auto visual = fingerprints.value(QStringLiteral("visualisation")).toObject();
    visual.insert(QStringLiteral("hash"), QString(64, QLatin1Char('0')));
    fingerprints.insert(QStringLiteral("visualisation"), visual);
    model.insert(QStringLiteral("fingerprints"), fingerprints);
    root.insert(QStringLiteral("model"), model);
    require(validation.open(QIODevice::WriteOnly | QIODevice::Truncate),
        "Cannot write stale visualization fingerprint.");
    require(validation.write(QJsonDocument(root).toJson()) >= 0, "Cannot write validation.");
    validation.close();
    const auto blocked = loadCatalog(temporary.path());
    const auto* blockedEntry = find(blocked, QStringLiteral("bleistift"));
    require(blockedEntry && blockedEntry->schemaValid && !blockedEntry->visualizationHashCurrent,
        "Stale visualization fingerprint did not block model view.");
    const auto blockedDisplay =
        loadDisplayData(*blocked.catalog, blockedEntry->id, blockedEntry->version);
    require(!blockedDisplay.inspection && blockedDisplay.geometry.teile.empty(),
        "Blocked visualization still exposes geometry.");
    const auto updated = segelphysik::aktualisiere_modell_fingerprints(
        std::filesystem::path(copiedModel.toStdWString()),
        {segelphysik::ModellFingerprintBereich::visualisation});
    require(updated.werte.size() == 1 && updated.validierungsdatei_aktualisiert,
        "Visualization-only refresh did not update exactly one area.");
    const auto refreshed = loadCatalog(temporary.path());
    const auto* refreshedEntry = find(refreshed, QStringLiteral("bleistift"));
    require(refreshedEntry && refreshedEntry->visualizationHashCurrent
        && !refreshedEntry->fingerprintsCurrent && !refreshedEntry->usable,
        "Visualization refresh incorrectly unlocked physics.");
    QFile data(QDir(copiedModel).filePath(QStringLiteral("modells_data.json")));
    require(data.open(QIODevice::WriteOnly | QIODevice::Truncate), "Cannot damage test model.");
    require(data.write("{") == 1, "Cannot write invalid JSON fixture.");
    data.close();
    const auto invalid = loadCatalog(temporary.path());
    const auto* invalidEntry = find(invalid, QStringLiteral("bleistift"));
    require(invalidEntry && !invalidEntry->schemaValid
        && !invalidEntry->diagnostics.isEmpty(), "Invalid JSON loses its Core diagnostics.");
    const auto invalidDisplay =
        loadDisplayData(*invalid.catalog, invalidEntry->id, invalidEntry->version);
    require(!invalidDisplay.inspection && invalidDisplay.geometry.teile.empty(),
        "Schema-invalid model exposes inspection geometry.");
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
        QFile file(dir.filePath(QStringLiteral("kaputt/modells_information.json")));
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
    const auto opened = loadDisplayData(*snapshot.catalog, quader->id, oldVersion);
    const auto fixedTree = segelphysik::inspectionTreeJson(*opened.inspection, "modell");
    const auto fixedMesh = segelphysik::inspectionMeshJson(*opened.inspection, "darstellung");
    {
        QFile file(dir.filePath(QStringLiteral("quader/modells_information.json")));
        require(file.open(QIODevice::ReadOnly), "Cannot read model.");
        auto model = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
        model.insert(QStringLiteral("name"), QStringLiteral("Changed reference model"));
        require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "Cannot modify model.");
        const auto bytes = QJsonDocument(model).toJson();
        require(file.write(bytes) == bytes.size(), "Cannot write changed model.");
    }
    bool rejected = false;
    try {
        static_cast<void>(loadDisplayData(*snapshot.catalog, QStringLiteral("quader"), oldVersion));
    } catch (const std::exception& error) {
        rejected = isOutdatedVersionError(QString::fromUtf8(error.what()));
    }
    require(rejected, "Outdated model version was not reported as CORE-KAT-0004.");
    require(segelphysik::inspectionTreeJson(*opened.inspection, "modell") == fixedTree
        && segelphysik::inspectionMeshJson(*opened.inspection, "darstellung") == fixedMesh,
        "Existing inspection changed when a new legacy load detected changed source files.");
    snapshot = loadCatalog(root.path());
    quader = find(snapshot, QStringLiteral("quader"));
    require(quader != nullptr && quader->version != oldVersion,
        "Reloaded catalog did not report a new model version.");
    static_cast<void>(loadDisplayData(*snapshot.catalog, quader->id, quader->version));

    require(QFile::remove(dir.filePath(QStringLiteral("kaputt/modells_information.json"))),
        "Cannot remove test model.");
    require(loadCatalog(root.path()).entries.size() == 1, "Removed model is still listed.");
}

QJsonValue renameIdentity(const QJsonValue& value)
{
    if (value.isString())
        return value.toString() == QStringLiteral("Ruder")
            ? QJsonValue(QStringLiteral("teil_017")) : value;
    if (value.isArray()) {
        QJsonArray result;
        for (const auto& entry : value.toArray()) result.append(renameIdentity(entry));
        return result;
    }
    if (value.isObject()) {
        auto object = value.toObject();
        for (auto it = object.begin(); it != object.end(); ++it)
            it.value() = renameIdentity(it.value());
        return object;
    }
    return value;
}

} // namespace

void testProblemParsing()
{
    const auto problems = problemsFromText(QStringLiteral(
        "CORE-MOD-0007 [modells_information.json, Gruppe rigg, Element Spriet]: Ueberschneidung; Vergleich mit Mast\n"
        "freier Text ohne Kennung"));
    require(problems.size() == 2, "Two problems expected.");
    require(problems[0].code == QStringLiteral("CORE-MOD-0007"), "Code not parsed.");
    require(problems[0].location == QStringLiteral("modells_information.json, Gruppe rigg, Element Spriet"),
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
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--420-v02")) {
        try {
            test420Model02(QString::fromLocal8Bit(argv[2]));
            std::cout << "420 schema 0.2 model via GUI catalog passed.\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--simple-v02")) {
        try {
            testSimpleModels02(QString::fromLocal8Bit(argv[2]));
            std::cout << "Simple schema 0.2 models via GUI catalog passed.\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
    if (argc == 3 && (QString::fromLocal8Bit(argv[1]) == QStringLiteral("--model-format-03")
        || QString::fromLocal8Bit(argv[1]) == QStringLiteral("--model-format-03-single"))) {
        try {
            testModelFormat03(QString::fromLocal8Bit(argv[2]),
                QString::fromLocal8Bit(argv[1]) == QStringLiteral("--model-format-03") ? 11 : 1);
            std::cout << "Model format 0.3 via Core catalog passed.\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
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
