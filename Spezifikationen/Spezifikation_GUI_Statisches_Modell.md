# Spezifikation GUI: Statisches Modell und stationaere Fahrt

**Status:** Finalisiert durch Nutzerfreigabe am 02.11.2026; Umsetzung beauftragt, Nachweise im Umsetzungsplan.
**Stand:** 02.11.2026.

Abschnitt 6 wurde am 02.11.2026 ausdruecklich finalisiert und zur Umsetzung
freigegeben. Abschnitte 1-5 und deren Anforderungskennungen bleiben fuer
den bisherigen geradlinigen Modus gueltig; Abschnitt 6 erweitert diesen
um einen expliziten Kurvenfahrtmodus im neuen Vertrag V6.

Maessgebliche Entwurfspunkte:
[GUI_Statisches_Modell.json](Entwurfspunkte/GUI_Statisches_Modell.json).
Physikalischer Vertrag:
[Core: Stationaeres Fahrtgleichgewicht](../../CORE/Spezifikationen/Spezifikation_Stationaeres_Fahrtgleichgewicht.md).
Zielversion: G-0.9. Verbindlicher
[Umsetzungsplan](../Umsetzungsplaene/Umsetzungsplan_G-0.9.md).
Der veroeffentlichte Tag G-0.8 bleibt unveraendert.

## 1. Ziel und vorhandener Stand

Im Reiter Statisches Modell sollen konstante Fremdkraefte und konstante
Ruder-/Schwertstellungen vorgegeben werden. Core bestimmt daraus Schwimmlage,
Orientierung und stationaere Geschwindigkeit, nicht die vorgegebenen Lasten
oder Stellungen. Jede geaenderte Konfiguration benoetigt einen neuen Lauf.

Bereits umgesetzt, aber noch uncommittet: aktiver Modellname rechtsbuendig
auf Hoehe der Reiter in Titelgroesse; vollstaendiger Darstellungsblock links
anstelle der Modellauswahl. Diese Layoutaenderung ist kein Nachweis fuer einen
neuen stationaeren Solver.

Ausgangslage vor dem Umsetzungsauftrag am 02.11.2026:
- `Code/src/main_window.cpp`: `modeCombo_` bestimmt noch statisch/dynamisch,
  unabhaengig vom Arbeitsbereich. Testantrieb und Bewegungseingaben werden
  bislang nur im dynamischen Modus sichtbar.
- `Code/src/core_adapter.cpp`, `runSimulation`: uebergibt den bisherigen
  statischen Laufvertrag, keinen allgemeinen Fremdkraftfall.
- Core berechnet bislang nur hydrostatisches Z-/Roll-/Stampfgleichgewicht
  und setzt die Geschwindigkeit auf null. Hydrodynamik im statischen
  Lauf wird ausdruecklich abgewiesen.

## 2. A-G-00079: Reitergebundener Laufmodus

Im statischen Reiter entfaellt die Auswahl statisch/dynamisch. Ein dort
gestarteter Lauf verwendet immer den expliziten statischen beziehungsweise
stationaeren Core-Vertrag, niemals eine versteckte alte Dropdown-Einstellung.
Der dynamische Reiter bleibt als eigener Berechnungsweg erhalten.
Ein Reiterwechsel startet keinen Lauf.

**Freigegebene Verhaltensaenderung vom 02.11.2026, 18:30 Uhr:**
Beim Wechsel zwischen Bootsauswahl, Statisches Modell und Dynamisches Modell
wird der laufende Auftrag abgebrochen. Ergebnisse, Zeitauswahl, Fahrthistorie
und Richtungspfeil werden sofort geloescht; verspaetete Ergebnisse duerfen
nicht wieder erscheinen, auch nicht bei Rueckkehr in denselben Reiter.
Der naechste Lauf startet aus den Modellinitialwerten, nicht aus einem
Ergebnis des anderen Reiters. Bestehende Kraft- und feste Stellungseingaben
bleiben konfiguriert. Die Kamera folgt zurueck in die Ausgangslage des
Modells, unter Erhalt des bootsrelativen Blickwinkels.
Bis der abgebrochene Worker beendet ist, bleibt ein Neustart gesperrt.
Diese Freigabe ersetzt die bisherige Vorgabe, laufende Auftraege bei
Reiterwechsel unveraendert fortzusetzen; kein automatischer Neustart.

