# Spezifikation: GUI-Windbedienung und Darstellung

**Stand:** 13.11.2026 · **Status:** Fachlich freigegeben; Umsetzung, Tests
und Sichtabnahme noch offen. Diese Datei ist die GUI-Spezifikation für
Bedienung und Darstellung des Windes. Die
[GUI-Entwurfspunkteliste](Entwurfspunkte/GUI_Winddarstellung.json) enthält
die Herkunft der Anforderungen; der
[Umsetzungsplan G-0.23](../Umsetzungsplaene/Umsetzungsplan_G-0.23.md)
führt die Arbeiten; die Durchführung wurde ausdrücklich beauftragt.

Fachliche Quelle für Windfeld, Koordinaten, Profil, scheinbaren Wind und
Core–GUI-Vertrag ist die
[Core-Windspezifikation](../../CORE/Spezifikationen/Spezifikation_Windmodell.md).
Für die gemeinsame Datenübergabe und die Nutzung bereits dargestellter
Punkte gelten die [Core-Referenzpunkt-Spezifikation](../../CORE/Spezifikationen/Spezifikation_Referenzpunkte.md)
und die [GUI-Referenzpunkt-Spezifikation](Spezifikation_GUI_Referenzpunkte.md).
Die GUI dupliziert keine Windphysik und interpretiert die Eingabedateien des
Modells nicht als eigene Windquelle.

## Anforderungszuordnung

Alle Anforderungen sind zur Umsetzung offen; ihre Abnahmefälle stehen in
Abschnitt 7.

| Kennung | Fachlicher Vertrag und Abnahmekriterium |
|---|---|
| A-G-00157 | Windeingaben, Defaults, Begrenzung, Fehlermeldung und eine Nachkommastelle gemäß Abschnitten 1–2; gerundete Anzeige verändert den internen Wert nicht. |
| A-G-00158 | Beide gleich geformten, gemeinsam schaltbaren Mastspitzenpfeile mit Farben, Legende, Richtung und Längenmaßstab gemäß Abschnitt 3. |
| A-G-00159 | Kompassmarker, Herkunftsrichtung, Überlagerung und getrennte Wind-Anzeigeschwellen gemäß Abschnitt 4. |
| A-G-00160 | Zwei gleich breite Geschwindigkeits-/Profilfelder, Core-Profil, Achsenskalierung und Verhalten ohne Mast gemäß Abschnitt 5. |
| A-G-00161 | Konsistenter Core-Datenbezug für Live- und historische Zustände gemäß Abschnitten 2 und 6; keine GUI-eigene Windphysik. |
| A-G-00162 | Mastspitze und Fehlerfälle gemäß Abschnitten 3 und 6: fehlende oder mehrdeutige Daten sichtbar behandeln und keinen Ersatzpunkt erfinden. |

## 1. Umfang und Datenfluss

```text
GUI: Windvorgaben / Modus
         │
         ▼
Core: Validierung, Welt-Windfeld, Profil, lokaler scheinbarer Wind
         │
         ▼
Core-Ergebnis: Vektoren, Werte, Bezugspunkte, Status, Zeitpunkt
         │
         ├──► 3D-Ansicht: zwei Pfeile an der Mastspitze
         ├──► Kompass: zwei Windrichtungsmarker
         └──► Werte und Windprofil-Diagramm
```

Die GUI verwendet gültige Defaultwerte und prüft Eingaben bei der Eingabe
und vor Simulationsbeginn. Bei ungültiger Zahl bleibt der bisherige
gültige Wert erhalten; bei gültigen Zahlen außerhalb der zulässigen
Grenzen wird auf den Mindest- beziehungsweise Höchstwert begrenzt. Die
Korrektur oder Ablehnung ist sichtbar mitzuteilen. Die Berechnung startet
nicht mit ungültigen Werten; ein direkter Core-Aufruf mit ungültigen Werten
liefert einen Fehler. Es wird kein fehlender SDP erfunden. Eingabe,
bestätigter Rechenstand und angezeigtes Ergebnis müssen unterscheidbar
bleiben.

