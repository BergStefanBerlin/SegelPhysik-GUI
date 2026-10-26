#include "obj_mesh.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTextStream>

#include <vtkCellArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <array>
#include <stdexcept>

namespace {

vtkSmartPointer<vtkPolyData> loadObj(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error(
            QStringLiteral("Darstellungsmesh kann nicht geöffnet werden: %1")
                .arg(path)
                .toStdString());
    }

    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> polygons;
    QTextStream input(&file);
    int lineNumber = 0;
    while (!input.atEnd()) {
        const auto line = input.readLine().trimmed();
        ++lineNumber;
        if (line.startsWith(QStringLiteral("v "))) {
            const auto values = line.mid(2).simplified().split(QLatin1Char(' '));
            bool xOk = false;
            bool yOk = false;
            bool zOk = false;
            const double x = values.value(0).toDouble(&xOk);
            const double y = values.value(1).toDouble(&yOk);
            const double z = values.value(2).toDouble(&zOk);
            if (values.size() < 3 || !xOk || !yOk || !zOk) {
                throw std::runtime_error(
                    QStringLiteral("Ungültiger OBJ-Vertex in %1, Zeile %2.")
                        .arg(path)
                        .arg(lineNumber)
                        .toStdString());
            }
            points->InsertNextPoint(x, y, z);
        } else if (line.startsWith(QStringLiteral("f "))) {
            const auto fields = line.mid(2).simplified().split(QLatin1Char(' '));
            QVector<vtkIdType> indices;
            indices.reserve(fields.size());
            for (const auto& field : fields) {
                const auto vertex = field.section(QLatin1Char('/'), 0, 0);
                bool ok = false;
                const auto raw = vertex.toLongLong(&ok);
                if (!ok || raw == 0) {
                    throw std::runtime_error(
                        QStringLiteral("Ungültiger OBJ-Index in %1, Zeile %2.")
                            .arg(path)
                            .arg(lineNumber)
                            .toStdString());
                }
                const auto index = raw > 0
                    ? raw - 1
                    : static_cast<qint64>(points->GetNumberOfPoints()) + raw;
                if (index < 0 || index >= points->GetNumberOfPoints()) {
                    throw std::runtime_error(
                        QStringLiteral("OBJ-Index außerhalb des Meshs in %1, Zeile %2.")
                            .arg(path)
                            .arg(lineNumber)
                            .toStdString());
                }
                indices.push_back(static_cast<vtkIdType>(index));
            }
            if (indices.size() < 3) {
                throw std::runtime_error(
                    QStringLiteral("Entartete OBJ-Fläche in %1, Zeile %2.")
                        .arg(path)
                        .arg(lineNumber)
                        .toStdString());
            }
            for (qsizetype i = 1; i + 1 < indices.size(); ++i) {
                const vtkIdType triangle[] = {
                    indices[0], indices[i], indices[i + 1]};
                polygons->InsertNextCell(3, triangle);
            }
        }
    }
    if (points->GetNumberOfPoints() == 0 || polygons->GetNumberOfCells() == 0) {
        throw std::runtime_error(
            QStringLiteral("OBJ-Datei enthält kein darstellbares Dreiecksmesh: %1")
                .arg(path)
                .toStdString());
    }

    auto mesh = vtkSmartPointer<vtkPolyData>::New();
    mesh->SetPoints(points);
    mesh->SetPolys(polygons);
    return mesh;
}

QString displayPath(const QJsonObject& element, const QDir& baseDirectory)
{
    const auto display = element.value(QStringLiteral("Darstellung")).toObject();
    auto relativePath = display.value(QStringLiteral("datei")).toString();
    if (relativePath.isEmpty()) {
        const auto hydrostatics =
            element.value(QStringLiteral("Hydrostatik")).toObject();
        const auto volume = hydrostatics.value(QStringLiteral("volumen")).toObject();
        relativePath = volume.value(QStringLiteral("datei")).toString();
    }
    if (relativePath.isEmpty()) {
        return {};
    }
    return baseDirectory.absoluteFilePath(relativePath);
}

} // namespace

QVector<DisplayPart> loadDisplayParts(const QString& modelPath)
{
    QFile modelFile(modelPath);
    if (!modelFile.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(
            QStringLiteral("Modelldatei kann nicht geöffnet werden: %1")
                .arg(modelFile.errorString())
                .toStdString());
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(modelFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        throw std::runtime_error(
            QStringLiteral("Modelldatei ist kein gültiges JSON: %1")
                .arg(parseError.errorString())
                .toStdString());
    }

    QVector<DisplayPart> parts;
    const auto groups = document.object().value(QStringLiteral("gruppen")).toObject();
    const QDir baseDirectory(QFileInfo(modelPath).absolutePath());
    for (auto group = groups.constBegin(); group != groups.constEnd(); ++group) {
        const auto elements = group.value().toObject()
                                  .value(QStringLiteral("elemente")).toArray();
        for (const auto& value : elements) {
            if (!value.isObject()) {
                continue;
            }
            const auto element = value.toObject();
            const auto path = displayPath(element, baseDirectory);
            if (path.isEmpty()) {
                continue;
            }
            DisplayPart part;
            part.elementId = element.value(QStringLiteral("id")).toString();
            if (part.elementId.isEmpty()) {
                part.elementId = group.key();
            }
            part.mesh = loadObj(path);
            parts.push_back(part);
        }
    }

    if (parts.isEmpty()) {
        throw std::runtime_error(
            "Modell enthält keine Darstellungsmeshes oder Volumenmesh-Fallbacks.");
    }
    return parts;
}
