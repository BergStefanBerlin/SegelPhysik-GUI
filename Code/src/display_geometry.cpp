#include "display_geometry.hpp"

#include <vtkCellArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <stdexcept>
#include <utility>

QVector<DisplayPart> toDisplayParts(const segelphysik::Darstellungsgeometrie& geometry)
{
    QVector<DisplayPart> parts;
    for (const auto& part : geometry.teile) {
        if (part.punkte_m.empty() || part.dreiecke.empty()) {
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
                {"unsichtbar", S::Invisible}};
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
