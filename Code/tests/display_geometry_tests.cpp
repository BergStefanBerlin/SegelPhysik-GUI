#include "display_geometry.hpp"
#include "model_catalog.hpp"

#include <QCoreApplication>

#include <vtkCellArray.h>
#include <vtkPolyData.h>

#include <algorithm>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testQuader(const QString& folder)
{
    const auto snapshot = loadCatalog(folder);
    require(snapshot.catalog != nullptr, "Catalog missing.");
    const ModelEntry* quader = nullptr;
    for (const auto& entry : snapshot.entries) {
        if (entry.id == QStringLiteral("quader")) {
            quader = &entry;
        }
    }
    require(quader != nullptr, "quader missing in catalog.");
    const auto data = loadDisplayData(*snapshot.catalog, quader->id, quader->version);
    const auto parts = toDisplayParts(data.geometry);
    require(parts.size() == 1, "quader should have exactly one display part.");
    const auto& part = parts.front();
    require(part.mesh != nullptr, "Display part without mesh.");
    require(part.mesh->GetNumberOfPoints() == 8, "quader mesh should have 8 points.");
    require(part.mesh->GetNumberOfPolys() == 12, "quader mesh should have 12 triangles.");
    require(!part.elementId.isEmpty(), "Display part without element id.");
}

void testSimpleModels(const QString& folder)
{
    const auto snapshot = loadCatalog(folder);
    require(snapshot.catalog != nullptr, "Catalog missing.");
    const QStringList modelIds{
        QStringLiteral("bleistift"), QStringLiteral("kegel"),
        QStringLiteral("kugel"), QStringLiteral("platte"),
        QStringLiteral("pyramide"), QStringLiteral("quader"),
        QStringLiteral("rolle"), QStringLiteral("unregelmaessig")};
    for (const auto& modelId : modelIds) {
        const auto entry = std::find_if(snapshot.entries.cbegin(), snapshot.entries.cend(),
            [&](const auto& candidate) { return candidate.id == modelId; });
        require(entry != snapshot.entries.cend(),
            "Simple model missing from Core catalog.");
        require(entry->usable && entry->errorCount == 0,
            "Core rejected a simple model.");
        const auto data = loadDisplayData(*snapshot.catalog, entry->id, entry->version);
        const auto parts = toDisplayParts(data.geometry);
        require(!parts.isEmpty(), "Simple model has no GUI display geometry.");
        for (const auto& part : parts) {
            require(part.mesh != nullptr && part.mesh->GetNumberOfPoints() > 0
                    && part.mesh->GetNumberOfPolys() > 0,
                "Simple model contains an empty GUI display mesh.");
            require(!part.elementId.isEmpty(),
                "Simple model display mesh has no element identity.");
        }
        std::cout << entry->id.toStdString() << ": " << parts.size()
                  << " display part(s)\n";
    }
}

void test420(const QString& folder)
{
    const auto snapshot = loadCatalog(folder);
    require(snapshot.catalog != nullptr, "Catalog missing for 420 display geometry.");
    const auto entry = std::find_if(snapshot.entries.cbegin(), snapshot.entries.cend(),
        [](const auto& candidate) { return candidate.id == QStringLiteral("420"); });
    require(entry != snapshot.entries.cend() && entry->usable,
        "420 missing or unusable in display catalog.");
    const auto data = loadDisplayData(*snapshot.catalog, entry->id, entry->version);
    const auto parts = toDisplayParts(data.geometry);
    require(!parts.isEmpty(), "420 has no GUI display geometry.");
    for (const auto& part : parts)
        require(part.mesh != nullptr && part.mesh->GetNumberOfPoints() > 0,
            "420 display mesh has no points.");
}

void testEmptyGeometry()
{
    segelphysik::Darstellungsgeometrie empty;
    bool thrown = false;
    try {
        static_cast<void>(toDisplayParts(empty));
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    require(thrown, "Empty geometry must be rejected.");
}

void testMaterialContract()
{
    const std::pair<const char*, DisplayPart::Surface> surfaces[] = {
        {"lack", DisplayPart::Surface::Lacquer}, {"stoff", DisplayPart::Surface::Cloth},
        {"holz", DisplayPart::Surface::Wood}, {"metall_gebuerstet", DisplayPart::Surface::BrushedMetal},
        {"metall", DisplayPart::Surface::Metal}, {"matt", DisplayPart::Surface::Matte}};
    for (const auto& [name, expected] : surfaces) {
        segelphysik::Darstellungsgeometrie geometry;
        decltype(geometry.teile)::value_type part;
        part.element_id = "explicit-material";
        part.oberflaeche = name;
        part.punkte_m = {{{0, 0, 0}}, {{1, 0, 0}}, {{0, 1, 0}}};
        part.dreiecke = {{{0, 1, 2}}};
        geometry.teile.push_back(part);
        const auto display = toDisplayParts(geometry);
        require(display.size() == 1 && display.front().surface == expected,
            "Explicit Core material was not mapped.");
    }
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (argc != 2) {
        std::cerr << "Expected the reference model folder.\n";
        return 2;
    }
    try {
        testQuader(QString::fromLocal8Bit(argv[1]));
        test420(QString::fromLocal8Bit(argv[1]));
        testSimpleModels(QString::fromLocal8Bit(argv[1]));
        testEmptyGeometry();
        testMaterialContract();
        std::cout << "Display geometry via core passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
