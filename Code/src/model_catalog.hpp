#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct ModelEntry {
    QString path;
    QString name;
    QString id;
};

// Findet Modelle im Modellordner: eine modell.json direkt im Ordner und je
// eine modell.json in den unmittelbaren Unterordnern, nach Ordnername sortiert.
QVector<ModelEntry> scanModelFolder(const QString& folder);

// Pfade, deren Änderung eine erneute Suche bzw. Darstellung auslösen soll.
QStringList watchedModelPaths(const QString& folder);
