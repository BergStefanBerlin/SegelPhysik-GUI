# Umsetzungsplan G-0.8: Bootsauswahl und Objektinspektion

## Nutzerabnahme 04.11.2026

**Nutzerabnahmen: done.** Ausdruecklicher Nutzerauftrag vom 04.11.2026,
09:16 Uhr fuer den gelieferten Stand. Dieser datierte Abschluss ersetzt
alle aelteren offenen Nutzerabnahmestaende in diesem Plan; historische
Nachweise bleiben erhalten. Technische Testluecken und nicht finalisierte
Zusatzumfaenge werden dadurch nicht als nachgewiesen ausgegeben.
Kein Commit, Tag oder Push beauftragt.

**Status:** Technisch umgesetzt und integriert am 01.11.2026;
persönliche Sichtabnahme offen, bekannter Grafik-Baselinefehler separat.
**Stand:** 02.11.2026; Erweiterung und Ableitungsmigration aller elf Modelle technisch umgesetzt und getestet; persönliche Sichtabnahme separat offen.  
**Zielversion:** G-0.8.
**Maßgebliche Grundlage:** [Spezifikation Bootsauswahl und Objektinspektion](../Spezifikationen/Spezifikation_GUI_Bootsauswahl.md)
**Entwurfspunkte:** [GUI Bootsauswahl](../Spezifikationen/Entwurfspunkte/GUI_Bootsauswahl.json)

Dieser Plan ist die einzige Planungsdatei für den hier beschriebenen
Bootsauswahl-Umfang in G-0.8. Weitere Arbeiten für G-0.8 werden in diesen
maßgeblichen Versionsplan aufgenommen. Historische
Pläne werden nicht verändert.

Die finale Spezifikation enthält A-G-00057 bis A-G-00069; dieser Plan
ordnet sie U-G-000055 bis U-G-000062 zu. Voraussetzung für die Integration
ist die technische Core-C-0.8-Lieferung nach A-C-00098 bis A-C-00104 und
U-C-000084, nicht allein die Existenz des Core-Spezifikationsdokuments.

Der Nutzer hat nach Finalisierung die Umsetzung beider Pläne beauftragt.
Keine Commit-, Push-, Tag- oder Release-Freigabe.

## 1. Umfang und Grenzen

### Freigegebener Plan: objektzentrierte Bootsauswahl und festes Meterraster