Die Referenzgeschwindigkeit `v0` wird auf `0,01–100 m/s`, die
Referenzhöhe `h0` auf `0,01–100 m` begrenzt. Die Obergrenze für `v0`
begrenzt keine lokal berechneten Profilwerte. Bei einer Begrenzung zeigt
die GUI den korrigierten Wert und einen kurzen Hinweis am Feld; eine
ungültige Zahl wird am Feld kenntlich gemacht.

Windgeschwindigkeiten werden mit **einer Nachkommastelle** angezeigt:
Der intern bestätigte Mindestwert `0,01 m/s` erscheint als `0,0 m/s`.
Die Rundung ist ausschließlich Darstellung und ändert weder die
Berechnung noch den gespeicherten Wert. Auch wenn ein Eingabefeld den
gerundeten Wert `0,0` zeigt, bleibt ohne erneute Eingabe der bestätigte
Wert `0,01 m/s` erhalten. Eingaben müssen dennoch Werte wie `0,01`
annehmen; bei Eingabe von `0` wird `0,01` übernommen und als `0,0`
angezeigt. Der Hinweis auf die Begrenzung bleibt dabei erkennbar.

## 2. Bedienung und Einheiten

| Bedienung | Festlegung |
|---|---|
| Wahrer Wind | Richtung, Referenzhöhe `h0` in m und Referenzgeschwindigkeit `v0` in m/s vorgeben. Die Windrichtung wird als Weltkompasswinkel angegeben. |
| Statik | Richtung vor der Berechnung festlegen; bei jeder Auswahl der Statik ist der Modus bootmitgeführt gesetzt und nicht änderbar. |
| Dynamik | Ein Umschalter: Richtung mit Bootsausrichtung mitführen oder im Weltkoordinatensystem konstant halten. Kein zusätzlicher Testmodus. |
| Bestätigung | Core bestätigt gültige Einstellungen oder liefert eine sichtbare Diagnose; ungültige Eingaben dürfen nicht als angenommen erscheinen. |

Die Windeingaben stehen in einem eigenen Feld **unter Fremdkraft**. Die
Fremdkraft soll später durch diese Eingaben ersetzt werden; der Zeitpunkt
und die tatsächliche Ablösung sind hier nicht festgelegt. Voreinstellungen:
Wind aus Nord `0°`, `v0 = 5,0 m/s`, `h0 = 10 m`; der dynamische
Richtungsmodus beginnt **weltfest**. Richtung und Geschwindigkeit dürfen
während der Dynamik geändert werden; bestätigte Werte gelten ab dem
Änderungszeitpunkt und ändern frühere Ergebnisse nicht rückwirkend. Die
Referenzhöhe kann nur vor einem neuen Start geändert werden. Änderungen
anderer Parameter sind nicht Bestandteil dieses Vertrags.

Beim Umschalten während der Dynamik bleibt die aktuelle Weltrichtung
zunächst erhalten; danach gilt das neue Verhalten.

Windrichtungsachse: N 0°, O 90°, S 180°, W 270°. TWD/AWD bezeichnen als
**Zahl die Herkunft**: 0° kommt aus Norden, der 3D-Pfeil zeigt nach Süden;
90° kommt aus Osten, der Pfeil zeigt nach Westen. Text, Kompassmarker und
Pfeile müssen eindeutig zusammenpassen. Eine Transformation in ein anderes
Koordinatensystem geschieht nur bei Bedarf und mit explizitem Bezug.

## 3. Windpfeile in der 3D-Simulationsansicht

An der Mastspitze werden zwei voneinander unterscheidbare Pfeile
dargestellt:

| Pfeil | Inhalt | Ort |
|---|---|---|
| Wahrer Wind | Core-Windvektor auf Höhe der Mastspitze; blau | An der aktuellen Mastspitze |
| Scheinbarer Wind | Core-Ergebnis lokal an der Mastspitze; orange | Ebenfalls an der aktuellen Mastspitze |

