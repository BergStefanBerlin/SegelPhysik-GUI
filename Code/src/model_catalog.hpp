#pragma once

// Modellkatalog über den Core (G-0.2, A-G-00019 bis A-G-00022). Die GUI
// wählt und überwacht den Ordner; Inhalt, Status und Darstellungsgeometrie
// liefert ausschließlich der Core. Die Funktionen rufen den Core auf und
// gehören in einen Worker-Thread.

#include <segelphysik/katalog.hpp>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

#include <array>
#include <memory>
#include <optional>

// Ein Modellfehler, wie ihn der Core meldet, ergänzt um Kurztext und
// Handlungsanweisung aus dem Core-Fehlerkatalog (fehlerdokumentation_json).
struct ModelProblem {
    QString code;      // z. B. CORE-MOD-0007
    QString message;   // Detailmeldung des Core
    QString location;  // Fundstelle relativ zum Modellordner, falls geliefert
    QString summary;   // Kurztext aus dem Fehlerkatalog
    QString remedy;    // Handlungsanweisung aus dem Fehlerkatalog
};

struct ModelEntry {
    QString id;
    QString name;
    QString description;
    QString version;   // Modellfassung laut Core
    QString path;      // vom Core aufgelöste modell.json, für den Simulationslauf
    bool usable = false;
    int errorCount = 0;
    QStringList diagnostics;
    QVector<ModelProblem> problems;
};

// Zerlegt Core-Meldungen der Form "CORE-XXX-0000 [Fundstelle]: Text"
// (eine je Zeile) und ergänzt Kurztext und Handlungsanweisung.
QVector<ModelProblem> problemsFromText(const QString& text);
ModelProblem withErrorHelp(ModelProblem problem);

struct CatalogSnapshot {
    QString folder;
    std::shared_ptr<const segelphysik::Modellkatalog> catalog;
    QVector<ModelEntry> entries;
    QString error;
};

CatalogSnapshot loadCatalog(const QString& folder);

// Verzeichnisse und Dateien, deren Änderung einen neuen Katalogabruf
// auslösen soll (nur Existenzprüfung, kein Lesen des Modellformats).
QStringList watchedModelPaths(const QString& folder);

struct ModelDisplayData {
    QString modelId;
    QString version;
    QString path;
    segelphysik::Darstellungsgeometrie geometry;
    std::optional<double> totalMassKg;
    std::optional<std::array<std::array<double, 3>, 2>> bounds;
    // Element-ID → Modellgruppe für alle Elemente, auch unsichtbare (z. B. Luftkörper).
    QHash<QString, QString> elementGroups;
    QHash<QString, segelphysik::Bewegungsdefinition> elementMovements;
    QStringList partDiagnostics;
};

// Wirft std::runtime_error mit Core-Fehler-ID (z. B. CORE-KAT-0004 bei
// veralteter Fassung).
ModelDisplayData loadDisplayData(const segelphysik::Modellkatalog& catalog,
    const QString& modelId, const QString& version);

bool isOutdatedVersionError(const QString& message);