**Finalisierung und Umsetzung ausdrücklich beauftragt am 02.11.2026.**
Grundlage ist [Spezifikation Abschnitt 11](../Spezifikationen/Spezifikation_GUI_Bootsauswahl.md#11-finalisiert-02112026-objektzentrierte-bootsauswahl),
zugeordnet zu IDEE-G-00044 bis IDEE-G-00047 in der bestehenden
Entwurfspunkteliste. Zugeordnet sind A-G-00075 bis A-G-00078 und
U-G-000067 bis U-G-000072.
Historische Schritte und Nachweise bleiben unverändert.

#### Arbeitspakete und Abhängigkeiten

| Paket | Ergebnis / Prüfauftrag | Anforderungen | Voraussetzungen | Status |
|---|---|---|---|---|
| U-G-000067 Vertrag und Ausgangsstand | Gemeinsame Core-Zuordnungen, Feldinventar und Bewegungsdaten abgestimmt; vorhandene Regressionen und Grafikbaseline erhalten. | A-G-00075 bis A-G-00078 | Freigabe erteilt | Technisch abgeschlossen |
| U-G-000068 Festes Meterraster | 0,5-m-Teilung, ganze Meter solid, Zwischenlinien dotted; Ursprung und runde Ränder erhalten. | A-G-00078 | U-G-000067 | Implementiert und DPI-geprüft |
| U-G-000069 Gemeinsamer Objektbaum | Fachreiter entfernt, jedes Element einmal mit Quelle/Ableitungen; Bestandsquellen sichtbar gekennzeichnet. | A-G-00075 | U-G-000067, Core-Metadaten | Für alle elf migrierten Modelle technisch nachgewiesen |
| U-G-000070 Vollständige Tabelle | Alle Fachinformationen und Bereichsspalte aus Core; vollständige Werte ohne Duplikate. | A-G-00076 | U-G-000069, Core-Metadaten | Implementiert und getestet |
| U-G-000071 Bewegungsvorschau | Achse, Normal-/Referenzstellung und tatsächliche Extreme aus Core; keine Simulationsänderung. | A-G-00077 | U-G-000069, Core-Bewegungsdaten | Implementiert und getestet |
| U-G-000072 Integration und Abnahme | Einzelquellen-/Altbestand, tatsächliche Teilnetze, Felder, Bewegung, Raster, Kamera/Snapshot und DPI geprüft. | A-G-00075 bis A-G-00078 | U-G-000068, U-G-000070, U-G-000071, Core-Nachweise | Gesamtmigration/Integration technisch nachgewiesen; persönliche Abnahme separat |

#### Tatsächliche Ergebnisse dieser Erweiterung

- Release-Build sämtlicher Core-/GUI-Ziele erfolgreich.
- Integrierter Lauf ohne bekannte breite `gui_graphics_render_*`:
  **36/36 bestanden, 212,59 s** (20 Core-, 16 GUI-Fälle).
  Diese Baselinefälle wurden nicht erneut ausgeführt und nicht als grün gezählt.
- Neue `gui_model_catalog`-Prüfung: acht eindeutige Schema-3-Migrationen
  durch bestehenden GUI-Adapter, eine Quelle je Element, Masse/Hüllgrenzen
  unverändert; drei Konfliktboote sicher ohne Zielanlage abgelehnt.
  Nach Ergänzung separat **1/1 bestanden, 29,66 s**.
- Inspektionsprüfungen verwenden weiterhin ein reales Schema-2-Folkeboot
  mit kenntlich gemachten Bestandsquellen. Zusätzlich wird ein echtes
  Schema-3-Quader-Modell über Modellordnerauswahl/Katalog in MainWindow
  geladen und gerendert. Finaler Lauf `gui_boat_inspection_(100|150|200)`:
  **3/3 bestanden, 37,49 s**. Kein GUI-Importer oder neuer Solver.
- Ein Objektbaum ohne Fachreiter; Elemente einmal, Quelle und Ableitungen
  darunter. Bereichsspalte und vollständige Propertywerte aus Core,
  überlappende Auswahl ohne Doppelwerte, feste Snapshots und Kameraerhalt.
- Raster: durchgezogene Linien bei ganzzahligen Meterkoordinaten, punktförmige
  Vertices auf Halbmeterrasterlinien; gleiche Weltteilung auf drei offenen
  Flächen. Linien-/Punkttopologie und Positionen werden numerisch getestet.
- Checkbare Bewegungsvorschauen verwenden ausschließlich Core-Meshposen:
  normal Cyan, Minimum Orange, Maximum Violett, Achse Gelb. Fehlende
  Rudergrenzen werden als `nicht_vorhanden` gezeigt statt erfunden.
  Auswahl oder Checkbox schaltet Vorschauen ohne Änderung von Kamera,
  Quelldaten oder Simulation; Uncheck entfernt die temporären Grafiken.
- Standardstartpfad `C:\spdeps\guibuild\Release\segelphysik_gui.exe`
  erfolgreich mit dieser Erweiterung neu gebaut.

Aktuelle Bilder unter
`C:\spdeps\gui-workspace-tabs\graphics-evidence\boat-inspection-100`:
`boat_unified_source.png`, `boat_unified_derived.png`,
`boat_movement_preview.png`, `boat_schema3.png` aus gleichnamigen BMPs.
Visuell geprüft: gemeinsame Hierarchie, feste Linien-/Punktteilung,
Quellkennzeichnung, Bewegungsvorschau und Tabelle fehlender Grenzen.

**Historischer Sperrstand vor Ableitungsfreigabe:** Die reale Geometrie unterscheidet sich bei `420`, `folkeboot`
und `optimist`. Core verweigert deren Schema-3-Migration explizit mit
CORE-MOD-0009; Originale bleiben unverändert und nutzbar. Keine
eigenmächtige Quellwahl oder Ergebnisänderung. Die fachliche Rückfrage
blieb ohne verfügbare Antwort; diese drei Fälle blockieren die vollständige
Einzelquellenabnahme. Editor/Speichern und persönliche Sichtabnahme sind
weiterhin nicht geliefert bzw. offen. Kein Commit/Push/Tag/Release.
Die inzwischen erteilte Nutzerfreigabe und die folgende Lieferung lösen
die Migrationssperre auf; Editor/Speichern bleiben außerhalb dieses Auftrags.

Alle elf Schema-3-Modelle sind inzwischen dauerhaft unter
`C:\spdeps\gui-workspace-tabs\single-source-models` verfügbar; dieser Ordner
kann in der GUI ausgewählt werden. Die Original-Modellauswahl wurde nicht
automatisch umgestellt. Der
[vertiefte Core-Konfliktnachweis](../../CORE/Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md#vertiefter-konfliktnachweis-und-persistierte-migration)
enthält Flächen, Volumina, räumliche Ausschnitte und die Massenfolgen
einer Quelländerung. Insbesondere würden geschlossene Ersatznetze für
offene Masseschalen die bisherige Flutrandinformation verlieren;
das ist keine reine Dateibereinigung.
Deshalb verwendet die abschließende Migration eine explizite
Deckflächenauswahl statt eines Austauschs der Masseschale.

#### Abschließende Lieferung nach Freigabe der Teilgeometrien

Der Nutzer bestätigt Option 1: eine Quelle pro Element, bestehende Physik
über reproduzierbare Verwendungen/Teilnetze ausreichend genau erhalten;
keine Bitgleichheit gefordert. Core übernimmt Ableitung und Prüfung,
die GUI stellt nur Snapshot-Ergebnisse dar.

- Alle elf Modelle über den vorhandenen GUI-Katalog geladen: eindeutige
  Quelle je Element, Masse und Hüllgrenzen erhalten. Der strikte Konverter
  bleibt unverändert konfliktverweigernd; `--schema3-ableitungen` ist
  die ausdrückliche zusätzliche Freigabe für Ebenenschnitte/Flächenauswahl.
- Neue Teilnetze für Auftrieb, Masseschale oder Darstellung stehen unter
  „Abgeleitete Daten“. Tooltip und Tabelle zeigen die tatsächliche
  Definition. Die Verwendungsübersicht enthält sowohl unveränderte
  Verwendungen als auch `hydrostatik_volumen (Ableitung)` usw.
- Reales Schema-3-Optimist-Modell in MainWindow geladen und gerendert;
  Quelle einmal je Element, Auftriebsschnitt und offene Masseschale als
  Ableitungen, vollständige Tabellenwerte. Bild:
  `boat_schema3_derived.png` im bestehenden Evidenzordner.
- Gemeinsamer gezielter Lauf: **6/6 bestanden, 163,13 s**
  (Core-Migration/Inspektion, GUI-Katalog und drei DPI-Inspektionstests).
  Nach abschließender Ergänzung der Stützstellen-/Negativtests und
  Verwendungszeile erneut **4/4 bestanden, 115,14 s**
  (`modell_geometrie_v2`, `gui_boat_inspection_100/150/200`).
- Core-Vergleich für die drei Boote: Masse, Schwerpunkt, Trägheit,
  Massenstützstellen, offene Flutränder, Kräfte/Momente, vier Wasserlagen
  und 0,2-s-Dynamik geprüft. Größte Positionsabweichung ca. 1,19 µm;
  größte Geschwindigkeitsabweichung ca. 1,19e-5 m/s.
  Siehe tatsächliche Werte und Grenzen im Core-Plan.
- Standardstartpfad `C:\spdeps\guibuild\Release\segelphysik_gui.exe`
  mit abschließender Lieferung erfolgreich neu gebaut. Neuer Modellordner
  ist explizit auswählbar; keine Änderung der Originale oder heimliche
  Umstellung gespeicherter Nutzereinstellungen.

Damit bestehen keine offenen technischen Migrationsentscheidungen für diese
elf Referenzmodelle mehr. Persönliche Sichtabnahme und allgemeine
Langzeit-/Grafikbaselineabnahme werden davon nicht vorweggenommen.
Kein Commit/Push/Tag/Release.

#### Rückverfolgbarkeit der Erweiterung

| Anforderung / Spezifikationsabschnitt | Umsetzung | Tatsächlicher Nachweis | Status |
|---|---|---|---|
| [A-G-00075](../Spezifikationen/Spezifikation_GUI_Bootsauswahl.md#a-g-00075-ein-element-eine-quelle-ein-objektbaum) | U-G-000067, U-G-000069, U-G-000072 | Katalogvergleich aller elf migrierten Modelle; Schema-3-Laden und reale Teilnetze über MainWindow; eindeutiger Baum und Quell-/Ableitungszweige | Technisch nachgewiesen; persönliche Abnahme separat |
| [A-G-00076](../Spezifikationen/Spezifikation_GUI_Bootsauswahl.md#a-g-00076-sämtliche-objektinformationen-in-einer-tabelle) | U-G-000067, U-G-000070, U-G-000072 | Core-Feldinventar und Inspektionsprüfung: sechs Spalten, Bereiche, strukturierte Werte, deduplizierte Mehrfachauswahl | Technisch nachgewiesen; persönliche Abnahme offen |
| [A-G-00077](../Spezifikationen/Spezifikation_GUI_Bootsauswahl.md#a-g-00077-bewegung-direkt-am-element-sichtbar-machen) | U-G-000067, U-G-000071, U-G-000072 | Core-Bewegungsvergleich und drei DPI-Prüfungen: Checkbox, Achse, Posen, fehlende Grenzen, unveränderte Kamera | Technisch nachgewiesen; persönliche Abnahme offen |
| [A-G-00078](../Spezifikationen/Spezifikation_GUI_Bootsauswahl.md#a-g-00078-festes-metrisches-koordinatenraster) | U-G-000067, U-G-000068, U-G-000072 | Numerische Linien-/Vertexprüfung und tatsächliche 100-/150-/200-%-Renderings | Technisch nachgewiesen; persönliche Abnahme offen |

Raster ist nach Freigabe unabhängig von Core-Migration und Baum/Tabelle
umsetzbar. Tabelle und Bewegungsvorschau können nach dem gemeinsamen
Baum parallel vorbereitet werden. Keine doppelte GUI-Schemainterpretation,
kein zweiter Importer und keine neue Physik.

#### Meilensteine

| Meilenstein | Eintritt | Überprüfbares Austrittskriterium |
|---|---|---|
| Freigegebener Vertrag | Spezifikation und Plan finalisiert, Umsetzung beauftragt | Anforderungen/Schritte nummeriert und zugeordnet; Core-/GUI-Verantwortlichkeiten geklärt. |
| Raster nachgewiesen | Rasterpaket implementiert | −1/0/1 m solid, −0,5/0,5 m dotted; einheitliche Meterabstände in X/Y/Z unabhängig von Zoom, Kamerabewegung und DPI. |
| Objektbedienung integriert | Core-Metadaten und Baum-/Tabellenpakete geliefert | Jedes Element einmal, sämtliche vorhandenen Fachinformationen zugänglich; Nachkommen/Mehrfachauswahl ohne doppelte Eigenschaften. |
| Vorschauen und Regression geprüft | Bewegungsdaten und Vorschau geliefert | Achsen/Normal-/Extremstellungen korrekt, fehlende Grenzen explizit; Quell- und Simulationsdaten unverändert. |
| Technisch abgeschlossen | Alle Pakete und nötige Core-Migration nachgewiesen | Tests und tatsächliche Renderings dokumentiert, alle Entwurfspunkte nach Finalisierung auf Anforderungen/Nachweise abgebildet; persönliche Nutzerabnahme separat offen oder bestätigt. |

#### Prüfstrategie und Übergabe

- Rasterpositionen numerisch aus der tatsächlich gerenderten Geometrie
  prüfen, Punktmuster zusätzlich visuell; keine Prüfung allein anhand
  von UI-Labels. Ganze Meter nicht doppelt als dotted überlagern.
- Bei 100/150/200 % und geänderter Fenstergröße Clipmaske, Linienlesbarkeit,
  metrische Teilung und Tabellenbedienung prüfen; Kamera darf beim
  Auswahl-/Überlagerungswechsel nicht springen.
- Fachinformationen gegen das vollständige Core-Inventar vergleichen,
  einschließlich Typen, Einheiten, Status, strukturierter Werte und
  Bewegung. Einzel-/Gruppen-/Mehrfachauswahl sowie leere Auswahl testen.
- Bewegung mit vorhandenen Referenzdefinitionen und Transformationen
  vergleichen; fehlende Grenzen und verkettete Bezüge abdecken. Reine
  Vorschau darf weder Simulation starten noch ihren Zustand verändern.
- Altbestand und migrierte Einzelquellenmodelle getrennt kennzeichnen;
  ausstehende Core-Migrationskonflikte blockieren die vollständige
  Einzelquellenabnahme, nicht den unabhängigen Rasternachweis.
- Zuerst passende CMake-Ziele und gezielte Grafik-/Katalog-/Meshtests,
  danach integrierte Regression nach tatsächlichem Änderungsumfang.
  Bekannte breite Grafikbaselinefehler getrennt berichten.
- Standardstartpfad nach erfolgreichem Build aktualisieren. Bei gesperrter
  EXE laufenden Prozess nicht ungefragt beenden; Abweichung und verfügbaren
  neuen Buildpfad ausdrücklich melden.

Nachweise, Bilder, Befehle und offene Punkte werden in diesem Plan ergänzt,
erst wenn tatsächlich ausgeführt. Rasterteilung, Gesamtbaum und Vorschauen
sind implementiert; abschließende Integrationsnachweise folgen unten.
Keine Meshvereinfachung, kein Editor/Speichern und keine
Commit-/Push-/Tag-/Releasefreigabe.

Geplant ist die Umgestaltung des Reiters **Bootsauswahl** für Modellauswahl,
Objektbauminspektion, grafischen Auswahlfokus und tabellarische Werte.
Ansichtsspezifische Änderungen betreffen nur diesen Reiter.

Übergreifend werden ausschließlich die ausdrücklich beschriebenen
Änderungen vorgesehen:

- Titel und Untertitel oberhalb der drei Ansichtsauswahlreiter;
- Modellordnerbutton ausschließlich in der Bootsauswahl.

Die vorhandene Simulation, Physik und die nicht betroffenen Bedienabläufe
der statischen und dynamischen Ansicht bleiben erhalten. Die derzeit
identischen Inhalte der drei Reiter werden nur im freigegebenen Umfang
aufgetrennt. Eine neue Windphysik oder andere Core-Physik gehört nicht
zu diesem Plan.

## 2. Voraussetzungen und Festlegungen

Bestätigter Datenlebenszyklus: einmal vom Core eingelesener fester Stand;
keine automatische Ersetzung oder Entwertung durch Dateiänderungen.
Detailabfragen nutzen denselben Bestand. Tests müssen dies für Bäume,
Werte und Meshes nachweisen. Mesh-Updates, Mesh-Modifikationen und
weitergehende Neuladebedienung sind spätere Themen.

Vor Implementierungsbeginn:

- Gesonderten Implementierungsauftrag erhalten.
- Tatsächliche Core-Lieferung gegen den finalen Vertrag prüfen:
  fünf bedingte Baumsichten, sechs bedingte Mesh-Arten und vollständige
  Eigenschaften. Fehlende Lieferung nicht durch GUI-Dateileser ersetzen.
- Ausgangsfarben Wasser `#65283F`, Himmel `#C695A5`; Sichtabnahme getrennt.

Am 01.11.2026 fachlich bestätigt: K1 Mausbedienung ohne Navigationsanzeigen;
K2 transparenter Glaskontext; K3 gesamtes Boot normal ohne Auswahl;
K5 Werte einschließlich aller Unterobjekte; K6 gemeinsame Tabelle mit
Objekt/Pfad, ohne Duplikate oder Summierung; K7 gedecktes Bordeauxwasser
und hellerer Bordeaux-Rosé-Himmel, genaue Töne per Vorschau.
K4 und der feste Snapshot sind entschieden; der Core-Vertrag ist final
spezifiziert. Keine Implementierung ist damit beauftragt.

Vorhandene uncommittete Änderungen, insbesondere die bereits eingeführten
farbigen Ansichtsauswahlreiter, sind zu erhalten. Vor Arbeitsbeginn Branch,
Arbeitsbaum, GUI-/Core-Stand, Buildpfad und Testbaseline festhalten.

## 3. Arbeitspakete und Abhängigkeiten

U-G-000055 bis U-G-000062 sind technisch abgeschlossen. Nachweise stehen
in Abschnitt 6; persönliche Sichtabnahme ist ausdrücklich nicht erfolgt.

| U-ID / Paket | Geplantes Ergebnis | Anforderungen | Voraussetzung |
|---|---|---|---|
| U-G-000055 Ausgangsstand und Core-Bindung | Baseline und C-0.8-Lieferung einschließlich U-C-000085 prüfen; bestehende Katalog-/Anzeige-/Simulationspfade auf gemeinsame Core-Pipeline und GUI-Konvertierungen anbinden, keine zweite Modellverwaltung. Charakterisierungstests vor Migration; Snapshot im Worker öffnen, Besitz und Ergebnisse an Modellwahl binden. | A-G-00060, A-G-00067, A-G-00068, A-G-00069 | Implementierungsauftrag; Core U-C-000084 geliefert |
| U-G-000056 Kopfbereich und Layout | Titelblock über Reiter, Ordnerbutton unter linke Liste, Spaltenhöhe und Sichtbarkeit; rechte Simulation nur außerhalb Bootsauswahl. | A-G-00057, A-G-00058, A-G-00064, A-G-00068 | U-G-000055 |
| U-G-000057 Bootsauswahl-Darstellung | Snapshotgeometrie und alle Mesh-Arten anbinden; Maus erhalten, Instrumente ausblenden; Bordeauxwasser/-himmel nur hier. | A-G-00059, A-G-00060, A-G-00066, A-G-00067 | U-G-000056 |
| U-G-000058 Objektbäume und Auswahl | Core-Sichten als Baumreiter, vollständige Knoten und Ctrl-/Shift-/Gruppenauswahl über IDs; Reiterwechsel räumt Knotenauswahl auf. | A-G-00061, A-G-00062, A-G-00065 | U-G-000056 |
| U-G-000059 Grafischer Fokus | Elementvereinigung abbilden; Deckkraft Auswahl/Glaskontext, ohne Auswahl ganzes Modell; keine Geometrieänderung. | A-G-00065, A-G-00067 | U-G-000057, U-G-000058 |
| U-G-000060 Wertetabelle | Auswahl mit Nachkommen abfragen, Objekt/Pfad und Status/Einheiten, nichtskalare Werte ohne Verlust, keine Duplikate/Summen. | A-G-00063, A-G-00067 | U-G-000058 |
| U-G-000061 Integration und Fehler | Dateiänderungs-/Worker-Rennen testen; fehlende Meshteile/fehlerhafte Modelle sichtbar; Statik/Dynamik und Wiedergabe erhalten; Altverhalten und gemeinsame Anbindung nachweisen. | A-G-00060, A-G-00067, A-G-00068, A-G-00069 | U-G-000059, U-G-000060 |
| U-G-000062 Abschlussnachweise | Geometrie-/Zustandstests, 100/150/200-%-Renderings, Build, Matrix A→U→Test; persönliche Abnahme separat protokollieren. | A-G-00057 bis A-G-00069 | U-G-000061 |

Die Bootsauswahl-Darstellung und die Objektbaumanbindung können nach dem
Layout unabhängig bearbeitet werden. Grafischer Fokus benötigt beide.
Die Wertetabelle benötigt den Auswahl- und Datenvertrag, nicht den
abgeschlossenen grafischen Fokus.

## 4. Meilensteine

| Meilenstein | Eintritt | Überprüfbares Abnahmekriterium | Aktueller Stand |
|---|---|---|---|
| Fachlich spezifiziert | Nutzerentscheidungen liegen vor. | Finale Spezifikation, Zielversion und A-/U-Zuordnung dokumentiert. | Dokumentiert 01.11.2026, noch nicht implementiert |
| Datenbindung geliefert | Umsetzungsauftrag und Core-Lieferung liegen vor. | U-G-000055 weist die echten APIs und Tests der Core-Lieferung nach. | Technisch nachgewiesen, Abschnitt 6 |
| Grundlayout integriert | Datenbindung und Bestandsprüfung abgeschlossen. | Titelreihenfolge, Buttonposition/-sichtbarkeit, gleiche Spaltenhöhe sowie isolierte Navigations- und Umgebungsdarstellung sind geprüft. | Technisch nachgewiesen bei 100/150/200 % |
| Objektinspektion integriert | Grundlayout sowie Baum- und Wertevertrag liegen vor. | Einzel-, Mehrfach- und Gruppenauswahl steuern konsistent Baum, grafischen Fokus und Tabelle. | Technisch nachgewiesen, Abschnitt 6 |
| Technisch nachgewiesen | Alle Implementierungspakete integriert. | GUI baut; gezielte Tests und Regressionsprüfungen sind ausgeführt; Ergebnisse, Abweichungen und Screenshots sind dokumentiert. | 35/35 ausgewählte Regressionen bestanden; breiter Grafik-Baselinefehler bleibt |
| Fachlich abgenommen | Technische Nachweise und bekannte Einschränkungen liegen vor. | Nutzer bestätigt die sichtbare Bedienung und Darstellung; verbleibende Punkte sind ausdrücklich entschieden. | Offen |

Ein erfolgreicher Build oder ein Screenshot allein ersetzt keine
fachliche Abnahme. Blockierte und teilweise erledigte Pakete bleiben
entsprechend markiert.

## 5. Prüf- und Nachweiskonzept

Vorhandene CMake-/CTest- und Qt-/VTK-Integrationstests erweitern, statt
einen parallelen Testrahmen einzuführen. Zunächst die jeweils betroffenen
Tests ausführen; anschließend die für die Integration notwendigen
Regressionen.

| Prüfbereich | Geplanter Nachweis |
|---|---|
| Kopfbereich und Modellordnerbutton | Titel oberhalb der Reiter; Button unter der Modellauswahl; in beiden anderen Ansichten nicht sichtbar. |
| Höhe und Skalierung | Tatsächliche Geometrie der linken Gesamtspalte gegen das Darstellungsfenster prüfen; vorhandene 100-/150-/200-%-Skalierung berücksichtigen. |
| Navigation und Umgebung | Vereinbarte Elemente in Bootsauswahl nicht sichtbar; Wasser/Himmel entsprechen den vereinbarten Farbtönen; Rückwechsel stellt die übrigen Ansichten korrekt wieder her. |
| Modell und Bäume | Modellwechsel aktualisiert Darstellung und verfügbare Bäume; alle vorgesehenen Teile und Hierarchien werden gegen die maßgebliche Datenquelle geprüft. |
| Auswahlfokus | Einzelobjekt, mehrere Objekte und Gruppe prüfen; Auswahl klar erkennbar, andere Teile weiterhin als Kontext sichtbar. |
| Werte | Auswahl einschließlich aller Unterobjekte und Mehrfachauswahl anhand definierter Modelldaten prüfen; keine erfundenen oder fremden Snapshotwerte. |
| Integration | Wechsel zwischen Modellen, Bäumen und Hauptreitern ohne falsche Teilzuordnung oder veraltete Auswahl; Simulation und Wiedergabe in den anderen Ansichten regressionsprüfen. |
| Fehlerfälle | Fehlende oder ungültige Daten sichtbar nach Repository-Muster behandeln; kein scheinbar erfolgreicher leerer Ersatzbaum. |
| Gemeinsame Anbindung | Bestehende Katalog-/Anzeige-/Simulationsfälle vor/nach Migration vergleichen; Aufrufpfade zur gemeinsamen Core-Pipeline, gemeinsame Geometriekonvertierung und getrennte Datenlebenszyklen belegen. Keine sichtbare Änderung allein durch Refactoring. |
| Sichtabnahme | Tatsächliche Renderings der Bootsauswahl und Vergleichsansichten; Nutzer prüft Lesbarkeit, Gruppenauswahl, räumlichen Kontext und Bordeauxfarbgebung. |

Ausgangsstand vom 01.11.2026: Die Tests der bereits eingeführten farbigen
Hauptreiter bestanden bei 100/150/200 % Skalierung. Der breitere
Grafiktest meldete „420 disabling course trail must restore the exact image“
auch auf unverändertem GUI-HEAD. Dies ist ein bekannter Baselinebefund,
kein Nachweis für die noch nicht umgesetzte Bootsauswahl. Vor Beginn
den aktuellen Befund erneut feststellen und getrennt berichten.

## 6. Fortschritt und Abschluss

### Bestätigte Darstellungsvorschau 02.11.2026

| Schritt | Anforderungen | Abhängigkeit | Status |
|---|---|---|---|
| U-G-000066 Abgerundete Inspektion und Modellraster | A-G-00074 | Bestehender Inspektionsviewport U-G-000057 | Technisch abgeschlossen; persönliche Abnahme offen |

Nur Bootsauswahl: gerundete Render-Clipmaske mit Rahmen, Wasser/Tiefe und
Skybox aus, dunkler Bordeauxverlauf; Modellachsen am Ursprung und
modellfestes räumliches Linienraster mit einheitlicher Schrittweite.
Keine Änderung an Physik, Quellmeshzuordnung oder fachlichen Baumreitern.

Nachweise: Release-Build GUI und Grafiktest erfolgreich.
`gui_boat_inspection_100/150/200` prüfen tatsächliche Clipmaske
(Ecke ausgeschlossen, Mitte enthalten), sichtbare Modellachsen am Ursprung,
räumliches Linienraster und keine sichtbaren Wasser-/Tiefenpolygone.
Abschließend **3/3 bestanden, 30,56 s**, einschließlich kompakter farbiger
Achsenbeschriftung; Auswahl-/Modellwechsel erhalten die Achsen.
`gui_workspace_tabs_100/150/200`, `gui_navigation_camera` und
`gui_water_motion_render`: **5/5 bestanden, 69,98 s**. Damit acht gezielte
Fälle bestanden; kein neuer Gesamtsuitenlauf behauptet.

Aktuelles Bild visuell geprüft:
`C:\spdeps\gui-workspace-tabs\graphics-evidence\boat-inspection-100\boat_grid_actual.png`
(konvertiert aus `boat_unified_source.bmp`).
Neuer lauffähiger Stand:
`C:\spdeps\gui-workspace-tabs\Release\segelphysik_gui.exe`.
Aktualisierung des üblichen Startpfads
`C:\spdeps\guibuild\Release\segelphysik_gui.exe` durch laufenden Prozess
gesperrt (Linker LNK1104); Prozess nicht beendet, dort bleibt der vorherige
Stand. Quelländerungen uncommittet, kein Push/Tag/Release.

Nachtrag 02.11.2026, 11:32 Uhr: Nach Schließen der GUI durch den Nutzer
Release-Build am üblichen Startpfad erneut ausgeführt und erfolgreich
abgeschlossen. `C:\spdeps\guibuild\Release\segelphysik_gui.exe` ist nun
ebenfalls aktualisiert; die vorherige Dateisperre ist behoben.

### Ergänzung 02.11.2026: Einheitlicher Definitionspfad

Finaler Auftrag Alternative 2: ID und Rolle getrennt; die Anforderungen
A-G-00070 bis A-G-00073 ergänzen den bestehenden G-0.8-Umfang.
Kein paralleler Versionsplan. Stand: technisch umgesetzt und integriert;
persönliche fachliche Abnahme offen.

| Schritt | Ergebnis | Anforderungen | Abhängigkeit / Status |
|---|---|---|---|
| U-G-000063 Definitionsvertrag anbinden | Core-Dateivertrag und Herkunftsmetadaten über bestehende Snapshot-API verwenden; keine Namenheuristik. | A-G-00070, A-G-00072 | Neuer Core-Vertrag geliefert; technisch abgeschlossen |
| U-G-000064 Quelle/Ableitung integrieren | Baumgeometrieknoten und explizite Herkunft, Mehrfachauswahl/Überlagerung; separate Meshcombo entfernen. | A-G-00071, A-G-00072 | U-G-000063 erfüllt; technisch abgeschlossen |
| U-G-000065 Kompatibilität nachweisen | Alt-/Neuformat, Rollen/IDs, Quelldeduplikation, Katalog/Simulation und DPI-Renderings prüfen; tatsächliche Resultate dokumentieren. | A-G-00070 bis A-G-00073 | U-G-000064 und Core-Lieferung erfüllt; technische Tests bestanden, persönliche Abnahme offen |

#### Neue tatsächliche Nachweise vom 02.11.2026

- Gemeinsamer Release-Build einschließlich aller Tests erfolgreich.
  Integrierter CTest-Lauf ohne die bekannten breiten
  `gui_graphics_render_*`-Fälle: **36/36 bestanden**, 216,29 s
  (20 Core- und 16 GUI-Fälle).
- `gui_model_catalog` konvertiert alle elf Referenzmodelle ausschließlich
  in temporäre Ordner und vergleicht Alt-/Neuformat über die bestehenden
  GUI-Adapter: Masse, Hüllquader, Gruppen, Anzahl Bewegungen/Anzeigeteile
  sowie exakte Punkte, Dreiecke und Materialien aller bisherigen Mesharten.
  Zusätzlich bleibt die explizite Rolle `ruder` bei neutraler ID
  `teil_017` einschließlich Bewegung erhalten.
- Core-Prüffall `modell_geometrie_v2` bestanden (48,56 s im integrierten
  Lauf): Modell-/Physik-/Bewegungs- und Simulationsgleichheit sowie
  geschützte Konvertierung. Originalmodelle bleiben unverändert.
- `gui_boat_inspection_100/150/200` verwenden jetzt tatsächlich das
  konvertierte Schema 2: Quellen und Ableitungen auswählbar, Kontrollkästchen
  halten Überlagerungen, Quelldeduplikation, Kameraerhalt, keine Meshcombo
  oder Bewegungstabs und unveränderlicher Snapshot trotz Dateibeschädigung.
  Nach abschließender Ergänzung der Bilder erneut **3/3 bestanden**,
  44,61 s. PNG-Ausgabe ist im Testlauf nicht verfügbar; deshalb werden wie
  in den vorhandenen Tests BMP-Nachweise erzeugt und bei Bedarf konvertiert.
- Visuell geprüfte aktuelle Nachweise:
  `C:\spdeps\gui-workspace-tabs\graphics-evidence\boat-inspection-100\boat_unified_source.png`
  und `boat_unified_derived.png` (aus den gleichnamigen BMP-Dateien).
  Quellfarben, goldene Ableitung, Herkunftstabelle und separierte
  Baumbereiche sind sichtbar; konvertiertes Folkeboot ist verwendbar.
- Standardstartpfad
  `C:\spdeps\guibuild\Release\segelphysik_gui.exe` neu gebaut.
  CLI-Konvertierung und der verbindliche Core-Formatvertrag sind im
  GUI-README verlinkt; kein eigener GUI-Importer oder Editor eingeführt.

| Anforderungen | Umsetzung | Aktueller Nachweis |
|---|---|---|
| A-G-00070 | U-G-000063, U-G-000065 | Neutral-ID-/Rollenprüfung im migrierten Modell, Herkunftsdaten aus Core |
| A-G-00071 | U-G-000064, U-G-000065 | Drei DPI-Inspektionsfälle und geprüfte Quelle-/Ableitungsbilder |
| A-G-00072 | U-G-000063, U-G-000064, U-G-000065 | Elf Alt-/Neumodelle über `loadDisplayData`, gemeinsame Geometriekonvertierung, fester Snapshot |
| A-G-00073 | U-G-000065 | 36 integrierte Fälle, abschließend drei DPI-Fälle; getrennte Core-Simulationsgleichheit |

Bekannte Einschränkungen: Der frühere breite Grafik-Baselinefehler bleibt
offen; diese drei breiten Grafikfälle wurden für diesen Erweiterungslauf
nicht erneut ausgeführt und nicht als grün gezählt. Persönliche Abnahme,
Editor sowie Achs-/Extremvorschauen bleiben ausstehend bzw. außerhalb des
Auftrags. Keine Änderungen an Originalmodellen, kein Commit/Push/Tag/Release.

Vorhandene C-0.8/G-0.8-Nachweise nachfolgend bleiben historische
Ausgangsnachweise; sie belegen die neue Erweiterung nicht automatisch.

### Arbeitsstand 01.11.2026

- Ausgangsbranch GUI und Core: `main`; vorhandene uncommittete Änderungen
  der farbigen Hauptreiter und Vertragsdokumente erhalten.
- GUI-Arbeitsbuild `C:\spdeps\gui-workspace-tabs`, Release, C++17,
  Qt/VTK; vorhandene CMake-/CTest-Werkzeuge, GPU-Tests eingeschaltet.
- Neue `BoatInspection` bindet Baumreiter, Meshwahl, Mehrfachauswahl,
  Wertetabelle und eigenen Viewport an denselben Core-Snapshot.
  Getrennter Viewport erhält Simulation und deren Kamera beim Wechsel;
  gemeinsame Geometriekonvertierung statt zweitem Leser.
- Titel/Layout und Ordnerbutton umgeordnet; Watcher ersetzt keinen
  geöffneten Inspektionsstand. Worker-Ergebnisse sind generationsgebunden.
- GUI und sämtliche Testtargets bauen einschließlich neuer Core-Funktionen.
  Der Standardstartpfad `C:\spdeps\guibuild\Release\segelphysik_gui.exe`
  ist ebenfalls neu gebaut; kein alter Stand am üblichen Startpfad.
- Neue Prüffälle `gui_boat_inspection_100/150/200`: tatsächliche
  Viewport-/Spaltenhöhe mit ≤1 logischem Pixel, Baum-/Meshwechsel,
  deduplizierte Werte, festes Snapshotverhalten nach Dateibeschädigung,
  Punktdaten, Opazität und Erhalt der Simulationskamera.

### Tatsächliche technische Nachweise

Verwendete Werkzeuge: CMake/CTest aus Visual Studio BuildTools,
Konfiguration Release, `PATH` um `C:\spdeps\installed\x64-windows\bin`
ergänzt. Keine neue Testinfrastruktur oder zusätzlichen Pakete installiert.

```powershell
cmake --build C:\spdeps\gui-workspace-tabs --config Release --parallel 4
ctest --test-dir C:\spdeps\gui-workspace-tabs -C Release -E "^gui_graphics_render_" --output-on-failure
ctest --test-dir C:\spdeps\gui-workspace-tabs -C Release -R "^(gui_boat_inspection_(100|150|200)|gui_workspace_tabs_(100|150|200))$" --output-on-failure
cmake --build C:\spdeps\guibuild --config Release --target segelphysik_gui --parallel 4
```

- Integrierte Regressionen: **35/35 bestanden**, davon 19 Core- und
  16 GUI-Fälle; Gesamtzeit im dokumentierten Lauf 116,10 s.
- Nach letzter Anpassung von Bordeauxumgebung und direkter typisierter
  Wertedarstellung: sechs DPI-/Workspacefälle erneut **6/6 bestanden**
  (57,69 s). Die GUI verwendet C++-Propertydaten ohne JSON-Roundtrip;
  JSON wird nur zur verlustfreien Zellformatierung strukturierter Werte genutzt.
- `gui_graphics_render_100` separat erneut ausgeführt: weiterhin
  **fehlgeschlagen** mit „420 disabling course trail must restore the exact image“,
  wie bereits vor diesem Auftrag auf sauberem HEAD reproduziert.
  Die breiten 150-/200-%-Grafikfälle wurden nicht erneut ausgeführt;
  kein vollständiger grüner Gesamtsuitenstatus wird behauptet.
- Tatsächliche Bilder:
  `C:\spdeps\gui-workspace-tabs\graphics-evidence\boat-inspection-100\boat_display.png`,
  außerdem `boat_inspection.bmp`, `workspace_0/1/2.bmp` in den jeweiligen
  `boat-inspection-100/150/200`- und Workspace-Nachweisordnern.
  Das Defaultbild wurde geprüft: Boot, Bordeauxwasser/-himmel,
  Objektbaum, Titelreihenfolge und linker Button sichtbar.

| Anforderungen | Umsetzung | Nachweis |
|---|---|---|
| A-G-00057, A-G-00058, A-G-00064 | U-G-000056 | `gui_boat_inspection_100/150/200`: tatsächliche Grenzen ≤1 logischer Pixel, Buttonposition/-sichtbarkeit und Titelordnung |
| A-G-00059, A-G-00060, A-G-00066 | U-G-000057 | Instrumente unsichtbar, tatsächlicher Snapshot gerendert, Bilder `boat_display.bmp`; bestehende Kameratests bleiben grün |
| A-G-00061, A-G-00062 | U-G-000058 | Alle angebotenen Baumreiter abrufbar, Auswahl beim Baumwechsel zurückgesetzt |
| A-G-00065 | U-G-000059 | Baumknotenauswahl steuert Opazität, Kontext exakt 0,15 und Auswahl 1,0; Überlappungen dedupliziert |
| A-G-00063 | U-G-000060 | Root-/Nachkommenwerte, überlappende Auswahl ohne Duplikate, leere Auswahl ohne alte Werte |
| A-G-00067 | U-G-000055, U-G-000057, U-G-000061 | Alle angebotenen Mesh-Arten, Punktetopologie ohne künstliche Dreiecke, beschädigte Quelldatei ersetzt Snapshot nicht |
| A-G-00068, A-G-00069 | U-G-000055, U-G-000061, U-G-000062 | Gemeinsame Snapshotprojektionen/Geometriekonvertierung, `gui_model_catalog`, `gui_core_integration`, Live-/Wiedergabe-/Bewegungsregression und 19 Core-Tests |

Offen bleibt die persönliche fachliche Abnahme der Bedienung und Farben.
Keine Reparatur-/Schreibfunktion, kein Meshupdate, keine Windphysik.
Alle Änderungen bleiben uncommittet; kein Push, Tag oder Release.

Während der späteren Umsetzung wird diese Datei fortlaufend um tatsächliche
Paketstände, Testbefehle und Ergebnisse, Belegpfade, Abweichungen und offene
Punkte ergänzt. Die fachliche Quelle bleibt die Spezifikation.

Zum Abschluss werden alle finalisierten Anforderungen gegen Schritte und
tatsächliche Nachweise abgeglichen. Nicht gelieferte Datenverträge,
fehlende Nachweise und noch ausstehende Nutzerabnahmen bleiben sichtbar.
Git-Veröffentlichungen erfolgen ausschließlich auf gesonderten Auftrag.
