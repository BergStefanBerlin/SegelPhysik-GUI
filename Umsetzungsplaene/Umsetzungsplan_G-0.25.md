# GUI-Umsetzungsplan G-0.25: sailforce

**Stand:** 17.11.2026 · **Status:** FINAL fachlich geplant auf Nutzerauftrag.
Core-/GUI-Umsetzung inzwischen ausdruecklich beauftragt. Die GUI-Anbindung
an den deklarierten Core-Vertrag ist in Arbeit; gezielte gemeinsame Builds
und Integrationstests liegen vor, Gesamtintegration und Abnahme bleiben offen.
Aktuellster kontrollierter Leistungsbefund und offene Abschlussgates:
Abschnitt 16 (17.11.2026, finaler Leistungsnachweis nach 23:17 Uhr).
Einziger Plan fuer G-0.25. Keine Freigabe fuer
SegelMacher-/Modellkorrekturen oder Git-/Release-Aktion.

Massgeblich sind
[Kraftdarstellung und Bedienung](../Spezifikationen/Spezifikation_GUI_Kraft_und_Momentendarstellung.md#3-sailforce-darstellung-und-bedienung-g-025)
mit A-G-00167 bis A-G-00172 und
[SDP-Darstellung A-G-00166](../Spezifikationen/Spezifikation_GUI_Lateraldruckpunkt.md#6-g-025-aerodynamische-segeldruckpunkte).
Die physikalische Lieferung wird im getrennten
[Core-Plan C-0.25](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.25.md)
geplant. Bestehende Wind-/Punktarbeit verbleibt in
[G-0.23](Umsetzungsplan_G-0.23.md); dort offene Nachweise bleiben offen.

## 1. Bestand und Eingangsgates

Vor Vergabe am 17.11.2026 projektweit geprueft: A-G-00165,
U-G-000177, IDEE-G-00107. Lokales HEAD
`ba04c9950c5ebfe65cbd690eeaa06156b0335515` entsprach GitHub-main;
G-0.24 ist als Tag vorhanden. G-0.25 ist die neue Arbeitszielversion,
kein gesetzter Tag. Dieser Plan vergibt U-G-000178 bis U-G-000184,
die Spezifikationen A-G-00166 bis A-G-00172.

Die GUI verwendet ausschliesslich den echten Core-Vertrag. Keine
Segelphysik, Druckpunktberechnung, aerodynamische Modelldateisuche oder
eigene Streifeneinteilung. Fehlende Lieferungen sind ein sichtbarer
Blocker, kein Anlass zu Attrappen. Unit-Fixtures pruefen Darstellung,
ersetzen aber keine reale Core-Integration.

| Gate | Erforderlich | Aktueller Stand |
|---|---|---|
| Core C1/C2 | Konkreter gepruefter API-/Versionsvertrag, Lasten/Summen um S, Aktivierung, Luftdaten, bestaetigte Stellungsfassungen und unveraenderliche Snapshots | V8 und ErgebnisSnapshot-API geliefert, gezielter GUI-Historien-/Paritaetsnachweis einschliesslich Cacheverdraengung bestanden; gesamte fachliche Abnahme offen |
| Core C3 | Aerodynamische Einzel-/Gesamt-SDPs aus realer Lastverteilung, Status und passende Grenzlinien; Modell-/Quellen-/Paritaetsgate bestanden | Reale Lieferung und Core-Integration inklusive Spiegelung laut Hauptlauf bestanden; GUI-Gesamtabnahme offen |
| Core C4 | Numerik und Core-Regression bestanden, reproduzierbare reale Referenzlaeufe | Gezielte Core-Tests und C25-15-Kontrolllauf gemeldet; keine pauschale Gesamtregressionsfreigabe abgeleitet |
| Gemeinsame Leistung | Referenzhardware und bestaetigter Messrahmen A-G-00172 | G25-08 fuer alle drei Boote bestanden, A-G-00172 erfuellt; finaler voller Leistungsnachweis in Abschnitt 16. G25-09 und technische/persoenliche Gesamtabnahme bleiben offen |

Dieser GUI-Plan beauftragt keine Aenderung in Core, SegelMacher oder
Modellrepository. Vor Umsetzung die tatsaechlich verlinkte Core-Fassung
und deren Lieferstatus pruefen. Unvollstaendige historische C-0.23-/
G-0.23-Nachweise sind nicht automatisch Eingangsgate bestanden.

## 2. Umsetzungsschritte

Die Implementierung der Schritte U-G-000178 bis U-G-000183 ist in Arbeit;
ihre gesamten Fertigkriterien sind noch nicht vollstaendig abgenommen.
U-G-000184: G25-08 ist nach Abschnitt 16 **bestanden**, G25-09 und damit
der gesamte Schritt bleiben **offen**. GUI-eigene Tests koennen im erteilten
Umsetzungsauftrag mit klar bezeichneten Fixtures vorbereitet werden;
die jeweils genannte reale Integration bleibt dennoch erforderlich.

| U-ID | Arbeitspaket | Anforderungen | Abhaengigkeiten | Fertigkriterium |
|---|---|---|---|---|
| U-G-000178 | Tatsachlichen Core-Vertrag, Feature-/Versionsmatrix, Zeit-/Fassungs-/Statusfelder und Worker-/Snapshotpfad pruefen; Adapter gezielt erweitern, fehlende Lieferungen sichtbar melden. Keine GUI-Ersatzphysik. | A-G-00166 bis A-G-00171 | Core C1/C2; fuer SDP/Linien zusaetzlich Core C3 | G1: G25-01 und Adapterteil G25-07 bestanden; verwendete Core-Fassung dokumentiert |
| U-G-000179 | Pro-Segel-Aktivierung und bestaetigte Live-Stellwinkel anbinden; Settings -> Lufteigenschaften mit Validierung, Einheiten, Persistenz und Laufsperre ergaenzen. Fremdkraft unveraendert halten. | A-G-00169, A-G-00170 | U-G-000178, Core C2 | G25-02, G25-03; keine neue Konfiguration mit alten Lasten mischen |
| U-G-000180 | Einzel-/Segel-/Gesamtkraft und Momentenauswahl, Core-Summen, S-Bezug, gekennzeichnete Darstellungsbezugspunkte und Giermomentdetails integrieren. | A-G-00167 | U-G-000178, Core C2 | G25-04; keine lokale Summenphysik oder falschen Momentenboegen |
| U-G-000181 | Gemeinsamen SDP standardmaessig und Einzel-SDPs optional darstellen; Quellen/Status und Fremdkraft-Ersatz getrennt, Persistenz und Detailhinweise anbinden. | A-G-00166 | U-G-000178, Core C3 | G25-05 mit realen Core-SDPs; kein erfundener Marker |
| U-G-000182 | Dezentes Core-Linienoverlay mit eigenem Schalter, passender Geometriefassung und Cache darstellen; ohne Flaechen-/Drahtgitternachbau. | A-G-00168 | U-G-000178, Core C3 | G25-06 mit realer passender Segelgeometrie |
| U-G-000183 | Alle Anzeigen fuer Live, Pause, Wiedergabe und Fehler zustandskonsistent verbinden; spaete/abgebrochene Resultate abweisen, ausstehenden Winkelstand kenntlich machen. Asynchronitaet und GUI-unabhaengigen Core nachweisen. | A-G-00166, A-G-00167, A-G-00169, A-G-00171 | U-G-000179 bis U-G-000182, Core C3 | G25-07; historische Daten unveraendert, UI-Stall taktet keine Physik |
| U-G-000184 | Release-Build, gezielte und vollstaendige Regression, DPI-/Sichtpruefung sowie alle drei integrierten Echtzeitlaeufe durchfuehren. Anforderungsbelege und persoenliche Abnahme getrennt dokumentieren. | A-G-00166 bis A-G-00172 | U-G-000178 bis U-G-000183; Core C4 fuer Abschluss | G25-08/G25-09 und G3/G4; keine ungemessene fluessige Darstellung behaupten |

## 3. Testanforderungen

| Fall | Pruefung und erwartetes Ergebnis |
|---|---|
| G25-01 Vertrag | Tatsaechlich verwendete Core-Fassung und neue Daten/Status pruefen; fehlende/unbekannte neue Pflichtwerte diagnostizieren. Alte Ergebnisse ohne sailforce lesbar, nicht mit aktuellen Livewerten auffuellen. |
| G25-02 Aktivierung/Stellung | Pro Segel Aktivierung, initial an nur bei expliziter Zuordnung, bestaetigte Auswahl erhalten, fehlendes/defektes Modell erklaeren. Fremdkraft bleibt aktiv. Winkelvorzeichen, Nullstellung, schnelle Folge, bestaetigte Schrittgrenze, Pause und Laufende vor Uebernahme pruefen. Aktivierung erst fuer naechsten Lauf. |
| G25-03 Luft-Settings | Settings -> Lufteigenschaften: 1.225 kg/m^3 und 1.5e-5 m^2/s, Einheiten und Praezision, gueltige Aenderung/Persistenz/Neustart; ungueltiger Text, NaN/Inf und Core-Modellbereiche ohne stilles Klemmen. Im Lauf gesperrt; Historie behaelt alte Werte. |
| G25-04 Lastanzeige | Einzel-/Segel-/Gesamtauswahl, Kraft/Moment getrennt und gleichzeitig; echte Core-Summen, alle Momente um S. Abfragepunkt als Darstellungsbezug, reines Moment ohne Kraftpfeil, keine Punktmittelung fuer Summen; Quellenbezogene Giermomente und Zahlenparitaet. |
| G25-05 SDP | Gemeinsamer Marker standardmaessig, Einzelmarker optional; getrennte Schalter und Persistenz. Unbestimmbare Einzel-/Gesamtpunkte unabhaengig behandeln, Aussenpunkte nicht klemmen. LDP und Fremdkraft-Ersatz unveraendert und klar unterschieden. Live/Pose/Historie und Hinweise korrekt. |
| G25-06 Overlay | Echte Core-Grenzlinien auf Dreieck-/Vierecksegel, beide Seiten/Nullstellung, geneigte Lage und Bootspose; kein Z-Fighting, keine Flaechen oder eigene Einteilung. Ein/aus, Cache und Geometrie-/Modellwechsel pruefen. |
| G25-07 Asynchronitaet | Abruf verzoegern/UI voruebergehend blockieren, Core rechnet weiter. Pose, Lasten, SDP/LDP und Linienfassung aus demselben Stand; keine neue Stellgeometrie ueber alten Lasten. Pause/Rueckblick, Abbruch, Modellwechsel, spaete Antwort und Core-Fehler erhalten klare Status. |
| G25-08 Leistung | Ryzen 5 8600G/Radeon760M, Windows, Release, Technik, 1280x720. Je 420/Folkeboot/Optimist 60 s gueltige Live-Dynamik bei Echtzeitfaktor 1 ueber den ganzen 60-s-Messzeitraum, dt=0.01 s, alle zugeordneten Segel, gemeinsamer SDP und Overlay aktiv: Core <=60 s fuer 60 s Physik, p95 Bildabstand <=33.3 ms, Maximum <=100 ms. Ladezeit/Speicher und Core-Kontrolllauf ohne Anzeige getrennt; vollstaendige Rohwerte, Profile und Eingaben dokumentiert. |
| G25-09 Regression/Sicht | Release-EXE und gesamte GUI-CTest-Suite; insbesondere Kraefte, Druckpunkte, Wind, Montage, dynamische Bedienung, Navigation, Wiedergabe und Settings. 100/150/200 % DPI, reale Core-Daten und Bildbelege; Nutzer-Sichtabnahme nicht durch Testlauf ersetzen. |

## 4. Meilensteine und Pruefbefehle

| Gate | Eintritt | Abnahme |
|---|---|---|
| G1 Vertrag | U-G-000178, Core C1/C2 | Realer Kernvertrag und Buildzuordnung geprueft. SDP-/Linienteile bleiben bis Core C3 offen. |
| G2 Bedienung/Darstellung | G1; U-G-000179 bis U-G-000182; Core C3 | G25-02 bis G25-06 bestanden mit realen Core-Daten. |
| G3 Zustandskonsistente Integration | G2; U-G-000183 | G25-01/G25-07 bestanden; neue und historische Daten sauber getrennt. |
| G4 Technische Gesamtuebergabe | G3; U-G-000184; Core C4 | G25-08/G25-09 bestanden, Core C5 gemeinsam belegbar; alle Anforderungen nachvollziehbar. Nutzer-Sichtabnahme separat. |

Vor Build die vorhandene Konfiguration auf
`C:\_DATEN\Code\Cpp\Segelphysik\CORE` und die benoetigte Core-Fassung
pruefen. Fehlende Produzentenlieferung nicht durch eine alte verlinkte
Bibliothek kaschieren. Nach spaeterem Umsetzungsauftrag:

```powershell
cmake --build C:\spdeps\guibuild --config Release --parallel 6
ctest --test-dir C:\spdeps\guibuild -C Release -R "^gui_force_display$|^gui_core_integration$" --output-on-failure
ctest --test-dir C:\spdeps\guibuild -C Release --output-on-failure
```

Die neuen gezielten Tests zusaetzlich nach ihrer tatsaechlichen Registrierung
auswaehlen und Selektoren hier dokumentieren. Auslieferungsziel:
`C:\spdeps\guibuild\Release\segelphysik_gui.exe`.
Bild-/GPU-Leistung benoetigt einen echten Desktop-/Renderlauf;
Offscreen-Unit-Tests allein sind kein Leistungs- oder Sichtnachweis.

## 5. Rueckverfolgbarkeit und Status

| A-ID | Umsetzung | Tests | Status |
|---|---|---|---|
| A-G-00166 | U-G-000178, U-G-000181, U-G-000183, U-G-000184 | G25-01, G25-05, G25-07, G25-09 | offen; echte Core-SDP-Snapshots integriert und vorbereitete Bildbelege positiv geprueft, gesamte fachliche/DPI-/Regressionsabnahme fehlt |
| A-G-00167 | U-G-000178, U-G-000180, U-G-000183, U-G-000184 | G25-01, G25-04, G25-07, G25-09 | offen |
| A-G-00168 | U-G-000178, U-G-000182, U-G-000184 | G25-06, G25-08, G25-09 | offen |
| A-G-00169 | U-G-000178, U-G-000179, U-G-000183, U-G-000184 | G25-02, G25-07, G25-09 | offen |
| A-G-00170 | U-G-000178, U-G-000179, U-G-000184 | G25-03, G25-09 | offen |
| A-G-00171 | U-G-000178, U-G-000183, U-G-000184 | G25-01, G25-07, G25-09 | offen |
| A-G-00172 | U-G-000184 | G25-08 | erfuellt; alle drei regulaeren 60-s-Desktopfaelle bestanden, Abschnitt 16 |

## 6. Konsistenz- und Umsetzbarkeitspruefung 17.11.2026

- Die bestehenden Kraft-/Druckpunkt-Spezifikationen sind die einzigen
  thematischen Quellen; kein paralleles GUI-sailforce-Dokument.
- Keine GUI-Physik und keine Wiederverwendung des Seitenkraft-SDP als
  behauptetem Ansatzpunkt der vollstaendigen Summenlast.
- Der Core entscheidet Lasten, Punkte und Status; die GUI entscheidet
  ausschliesslich Darstellung und leitet bestaetigte Eingaben weiter.
- Voreingestellte Aktivierung ist keine GUI-Sichtbarkeit; Livewinkel
  sind nicht laufende Luftdaten-/Aktivierungsaenderungen.
- Die Bildzeitgrenzen und die gleichzeitige Core-Echtzeit stimmen
  mit dem Core-Messrahmen ueberein; kein Zyklus der Abschlussgates:
  Core C4 benoetigt nur den headless Nachweis, Core C5 den GUI-Nachweis.
- Die GUI ist nach Lieferung des getesteten Core-Vertrags umsetzbar.
  Zur Planfinalisierung fehlten neue Last-/Punkt-/Geometriedaten,
  Integration und Leistungsbelege; der inzwischen gelieferte Bestand und
  tatsaechliche Teilnachweise stehen in Abschnitt 9 bis 12.
  Technische Umsetzung ist nicht bereits Abnahme.

**Tatsaechliche G-0.25-Implementierungsnachweise:** siehe Abschnitt 9.

## 7. Nachweis der Dokumentfinalisierung

Am 17.11.2026 JSON, eindeutige Anforderungsdefinitionen, IDEE-Zuordnungen,
Anforderungsabdeckung, neue Dateiverweise/Anker und den gemeinsamen
Abhaengigkeitsgraphen strukturell geprueft. Alle 7 GUI-Anforderungen
sind abgedeckt; die 17 Schritte beider Projekte sind einschliesslich
Core-/GUI-Gates azyklisch. `git diff --check` und Whitespacepruefung
neuer Dateien bestanden. Kein GUI-Build, Physiktest, Render-/
Echtzeitlauf oder Nutzer-Sichtnachweis durch diesen Dokumentauftrag.

## 8. Historische erste Build-/Integrationspruefung nach Umsetzungsauftrag

Verlinkter Core-Quellbaum ist `C:\_DATEN\Code\Cpp\Segelphysik\CORE`.
Seine unabhaengigen sailforce-/Wind-/Luft-/SDP-Vertragsbausteine sind
teilweise implementiert und gezielt getestet; damit ist C2/C3 noch nicht
geliefert. Keine Attrappenbedienung oder lokale GUI-Ersatzphysik eingebaut.
Formparitaet sowie die expliziten kompatiblen Modellzuordnungen bleiben
offen; nicht beauftragte SegelMacher-/Modellkorrekturen unterbleiben.

Mit vorhandenem Visual-Studio-CMake 3.31 die bestehende Konfiguration
erneuert und Release-Ziele `segelphysik_gui` sowie
`segelphysik_gui_core_tests` erfolgreich gebaut. Produktive EXE unter
`C:\spdeps\guibuild\Release\segelphysik_gui.exe` aktualisiert.

`gui_core_integration` scheitert vor Simulationsbeginn:
`Core rejected the reference model`. Sein konfigurierter Legacy-Pfad
`Modelle_alt\quader\modells_information.json` existiert nicht;
der gesamte konfigurierte Ordner `Modelle_alt` fehlt. Keine Bestands-
fixture ersetzt und keinen bestandenen GUI-Regressionstest behauptet.
Kein G25-08-Leistungsnachweis und keine Sichtabnahme.

## 9. Laufende GUI-Umsetzung

Adapter, V8-Parser, GUI-Bedienung, Snapshot-Geometrie, Last- und
Punktdarstellung sowie Core-Grenzlinien sind in Bearbeitung. Der Live-Abruf
erfolgt auf einem eigenen Worker; Segelwinkel werden zum naechsten
Integrationsschritt angefordert und die Geometrie ausschliesslich anhand
des angezeigten Core-Snapshots aktualisiert. Der Aktivierungsbestand wird
aus einem vorbereiteten Core-Kontext gelesen; bei fehlerhafter Vorbereitung
ist dessen Diagnose sichtbar. Luftwerte werden vor Persistenz mit dem
aktuellen Modell durch den Core geprueft.

Der gezielte Parser-Test liegt in `gui_force_display`; der gezielte
Worker-Historientest ist im bestehenden Adapter-Testprogramm ueber
`--sailforce-history-only` mit einem echten zugeordneten Modell aufrufbar.
Der Render-Harness bietet `--sailforce-performance-only <Modellordner>
<Evidenceordner>`: drei echte Live-Laeufe, je 60 s / 0,01 s / Faktor 1,
Anfangszustand Core-Hydrostatikgleichgewicht, Wind 3 m/s aus 65 Grad,
Referenzhoehe 10 m. Er speichert Einzelzeitpunkte von
`QOpenGLWidget::frameSwapped`, Grenzen, Core-Zeiten, Zustand und
Speicherbeleg; er verwendet kein Replay. Die Modellbedingungen sind mit
dem Core-Kontrolllauf abzustimmen.

Der erste gemeinsame Release-Build der GUI und ihrer drei gezielten
Testziele bestand mit `/WX`; `gui_force_display` bestand. Moderne
Modellformat-0.3-Tests bestanden laut gemeinsamem Testlauf. Die
vollstaendige GUI-Suite wurde wegen der historischen Tests gestoppt:
`gui_core_integration` uebergibt `modells_information.json` (0.2), der
aktuelle Core akzeptiert nur `modells_data.json` (0.3). Auch weitere
Fixturepfade des Tests erwarten das nicht mehr unterstuetzte Altformat.
Dies ist eine bekannte Vorbelastung, kein bestandener Regressionstest;
keine historischen Daten als neue Physik ausgeben.

Das SegelMacher-Runtime-Deployment bestand laut gemeinsamem Testlauf
ohne zusaetzlichen PATH. Der gezielte Historientest scheiterte zuerst an
`CORE-SIM-0001: Diagnose fuer fehlende Mastspitze erforderlich.`
Der Adapter ergaenzt jetzt bei fehlender Mastreferenz und leerer Diagnose
den expliziten Hinweis auf die fehlende Referenz; Punktwind bleibt echte
Core-Berechnung. Der Test gibt die konkrete Core-Startdiagnose aus.
Am 17.11.2026 wurde ausschliesslich das freigegebene Ziel
`segelphysik_gui_core_tests` erneut mit VS-CMake 3.31 in Release gebaut;
der Aufruf mit `Modelle\420\modells_data.json --sailforce-history-only`
bestand: unveraenderliche Historie und bestaetigter Live-Stellwinkel.
Dies ist ein Teilnachweis fuer G25-07, keine vollstaendige Abnahme.
G25-08 war zu diesem Zwischenstand noch ungemessen; aktuelle Messungen
stehen in Abschnitt 11. Keine technische Gesamt-/Leistungs-/
Renderabnahme und keine Nutzer-Sichtabnahme.

V8-Vertragsergaenzung fuer G25-01/G25-06: Der Parser verlangt parallel
zu jeder Grenzlinie `grenzlinien_zuordnung` und fuehrt Linien-ID,
Grenz-ID sowie optionale untere/obere Streifen-ID mit. Geprueft werden
Laengengleichheit, eindeutige nichtnegative ganzzahlige Linien-IDs,
gueltige Grenz-/Nachbar-IDs und mindestens ein Nachbar; bei aktiven
Segeln muessen Nachbarn in deren Streifenlasten vorkommen. Inaktive
Segel behalten Metadaten ohne Streifenlasten. Fixturetests fuer fehlende,
falsche und doppelte Zuordnungen sowie mehrere Linien derselben Grenze
und inaktive Segel ergaenzt. Build und Testlauf dieser Ergaenzung bleiben
auf ausdruecklichen Nutzerauftrag bis zum freien Buildfenster ausgesetzt.

## 10. Gemeinsame Desktop-Messung am 17.11.2026

Historischer Vorlaeufer, **kein gueltiger G25-08-Nachweis**: Die spaetere
Profilpruefung ergab, dass `dynamicDefaults()` Optik statt der bezeichneten
Technik lieferte. Zudem markierte das Harness seine per `setText()` gesetzten
Windfelder nicht als Benutzereingabe: tatsaechlich 5 m/s und bootmitgefuehrter
Wind statt der bezeichneten 3 m/s aus 65 Grad. Werte unten bleiben als
Diagnosehistorie erhalten; sie sind nicht direkt mit C25-15 vergleichbar.

Parser und Historientest nach Metadatenergaenzung laut Hauptlauf bestanden.
Der erste Desktop-Harness klickte nach dem Reiterwechsel, bevor dessen
asynchrone Vorbereitung den Startknopf wieder freigab. Der Harness wartet
jetzt auf den freigegebenen dynamischen Arbeitsbereich und protokolliert
konkrete Dialog-/Core-/VTK-Diagnosen. Ein realer kurzer Core-Lauf von 0,01 s
bereitet je Boot einen gueltigen Sailforce-Snapshot fuer einen Screenshot
vor; danach beginnt der unabhaengige 60-s-Lauf erneut im unveraenderten
hydrostatischen Gleichgewicht. Screenshots werden ausschliesslich vor der
Messung gespeichert. Waehrend der Zeitmessung bleiben Rohzeiten im Speicher;
keine periodischen Dateischreibvorgaenge oder Screenshots.

Auf ausdrueckliche Freigabe nur `segelphysik_gui_graphics_render_tests`
und `segelphysik_gui` in Release gebaut. Echter Desktop mit
`QT_QPA_PLATFORM=windows`, 8600G laut verifizierter Nutzerangabe.
Kein paralleler Build/Test oder weitere Agenten-CPU-Arbeit waehrend der
Messung. Alle drei Laeufe abgeschlossen, je 6001 Zustaende, alle Segel
aktiv und Grenzlinien vorhanden, keine VTK-Fehler. Kriterien unveraendert:

| Boot | Core s | Wand s | p95 Bildabstand ms | Maximum ms | G25-08 |
|---|---:|---:|---:|---:|---|
| 420 | 78,8434 | 78,8717 | 140,9488 | 186,3175 | Nicht bestanden |
| Folkeboot | 39,5364 | 60,0084 | 93,5920 | 149,8032 | Nicht bestanden |
| Optimist | 10,5874 | 60,0016 | 73,7386 | 95,1069 | Nicht bestanden |

Der separate Core-Nachweis C25-15 meldet 59,0723 / 27,3152 / 6,04252 s
fuer jeweils 6000 gueltige Schritte bei 3 m/s aus 65 Grad und dt=0,01 s.
Die GUI-Integration ueberschreitet damit insbesondere beim 420 die
Core-Zeitgrenze; alle drei Bildabstands-p95 verfehlen 33,3 ms. Ursache der
Leistungsabweichung im Anzeige-/Snapshot-/Renderingpfad noch nicht
abschliessend isoliert; keine Leistungsfreigabe behauptet.

Rohdaten, Einzelzeitpunkte, Status, Grafikfeatures, End-Sailforce-/Winddaten
und Fehlerlisten:
`C:\spdeps\guibuild\evidence\G25-08\sailforce-performance.json`.
Screenshots echter vorbereiteter Zustaende im selben Ordner:
`420-sailforce-prepared.bmp`, `folkeboot-sailforce-prepared.bmp`,
`optimist-sailforce-prepared.bmp`. Bericht wird schon vor jedem Fall,
bei Vorbereitungsfehlern/Abbruch sowie nach jedem abgeschlossenen Fall
gespeichert; behandelter Abbruch erhaelt bis dahin erfasste Frame-Rohzeiten.
Ein harter Prozessabsturz waehrend der Messung kann die noch ausschliesslich
im Speicher liegenden Rohzeiten verlieren. Der Nutzer hat die drei
vorbereiteten Bildbelege auf leichte Linien, gemeinsamen SDP und fehlende
zusaetzliche Teilflaechen positiv geprueft; dies ersetzt keine vollstaendige
Sicht-/DPI-/Regressionsabnahme.

## 11. Profilierung und Snapshot-Doppelarbeit, Zwischenstand 17.11.2026, 21:50 Uhr

Das Harness setzt inzwischen isoliert die vorhandenen Technik-Defaults und
markiert die Windfelder korrekt als veraendert. Vor der Messung werden die
tatsaechlichen Core-Windwerte (weltfest, 3 m/s, Herkunft 65 Grad, Hoehe 10 m),
Luftwerte und aktive Segel bei 0 Grad geprueft. Die Messung laeuft im nativen
Qt-Eventloop statt einer manuell schlafenden Event-Pollschleife. Gezaehlt
werden nur echte Praesentationen eines neuen vollstaendigen Core-Snapshots,
einschliesslich Anfangs-/Endluecke des vollen Messzeitraums. Der endgueltige
Snapshot wird nach Messende noch asynchron zugestellt und im Bericht gesichert.

Letzter vollstaendiger Desktoplauf, Bericht geschrieben um 21:46:13 Uhr,
jeweils 60 s Physik, 6000 Schritte/6001 Zustaende:

| Boot | Core s | Wand s | p95 Bildabstand ms | Maximum ms | G25-08 |
|---|---:|---:|---:|---:|---|
| 420 | 79,2804 | 79,3067 | 38,3961 | 54,6586 | Nicht bestanden |
| Folkeboot | 37,9327 | 60,0082 | 37,4071 | 76,4014 | Nicht bestanden |
| Optimist | 10,4016 | 60,0025 | 37,5510 | 146,6723 | Nicht bestanden |

Beim 420 fehlen damit 19,3067 s Wandzeit zur Echtzeit. Ein vorheriger
vollstaendiger Lauf mit bereits korrigiertem Wind lag bei 63,3258 s Core,
35,5836 ms p95 und 51,9538 ms Maximum. Die Laufzeitschwankung ist nicht
abschliessend erklaert; kein isolierter GPU-Befund und keine Freigabe.
Waehrend der Messung liefen keine zusaetzlichen Agenten-Toolarbeiten;
anderweitige Systemlast wurde nicht durch Prozessbeendigung manipuliert.

Mediananteile des letzten 420-Laufs (ueberlappende Phasen nicht addieren):
Core-Auswertung/JSON 8,978 ms, JSON-Decodierung 1,479 ms,
Worker-Motionvorbereitung 4,296 ms, Worker-Forceparser 0,606 ms,
gesamter Abruf 16,365 ms, anschliessende GUI-/Praesentationswartezeit
13,480 ms, Sceneupdate inklusive VTK-Render 21,667 ms, darin Render
20,327 ms, sonstige UI einschliesslich Tabelle 1,298 ms.
Der reine VTK-Renderaufruf ist eine Wandzeit, keine GPU-Zeitmessung.

Umgesetzte GUI-Optimierungen: immutable Segelgeometriecache nach
Modell-/Quellfassung und bestaetigtem Winkel/Spiegelstand; gemeinsame
Meshvorbereitung der Ansichten; gebuendelte gleichartige Kraft-Actors mit
erhaltenen individuellen Picking-IDs; keine erzwungene Tabellen-Neusortierung
bei jedem Frame; keine Aktualisierung versteckter Legacy-Wertetabellen im
Simulationsworkspace; Snapshot-Prefetch mit begrenztem Takt und Kopplung an
echte Qt-Praesentation statt mehrfacher unpraesentierter Renderings.
Die bestehende Physik und deren Integratorauswertungen bleiben unveraendert.

Exakte Wasserlinienschnitte werden im Abrufworker vorbereitet und nur bei
identischer Pose und identischen immutable Meshquellen wiederverwendet;
anderenfalls berechnet die bestehende GUI-Schnittpipeline sie erneut.
Die Schnittarbeit im GUI liegt im letzten Lauf bei 0,591 ms Median,
die Segelgeometriearbeit bei 0,007 ms. Reflexionsclipping erfolgt ohne
CPU-Neuvernetzung an der Welt-Wasserflaeche. Der vorhandene echte
`--water-reflection-only`-Test bestand nach den Aenderungen, einschliesslich
Bildparitaet off-thread vorbereiteter Schnitte und Verwerfung einer fremden
Bootspose. Keine Segelbahnen, Grenzlinien oder Physikauswertungen ausgelassen.

**Enger naechster Schritt: ErgebnisSnapshot statt erneuter Kraftauswertung.**
Reader und Worker besitzen bereits verschiedene `Modellaufbereitung`-Instanzen;
der gemeinsame Historienmutex wird vor JSON freigegeben. Die Hypothese eines
gemeinsam ueberschriebenen Aufbereitungs-Caches ist fuer diesen GUI-Pfad
widerlegt. Bestaetigt ist die erneute Kraftauswertung in `zustand_json()`
pro Anzeigeabruf. Der Hauptagent liefert dafuer CPPI-0206 bis CPPI-0208:
immutable `ErgebnisSnapshot`, `Modellaufbereitung::snapshot()` und
`snapshot_json()` ohne erneute Physik. Die Core-seitige Paritaets-,
Lebensdauer- und Mehrleserpruefung wurde als bestanden gemeldet.

GUI-Vorbereitung: Ringcache der 64 neuesten Ergebnis-Snapshots;
Capture ausschliesslich ueber die Worker-Aufbereitung, gemeinsame
Veroeffentlichung von Historienindex und Snapshot unter kurzem Mutex,
Freigabe verdraengter Snapshots ausserhalb dieses Mutex. Reader kopiert
bei Treffern nur den immutable Handle und serialisiert ausserhalb des Mutex.
Aeltere Historie verwendet weiterhin die separate Reader-Aufbereitung.
Der Startsnapshot wird einmal vorbereitet und zur ersten Veroeffentlichung
uebergeben. Keine Historie aus 6000 vollstaendigen Kraefteobjekten.

Capture-Laufzeit (Summe/Maximum/Anzahl), Reader-Treffer/Fallbacks und
Capture-/Trefferdaten je angezeigtem Snapshot sind instrumentiert.
Der gezielte Historientest prueft zusaetzlich die 64/65-Verdrängungsgrenze
und identische alte JSON-Daten nach dem Live-Winkelwechsel.
Nach gemeinsamer Buildfreigabe wurden `segelphysik_gui_core_tests`,
`segelphysik_gui_graphics_render_tests` und `segelphysik_gui` mit VS-CMake
3.31 in Release `/WX` erfolgreich gebaut. Der gezielte Aufruf mit
`Modelle\420\modells_data.json --sailforce-history-only` bestand,
einschliesslich Treffer/Fallback, 64/65-Verdrängungsgrenze und alter
JSON-Paritaet nach Live-Stellwinkelaenderung.
Eine Leistungsaussage zur Snapshot-Aenderung liegt noch nicht vor.
Naechster Nachweis: kurze Phasen-/Trefferratenprobe nach abgestimmtem Ende
des parallelen separaten Core-Builds; erst danach abgestimmter
420-Kontrolllauf mit/ohne Anzeige.
G25-08, G25-09 und Gesamtfreigabe bleiben offen.

## 12. Kontrollierter Snapshot-/GUI-Nachweis, Stand 17.11.2026, 22:27 Uhr

Dieser Abschnitt schreibt U-G-000178, U-G-000183 und U-G-000184 mit den
tatsaechlichen Teilnachweisen fort; keine neue Spezifikation und keine
Aenderung der Anforderungen oder Leistungsgrenzen.

### 12.1 Snapshot-Handoff und einmalige GUI-Vorbereitung

Die in Abschnitt 11 vorbereitete ErgebnisSnapshot-Anbindung ist gebaut und
im echten Desktoppfad gemessen. Ringcache 64, kurzer Historienmutex und
separater Reader-Fallback bleiben wie beschrieben. Die explizite
instrumentierte Core-Pruefung, dass Capture weder eine neue Kraftauswertung
noch Modellinitialisierung verursacht, ist laut Hauptagent ebenfalls gruen;
dies wurde nicht als eigener GUI-Test ausgegeben.

Der GUI-Abruf verwendet jetzt einen langlebigen LowPriority-Readerthread
mit Arbeitsqueue und hoechstens einem laufenden Abruf statt eines neu
erzeugten QThread pro Snapshot. Generations-/Modellpruefung,
Praesentationsgate und historienkonsistente Zustellung bleiben erhalten.
Der Thread wird beim Schliessen beendet und abgewartet.

Motion-Wasserlinienschnitte verwenden je immutable Meshquelle einmalig
vorbereitete Cutter und Stripper sowie einen wiederverwendeten Konturbuffer.
Ein konservativer Plane/AABB-Test weist nur geometrisch garantiert leere
GUI-Schnitte zurueck; randnahe oder moegliche Schnitte bleiben im exakten
VTK-Pfad. Keine Segelbahnen, Grenzlinien, Kraefte, Integratorauswertungen
oder Motion-Historienzustaende werden reduziert.

Die gezielte 420-Kurzprobe prueft die Kontaktverfuegbarkeit sowie
Leading-/Trailing-Punkte und Breite exakt gegen den ungefilterten Schnittpfad,
einschliesslich vollstaendig trockener und getauchter Testpose. Bestanden;
diese Geometriepruefung ist kein Ersatz fuer einen realen Leistungsnachweis.
Median der Motion-Vorbereitung in den aufeinanderfolgenden 5-s-Kurzproben:
3,969 -> 3,252 ms; Core 5,169 -> 5,123 s, jeweils 187/187 Cachetreffer.
Die Kurzproben sind ausdruecklich **keine 60-s-Abnahme**.

### 12.2 Identischer 420-Kontrolllauf ohne und mit Anzeige

Beide 60-s-Laeufe verwenden denselben GUI-Adapter und exakt dieselbe
tatsaechliche Startkonfiguration (`lastRunInput()`): Hydrostatikgleichgewicht,
dt=0,01 s, Faktor 1, Wind weltfest 3 m/s aus 65 Grad bei 10 m,
Luft 1,225 kg/m^3 und 1,5e-5 m^2/s, alle zugeordneten Segel aktiv bei
0 Grad, Hydrodynamik aktiv. Kein GUI-eigener Physikersatz.
Der Kontrolllauf ohne Anzeige besitzt ebenfalls Historie und Snapshotcache;
waehrend seiner Physik erfolgen keine JSON-/Renderabrufe.
End-JSON entsteht erst nach dessen Zeitmessung.

Echte Windows-Desktopmessung auf der bestaetigten Referenzhardware,
1280x720, Technikdefaults, gemeinsamer SDP und alle Grenzlinien.
Keine zusaetzlichen Agenten-Builds, Tests oder CPU-Arbeiten waehrend der
Messung; Screenshots nur im vorbereiteten Zustand vor der Messung.
Framezeiten sind tatsaechliche Praesentationen neuer Sailforce-Snapshots,
keine Dummy-Renderings. Rohzeiten werden waehrend des Laufs im Speicher
gehalten und anschliessend einschliesslich Fehlern persistiert.

Letzter abgeschlossener kontrollierter Vergleich:

| Groesse | Ohne Anzeige | Mit echter Anzeige |
|---|---:|---:|
| Simulationsdauer s / Schritte / Zustaende | 60 / 6000 / 6001 | 60 / 6000 / 6001 |
| Corezeit s | 59,4348304 | 61,2068622 |
| Wandzeit s | 60,0149883 | 61,3730269 |
| Physikworker-CPU-Zeit s | 59,5312500 | 61,2656250 |
| Wandzeit ausschliesslich in `schritt()` s | 59,3485756 | 61,1119968 |
| Pacing-Wartezeit s | 0,4321764 | 0,0113521 |
| Historienmutex-Wartezeit s | 0,0016398 | 0,0015420 |
| Snapshot-Capture gesamt s | 0,0742149 | 0,0832270 |
| Laufende Captures | 6000 | 6000 |
| Reader-Treffer / Fallbacks waehrend Messung | 0 / 0 | 2194 / 0 |
| Frame-p95 ms | nicht anwendbar | 30,0578 |
| Frame-Maximum ms | nicht anwendbar | 49,7771 |

Die physikalischen Endzustaende sind exakt identisch geprueft
(Pose, Geschwindigkeiten, Bewegungen, Elementtransformationen, Sailforce,
Wind und Vektoren). Die geringe Mutexwartezeit und nahezu vollstaendig
aktive Worker-CPU-Zeit belegen keinen Mutex-/Pacingstau.
Der Unterschied von 1,7720318 s Corezeit steckt ueberwiegend in der
gemessenen `schritt()`-Laufzeit bei aktiver Anzeige. Daraus folgt weder eine
GPU-Zeitmessung noch eine isoliert bewiesene GPU-Ursache.

Aktuelle Mediananteile des Anzeigelaufs, ohne vorbereiteten Index 0:
Snapshot-JSON 4,906 ms, Motion-Vorbereitung 3,364 ms,
gesamter Abruf 10,186 ms, Sceneupdate inklusive Render 19,542 ms,
darin Render 18,385 ms; sonstige UI inklusive Tabelle 1,043 ms.
Ueberlappende Phasen nicht addieren; Render ist CPU-seitige Wandzeit,
keine getrennt gemessene GPU-Zeit.

**Ergebnis: 420-Bildkriterien bestanden, Corezeit noch 1,2068622 s ueber
60 s. G25-08 bleibt nicht bestanden.** Keine Kriteriumsaenderung,
Physik-/Streifenreduktion oder Auslassung von Integratorstufen.

### 12.3 Rohdaten und verbleibende Gates

Massgeblicher letzter Vergleich einschliesslich Fehler-/Frame-/Phasenrohdaten,
tatsaechlicher Konfiguration und Endparitaet:

`C:\spdeps\guibuild\evidence\G25-snapshot-420-motion-comparison\sailforce-performance.json`

Zugehoerige 5-s-Kurzprobe einschliesslich exakter Schnittkontaktpruefung:

`C:\spdeps\guibuild\evidence\G25-snapshot-420-motion-profile\sailforce-performance.json`

Unmittelbar vorheriger Vergleich mit langlebigem Reader:
Core ohne/mit Anzeige 59,6824 / 61,1428 s,
Frame-p95 30,0424 ms / Maximum 44,8777 ms, 2183/2183 Treffer:

`C:\spdeps\guibuild\evidence\G25-snapshot-420-persistent-reader\sailforce-performance.json`

Die letzte Motion-Aenderung verbessert die gezielte Vorbereitungsphase,
belegt gegenueber diesem vorherigen Vergleich aber **keinen weiteren
Gesamt-Corezeitgewinn**. Die berichteten Schwankungen werden nicht als
erreichte Echtzeit ausgegeben.

Aktueller Build-/Abnahmestand:

- Renderharness mit langlebigem Reader, konservativer Schnittvorpruefung
  und Stripper-/Konturbuffer-Reuse in Release `/WX` gebaut und gemessen.
- GUI-EXE und Adaptertest wurden nach Snapshotintegration erfolgreich gebaut;
  der Historientest einschliesslich 64/65-Verdrängung und alter
  JSON-Paritaet bestand. Die spaeteren GUI-Optimierungen sind noch nicht
  in einer abschliessend neu gebauten produktiven EXE nachgewiesen.
- Seit Nutzerabstimmung um 22:27 Uhr keine weiteren GUI-Builds oder
  Messungen: CPU-Messfenster gehoert der engen Core-Pruefung.
- Nach expliziter Core-/Buildfreigabe produktive GUI-EXE und benoetigte
  Testziele gegen den dann geprueften Core bauen, kontrolliertes
  420-Gate erneut messen; nur bei Erfolg regulaere drei echte
  60-s-Bootslaeufe fuer G25-08.
- G25-09 bleibt wegen fehlender vollstaendiger Regression und
  100/150/200-%-DPI-/Gesamtsichtabnahme offen. Bekannte historische
  Modellformat-/Fixturevorbelastungen aus Abschnitt 8/9 bleiben sichtbar.
  Positive vorbereitete Screenshot-Sichtpruefung ersetzt diese Gates nicht.
- Keine Git-Aktion; GUI-Code, Tests und Dokumentation bleiben uncommittet.

## 13. Finaler Build und regulaere Dreiboatmessung, 17.11.2026, 22:39 Uhr

Nach ausdruecklicher Freigabe um 22:30 Uhr gegen den vom Hauptagenten
optimierten und geprueften Core gebaut. Laut Core-Liefernachweis werden
bereits im Bounds-Scan berechnete vorzeichenbehaftete Punkthoehen im
Wasserschnitt wiederverwendet; keine Physik-/Toleranzaenderung.
Hydrobenchmark sowie bitidentische Dynamik-Endzustaende aller drei Boote
und native Standard-/Instrumentierungstests wurden vom Hauptagenten
als bestanden gemeldet; keine eigenen GUI-Messungen daraus abgeleitet.

### 13.1 Auslieferungsstand und gezielte Paritaetstests

Mit VS-CMake 3.31 in Release `/WX` erfolgreich gebaut:
`segelphysik_gui`, `segelphysik_gui_graphics_render_tests`,
`segelphysik_gui_core_tests`, `segelphysik_gui_force_display_tests`.
Die produktive EXE enthaelt jetzt auch langlebigen Reader und die
GUI-Schnittoptimierungen aus Abschnitt 12:

`C:\spdeps\guibuild\Release\segelphysik_gui.exe`

Dateistand 17.11.2026, 22:31:13 Uhr, 4.071.936 Bytes.
Keine Core-Quellenaenderung durch die GUI-Umsetzung.

Tatsaechliche gezielte Testergebnisse gegen diesen Core:

- `segelphysik_gui_force_display_tests.exe`: **766 Pruefungen bestanden**.
- `segelphysik_gui_core_tests.exe <Modelle>\420\modells_data.json
  --sailforce-history-only`: unveraenderliche Historie,
  bestaetigter Livewinkel, Snapshot-Treffer/64er-Verdrängung und alte
  JSON-Paritaet **bestanden**.
- `segelphysik_gui_graphics_render_tests.exe --water-reflection-only
  <Modelle> C:\spdeps\guibuild\evidence\G25-final-core-reflection`:
  echter Windows-Desktop, Reflexions-/vorbereitete Wasserlinienparitaet
  einschliesslich Verwerfung einer fremden Pose **bestanden**.

### 13.2 Kontrolliertes 420-Gate gegen neuen Core

Gleiche unveraenderte Konfiguration und Endparitaetspruefung wie Abschnitt
12.2, wiederum 6000 Schritte / 6001 Zustaende / 60 s Physik.

| Groesse | Ohne Anzeige | Mit echter Anzeige |
|---|---:|---:|
| Corezeit s | 58,2297756 | 59,7495093 |
| Wandzeit s | 60,0110846 | 60,0663862 |
| Physikworker-CPU-Zeit s | 58,0468750 | 59,7656250 |
| Wandzeit ausschliesslich in `schritt()` s | 58,1435541 | 59,6583191 |
| Historienmutex-Wartezeit s | 0,0017113 | 0,0014728 |
| Snapshot-Capture gesamt s | 0,0739833 | 0,0789487 |
| Reader-Treffer / Fallbacks waehrend Messung | 0 / 0 | 2160 / 0 |
| Frame-p95 ms | nicht anwendbar | 29,9420 |
| Frame-Maximum ms | nicht anwendbar | 51,3272 |

Physikalischer Endzustand exakt identisch.
**Dieses kontrollierte 420-Gate ist bestanden**, Core-Reserve aber nur
0,2504907 s. Daraus wird keine robuste Dreiboatfreigabe abgeleitet.

Rohdaten einschliesslich Konfiguration, Endparitaet und Frame-/Phasenzeiten:

`C:\spdeps\guibuild\evidence\G25-final-core-420-comparison\sailforce-performance.json`

### 13.3 Anschliessende regulaere drei echte 60-s-Laeufe

Unveraenderte G25-08-Bedingungen aus Abschnitt 12.2; Windows-Desktop,
keine parallelen Builds/Tests/Agenten-CPU-Arbeiten. Vorbereitete echte
Screenshots jeweils vor der Zeitmessung, keine zusaetzlichen
Screenshot-/Dateischreibspitzen waehrenddessen.

| Boot | Core s | Wand s | Frame-p95 ms | Maximum ms | Coregate | Bildgate | Gesamtfall |
|---|---:|---:|---:|---:|---|---|---|
| 420 | 60,7687345 | 61,0248788 | 31,5942 | 46,9498 | Nicht bestanden | Bestanden | Nicht bestanden |
| Folkeboot | 30,1871174 | 60,0057377 | 26,3708 | 37,3823 | Bestanden | Bestanden | Bestanden |
| Optimist | 9,0541307 | 60,0032377 | 28,6827 | 93,1439 | Bestanden | Bestanden | Bestanden |

Jeweils 6001 Zustaende; Snapshot-Treffer 2107 / 2480 / 2434, **kein
Fallback**. Ladezeiten 4,8056 / 2,0004 / 1,1706 s.
Gemeldeter maximaler Working Set ueber die gesamte Lebensdauer dieses
gemeinsamen Harnessprozesses: 726.765.568 Bytes; kein unabhaengiger
Speichermesswert je Boot.

Beim 420 liegt die Corezeit **0,7687345 s ueber der Grenze**.
`schritt()` benoetigt 60,6719582 s, Capture 0,0837503 s und
Historienmutex-Warten insgesamt 0,0017709 s. Der Abstand zum unmittelbar
vorher bestandenen kontrollierten Anzeigelauf betraegt 1,0192252 s.
Die beobachtete Schwankung ist keine isoliert bewiesene GPU-Ursache und
wird weder gemittelt noch durch Auswahl des besseren Laufs kaschiert.
Keine weitere blinde Messrunde und keine Reduktion der Physik/Geometrie.

Massgeblicher regulaerer Bericht mit vollstaendigen Frame-/Phasenrohwerten,
geprueften Eingaben und Endzustaenden:

`C:\spdeps\guibuild\evidence\G25-final-three-boats\sailforce-performance.json`

Echte vorbereitete Bildbelege im selben Ordner:
`420-sailforce-prepared.bmp`, `folkeboot-sailforce-prepared.bmp`,
`optimist-sailforce-prepared.bmp`.

**Gesamtstand:** Alle drei Bildzeitgates bestanden; Folkeboot und Optimist
bestehen G25-08 vollstaendig. Wegen des regulaeren 420-Corezeitfehlers
bleibt G25-08 insgesamt **offen/nicht bestanden**. G25-09 mit gesamter
GUI-Regression und DPI-/Gesamtsichtabnahme bleibt ebenfalls offen;
bekannte Legacy-Format-/Fixturevorbelastungen unveraendert. Keine
Gesamtfreigabe und keine Git-Aktion. Code, Tests und Plan bleiben
uncommittet. Alle gestarteten Messprozesse sind abgeschlossen; das
CPU-Messfenster ist wieder fuer den Hauptagenten frei.

## 14. IPO-Core: finaler Build und regulaerer Lauf, 17.11.2026, 22:59 Uhr

Nach ausdruecklicher Freigabe um 22:54 Uhr die vier GUI-/Testziele aus
Abschnitt 13.1 gegen den final gelieferten Release-IPO-Core erneut
erfolgreich gebaut. Core verwendet laut Liefervertrag `/GL`,
konsumierende MSVC-EXEs transitive `/LTCG`, weiterhin `/fp:precise`,
kein Fast-Math. Keine GUI-Code-/Physik-/Aufloesungsaenderung zwischen
den Messstaenden aus Abschnitt 13 und 14.

Produktive EXE: `C:\spdeps\guibuild\Release\segelphysik_gui.exe`,
17.11.2026, 22:54:22 Uhr, 4.042.240 Bytes.

Gezielt erneut bestanden: Parser **766 Pruefungen**,
echte 420-Snapshot-Historie/Livewinkel/64er-Verdrängung/JSON-Paritaet
und bestehender Windows-Desktop-Reflexions-/Wasserlinienparitaetstest.
Bildnachweis dieses Paritaetstests:
`C:\spdeps\guibuild\evidence\G25-final-ipo-reflection`.

Auf Nutzerauftrag direkt der regulaere Dreiboottest ohne weitere
vorgeschaltete 420-Kontrollrunde. Identische volle Physik und
G25-08-Kriterien; echtes Windows-Desktop, keine parallele CPU-Arbeit.

| Boot | Core s | Wand s | Frame-p95 ms | Maximum ms | Coregate | Bildgate | Gesamtfall |
|---|---:|---:|---:|---:|---|---|---|
| 420 | 60,8379435 | 61,0412030 | 46,3195 | 107,8527 | Nicht bestanden | Nicht bestanden | Nicht bestanden |
| Folkeboot | 30,8363189 | 60,0060669 | 45,6269 | 72,9476 | Bestanden | Nicht bestanden | Nicht bestanden |
| Optimist | 9,3988859 | 60,0026036 | 44,9073 | 76,3327 | Bestanden | Nicht bestanden | Nicht bestanden |

Jeweils 6001 Zustaende; Snapshot-Treffer 1386 / 1380 / 1401,
kein Fallback. 420-Coreueberschreitung 0,8379435 s;
`schritt()` 60,7322727 s, Capture 0,0921584 s,
Historienmutex-Warten 0,0013070 s.

Die vorhandenen Rohprofile zeigen gegenueber dem unmittelbar vorherigen
regulaeren Lauf eine deutliche Verlaengerung der Renderaufrufe bei allen
Booten, nicht eine neue Kraftauswertung im Reader:

| Median ms | 420 vorher -> IPO | Folkeboot vorher -> IPO | Optimist vorher -> IPO |
|---|---:|---:|---:|
| Snapshot-JSON | 4,899 -> 4,766 | 3,871 -> 3,280 | 1,928 -> 1,408 |
| VTK-Renderaufruf | 19,115 -> 33,552 | 15,474 -> 33,588 | 15,468 -> 33,718 |

420-Sceneupdate inklusive Render 34,734 ms Median, sonstige UI
1,245 ms; GUI-/Praesentationswartezeit 31,758 ms.
Der OpenGL-Bericht nennt in beiden Laeufen denselben Renderer/Treiber.
Die Ursache der renderseitigen Laufzeitverschlechterung ist damit
**noch nicht isoliert**: Render ist eine CPU-seitige Wandzeit und kein
direkter GPU-Zeit-/Lastnachweis; weder GPU-Last noch IPO allein werden
ohne weiteren Beleg als Ursache behauptet. Keine Auswahl des besseren
historischen Laufs und keine Mittelung zu einem bestandenen Gate.

Rohdaten, Eingaben, vollstaendige Frame-/Phasenzeiten, Endzustaende und
vorbereitete echte Screenshots:

`C:\spdeps\guibuild\evidence\G25-final-ipo-three-boats\sailforce-performance.json`

Die ergaenzend beauftragte einzelne 420-Bestaetigungsrunde gilt nur,
falls der regulaere IPO-420 besteht und weniger als 2 s Reserve hat.
Diese Voraussetzung ist **nicht erfuellt**; deshalb kein weiterer Lauf.
Kein Code-/Buildwechsel und keine weitere blinde Messrunde.

**Aktueller Abschlussstand:** IPO-GUI-EXE gebaut, gezielte Paritaetstests
bestanden, G25-08 weiterhin **nicht bestanden** (420-Core und aktuelle
Bildzeitgates). G25-09/Nutzer-/DPI-/Gesamtregressionsabnahme bleiben
offen. Alle gestarteten Prozesse beendet, CPU-Messfenster freigegeben.
Keine Git-Aktion; GUI-Code, Tests und Dokumentation uncommittet.

Naechste gezielte Ursachenpruefung auf Nutzerauftrag: im bestehenden
Renderharness `--sailforce-frozen-presenter-only` vorbereiten, echter
eingefrorener 420-Sailforce-Snapshot nach beendetem Core-Lauf, gleiche
1280x720-/Technikdarstellung. Fuenf Sekunden ausschliesslich
Praesentation; Render-Wandzeit und GUI-Thread-CPU (`GetThreadTimes`),
`frameSwapped`-Abstaende, angefordertes/gemeldetes Swapintervall und
Qt-/Windows-Monitorfrequenz getrennt mit Rohwerten protokollieren.
Eine diagnostische Swapintervall-Vorgabe bleibt auf diesen Harnessmodus
beschraenkt; keine produktive Presenteraenderung ohne Befund.
Instrumentierung ist vorbereitet, aber nach angekuendigtem parallelem
Core-Build noch **nicht gebaut oder gemessen**. Vor unbelasteter Probe
ist ein ausdruecklich abgestimmtes Messfenster erforderlich.

## 15. Frozen-Presenter A/B/A, 17.11.2026, 23:09 Uhr

Nach expliziter kurzer Build-/Messfreigabe ausschliesslich den
Renderharness gebaut; dessen Core-Abhaengigkeit uebernahm den inzwischen
gelieferten exakten Dreieck-Schnellpfad. Danach drei je etwa 5-s-Proben
auf **demselben unveraenderten Build**, immer derselbe echte vorbereitete
420-Snapshot, 1280x720 Technik, Core-Lauf bereits beendet:
Default -> diagnostische globale Swapvorgabe 0 -> explizite Vorgabe 1.
Keine 60-s-Messung, kein Replay und kein Physik-/Qualitaetswechsel.

| Groesse | Default (1) | Vorgabe 0 | Rueckkehr 1 |
|---|---:|---:|---:|
| Messdauer ms | 5035,6250 | 4986,5136 | 5033,1360 |
| Renderaufrufe | 109 | 1051 | 109 |
| Render-Wandzeit Median ms | 46,0411 | 4,4027 | 46,0244 |
| Render-Wandzeit p95 ms | 47,8880 | 5,1184 | 47,7745 |
| Render-Wandzeit gesamt ms | 4982,1515 | 4654,3206 | 4974,4134 |
| GUI-Thread-CPU innerhalb Render gesamt ms | 718,7500 | 3375,0000 | 187,5000 |
| Gesamte GUI-Thread-CPU ms | 796,8750 | 3625,0000 | 218,7500 |
| `frameSwapped`-Abstand Median ms | 46,5423 | 4,7134 | 46,5666 |
| `frameSwapped`-Abstand p95 ms | 48,4759 | 5,4982 | 48,3817 |

Alle drei Proben: **0 Stateupdates, 0 Meshvorbereitungen**, echter
Core-Snapshot unveraendert. Qt und Windows melden denselben Monitor
`\\.\DISPLAY1` mit **100 Hz**. Ein weiterer Render wird jeweils erst nach
`frameSwapped` angefordert; kein ungebremster Timer mit mehreren
Renderings vor einer Praesentation.

Wichtige Metadatengrenze: `QSurfaceFormat::defaultFormat().swapInterval()`
wechselt wie vorgegeben 1 -> 0 -> 1; Widget und Unterkontext melden
hingegen in allen drei Faellen 1. Diese Unterkontextangabe beweist
folglich nicht das wirksame Swapverhalten der gesamten Qt-Praesentation.
`GetThreadTimes` liefert quantisierte CPU-Zeit (hier teilweise
15,625-ms-Schritte); einzelne Renderaufrufe mit 0 CPU-ms sind keine
GPU-Messung. Gesamtsummen und Wandzeiten werden getrennt berichtet.

**Isolierter Befund:** Die starke Wartezeit reproduziert sich ohne
Corelast und kehrt beim A/B/A-Swapwechsel zurueck. Globale Swapvorgabe 0
loest sie auf demselben Build bei unveraenderter Szene auf. Damit ist eine
Swap-/Praesentationsbindung belegt, nicht eine neue Physikauswertung
oder alleinige IPO-Ursache; der genaue interne Treiber-Wartepunkt wurde
nicht per GPU-Profiler bestimmt.

Nachweisordner, jeweils `sailforce-performance.json` mit einzelnen
Render-CPU-/Wandwerten und `frameSwapped`-Zeitpunkten:

- `C:\spdeps\guibuild\evidence\G25-frozen-default`
- `C:\spdeps\guibuild\evidence\G25-frozen-swap0`
- `C:\spdeps\guibuild\evidence\G25-frozen-swap1-confirm`

Diese bewusst wiederholte Darstellung eines eingefrorenen Snapshots
ist **nur Ursachenanalyse, kein G25-08-Bildnachweis**. Produktionscode
ist zu diesem Zwischenstand noch nicht geaendert. Enger Folgeschritt:
belegte globale Praesentationskonfiguration konsistent in Anwendung und
Harness uebernehmen, vorhandene Ein-Snapshot-/Praesentationskopplung
beibehalten; erst nach den noch ausstehenden Standard-Coretests und
erneuter Freigabe kurze Liveprobe. Keine Overlay-/Qualitaets- oder
Physikreduktion. Messprozesse abgeschlossen, kurzes CPU-/Buildfenster
an den Hauptagenten zurueckgegeben.

Produktionsfix um 23:10 Uhr ausdruecklich freigegeben und eng umgesetzt:
`Code\src\main.cpp` und der bestehende Renderharness setzen vor
`QApplication` auf einer Kopie von
`QVTKOpenGLNativeWidget::defaultFormat()` ausschliesslich
`setSwapInterval(0)`. Alle uebrigen Formateinstellungen bleiben erhalten.
Live-Abruftakt, hoechstens ein Abruf in flight und die vorhandene
Ein-Snapshot-/Praesentationskopplung sind unveraendert. Keine eingefrorene
Diagnoseschleife im Produkt, kein erhoehter Live-Render-/Abrufgrenzwert.
Noch **kein Build und keine Live-/Leistungsmessung** dieses Fixes:
Standard-Coretests/Kurzprofil laufen im zurueckgegebenen Fenster.
Nach erneuter Freigabe zuerst kurze echte Liveprobe, bei plausiblem
Befund regulaere Dreiboatmessung und gezielte Paritaetschecks sowie
finale produktive EXE. Eine zusaetzliche einzelne 420-Bestaetigungsrunde
erfolgt nur bei bestandenem regulaerem 420 mit weniger als 2 s
Core-Reserve; beide Ergebnisse werden dann berichtet.

## 16. Finaler G25-08-Nachweis: alle drei Boote bestanden

Nach letzter Core-/Buildfreigabe um 23:11 Uhr gegen den finalen
precise/IPO-Core mit exakten Dreieck-Schnellpfaden gebaut.
Laut Liefernachweis des Hauptagenten Standard-Core 6/6 und geaenderter
Clipper nativ instrumentiert 5/5 bestanden; unveraenderte vollstaendige
Endzustaende aller drei Boote nach 100 echten Schritten. Die GUI
uebernimmt diese Core-Liefernachweise ohne eigene Core-Quellenaenderung.

### 16.1 Finaler Build und gezielte Paritaetspruefungen

Alle vier Ziele aus Abschnitt 13.1 in Release mit VS-CMake 3.31 und
`/WX` erfolgreich gebaut. Produktive EXE einschliesslich globaler
Swapvorgabe 0 und aller beschriebenen GUI-Optimierungen:

`C:\spdeps\guibuild\Release\segelphysik_gui.exe`

Finaler Dateistand **17.11.2026, 23:12:07 Uhr**, 4.042.240 Bytes.
Zwischen gezielten Tests, kurzer Liveprobe und regulaerem Dreiboattest
keine weiteren Code-/Build-/Qualitaets-/Physikaenderungen.

Erneut tatsaechlich bestanden:

- Parser: **766 Pruefungen**.
- Echter 420-Historientest: unveraenderliche Snapshots, Livewinkel,
  64er-Cacheverdraengung und alte JSON-Paritaet.
- Bestehender echter Windows-Desktop-Reflexions-/Wasserlinienparitaetstest,
  Nachweisordner `C:\spdeps\guibuild\evidence\G25-final-swap0-reflection`.
- Kurze echte 420-Liveprobe: 5 s Physik, Core 4,3367283 s,
  Wand 5,0094380 s, Frame-p95 30,7445 ms, Maximum 45,2172 ms,
  177/177 Snapshot-Treffer, kein Fallback.
  Render-Median 18,511 ms statt zuvor etwa 33,6 ms; diese Probe ist
  Diagnose, nicht die unten ausgewiesene 60-s-Abnahme.
  Rohdaten `C:\spdeps\guibuild\evidence\G25-final-swap0-profile\sailforce-performance.json`.

### 16.2 Regulaere drei 60-s-Desktopfaelle

Unveraenderte G25-08-Bedingungen: Ryzen 5 8600G/Radeon760M,
Windows-Desktop, Release, Technik, 1280x720, 60 s / dt=0,01 s / Faktor 1,
Core-Hydrostatikgleichgewicht, weltfester Wind 3 m/s aus 65 Grad,
Referenzhoehe 10 m, Luft 1,225 kg/m^3 und 1,5e-5 m^2/s, alle
zugeordneten Segel bei 0 Grad aktiv, voller Hydrodynamikpfad,
gemeinsamer SDP und alle Grenzlinien. Keine Streifen-/Modenreduktion,
ausgelassenen Integratorauswertungen oder Overlay-/Qualitaetsreduktionen.
Unveraenderte Abruf-/Praesentationsdrosseln und ein Abruf in flight.

Keine parallelen Builds, Tests oder Agenten-CPU-Arbeiten waehrend der
Messung. Echte vorbereitete Screenshots vor der Zeitmessung, keine
periodischen Datei-/Screenshotspitzen darin. Framezeiten stammen von
neuen vollstaendigen live praesentierten Snapshots, nicht von der
Frozen-Diagnoseschleife.

| Boot | Core s | Wand s | Frame-p95 ms | Maximum ms | Coregate | Bildgate | Gesamtfall |
|---|---:|---:|---:|---:|---|---|---|
| 420 | 51,7658714 | 60,0113410 | 31,2732 | 44,0536 | Bestanden | Bestanden | Bestanden |
| Folkeboot | 30,3125725 | 60,0063603 | 26,6654 | 32,8688 | Bestanden | Bestanden | Bestanden |
| Optimist | 8,7310538 | 60,0030026 | 26,5658 | 33,2296 | Bestanden | Bestanden | Bestanden |

Jeweils 6000 Schritte / 6001 Zustaende; Snapshot-Treffer
2092 / 2469 / 2499, **kein Fallback**. Ladezeiten
4,7258 / 1,9830 / 1,2037 s. Maximaler Working Set ueber die gesamte
Lebensdauer des gemeinsamen Harnessprozesses: **725.630.976 Bytes**,
kein separat isolierter Speichermesswert je Boot.

420-Core-Reserve **8,2341286 s**. Die Nutzerbedingung fuer einen
zusaetzlichen Bestaetigungslauf (bestanden, aber weniger als 2 s Reserve)
ist damit nicht erfuellt: keine Zusatzrunde. Nicht durch Auswahl eines
besseren Wiederholungslaufs bestanden.

**G25-08 insgesamt bestanden, A-G-00172 erfuellt.**

Massgeblicher vollstaendiger Bericht:

`C:\spdeps\guibuild\evidence\G25-final-swap0-three-boats\sailforce-performance.json`

Enthaelt tatsaechliche Eingaben, reale End-Sailforce-/Winddaten,
Frame-/Phasenrohwerte und Status. Vorbereitete echte Bildbelege im
selben Ordner: `420-sailforce-prepared.bmp`,
`folkeboot-sailforce-prepared.bmp`, `optimist-sailforce-prepared.bmp`.

### 16.3 Bestehender 6000-Schritte-Enddatenvergleich und Abschlussgrenzen

Ohne neuen Harnessumbau die bereits gespeicherten Daten des finalen
regulaeren Laufs gegen den Vor-Clipstand
`G25-final-ipo-three-boats\sailforce-performance.json` verglichen.
Bei **allen drei Booten inhaltlich identisch**:

- physikalischer Anfangszustand;
- vorbereiteter Wind und Lufteigenschaften;
- vollstaendig gespeicherter `sailforce_endzustand` nach 6000 Schritten;
- vollstaendig gespeicherter `wind_endzustand` nach 6000 Schritten.

Keine Wandzeit oder GUI-eigenen Zaehler als Physikgleichheit verglichen.
Die Berichte enthalten keinen vollstaendigen Root-Endzustand mit Pose,
Geschwindigkeiten, allen Bewegungen und Hydrobeitraegen. Daher belegt
dieser vorhandene 6000-Schritte-Vergleich **nur die gespeicherten
Sailforce-/Wind-Endbloecke**, nicht die vollstaendige Endphysik.
Der separate Core-Nachweis vollstaendiger Endzustaende ueber 100 Schritte
bleibt als solcher bezeichnet; keine neue 6000-Schritte-Vollparitaet
ohne vorhandene Daten behauptet.

**Offen bleiben G25-09, vollstaendige GUI-Regression, DPI-/
Gesamtsichtabnahme und persoenliche Nutzerabnahme.**
Historische Format-/Fixturevorbelastungen aus Abschnitt 8/9 wurden
weder behoben noch verdeckt. U-G-000184 und technische Gesamtuebergabe
bleiben bis zu diesen Nachweisen offen; bestandene Leistung allein
ist keine Gesamtfreigabe.

Alle gestarteten Build-/Test-/Messprozesse beendet; finales Produkt
liegt am angegebenen EXE-Pfad. Keine weiteren CPU-Arbeiten/Messungen,
Messfenster an den Hauptagenten zurueckgegeben. Keine Git-Aktion;
GUI-Code, Tests und Dokumentation uncommittet.
