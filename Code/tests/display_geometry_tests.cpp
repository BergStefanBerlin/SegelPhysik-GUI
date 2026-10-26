#include "display_geometry.hpp"
#include "model_catalog.hpp"

#include <QCoreApplication>

#include <vtkCellArray.h>
#include <vtkPolyData.h>

#include <iostream>
#include <stdexcept>

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
        testEmptyGeometry();
        std::cout << "Display geometry via core passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