Die Unterscheidung hydrostatische Ruhe und stationaere Fahrt muss fachlich
sichtbar bleiben. Bis Core die stationaere Fahrt unterstuetzt, darf die GUI
keine scheinbar erfolgreiche Ersatzberechnung oder Nullgeschwindigkeit als
berechnetes Fahrtergebnis ausgeben.

## 3. A-G-00080: Konfiguration unter den Laufbuttons

Direkt unter Core-Lauf starten / Lauf stoppen: Fremdkrafteinwirkung auswaehlen,
aktivieren und einstellen. Benoetigt werden Betrag/Richtung oder aequivalente
Vektorkomponenten, Angriffspunkt und explizites Bezugssystem, mit eindeutigen
Einheiten N und m. Der temporaere Testantrieb samt Geschwindigkeitsbegrenzer
ist kein Ersatz fuer diese Lastdefinition.

Genau eine Fremdkraft ist aktivierbar. Der Angriffspunkt ist modellfest,
der Kraftvektor wahlweise modell- oder weltfest; diese Auswahl wird sichtbar
mit dem unveraenderlichen Rechenfall gespeichert.

Direkt unter dem Bereich Fremdkrafteinwirkung folgt der Einstellbereich
**Ruder- und Schwertstellung**. Die vertikale Reihenfolge rechts lautet:
Laufbuttons, Fremdkraft, Ruder-/Schwertstellungen. Jedes vorhandene Ruder
und Schwert erhaelt eine eindeutig dem Element zugeordnete Einstellung;
mehrere Elemente bleiben getrennt einstellbar.

Feste Stellungen saemtlicher vom Core angebotenen beweglichen Elemente
einschliesslich unterschiedlicher Ruder und Schwerter sind vor dem Lauf
einstellbar. Identitaeten, Bewegungstypen, Einheiten, Normalwerte und vorhandene
Grenzen kommen aus Core; keine aus Namen geratenen Rollen oder erfundenen
physikalischen Grenzwerte. Fehlende Grenzwerte benoetigen eine explizite
Eingabe-/Validierungsregel.

Vor Start wird ein vollstaendiger unveraenderlicher Rechenfall erstellt.
Waehrend des Laufs sind dessen Eingaben gesperrt; keine Live-Ereignisse oder
Stellungsoptimierung. Abbruch bleibt moeglich. Eine spaetere Eingabeaenderung
markiert das angezeigte Ergebnis als zu einem anderen Eingabestand gehoerig.

## 4. A-G-00081: Ergebnis und Bedienung

Darstellung von Tiefenlage, Rollen, Stampfen, Gieren/Kurs, Fahrtbetrag,
Weltgeschwindigkeitsvektor, Geschwindigkeit relativ zum Wasser und Abdrift
mit eindeutigen Winkelkonventionen. X/Y bleiben Referenzkoordinaten und
werden nicht als ermittelte horizontale Gleichgewichtslage bezeichnet.
Vertikalfahrt und Drehraten sind im geradlinigen stationaeren Fall fest null.

Alle relevanten Einzelkraefte und Momente einschliesslich Fremdkraft,
Hydrodynamik und Summen werden im bestehenden Darstellungs-/Tabellenpfad
ausgewiesen. Vorhandene Auswahl, Skalierung, Kamera und Herkunftsangaben
bleiben erhalten; keine GUI-eigene Parallelphysik.

Alle sechs Residuen, verwendete Toleranzen, Iterationen und Status kommen
aus Core. Kein Gleichgewichtslabel allein aus gerundeten Anzeigewerten.
Nicht eindeutiger Kurs, nicht gefundenes Gleichgewicht, ungueltige Eingaben,
Flutung und Nutzerabbruch sind sichtbar. Ein Ergebnis ist weder automatisch
dynamisch stabil noch global eindeutig. Vorschau-/Iterationszustaende
duerfen nicht als physikalischer Zeitverlauf erscheinen.

