# Spezifikation GUI: Bootsauswahl und Objektinspektion

**Aktueller Nachfolgevertrag:** Die Nutzerkorrektur vom 06.11.2026,
21:32 Uhr wird in der
[Segelkonfigurator-Spezifikation](Spezifikation_GUI_Segelkonfigurator.md)
und dem zugehoerigen G-0.13-Plan gefuehrt. Sie ersetzt die hier historisch
beschriebene Auswahl zusaetzlicher Quell-/Ableitungsgeometrien im Objektbaum
durch genau eine aktuelle Referenz je Objekt. Physische Quellen und ihre
Metadaten bleiben erhalten. Die historischen Anforderungstexte und
Nachweise unten werden nicht nachtraeglich umgeschrieben.

**Status:** FINAL fachlich spezifiziert am 01.11.2026 auf Nutzerauftrag.
Implementierung am 01.11.2026 beauftragt und technisch nachgewiesen;
persönliche Abnahme offen. Keine Git-Freigabe.
**Stand:** 01.11.2026  
**Zielversion:** G-0.8.
**Entwurfspunkte:** [GUI Bootsauswahl](Entwurfspunkte/GUI_Bootsauswahl.json)
**Umsetzungsplan G-0.8:** [Bootsauswahl und Objektinspektion](../Umsetzungsplaene/Umsetzungsplan_GUI_Bootsauswahl.md)