Eine kleine Legende erklärt die Farben: Blau steht für wahren Wind,
Orange für scheinbaren Wind. Beide Pfeile haben dieselbe Form und lassen
sich gemeinsam ein- und ausschalten; die geschwindigkeitsabhängige
Ausblendung gilt unabhängig von diesem Schalter für jeden Pfeil einzeln.

Die Pfeilspitzen zeigen **in Strömungsrichtung**, also dorthin, wohin
der Wind geht. Der wahre Wind bleibt parallel zur Wasseroberfläche, auch
wenn das Boot krängt oder stampft. Die Position der Mastspitze folgt der
Pose, die Richtung des wahren Windes aber nicht der vollständigen
Bootsrotation. Der scheinbare Wind kann aus lokaler Bewegung eine
vertikale Komponente haben; der 3D-Pfeil zeigt den räumlich korrekten
Vektor. Die Pfeillänge zeigt die jeweilige Windgeschwindigkeit; beide
Pfeile verwenden denselben linearen Maßstab: `5 m/s` entsprechen `0,5 m`
Pfeillänge in der 3D-Szene. Die Berechnung verwendet den ungerundeten
Geschwindigkeitsbetrag; die Anzeigeschwelle gilt weiterhin.

Der Mastspitzenpfeil zeigt den lokal an der Mastspitze berechneten Wert.
Eine separate SDP-Windanzeige folgt erst später, wenn ein echter SDP
verfügbar ist. Ist ein benötigter Punkt nicht verfügbar, keinen
erfundenen Ersatz verwenden.

Die Mastspitze kommt aus der gemeinsamen Core-Referenzpunktsammlung;
ohne Mast gibt es keinen
Mastspitzenpfeil. Bei mehreren Masten wird der eindeutig als Hauptmast
bezeichnete Mast verwendet; fehlt eine eindeutige Bezeichnung, wird ein
sichtbarer Hinweis ausgegeben statt einen Mast zu erraten. Für das
Ausblenden bei geringer Geschwindigkeit gilt die Schwelle in Abschnitt 4.

## 4. Kompass

Der bestehende Kompass zeigt weiterhin die Bootsausrichtung. Das
Zeigerelement verwendet für den scheinbaren Wind dessen horizontale
Komponente; die 3D-Darstellung verwendet den vollständigen Vektor.
Zusätzlich
werden zwei schmale, gleichschenklige Winddreiecke am Kompassumfang
geführt. Die Spitze zeigt nach innen, also von der **Herkunftsseite**
der Luft zum Kompasszentrum.

| Marker | Aussehen |
|---|---|
| Wahrer Wind | Größeres Dreieck, nicht ausgefüllt, mit Rahmen |
| Scheinbarer Wind | Kleineres, ausgefülltes Dreieck |

Der Nutzer hat bestätigt: Das kleinere ausgefüllte Dreieck zeigt den
scheinbaren Wind. Beide Dreiecke sind schmal und **gleichschenklig**.

Bei identischer Windrichtung liegt das kleinere Dreieck mittig innerhalb
des größeren; zwischen Füllung und Rahmen bleiben mindestens 1–2 sichtbare
Pixel Abstand. Unterschiedliche Richtungen positionieren die Marker
separat auf dem Umfang. Falls ein Richtungsergebnis nicht bestimmbar ist,
wird kein erfundener Marker angezeigt. Kompassmarkierung und
3D-Strömungspfeil sind geometrisch konsistent: Ein Luftstrom nach Süden
kommt aus Norden; der Nordmarker zeigt nach innen.

Bei Windgeschwindigkeit bis einschließlich `0,1 m/s` wird die
entsprechende Richtung ausgeblendet. Wahrer und scheinbarer Wind werden
getrennt beurteilt: Für die Kompassmarker zählt jeweils der horizontale,
für die 3D-Pfeile der räumliche Betrag. Ein bei geringer horizontaler
Geschwindigkeit ausgeblendeter scheinbarer Kompassmarker schließt deshalb
einen ausreichend großen räumlichen 3D-Pfeil nicht aus.

