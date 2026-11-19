# Spezifikation GUI: Kraft- und Momentendarstellung

**Status: FINAL – freigegeben am 26.10.2026, in G-0.2 umgesetzt, Abnahme offen (siehe Umsetzungsplan G-0.2).**
**Anforderungen:** A-G-00009 bis A-G-00018; Nachträge A-G-00023, A-G-00024, A-G-00026
**Umsetzungsplan:** [Umsetzungsplan G-0.2](../Umsetzungsplaene/Umsetzungsplan_G-0.2.md)
**Entwurfspunkte:** [Kraft- und Momentendarstellung](Entwurfspunkte/GUI_Kraft_und_Momentendarstellung.json) (IDEE-G-00008 bis IDEE-G-00014)

**Finaler Nachtrag 17.11.2026:** sailforce fuer G-0.25, Abschnitt 3,
A-G-00167 bis A-G-00172. Der historische Bestand bleibt erhalten.
Einziger neuer Versionsplan:
[G-0.25](../Umsetzungsplaene/Umsetzungsplan_G-0.25.md).
SDP-Darstellung A-G-00166 steht ausschliesslich in der
[Druckpunkt-Spezifikation](Spezifikation_GUI_Lateraldruckpunkt.md#6-g-025-aerodynamische-segeldruckpunkte).

## 1. Zweck und Abgrenzung

Die GUI zeigt, welche Kräfte und Momente auf einen Körper wirken, wo sie
angreifen, woher Momente stammen und wie die Einzelbeiträge die Bilanz
bilden: Kraftpfeile, Momentenbögen, Wertefenster und die Verknüpfung von
Tabelle und 3D-Ansicht.

**Aufteilung:** Die physikalischen Daten (Beiträge, Momente um S, Summen,
Datenzustände, Vollständigkeit, Gleichgewichtsangaben) liefert der Core
gemäß „Spezifikation Kraft- und Momentenbilanz im Ergebnisvertrag“ im
Core-Projekt (A-C-00034 bis A-C-00041, Vertragsversion 2). Diese
Spezifikation beschreibt nur Darstellung, Bedienung und Formatierung. Die GUI
rechnet keine Physik; rein geometrische Hilfskonstruktionen aus gelieferten
Werten (Hebelarm) sind zulässig.

**Stufung (IDEE-G-00008):** Teile mit **[V1]** sind mit dem heutigen
Ergebnisvertrag umsetzbar und werden zuerst gebaut. Teile mit **[V2]**
setzen Vertragsversion 2 des Cores (C-0.2) voraus. Die GUI liest beide
Versionen.

**Bezug zu G-0.1:** A-G-00009 bis A-G-00018 präzisieren und erweitern die
Kraftanzeige aus A-G-00007 ([GUI-Erstausbau](Spezifikation_GUI_Erstausbau.md)).
A-G-00007 bleibt gültig; dessen einfache Längenskalierung relativ zum
größten Betrag wird durch A-G-00011 abgelöst.

Herkunft: Altprojekt-Spezifikation „V0.11“ (SegelPhysik3, Abschnitte 4–7),
`BeispielSpecPfeile.md`, `Mockup_Wertefenster.html`. Altkennungen sind nur
Herkunftsverweise.

**Nicht Teil (IDEE-G-00011, zurückgestellt):** äquivalente Wirkungslinie
mit Restmoment; erst wenn Beiträge ohne realen Angriffspunkt auftreten.

## 2. Anforderungen

| ID | Kurztitel | Stufe | Version | Status |
|---|---|---|---|---|
| A-G-00009 | Kraftpfeile am echten Angriffspunkt | V1 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00010 | Momentenbögen | V1 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00011 | Skalierung | V1 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00012 | Feste Bildschirmgrößen | V1 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00013 | Farben und Linienarten | V1/V2 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00014 | Sichtbarkeit und Auswahl | V1 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00015 | Hebelarm-Hilfsgeometrie | V1 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00016 | Wertefenster | V1/V2 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00017 | Datenzustände und Zahlenformat | V1/V2 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00018 | Einstellungen, Aktualisierung und Rückblick | V1 | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00023 | Modellfehler im Wertefenster | V2 | G-0.2 (Nachtrag) | umgesetzt, Abnahme offen |
| A-G-00024 | Anzeige vollgelaufener Elemente | V2 | G-0.2 (Nachtrag) | umgesetzt, Abnahme offen |
| A-G-00026 | Vektorauswahl in Rubriken | V2 | G-0.2 (Nachtrag) | umgesetzt, Abnahme offen |

### A-G-00009 – Kraftpfeile am echten Angriffspunkt [V1]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00009, -00010

Jeder Kraftbeitrag wird als Pfeil vom gelieferten Angriffspunkt in
Kraftrichtung gezeichnet (Punkt rotiert und verschoben, Vektor nur rotiert).
Ein Nullvektor erhält keinen Pfeil, bleibt aber im Wertefenster. Ein Beitrag
ohne Angriffspunkt (`null`) wird nicht an einer erfundenen Stelle gezeichnet.

**Abnahme:** Tests prüfen Pfeilursprung und -richtung nach
Pose-Transformation; ein Nullvektor erzeugt keinen Pfeil.

### A-G-00010 – Momentenbögen [V1]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00011, -00044

Momente werden als Bogen über 240° um den gelieferten Momentenbezug
gezeichnet, in der Ebene senkrecht zum Momentenvektor, Drehsinn nach der
Rechte-Hand-Regel mit Pfeilspitze am Bogenende. Der Betrag bestimmt den
Radius. Die Bogenbasis bleibt bei kleinen Richtungsänderungen stabil. Ein
reines Moment erzeugt keinen Kraftpfeil.

**Abnahme:** Tests prüfen Drehsinn, Ebene und Stabilität der Basis bei
kleinen Richtungsänderungen.

### A-G-00011 – Skalierung [V1]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00013 bis -00016; IDEE-G-00009

Je Größenart (Kraft, Moment) gibt es eine Referenz R und einen Modus:

- logarithmisch (Standard): `u = log1p(m / m0) / log1p(R / m0)`, `m0 = R / 100`,
- linear: `u = m / R`.

`u` wird auf [0, 1] begrenzt; Werte über R werden als „über Skala“, sehr
kleine Werte als „Kleinwert“ markiert. Start: R_F = max(m·g, 1 N),
R_M aus dem ersten gültigen Zustand. Während eines Laufs bleibt R
eingefroren; „Neu einpassen“ setzt R auf das 1,1-Fache des aktuellen
Maximums. Grafische Schwellen sind keine physikalischen Toleranzen.

**Abnahme:** `u(0) = 0`, `u(R) = 1`, Monotonie, Markierungen in beiden
Modi; ungültige Referenzen werden abgelehnt.

### A-G-00012 – Feste Bildschirmgrößen [V1]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00017, -00032; IDEE-G-00010, -00014

Pfeilkopf 10 px, Strich 3 px, Kraftpfeil höchstens 180 px, Momentradius
höchstens 48 px, in logischen Pixeln, unabhängig von Zoom und DPI.

**Abnahme:** Prüfung bei 100 %, 150 % und 200 % DPI und mehreren
Zoomstufen; Abweichung höchstens 1 logisches Pixel in nicht verkürzter Lage.

### A-G-00013 – Farben und Linienarten [V1, Gruppen V2]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00018, -00019; IDEE-G-00010

| Beitrag | Darstellung |
|---|---|
| Gewicht | blau `#2166ac`, durchgezogen |
| Auftrieb | blau `#2166ac`, gestrichelt |
| ΣF, ΣM | hellgrau `#dddddd` |
| Test-/Fremdkraft | rot |
| nicht zugeordnet | magenta |
| spätere weitere Quellen | eigene Farbe je Gruppe |

Bei V1 erfolgt die Zuordnung über die ID-Endung, bei V2 über `quelle` und
`gruppe` aus dem Core. Eine Legende erklärt die Zuordnung.

**Abnahme:** Sichtprüfung und Test der Zuordnung für V1- und V2-Daten.

### A-G-00014 – Sichtbarkeit und Auswahl [V1]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00020 bis -00022

Modus „räumlich“ (vom Körper verdeckt) oder „durch Körper sichtbar“.
Gruppen und Summen sind ein- und ausblendbar (Gliederung siehe A-G-00026);
das ändert nur die Grafik.
Ein Klick auf Pfeil oder Bogen wählt den Beitrag aus (Trefferzone 6 px); bei
mehreren Treffern ist die Wahl eindeutig möglich. Die Auswahl wird in 3D
hervorgehoben und im Wertefenster markiert.

**Abnahme:** Auswahl per Klick inklusive Mehrfachtreffer; Ausblenden
ändert keine Werte.

### A-G-00015 – Hebelarm-Hilfsgeometrie [V1]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00012

Für einen gewählten Kraftbeitrag kann der senkrechte Hebelarm zum
Momentenbezug eingeblendet werden: `r⊥ = r − (r·F)/(F·F) · F`, `r = A − P`.
Bei `|F| = 0` entfällt die Hilfslinie.

**Abnahme:** `r⊥ · F = 0` im Test; keine Hilfslinie bei F = 0.

### A-G-00016 – Wertefenster [V1, Bilanzkarten V2]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00023 bis -00027; IDEE-G-00012

Ein andockbares Fenster (QDockWidget mit QTreeView) wie im Mockup:

- Bilanzkarten: ΣF, ΣM mit Bezug, Bilanzstatus und Gleichgewichtsangaben
  [V2]; bei V1 Solverstatus und „Gleichgewicht nicht bewertet“,
- Hierarchie Körper → Gruppe → Beitrag mit Betrag und Komponenten,
- Detailbereich: Angriffs- bzw. Bezugspunkt, Frame, Moment um S, Zustand,
  Quelle.

Tabelle und 3D sind synchronisiert und zeigen denselben Zustand (Zeit und
Schritt sichtbar). Werte lassen sich in die Zwischenablage kopieren; einen
Dateiexport gibt es nicht.

**Abnahme:** Auswahl in Tabelle oder 3D markiert in beiden; Kopieren
liefert die angezeigten Werte.

### A-G-00017 – Datenzustände und Zahlenformat [V1, Zustände V2]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00028, -00039 bis -00041

Zahlen stets mit eigener Einheit (auch jede Vektor- und Punktkomponente)
und fester Auflösung, nie in Exponentenschreibweise:
Kräfte auf 1 N, Momente auf 1 N·m, Längen und Koordinaten auf 0,01 m,
Skalenanteile in Prozent ohne Nachkommastelle, Zeit auf 0,01 s. Auf null
gerundete Werte erscheinen ohne Vorzeichen. Toleranzen in der Bilanzkarte
behalten 4 signifikante Stellen (geändert auf Nutzerwunsch 26.10.2026).
Null, deaktiviert, nicht
berechnet, ungültig und veraltet erscheinen als Text, nie als Leerzelle oder
0. „Gesamtbilanz“ heißt die Summe nur bei Bilanzstatus `vollstaendig`, sonst
„Verfügbare Teilsumme – unvollständig“ mit Liste der Lücken.

**Abnahme:** Tests mit V2-Daten für jeden Zustand und jeden Bilanzstatus.

### A-G-00018 – Einstellungen, Aktualisierung und Rückblick [V1]

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** A-V0.11-00029, -00033, -00037, -00038; IDEE-G-00013, -00014

Skalenmodus, Referenzen, Sichtbarkeit und Fensterlayout werden gespeichert
(QSettings). Grafik und Tabelle aktualisieren sich nur bei neuen Zuständen
und höchstens mit der Bildrate der Live-Anzeige; Actors werden
wiederverwendet. Die Anzeige bremst den Core nicht (A-G-00005). Kamera- und
Stiländerungen lösen keine Core-Auswertung aus. Beim Rückblick zeigen Pfeile
und Wertefenster den gewählten gespeicherten Zustand.

**Abnahme:** Laufzeitvergleich der Live-Dynamik mit Anzeige ein/aus; die
Grenzwerte legt die Nutzerin nach der ersten Messung fest. Rückblick zeigt
die Werte des gewählten Zustands.

### A-G-00023 – Modellfehler im Wertefenster [V2]

**Version:** G-0.2 (Nachtrag auf Nutzerauftrag 27.10.2026) · **Status:** umgesetzt, Abnahme offen

Wird ein fehlerhaftes Modell aufgerufen, ist keine Kraftrechnung möglich.
Das Wertefenster zeigt dann statt der Kräfte die Fehler des Modells und wird
dazu eingeblendet, falls es ausgeblendet ist:

- Bilanzkarte „Modell fehlerhaft – keine Kraftrechnung möglich“ mit
  Fehleranzahl und Hinweis, dass das Modell nach Korrektur automatisch neu
  geladen wird,
- je Fehler eine Zeile „Fehler-ID · Kurztext“,
- im Detailbereich zum gewählten Fehler Fundstelle (relativ zum
  Modellordner), konkrete Meldung des Cores und Abhilfe.

Kurztext und Abhilfe stammen aus dem Fehlerkatalog des Cores
(`fehlerdokumentation_json`), Meldung und Fundstelle aus `modelldiagnose`.
Die GUI prüft das Modell nicht selbst. „Werte kopieren“ liefert die
Fehlerliste als Tabelle.

**Abnahme:** Aufruf eines abgelehnten Referenzmodells zeigt Fehler-ID,
Kurztext, Fundstelle und Abhilfe; kein Kraftpfeil, keine Bilanz.

### A-G-00024 – Anzeige vollgelaufener Elemente [V2]

**Version:** G-0.2 (Nachtrag auf Nutzerauftrag 27.10.2026) · **Status:** umgesetzt, Abnahme offen

Liefert der Core vollgelaufene Elemente (`geflutete_elemente`, A-C-00047),
zeigt die Bilanzkarte direkt unter dem Titel in Rot
„Vollgelaufen: <Element-IDs>“. Ohne Flutung entfällt die Zeile. Ältere
Aufzeichnungen ohne das Feld gelten als nicht geflutet.

**Abnahme:** Ein Lauf, bei dem ein Bordrand eintaucht, zeigt „Vollgelaufen:
Rumpf“; die Statik desselben Modells zeigt die Zeile nicht. Hinweis: Der
Optimist schwimmt seit der Anhebung seiner Geometrie (27.10.2026) ab
Initialpose aufrecht und läuft dort nicht mehr voll; für den Nachweis ist
eine Startlage mit eingetauchtem Bordrand nötig.

### A-G-00026 – Vektorauswahl in Rubriken [V2]

**Version:** G-0.2 (Nachtrag auf Nutzerauftrag 27.10.2026) · **Status:** umgesetzt, Abnahme offen · **Präzisiert:** A-G-00014 (Ein- und Ausblenden)

Die Auswahl der angezeigten Vektoren ist in zwei Rubriken gegliedert, oben
**Kräfte**, darunter **Momente**. Jede Rubrik bietet mit kurzen Texten:

| Zeile | gesamt | Teile |
|---|---|---|
| Gewicht | Gesamtgewicht | Gewicht je Element |
| Auftrieb | Gesamtauftrieb | Auftrieb je Element |
| Summe ΣF bzw. ΣM | Resultierende | – |

- Kräfte „gesamt“ liegen im betragsgewichteten Mittelpunkt der
  Teilkräfte (bei parallelen Kräften Schwer- bzw. Auftriebspunkt).
- Momente „Teile“ und „gesamt“ sind die Momente um S; die Teilmomente
  stammen aus `moment_s_nm` der Core-Beiträge.
- Die GUI bildet Gesamtwerte nur durch Aufsummieren gültiger Core-Beiträge
  gleichen Frames; sie rechnet keine Physik. Gesamtwerte erscheinen hellblau
  `#67a9cf` und etwas breiter, sind in 3D wählbar und im Detailbereich als
  „Anzeigewert der GUI“ gekennzeichnet. In der Tabelle stehen weiterhin nur
  die Core-Beiträge.
- Test-, Fremd- und nicht zugeordnete Beiträge bleiben immer sichtbar.
- Voreinstellung: Kräfte Teile und ΣF, Momente nur ΣM. Die Auswahl wird
  gespeichert (A-G-00018).

**Abnahme:** Test der Summen, Angriffspunkte und Teilmomente; Sichtprüfung:
Gesamtgewicht und Gesamtauftrieb liegen im statischen Gleichgewicht auf
einer Wirkungslinie; Ausblenden ändert keine Werte.

## 3. sailforce-Darstellung und Bedienung G-0.25

**Stand: 17.11.2026. FINAL auf ausdruecklichen Nutzerauftrag.**
Umsetzung, Tests, Integration und Nutzerabnahme offen. Keine
Implementierungs- oder Git-/Release-Aktion durch diesen Dokumentauftrag.
Herkunft und IDEE-zu-A-Zuordnung bleiben in der bestehenden
[Liste](Entwurfspunkte/GUI_Kraft_und_Momentendarstellung.json) erhalten.

Physik, Lasten, Summen, Punkte, Geometrie und Status liefert ausschliesslich
der [Core](../../CORE/Spezifikationen/Spezifikation_sailforce.md).
Die GUI liest keine aerodynamischen Modelldateien als Parallelpfad.
Keine eigene Wind-/Segelphysik, Druckpunktberechnung, Segelinterferenz,
automatische Schotfuehrung oder vollstaendige Wirkungsliniendarstellung.
Bestehende Windbedienung und Fremdkraft bleiben erhalten.
Der neue Plan umfasst auch A-G-00166 aus der Druckpunkt-Spezifikation.

| ID | Finale Anforderung | Herkunft |
|---|---|---|
| A-G-00167 | Einzel- und Summenkraefte/-momente | IDEE-G-00106 |
| A-G-00168 | Dezentes Streifengrenzlinienoverlay | IDEE-G-00107 |
| A-G-00169 | Aktivierung und quasistatische Segelverstellung | Finalisierungsentscheidung 17.11.2026 |
| A-G-00170 | Settings: Lufteigenschaften | Finalisierungsentscheidung 17.11.2026 |
| A-G-00171 | Asynchroner Abruf, Historie und Fehler | Bestaetigte Architektur und Finalisierung 17.11.2026 |
| A-G-00172 | Echtzeit und fluessige Darstellung | Bestaetigter Messrahmen 17.11.2026 |

### A-G-00167: Einzel- und Summenlasten

Streifenbeitraege, Summen je Segel und sailforce-Gesamtsumme sind getrennt
auswaehlbar; Einzelbeitraege und Summen koennen gleichzeitig sichtbar
sein. Kraft- und Momentensichtbarkeit bleiben getrennt. Bestehende
Farben, Skalen, Bildschirmgroessen und Wertefenster werden verwendet.
Vorhandene Voreinstellungen bleiben nach ihrer Bedeutung erhalten:
Kraefte Teile und Summe, Momente zunaechst Summe. Keine neue GUI-
Summationsregel fuer sailforce: ausschliesslich Core-Summen verwenden.
Die bestehende betragsgewichtete Angriffspunktbildung aus A-G-00026
gilt nicht als Ersatz fuer diese Core-Ergebnisse.

Alle angezeigten Momente beziehen sich auf den aktuellen Schwerpunkt S;
Boegen werden dort dargestellt. Streifenkraftpfeile liegen am gelieferten
Abfragepunkt als gekennzeichnetem Darstellungsbezug, nicht als behauptetem
realem Druckpunkt. Dies praezisiert fuer sailforce A-G-00009. Ein
Seitenkraft-SDP ist nicht der Ansatzpunkt der vollstaendigen Summenlast.
Fuer Summenkraftpfeile nur einen explizit gelieferten Core-Darstellungsbezug
nutzen; keinen Punkt aus den Einzelbeitraegen erfinden.
Reine Momente bleiben ohne Kraftpfeil darstellbar. Quellen und Rollen
unterscheiden, Summen nicht als zusaetzliche Lasten zaehlen. Tatsaechliche
Giermomentanteile um S nach Quellen sind im Wertefenster auswaehlbar,
ohne automatische Luv-/Leegierigkeitsdiagnose.
**Abnahme:** Auswahlmatrix einzeln/je Segel/gesamt, Kraefte/Momente,
Nullkraft und reines Moment, Pose und S-Bezug; Werte stimmen mit
dem Core-Snapshot ueberein. Sichtbarkeit aendert keine Physik.

### A-G-00168: Linienoverlay

Die vom Core gelieferten tatsaechlichen Streifen-Schnittgrenzen werden
als dezente Linien direkt auf dem vorhandenen Segelmesh dargestellt.
Kein Dreiecksdrahtgitter, keine zusaetzlichen gefuellten/eingefaerbten
Streifenflaechen und keine eigene Einteilung. Ein eigener
Anzeigeschalter ermoeglicht das Overlay unabhaengig von Kraftpfeilen;
bestehende Einstellungen bleiben erhalten, ohne Vorwert zunaechst aus.
Geometrie, aktuelle Segellage und Bootspose verwenden dieselbe Fassung
und Transformation, kein flackerndes koplanares Ueberlagern.
Unveraenderte Linien wiederverwenden; Aenderung von Modell oder
Geometriefassung invalidiert die Anzeige.
**Abnahme:** Dreieckiges/viereckiges Segel, beide Stellseiten und
Nullstellung, Modellwechsel, 100/150/200 % DPI, Overlay ein/aus;
keine Neuvernetzung je Bild und keine Rueckwirkung auf Lasten.

### A-G-00169: Aktivierung und Verstellung

Pro Segel eine fachlich von Anzeigeoptionen getrennte Aktivierung
anbieten. Ohne bestaetigte Auswahl sind Segel mit expliziter
Core-Modellzuordnung zunaechst aktiv; gespeicherte Auswahl hat Vorrang.
Ohne Modellzuordnung ist die Aktivierung mit erklaertem Grund nicht
verfuegbar. Defekte aktive Modelle blockieren den Lauf sichtbar.
Aktivierungsaenderungen gelten fuer den naechsten Lauf.
Fremdkraft nicht automatisch deaktivieren.

Grosssegel- und Fock-Stellwinkel sind in der Dynamik live quasistatisch
aenderbar. Bestehende Achsen-/Vorzeichenkonventionen des
[Segelkonfigurators](Spezifikation_GUI_Segelkonfigurator.md) verwenden.
Eingaben an den Core bestaetigen lassen; bis zur Uebernahme als
ausstehend kennzeichnen. Lasten und laufende Segeldarstellung erst
mit der bestaetigten Ergebnisfassung zusammen anzeigen, keine neue
Geometrie ueber alte Lasten legen. Keine Stellgeschwindigkeit aus
Mausbewegung ableiten. Im stationaeren Rechenfall gelten neue
Stellungen erst fuer eine neue Suche.
**Abnahme:** Ein-/Zweisegler, Voreinstellung, gespeicherte Wahl,
fehlendes/defektes Modell, beide Winkelvorzeichen, Pause,
ausstehende/abgelehnte Eingabe und Laufende vor Uebernahme.

### A-G-00170: Settings: Lufteigenschaften

Unter **Settings -> Lufteigenschaften** vor Laufstart einstellbar:
Luftdichte mit Vorgabe `1,225 kg/m^3` und kinematische Viskositaet mit
Vorgabe `1,5e-5 m^2/s`. Einheiten und genuegend Ziffern beziehungsweise
wissenschaftliche Schreibweise anzeigen; bestehende Lokalisierung
verwenden. Keine Umdeutung als dynamische Viskositaet.

Vorhandene Settings-Persistenz verwenden. Core validiert Endlichkeit,
Positivitaet und Bereiche aller aktiven Segelmodelle; die GUI zeigt
Ablehnungsgrund und gueltige Grenzen. Nicht still begrenzen oder
ungueltige Eingaben als neue Vorgaben speichern. Bestaetigte Luftwerte
sind im Lauf konstant; Bearbeitung waehrend eines Laufs sperren.
Wiedergabe zeigt die damaligen Laufwerte, nicht aktuelle Settings.
Unveraendertes Uebernehmen einer gerundeten Anzeige verliert keine
gespeicherte Praezision.
**Abnahme:** Defaults, Persistenz/Neustart, Wertebereiche, NaN/Inf,
Dezimalkomma/-punkt gemaess vorhandener Eingabe, unveraenderte
Uebernahme und getrennte historische Werte.

### A-G-00171: Asynchroner Abruf und Status

Der Core bleibt unabhaengiger Rechner; GUI-Anzeige/Abruf takten keine
Physikschritte. Vorhandenen Worker-/Snapshotpfad erweitern, nicht
Simulation auf dem UI-Thread ausfuehren. Je angezeigtem Stand stammen
Pose, Segellage, Lasten, S, SDP/LDP, Zeit und Konfiguration aus
demselben unveraenderlichen Ergebnis. Geometrie nur bei passender
Quellfassung wiederverwenden.

Langsameres Zeichnen darf Zwischenbilder auslassen, aber weder
Core-Schritte noch vorgeschriebene Aufzeichnung. Pause, Rueckblick,
Abbruch und Modellwechsel erhalten ihre bisherige Semantik; spaete
Antworten fremder/abgebrochener Laeufe werden nicht angezeigt.
Altdaten ohne sailforce bleiben lesbar und als nicht verfuegbar
erkennbar. Bereichs-/Datenfehler klar anzeigen; letzten gueltigen
Zustand nicht als neu erfolgreiches Ergebnis ausgeben.
**Abnahme:** Verzoegerter Abruf und UI-Stall bei weiterrechnendem
Core, konkurrierende Modell-/Laufwechsel, Liveaenderung, Fehler,
Pause/Wiedergabe und fehlende neue Felder.

### A-G-00172: Echtzeit und fluessige Darstellung

Auf vorhandener **Ryzen 5 8600G / Radeon 760M unter Windows**,
Release-Build, Technikprofil und reproduzierbarem Messfenster
**1280 x 720** pruefen. 420, Folkeboot und Optimist jeweils einzeln;
alle zugeordneten Segel aktiv, gemeinsamer SDP und Streifenoverlay an.
Vorhandene Technikprofil-Defaults fuer Kraefte/Momente verwenden und
den tatsaechlichen Featurestand mit dem Messlauf dokumentieren.

Je Modell nach Laden/Vorbereitung 60 s ungestoerte Dynamik mit
0,01 s Simulationsschritt bei Echtzeitfaktor 1 darstellen; Bildabstaende
ueber den gesamten 60-s-Live-Messzeitraum erfassen, nicht nur ueber
eine kurze, beschleunigt fertig gerechnete Sequenz. Kein vorab
offline erzeugter Verlauf als integrierter Live-Nachweis.
Die Core-Kette muss 60 s Physik in
hoechstens 60 s Wandzeit berechnen; gleichzeitig sind mindestens
95 % der Bildabstaende hoechstens 33,3 ms und kein Bildabstand
groesser als 100 ms. Tatsaechliche Praesentations-/Bildabstaende
messen, nicht nur Rechenzeit eines Renderfunktionsaufrufs.
Core-Zeit, Bildzeitmedian/p95/Maximum, Speicher, Start-/Ladezeit,
Modell-/Bibliotheksfassungen und vollstaendige Rechenkonfiguration
berichten. Laden/Vorbereitung separat; Abbrueche oder ungueltige
Physikzustaende bestehen den Lauf nicht. Externe Stoerungen
dokumentieren und Messung unter reproduzierbaren Bedingungen
wiederholen, keine langsamen Bilder still aus dem Datensatz entfernen.

Kein Leistungsgewinn durch ausgeschaltete Pflichtdarstellungen,
ausgelassene Physik oder unbemerkte Grafikreduktion. Ein Kontrolllauf
ohne Anzeige dient der Entkopplungspruefung. Dieses Ziel ist keine
Garantie fuer Optikprofil oder beliebige Modelle.
**Abnahme:** Alle drei integrierten Messlaeufe erfuellen die Grenzen.
Sichtabnahme und technische Leistung getrennt ausweisen; aktuell
noch kein sailforce-Laufzeitnachweis vorhanden.
