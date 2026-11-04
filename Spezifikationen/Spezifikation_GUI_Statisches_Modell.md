# Spezifikation GUI G-0.10: Statische Anzeige und Dreifachansicht

## Freigegebener Dynamik-Nachtrag 04.11.2026: laufende Eingaben

**A-G-00134:** Nutzerfreigabe 08:52 Uhr: Fremdkraft EIN/AUS,
XYZ-Komponenten, Ansatzpunkt und Core-Ruder-/Schwertstellungen bleiben
im dynamischen Lauf bedienbar. Numerische Eingaben erst nach bestaetigter
Zahleingabe senden. Aenderungen gelten ab dem naechsten noch nicht
berechneten Schritt, auch wenn gerade ein historischer Zustand
betrachtet wird. Die Historie wird nie nachgerechnet oder ueberschrieben.
Eingabefelder zeigen angeforderte Werte; Darstellung, Kraefte, Druckpunkte
und bewegte Meshes zeigen ausschliesslich den ausgewaehlten bestaetigten
Core-Zustand. Statische Rechnungen behalten ihre bisherige Eingabesperre.
Fehler und vor Laufende nicht mehr uebernommene Eingaben sichtbar melden.
Abnahme: echte Eingabe waehrend eines laufenden GUI-Versuchs, bestaetigte
Folgezustaende, veraenderte Elementtransformationen und unveraenderter
Rueckblick. Keine neuen Modell-/Meshdaten erfinden, vorhandene
stellungsabhaengige Aufbereitung verwenden. Kein Git-/Release-Auftrag.

**Status:** Durch Umsetzungsauftrag vom 03.11.2026 finalisiert und zur Umsetzung freigegeben.
**Nachtrag:** Abschnitt 6 durch Umsetzungsauftrag vom 03.11.2026,
13:42 Uhr finalisiert und zur Implementierung freigegeben.
**Stand:** 03.11.2026.
**Nachtrag Dynamik:** Abschnitt 12 erweitert ab dem ausdruecklichen
Umsetzungsauftrag vom 03.11.2026, 17:59 Uhr das gemeinsame Layout auf
das Dynamische Modell. Fruehere Abgrenzungen bleiben als Herkunft
lesbar; fuer den aktuellen Anzeigeumfang gilt dieser Nachtrag.
**Nachtrag Kraftmenue:** Abschnitt 9 wurde durch „bitte umsetzen“ am
03.11.2026, 16:15 Uhr finalisiert und zur Implementierung freigegeben.
Implementiert und technisch abgeschlossen laut Abschlussbericht vom
16:31 Uhr; finaler Wiederholungstest nach Render-/Tooltipbereinigung
bestanden. Persoenliche Nutzerabnahme offen, GUI uncommittet.
**Zielversion:** G-0.10.
**Ausgangsstand:** G-0.9, auf GitHub veroeffentlicht.

