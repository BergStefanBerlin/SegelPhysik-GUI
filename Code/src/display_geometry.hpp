#pragma once

#include <segelphysik/katalog.hpp>
#include <segelphysik/inspektion.hpp>

#include <QString>
#include <QVector>
#include <QJsonArray>
#include <memory>

#include <vtkSmartPointer.h>
#include <vtkType.h>

class vtkPolyData;

struct DisplayMeshData {
    vtkSmartPointer<vtkPolyData> mesh;
    vtkSmartPointer<vtkPolyData> material;
    vtkSmartPointer<vtkPolyData> cutMesh;
};

// One bounded, immutable preparation per model part and effective element matrix.
struct DisplayMeshCache {
    vtkSmartPointer<vtkPolyData> source;
    vtkMTimeType sourceTime = 0;
    std::shared_ptr<const DisplayMeshData> base;
    QJsonArray matrix;
    std::shared_ptr<const DisplayMeshData> transformed;
};

struct DisplayPart {
    enum class Surface {
        Default, Matte, Glossy, Metal, Chrome, Lacquer, Glass, Glow, Iridescent, Faceted,
        Wireframe, Pearl, Invisible, Cloth, Wood, BrushedMetal
    };

    QString elementId;
    QString group;
    QString origin;  // darstellung oder volumen_ersatz
    vtkSmartPointer<vtkPolyData> mesh;
    std::shared_ptr<DisplayMeshCache> preparation = std::make_shared<DisplayMeshCache>();
    bool hasColor = false;
    double color[3] = {0.0, 0.0, 0.0};
    Surface surface = Surface::Default;
};

// Wandelt die vom Core gelieferte Darstellungsgeometrie in VTK-Netze um.
// Wirft std::runtime_error, wenn kein Teil darstellbar ist.
QVector<DisplayPart> toDisplayParts(const segelphysik::Darstellungsgeometrie& geometry);
QVector<DisplayPart> toDisplayParts(const std::vector<segelphysik::InspectionMesh>& meshes);