Der Abstand muss bei jeder DPI-Skalierung mindestens 1–2 sichtbar getrennte
Bildschirmpixel betragen. Die Lesbarkeit bei nahe beieinanderliegenden,
aber nicht identischen Richtungen ist noch zu prüfen.

## 5. Geschwindigkeitsfeld und Windprofil

Das bestehende Element **Geschwindigkeit** wird horizontal in zwei gleich
breite Felder aufgeteilt. Die Lücke zwischen den Feldern wird von der
ursprünglichen Gesamtbreite abgezogen:

```text
Feldbreite = (ursprüngliche Gesamtbreite − Lückenbreite) / 2
┌─────────────────────┐  ┌─────────────────────┐
│ GESCHWINDIGKEIT     │  │ WINDPROFIL          │
│ 1,9 m/s             │  │ Höhe ↑              │
│ +4,8 °/s            │  │      │    ╱         │
│ Welt-X      −1,9 m/s│  │      │  ╱  ● h0/v0 │
│ Welt-Y       0,3 m/s│  │      └───────→ TWS  │
│ Welt-Z       0,0 m/s│  │   m           m/s   │
└─────────────────────┘  └─────────────────────┘
```

Die Werte im bestehenden linken Feld rücken nach links; sie erhalten
jeweils **eine Nachkommastelle**, einschließlich Winkelgeschwindigkeit
und Weltkomponenten. Die Winkelgeschwindigkeit steht rechtsbündig neben
dem Geschwindigkeitsbetrag; die drei Weltkomponenten nutzen den
verbleibenden Platz mit gut lesbarer Schrift. Vorzeichen und Einheiten bleiben erkennbar. Das neue
rechte Feld zeigt ein ansprechendes Diagramm im bestehenden GUI-Design:
Höhe über Wasser in m auf der vertikalen Achse, wahre
Windgeschwindigkeit in m/s auf der horizontalen Achse. Das
Core-Höhenprofil wird dargestellt; der Referenzpunkt `h0/v0` ist
erkennbar. Die schematische Skizze ist keine Festlegung von Kurvenform,
Achsenbereich oder Design.

Die GUI verwendet das bestätigte Core-Profil bzw. Core-Auswertungen und
keine unabhängig gepflegte zweite Formel. Sie extrapoliert nicht
stillschweigend außerhalb des gültigen Bereichs. Die Achsen werden
automatisch passend skaliert. Der obere Höhenwert ist der nächste
5-m-Schritt oberhalb der Mastspitze: 2,4 m → 5 m, 8 m → 10 m,
10,5 m → 15 m, bei exakt 10 m → 15 m. Die Kurve beginnt bei 0,5 m Höhe;
die Höhenachse beginnt weiterhin bei 0 m. Die Geschwindigkeitsskala endet
beim passenden aufgerundeten Zehnerschritt in m/s, mindestens bei 10 m/s.
Bei einem Modell ohne Mast wird das Diagramm ausgeblendet.

Liegt `h0` außerhalb des gewählten Diagrammbereichs, kann der
Referenzpunkt dort nicht als Punkt in der Kurve erscheinen; `h0/v0`
bleiben stattdessen im Tooltip als Referenzwerte kenntlich; dieser nennt auch
den Beginn der Kurve bei 0,5 m Höhe. Der Platz unter
der Überschrift steht dem Diagramm zur Verfügung. Stützstellen und konkrete
Mindestgrößen werden bei der Umsetzung so gewählt, dass der Profilverlauf
und die Beschriftung auch bei schmalen Fenstern und High DPI lesbar sind.
Bei ungültigem Profil wird ein Fehler angezeigt statt eines
leeren, scheinbar gültigen Diagramms.

## 6. Zustand und Konsistenz

- Wahre und scheinbare Windangaben kommen aus dem Core und sind inhaltlich
  durch Beschriftung/Legende unterscheidbar.