Diese Spezifikation enthaelt ausschliesslich die aktuell gesammelten
Aenderungen fuer G-0.10. Die bisherigen Anforderungen und ihre Kennungen
bleiben im unveraenderten
[G-0.9-Stand dieser Spezifikation](https://github.com/BergStefanBerlin/SegelPhysik-GUI/blob/G-0.9/Spezifikationen/Spezifikation_GUI_Statisches_Modell.md)
nachvollziehbar; sie werden hier nicht erneut als offene Arbeit aufgefuehrt.

Die Herkunft und IDEE-zu-A-Zuordnung stehen in der
[Entwurfspunkteliste](Entwurfspunkte/GUI_Statisches_Modell.json).
Anforderungen A-G-00093 bis A-G-00097 wurden oberhalb des bisherigen
Hoechstwerts A-G-00092 vergeben. Der zugehoerige
[Umsetzungsplan G-0.10](../Umsetzungsplaene/Umsetzungsplan_G-0.10.md)
enthaelt die freigegebenen Schritte und die tatsaechlichen Nachweise.

## 1. A-G-00093: Geltungsbereich und reduzierte Darstellung

Ausschliesslich der Reiter **Statisches Modell** wird geaendert.
Bootsauswahl und Dynamisches Modell bleiben unveraendert.

- Alle Momentenpfeile und Momentenboegen werden zunaechst ausgeblendet.
- Einstellmoeglichkeiten fuer die grafische Kraft- und Momentendarstellung
  entfallen im statischen Reiter, einschliesslich Gruppen-/Summenauswahl,
  Skalierung und Hebelarmanzeige.
- Als physikalische Punkt-/Vektorueberlagerungen bleiben ausschliesslich
  **LDP** und **SDP (Ersatz)** sichtbar: beschriftete Punkte, keine
  Kraftpfeile, Momentenpfeile oder Verbindungslinien.
- Die Punkte verwenden weiterhin die bestehenden Core-Positionen und
  Gueltigkeitsregeln der
  [Druckpunktspezifikation](Spezifikation_GUI_Lateraldruckpunkt.md#5-ldp-und-ersatz-sdp-als-punkte-02112026).
  Fehlende oder unbestimmbare Punkte nicht erfinden.

Mit Kraft-/Momenten-Settings ist gemaess Abstimmung ausschliesslich die
grafische Bedienung gemeint, nicht die Definition der Fremdkraft als
Solvereingabe. Fremdkraft und Angriffspunkt sowie Ruder-/Schwertstellungen
bleiben einstellbar. Berechnung und numerische Ergebnistabelle bleiben
erhalten.

## 2. A-G-00094: Anordnung und verbindliches Hochformat

In einer eigenen Spalte zwischen der grossen Bootsansicht und der rechten
Rechenbedienung stehen drei vertikal gestapelte Zusatzansichten.
Keine Ueberlagerung der Hauptansicht oder der Rechenbedienelemente.

Die nutzbare Bildhoehe der Zusatzspalte wird nach Abzug der
Beschriftungen/Rahmen und gleichmaessiger Zwischenabstaende in drei
gleiche Teile geteilt. Jeder Bildbereich belegt genau einen dieser Teile.

**Hoehe : Breite = 4 : 3, ausdruecklich Hochformat.**
Fuer die Hoehe `h` eines Bildbereichs betraegt seine Breite `3*h/4`.
Das Verhaeltnis gilt fuer den eigentlichen Bildbereich ohne
Beschriftung/Rahmen und bleibt bei Fenstergroessen- und DPI-Aenderungen
erhalten. Keine Streckung der Yacht und keine Umdeutung zu Querformat.

## 3. A-G-00095: Reihenfolge und Kameras

Alle drei Zusatzansichten verwenden orthographische Parallelprojektion,
keine perspektivische Verkuerzung.

| Position | Ansicht | Blickrichtung und Wasserbezug |
|---|---|---|
| oben | Seitenansicht von Backbord | Von der linken Bootsseite auf die Yacht; Kamera und Zielpunkt auf Welt-Z=0. Horizontaler Blick in der Wasserebene, nicht von erhoehter Position. |
| Mitte | Heckansicht | Vom Heck in Richtung Bug; Kamera und Zielpunkt auf Welt-Z=0. Horizontaler Blick in der Wasserebene. |
| unten | Draufsicht | Senkrecht von oben auf die Wasseroberflaeche, Blickrichtung Welt-minus-Z. |

Die Wasseroberflaeche bleibt Welt-Z=0 und in Seiten-/Heckansicht
bildschirmhorizontal. Die Welt wird nicht mit der Yacht gekippt.
Die Yacht behaelt ihre vollstaendige Core-Pose einschliesslich Kraengung,
Stampfen und Tiefenlage; keine kuenstliche Aufrichtung fuer die Kameras.
Insbesondere bleibt die Kraengung in der Heckansicht sichtbar.

Fuer Seiten-/Heckrichtung wird nur die horizontale Laengsrichtung der
Yacht verwendet, nicht ihre gekraengte Hochachse als Kamera-Oben.
Die Seitenansicht von Backbord ist ausdruecklich gewaehlt.
Die mit dem Umsetzungsauftrag uebernommene Draufsicht zeigt den Bug oben.

## 4. A-G-00096: Gemeinsamer Zustand und Projektgrenze

Hauptansicht und alle drei Zusatzansichten zeigen denselben ausgewaehlten
Core-Zustand, dieselbe Yachtpose und dieselben Druckpunkte. Die
Zusatzansichten loesen keine eigenen Simulationen oder Physikberechnungen
aus. LDP und Ersatz-SDP duerfen nicht aus unterschiedlichen Laeufen stammen.
Darstellungswechsel veraendern keine Kraft, Stellung oder Solverparameter.
Bestehender Reiterabbruch/-reset und Schutz vor verspaeteten Ergebnissen
gelten auch fuer alle drei Zusatzansichten.

**Projektzuordnung: GUI ja, Core nein.** Die vorhandenen Mesh-, Pose- und
Druckpunktvertraege reichen nach aktuellem Stand aus. Sollte die technische
Pruefung spaeter eine Vertragsluecke zeigen, wird diese vor einer
Core-Erweiterung ausdruecklich gemeldet.

## 5. A-G-00097: Abnahmekriterien und Freigabe

- Nur der statische Reiter wird reduziert; keine Kraft-/Momenten-Settings
  und keine entsprechenden Pfeile/Boegen/Hilfslinien bleiben dort sichtbar.
- Reihenfolge Seite, Heck, oben; drei gleich hohe Bildbereiche mit
  Hoehe/Breite 4/3, pruefbar bei 100/150/200 Prozent Skalierung und
  unterschiedlichen Fenstergroessen, Rundungstoleranz ein Geraetepixel.
- Wasserlinienblick hat in Seite/Heck exakt Kamera-Z und Ziel-Z gleich
  null; Draufsicht blickt senkrecht. Parallelprojektion in allen drei.
- Ein echter gekraengter/gestampfter Core-Zustand bleibt unveraendert
  und in allen Ansichten konsistent; Welt-/Wasserebene kippt nicht mit.
- LDP/Ersatz-SDP entsprechen denselben Core-Koordinaten und werden auch
  beim Reset in allen Ansichten gemeinsam entfernt.
- Bootsauswahl, Dynamik, Berechnung und gespeicherte Einstellungen
  ausserhalb des beauftragten statischen Anzeigeumfangs bleiben erhalten.

Die drei abgefragten Auslegungspunkte sind entschieden: eigene Spalte
zwischen Bootsansicht und Rechenbedienung, Seitenansicht von Backbord,
nur grafische Kraft-/Momenteneinstellungen entfernen.
Die Zielversion ist G-0.10. Der ausdrueckliche Auftrag vom 03.11.2026,
12:52 Uhr finalisiert diesen Umfang und gibt seine Umsetzung frei.
Ergebnisse und noch offene fachliche Nutzerabnahme stehen im Plan;
die Freigabe ist kein vorweggenommener Abnahmenachweis.

## 6. Vier benannte, bedienbare und tauschbare Ansichten

Dieser Nachtrag beschreibt ausschliesslich die anschliessend gesammelten
Fensterdefinitionen. Herkunft und Status der sechs neuen Punkte stehen
in der [Entwurfspunkteliste](Entwurfspunkte/GUI_Statisches_Modell.json).
Anforderungen A-G-00098 bis A-G-00103 wurden oberhalb des bisherigen
Hoechstwerts A-G-00097 vergeben; IDEE-Zuordnung bleibt erhalten.
Die bisherigen Anforderungen und Nachweise bleiben nachvollziehbar.
Dieser freigegebene Nachtrag ersetzt die feste Zuordnung von
Ansicht zu Fensterplatz und die bisher gesperrte Zusatzkamerabedienung.
Die Kamerabindung an Z=0 gilt nun fuer Defaultansichten; vertikales Pan
ist bei weiterhin horizontalem Blick zulaessig.

### 6.1 Ansichten und Ausgangsanordnung

Es gibt vier Fensterplaetze: einen grossen und rechts davon drei kleine
uebereinander. Die Ansichten besitzen eine vom Fensterplatz unabhaengige
Identitaet:

| Anforderung | Ausgangsplatz | Ansichtsname | Bedienung und Blickbindung |
|---|---|---|---|
| A-G-00098 | gross | Frei | Bisherige Funktionalitaet unveraendert, einschliesslich freier Kamerabedienung. |
| A-G-00099 | klein, oben | Seite | Backbordansicht; Zoomen und Verschieben wie bisher moeglich, Blick immer parallel zur Wasserebene. |
| A-G-00100 | klein, Mitte | Heck | Blick vom Heck zum Bug; Zoomen und Verschieben wie bisher moeglich, Blick immer parallel zur Wasserebene. |
| A-G-00101 | klein, unten | Draufsicht | Zoomen und Verschieben wie bisher moeglich, Blick immer orthogonal von oben auf die Wasserebene. |

Seite, Heck und Draufsicht behalten die orthographische Parallelprojektion.
Freie Rotation darf deren vorgeschriebene Blickrichtung nicht veraendern;
auch keine Kamerarollbewegung mit der Kraengung. Die Yacht behaelt ihre
vollstaendige Core-Pose. Zoom und Verschieben veraendern ausschliesslich
die Kamera, nicht Modell, Lasten, Solver oder Ergebnis.

### 6.2 Zoomen, Verschieben und Defaultansicht

Fuer Seite, Heck und Draufsicht verschiebt Ziehen mit gedrueckter
linker Maustaste ohne Zusatztaste das Bild. Mittlere Maustaste und
Umschalt+links bleiben ebenfalls zum Verschieben nutzbar.
Mausrad, rechte Maustaste und Strg+Umschalt+links dienen dem Zoom.
Keine Geste darf die gebundene Kamera aus ihrer Ansicht drehen.
Dieser gemeinsame Bedienvertrag gehoert zu A-G-00099 bis A-G-00101.

Seite und Heck behalten beim Verschieben einen horizontalen Blick:
Kamera und Ziel werden gemeinsam verschoben, ihre Hoehendifferenz bleibt
null, Welt-Z bleibt Kamera-Oben. Die bisherige absolute Bindung beider
Z-Koordinaten an null gilt fuer die Defaultansicht, nicht fuer vertikales
Verschieben. Draufsicht behaelt Blickrichtung Welt-minus-Z und Bug oben;
Verschieben erfolgt in der Bildebene. Zoomen aendert den Bildmassstab,
nicht die Projektion oder Ausrichtung.

Jede dieser drei Ansichten besitzt einen Button **Defaultansicht**.
Er setzt nur die zugehoerige Kamera auf die automatisch eingepasste
Standardansicht des aktuell dargestellten Modells/Zustands zurueck:
Benutzerzoom und Verschiebung entfallen, Seite/Heck wieder auf
Wasserlinien-Augenhoehe Z=0, Draufsicht wieder senkrecht mit Bug oben.
Andere Kameras, Fensterzuordnung und Core-Zustand bleiben unveraendert.
Fuer Frei wird kein neuer Defaultbutton gefordert.

### 6.3 Doppelklick und Ansichtstausch

**A-G-00102**, Herkunft IDEE-G-00066.

Ein Doppelklick auf den Bildbereich eines kleinen Fensters tauscht dessen
Ansicht mit der aktuell grossen Ansicht. Die beiden anderen kleinen
Fenster bleiben unveraendert. Das gilt auch fuer Frei, wenn es zuvor
in einen kleinen Fensterplatz getauscht wurde.

Name, Kameratyp, Zoom, Verschiebung, Ausrichtung, bisherige
Ansichtsfunktionalitaet und zugehoeriger Defaultbutton wandern mit der
Ansicht. Beim Wechsel der Bildgroesse bleiben die Kameraparameter erhalten;
kein unbeauftragtes Neueinpassen oder Zuruecksetzen.
Die kleinen Fensterplaetze behalten ihre gleich hohen Bildbereiche im
Hochformat Hoehe:Breite 4:3; dieses Format ist keine Bindung an einen
bestimmten Ansichtsnamen.

Doppelklick im grossen Bildbereich hat keine Wirkung, unabhaengig von
der dort gezeigten Ansicht. Buttons und andere Bedienelemente loesen
keinen Ansichtstausch aus. Ein Defaultbutton setzt insbesondere nicht
die Fensteranordnung zurueck.

### 6.4 Beschriftung und gemeinsame Daten

**A-G-00103**, Herkunft IDEE-G-00067, fuer Namen und deren Zuordnung.
Gemeinsame Daten bleiben dem bestehenden A-G-00096 zugeordnet.

In allen vier Fenstern steht der jeweilige Ansichtsname **unten rechts
im Fenster**: Frei, Seite, Heck oder Draufsicht. Er bleibt lesbar und
wandert beim Tauschen mit. Die bisherigen Titel oberhalb der kleinen
Ansichten werden dadurch ersetzt, nicht parallel weitergefuehrt.
Name und Bedienung duerfen sich weder gegenseitig noch die
Defaultbuttons ueberdecken.

Alle vier Ansichten zeigen weiterhin denselben ausgewaehlten Core-Zustand
und dieselben gueltigen Druckpunkte. Kamerabedienung und Tauschen starten
keinen Solverlauf. Die bisherigen Regeln zu Modellwechsel, Abbruch,
Reiterreset und verspaeteten Ergebnissen bleiben erhalten.
Eine Speicherung von Kamera- oder Tauschzustaenden ueber Programmstarts
ist nicht Teil dieses Nachtrags.

**Projektzuordnung aller neuen Punkte: GUI ja, Core nein.**

### 6.5 Geplante Abnahme

- Ausgangsanordnung und vier exakte Namen pruefen; Namen immer unten rechts,
  auch nach mehrfachem Tauschen und bei 100/150/200 Prozent DPI.
- Reale Mausgesten pruefen: Frei unveraendert; in Seite/Heck/Draufsicht
  funktionieren Zoom und Verschieben, ohne die Blickbindung aufzuheben.
- Nach Verschieben Seite/Heck horizontale Blickrichtung und Welt-Oben
  pruefen, nicht faelschlich weiterhin absolute Kamera-Z=0 verlangen.
  Nach Default dagegen Kamera-Z und Ziel-Z wieder exakt null pruefen.
- Default fuer jede gebundene Ansicht pruefen, im grossen und kleinen
  Fensterplatz; nur deren Benutzerkamera wird zurueckgesetzt.
- Doppelklick auf jeden kleinen Bildbereich tauscht genau zwei Ansichten;
  erneuter Tausch erhaelt Kamerazustand und Funktion. Doppelklick gross
  und Bedienung eines Defaultbuttons aendern die Zuordnung nicht.
- Bildformate, gemeinsame Pose/Druckpunkte, unveraenderte physikalische
  Eingaben sowie Bootsauswahl und Dynamik regressionspruefen.

Nachweise aus dem bisherigen G-0.10-Build belegen diese neue Bedienung
nicht. Neue Umsetzungs- und Pruefergebnisse stehen getrennt im Plan.
Technische Pruefung und persoenliche Nutzerabnahme sind zu unterscheiden.

## 7. Feste schematische Darstellung der gebundenen Ansichten

**A-G-00104**, Herkunft IDEE-G-00069. Durch ausdruecklichen
Umsetzungsauftrag vom 03.11.2026, 14:10 Uhr finalisiert und freigegeben.
Die zugleich praezisierte linke Verschiebegeste in Abschnitt 6.2
gehoert weiterhin zu A-G-00099 bis A-G-00101, Herkunft IDEE-G-00068.

Nur Seite, Heck und Draufsicht erhalten eine feste, nicht konfigurierbare
schematische Darstellung, unabhaengig vom kleinen oder grossen Fensterplatz:

- Yacht als vollstaendige, deckende, matte Geometrie in ihren Modellfarben
  auf hellem, ruhigem Hintergrund; auch Glas-/Stoffteile deckend.
  Keine Materialtexturen, Spiegellichter, Schatten oder Umgebungsreflexion.
- Wasserlinie als Schnittkontur bei Welt-Z=0 bleibt sichtbar, soweit
  ein tatsaechlicher Schnitt vorliegt. Nach Praezisierung vom
  03.11.2026, 14:24 Uhr bleibt die Wasserebene dezent gezeichnet
  (14 Prozent Deckkraft); Seite/Heck zeigen bei exakt horizontalem
  Blick zusaetzlich ihre feine Kantenlinie. Keine deckende Wasserflaeche,
  Tiefenebene, Wellen, Animation, Boots-/Umgebungsspiegelung oder Himmel;
  keine Fahrspur, Kielwasser, Raster oder zusaetzlichen Achsen.
- Feste Kantenglaettung und geglaettete Normalen unterstuetzen Lesbarkeit.
  Gueltige LDP-/Ersatz-SDP und die unveraenderte Core-Pose bleiben erhalten.
  Punkte und Beschriftungen liegen in den drei gebundenen Ansichten
  immer ueber der Szene, ohne Verdeckung durch Yacht oder Wasser.
  Fehlende/ungueltige Core-Punkte werden weiterhin nicht erfunden.
- Die Darstellungsseite konfiguriert ausschliesslich Frei. Ihre
  Einstellungen und Materialvorschauen duerfen die drei gebundenen
  Ansichten nicht veraendern. Bestehende gespeicherte Werte bleiben
  erhalten; Frei, Bootsauswahl und Dynamik werden nicht umgestaltet.
- Default passt wie bisher das aktuelle Modell vollstaendig ein;
  manuelles Zoom/Pan und deren Erhalt beim Tauschen bleiben moeglich.
  Kein automatisches Neueinpassen nach jeder Benutzerbewegung.

Dieser Nachtrag ersetzt fuer die drei gebundenen Ansichten die bisherige
Uebernahme des konfigurierbaren statischen Grafikprofils. GUI ja, Core nein.

Abnahme: linkes Ziehen in kleinen und grossen Plaetzen verschiebt Auge
und Ziel gleich weit, ohne Blickvektor, Welt-Oben oder Zoom zu aendern.
Seite/Heck bleiben wasserparallel, Draufsicht senkrecht. Alle Features
und tatsaechlichen Yachtmaterialien werden auch gegen abweichende
Grafikeinstellungen geprueft; Wasserlinie und deckende Geometrie
bleiben erhalten. Die Pruefung erfolgt bei 100/150/200 Prozent DPI.
Tatsaechliche Ergebnisse stehen im einzigen G-0.10-Umsetzungsplan;
persoenliche Nutzerabnahme bleibt davon getrennt.

## 8. Gesamtmoment und Druckpunktkraefte in den gebundenen Ansichten

**A-G-00105: zurueckgezogen am 03.11.2026, 15:23 Uhr auf Nutzerauftrag.**
Historie: um 14:54 Uhr Antriebsmomente, um 15:12 Uhr Gesamtmoment
beauftragt. Die Momentendarstellung wird vollstaendig entfernt:
keine Boegen, Achsenkreuze oder Momentenlegenden in den gebundenen
Ansichten. Kennung bleibt archiviert und wird nicht wiederverwendet.

**A-G-00107**, Herkunft IDEE-G-00072, freigegeben um 15:29 Uhr:
Seite, Heck und Draufsicht zeigen den Core-Massenschwerpunkt als
blauen Punkt mit Beschriftung **S**, wie SDP/LDP unverdeckt ueber
Yacht und Wasser. Fuer stationaere Ergebnisse entspricht
`moment_bezug_modell_m` dem Core-Schwerpunkt. Ohne stationaeren
Nachweis oder ohne Bezugspunkt keinen Schwerpunkt erfinden.
Pose und Ansichtstausch werden beruecksichtigt. S bezeichnet keine
feststehende Kippachse. Frei bleibt unveraendert.

**A-G-00106**, Herkunft IDEE-G-00071, durch denselben Auftrag finalisiert:
Am gueltigen Ersatz-SDP steht die Summe der gelieferten externen
Kraftbeitraege (`externe_kraft`/`fremdkraft`), am gueltigen LDP
ausschliesslich die Core-Querkraft `querkraft_modell_n` entlang Modell-Y.
Keine gesamte Wasserwiderstandskraft am LDP erfinden und keine
Gesamtkraft doppelt an beiden Punkten ansetzen.
Die Kraftpfeile werden nach Welttransformation auf die Bildebene
projiziert, exakt in der Farbe des zugehoerigen Punktes:
Seit Nutzerauftrag vom 03.11.2026, 15:42 Uhr:
SDP dunkelgruen `#176b38`, LDP hellgruen `#63cf79`. Senkrechte/verschwindende
Projektion ergibt keinen Pfeil. Die Laenge wird mit der bestehenden
logarithmischen Kraftskalierung lesbar begrenzt; kein metrischer
Laengenmassstab. Fehlende Kraftdaten ergeben keinen Ersatzpfeil.
Beide Pfeile sind wie Druckpunkte unverdeckt, in allen drei Ansichten
bei nicht verschwindender Projektion. Keine Symbole fuer senkrechte
Komponenten. Die zwei Pfeile allein rekonstruieren nicht die
vollstaendige Bilanz.

### 8.1 Auftriebspunkt und vertikale Summenkraefte

**A-G-00108**, Herkunft IDEE-G-00073, durch Umsetzungsauftrag vom
03.11.2026, 15:42 Uhr finalisiert. Nur die drei gebundenen Ansichten:

- **A** kennzeichnet den Angriffspunkt der gesamten Auftriebskraft,
  hellblau `#55b6f0`. Vorhandene Aggregation der parallelen Core-
  Auftriebsbeitraege verwenden: Kraftsumme und betragsgewichteter
  Angriffspunkt, mit korrekter Modell-/Welttransformation.
- **S** bleibt dunkelblau `#1a4dcc`; dort steht der Pfeil der
  aufsummierten Gewichtskraefte. S wird weiterhin nur aus dem
  nachgewiesenen stationaeren Core-Schwerpunkt angezeigt.
- Punkt, Beschriftung und zugehoeriger Kraftpfeil haben jeweils
  dieselbe Farbe. SDP dunkelgruen, LDP hellgruen wie oben.
- Alle Punkte und Pfeile bleiben unverdeckt. Kraftpfeile werden nur
  mit ihrer Bildebenenkomponente gezeichnet. Insbesondere erscheinen
  senkrechte Gewicht-/Auftriebskraefte in Seite und Heck; in der
  Draufsicht bleiben A/S sichtbar, aber ohne Pfeil fuer Welt-Z.
- Fehlende/ungueltige Einzelbeitraege nicht als vollstaendige Summe
  darstellen. Ohne belastbaren Angriffspunkt kein A erfinden.
  Keine neue Physik, Momentendarstellung oder Konfiguration;
  Frei und seine bisherigen Farben bleiben unveraendert.

## 9. Gemeinsames Kraftpfeil-Menue im statischen Modell

**Status:** Entwurf vom 03.11.2026, 16:12 Uhr durch ausdruecklichen
Umsetzungsauftrag „bitte umsetzen“ um 16:15 Uhr finalisiert und freigegeben;
Implementiert und technisch abgeschlossen laut Abschlussbericht vom
16:31 Uhr; finaler Wiederholungstest bestanden, persoenliche Nutzerabnahme offen.
Herkunft: IDEE-G-00074 bis IDEE-G-00079 in der
[Entwurfspunkteliste](Entwurfspunkte/GUI_Statisches_Modell.json).
A-G-00109 bis A-G-00114 wurden nach Pruefung oberhalb des bisherigen
projektweiten Hoechstwerts A-G-00108 vergeben. Die Umsetzungsschritte
und tatsaechlichen Nachweise stehen ausschliesslich im G-0.10-Plan.
Die bisherigen Abschnitte bleiben als Historie erhalten; im hier
ausdruecklich erweiterten Umfang hat Abschnitt 9 Vorrang, insbesondere
vor der Reduktion aus Abschnitt 1, den festen Farben und „Frei unveraendert“.

### 9.1 Geltungsbereich und feste stationaere Eingaben

**A-G-00109**, Herkunft IDEE-G-00074: Feste stationaere Vorgaben und
bereinigte Bedienung.

Nur Statisches Modell; GUI-Aenderung, keine Core-Aenderung vorgesehen.
Stationaere Kurvenfahrt ist immer zugelassen und der Kraftbezug der
Fremdkraft immer modellfest. Beide Auswahlfelder entfallen, waehrend
der Laufvertrag die beiden Werte ausdruecklich fest vorgibt.
Fremdkraft aktiv sowie Kraft-/Angriffspunktwerte und Ruder-/Schwert-
stellungen bleiben bedienbar. Alte gespeicherte Auswahlwerte duerfen
die festen Vorgaben nicht umgehen. Dynamik und Bootsauswahl unveraendert.

Der Langtext unter dem rechten Button **Darstellung** und die bisherigen
Statusangaben **Druckpunkte / LDP / SDP** entfallen. Ergebniswerte und
fachliche Fehler-/Warnmeldungen bleiben erhalten.

Unter Darstellung folgt ein gemeinsames Menue in dieser Reihenfolge:
**Kraftpfeile (einfach)**, **Kraftpfeile (vollstaendig)**, **Default Farben**.
Die sichtbare zweite Ueberschrift lautet mit Umlaut
**Kraftpfeile (vollständig)**.

### 9.2 Auswahl, Farben und Tooltips

**A-G-00110**, Herkunft IDEE-G-00075: Gemeinsames Kraftmenue mit
kurzen Auswahltexten, Tooltips und gemeinsamem Schalten von Punkt/Pfeil/Text.
Der ansichtsuebergreifende Vertrag steht ergaenzend in Abschnitt 9.3.

Checkboxtexte sind ausschliesslich die kurzen Namen der folgenden Tabelle.
Die Erklaerungen stehen als Mouseover-Tooltips bereit. Rechtsbuendig
neben jeder Zeile steht ein anklickbares Farbfeld, kein zweiter
Auswahlzustand. Seine Farbe entspricht der aktuell angewendeten Farbe.

| Abschnitt | Checkboxtext | Defaultfarbe | Initialauswahl | Erklaerung / Tooltip |
|---|---|---|---|---|
| Kraftpfeile (einfach) | Gewichtskraft | Dunkelblau `#1A4DCC` | ein | Gesamte Gewichtskraft am Massenschwerpunkt S. |
| Kraftpfeile (einfach) | Auftrieb | Hellblau `#55B6F0` | ein | Gesamte Auftriebskraft am Auftriebspunkt A. |
| Kraftpfeile (einfach) | Segeldruckpunkt | Dunkelgruen `#176B38` | ein | Resultierende Fremdkraft am Ersatz-SDP; noch keine Windphysik. |
| Kraftpfeile (einfach) | Lateraldruckpunkt | Hellgruen `#63CF79` | ein | Zugeordnete hydrodynamische Querkraft am LDP, nicht der gesamte Wasserwiderstand. |
| Kraftpfeile (einfach) | Richtungspfeil | Orange `#E88924` | ein | Gebogener Pfeil fuer die gueltige Core-Kursdrehrichtung; kein Kraft- oder Momentenpfeil. |
| Kraftpfeile (vollständig) | Reibungskraft | Rot `#D1495B` | aus | Summe aller hydrodynamischen Reibungsbeitraege des Objekts. |
| Kraftpfeile (vollständig) | Rumpfdruckkraft | Violett `#9467BD` | aus | Summe aller hydrodynamischen Rumpfdruckbeitraege. |
| Kraftpfeile (vollständig) | Ruderkraft | Violett `#9467BD` | aus | Resultierende aller hydrodynamischen Kraftbeitraege des Ruders, einschliesslich Reibung und Seitenkraft. |
| Kraftpfeile (vollständig) | Schwertkraft | Tuerkis `#159EAD` | aus | Resultierende aller hydrodynamischen Kraftbeitraege des Schwerts, einschliesslich Reibung und Seitenkraft. |
| Kraftpfeile (vollständig) | Tauchdämpfungskraft | Braun `#A66A3F` | aus | Summe der gelieferten vertikalen Tauchdaempfungskraefte. |
| Kraftpfeile (vollständig) | Testantrieb | Gelb `#D6A800` | aus | Resultierende Testantriebskraft, sofern diese Quelle beim Objekt vorhanden ist. |
| Kraftpfeile (vollständig) | Gesamtkraft ΣF | Dunkelgrau `#45515E` | aus | Vollstaendige Core-Gesamtkraft aus allen Beitraegen, nicht Summe der eingeschalteten Anzeigezeilen. |

**A-G-00111**, Herkunft IDEE-G-00076: Nur die sieben vollstaendigen
Summenkraftarten der Tabelle; Aggregationsvertrag in Abschnitt 9.4.
Die separate Checkbox **Seitenkraft** entfaellt zugunsten von Ruderkraft
und Schwertkraft. Keine Einzelkraftauswahl je Beitrag/Panel.
Nur beim ausgewaehlten Objekt vorhandene Kraftarten werden angeboten;
ein gueltiger Nullbeitrag ist nicht mit einer fehlenden Kraftart
gleichzusetzen. Keine Rolle aus einem beliebigen Bauteilnamen erraten.
Weitere Tragflaechen werden nicht unbeauftragt als Ruder/Schwert einsortiert.

Jede Checkbox schaltet Pfeil, zugehoerigen Punkt und Beschriftung
gemeinsam. Der Nutzer braucht zwischen Punkt und Kraft nicht zu
unterscheiden. Nicht vorhandene/ungueltige Punkte werden auch bei
eingeschalteter Auswahl nicht erfunden. Der Richtungspfeil hat keinen
zusaetzlichen Kraftangriffspunkt.

**A-G-00112**, Herkunft IDEE-G-00077: Persistente Farbfelder und
farbreiner Default-Reset gemaess der dokumentierten Palette.
Ein Klick auf das Farbfeld oeffnet einen Farbpicker mit der aktuellen
Farbe. Bestaetigen wendet die neue Farbe sofort auf Pfeil, Punkt und
Beschriftung an und speichert sie dauerhaft im statischen Namensraum.
Abbrechen veraendert weder Darstellung noch Speicherung.
Die Farben gelten ansichtsuebergreifend und bleiben nach Neustart,
Modellwechsel und Reiterwechsel erhalten; keine Aenderung dynamischer
Farben oder Profile. Ungueltige gespeicherte Farben werden ausdruecklich
diagnostiziert, nicht stillschweigend als erfolgreich geladen behandelt.

**Default Farben** steht unter allen Auswahlzeilen und setzt alle
Farben dieses Menues auf die Tabelle zurueck, auch derzeit nicht
angebotene Kraftarten. Ruecksetzung wird sofort angewendet und
persistent gespeichert; Checkboxzustaende, Kamera und Physik bleiben
unveraendert.

**A-G-00114**, Herkunft IDEE-G-00079: Eindeutige Initialauswahl ohne
konkurrierende alte statische Vektorcheckboxen.
Initialauswahl ohne vorhandenen Benutzerzustand:
alle einfachen Eintraege ein, alle vollstaendigen aus. Ein bestehender
Anzeige-/Benutzerzustand darf diese neuen Defaults nicht unbeabsichtigt
durch alte versteckte Vektorcheckboxen ueberschreiben.

### 9.3 Gemeinsame Wirkung in allen vier Ansichten

Ergaenzender Vertrag zu **A-G-00110**, Farben gemaess **A-G-00112**
und Initial-/Auswahlzustand gemaess **A-G-00114**:
Die Auswahl und Farben gelten fuer **Frei, Seite, Heck und Draufsicht**
gemeinsam im gesamten statischen Modell, unabhaengig vom Fensterplatz.
Diese ausdrueckliche Erweiterung ersetzt fuer Kraftueberlagerungen
die bisherige Ausnahme „Frei unveraendert“. Freie Kamerabedienung und
die konfigurierbare Grunddarstellung von Frei bleiben erhalten.
Die drei gebundenen Ansichten bleiben schematisch; das Menue
konfiguriert nur die hier beschriebenen Ueberlagerungen.
Keine widersprechende parallele statische Kraftauswahl in der
Technikdarstellung; deren Optikeinstellungen bleiben abgrenzbar.

In den gebundenen Ansichten werden nur Bildebenenkomponenten
dargestellt. Senkrechte/verschwindende Projektion erzeugt keinen Pfeil
und kein Ersatzsymbol; zugehoerige gueltige Punkte bleiben gemaess
Checkbox sichtbar. Yacht/Wasser verdecken die Ueberlagerungen dort
nicht. In Frei bleiben raeumliche Kraftvektoren und der bisherige
Verdeckungsvertrag erhalten; gleiche Auswahl/Farbe bedeutet nicht
gleiche Projektionsgeometrie.

**A-G-00113**, Herkunft IDEE-G-00078: Gemeinsamer Richtungspfeil.
Der **Richtungspfeil** nutzt die vorhandene Core-Kursdrehrate und die
bisherige gebogene Geometrie aus Frei. Er ist mit gemeinsamer Checkbox
und Farbe in **Frei und Draufsicht** sichtbar, nicht in Seite/Heck.
Bei Nullrate keinen Drehsinn erfinden; gemaess ausdruecklicher
Praezisierung vom 03.11.2026, 16:28 Uhr wird wie bisher in Frei
ein gerader Richtungspfeil dargestellt. Die gemeinsame Auswahl in
Frei/Draufsicht ist unabhaengig vom Kompass. Bei fehlendem/ungueltigem
Zustand oder Reset keinen Ersatzpfeil erfinden.
Diese Praezisierung ersetzt die bisherige Vorgabe „Nullrate ohne Pfeil“;
sie behauptet keine Kursdrehung bei Nullrate.
Kein Wiederbeleben der zurueckgezogenen
Momentendarstellung A-G-00105.

### 9.4 Summenbildung und physikalische Grenzen

Ergaenzender Aggregations- und Angriffspunktvertrag zu **A-G-00111**:

Vorhandene Core-Einzelbeitraege, Rollen und Summenvertraege verwenden.
Je Menueeintrag nur eine entsprechende Resultierende; Originalbeitraege
und bereits abgeleitete Summen nicht doppelt zaehlen. Welt-/Modellframes,
Pose und Ruder-/Schwertstellungen beachten. Reibungs-, Bauteil- und
Gesamtkraefte sind teilweise ueberlappende Auswertungen derselben Daten:
ihre gleichzeitige Anzeige erzeugt keine zusaetzlichen physikalischen
Kraefte. Dies wird im Tooltip der vollstaendigen Auswahl erlaeutert.

Parallele Gewicht-/Auftriebskraefte behalten ihre nachgewiesenen
resultierenden Angriffspunkte S/A. Bei allgemeinen nicht parallelen
Summen ist nicht immer ein eindeutiger gemeinsamer Angriffspunkt
bestimmt: nur einen belegten aequivalenten Angriffspunkt verwenden,
andernfalls die Resultierende am Core-Bezug S als
**Resultierende, Bezug S** kennzeichnen. Das ist keine Behauptung,
damit auch das vollstaendige Moment der Einzelbeitraege abzubilden.
Keine erfundene Bauteilmitte oder ein neuer Druckpunkt.

Fehlende/ungueltige erwartete Einzelbeitraege ergeben keine als
vollstaendig ausgegebene Teilsumme. Betroffene Darstellung unterdruecken
und den Datenmangel am Menueeintrag diagnostizieren, ohne die entfernten
pauschalen LDP-/SDP-Statusbloecke wieder einzufuehren.
Keine Core-Aenderung geplant; eine tatsaechliche Vertragsluecke vor
fachlichen Ersatzannahmen melden.

### 9.5 Verbindliche Abnahmekriterien

- A-G-00109: Entfernte stationaere Auswahlfelder sind nicht mehr sichtbar; der
  tatsaechliche Core-Auftrag enthaelt immer Kurvenfahrt=true und
  modellfesten Fremdkraftbezug. Andere Reiter bleiben unveraendert.
- A-G-00110, A-G-00112, A-G-00114: Beide Menuegruppen, Reihenfolge, Kurznamen, Tooltips und rechtsbuendige
  Farbfelder stimmen mit der Tabelle ueberein. Unpassende Kraftarten
  fehlen. Initial einfach ein/vollstaendig aus.
- A-G-00110, A-G-00114: Jede Auswahl wirkt sofort auf alle vier statischen Ansichten und
  schaltet Punkt/Pfeil/Text gemeinsam; Tausch aendert keine Einstellung.
- A-G-00112, A-G-00114: Farben stimmen fuer Punkt/Pfeil/Text ueberein; Picker-Abbruch,
  Neustartpersistenz und Default Farben ohne Auswahlreset nachgewiesen.
- A-G-00111: Kontrollierte Core-Beitraege pruefen jede Resultierende, insbesondere
  getrennte Ruder-/Schwertsumme ohne doppelte Aggregation und die
  unabhaengige Core-Gesamtkraft. Datenluecken sind sichtbar behandelt.
- A-G-00113: Richtungspfeil stimmt in Frei/Draufsicht fuer beide Vorzeichen;
  Nullrate mit geradem Pfeil wie bisher in Frei, Reset ohne Pfeil;
  unabhaengig vom Kompass, Seite/Heck ohne Richtungspfeil.
- A-G-00109, A-G-00110: Gebundene Projektion und unverdeckte Ueberlagerungen, freie Kamera,
  Ergebnisse sowie Dynamik/Bootsauswahl bleiben funktional.

Technische Nachweise und persoenliche Nutzerabnahme werden getrennt
im bestehenden G-0.10-Plan dokumentiert. Gemeldete Build-/Testergebnisse
stehen dort in Abschnitt 9; der finale Wiederholungstest ist bestanden.
Alle technischen Gates dieses Nachtrags sind abgeschlossen;
persoenliche Nutzerabnahme bleibt offen.

### 9.6 Umsetzungsstand und Darstellungsentscheidung, 03.11.2026, 16:28 Uhr

Laut Umsetzungsbericht sind gemeinsame statische Auswahl/Farben in allen
vier Ansichten, persistenter Farbpicker und reiner Farbreset implementiert.
Modellrollen stammen aus der bestehenden Core-Inspektion; keine
Core-Aenderungen. Ruderkraft und Schwertkraft summieren jeweils alle
hydrodynamischen Komponenten. Modell-/Weltframes werden umgerechnet,
gueltige Nullwerte von fehlenden Arten unterschieden und ungueltige oder
unvollstaendige Inventare nicht als vollstaendige Teilsummen ausgegeben.
Die Gesamtkraft stammt exklusiv aus dem Core-Gesamtwert, nicht aus
addierten Anzeigezeilen. Kurvenfahrt/modellfest sind fest vorgegeben;
zugehoerige Auswahlfelder entfernt, Langtext/Status nur statisch verborgen.

Dokumentierte Ausfuehrung des bestehenden Kennzeichnungsvertrags,
**keine neue Anforderung**: Die betreffenden Zeilentooltips erklaeren
ausdruecklich **„Resultierende, Bezug S: kein nachgewiesener Angriffspunkt
und keine vollständige Momentenäquivalenz.“** Zusaetzliche Namen der
vollstaendigen Resultierenden wie „Name (an S)“ werden bewusst nicht
ins Bild gezeichnet, weil sie sich in den kleinen Ansichten ueberlagern.
Die bestehenden A-/S-/SDP-/LDP-Beschriftungen bleiben an die einfachen
Auswahlen gebunden. Gemeinsames Schalten/Faerben von Punkt/Pfeil/Text
bezieht sich auf diese vorhandenen zugeordneten Elemente, nicht auf
neu einzufuehrende Bildtexte fuer jede vollstaendige Resultierende.
Persoenliche Abnahme der Lesbarkeit bleibt offen.

**Finaler Nachweis, 03.11.2026, 16:31 Uhr:** Laut Abschlussbericht
Standard-EXE erneut fehlerfrei als Release gebaut. Ein gemeinsamer
CTest-Lauf nach der Bereinigung: `gui_workspace_tabs_100` bestanden,
40,06 s; `gui_workspace_tabs_200` bestanden, 42,37 s;
`gui_force_display` bestanden, 0,14 s; insgesamt 3/3, 82,59 s.
Der erweiterte Workspace-Test prueft die gewaehlte Farbe an echten
VTK-Punkt- und Pfeilakteuren in allen vier Ansichten, den echten
Farbpicker, Persistenz nach erneutem Oeffnen, Auswahlbeibehaltung beim
Farbreset, den kompassunabhaengigen Richtungspfeil in Frei/Draufsicht
und alle vorzeichenbehafteten Kurvenfahrtfaelle. Der reine Test prueft
auch gemischte Welt-/Modellframes der Auftriebskraefte und die
Unterdrueckung unvollstaendiger Summen. Alle technischen Gates
abgeschlossen; keine persoenliche Nutzerabnahme vorweggenommen.
GUI bleibt uncommittet, keine Core-Aenderungen oder Git-Aktionen.

## 10. Statische Wertetabelle und Auswahlreihenfolge

Freigegeben durch Fortsetzungsauftrag vom 03.11.2026, 17:13 Uhr und
anschliessende Praezisierung zur Reihenfolge.

**A-G-00115: Wertetabelle statt linkem Infoblock.**
Im statischen Modell entfallen der linke Bilanz-/Keine-Daten-Block,
Zeit-/Schritttext und der Button „Werte kopieren“. Der Wertebereich
enthaelt eine Tabelle ueber die gesamte Breite mit den Spalten:
Objekt/Pfad, Art, Bezeichnung, Betrag, X, Y, Z, Einheit, Bezugspunkt,
Status. Alle gelieferten Kraft-/Momentenbeitraege sowie eindeutig
bezeichnete Gesamtdarstellungen sind tabellarisch verfuegbar, unabhaengig
von der Pfeilauswahl. Filter je Spalte kombinieren sich wie in
Bootsauswahl; Sortieren (Zahlen numerisch), veraenderbare Breiten und
Mehrfachauswahl stehen bereit. Strg+C kopiert die ausgewaehlten
sichtbaren Zeilen, ohne Auswahl alle gefilterten Zeilen.

Komponenten erscheinen einheitlich im Modellframe, Momente um S.
Weltfeste Vektoren und Punkte werden mit der aktuellen Core-Pose
umgerechnet. Bezugspunkte zeigen Modellkoordinaten, bei nachgewiesenen
Gesamtwerten A beziehungsweise S. Ungueltige Werte bleiben als Status
erkennbar statt als gueltige Nullwerte. Dynamik und Bootsauswahl
behalten ihre bisherige Darstellung.

Abnahme: echte Core-Kraefte und Momente in der Tabelle, linker Block
statisch unsichtbar, Filterkombination/Numeriksortierung/Mehrfachauswahl
bedienbar und Datenreset ohne alte Zeilen.

**A-G-00116: Gewichtskraft vor Auftrieb.**
Die ersten beiden Auswahlzeilen unter „Kraftpfeile (einfach)“ lauten
Gewichtskraft, Auftrieb. Farben, Zuordnung und gespeicherte Auswahl
bleiben unveraendert. Abnahme: sichtbare Zeilenreihenfolge.

## 11. Navigation neben der Hauptansicht

**A-G-00117**, direkter Umsetzungsauftrag vom 03.11.2026, 17:23 Uhr:
Im statischen Modell steht die bestehende Navigation als eigene schmale
Spalte zwischen Auswahl/Darstellung links und Hauptbild. Kompass,
Lageanzeigen, Geschwindigkeit und Kursdrehrate bleiben unveraendert
an den Core-Zustand gebunden. Sie ueberdecken das Hauptbild nicht mehr.
Bei geringer Hoehe ist die Spalte scrollbar statt Instrumente abzuschneiden.
Der bestehende Navigationsschalter blendet die gesamte Spalte ein/aus.
Ein Tausch von Frei mit einer kleinen Ansicht verschiebt die Navigation
nicht. Beim Wechsel zur Dynamik kehrt dieselbe Navigation in das
bisherige verschiebbare/skalierbare Overlay zurueck.

Abnahme: Platzierung zwischen Auswahl und Hauptbild, keine Ueberdeckung,
vollstaendige Instrumente, Ein-/Ausblenden ohne leere Restspalte und
unveraenderte Werte beim Ansichtstausch/Workspacewechsel.

### 11.1 Hoehenaufteilung, Lageansichten und Wiedergabe

**A-G-00118**, Nutzerauftrag vom 03.11.2026, 17:33 Uhr:
In der statischen Navigationsspalte entfaellt die Ueberschrift Navigation.
Die Geschwindigkeitsanzeige behaelt ihre inhaltsbasierte Hoehe; nur ihre
Breite folgt der Spalte. Kompass, Kraengung und Stampfen teilen sich die
restliche Hoehe gleichmaessig. Die Spaltenbreite waechst passend zur
Instrumentgroesse. Abstaende betragen wie rechts 6 logische Pixel.
Dynamisches verschiebbares Overlay bleibt unveraendert.

**A-G-00119**, gleicher Auftrag: Direkt unter „Kompass und Lageanzeigen“
steht statisch die Checkbox „Lageansichten“, initial ein, persistent.
Sie blendet die gesamte rechte Dreifachansicht ein/aus, ohne Zustaende,
Kameras oder die davon unabhaengige Navigationsspalte zurueckzusetzen.

**A-G-00120**, anschliessender Auftrag „Wiedergabe entfernen“:
Im statischen Modell entfaellt die vollstaendige Wiedergabezeile,
einschliesslich ihres Leerraums. Dynamische Zeitnavigation bleibt erhalten.

Abnahme: versteckte Navigationsueberschrift, konstante Geschwindigkeitshoehe
beim Resize, Instrumente nutzen Resthoehe mit 6-Pixel-Abstaenden,
Lageansichten schaltbar und keine sichtbare statische Wiedergabezeile.

### 11.2 Sichtbare Kartenkanten und gemeinsame Rundungen

**A-G-00121**, direkter Auftrag vom 03.11.2026, 17:40 Uhr:
Kompass beginnt sichtbar buendig mit den Nachbarfeldern. Unsichtbare
SVG-Aussenraender werden in der statischen Kartenanzeige ausgespart.
Aus verfuegbarer Hoehe H, fester Geschwindigkeitshoehe V und drei
Abstaenden g = 6 logischen Pixeln folgt die Instrumenthoehe
(H - V - 3g) / 3. Restpixel werden auf die drei Instrumente verteilt;
erst danach wird die Spaltenbreite aus der Instrumenthoehe bestimmt.
Geschwindigkeit bleibt unten, alle drei sichtbaren Zwischenraeume sind
6 Pixel. Karten-/Panelrundungen in Bootsauswahl und statischer Anzeige
werden auf 12 logische Pixel angeglichen, insbesondere Geschwindigkeit
und Instrumentkarten; Bedienkontrollen behalten ihre eigene Form.

### 11.3 Sichtbare Spaltengrenzen

**A-G-00122**, direkter Auftrag vom 03.11.2026, 17:50 Uhr:
Die Filtertabellen in Bootsauswahl und statischem Modell erhalten helle
senkrechte Spaltentrenner im Kopf. An interaktiv veraenderbaren
Spaltengrenzen erscheint der horizontale Doppelpfeil-Cursor.
Filterfelder lassen den ziehbaren Rand frei. Zunaechst keine Tests
gemaess Nutzerauftrag; am 03.11.2026, 17:54 Uhr ausdruecklich nur
Linien-/Cursor-Pruefung freigegeben. Nachweis im G-0.10-Plan.

## 12. Gemeinsames Layout auch im Dynamischen Modell

Direkter Nutzerauftrag vom 03.11.2026, 17:59 Uhr, anschliessend durch
„go“ zur Umsetzung freigegeben. Dieser Nachtrag erweitert den bisherigen
statischen Layoutumfang gezielt auf Dynamik. Aussagen der vorherigen
Abschnitte „Dynamik unveraendert“ gelten fuer diese Anzeigeoberflaechen
ab jetzt nicht mehr. Die dynamische Physik bleibt unveraendert.

**A-G-00123: Dynamische Anzeige nahezu identisch aufbauen.**
Das Dynamische Modell verwendet dieselben vorhandenen Komponenten wie
das Statische Modell: linke Darstellungsauswahl mit Kraftmenue/Farben,
separate Navigation, grosse freie Ansicht, drei schaltbare und per
Doppelklick tauschbare Lageansichten, vollbreite Kraft-/Momententabelle
mit Filtern, Sortierung, Mehrfachauswahl und Kopieren. Das alte dynamische
Modellverzeichnis, die alten Vektorkontrollen und der alte Werteblock
werden dort nicht mehr angezeigt. Die dunkelgruene Workspace-Farbe bleibt.
Auswahl/Farben sowie Sichtbarkeit von Navigation/Lageansichten werden
zwischen beiden Modellreitern gemeinsam verwendet; bestehende
Speicherschluessel bleiben erhalten. Alle vier Szenen erhalten denselben
tatsaechlichen dynamischen Core-Snapshot, auch beim Rueckblick.
Grafikprofile der freien Ansicht bleiben pro Modellmodus getrennt.

**A-G-00124: Dynamische Laufbedienung erhalten und ergaenzen.**
Dauer (initial 10,000 s), Schrittweite (initial 0,0100 s) und
Takt „Echtzeit (1 s = 1 s)“ bleiben dynamisch sichtbar. Testantrieb aktiv,
Testkraft, Geschwindigkeitsgrenze, Ruder-/Schwertsteuerung, Start/Stop
und dynamische Wiedergabe bleiben funktionsfaehig. Direkt unter
„Testantrieb aktiv“ steht die Auswahl „Koerpermodell“ / „Weltmodell“
(sichtbar mit Umlaut). Keine stationaeren Solveroptionen in den
dynamischen Core-Auftrag uebertragen.

**Offengelegte technische Interpretation:** Die Auswahl steuert den
Kamerabezug der freien Ansicht, beschriftet mit „Kamerabezug“:
Koerpermodell folgt der Bootspose (bisheriger Default), Weltmodell
haelt Kamera, Zielpunkt und Blickoben weltfest. Umschalten verursacht
keinen Kamerasprung; es wirkt auf nachfolgende Zustandsdarstellung.
Die drei gebundenen Ansichten behalten ihren definierten Bezug.
Antriebsphysik und modellfeste Zahlenkoordinaten aendern sich nicht.
Die Rueckfrage zur Bedeutung war nicht beantwortbar; diese Interpretation
ist keine ausdrueckliche fachliche Nutzerbestaetigung und bleibt zur
Sicht-/Bedeutungsabnahme offen.

Abnahme: kurzer echter dynamischer Core-Lauf samt Testantrieb fuellt die
Tabelle und alle vier Ansichten; grafisches Abschalten entfernt keine
Zahlenzeilen. Filtern und Ansichtstausch funktionieren auch dynamisch.
Kamerabezug mit kontrollierter Translation nachweisen. Beim Reiterwechsel
werden Ergebnisse verworfen, dynamische Eingaben bleiben erhalten und
stationaere Eingaben erscheinen wieder ausschliesslich statisch.
Linien-/Cursor-Pruefung aus A-G-00122 gilt nun auch fuer Dynamik.

<a id="13-entwurf-identische-rechenbedienung-fuer-statik-und-dynamik"></a>

## 13. Identische Rechenbedienung fuer Statik und Dynamik

**Status:** Durch ausdruecklichen Umsetzungsauftrag vom 03.11.2026,
18:43 Uhr fuer den GUI-Umbau und die Core-Mindestintegration finalisiert.
Der Haltepunkt vor persoenlicher GUI-Pruefung bleibt verbindlich;
historischer Vergleich und weitere Optimierung sind noch nicht freigegeben.
Dieser Abschnitt ist die einzige GUI-Spezifikation dieses Themas und
ersetzt die widersprechenden dynamischen Bedien-/Speicherregeln aus Abschnitt 12.
Entwurfspunkte und offene Entscheidungen:
[GUI_Dynamische_Rechenbedienung.json](Entwurfspunkte/GUI_Dynamische_Rechenbedienung.json).
Core-Gegenstueck:
[Dynamische Fremdkraft](../../CORE/Spezifikationen/Spezifikation_Dynamische_Fremdkraft.md).

### 13.1 Ziel und sichtbare Bedienung

**A-G-00125**, Herkunft IDEE-G-00080: Nahezu identische Oberflaechen
mit strikt moduslokaler Darstellung gemaess diesem Abschnitt.

Der Nutzerauftrag vom 03.11.2026, 18:17 Uhr ersetzt die rechte dynamische
Bedienleiste durch die im Nutzerbild gezeigte statische Bedienleiste,
inhaltlich und geometrisch 1:1, mit dynamischer Workspace-Farbgebung.
Gemeint ist die Bedienleiste, nicht ein eingefrorenes Screenshotbild.
Gemeinsame Komponenten verwenden; keine zweite abweichende Bedienlogik.

Praezisierung durch Nutzer vom 03.11.2026, 18:22 Uhr:
**Statische und dynamische Eingabe sind nahezu identisch. Settings,
Darstellung sowie Look and Feel sind gleich; Dynamik verwendet Gruen
statt Blau.** Dies gilt fuer die gesamte Modelloberflaeche, nicht nur
die rechte Rechenleiste: gleiche Anordnung, Beschriftungen gemeinsamer
Felder, Einheiten, Wertebereiche, Zahlenformate, Tooltips, Defaults,
Abstaende, Rundungen, Schriftgroessen, Fokus-/Tastaturbedienung,
Scrollverhalten und Einstellungsmoeglichkeiten.
Praezisierung vom 03.11.2026, 18:26 Uhr: Gleichheit meint dieselben
Optionen, Bedienung und Ausgangsdefaults, **nicht gemeinsam veraenderte
Darstellungswerte**. „Darstellung“ im statischen Reiter aendert nur die
statische Darstellung, im dynamischen Reiter nur die dynamische.
Technische Grafikprofile, Darstellungsauswahl und benutzerdefinierte
Darstellungswerte werden je Modellmodus getrennt gehalten, gespeichert
und beim Wechsel wiederhergestellt. Eine Aenderung darf weder sofort
noch nach Neustart in den anderen Modus durchschlagen. Gemeinsame
Widgets sind erlaubt, gemeinsam ueberschriebene Konfiguration nicht.
Kraft-/Objektfarben haben gleiche Ausgangsdefaults; explizite
Darstellungsaenderungen bleiben moduslokal. Nur die
Workspace-/Bedienelement-Themepalette wechselt grundsaetzlich von Blau
zu Gruen. Bestehende technische Grafikprofile erhalten, nicht loeschen.
Diese Praezisierung ersetzt fuer den geplanten neuen Umfang die
gemeinsame Darstellungsspeicherung aus Abschnitt 12.

Erlaubte dynamische Besonderheiten sind die Laufparameter
Simulationsdauer/Schrittweite (mit bestehender Echtzeittaktung im
Startdialog), die Koerpermodell-/Weltmodell-Auswahl sowie die
notwendige Zeitnavigation fuer dynamische Ergebnisse. Keine weiteren
abweichenden Eingabebloecke ohne Nutzerfreigabe.

Reihenfolge und Inhalt:

1. SIMULATION, „Core-Lauf starten“, „Lauf stoppen“.
2. „Eine Fremdkraft“, „Fremdkraft aktiv“.
3. „Kraftvektor (X, Y, Z)“ mit drei Eingaben in N.
4. „Gemeinsamer Ansatzpunkt (Modell)“ mit drei Eingaben in m.
5. „Ruder- und Schwertstellung (Core)“ mit festen Startstellungen,
   Einheiten, Grenzen und Verfuegbarkeit aus dem geladenen Core-Modell.
6. LAUFSTATUS und Simulationszeit.

Abstaende, Rundungen, Feldbreiten, Zahlenformatierung und Scrollverhalten
entsprechen der statischen Leiste. Modellabhaengige Werte im Bild
(420er, Ruder 0 Grad, Schwert 100 Prozent) sind keine hartkodierten
Vorgaben fuer andere Boote. Unverfuegbare Bewegungen nicht erfinden.
Die bisherige dynamische Testantriebsgruppe und die Live-Stellungsslider
entfallen aus dieser Leiste. Die Koerpermodell-/Weltmodell-Auswahl bleibt
hingegen als ausdruecklich gewuenschte dynamische Besonderheit erhalten,
ohne eine Testantriebsgruppe dafuer beizubehalten. Vorgeschlagene
Platzierung: unmittelbar nach den gemeinsamen Fremdkrafteingaben.
Das ist keine Loeschung des Core-Testantriebsvertrags.
Vier Ansichten, gemeinsame Darstellungsauswahl, Navigation, Wertetabelle
und dynamische Wiedergabe bleiben erhalten.
Die Modellwahl bleibt Kamerabezug, nicht Kraftkoordinatensystem.
Technische Auslegung bei nicht erreichbarem Nutzer: Koerpermodell folgt
Translation und Rotation, Weltmodell folgt nur der Bootsposition mit
weltorientiertem Blickoben. Beide erhalten Zoom und manuelles Pan und
lassen das Boot nicht aus dem Bild fahren. Diese Auslegung ist keine
gesondert bestaetigte Nutzerentscheidung und wird bei der persoenlichen
GUI-Pruefung ausdruecklich vorgelegt; keine Aenderung der Kraftphysik.

**Kamerareferenz geprueft:** Im veroeffentlichten G-0.9-Stand von
[vtk_scene.cpp](../Code/src/vtk_scene.cpp), `setState`, werden
Kameraposition, Zielpunkt und Blickoben erst in den bisherigen
Bootsframe und dann in die neue Bootspose transformiert. Die Kamera
folgt also Translation und Rotation des Bootes; das Boot bleibt bei
unveraenderter Bedienung im gewaehlten Bildausschnitt beziehungsweise
nach Default-Einpassung im Zentrum. Das ist keine weltfest stehende
Kamera, aus deren Bild das Boot herausfaehrt. Dieses bisherige
Nachfuehren erhalten, ohne pro Simulationsschritt Zoom oder manuelles
Pan neu einzupassen. Die zuletzt eingefuehrte absolute Weltkamera
ist keine vom Nutzer bestaetigte Ersatzreferenz. Die obige technische
Auslegung ersetzt diese absolute Weltkamera und erhaelt die Zentrierung.

### 13.2 Laufparameter erst beim Start

**A-G-00126**, Herkunft IDEE-G-00081: Startdialog und unveraenderter
Abbruchvertrag gemaess diesem Abschnitt.

Dauer und Schrittweite stehen nicht mehr dauerhaft in der rechten
dynamischen Leiste. „Core-Lauf starten“ oeffnet dynamisch einen modalen
Dialog mit Dauer und Schrittweite; bisherige Anfangswerte 10,000 s und
0,0100 s beibehalten. Weitere Laufstarts bieten die zuletzt bestaetigten
Werte der Sitzung an. Der statische Start bleibt ohne Dynamikdialog.

„Starten“ prueft Eingaben und startet genau einen dynamischen Lauf;
Abbrechen startet nichts, verwirft keine bisherigen Ergebnisse und
veraendert keine zuletzt bestaetigten Laufparameter. Ungueltige Werte
werden mit konkreter Meldung am Dialog abgewiesen, nicht still korrigiert.
Bestehende Bedeutung Dauer=0 (bis Abbruch), positive Schrittweite,
Bereichs- und Zustandszahlgrenzen bleiben erhalten und sichtbar erklaert.
Die bisherige Echtzeitwahl steht ebenfalls im Startdialog, mit bisherigem
Default ein; kein weiterer dauerhafter Unterschied in der Rechenleiste.

### 13.3 Eingaben als dynamischer Rechenfall

**A-G-00127**, Herkunft IDEE-G-00082: Unveraenderlicher modellfester
Fremdkraftfall mit exakter Core-Uebergabe, ohne Ersatz-Testantrieb.
**A-G-00128**, Herkunft IDEE-G-00083: Feste Core-Stellungen ab Zustand
null, gemeinsame physikalische Eingaben und sicherer Reiterwechsel.

Beim bestaetigten Start werden Aktivflag, Kraftvektor, Ansatzpunkt und
alle angebotenen Elementstellungen als unveraenderlicher Rechenfall
erfasst und zusammen mit Dauer/Schrittweite an den Core uebergeben.
Keine stationaere Gleichgewichtsberechnung vorschalten; nicht automatisch
ein vorheriges statisches Ergebnis als Anfangszustand verwenden.
Die bestehende dynamische Anfangszustandsbehandlung bleibt erhalten.
Feste Stellungen gelten ab dem ersten ausgegebenen Zustand und ueber den
gesamten Lauf. Diese Eingaben sind waehrend eines Laufs nicht editierbar;
Live-Aenderungen sind nicht Bestandteil dieses Umfangs.

**Nahtloser Reiterwechsel, Praezisierung 18:26 Uhr:** Gemeinsame
physikalische Eingaben rechts sind kein moduslokales Darstellungssetting.
Fremdkraft-Aktivflag, X/Y/Z, Modellansatzpunkt sowie angebotene feste
Ruder-/Schwertwerte bleiben fuer dasselbe Modell beim Wechsel
Statik -> Dynamik und Dynamik -> Statik erhalten. Nicht auf Defaults
zuruecksetzen und keine getrennten, veralteten Eingabekopien anbieten.
Modellwechsel oder unterschiedliche Core-Metadaten duerfen nur
tatsaechlich nicht mehr gueltige Werte mit konkreter Meldung zurueckweisen;
gueltige Werte soweit moeglich erhalten, niemals still klemmen.
Modusspezifische Laufparameter und Darstellungseinstellungen bleiben
getrennt erhalten. Jeder laufende Simulationslauf wird beim Reiterwechsel
wie bisher gestoppt; verspaetete Ergebnisse duerfen den neuen Reiter
nicht befuellen. Bestehender Ergebnisreset bleibt erhalten, darf aber
keine Nutzereingaben loeschen. Nahtlos bedeutet Eingabeuebernahme,
nicht automatische Fortsetzung oder Uebernahme eines Solverzustands.

Kraftvertrag wie in der Statik: Kraftkomponenten und
Ansatzpunkt sind modellfest; der Kraftvektor dreht mit der Bootspose.
Die erhaltene Koerpermodell-/Weltmodell-Auswahl aendert ausschliesslich
den Kamerabezug, niemals den Kraftframe.
Fremdkraft deaktiviert bedeutet kein Fremdkraftbeitrag, nicht Boot stoppen.
Der alte Testantrieb darf nicht zusaetzlich oder verborgen aktiv bleiben.
Kraefte/Momente werden allein aus Core-Ergebnissen visualisiert; GUI
erfindet keine Ersatzkraft oder Momentensumme.

Ist der neue Core-Vertrag nicht verfuegbar, den dynamischen Start mit
konkreter Inkompatibilitaetsmeldung verhindern; keine scheinbar
erfolgreiche Simulation ohne die angeforderten Eingaben.

### 13.4 Verantwortlichkeiten und geplante Abnahme

**A-G-00129**, Herkunft IDEE-G-00084: Tatsaechliche technische Nachweise
fuer GUI/Core-Paritaet und sichtbare Trennung von Umsetzung, fachlicher
Abnahme sowie noch gesperrtem historischem Vergleich.

| Bereich | GUI | Core |
|---|---|---|
| Leistenlayout und Startdialog | Umsetzung, Validierung und Bedienung | keine UI-Aufgabe |
| Dauer/Schrittweite | vorhandene Werteuebergabe aus Dialog | vorhandene Laufsteuerung weiterverwenden |
| Feste Ruder-/Schwertstellung | Metadaten anzeigen, Startwerte uebergeben | bestehende Bewegungsmechanik verwenden und Wirkung ab erstem Zustand nachweisen |
| Allgemeine Fremdkraft | Konfiguration uebergeben, Beitraege anzeigen | neuer dynamischer Kraft-/Momentenvertrag |
| Ergebnis und Wiedergabe | gelieferten Snapshot anzeigen | konsistente Beitraege, Summen und Historie liefern |

Pruefkriterien; tatsaechliche Ergebnisse im einzigen G-0.10-Plan:

- Leiste gegen statische Referenz bei 100/200 Prozent vergleichen;
  keine permanenten Dauer-/Schritt- oder alten Testantriebsfelder.
- Gesamte statische/dynamische Oberflaeche bei gleichem Modell,
  gleicher Fenstergroesse und gleich gesetzten moduslokalen Einstellungen
  direkt vergleichen: gleiche Settings, Darstellung und Bedienung.
  Gemeinsame Controls haben gleiche Geometrie, Wertebereiche, Defaults
  und Tastaturinteraktion. Unterschiede ausschliesslich anhand der
  oben erlaubten dynamischen Besonderheiten und gruenem statt blauem
  Theme erklaerbar; Kraft-/Objektfarben stimmen exakt ueberein.
- Koerpermodell-/Weltmodell-Auswahl ist dynamisch vorhanden, statisch
  nicht zusaetzlich eingefuehrt. Wirkung gemaess offengelegter technischer
  Auslegung pruefen; nicht nur Vorhandensein des Widgets nachweisen.
- Darstellungstrennung: statisch einen Grafikwert aendern, dynamischen
  Wert unveraendert nachweisen, dann umgekehrt; Wiederherstellung beim
  Reiterwechsel und nach Neustart pruefen.
- Reiterwechsel mit nichttrivialer Fremdkraft, Ansatzpunkt und gueltigen
  festen Stellungen in beide Richtungen: Eingaben bleiben exakt erhalten;
  laufender Solver stoppt, spaete Ergebnisse werden verworfen.
- Kamera mit Translation und Rotation gegen G-0.9-Referenz pruefen:
  Boot bleibt ohne Bedienung zentriert, manuelles Pan/Zoom wird nicht
  durch staendiges Neueinpassen geloescht.
- Startdialog bestaetigen/abbrechen; kein Lauf bei Abbruch oder
  ungueltigen Werten, kein Verlust bestehender Ergebnisse bei Abbruch.
- Uebergabetest mit von null verschiedenen X/Y/Z, seitlichem Ansatzpunkt
  und von Defaults abweichenden Ruder-/Schwertstellungen; erfasste Werte
  stimmen exakt mit dem Core-Rechenfall ueberein.
- Echter dynamischer Lauf: Fremdkraft und Moment in Zahlen und Ansichten;
  deaktivierter Lauf ohne Fremdkraft, kein alter Geschwindigkeitsbegrenzer.
- Statik bleibt unveraendert, keine Verwechslung der Solvermodi;
  Stop, Rueckblick und Reiterwechsel bleiben funktionsfaehig.

Offene fachliche Punkte werden ausschliesslich in der verlinkten
Entwurfspunkteliste verfolgt. Der anschliessend ausdruecklich
beauftragte Plan steht im bestehenden
[G-0.10-Plan, Abschnitt 13](../Umsetzungsplaene/Umsetzungsplan_G-0.10.md#13-einheitliche-rechenbedienung-und-performancevergleich).
IDEE-G-00080 bis IDEE-G-00084 bleiben als Herkunft dauerhaft reserviert;
zugeordnet sind A-G-00125 bis A-G-00129 und U-G-000131 bis U-G-000139.

### 13.5 Nachpruefung Simulationsvertraege und Performance

Auf Nutzerauftrag vom 03.11.2026, 18:28 Uhr zuerst die fachliche
Uebernahme und danach Performance untersucht. Verbindliche fachliche
Pruefmatrix fuer den neuen Core-Entwurf:
[Abschnitt 6.1 der Core-Spezifikation](../../CORE/Spezifikationen/Spezifikation_Dynamische_Fremdkraft.md#61-vollstaendige-fachliche-uebernahme-aus-der-statik).
Vollstaendige Uebernahme ist derzeit **nicht nachgewiesen**.
Historischer Vorbefund vor G-0.10/C-0.9: allgemeine dynamische Fremdkraft
fehlte und bewegte Geometrie sowie Integrator-Schwerpunkt/-Traegheit
waren inkonsistent. Diese Mindestluecken sind inzwischen implementiert
und gezielt getestet; die weitergehende Matrix bleibt offen.
Bereits vorhandene lokale
Rotationsanstroemung, Kirchhoff-/Kreiselterme und V6-Ausgabe sind keine
automatische Abnahme des neuen Gesamtrechenfalls.
GUI uebernimmt alle Kraft-/Momentenkomponenten, korrekte Frames,
signierte Welt-Kursdrehrate, verifizierte S-/Druckpunktbezuege und
ehrliche Datenlueckendiagnosen auch dynamisch. Keine stationaere
Konvergenzforderung Summe F/M=0 auf eine beschleunigte Bahn anwenden.
Stationaere numerische Suchheuristiken nicht in Zeitschritte kopieren.

**Historische Voruntersuchung vor Abschluss der Mindestintegration:**
Die folgenden Zahlen bleiben als damaliger Befund erhalten, nicht als
aktuelle Versionsvergleichsmessung. Die autorisierte erneute Analyse
steht in Abschnitt 13.7 und im G-0.10-Plan ab Abschnitt 13.6.
Reproduzierbarer Testmodus
`--performance-audit-only <Modellordner> <Nachweisordner>` im vorhandenen
Grafiktestprogramm. Je Modell drei Laeufe, 0,5 s Simulationszeit,
dt=0,01 s, 50 Schritte, Testantrieb 200 N, keine Echtzeitbremse.
Core-Rechnung ohne laufende GUI gemessen, danach acht echte
Zustandsdarstellungen je Wiederholung mit einer freien und drei
kleinen Ansichten. Angaben sind Mediane der drei Wiederholungen,
keine garantierten Grenzwerte:

| Aufwand | 420er | Folkeboot |
|---|---:|---:|
| Core fuer 0,5 s Simulation, ohne GUI | 1243,5 ms | 16,8 ms |
| Einzelne freie Ansicht je Snapshot | 111,2 ms | 8,2 ms |
| Zusaetzliche drei Ansichten je Snapshot | 266,1 ms | 43,1 ms |
| Unveraendertes Profil erneut allen vier Ansichten zuweisen | 34,3 ms | 28,6 ms |
| Vier Ansichten plus eine Profilzuweisung je Snapshot | 410,9 ms | 80,6 ms |

Die gesamte Vieransicht-Darstellung kostet in diesem kontrollierten
Vergleich etwa 3,7-mal beziehungsweise 9,9-mal die einzelne freie
Ansicht. Das ist ein aktueller Komponentenvergleich, **kein gemessener
Vorher-nachhervergleich zweier historischer EXEs**. JSON-Erzeugung fuer
acht Snapshots zusaetzlich etwa 111 ms/8,8 ms. Tabellenaktualisierung,
GUI-Livekonkurrenz, abweichende Fenstergroessen/DPI und benutzerspezifische
Profile sind nicht in diesen Einzelzeiten enthalten.

Historische Codebefunde vor Abschluss der Mindestintegration
(nicht als aktuelle Behauptung unveraenderter Core-Quellen verstehen):

- Die juengste Layoutarbeit hat Core und GUI-Core-Adapter nicht geaendert
  (Git-Diffs gegen den damaligen HEAD leer). Neue GUI-Arbeit vervielfacht
  dagegen Zustandstransformation und synchrones Rendering.
- `displayState` aktualisiert jetzt vier Szenen. Jede `setState`-
  Ausfuehrung rendert synchron und verarbeitet Geometrie/Wasserlinie.
- `updateLiveProgress` und `displayState` weisen Profile erneut zu.
  Fuer freie Ansicht existiert ein Gleichheitsabbruch; der gebundene
  Profilzweig fuehrt auch bei unveraenderten Werten erneut Styling und
  Render aus. Die Messung enthaelt nur eine solche Profilrunde.
- Verborgene rechte Ansichten werden aktuell weiterhin mit Zustaenden
  und Profilen versorgt; Ausblenden beseitigt diesen Aufwand nicht.
- Sowohl neue Tabelle als auch verborgene alte Werteansicht werden
  aktualisiert. Deren Anteil ist in obiger Messung nicht isoliert.
- Beim 420er benoetigt schon die isolierte Core-Rechnung etwa 2,5 s
  Rechenzeit pro simulierter Sekunde. Eine reine GUI-Optimierung kann
  fuer diesen Fall keine Echtzeit garantieren.

Fuer die spaetere Implementierung nach Freigabe: unveraenderte
Profilzuweisungen ohne erneutes Styling/Render, keine unnoetige Arbeit
an verborgenen Ansichten/Legacy-Tabelle, Renderanforderungen je sichtbarem
Snapshot zusammenfassen. Dabei keine Core-Schritte auslassen,
keine Schrittweite vergroessern, Physik vereinfachen oder Resultate
verlieren; Reduktion der Anzeige darf nur Zwischenbilder betreffen.
Mit denselben Modellen/Schritten/Profileinstellungen vor/nach messen
und Ergebnisse numerisch vergleichen. Keine Beschleunigung ohne Beleg
behaupten. Dieser Audit aendert noch keine Laufzeitimplementierung.

### 13.6 Historischer Vergleich erst nach funktionaler Integration

Nutzerauftrag vom 03.11.2026, 18:37 Uhr und anschliessender Planauftrag:
zuerst gemeinsame Rechenbedienung und minimal erforderlichen Core-Vertrag
umsetzen, dann integriert testen und die Performance gegen aeltere
GUI-/Core-Versionen bis etwa eine Woche zurueck untersuchen.

Reine Core-Schrittzeit, JSON-/Kraftausgabe, Werteaufbereitung und
GUI-Rendering getrennt erfassen. Identische Eingaben, Schrittzahlen
und Ergebnisse bei vergleichbarem Physikumfang sicherstellen.
Fenstergroesse/DPI, Renderprofil, Kamera und sichtbare Ansichten
dokumentieren; Einzelansicht, Vieransicht und ausgeblendete
Zusatzansichten getrennt messen. Historische Inkompatibilitaeten
sichtbar benennen statt ungleiche Faelle als Regression auszugeben.
Ursachen mit Versionsdiffs und reproduzierbaren Rohmessungen belegen.
Keine vorweggenommene Performancezusage und keine Optimierung durch
groessere Zeitschritte, ausgelassene Core-Schritte oder reduzierte
Physik. Das genaue Vorgehen und die historischen Kandidaten stehen
im einzigen GUI-Plan, Abschnitt 13.4, sowie im Core-Gegenplan.

### 13.7 Freigegebene vorrangige Performance-Ursachenanalyse

Nutzerauftrag 03.11.2026, 19:21 Uhr und ausdrueckliches Go 19:23 Uhr:
**Analyse jetzt vor weiterer Umsetzung und Optimierung.**
Dies ersetzt nur fuer die Untersuchung das fruehere Eintrittsgate
persoenlicher GUI-Abnahme/vollstaendiger Physikmatrix. Es nimmt keine
fachliche Abnahme vorweg und autorisiert keine Optimierung.
A-G-00129 und U-G-000139 werden fuer diese Analyse praezisiert;
keine neue Anforderung fuer denselben fachlichen Gegenstand anlegen.

Alle laufzeitrelevanten Aenderungen in der untersuchten Versionsspanne
systematisch inventarisieren: GUI G-0.9 gegen G-0.10, aeltere GUI-Staende
bis etwa 27.10.2026 fuer Ursprungszuordnung; Core bevorzugt C-0.7,
Zwischenstand C-0.8 und C-0.9. Vollstaendigkeit bedeutet nachvollziehbare
Abdeckung der Diffs, keine unbelegbare Zusage, jeden Hardwareeffekt oder
jede denkbare Modelleingabe gemessen zu haben.

Statische und dynamische Pfade getrennt kennzeichnen. Identische
allgemeine Kraft-, Geometrie-, Modellbewegungs-, Massen- und
Traegheitsgrundlagen verlangen; dynamische Beschleunigung, lokale
Rotationsanstroemung, Zusatztraegheit und stationaere Solversuche nicht
verwechseln. Keine Gleichgewichtsbedingung auf beschleunigte Dynamik
uebertragen. Offene Physiknachweise aus 13.5 bleiben sichtbar.

GUI bei ansonsten identischem Modell, Zustand, Kamera, Profil,
Fenstergroesse und DPI mit allen fuenf einfachen Darstellungen
(Gewicht, Auftrieb, Ersatz-SDP, LDP, Richtungspfeil) gemaess Default
und mit allen fuenf ausgeschaltet untersuchen. Vollstaendige
Kraftdarstellungen bleiben in beiden Faellen ausgeschaltet.
Abschalten betrifft ausschliesslich die Darstellung, nicht den Core.
Nicht bloss Checkboxkosten messen, sondern Zustandsdarstellung,
Geometrie, Tabelle und Rendering; auch vier/eine/verborgene Ansichten.

Verbindliches Ausgabeformat im einzigen GUI-Plan:
Tabelle mit Aenderung/Befund, GUI/Core, Statisch/Dynamisch,
Quelle/Version, gemessenen Mehrkosten oder explizit fehlender Messung,
potenziellem Einsparpotenzial und physikalischem Risiko.
Messung, Codebefund und Hypothese strikt unterscheiden.
Einsparpotenziale ohne Optimierungsversuch qualitativ bewerten;
keine additive Gesamtersparnis aus ueberlappenden Teilzeiten ableiten.
Auch statische Optimierungsmoeglichkeiten auffuehren.

Ein Warmlauf plus fuenf Wiederholungen, Rohwerte/Median/Min/Max,
eingefrorene identische Snapshots und Modellidentitaet dokumentieren.
Historische Rendervergleiche mit heutigem Core sind explizite
Komponentenvergleiche, keine historischen Gesamt-EXE-Benchmarks.
Modellkomplexitaet, Ableitungen, Cache-Invalidierung, Wiederholungen,
JSON/Parser, Tabellen, Render-/Profilrunden und Livekonkurrenz trennen.
Produktionscode und Standard-EXE nicht aendern, keine Git-Aktionen.
Isolierte Messprogramme ausserhalb des Repositorys sind Nachweise,
keine Optimierung. Ergebnisse vorlegen und danach erneut stoppen.

**Nachweisstand:** Vollstaendiges GUI-Dateiinventar mit Ursachen und
Risiken im [Plan, Abschnitt 13.7](../Umsetzungsplaene/Umsetzungsplan_G-0.10.md#137-vollstaendiges-gui-diff-inventar-und-ursachenbefunde).
Die beendete Ein/Aus-/Ansichten-/Workspace-Messmatrix samt historischem
Komponentenvergleich und Rohwerten steht im
[Plan, Abschnitt 13.8](../Umsetzungsplaene/Umsetzungsplan_G-0.10.md#138-gui-messnachweis-g-010-und-g-09).
Keine Produktionsoptimierung umgesetzt. Die dort benannten Messgrenzen
und die offene fachliche Gesamtmatrix bleiben verbindlich sichtbar.
