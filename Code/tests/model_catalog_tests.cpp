#include "model_catalog.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testReferenceFolder(const QString& folder)
{
    const auto entries = scanModelFolder(folder);
    require(entries.size() >= 2, "Reference folder lists too few models.");
    bool hasQuader = false;
    for (const auto& entry : entries) {
        require(QFile::exists(entry.path), "Listed model file does not exist.");
        require(!entry.name.isEmpty(), "Listed model has no display name.");
        hasQuader = hasQuader || entry.id == QStringLiteral("quader");
    }
    require(hasQuader, "Reference folder omitted the quader model.");
    require(watchedModelPaths(folder).size() > entries.size(),
        "Folder watch list misses model directories or files.");
}

void testFolderChanges()
{
    QTemporaryDir root;
    require(root.isValid(), "Temporary folder unavailable.");
    const QDir dir(root.path());
    require(scanModelFolder(root.path()).isEmpty(), "Empty folder lists models.");

    require(dir.mkpath(QStringLiteral("b_ohne_name")), "mkdir failed.");
    require(dir.mkpath(QStringLiteral("a_mit_name")), "mkdir failed.");
    require(dir.mkpath(QStringLiteral("leer")), "mkdir failed.");
    auto write = [&dir](const QString& relative, const QByteArray& content) {
        QFile file(dir.filePath(relative));
        require(file.open(QIODevice::WriteOnly), "Cannot write test model.");
        file.write(content);
    };
    write(QStringLiteral("a_mit_name/modell.json"),
        R"({"id":"a","name":"Alpha"})");
    write(QStringLiteral("b_ohne_name/modell.json"), R"({"id":"b"})");

    const auto entries = scanModelFolder(root.path());
    require(entries.size() == 2, "Folder without modell.json was listed.");
    require(entries.at(0).name == QStringLiteral("Alpha"),
        "Model name from JSON was not used or order is wrong.");
    require(entries.at(1).name == QStringLiteral("b"),
        "Model id fallback was not used.");

    require(QFile::remove(dir.filePath(QStringLiteral("a_mit_name/modell.json"))),
        "Cannot remove test model.");
    require(scanModelFolder(root.path()).size() == 1,
        "Removed model is still listed.");
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
        testReferenceFolder(QString::fromLocal8Bit(argv[1]));
        testFolderChanges();
        std::cout << "Model catalog scan passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
