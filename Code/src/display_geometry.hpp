#pragma once

#include <segelphysik/katalog.hpp>

#include <QString>
#include <QVector>

#include <vtkSmartPointer.h>

class vtkPolyData;

struct DisplayPart {
    enum class Surface {
        Default, Matte, Glossy, Metal, Chrome, Lacquer, Glass, Glow, Iridescent, Faceted,
        Wireframe, Pearl
    };

    QString elementId;
    QString group;
    QString origin;  // darstellung oder volumen_ersatz
    vtkSmartPointer<vtkPolyData> mesh;
    bool hasColor = false;
    double color[3] = {0.0, 0.0, 0.0};
    Surface surface = Surface::Default;
};

// Wandelt die vom Core gelieferte Darstellungsgeometrie in VTK-Netze um.
// Wirft std::runtime_error, wenn kein Teil darstellbar ist.
QVector<DisplayPart> toDisplayParts(const segelphysik::Darstellungsgeometrie& geometry);
