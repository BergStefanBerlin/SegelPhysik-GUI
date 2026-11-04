# GUI-Umsetzungsplan: ausschliesslich Performance der Darstellung

## Nutzerabnahme 04.11.2026

**Nutzerabnahmen: done.** Ausdruecklicher Nutzerauftrag vom 04.11.2026,
09:16 Uhr fuer den gelieferten Stand einschliesslich Performance und
Nutzerinteraktion. Dieser datierte Abschluss ersetzt alle aelteren offenen
Nutzerabnahmestaende in diesem Plan; historische Nachweise bleiben erhalten.
Technische Testluecken und nicht finalisierte Zusatzumfaenge werden dadurch
nicht als nachgewiesen ausgegeben. Kein Commit, Tag oder Push beauftragt.

## Revisionsvorbereitung 04.11.2026: vollstaendige Regression

Der Nutzer hat den gelieferten Hydrostatik-/Hydrodynamik-/Bedienstand
als Major-Meilenstein abgenommen und eine vollstaendige fehlerfreie
Testpruefung beauftragt, ausdruecklich **ohne Commit, Tag oder Push**.
Es wird damit keine neue Versionsnummer vergeben.

| Abschlusspruefung | Ergebnis | Laufzeit | Nachweis |
|---|---|---|---|
| Vollstaendiger GUI-Release-Build und ungefilterter CTest-Lauf | **45/45 bestanden**, 0 Fehler, 0 uebersprungen | 606,67 s | [Build](Nachweise/Meilenstein_2026-11-04/gui-build.log), [CTest](Nachweise/Meilenstein_2026-11-04/gui-tests.log), [Detailprotokoll](Nachweise/Meilenstein_2026-11-04/gui-tests-detail.log), [JUnit](Nachweise/Meilenstein_2026-11-04/gui-tests.xml) |
| Startuebergang/Live-Eingaben: Dynamiklayout 100/150/200 % und Core-Integration, je drei Wiederholungen mit `--repeat until-fail:3` | **12/12 Ausfuehrungen bestanden**, kein Wiederholen bis zum Erfolg | 173,06 s | [Wiederholungsprotokoll](Nachweise/Meilenstein_2026-11-04/gui-startup-regression.log) |
| Produktions-EXE `C:\spdeps\guibuild\Release\segelphysik_gui.exe` | Fenster gestartet, reaktionsfaehig, regulaer geschlossen, **Exitcode 0** | separater Start nach den Grafiktests | [Smokecheck](Nachweise/Meilenstein_2026-11-04/gui-smoke.json) |