Diese Datei ist die maßgebliche Spezifikation für Bootsauswahl und
Objektinspektion. Anforderungen A-G-00057 bis A-G-00073 sind nachfolgend
festgelegt; die Herkunft bleibt in der verlinkten Entwurfspunkteliste
nachvollziehbar. Maßgeblicher Liefervertrag ist
[Core C-0.8, Abschnitt 3](../../CORE/Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md#3-c-08-objektbäume-eigenschaften-und-mesh-abfragen).

## 1. Zweck und Geltungsbereich

Im Reiter **Bootsauswahl** soll ein Boot beziehungsweise Objekt ausgewählt,
als Ganzes betrachtet und anhand seiner Teile, Objektbäume und Werte
untersucht werden können.

Die hier beschriebene inhaltliche Umgestaltung betrifft ausschließlich die
Bootsauswahl. Für **Statisches Modell** und **Dynamisches Modell** werden
keine neuen fachlichen Ansichten festgelegt. Zwei ausdrücklich genannte
Auswirkungen betreffen den gemeinsamen Aufbau:

- Der Button **Modellordner wählen** entfällt in den anderen Ansichten.
- Der gemeinsame Titelblock steht oberhalb der drei Ansichtsauswahlreiter.

Die bereits eingeführten Farben bleiben bestehen: Bootsauswahl bordeauxrot,
statisch dunkelblau und dynamisch dunkelgrün. Auch inaktive Ansichtsauswahlreiter
behalten ihre jeweilige Farbe.

## 2. Aufbau der Oberfläche

Die Bootsauswahl gliedert sich von oben nach unten:

1. Titel **SEGELPHYSIK** mit Untertitel
   **Hydrostatik · Gleichgewicht · Bewegung**.
2. Ansichtsauswahlreiter **Bootsauswahl**, **Statisches Modell** und
   **Dynamisches Modell**.
3. Hauptbereich mit Modellauswahl links, Darstellungsfenster in der Mitte
   und Objektbaum rechts.
4. Unteres Fenster **Werte** für die zur Objektauswahl gehörenden Daten.

Die folgende Skizze beschreibt die Anordnung, nicht feste Pixelmaße:

```text
SEGELPHYSIK
Hydrostatik · Gleichgewicht · Bewegung

[Bootsauswahl] [Statisches Modell] [Dynamisches Modell]

+--------------------+------------------------+------------------------+
| Modellauswahl      | Darstellungsfenster    | Objektbaum-Reiter      |
|                    |                        | z. B. Hydrodynamik/GUI |
|                    | Boot / Objekt          |                        |
|                    | mit Auswahlfokus       | Objektbaum mit Teilen  |
|--------------------|                        |                        |
| Modellordner wählen|                        |                        |
+--------------------+------------------------+------------------------+

+--------------------------------------------------------------------+
| Werte der ausgewählten Knoten, tabellarisch                          |
+--------------------------------------------------------------------+
```

### 2.1 Modellauswahl und Ordnerbutton

Der Button **Modellordner wählen** steht unterhalb der Modellauswahl und
ist ausschließlich im Reiter Bootsauswahl sichtbar.

Die Modellauswahl und der darunterliegende Button bilden gemeinsam eine
Spalte. Deren Gesamthöhe entspricht der Höhe des benachbarten
Darstellungsfensters; nicht beide Elemente einzeln erhalten dessen Höhe.

### 2.2 Titel und Untertitel

Der Titelblock und die Ansichtsauswahl tauschen gegenüber dem gegenwärtigen
Aufbau ihre Reihenfolge. Die Texte werden dabei nicht geändert.

## 3. Darstellung und Navigation

Das Darstellungsfenster zeigt das ausgewählte Boot beziehungsweise Objekt.
Die Bootsauswahl enthält keine Navigationselemente. Dies ist keine
Festlegung für die statische oder dynamische Ansicht.

Fachliche Entscheidung K1 vom 01.11.2026: Kompass, Lage- und
Geschwindigkeitsanzeigen werden in der Bootsauswahl ausgeblendet.
Drehen, Verschieben und Zoomen per Maus bleiben möglich.

### 3.1 Farbe von Wasser und Himmel

Im Darstellungsfenster der Bootsauswahl wird die **Wasserebene
bordeauxfarben** und der **Himmel hellbordeaux** dargestellt, passend
zur Farbgebung dieses Reiters. Diese Festlegung betrifft ausschließlich
die Bootsauswahl. Gemäß Entscheidung K7 ist das Wasser gedeckt
bordeauxfarben, der Himmel deutlich heller in Bordeaux-Rosé.
Für die Implementierung gelten Wasser `#65283F` und Himmel `#C695A5`
als prüfbare Ausgangsfarbwerte. Beleuchtung darf die gerenderten Pixel
verändern; die konfigurierte Grundfarbe muss stimmen. Die persönliche
Sichtabnahme bleibt offen und ist kein Implementierungsblocker.

## 4. Objektbäume im rechten Fenster

Das bisherige Rechenfenster dient in der Bootsauswahl der Objektinspektion.
Es zeigt einen menschenlesbaren Objektbaum mit allen Teilen. Die
hierarchischen Beziehungen der Teile sollen darin erkennbar sein.

Oberhalb des Baums befinden sich eigene Reiter zur Auswahl unterschiedlicher
Objektbäume. **Hydrodynamik** und **GUI** sind ausdrücklich genannte Beispiele;
gemäß Nutzerentscheidung sind alle vorhandenen Objektbaumtypen und
fachlichen Eigenschaften anzubieten. Die vollständige Liste liefert die
für C-0.8 vorgesehene Core-Inspektionsschnittstelle, einschließlich
physikalischer Mesh-Arten als lesbare Inspektionsdaten.

Die Objektbaum-Reiter sind von den drei übergeordneten
Ansichtsauswahlreitern zu unterscheiden.

## 5. Objektauswahl und grafischer Fokus

Im rechten Objektbaum können ausgewählt werden:

- ein einzelnes Subobjekt;
- mehrere Subobjekte;
- eine Gruppe.

Das Darstellungsfenster stellt die ausgewählten Objekte beziehungsweise
die ausgewählte Gruppe deutlich heraus. Nicht ausgewählte Elemente werden
nicht vollständig ausgeblendet: Sie bleiben als räumlicher Kontext
erkennbar, sind aber gegenüber der Auswahl klar zurückgenommen.

Fachliche Entscheidung K2: Nicht ausgewählte Teile erscheinen dezent
transparent wie Glas, ausgewählte Teile normal und deckend.
Fachliche Entscheidung K3: Ohne ausgewählten Baumknoten wird das
gesamte Boot normal dargestellt.

## 6. Tabellarische Werte

### Fester Datenstand

Die Bootsauswahl verwendet den einmal vom Core eingelesenen
Inspektionsstand. Dateiänderungen aktualisieren oder entwerten weder
Objektbäume noch Werte oder Meshes. Detailabfragen beziehen sich auf
denselben festen Bestand. Ein neuer Stand erfordert ein ausdrücklich
angefordertes neues Einlesen. Mesh-Updates, Mesh-Modifikationen und
weitergehende Neuladebedienung werden später besprochen und sind nicht
Bestandteil dieses Umfangs. Bestehende Simulationsverträge bleiben unberührt.

### Werteumfang

Das untere Fenster **Werte** zeigt die Daten zur Auswahl im Objektbaum
tabellarisch. Gemäß Entscheidung K5 gilt **Mit Unterobjekten**:
Einbezogen werden die ausgewählten Knoten und sämtliche Nachkommen.
Eine zusätzliche Umschaltung auf „Nur Auswahl“ ist nicht beauftragt.

Gemäß Entscheidung K6 werden bei Mehrfach- und Gruppenauswahl die Werte
gemeinsam mit einer Spalte **Objekt/Pfad** zur eindeutigen Zuordnung
aufgelistet. Überschneidungen werden nur einmal angezeigt; Werte werden
nicht automatisch summiert.

## 7. Anforderungen und Abnahme

Die folgenden Prüfpunkte beschreiben den bereits gesammelten Zielzustand;
sie behaupten keine erfolgte Umsetzung oder Abnahme.

| Anforderung / Gegenstand | Vorgesehene Prüfung | Herkunft |
|---|---|---|
| A-G-00057 Ordnerbutton | Unter der Modellauswahl sichtbar; in statischer und dynamischer Ansicht nicht sichtbar. | IDEE-G-00034 |
| A-G-00058 Höhe | Linke Gesamtspalte und Darstellungsfenster haben dieselben oberen/unteren Layoutgrenzen; Toleranz maximal 1 logischer Pixel bei 100/150/200 % Skalierung. | IDEE-G-00035 |
| A-G-00059 Navigation | Kompass, Lage- und Geschwindigkeitsanzeigen fehlen in Bootsauswahl; Mausdrehen/-verschieben/-zoomen bleiben bedienbar, Verhalten anderer Ansichten bleibt erhalten. | IDEE-G-00036 |
| A-G-00060 Objektanzeige | Modellwahl zeigt Geometrie aus Core-Snapshot, ohne GUI-Dateileser oder Physikinterpretation; ohne Teile expliziter Status statt Ersatzobjekt. | IDEE-G-00037 |
| A-G-00061 Objektbaum | Alle vom Core gelieferten Teile, Fachblöcke und Beziehungen erscheinen menschenlesbar rechts; keine Heuristik nach Elementnamen. | IDEE-G-00038 |
| A-G-00062 Baumauswahl | Für jeden vom Snapshot verfügbaren Baumtyp existiert ein eigener Reiter mit Core-Anzeigename; Wechsel zeigt exakt dessen Baum. | IDEE-G-00039 |
| A-G-00063 Werte | Auswahl und alle Nachkommen tabellarisch, Objekt/Pfad und Eigenschaft, Wert, Einheit, Status; keine Duplikate oder automatische Summe. | IDEE-G-00040 |
| A-G-00064 Kopfbereich | Titel und unveränderter Untertitel stehen oberhalb der Ansichtsauswahlreiter. | IDEE-G-00041 |
| A-G-00065 Grafischer Fokus | Einzel-, Mehrfach- und Gruppenauswahl mit Ctrl/Shift; Auswahl normal/deckend, Kontext glasartig transparent, ohne Auswahl gesamtes Boot normal. | IDEE-G-00042 |
| A-G-00066 Wasser und Himmel | Ausschließlich Bootsauswahl: Wassergrundfarbe `#65283F`, Himmel `#C695A5`; Rückwechsel stellt vorheriges Grafikprofil der anderen Ansicht wieder her. | IDEE-G-00043 |
| A-G-00067 Fester Core-Stand und Mesh-Arten | Unveränderlicher C-0.8-Snapshot für Baum, Werte und Geometrie; Dateiänderungen bewirken keine Ersetzung/Entwertung; alle verfügbaren Mesh-Arten inspizierbar. | Nutzerentscheidungen K4 und fester Datenstand |
| A-G-00068 Integration, Fehler und Nachweise | Keine Veränderung von Physik/Simulation durch Inspektion; Diagnosen sichtbar, Build und fokussierte Tests bei 100/150/200 %, persönliche Sichtabnahme getrennt. | Projektregeln und Nutzerentscheidungen |
| A-G-00069 Gemeinsame Core-Anbindung | Bestehende und neue Anzeigen verwenden die gemeinsame Core-Aufbereitung nach A-C-00104 und gemeinsame GUI-Konvertierungen; keine parallelen Leser. Technische Migration verändert keine bisher sichtbare Funktion. | Nutzerergänzung vom 01.11.2026 |

## 8. Verbindliche Integration und Bediengrenzen

### 8.1 Snapshot und Modellwechsel

Core-Vertrag A-C-00098–00103 ist Voraussetzung für A-G-00061–00063
und A-G-00067. Bei erstmaliger bewusster Modellwahl einmal im Worker
`readModelInspection` aufrufen. Solange derselbe Snapshot geöffnet ist,
nutzen sämtliche Baum-/Werte-/Meshabfragen ausschließlich diesen Bestand.
Dateiwatcher dürfen ihn weder ersetzen noch seine Auswahl zurücksetzen.
Die bestehende Ordnerliste darf Änderungen melden; ein automatisches
Wiederöffnen des ausgewählten Inspektionsmodells ist unterbunden.

Bewusster Wechsel auf ein anderes Modell öffnet dessen eigenen Stand.
Eine weitergehende Reload-Schaltfläche und Meshänderung sind nicht Teil
von G-0.8. Ausstehende Worker-Ergebnisse gehören zur jeweiligen Modellwahl;
veraltete Ergebnisse dürfen nicht eine neuere Auswahl überschreiben.
Core-Snapshot und Simulationsdaten sind getrennt: laufende Simulationen
werden nicht gestoppt oder mit Inspektionsdaten überschrieben.

### 8.2 Meshwahl und Auswahlbindung

Die separate Meshwahl dieses Abschnitts beschreibt den bisherigen Stand.
Sie wird durch A-G-00071 ersetzt; die ursprüngliche Festlegung bleibt
zur Nachverfolgbarkeit erhalten.

Über dem Darstellungsfenster bietet eine beschriftete Auswahl
**Mesh-Darstellung** sämtliche von `getAvailableMeshTypes` gelieferten
Typen an. Standard ist `darstellung`, sonst der erste verfügbare Typ.
Baumtyp und Mesh-Typ bleiben unabhängig. Punktdaten werden als Punkte
gerendert, keine erfundenen Flächen. Darstellungsmaterial `unsichtbar`
bleibt in der normalen GUI-Geometrie unsichtbar; entsprechende physische
Geometrie kann über deren Mesh-Art inspiziert werden.

Ausgewählte Gruppen/Fachknoten bilden die Vereinigung ihrer und ihrer
Nachkommen-Element-IDs. Keine ID-Zuordnung aus Anzeigetexten. Sind die
selektierten Elemente im aktuellen Mesh-Typ nicht geometrisch enthalten,
erscheint ein Hinweis; kein Wechsel auf eine andere Geometrie.
Kontextmaterial verwendet Opazität 0,15, ohne Geometrieänderung.
Reiterwechsel im Objektbaum setzt dessen Knotenauswahl und Wertetabelle
zurück, zeigt zunächst das gesamte Modell und ändert weder Kamera noch
Simulation. Rückkehr zu Hauptreitern erhält deren bisherigen Bedienzustand.

### 8.3 Tabellen und Abgrenzung

Ohne Baumknotenauswahl zeigt die Tabelle „Kein Objekt ausgewählt“.
Vektoren/Matrizen/Listen/Objekte werden ohne Werteverlust lesbar formatiert;
große Werte dürfen als expandierbare Zellansicht dargestellt werden.
Fehlend/nicht anwendbar/fehlerhaft werden nicht als null formatiert.
Meshpunkte und Dreiecke werden nicht zusätzlich vollständig in
Eigenschaftszeilen dupliziert; Meshreferenz, Typ und Anzahlen bleiben
sichtbar, tatsächliche Geometrie wird über die Mesh-Abfrage geliefert.

Die Simulationssteuerung des bisherigen rechten Fensters ist in
Bootsauswahl nicht mehr sichtbar; sie bleibt in den anderen Ansichten
verfügbar. Wiedergabe-/Kraftauswertung dürfen dort ihre bisherige Funktion
behalten, steuern jedoch nicht Baum oder Tabelle der Bootsauswahl.
Reparaturen, Ummodellierung, Speichern, Mesh-Updates, Windphysik und
neue Simulationseingaben sind ausdrücklich nicht Teil dieses Umfangs.

### 8.4 Abnahmestand

Alle Anforderungen sind fachlich spezifiziert und technisch umgesetzt.
Nachweise und bekannter Grafik-Baselinefehler stehen im maßgeblichen
Umsetzungsplan, Abschnitt 6. **Persönliche fachliche Abnahme offen.**
Die farbigen Hauptreiter waren Ausgangsstand und werden nicht als alleiniger
Nachweis für die C-/G-0.8-Objektinspektion verwendet.

### 8.5 Wiederverwendung statt paralleler Anbindungen

Die bestehenden Katalog-/Anzeigepfade in `model_catalog.cpp` und der
Simulationsimport in `core_adapter.cpp` sind bei der Integration ausdrücklich
mit zu berücksichtigen. Neue Inspektionsdaten werden in der vorhandenen
Core-Anbindung ergänzt; keine unabhängige zweite Modellverwaltung oder
Geometriekonvertierung für die Bootsauswahl. Gemeinsam benötigte
Element-, Material- und Geometriedaten nutzen gemeinsame Konvertierungen.
Kompatible Core-Altaufrufe bleiben nach A-C-00104 zulässig, weil sie
dieselbe Pipeline verwenden; sie sind kein zweiter fachlicher Importweg.

Simulation und Inspektion behalten dennoch getrennten Besitz und ihre
bisher festgelegten Einlesezeitpunkte. Eine Simulation wird weder aus
Inspektions-JSON rekonstruiert noch ungefragt an einen festen Snapshot
gebunden. Die technische Zusammenführung allein verändert weder
Bedienung noch Darstellung, Diagnoseanzeige, Modellwahl oder Simulation.
Die ausdrücklich spezifizierte neue Bootsauswahl bleibt davon unberührt.

**Abnahme A-G-00069:** Bestehende Anzeige-/Simulationsfälle vor und nach
der Migration mit identischen Eingaben vergleichen; Material, Geometrie,
Gruppen, Diagnosen und Simulationsergebnisse bleiben gleich. Aufrufpfade
bis zur gemeinsamen Core-Pipeline und wiederverwendete GUI-Konvertierungen
nachweisen. Sicht-/Verhaltenstests von den absichtlichen
Bootsauswahl-Layoutänderungen trennen.

## 9. Einheitlicher Definitionspfad: ID und Rolle getrennt

**Final spezifiziert und Umsetzung beauftragt am 02.11.2026.**
Fachliche Grundlage ist Alternative 2 des Nutzerauftrags: eine gemeinsame
OBJ-Datei mit benannten Quellnetzen und eine JSON mit Objektidentitäten,
Eigenschaften und Verwendungsregeln. Der genaue Dateivertrag steht
ausschließlich in der [Core-Modellbeschreibung](../../CORE/Spezifikationen/Spezifikation_Modellbeschreibung.md).
Keine GUI-eigenen Formatregeln oder zusätzlichen Importer.

### A-G-00070: Identität, Rolle und Herkunft explizit anzeigen

ID ist die stabile Identität, Anzeigename ist frei wählbar, hydrodynamische
Rolle ist eine ausdrückliche Facheigenschaft. Die GUI leitet Rollen,
Meshzuordnungen und Berechnungsverfahren niemals aus ID-/Anzeigetexten ab.
Alle Angaben stammen aus dem bestehenden Core-Snapshot.
Alte Modellformate bleiben über denselben Adapter verwendbar.

**Abnahme:** Ein Teil mit neutraler ID und Rolle Ruder sowie zwei Teile mit
derselben Rolle anzeigen; ID, Rolle und Geometrie bleiben unterscheidbar.
Normales Laden, Modellwahl und Simulation funktionieren für Alt- und Neuformat.

### A-G-00071: Quelldaten und Ableitungen getrennt auswählen

Die separate Auswahl „Mesh-Darstellung“ entfällt. Im rechten Objektbaum
stehen Geometrien unter eindeutig benannten Bereichen „Quelldaten“ und
„Abgeleitete Daten“. Quelle umfasst tatsächlich gespeicherte Netze; ein
unverändert wiederverwendetes Netz wird nicht wegen anderer physikalischer
Rollen als neue Berechnung ausgegeben. Reale Quellvarianten (etwa offene
Schale und geschlossenes Verdrängungsnetz) bleiben ausdrücklich sichtbar.
Geometrische Ableitungen benennen Quelle und Herleitung.
Ausgewählte Geometrieknoten steuern die Darstellung; abgeleitete Daten
können über Kontrollkästchen zusätzlich eingeblendet werden. Mehrfach-
und Gruppenauswahl bleibt möglich. Andere Teile bleiben als Glaskontext
erkennbar. Quelldarstellung und Ableitungsüberlagerung sind durch
Baumkennzeichnung und unterschiedliche Farben eindeutig unterscheidbar.
Fehlende Daten erzeugen einen sichtbaren Hinweis, keinen stillen Ersatz.
Die Bewegungseigenschaften bleiben am Objekt sichtbar; ein eigener
Bewegungsreiter ist für diese Oberfläche nicht nötig. Achs-/Extremvorschauen
werden mit diesem Definitionsauftrag noch nicht eingeführt.

**Abnahme:** Geometrien über den rechten Baum auswählen und überlagern;
keine separate Meshcombo vorhanden. Identische Quelle wird für mehrere
Rollen nicht mehrfach als unabhängige Geometrie gepflegt. Ohne Auswahl
normale Modellansicht; Auswahl- und Überlagerungswechsel erhalten Kamera
und laufende Simulation.

### A-G-00072: Gemeinsame Schnittstelle, kein Editor vorweggenommen

Snapshot, bestehende Modellanzeige, Geometriekonvertierung und
Simulationsanbindung nutzen weiterhin dieselbe Core-Pipeline. Die GUI
speichert keine eigene semantische Kopie des Modells. Herkunft/Zuordnungen
werden vom Core geliefert, nicht durch Lesen der JSON oder OBJ ermittelt.
Dieser Schritt bereitet spätere Bearbeitung vor; Quelländerungen,
automatische Neuberechnung, Speichern und Reparieren sind kein Teilauftrag.
Fester eingelesener Stand und davon getrennter Simulationslebenszyklus
bleiben erhalten.

**Abnahme:** Alt-/Neuformat durch bestehende Adapter laden; geteilte
Geometriekonvertierung und Core-Schnittstellen im Aufrufpfad nachweisen.
Dateiänderungen ersetzen keinen geöffneten Snapshot.

### A-G-00073: Technische Regression und Nachweis

Die neuen Definitionsmodelle in Katalog-/Render-/Simulationsintegration
prüfen, insbesondere explizite Rollen bei nichtsprechenden IDs und
mehrere Verwendungen eines Quellnetzes. Vorhandene Core-/GUI-Tests erhalten.
Layout und Auswahl bei 100/150/200 % prüfen. Bekannte Baselinefehler von
neuen Regressionen trennen; persönliche Abnahme nicht als erfolgt ausgeben.

## 10. Bestätigte Darstellungsvorschau vom 02.11.2026

### A-G-00074: Abgerundeter Inspektionsbereich mit Modellraster

Die vom Nutzer bestätigte Vorschau ersetzt in Bootsauswahl die bisherige
Wasser-/Himmelsumgebung: keine Wasser- oder Tiefenfläche, neutraler dunkler
Bordeauxhintergrund. Der Darstellungsbereich erhält wie die Menüfenster
abgerundete Ecken und einen dezenten Rahmen; auch der Renderinhalt wird
an den Ecken beschnitten.

Das Hauptobjektkoordinatensystem steht am tatsächlichen Modellursprung
(nicht automatisch am Schwerpunkt), X rot, Y grün, Z blau. Ein dezentes
dreidimensionales Linienraster mit gleicher Schrittweite in allen
Raumrichtungen bildet offene Referenzflächen ohne gefüllte Flächen.
Raster und Achsen sind modellfest und werden mit derselben Kamera wie
das Modell dargestellt. Andere Hauptansichten und Physik bleiben unverändert.

**Abnahme:** Sichtbare Modellachsen am Ursprung und Raster mit räumlicher
Ausdehnung, keine Wasserfläche, tatsächlich beschnittene Ecken nach
Größenänderung bei 100/150/200 %; Kamera-/Workspace-Regressionen erhalten.
Diese Freigabe betrifft die Darstellung, nicht die separat besprochene
Ein-Quellmesh-Migration oder Zusammenführung der fachlichen Baumreiter.

## 11. Finalisiert 02.11.2026: objektzentrierte Bootsauswahl

**Vorrang für die neue Umsetzung:** A-G-00075 ersetzt die fachliche
Baumreiterbedienung aus A-G-00062 sowie die globalen Geometriebereiche aus
A-G-00071 durch einen gemeinsamen Baum mit Geometrien unter dem Element.
A-G-00076 erweitert A-G-00063 um die Bereichsspalte und vollständige
bereichsübergreifende Auswahl; A-G-00077 hebt die bisherige Zurückstellung
der Bewegungsvorschauen auf. A-G-00078 präzisiert A-G-00074 durch feste
Meterteilung und Linienstile. Frühere Festlegungen bleiben als historische
Quelle erhalten, gelten aber nicht als parallele Bedienalternative.

**Status: finalisiert und Umsetzung ausdrücklich beauftragt am 02.11.2026.**
Erfasst die anschließend besprochenen offenen Punkte einschließlich
fester Rasterteilung. Anforderungen A-G-00075 bis A-G-00078;
Umsetzungsstand und tatsächliche Nachweise im bestehenden Versionsplan.
Zuordnung der Entwurfspunkte:
[GUI_Bootsauswahl.json](Entwurfspunkte/GUI_Bootsauswahl.json).
Die früheren Anforderungen und technischen Nachweise bleiben historisch
nachvollziehbar; sie belegen diese Erweiterung nicht.

### A-G-00075: Ein Element, eine Quelle, ein Objektbaum

Rechts gibt es einen gemeinsamen Objektbaum, keine Reiter „Modell“,
„GUI“, „Hydrostatik“ oder „Hydrodynamik“. Modell und Baugruppen enthalten
jedes Element genau einmal. Darunter sind sein einziges Quellmesh und
die tatsächlich verfügbaren Ableitungen eindeutig getrennt erreichbar:

```text
Modell
  Baugruppe
    Element
      Quellmesh
      Abgeleitete Daten
        vorhandene Punkte / Berechnungsgeometrien
```

Keine globalen zweiten Elementlisten pro Quelle oder physikalischer
Verwendung. Mehrfachverwendung desselben Netzes steht als Information am
Element, nicht als Kopie. Auswahl und Kontrollkästchen steuern weiterhin
Geometrie und Überlagerungen; Ableitungen sind farblich und textlich
von der Quelle unterscheidbar. Nicht ausgewählte Teile bleiben Kontext.
Kamera und laufende Simulation ändern sich durch Auswahl nicht.

Die GUI verwendet ausschließlich Core-Identitäten und Herkunftsdaten.
Der Ein-Quellmesh-Vertrag wird nicht durch bloßes Verstecken mehrerer
Quellen vorgetäuscht: noch nicht bereinigte Bestandsmodelle sind mit
ihren Konflikten erkennbar, statt als zielkonform ausgegeben zu werden.
Keine eigenen JSON-/OBJ-Importer oder neue Meshvereinfachung.
Der verbindliche Geometrievertrag steht in der
[Core-Modellbeschreibung](../../CORE/Spezifikationen/Spezifikation_Modellbeschreibung.md).

Nach der Nutzerentscheidung zur ausreichend genauen Erhaltung der bisherigen
Physik dürfen auch Darstellung, Masseschale und Auftriebsvolumen ausdrücklich
definierte Ableitungen der einen Quelle sein. Die GUI listet solche
Teilnetze unter „Abgeleitete Daten“, zeigt die konkrete Core-Definition
im Tooltip und die Parameter in der gemeinsamen Eigenschaftentabelle.
Ein solcher Teilkörper ist kein zusätzliches editierbares Quellmesh.
Die GUI interpretiert oder erzeugt die Schnitte/Flächenauswahlen nicht selbst.

### A-G-00076: Sämtliche Objektinformationen in einer Tabelle

Die untere Tabelle enthält die vorhandenen Informationen der Auswahl aus
allen Fachbereichen. Spalten: **Objekt/Pfad, Bereich, Eigenschaft, Wert,
Einheit, Status**. „Bereich“ kennzeichnet Identität/Modell, Geometrie,
Darstellung, Masse/Material, Hydrostatik, Hydrodynamik oder Bewegung
anhand der Core-Zuordnung, nicht anhand geratener Anzeigenamen.
Alle Spaltenbreiten bleiben manuell einstellbar.

Schwerpunkt und Trägheit werden ebenso wie die vorhandenen
Stützstelleninformationen angezeigt, nicht neu berechnet. Vektoren,
Matrizen und Listen bleiben vollständig zugänglich; keine nur optisch
abgeschnittenen Ersatzwerte. Herkunft, Quelle, Verwendung und Herleitung
bleiben lesbar. Nicht vorhanden, nicht anwendbar und fehlerhaft werden
unterschieden; keine erfundenen Ersatzwerte.

Einzel-, Gruppen- und Mehrfachauswahl einschließlich Nachkommen gelten
bereichsübergreifend, mit eindeutigem Objektbezug, ohne doppelte logische
Eigenschaften bei überlappender Auswahl und ohne automatische Summen.
Eine leere Auswahl entfernt alte Auswahlwerte.

### A-G-00077: Bewegung direkt am Element sichtbar machen

Kein eigener Bewegungsreiter. Bei vorhandener Bewegungsdefinition zeigt
die Tabelle die tatsächlichen Parameter. Eine eindeutig als Vorschau
gekennzeichnete, am Element zugängliche Einblendung zeigt Dreh-/Kippachse,
Normal-/Referenzstellung und die vorhandenen Extremstellungen.
Achse und Geometrie verwenden die vom Core definierten Bezugssysteme
und Transformationen; keine Verschiebung auf den Schwerpunkt.

Normalstellung und Extreme sind farblich bzw. textlich unterscheidbar.
Grenzen werden nicht geschätzt; bei fehlender Grenze wird genau diese
Vorschau als nicht verfügbar gekennzeichnet. Die Vorschau verändert
weder Quelldaten, Auswahlidentität noch Simulationszustand. Allgemeiner
Mesh-/Definitionseditor, Reparatur und Speichern bleiben spätere Arbeit.

### A-G-00078: Festes metrisches Koordinatenraster

Das räumliche Raster der Bootsauswahl verwendet auf allen Referenzflächen
und in allen drei Richtungen **0,5 m Rasterabstand**:

- Linien bei ganzzahligen Meterkoordinaten, also `n × 1 m` für positive
  und negative ganze `n` einschließlich 0: **durchgezogen (solid)**.
- Dazwischen bei `n × 1 m + 0,5 m`: **gepunktet (dotted)**,
  nicht gestrichelt.
- Jede Position hat genau einen Linienstil; keine doppelt überlagerten
  Solid-/Punktlinien an vollen Metern. Achsenfarben X rot, Y grün, Z blau
  bleiben davon unberührt.

Die Teilung bezieht sich auf Modellkoordinaten in Metern, unabhängig von
Kamera, Zoom, Fenstergröße oder DPI. Kein automatischer Wechsel auf
andere Weltabstände wie im derzeitigen adaptiven Raster. Punktgröße und
Linienstärke sollen lesbar bleiben; die geometrische Teilung ändert
sich dabei nicht. Dies ist ein Darstellungsraster, kein Berechnungsnetz
und keine Änderung der Stützstellen.

Ursprung, offene Linienflächen, keine Wasserfläche sowie abgerundeter
Viewport bleiben wie A-G-00074. Erweiterungen für sehr große Modelle
rechtfertigen kein stilles Weglassen oder Ändern der zugesagten Teilung.

### Prüffälle und Abhängigkeiten

| Bereich | Prüfbarkeit |
|---|---|
| Einzelquelle | Nach Core-Migration jedes geometrische Element genau einmal, genau eine Quelle, alle Verwendungen korrekt; Altbestandskonflikte sichtbar. |
| Gesamtinformationen | Bisherige Fachinformationen einschließlich Bewegung gegen Core-Inventar abgleichen; Mehrfach-/Gruppenauswahl ohne Doppelwerte. |
| Bewegung | Achse, Normalstellung und tatsächliche Grenzen an Referenzelementen mit Core-Transformationen vergleichen; fehlende Grenzen ausdrücklich testen. |
| Raster | Linien z. B. bei −1, 0, 1 m durchgezogen, bei −0,5 und 0,5 m gepunktet; gleiche Teilung in X/Y/Z und auf allen Referenzflächen; Zoom/Pan und 100/150/200 % ändern keine Meterpositionen. |
| Bestand | Snapshot bleibt unveränderlich; Kamera, Simulation und andere Hauptansichten behalten ihr Verhalten; abgerundete Ecken und fehlendes Wasser bleiben erhalten. |

Core liefert den gemeinsamen Definitions- und Datenvertrag gemäß
[Inspektionsspezifikation](../../CORE/Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md).
GUI liefert Baum, Tabelle, Überlagerungen und Raster. Die Rasterumstellung
ist unabhängig von der Modellmigration möglich. Die vollständige
Ein-Quellmesh-Abnahme hängt dagegen von deren erfolgreicher fachlicher
Prüfung ab. Abweichungen und ausstehende persönliche Sichtabnahme sind
gesondert auszuweisen; keine geplanten Tests als bestanden darstellen.