Bei physikalisch unbestimmtem absolutem Kurs bleibt der Startkurs als
sichtbar gekennzeichnete Referenz erhalten. Er darf nicht als berechneter
Gierwinkel ausgegeben werden; die vollstaendige Momentenbilanz bleibt Pflicht.

## 5. A-G-00082: Abnahme und Integration

- Modus nach Wechsel dynamisch -> statisch ist garantiert stationaer;
  alter Dropdown-Wert hat keinen Einfluss.
- Fremdkrafteingaben befinden sich unter Lauf stoppen und werden mit
  Einheiten/Bezugssystem unveraendert an Core uebergeben.
- Unterschiedliche feste Elementstellungen kommen im eingefrorenen
  Rechenfall an und bleiben waehrend der Iterationen konstant.
- Ergebnis und Kraftauswahl entsprechen den Core-Werten, auch fuer
  Abbruch, fehlende Loesung und unbestimmten Gierwinkel.
- Layout und Bedienbarkeit bei 100/150/200 Prozent sowie Regression der
  Bootsauswahl und dynamischen Steuerung pruefen.

Core-Abnahmeobergrenzen sind 0,1 N je Kraftkomponente, 0,1 Nm je
Momentkomponente und 0,1 m/s im analytischen Geschwindigkeitsvergleich.
Hohe interne Genauigkeit wird angestrebt; die GUI zeigt tatsaechliche
Core-Residuen und Toleranzen und verspricht keine unbelegte Genauigkeit.
Suchgrenzen und Solverstrategie werden im Core-Plan vor Implementierung
konkretisiert. Es werden hier weder neue Physik implementiert noch
Git-Aktionen ausgeloest.

## 6. Stationaere Kurvenfahrt und Drehratenanzeige

**Status:** Finalisiert und Umsetzung beauftragt am 02.11.2026.
Herkunft in der bestehenden
[Liste](Entwurfspunkte/GUI_Statisches_Modell.json).
Physikalisch massgeblich ist
[Core-Spezifikation Abschnitt 7](../../CORE/Spezifikationen/Spezifikation_Stationaeres_Fahrtgleichgewicht.md).
Zielversionen: GUI G-0.9 und Core C-0.8.

### 6.1 A-G-00083: Fachliche Bedeutung und Bedienung

Der statische Reiter muss sowohl geradlinige stationaere Fahrt
als auch stationaere Kurvenfahrt darstellen: bootsfeste wasserrelative
Geschwindigkeit, Roll-/Stampflage und Kursdrehrate bleiben konstant.
Konstanter Fahrtbetrag allein und konstantes unausgeglichenes Drehmoment
sind keine Erfolgskriterien. Geradeausfahrt bleibt der Sonderfall mit
Drehrate null; bei Kurvenfahrt aendert sich der absolute Kurs weiter.

Weiterhin genau eine Fremdkraft mit X/Y/Z-Komponenten und einem
gemeinsamen modellfesten Ansatzpunkt X/Y/Z; Kraftbezug bleibt sichtbar.
Ruder-/Schwertstellungen werden fest vorgegeben, pro Lauf eingefroren
und niemals zur Konvergenzerzeugung heimlich angepasst.
Core entscheidet, ob insbesondere eine weltfeste Last mit dem gesuchten
stationaeren Drehzustand vereinbar ist. Keine GUI-eigene Korrektur,
Kreisfahrt aus geschwindigkeitsgerundeten Anzeigen oder automatische
Umschaltung auf einen dynamischen Lauf.

### 6.2 A-G-00084: Reihenfolge der Geschwindigkeitsanzeige

In der vorhandenen Geschwindigkeitskarte des Navigations-Overlays:

1. Fahrtbetrag in **m/s**, bestehende Bedeutung erhalten:
   Betrag der Weltgeschwindigkeit des Gesamtschwerpunkts.
2. **Kursdrehrate** als vorzeichenbehafteter Wert in **Grad/s**
   (sichtbares Einheitenzeichen `°/s`, gleichbedeutend mit Nutzerangabe
   `°/sec`), unmittelbar unter dem Fahrtbetrag.
