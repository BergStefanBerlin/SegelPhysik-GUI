# Spezifikation GUI: Dynamische Ruder- und Schwertsteuerung

**Status:** Fachlich spezifiziert; Umsetzung und Abnahme offen. Diese
Spezifikation erteilt keinen Implementierungs- oder Git-Auftrag.
**Stand:** 28.10.2026
**Maßgeblicher Core-Vertrag:** [Hydrodynamik, Abschnitt 13](../../CORE/Spezifikationen/Spezifikation_Hydrodynamik.md#13-dynamische-verstellung-von-ruder-und-schwert)
**Getrennter GUI-Plan:** [Umsetzungsplan G-0.7](../Umsetzungsplaene/Umsetzungsplan_G-0.7_Ruder_Schwertsteuerung.md)

Diese Datei ist die maßgebliche Spezifikation für die Darstellung und
Bedienung der dynamischen Ruder- und Schwertverstellung in der GUI. Die
Bewegungsdefinition, Achsen, Drehpunkte, Simulationsereignisse und
Hydrodynamik gehören ausschließlich zum Core-Vertrag. Die GUI erfasst
Bedienung, übermittelt Eingaben und stellt die vom Core bestätigte Pose dar;
sie dupliziert keine Core-Physik.

Die Bedienung ist für den späteren GUI-Umfang vorgesehen und ändert keine
älteren GUI-Versionen oder Spezifikationen rückwirkend. Sie setzt einen
Core voraus, der den Bewegungsvertrag aus A-C-00079 bis A-C-00085
unterstützt. Diese GUI-Spezifikation genehmigt weder Core-Änderungen noch
die Umsetzung.

## 1. Zweck und Abgrenzung

Die Person am Steuer soll während einer laufenden dynamischen Simulation den
Ruderwinkel und die Stellung beweglich deklarierter Schwerter ändern können.
Die Änderung muss gleichzeitig in der Darstellung sichtbar sein und über
den Core die Hydrodynamik beeinflussen.

Die Zuständigkeiten sind getrennt:

| Bereich | Zuständigkeit |
|---|---|
| GUI | Overlay, Eingabe, sichtbare Stellwerte, Übermittlung, Status und Darstellung der bestätigten Core-Pose |
| Core | Modellvalidierung, Bewegungsdefinition, Ereigniszeit und Reihenfolge, geometrische Transformation, Starrkörperzustand, Hydrodynamik und Ergebnis |
| Modell-JSON | Deklaration der Bewegungsart und der modellfesten Achse/Richtung; Drehpunkt bei Drehbewegung; Zuordnung gemeinsam bewegter Darstellungselemente |

Die GUI berechnet weder Bewegungsachsen und Drehpunkte noch Hubwege,
hydrodynamische Kräfte, Schwerpunkt oder Trägheit. Der Core-Vertrag ist
[hier maßgeblich](../../CORE/Spezifikationen/Spezifikation_Hydrodynamik.md#13-dynamische-verstellung-von-ruder-und-schwert).

## 2. Anforderungen

| ID | Kurztitel | Status |
|---|---|---|
| A-G-00046 | Bewegliche Elemente erkennen und Bedienelemente anbieten | offen |
| A-G-00047 | Ruderwinkel und Schwertstellung bedienen | offen |
| A-G-00048 | Eingaben während des dynamischen Laufs | offen |
| A-G-00049 | Bestätigte Stellung und vollständige Baugruppe darstellen | offen |
| A-G-00050 | Fehler, Laufzustände und Bestandsverhalten | offen |
| A-G-00051 | GUI/Core-Integration und Abnahmenachweise | offen |

### A-G-00046: Bewegliche Elemente erkennen und Bedienelemente anbieten

Die GUI bezieht Modellstruktur und Bewegungsdefinitionen aus dem
maßgeblichen Core-Modellkatalog beziehungsweise der dafür vorgesehenen
Core-Schnittstelle. Sie liest dafür keine `modell.json`- oder Meshdateien
selbst und verwendet keine Namen, Gruppennamen oder Geometrieheuristiken
als Ersatz für eine gültige Bewegungsdefinition.

Das Steuer-Overlay enthält ausschließlich Bedienelemente für im geladenen
Modell gültig deklarierte bewegliche Elemente:

- ein Ruder mit Bewegungsdefinition `drehen` erhält einen Winkelregler;
- ein Schwert mit Bewegungsdefinition `drehen` oder `heben` erhält einen
  Stellungsregler;
- nicht deklarierte, ungültige oder nicht bewegliche Kiel-/Schwertteile
  erhalten keinen Regler;
- ein als Hub-Kiel deklarierter Mechanismus darf nicht als unterstütztes
  Schwert-Steuerelement angeboten werden.

Mehrere passende Elemente werden anhand stabiler Core-Element-IDs
unterschieden. Anzeigenamen sind lesbar und eindeutig; die Bedienung darf
nicht von der zufälligen Reihenfolge der Meshes abhängen.

**Abnahmekriterium:** Für Modelle mit und ohne Bewegungsdefinition
erscheinen genau die vom Core als beweglich und gültig gemeldeten
Bedienelemente. Unbewegliche Elemente erzeugen keine Regler. Fehlerhafte
Definitionen werden nicht als gültige Steuerung dargestellt.

### A-G-00047: Ruderwinkel und Schwertstellung bedienen

Für jedes angebotene Ruder zeigt die GUI einen kontinuierlichen Regler im
Bereich **-90° bis +90°** einschließlich beider Grenzen. Der Zahlenwert
wird neben dem Regler sichtbar dargestellt. Negative Winkel bedeuten
Backbord, positive Winkel Steuerbord; diese Vorzeichenkonvention ist
projektweit und wird nicht je Modell in der GUI konfiguriert.

Für jedes angebotene Schwert zeigt die GUI einen kontinuierlichen Regler
von **0 % bis 100 %**, ebenfalls mit sichtbarem Zahlenwert und erreichbaren
Grenzen. Null- und Vollausfahrstellungen sind projektweit durch den Core-
Vertrag definiert und werden von der GUI nicht anhand von Meshgeometrie
abgeleitet. Damit umfasst der erste Umfang sowohl das hebbare Optimist-
Schwert als auch das drehbare 420er-Schwert; diese Mechanismen werden nicht
als Hub-Kiel interpretiert.

**Abnahmekriterium:** Beide Reglerbereiche einschließlich Endpunkten sind
bedienbar. Angezeigte Einheit und Vorzeichen stimmen mit dem übermittelten
Core-Stellwert überein. Das GUI-Verhalten verwendet für alle passenden
Modelle dieselben projektweiten Wertebereiche.

### A-G-00048: Eingaben während des dynamischen Laufs

Die Ruder- und Schwertbedienung ist während einer dynamischen Simulation
verfügbar. Eine vom Benutzer bestätigte Änderung wird als Core-Eingabe
übermittelt und ab dem nächsten Simulationszeitschritt wirksam. Die GUI
ordnet Eingaben weder nach lokaler Wandzeit noch erzeugt sie eine eigene
Stell- oder Aktuatordynamik. Der Core bestätigt die angenommene Eingabe
einschließlich der für die Anzeige nötigen aktuellen Stellung.

Die Bedienung bleibt beim Pausieren und Fortsetzen eindeutig: In Pause
werden Eingaben nicht stillschweigend mit einer anderen Simulationszeit
versehen; die GUI zeigt an, ob eine Eingabe angenommen und für welchen
Simulationszustand sie vorgemerkt wurde. Die Zeitzuordnung, Reihenfolge
gleichzeitiger Ereignisse und Reproduzierbarkeit richten sich nach dem
Core-Vertrag. Nach Reset, Abbruch oder Ende eines Laufs darf kein
ausstehender Stellwert unbeabsichtigt in einen neuen Lauf übernommen
werden.

**Abnahmekriterium:** Ein GUI-Testlauf mit mehreren Verstellungen zeigt,
dass bestätigte Änderungen ab dem zugehörigen Core-Zeitschritt in
Simulation und Darstellung wirksam sind. Pausieren, Fortsetzen, Reset und
Abbruch erzeugen keine verlorenen, doppelt gesendeten oder in den nächsten
Lauf verschleppten Eingaben. Die aus dem Core exportierte Ereignisfolge
bleibt maßgeblich für Wiedergabe und Reproduzierbarkeit.

### A-G-00049: Bestätigte Stellung und vollständige Baugruppe darstellen

Die GUI rendert die vom Core bestätigte Bauteilpose und nicht eine
unabhängig aus dem Sliderwert errechnete Drehung oder Verschiebung. Während
eine neue Eingabe noch nicht bestätigt ist, unterscheidet die Anzeige
eindeutig zwischen angefordertem und aktuell angewendetem Stellwert; sie
stellt einen nicht bestätigten Zustand nicht als bereits simuliert dar.

Die sichtbare Baugruppe bleibt vollständig und wird gemäß den vom Core
gelieferten Bewegungsdaten gemeinsam bewegt. Für den Optimisten umfasst
die Ruderbaugruppe Ruder, Pinne, Pinnenausleger und Gummigelenk; beim
420er gehört der Ruderkopf zur Ruderbewegung. Die GUI lädt und rendert
weiterhin die vollständigen Darstellungsmeshes und nicht das möglicherweise
vereinfachte hydrodynamische Simulationsmesh.

**Abnahmekriterium:** Bei End- und Zwischenstellungen folgen alle
zugeordneten sichtbaren Baugruppenteile derselben vom Core bestätigten
Pose. Der dargestellte Stellwert und die Core-Pose stimmen in laufender
Simulation, Pause und Wiedergabe überein. Die Darstellung verändert keine
Hydrostatik- oder Massenwerte.

### A-G-00050: Fehler, Laufzustände und Bestandsverhalten

Das Overlay ist im Simulationsbereich klar erkennbar, mit Ruder- und
Schwertreglern pro Element beschriftet und darf bestehende Start-, Pause-,
Fortsetzen- und Stoppbedienelemente nicht verdecken. Es enthält keine
neuen Anzeigen für Kräfte, Schwerpunkt, Masse oder Trägheit.

Core-Ablehnung, veraltete Modellfassung, ungültige Eingabe und
Kommunikationsfehler werden für das betroffene Element sichtbar
diagnostiziert. Die GUI übernimmt keinen abgelehnten Wert als wirksam und
setzt ihn nicht still auf einen Default. Bei ungültiger oder nicht
unterstützter Bewegungsdefinition wird kein aktiver Regler angeboten;
Core-Modellfehler bleiben über den bestehenden Modellkatalog- und
Diagnosepfad sichtbar.

Modelle ohne Bewegungsdaten behalten das bisherige Verhalten vollständig:
kein Overlay-Regler, keine zusätzliche Core-Eingabe und keine
Darstellungsänderung. Die Steuerung verändert ausschließlich die
Ruder-/Schwertstellung; sie ändert weder Gesamtmasse, Schwerpunkt,
Trägheit, Auftrieb, Wasserlinie noch Flutungszustand. Kollisionen mit
Rigg, Großbaum oder Segeln werden nicht durch die GUI simuliert.

**Abnahmekriterium:** Fehler und Ablehnungen sind verständlich sichtbar;
der zuletzt bestätigte Zustand bleibt erkennbar. Regressionstests belegen
unverändertes Verhalten für Modelle ohne Bewegungsdefinition und erhaltene
Laufsteuerung, Hydrostatik- und Massendarstellung.

### A-G-00051: GUI/Core-Integration und Abnahmenachweise

Die GUI nutzt den Core-Modellkatalog, Core-Eingabepfad, Core-
Ereigniszeitbezug und Core-Pose für Live-Simulation und Wiedergabe.
Batch- und Live-Pfad verwenden denselben Core-Vertrag. Die GUI berechnet
keine Physik und verändert die unveränderliche Massenverteilung nicht.

Der Integrationsnachweis umfasst mindestens:

- Modelle mit Ruder und heb- bzw. drehbarem Schwert sowie Modelle ohne
  Bewegungsdefinition;
- Wertebereiche, sichtbare Zahlenwerte und beide Endstellungen;
- erfolgreiche und abgelehnte Eingaben sowie veraltete Modellfassung;
- mehrere zeitgeordnete Eingaben einschließlich Pause, Fortsetzen, Reset
  und Abbruch;
- gemeinsame Darstellung der zugeordneten vollständigen Baugruppen;
- Übereinstimmung von Core-bestätigter Stellung, Ergebnis/Wiedergabe und
  gerenderter Pose;
- Regression der bisherigen Simulation und des Overlay-freien
  Bestandsverhaltens.

**Abnahmekriterium:** Automatisierte GUI-/Core-Integrationstests und ein
GUI-Build bestehen für den freigegebenen Funktionsumfang. Die Nachweise
belegen, dass die GUI Eingaben und Darstellung integriert, während
Bewegungsvalidierung, zeitliche Eingabeordnung und Hydrodynamik im Core
bleiben. Sicht- und Bedienabnahme sind getrennt zu dokumentieren.

## Herkunft und Abgrenzung zum Core

Diese GUI-Anforderungen konkretisieren ausschließlich Overlay, Eingaben,
Statusanzeige und Darstellung. Die maßgeblichen Core-Anforderungen sind
A-C-00079 bis A-C-00085 in der
[Core-Hydrodynamik-Spezifikation](../../CORE/Spezifikationen/Spezifikation_Hydrodynamik.md#13-dynamische-verstellung-von-ruder-und-schwert).
Die GUI-Anforderungen A-G-00046 bis A-G-00051 stehen unabhängig davon
unter der projektweiten GUI-Nummerierung. Die Erstellung dieser
Spezifikation beauftragt weder ihre Umsetzung noch eine neue
Versionszuordnung.
