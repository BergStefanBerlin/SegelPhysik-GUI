#include "display_geometry.hpp"

#include <vtkCellArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <stdexcept>
#include <utility>

namespace {
QVector<DisplayPart> convertParts(const segelphysik::Darstellungsgeometrie& geometry,
    bool pointTopology)
{
    QVector<DisplayPart> parts;
    for (const auto& part : geometry.teile) {
        if (part.punkte_m.empty() || (!pointTopology && part.dreiecke.empty())) {
            continue;
        }
        vtkNew<vtkPoints> points;
        points->SetNumberOfPoints(static_cast<vtkIdType>(part.punkte_m.size()));
        for (std::size_t i = 0; i < part.punkte_m.size(); ++i) {
            const auto& p = part.punkte_m[i];
            points->SetPoint(static_cast<vtkIdType>(i), p[0], p[1], p[2]);
        }
        vtkNew<vtkCellArray> triangles;
        for (const auto& triangle : part.dreiecke) {
            vtkIdType ids[3];
            for (std::size_t corner = 0; corner < 3; ++corner) {
                if (triangle[corner] >= part.punkte_m.size()) {
                    throw std::runtime_error(
                        "Core-Darstellungsgeometrie enthält einen ungültigen Punktindex.");
                }
                ids[corner] = static_cast<vtkIdType>(triangle[corner]);
            }
            triangles->InsertNextCell(3, ids);
        }
        DisplayPart display;
        display.elementId = QString::fromStdString(part.element_id);
        display.group = QString::fromStdString(part.gruppe);
        display.origin = QString::fromStdString(part.herkunft);
        display.mesh = vtkSmartPointer<vtkPolyData>::New();
        display.mesh->SetPoints(points);
        display.mesh->SetPolys(triangles);
        if (pointTopology && part.dreiecke.empty()) {
            vtkNew<vtkCellArray> vertices;
            for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
                vertices->InsertNextCell(1, &i);
            display.mesh->SetVerts(vertices);
        }
        if (part.farbe_rgb) {
            display.hasColor = true;
            for (std::size_t i = 0; i < 3; ++i) {
                display.color[i] = (*part.farbe_rgb)[i];
            }
        }
        if (part.oberflaeche) {
            using S = DisplayPart::Surface;
            static const std::pair<const char*, S> names[] = {{"matt", S::Matte},
                {"glaenzend", S::Glossy}, {"metall", S::Metal}, {"chrom", S::Chrome},
                {"lack", S::Lacquer}, {"glas", S::Glass}, {"leuchtend", S::Glow},
                {"schillernd", S::Iridescent}, {"facettiert", S::Faceted},
                {"drahtgitter", S::Wireframe}, {"perlmutt", S::Pearl},
                {"unsichtbar", S::Invisible}, {"stoff", S::Cloth}, {"holz", S::Wood},
                {"metall_gebuerstet", S::BrushedMetal}};
            for (const auto& [name, value] : names) {
                if (*part.oberflaeche == name) {
                    display.surface = value;
                }
            }
        }
        parts.push_back(display);
    }
    if (parts.isEmpty()) {
        throw std::runtime_error(
            "Der Core liefert für dieses Modell keine darstellbare Geometrie.");
    }
    return parts;
}
}

QVector<DisplayPart> toDisplayParts(const segelphysik::Darstellungsgeometrie& geometry)
{
    return convertParts(geometry, false);
}

QVector<DisplayPart> toDisplayParts(const std::vector<segelphysik::InspectionMesh>& meshes)
{
    segelphysik::Darstellungsgeometrie geometry;
    for (const auto& mesh : meshes) {
        segelphysik::Darstellungsteil part;
        part.element_id = mesh.element_id;
        part.gruppe = mesh.group;
        part.herkunft = mesh.origin;
        part.punkte_m = mesh.points_m;
        part.dreiecke = mesh.triangles;
        part.farbe_rgb = mesh.color_rgb;
        part.oberflaeche = mesh.surface;
        geometry.teile.push_back(std::move(part));
    }
    return convertParts(geometry, true);
}