- 3D-Pfeile, Kompass und Werte verwenden denselben aktuellen
  Simulationsstand; explizit verschiedene Bezugspunkte bleiben erkennbar.
- Bei Wiedergabe werden zum dargestellten Ergebniszeitpunkt passende
  Winddaten und Bootszustände verwendet, nicht aktuelle Livewerte.
- Windstillstand, unbestimmbare Richtung, fehlende Mastspitze oder
  fehlender SDP werden explizit behandelt; weder Nullwerte noch eine
  Ersatzposition dürfen einen Fehler kaschieren.
- Die bestehende Bootsrichtung und die übrigen Geschwindigkeitsdaten
  bleiben unabhängig vom neuen Windvertrag erhalten.

## 7. Abnahmefälle

Diese Fälle sind verbindlich geplant, aber noch keine ausgeführten Tests:

| Bereich | Prüffall |
|---|---|
| Bedienung | Statik-Parameter und beide Dynamikmodi gelangen korrekt zum Core. Ungültige Zahlen behalten den alten Wert und werden kenntlich gemacht; gültige Zahlen außerhalb der Referenzgrenzen werden mit Hinweis begrenzt. |
| Eingabe und Anzeige | `v0 = 0` wird als `0,01 m/s` übernommen und als `0,0 m/s` angezeigt; bloße Anzeige oder erneutes Öffnen ändert den bestätigten Wert nicht. `h0` wird auf `0,01–100 m` begrenzt. Lokale Profilwerte dürfen `100 m/s` überschreiten. |
| 3D | Gleich geformte blaue wahre und orangefarbene scheinbare Pfeile an der posierten Mastspitze, mit erklärender Legende; `5 m/s` ergeben für beide `0,5 m` Pfeillänge, `10 m/s` ergeben `1 m`. Pfeilspitzen folgen dem jeweiligen Strömungsvektor. Wahrer Wind bleibt bei Krängung wasserhorizontal. |
| Pfeilschalter | Beide Pfeile gemeinsam ausschalten und wieder einschalten; die geschwindigkeitsabhängige Ausblendung wirkt für jeden Pfeil weiterhin getrennt. |
| Bezugspunkt | Der Mastspitzenpfeil verwendet den lokalen Wert; mögliche SDP-Werte werden niemals damit verwechselt. |
| Kompass | Nord/Ost/Süd/West und 0°/360°; Herkunftsseite und nach innen zeigende Spitze stimmen zum Strömungspfeil. |
| Kompassüberlagerung | Bei gleicher Richtung kleiner Marker vollständig im größeren, 1–2 Pixel Mindestabstand; bei unterschiedlichen Richtungen separat und lesbar, einschließlich High DPI. |
| Wind-Anzeigeschwelle | Beide Windarten separat bei `0,1 m/s` ausblenden und knapp darüber anzeigen; Kompass anhand des horizontalen, 3D-Pfeil anhand des räumlichen Betrags prüfen. |
| Layout | Beide Felder gleich breit nach Abzug der Lücke; Werte links, alle Geschwindigkeitsanzeigen des Feldes mit einer Nachkommastelle; keine Überlappung oder abgeschnittene Einheit. |
| Profil | Achsen und Referenzpunkt stimmen mit dem bestätigten Core-Profil; bei ungültigen oder fehlenden Daten sichtbare Diagnose statt erfundener Kurve. |
| Synchronität | Live und Wiedergabe zeigen Pose, Marker, Vektoren und Profil des gleichen Zustands. |

## 8. Abgrenzung und noch ausstehende Nachweise

Die spätere Ablösung der Fremdkraft und eine SDP-Windanzeige sind nicht
Bestandteil dieser Freigabe. Umsetzung, Core-Integration, Tests und
persönliche Sichtabnahme sind offen. Technische Details, insbesondere
Diagrammabtastung und konkrete Diagnoseformulierungen, dürfen den oben
festgelegten fachlichen Vertrag nicht abschwächen.