3. Bestehende Achsengeschwindigkeiten **Welt-X, Welt-Y, Welt-Z** in m/s.

Die neue Zeile steht somit ueber allen Achsengeschwindigkeiten.
Tooltip: Drehung um die Weltvertikale, positive Richtung nach
Rechte-Hand-Regel; positiver Wert gegen den Uhrzeigersinn beim Blick
von oben entlang negativer Welt-Z-Richtung. Es ist nicht der
Winkelgeschwindigkeitsbetrag und bei Krängung/Stampfen nicht einfach
omega_Modell_Z. Die rotationsinvariante Gesamtwinkelgeschwindigkeit
und ihre Modellkomponenten koennen im Wertebaum getrennt erscheinen.

Core ist Quelle der Kursdrehrate in rad/s und des Status. Die GUI
konvertiert rad/s -> Grad/s; sie differenziert weder gerundete
Kompasswinkel noch Simulations-/Iterationsbilder. Bei dynamischen
Zustaenden denselben fachlichen Wert verwenden; fehlt er im jeweiligen
Altvertrag, explizit als nicht verfuegbar kennzeichnen statt einen
anderen Winkelgeschwindigkeitswert als Kursdrehrate auszugeben.
Eine kuenftige Erweiterung dynamischer Lieferungen ist dabei mit dem
Core-Vertrag abzustimmen, kein stiller V5-Umbau.

Null als numerischen Nullwert zeigen, fehlende/ungueltige Werte als
`-- °/s` mit Grund. Nichtkonvergierte Werte sichtbar als Naeherung
kennzeichnen; nachtraegliche Eingabeaenderungen machen den Ergebnisstand
wie bisher veraltet. Eine Anzeigeaufloesung von zwei Nachkommastellen
fuer Grad/s ist vorgesehen; exakter Core-Wert steht im Tooltip und
Wertebaum. Die Aufloesung garantiert keine physikalische Genauigkeit.
Fahrtbetrag bleibt unabhaengig von seiner Anzeigeaufloesung mit
Abnahmeziel 0,1 m/s bewertet.

### 6.3 A-G-00085: Bilanzanzeige und Vertragsgrenze

GUI uebernimmt Core-Loesungstyp, Status, Toleranzen und alle sechs
**Bewegungsresiduen**. Aeussere Kraft-/Momentensummen bleiben getrennt
sichtbar und als solche beschriftet. Bei Kurvenfahrt darf eine
resultierende Seitenkraft vorhanden sein; entscheidend ist die Bilanz
mit der von Core gelieferten Bewegungsbeschleunigung. Keine
Erfolgspruefung anhand gerundeter Summen, konstantem Fahrtbetrag oder
der kleineren Aenderung zwischen Solveriterationen.

Geradeausfahrt, Kurvenfahrt, reine Drehung, Referenzkurs, Nichtkonvergenz,
inkompatible Last, Abbruch und Flutung sind unterscheidbar. Bei einem
Referenzkurs nicht behaupten, der absolute Kurs sei geloest oder konstant.
Ein optionaler Radius bezeichnet den Kreis relativ zum Wasser; bei
Null-Drehrate kein erfundener endlicher Wert.
Keine zusaetzliche Bahn-/Kreisanimation aus Iterationszustaenden;
numerische Suche bleibt vom physikalischen Zeitverlauf getrennt.

Bis zur expliziten Core-Vertragslieferung bleibt der bisherige
geradlinige Laufvertrag erhalten; alte Ergebnisse nicht als Kurvenloesung
umdeuten. Neue Core-Felder und Versionen vor Implementierung mit dem
Core-Register abgleichen. GUI besitzt keine Parallelphysik und vergibt
keine eigenen Core-CPPI-Kennungen.

### 6.4 A-G-00086: Abnahme und Integration