Die 45 Tests umfassen 23 GUI-/Integrationspruefungen und 22 eingebundene
Core-Tests. Aktiv: `BUILD_TESTING=ON`, `SEGELPHYSIK_GRAPHICS_GPU_TEST=ON`.
Grafik, Workspaces, dynamisches Layout und Bootinspektion wurden jeweils
bei `QT_SCALE_FACTOR=1/1.5/2` mit echtem Windows/OpenGL geprueft.
GUI-Instrumentierung AUS; Standard-Core und instrumentierter Core
zusaetzlich separat **je 22/22 bestanden**:
[Core-Nachweis](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.10.md#revisionsvorbereitung-04112026-vollstaendige-regression).

Aufruf: `cmake --build C:\spdeps\guibuild --config Release --parallel 3`,
danach `ctest --test-dir C:\spdeps\guibuild -C Release --output-on-failure`.
Laufzeitpfade: `C:\spdeps\installed\x64-windows\bin` auf `PATH`,
`QT_PLUGIN_PATH=C:\spdeps\installed\x64-windows\Qt6\plugins`.
Keine Tests ausgeblendet, keine Bildtoleranzen vergroessert und keine
Zeitlimits angehoben.

### Geschlossene Befunde und erhaltene Pruefvertraege

| Befund | Korrektur / Nachweis |
|---|---|
| `gui_graphics_render_*`: Kursspur AUS stellte das Bild nicht exakt wieder her | Neu geladene Modelle bekamen das bereits aktive Defaultprofil nicht, weil der idempotente Profilsetter korrekt abbrach. `setModel` wendet jetzt das aktive Profil einschliesslich Materialdeckkraft und Texturen auf die neuen Actors an. Pixelgenaue Rueckkehr des Bildes bleibt verlangt und besteht bei allen drei DPI-Stufen. |
| Bootinspektion: Filter schmaler und seitlich versetzt | Filtergeometrie stimmt wieder exakt mit Spaltenbreite und gescrollter Position ueberein. Resize-/Scrollpruefung besteht bei allen drei DPI-Stufen. |
| Veraltete Erwartungen an V4, feste Dynamikeingaben und optische Defaults | Erwartungen auf den bestehenden V6-Livevertrag und die ausdruecklich beauftragten Defaults/Live-Eingaben umgestellt. Sichtbarkeit, Ein-/Ausschalten, Persistenz und Deckkraftstatus bleiben tatsaechlich geprueft. |
| Zeitabhaengige Ablehnung einer Stelleingabe beim Start | Der Adapter verlangte faelschlich bereits einen veroeffentlichten Zustand. Eingaben nach erfolgreicher Kontexterzeugung werden jetzt auch vor dem ersten Snapshot fuer den naechsten Schritt gepuffert. Initialzustand und Historie bleiben unveraendert; abgeschlossene Laeufe weisen Eingaben weiterhin explizit ab. Neuer deterministischer Adaptertest plus zwoelf wiederholte Ausfuehrungen und anschliessender ungefilterter Gesamtlauf bestanden. |
| Zwei bekannte Core-Altfehler | Katalogfixtures aktualisiert und Schema-3-Ableitungen beim Schema-2-Export exakt materialisiert; beide vollstaendigen Core-Konfigurationen gruen, siehe Core-Nachweis. |

Der erste Gesamtlauf nach den uebrigen Korrekturen entdeckte den
Startuebergangsfehler bei 200 %:
[historisches Fehlerprotokoll](Nachweise/Meilenstein_2026-11-04/gui-before-startup-fix.log).
Er ist durch die obigen Korrekturen und neueren Abschlussnachweise
geschlossen; die aelteren Fehlerangaben weiter unten sind nicht der
aktuelle Teststatus.

Gepruefte Git-Basis: GUI `14778a03cb5e2fe073ba3915c384d2dc6a4044a3`,
Core `a8c1ddcdab75366f6eed16b91e672fd4168273ef`, jeweils `main`
mit den uncommittierten Aenderungen vom 04.11.2026.
[GUI-Quell-/Binaerhashes](Nachweise/Meilenstein_2026-11-04/source-manifest.json)
und [Core-Quell-/Binaerhashes](../../CORE/Umsetzungsplaene/Nachweise/Meilenstein_2026-11-04/source-manifest.json)
binden die Nachweise an diesen Arbeitsstand.
Nutzerabnahmen sind `done`. Nicht finalisierte erweiterte Physikmatrizen
und externe Messreferenzen werden nicht als durch diese Regression
nachgewiesen ausgegeben. Windphysik ist nicht Teil dieses Revisionspunkts.
Alle Aenderungen bleiben uncommittiert; keine Git-Veroeffentlichung.

**Stand:** 03.11.2026, Dokumentationsauftrag 20:40 Uhr.
**Status:** Finalisiert und zur Umsetzung freigegeben am 03.11.2026, 20:45 Uhr.
**Arbeitszielversion:** G-0.11; kein Tag-/Release-Auftrag.
Einziger Plan fuer diesen vorgeschlagenen Versionsschritt.
Der [bisherige G-0.10-Plan](Umsetzungsplan_G-0.10.md) behaelt seine
Historie und offenen fachlichen Nachweise; diese werden nicht als
Arbeitspakete in den neuen Performanceumfang uebernommen.

Massgeblich: [Performance-Spezifikation](../Spezifikationen/Spezifikation_Performance_Darstellung.md).
Entwurfskennungen ausschliesslich in der
[Liste](../Spezifikationen/Entwurfspunkte/Performance_Darstellung.json).
Anforderungen A-G-00130 bis A-G-00133; Schritte U-G-000140 bis U-G-000144.

## 1. Umfang und Eintritt

Vieransicht als Funktion unangetastet lassen. Abgeschaltete Szenen
wirklich stilllegen, unveraenderte Profile ohne Render behandeln und
exakte gemeinsame Geometrievorbereitung wiederverwenden.
Keine Tabellen-/Menue-Neugestaltung, keine Kamera-/Layoutarbeit,
keine Core-Physik- oder statische Solveroptimierung.

Eintrittsgate am 03.11.2026, 20:45 Uhr durch Nutzerfreigabe passiert.
Bestehende uncommittete
Arbeiten bleiben erhalten; kein Commit/Push/Tag/Release.

## 2. Geordnete Arbeitsabschnitte

Status: U-G-000140 bis U-G-000144 technisch umgesetzt und nachgewiesen.
Massgebliche Endnachweise stehen in Abschnitt 5. Fachliche Nutzerabnahme
und bestehende Altbefunde werden dadurch nicht geschlossen.

| Arbeitsabschnitt | Entwurfsbezug | Abhaengigkeit | Ergebnis / Nachweis |
|---|---|---|---|
| U-G-000140: Referenz und Aufrufzaehler | A-G-00133 | Finalisierungsgate | Eingefrorene G-0.10-Snapshots, aktuelle Buildbaseline, Zaehler je Szene fuer State/Geometrie/Profil/Render und Bild-/Zahlenreferenzen |
| U-G-000141: Abgeschaltete Ansichten stilllegen | A-G-00130 | U-G-000140 | Alle direkten/indirekten Updatepfade gesperrt; Null-Aufruf-Kriterien; korrekte einmalige Reaktivierung auch im Rueckblick und nach Modellwechsel |
| U-G-000142: Profilabbruch und Renderbuendelung | A-G-00131 | U-G-000140 | Effektive Profile einschliesslich Orthografie vergleichen; Live-Tick/State-Doppelrunden vermeiden; notwendige sichtbare Render bleiben |
| U-G-000143: Gemeinsame exakte Meshvorbereitung | A-G-00132 | U-G-000141, U-G-000142 | Modell-/Stellungsdaten wiederverwenden; getrennte mutable Pipelines/Kameras; Invalidierung und grosse Meshes pruefen |
| U-G-000144: Integration und Nachweis | A-G-00130 bis A-G-00133 | U-G-000140 bis U-G-000143; stabiler Core-Vertrag falls angepasst | Beide Workspaces, Ansichten/Einfache EIN/AUS, Reiterwechsel/Stop/Rueckblick, Bild-/Zahlenvergleich und Zeit-/Zaehlernachweis |

Stilllegung und Profilabbruch haben voneinander keine fachliche
Abhaengigkeit. Sie benoetigen keinen vorherigen Core-Umbau.
Der Meshabschnitt folgt beiden, um Kostenursachen getrennt nachzuweisen.
Core-Arbeit wird allein im
[Core-Plan](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.10.md)
gefuehrt, nicht hier dupliziert.

## 3. Meilensteine und Messmatrix

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| Referenzen bereit | Freigabegate passiert | Reproduzierbare Eingaben und getrennte Timing-/Zaehlerlaeufe vorbereitet |
| AUS bedeutet keine Szenenarbeit | Stilllegung implementiert | Mehrere Live-Ticks/Snapshots erzeugen exakt null abgeschaltete State-/Geometrie-/Profil-/Renderaufrufe; EIN zeigt gewaehlten Zustand ohne Zwischenbild-Nachholen |
| Profile sind idempotent | Profilabschnitt implementiert | Gleiches effektives Profil erzeugt null profilbedingte Render; geaendertes Profil/Kamera/Fenster funktioniert |
| Gemeinsame Vorbereitung korrekt | Meshabschnitt implementiert | Keine wiederholten identischen tiefen Meshvorarbeiten; keine stale Wasserlinie/Elementstellung |
| Integration belegt | Vorige Meilensteine bestanden | Release-Build und betroffene Grafik-/Workspace-/Adaptertests; Bildbelege, numerische Vergleiche und Rohzeiten |

Fuer 420 und Folkeboot jeweils statischen und dynamischen Workspace,
Zusatzansichten EIN/AUS, einfache Darstellung EIN/AUS abdecken.
Profile, Snapshots, Fenstergroesse/DPI und Kamera vor/nach identisch;
Warmup+mindestens fuenf Messungen, Reihenfolge variieren.
Echte Live-Integration zusaetzlich zum isolierten Snapshot-Replay
pruefen. Statischer Workspace-Replay ist kein statischer Solverbenchmark.
EIN ist Regressionstest, keine allgemeine Vieransichtoptimierung.

## 4. Abschluss und Halt

Nachweise gegen die finalisierten Anforderungen abgleichen.
Null-Aufruf-Kriterien und gemessene Zeitersparnis getrennt berichten;
keine Einsparung aus ueberlappenden Zeiten addieren.
Moduslokale Darstellung, gemeinsame Physikeingaben, vollstaendige
Historie und Stopverhalten muessen erhalten bleiben.
Offene fachliche Abnahmen nicht durch Performancetests als erledigt
markieren. Am Ende Ergebnisse und uncommittierten Stand vorlegen;
weitere Themen und Git-Aktionen nur auf neuen Auftrag.

## 5. Implementierung und Nachweise

### 5.1 Mechanismen und Zuordnung

| Anforderung | Spezifikation | Schritte | Implementierung / Abnahmekriterium | Status |
|---|---|---|---|---|
| A-G-00130 | Abschnitt 2 | U-G-000140, U-G-000141, U-G-000144 | `updateSceneActivity` beruecksichtigt die tatsaechlich grosse Ansicht auch nach Tausch; Qt-/Interaktor-/eigene Renderpfade gesperrt; Live-/Rueckblick-/Modellwechseltest und Nullzaehler | technisch nachgewiesen, Abschnitte 5.2-5.4 |
| A-G-00131 | Abschnitt 3 | U-G-000140, U-G-000142, U-G-000144 | Orthografischen effektiven Profilwert vor Gleichheitsvergleich bilden; identische Profile erzeugen keine Profilrunde/Render | technisch nachgewiesen, struktureller Test und Releasezaehler |
| A-G-00132 | Abschnitt 4 | U-G-000140, U-G-000143, U-G-000144 | `DisplayPart` haelt modelllokalen geteilten Cache: Basis und genau letzte Matrixfassung; immutable Mesh-/Material-/Schnittnormaldaten, getrennte Kameras/Actors/Filter; Inputquelle und MTime invalidieren beim Laden | technisch nachgewiesen, Invalidierungstest und exakter Bildvergleich |
| A-G-00133 | Abschnitt 5 | U-G-000140 bis U-G-000144 | Struktureller GPU-Regressionstest, reale Live-/Rueckblick-/Tausch-/Modellwechselpruefung; Release-Vorher/Nachher mit identischen Snapshots | technisch nachgewiesen, vollstaendige Matrix und Paritaet |

Beim Wiedereinschalten wird nur der ausgewaehlte Zustand synchronisiert;
die Core-Historie bleibt vollstaendig. Die freie Szene kann nach
Ansichtstausch selbst in der abgeschalteten Spalte liegen. Die separat
sichtbare angedockte Navigation erhaelt dann nur ihre Zahlen aus dem
gewaehlten Snapshot, keine versteckte Mesh-/Renderaktualisierung.
Tabellen und Kraftmenue lesen den aktuellen Posewert der aktiven grossen
Szene statt eine veraltete Pose der verborgenen freien Szene.

Die Cacheobjekte gehoeren zu den bestehenden Modell-Darstellungsteilen,
nicht zu einem globalen Verzeichnis. Alte Meshfassungen bleiben nur
solange eine Szene sie verwendet; pro Teil wird nur eine aktuelle
transformierte Fassung fuer neue Zugriffe vorgehalten. Bei anderer
Elementmatrix wird eine neue immutable Fassung erzeugt, nicht die
Geometrie anderer Szenen veraendert. Kein Pose-/Wasserlinienschnitt wird
aus einem anderen Zustand uebernommen.

Der Live-Adapter verwendet die finale Core-API CPPI-0113 bis CPPI-0118:
je Lauf eine langlebige `Modellaufbereitung` fuer Integrationsschritte
und eine getrennte fuer JSON-/Rueckblickausgaben. Die Trennung erhaelt
die bisherige Worker-/GUI-Nebenlaeufigkeit; kein gemeinsamer mutabler
Cache und kein Anzeigezugriff, der den Rechenschritt verriegelt.
Die bisher schon getrennten zwei Modellkopien werden durch diese
beiden Kontextbesitzer ersetzt. Bei neuem Modell/Lauf entstehen neue
Kontexte. Ereignisse bleiben im bestehenden Worker-Uebergang, Historie
enthaelt weiterhin alle Rohzustaende. Batch nutzt die Core-interne
Aufbereitung ohne geaenderten Adapter-/Solververtrag.

### 5.2 Regressionen und unveraenderte Altbefunde

Der erste betroffene Testlauf bestand 8/10 Faelle einschliesslich des
neuen strukturellen Tests, Kamera, Workspacewechsel, Dynamiklayout,
Core-Adapter und Kraftdarstellung. Zwei bestehende erweiterte Grafiktests
scheiterten. Beide wurden mit unveraenderten, getrennt archivierten
Quellen G-0.10 und C-0.9 reproduziert:

| Test | Unveraenderte Baseline | Befund |
|---|---|---|
| `gui_water_motion_render` | identischer Fehler nach 10,25 s | `movement model not V4`; bestehende V4-Erwartung im Test, kein neuer Vertragswechsel durch die Performancearbeit |
| `gui_graphics_render_100` | identischer Fehler nach 23,46 s | `420 disabling course trail must restore the exact image`; bestehender Bildvergleichsbefund |

[Baselineprotokoll](Nachweise/Performance_G-0.11_2026-11-03/baseline-g010-c09-tests.log)
und [erster gezielter Lauf](Nachweise/Performance_G-0.11_2026-11-03/initial-targeted-tests.log).
Die alten Assertions wurden nicht geloescht oder abgeschwaecht. Ihre
Reparatur ist kein Bestandteil dieses Performanceplans.
Ein vorheriger isolierter Startversuch ohne `QT_PLUGIN_PATH` blockierte
bereits im Qt-Startdialog; dessen Timeouts sind kein fachliches Testergebnis
und werden nicht als Baseline verwendet.

Der finale GUI-Releasebuild gegen den fertigen Core C-0.10 bestand.
Der gezielte Retest bestand **8/8** Faelle in 136,28 s:
`gui_performance_regression`, `gui_navigation_camera`,
`gui_workspace_tabs_100`, `gui_dynamic_layout_100`,
`gui_workspace_tabs_200`, `gui_dynamic_layout_200`,
`gui_core_integration` und `gui_force_display`.
Der strukturelle Test prueft zusaetzlich Interaktor-Renderunterdrueckung,
Quell-MTime-Invalidierung, Modellwechsel bei AUS und genau eine
Reaktivierung mit zuvor leerer freier Historie.
[Finales Testprotokoll](Nachweise/Performance_G-0.11_2026-11-03/final-targeted-tests.log)
und [Strukturbericht](Nachweise/Performance_G-0.11_2026-11-03/structural-regression.json).

Nach Ergaenzung des vollstaendigen Live-JSON-Vergleichs bestand
`gui_core_integration` erneut in 29,27 s. Der zusaetzliche direkte
`--preparation-only`-Lauf bestand fuer 420 und Folkeboot:
alle vier vollstaendigen JSON-Datensaetze eines 0,03-s-Laufs mit
Hydrodynamik und Testantrieb, danach historische Reihenfolge
3/0/2/1/3, **exakt identisch** zur ungebundenen Core-Kompatibilitaets-API.
Kein Vergleich nur einzelner Positionsfelder und keine neue Toleranz.
[Paritaetsprotokoll](Nachweise/Performance_G-0.11_2026-11-03/live-json-parity.log)
und [erweiterter Integrationstest](Nachweise/Performance_G-0.11_2026-11-03/final-core-integration-tests.log).
Die eigenstaendige Core-Vorhermatrix gegen C-0.9 bleibt im Core-Plan.
Die finale Produktions-EXE wurde erneut gebaut, mit Folkeboot gestartet,
als sichtbares responsives Fenster geprueft und normal geschlossen:
[Smoke-Test, Exitcode 0](Nachweise/Performance_G-0.11_2026-11-03/production-smoke.json).

### 5.3 Release-Vorher/Nachher mit identischen Snapshots

Unveraenderte GUI G-0.10 mit Core C-0.9 gegen aktuellen G-0.11-Arbeitsstand
mit finalem Core C-0.10; MSVC 19.44, Release x64, Qt 6/VTK 9.3,
Ryzen 5 8600G/Radeon 760M, Windows, DPI-Faktor 1.
Beide Messprogramme liefen seriell ohne parallele eigene Builds/Tests
und ohne Core-Messung. Je Fall ein Warmup und fuenf Messrunden mit
sechs eingefrorenen identischen Snapshots; die Abspielreihenfolge
wechselt vorwaerts/rueckwaerts. Zeit pro Snapshot in Millisekunden.
Fenster 1600x1000; isolierte freie Szene 720x480 plus drei 180x160.
Kein Physiklauf innerhalb des gemessenen GUI-Replays.

**28 Faelle je Version:** je Modell sechs isolierte Szenenfaelle
(single/four/hidden_aux x einfach EIN/AUS) und acht MainWindow-Faelle
(statisch/dynamisch x Zusatzansichten EIN/AUS x einfach EIN/AUS).
Die Zaehler werden ausserhalb der Timingabschnitte gelesen; der
eigenstaendige Strukturtest prueft die Semantik unabhaengig vom Timing.
Vorbereitungs-/State-/Profilzeiten sind getrennt in den Rohdaten;
ueberlappende Teilzeiten werden nicht addiert. Kaltladezeiten je Szene
sind Einzelbeobachtungen, keine fuenffach abgesicherten Ladebenchmarks.

Mediane des echten MainWindow-`displayState`, **vorher -> nachher**:

| Modell | Workspace | Zusatzansichten | Einfach EIN, ms | Einfach AUS, ms |
|---|---|---|---:|---:|
| 420 | Statisch | EIN | 402,60 -> 78,39 | 384,95 -> 65,36 |
| 420 | Statisch | AUS | 365,64 -> 24,22 | 354,21 -> 23,79 |
| 420 | Dynamisch | EIN | 358,82 -> 75,58 | 347,81 -> 83,76 |
| 420 | Dynamisch | AUS | 350,31 -> 23,86 | 348,47 -> 23,00 |
| Folkeboot | Statisch | EIN | 84,75 -> 83,00 | 81,64 -> 82,20 |
| Folkeboot | Statisch | AUS | 86,85 -> 30,30 | 81,66 -> 26,95 |
| Folkeboot | Dynamisch | EIN | 102,39 -> 87,42 | 147,60 -> 83,74 |
| Folkeboot | Dynamisch | AUS | 125,88 -> 28,29 | 102,58 -> 27,06 |

Alle AUS-Faelle haben vollstaendig getrennte verbesserte Min-/Max-Spannen.
420 mit AUS ist hier 14,68-15,15x schneller; Folkeboot mit AUS
2,87-4,45x. Das ist GUI-Replay, keine Beschleunigung der gesamten
Simulation um denselben Faktor.
Folkeboot/Statisch/EIN ist praktisch unveraendert:
Einfach AUS 79,16..82,70 -> 76,86..92,56 ms mit leicht schlechterem
Median; Einfach EIN 83,15..86,42 -> 81,17..87,41 ms.
Dort keine belastbare Verbesserung und keine reproduzierbare Regression
aus ueberlappenden Spannen behaupten.

Die wiederholte identische Profilrunde ueber vier Szenen sank beim
420 von 30,62-32,22 auf rund 0,002 ms und beim Folkeboot von
24,48-25,78 auf 0,002-0,003 ms. Sie erzeugt **null** Profilneuaufbauten
und **null** profilbedingte Render. Das ist eine Teilzeit und wird
nicht zusaetzlich zur `displayState`-Ersparnis aufaddiert.

### 5.4 Exaktheit, Rohdaten und verbleibende Punkte

**16/16 MainWindow-Faelle:** alle sichtbaren Tabellenzellen der sechs
Referenzzustaende exakt gleich; alle finalen freien Szenenbilder
pixelgenau gleich (null geaenderte Pixel, keine Bildtoleranz).
Bilddimensionen und SHA256 beider BMP-Referenzen stehen im
[Vergleich](Nachweise/Performance_G-0.11_2026-11-03/comparison.json).
Die identischen Bilder sind platzsparend als PNG je Modell/Workspace/
Ansichten/Einfach-Kombination im selben Nachweisordner gespeichert.
Beispiel:
[420 dynamisch, Zusatzansichten AUS, einfach EIN](Nachweise/Performance_G-0.11_2026-11-03/420.2.0.1.png).

Jede abgeschaltete isolierte Szene hat ueber 36 Snapshot-Anforderungen
**0 State-, 0 Mesh-, 0 Profil- und 0 Renderarbeit**. Aktive Szenen haben
36 State-/Renderaufrufe und keine Profilneuaufbauten. Nach der ersten
gemeinsamen Stellungsaufbereitung entstehen bei festen Matrizen keine
weiteren tiefen Meshvorbereitungen; geaenderte Matrizen und Quell-MTime
werden im separaten Strukturtest invalidiert.

Reproduzierbare Quellen und Belege:

- [Vorher-Rohzeiten](Nachweise/Performance_G-0.11_2026-11-03/before-optimization.json),
  [Nachher-Rohzeiten](Nachweise/Performance_G-0.11_2026-11-03/after-optimization.json),
  [CSV mit Faktoren](Nachweise/Performance_G-0.11_2026-11-03/timing-summary.csv)
  enthalten alle Wiederholungen bzw. Median/Min/Max.
- [Messrunner](Nachweise/Performance_G-0.11_2026-11-03/audit.cpp),
  [Messbuild](Nachweise/Performance_G-0.11_2026-11-03/CMakeLists.txt),
  [Vergleichsskript](Nachweise/Performance_G-0.11_2026-11-03/compare.py),
  [Build-/Snapshot-Hashes](Nachweise/Performance_G-0.11_2026-11-03/build-manifest.json).
  Runnerargumente: Modellordner, Ausgabe-JSON, Snapshotordner mit
  `420.json` und `folkeboot.json`; abgelegte `snapshots-*.json` dafuer
  entsprechend kopieren. CMake-Parameter `GUI_SOURCE`, `CORE_SOURCE`,
  `CORE_LIBRARY`, `AUDIT_CURRENT=ON`, `AUDIT_OPTIMIZED=OFF/ON`,
  `AUDIT_REVISION`. Laufzeit: `QT_QPA_PLATFORM=windows`,
  `QT_SCALE_FACTOR=1`, passender `QT_PLUGIN_PATH` und DLL-Suchpfad.

Technischer Performanceumfang abgeschlossen; kein Commit/Push/Tag.
Die zwei exakt reproduzierten alten GUI-Testfehler und die zwei
bekannten Core-Altfehler bleiben sichtbar. Kein statischer Solver
optimiert. Statischer Workspace-Replay ist kein statischer
Solverbenchmark. Nutzerpruefung und die alten offenen fachlichen
Physiknachweise aus G-0.10/C-0.9 bleiben unveraendert offen.

## 6. Nachpruefung: Kraftauswahl und Ansichtswechsel

Nutzerbefund vom 03.11.2026 nach Abschnitt 5: ausgewaehlte Kraefte
fehlten; Wechsel zwischen freier und einfacher Ansicht blockierten
die Anwendung. Windows meldete AppHangB1, keinen nachgewiesenen
nativen Absturz. Der folgende Nachtrag korrigiert die betroffenen
Vertraege; er eroeffnet keinen weiteren Performanceumfang.

| Befund | Ursache und Korrektur | Tatsaechlicher Nachweis |
|---|---|---|
| Erneute Anzeige desselben Zustands spielte die gesamte Bewegungshistorie ab | `motionStateCount` ist eine Anzahl, kein Index. Ruecksprung nur bei Anzahl groesser `index + 1`; echte Rueckblicke bleiben erhalten | Vor Korrektur reproduziert; neuer `motionInputs`-Zaehler weist null alte Eingaben bei unveraendertem Zustand nach |
| Gewicht/Auftrieb trotz Auswahl nicht gezeichnet | Core erwartete im dynamischen Bilanzinventar irrtuemlich `stationaer.fremdkraft`; GUI unterdrueckte deswegen korrekt unvollstaendige Resultanten | Core korrigiert die ID am Ursprung; reale dynamische Pfeil-Actors und Bildaenderungen fuer EIN/AUS geprueft |
| Fremdkraft-/Ersatz-Segelpfeil fehlte weiterhin | Dynamische Snapshots lieferten keinen Ersatz-SDP. Core liefert den tatsaechlichen modellfesten Ansatzpunkt; keine GUI-Ersatzannahme | Aktiver Ansatzpunkt in Batch/Live/Historie; inaktive und aktive Nullkraft ohne erfundene Position; echter Segelpfeil samt Bildaenderung |
| Freie/einfache Ansicht im Wechsel | Reale Maus-Press/Release/Doppelklick-Folgen, grosse einfache Szene bei abgeschalteter Spalte, Kraftwechsel bei verborgener freier Szene | 18 Wechsel, richtiger ausgewaehlter Zustand, zurueckkehrender Kraftpfeil und null Wiederabspielung alter Bewegungseingaben |

Der vorangegangene exakte Vorher-/Nachher-Bildvergleich hatte auch die
alte fehlende Kraftdarstellung reproduziert. Er belegte keine
fachlich korrekte Checkboxwirkung. Diese Luecke wird nun durch echte
Actors und Bildaenderungen statt allein durch Optionswerte geschlossen.
Der bestehende Projektions-/Verdeckungsvertrag bleibt unveraendert:
kein Pfeil fuer eine Nullprojektion und keine neue Durchkoerperdarstellung.

Abschliessend **8/8 gezielte GUI-Tests bestanden**: Performance-Regression,
Navigation/Kamera, Workspace-Reiter und dynamisches Layout jeweils bei
100/200 Prozent sowie Core-Integration und Kraftdarstellung.
Core zusaetzlich **3/3**: Modellaufbereitung, Simulation-Gesamtlauf,
Kraft-/Momentenbilanz. Keine physikalischen Kraft-/Momentenformeln,
Integratorschritte oder Solververfahren geaendert. Korrigierte
Bilanzmetadaten und der dynamische Ersatz-SDP sind beabsichtigte
JSON-Unterschiede zur alten Referenz; historische Rohmessungen bleiben
unveraendert.

Belege:
[GUI-Testprotokoll](Nachweise/Kraftauswahl_Ansichtswechsel_2026-11-03/gui-tests.log),
[struktureller Bericht](Nachweise/Kraftauswahl_Ansichtswechsel_2026-11-03/report.json),
[Core-Nachtrag](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.10.md#10-nachpruefung-dynamisches-kraftinventar-und-ersatz-sdp).
Persoenliche Nutzerpruefung bleibt offen; kein Commit/Push/Tag.

Der normale Release-Build unter
`C:\spdeps\guibuild\Release\segelphysik_gui.exe` wurde anschliessend
aktualisiert. Der anfangs durch die geoeffnete Anwendung gesperrte
Build wurde erst nach deren Beendigung ersetzt; kein fremder Prozess
wurde beendet. Separater Produktionstest: Folkeboot laden, sichtbares
antwortendes Fenster und geordnetes Schliessen mit Exitcode 0.
[Start-/Ende-Nachweis](Nachweise/Kraftauswahl_Ansichtswechsel_2026-11-03/production-smoke.log).
