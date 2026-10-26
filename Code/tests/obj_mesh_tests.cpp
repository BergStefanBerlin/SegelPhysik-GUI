#include "obj_mesh.hpp"

#include <QCoreApplication>

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

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (argc != 2) {
        std::cerr << "Expected the Core reference-model path.\n";
        return 2;
    }
    try {
        const auto parts = loadDisplayParts(QString::fromLocal8Bit(argv[1]));
        require(parts.size() == 1,
            "Quader reference should have one display element.");
        require(parts.constFirst().elementId == QStringLiteral("Rumpf"),
            "Display element ID was not preserved.");
        require(parts.constFirst().mesh->GetNumberOfPoints() == 8,
            "OBJ vertex count is incorrect.");
        require(parts.constFirst().mesh->GetNumberOfPolys() == 12,
            "OBJ triangulation did not preserve all triangles.");
        std::cout << "Display mesh loading and triangulation passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
