#pragma once

// Modellkatalog über den Core (G-0.2, A-G-00019 bis A-G-00022). Die GUI
// wählt den Ordner; Inhalt, Status und Darstellungsgeometrie
// liefert ausschließlich der Core. Die Funktionen rufen den Core auf und
// gehören in einen Worker-Thread.

#include <segelphysik/katalog.hpp>
#include <segelphysik/inspektion.hpp>

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
    QString path;      // vom Core aufgeloeste Modelldatei fuer den Simulationslauf
    bool usable = false;
    bool schemaValid = true;
    bool visualizationHashCurrent = true;
    bool fingerprintsCurrent = true;
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

struct ModelDisplayData {
    std::shared_ptr<const segelphysik::ModelInspection> inspection;
    segelphysik::Modellstruktur structure;
    bool modelFormat03 = false;
    QString modelId;
    QString version;
    QString path;
    segelphysik::Darstellungsgeometrie geometry;
    std::optional<double> totalMassKg;
    std::optional<std::array<std::array<double, 3>, 2>> bounds;
    // Element-ID → Modellgruppe für alle Elemente, auch unsichtbare (z. B. Luftkörper).
    QHash<QString, QString> elementGroups;
    QHash<QString, QString> hydrodynamicRoles;
    QHash<QString, segelphysik::Bewegungsdefinition> elementMovements;
    QStringList partDiagnostics;
};

// Wirft std::runtime_error mit Core-Fehler-ID (z. B. CORE-KAT-0004 bei
// veralteter Fassung).
ModelDisplayData loadDisplayData(const segelphysik::Modellkatalog& catalog,
    const QString& modelId, const QString& version);
void loadSelectedModelPhysics(ModelDisplayData& data,
    const segelphysik::Modellkatalog& catalog);

bool isOutdatedVersionError(const QString& message);
