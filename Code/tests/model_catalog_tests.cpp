#include "model_catalog.hpp"
#include <segelphysik/modell.hpp>

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

#include <iostream>
#include <cmath>
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

bool isSingleSource(const QString& path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Cannot read model schema.");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    require(error.error == QJsonParseError::NoError && document.isObject(),
        "Cannot parse model schema.");
    return document.object().value(QStringLiteral("schema_version")).toString()
        == QStringLiteral("3");
}

void prepareUnifiedModel(const QString& source, const QString& target)
{
    if (isSingleSource(source))
        copyTree(QFileInfo(source).absolutePath(), target);
    else
        segelphysik::konvertiere_modell_v2(std::filesystem::path(source.toStdWString()),
            std::filesystem::path(target.toStdWString()));
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
    require(data.elementMovements.isEmpty(),
        "GUI must not infer movement absent from the 420 model.");
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

void testIdentityIndependentOfRole(const QString& referenceFolder)
{
    QTemporaryDir root;
    require(root.isValid(), "Temporary neutral-ID model folder.");
    const auto target = QDir(root.path()).filePath(QStringLiteral("v2"));
    prepareUnifiedModel(QDir(referenceFolder).filePath(QStringLiteral("420/modells_information.json")), target);
    QFile file(QDir(target).filePath(QStringLiteral("modells_information.json")));
    require(file.open(QIODevice::ReadOnly), "Read neutral-ID source fixture.");
    const auto original = QJsonDocument::fromJson(file.readAll());
    file.close();
    require(!original.isNull(), "Parse neutral-ID source fixture.");
    const auto renamed = renameIdentity(original.object()).toObject();
    require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "Write neutral-ID fixture.");
    file.write(QJsonDocument(renamed).toJson());
    file.close();
    const auto catalog = loadCatalog(target);
    require(catalog.error.isEmpty() && catalog.entries.size() == 1
        && catalog.entries.front().usable, "Neutral identity must not change model validation.");
    const auto& entry = catalog.entries.front();
    const auto display = loadDisplayData(*catalog.catalog, entry.id, entry.version);
    require(display.elementGroups.contains(QStringLiteral("teil_017")),
        "Group projection must follow neutral element identity.");
    require(display.elementGroups.value(QStringLiteral("teil_017")) == QStringLiteral("ruder"),
        "Semantic rudder group must remain inspectable for neutral identity.");
}

void testUnifiedModels(const QString& folder)
{
    const auto legacy = loadCatalog(folder);
    QTemporaryDir root;
    require(root.isValid(), "Temporary unified model folder.");
    for (const auto& entry : legacy.entries) {
        const auto target = QDir(root.path()).filePath(entry.id);
        prepareUnifiedModel(entry.path, target);
    }
    const auto unified = loadCatalog(root.path());
    require(unified.error.isEmpty() && unified.entries.size() == legacy.entries.size(),
        "Unified GUI catalog must retain all reference models.");
    for (const auto& entry : legacy.entries) {
        const auto* converted = find(unified, entry.id);
        require(converted && converted->usable == entry.usable,
            "Unified GUI catalog must retain model validation.");
        const auto before = loadDisplayData(*legacy.catalog, entry.id, entry.version);
        const auto after = loadDisplayData(*unified.catalog, converted->id, converted->version);
        require(before.totalMassKg == after.totalMassKg && before.bounds == after.bounds
            && before.elementGroups == after.elementGroups
            && before.elementMovements.size() == after.elementMovements.size()
            && before.geometry.teile.size() == after.geometry.teile.size(),
            "GUI projections must preserve mass, bounds, groups, movements and display parts.");
        for (const auto& type : segelphysik::getAvailableMeshTypes(*before.inspection)) {
            if (type.id == "quelle") continue;
            const auto oldMeshes = segelphysik::getMesh(*before.inspection, type.id);
            const auto newMeshes = segelphysik::getMesh(*after.inspection, type.id);
            require(oldMeshes.size() == newMeshes.size(), "Unified mesh count changed.");
            for (std::size_t i = 0; i < oldMeshes.size(); ++i)
                require(oldMeshes[i].element_id == newMeshes[i].element_id
                    && oldMeshes[i].points_m == newMeshes[i].points_m
                    && oldMeshes[i].triangles == newMeshes[i].triangles
                    && oldMeshes[i].color_rgb == newMeshes[i].color_rgb
                    && oldMeshes[i].surface == newMeshes[i].surface,
                    "Unified GUI mesh geometry or material changed.");
        }
    }
}

void testSingleSourceModels(const QString& folder)
{
    const auto legacy = loadCatalog(folder);
    QTemporaryDir root;
    require(root.isValid(), "Temporary single-source model folder.");
    int migrated = 0, conflicts = 0;
    for (const auto& entry : legacy.entries) {
        const auto target = QDir(root.path()).filePath(entry.id);
        const bool conflict = !isSingleSource(entry.path) && (entry.id == QStringLiteral("420")
            || entry.id == QStringLiteral("folkeboot") || entry.id == QStringLiteral("optimist"));
        try {
            segelphysik::konvertiere_modell_v3(std::filesystem::path(entry.path.toStdWString()),
                std::filesystem::path(target.toStdWString()));
            require(!conflict, "Conflicting boat must not silently migrate.");
            ++migrated;
        } catch (const std::runtime_error& error) {
            if (!conflict || std::string(error.what()).find("CORE-MOD-0009") == std::string::npos) throw;
            require(!QDir(target).exists(), "Conflicting migration must not create output folder.");
            segelphysik::konvertiere_modell_v3(std::filesystem::path(entry.path.toStdWString()),
                std::filesystem::path(target.toStdWString()), true);
            ++conflicts;
        }
    }
    require(migrated + conflicts == 11, "All eleven models must be explicitly classified.");
    const auto single = loadCatalog(root.path());
    require(single.error.isEmpty() && single.entries.size() == migrated + conflicts,
        "Single-source models must load in normal GUI catalog.");
    for (const auto& entry : single.entries) {
        const auto* original = find(legacy, entry.id);
        require(original, "Migrated model retains identity.");
        const auto before = loadDisplayData(*legacy.catalog, original->id, original->version);
        const auto after = loadDisplayData(*single.catalog, entry.id, entry.version);
        require(before.totalMassKg && after.totalMassKg &&
            std::abs(*before.totalMassKg - *after.totalMassKg) <= 1e-8 * *before.totalMassKg
            && before.bounds == after.bounds,
            "Single-source migration preserves GUI mass and bounds.");
        const auto meshes = segelphysik::getMesh(*after.inspection, "quelle");
        QSet<QString> elements;
        for (const auto& mesh : meshes) {
            require(mesh.element_ids.size() == 1, "Source belongs to exactly one element.");
            const auto id = QString::fromStdString(mesh.element_ids.front());
            require(!elements.contains(id), "Each geometric element owns exactly one source.");
            elements.insert(id);
        }
    }
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
    if (argc != 2) {
        std::cerr << "Expected the reference model folder.\n";
        return 2;
    }
    try {
        const QString folder = QString::fromLocal8Bit(argv[1]);
        testProblemParsing();
        testReferenceFolder(folder);
        testFolderChanges(folder);
        testIdentityIndependentOfRole(folder);
        testUnifiedModels(folder);
        testSingleSourceModels(folder);
        std::cout << "Model catalog via core passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