- Echte Core-Referenzen fuer Geradeaus, positive/negative Kursdrehrate,
  Schraeglage und reine Drehung. Umrechnung pruefen: +/-pi/2 rad/s
  ergibt +/-90 Grad/s. Bei Schraeglage Kursdrehrate nicht mit
  Modell-Z-Komponente oder Betrag von omega verwechseln.
- Nachweis der Reihenfolge Fahrtbetrag -> Kursdrehrate -> Welt-X/Y/Z
  bei 100/150/200 Prozent, einschliesslich vergroesserter/verkleinerter
  Navigationskarte; kein Abschneiden oder Verdecken bestehender Zeilen.
- Genaue Uebernahme der Core-Residuen und Toleranzen; korrektes Ergebnis
  trotz aeusserer Zentripetalkraft, klare Warnung bei fehlender Bilanz.
  Nicht verfuegbar, Null, Fehler, Abbruch, veraltet und Naeherungszustand.
- Altvertrags-, Bootsauswahl-, Dynamik- und Einstellungsregression;
  Kraftframe und feste Elementstellungen nachweisbar unveraendert.
- Freigegeben: Drehratenfehler zur Referenz <=0,01 Grad/s,
  Roll-/Stampfwinkelfehler <=0,1 Grad je Winkel, vertikale
  Schwerpunktgeschwindigkeit <=0,001 m/s, Roll-/Stampfraten
  <=0,01 Grad/s. Fahrtfehler <=0,1 m/s, alle sechs Bewegungsresiduen
  <=0,1 N/Nm. V6 liefert stationaere und dynamische Kursdrehraten;
  V1-V5 bleiben kompatibel und werden nicht umgedeutet.

### 6.5 A-G-00087: Richtungspfeil vor dem Bug

Direkter Nutzerauftrag nach drei Renderentwuerfen, freigegeben am
02.11.2026, 18:30 Uhr. In beiden Simulationsreitern erscheint mit den
Navigationsanzeigen ein mintfarbener Pfeil vor dem Bug, knapp ueber der
Wasserflaeche und parallel zur Welt-XY-Ebene. Ausgangsrichtung ist die
horizontale Bugrichtung, nicht der Geschwindigkeitsvektor.

Quelle ist ausschliesslich die bereits vertraglich gelieferte aktuelle
Core-Kursdrehrate: null ergibt einen geraden Pfeil, positive Rate eine
Linksbiegung/Backbord, negative Rate Rechtsbiegung/Steuerbord.
Die symbolische Gesamtbiegung ist `clamp(4 s * rate, -80 Grad, +80 Grad)`.
Innerhalb der Saettigung nimmt die Biegung stetig mit dem Betrag zu.
Keine Bahnprognose, Kraft, Moment oder neue Gleichgewichtsberechnung.

Laenge 45 Prozent der dargestellten Modellausdehnung entlang Modell-X;
Abstand vor dem Bug 7 Prozent, Hoehe ueber Wasser 1,5 Prozent derselben
Laenge. Die Ansicht bleibt unabhaengig von Kraft-/Momentenfiltern.
Fehlende Core-Rate, singulaere horizontale Bugrichtung, Bootsauswahl,
ausgeblendete Navigation oder geloeschter Zustand blenden den Pfeil aus.
Bei Naeherungsergebnissen bleibt die vorhandene Naeherungskennzeichnung
der numerischen Kursdrehrate massgeblich.

Abnahme: Null, beide Vorzeichen, zunehmende Biegung, Saettigung, andere
Bugrichtung und horizontale Lage; echte Core-Kurvenloesung in der GUI
bei 100/150/200 Prozent sowie Ausblenden und Reiterreset pruefen.
Core-V6 und seine Physik bleiben unveraendert.

## Herkunft

| Entwurfspunkt | Finale Anforderung |
|---|---|
| IDEE-G-00048 | A-G-00079 |
| IDEE-G-00049 | A-G-00080 |
| IDEE-G-00050 | A-G-00081 |
| IDEE-G-00051 | A-G-00082 |
| IDEE-G-00052 | A-G-00083 |
| IDEE-G-00053 | A-G-00084 |
| IDEE-G-00054 | A-G-00085 |
| IDEE-G-00055 | A-G-00086 |
