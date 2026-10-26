#pragma once

#include <QString>
#include <QVector>

#include <vtkSmartPointer.h>

class vtkPolyData;

struct DisplayPart {
    QString elementId;
    vtkSmartPointer<vtkPolyData> mesh;
};

QVector<DisplayPart> loadDisplayParts(const QString& modelPath);
