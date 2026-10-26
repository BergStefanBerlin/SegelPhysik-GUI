#include "model_catalog.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

const QString modelFileName = QStringLiteral("modell.json");

ModelEntry readEntry(const QString& path, const QString& fallbackName)
{
    ModelEntry entry;
    entry.path = QFileInfo(path).absoluteFilePath();
    entry.name = fallbackName;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        const auto object = QJsonDocument::fromJson(file.readAll()).object();
        entry.id = object.value(QStringLiteral("id")).toString();
        const auto name = object.value(QStringLiteral("name")).toString();
        if (!name.trimmed().isEmpty()) {
            entry.name = name.trimmed();
        } else if (!entry.id.isEmpty()) {
            entry.name = entry.id;
        }
    }
    return entry;
}

QFileInfoList modelDirectories(const QDir& dir)
{
    return dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name | QDir::IgnoreCase);
}

} // namespace

QVector<ModelEntry> scanModelFolder(const QString& folder)
{
    QVector<ModelEntry> entries;
    const QDir dir(folder);
    if (folder.isEmpty() || !dir.exists()) {
        return entries;
    }
    if (dir.exists(modelFileName)) {
        entries.push_back(
            readEntry(dir.filePath(modelFileName), dir.dirName()));
    }
    for (const auto& sub : modelDirectories(dir)) {
        const QDir subDir(sub.absoluteFilePath());
        if (subDir.exists(modelFileName)) {
            entries.push_back(
                readEntry(subDir.filePath(modelFileName), sub.fileName()));
        }
    }
    return entries;
}

QStringList watchedModelPaths(const QString& folder)
{
    QStringList paths;
    const QDir dir(folder);
    if (folder.isEmpty() || !dir.exists()) {
        return paths;
    }
    paths << dir.absolutePath();
    if (dir.exists(modelFileName)) {
        paths << dir.absoluteFilePath(modelFileName);
    }
    for (const auto& sub : modelDirectories(dir)) {
        const QDir subDir(sub.absoluteFilePath());
        if (!subDir.exists(modelFileName)) {
            continue;
        }
        paths << subDir.absolutePath();
        for (const auto& file : subDir.entryInfoList(QDir::Files)) {
            paths << file.absoluteFilePath();
        }
    }
    return paths;
}
