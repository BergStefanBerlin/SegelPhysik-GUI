# GUI-Umsetzungsplan G-0.21: Modellformat 0.3

**Stand:** 12.11.2026. **Status:** GUI-Postgate nach dataset-only Core-Cutover bestanden; keine Git-Veröffentlichung.
Einziger maßgeblicher GUI-Plan für G-0.21.
Maßgeblich ist die
[GUI-Spezifikation](../Spezifikationen/Spezifikation_GUI_Modellkatalog_Anbindung.md#gui-betrieb-mit-modellformat-03);
Core-Lieferungen und der physische Cutover sind im getrennten
[Core-Plan C-0.21](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.21.md)
verbindlich erfasst.

**Projekttrennung:** Dieser Plan verantwortet ausschließlich GUI-Code,
GUI-Tests und GUI-Abnahme. Die GUI verschiebt oder benennt keine Modellordner.
Der dataset-only-Wechsel aus `Modelle-NEW` in die vorhandene Repository-
Wurzel `Modelle` wurde einmalig gemäß Core-Plan ausgeführt; dieser Plan
dokumentiert GUI-Vor- und -Nachtests.

## 1. Bestand, Umfang und Lieferabhängigkeiten

Die GUI benutzt für Format 0.3 ausschließlich den Core-Modellkatalog,
Core-Diagnosen und die öffentliche Fingerprint-API. Sie erhält keine eigene
Schema-, JSON- oder Hashimplementierung. Das Core-Lieferpaket umfasst:

- C-0.21 / U-C-000131: schema-validierter 0.3-Import und Nutzungsstatus;
- C-0.21 / U-C-000132: aktiver Katalog, Modellstruktur und Geometrie;
- C-0.21 / U-C-000133: Fingerprintprüfung und öffentliche
  Bereichsaktualisierung;
- C-0.21 / U-C-000134: gemeinsame Laufzeitkonfiguration.

Bestandsprüfung von `GUI/CMakeLists.txt` und `GUI/vcpkg.json` am 10.11.2026:
die GUI bindet Core direkt als Unterprojekt ein und deklariert Qt6/VTK, aber
keine Schema- oder SHA-256-Bibliothek. Neue Validator-/Hash-Abhängigkeiten
gehören daher in den Core-Abhängigkeits- und Auslieferungspfad; eine zweite
GUI-Abhängigkeit oder Implementierung wird nicht eingeführt.

Die finale GUI-Integration hängt vom erfolgreichen Core-Pretest
U-C-000135 ab. Der Cutover hängt wiederum von bestandenem GUI-Pretest
U-G-000160 ab. Änderungen am Core-Liefervertrag werden nicht durch einen
GUI-Ersatzleser kompensiert; sie müssen in Core geklärt und nachgeliefert
werden.

Projektweit vor Vergabe geprüft: vorher höchste A-G-00146 und U-G-000155;
dieser Plan vergibt U-G-000156 bis U-G-000161. Die Anforderungen
A-G-00147 bis A-G-00150 sind in der GUI-Spezifikation geführt.

## 2. Geordnete Umsetzung

| U-ID | Umsetzungsschritt | Anforderungen | Abhängigkeiten | Nachweis/Abnahmekriterium |
|---|---|---|---|---|
| U-G-000156 | GUI-Datenpfad auf die Core-0.3-Katalog-/Inspektionsdaten umstellen; Modellidentität, Struktur, Geometrie, Status, Diagnosen und Fingerprintfassung ausschließlich vom Core beziehen. Direkte 0.3-Datei- und Schema-Leser entfernen. | A-G-00147 | extern: C-0.21 / U-C-000131, U-C-000132, U-C-000133 | GUI baut gegen den gelieferten API-Vertrag; statische Prüfung findet keinen GUI-eigenen Parser, Schema-Validator oder Hashcode für Format 0.3. |
| U-G-000157 | Freigabezustände für Ansicht und Simulation gemäß Core-Prüfergebnis umsetzen: schema-/Identitäts-/Ressourcenfehler sperren beides; aktueller Visualisierungs-Hash erlaubt Ansicht trotz anderer Hashfehler; solche Fehler sperren weiter die Simulation und bleiben sichtbar. | A-G-00147 | U-G-000156 | Positive und negative UI-Tests bestätigen Sichtbarkeit, Diagnose und Simulationstaste/Startverhalten ohne Sonderfallback. |
| U-G-000158 | Kontextmenü „Visualisation-Hash erneuern“ für Modelle mit schema-validen Modelldateien und vom Core als ungültig/veraltet gemeldetem Visualisierungs-Hash einführen; ausschließlich die Core-API für `visualisation` aufrufen. Nach Erfolg Katalog, Ansicht und Status neu laden; Fehler unverändert sichtbar lassen. | A-G-00148 | U-G-000156 | Test belegt Verfügbarkeit nur bei Bedarf, Update genau eines Bereichs, erneute Core-Prüfung und unveränderten Zustand bei Fehler. |
| U-G-000159 | Gemeinsame `defaults.json`-Felder und Vorrangregeln für Startpfad/Schemaauflösung integrieren; Ordnerwahl, Dateiüberwachung und Cacheinvalidierung auf Modelldaten, Validierungsdaten, Ressourcen und beide Schemas erweitern. | A-G-00147, A-G-00149 | extern: C-0.21 / U-C-000134 | Benutzerwahl/Test-Override/Umgebungsvariable/Datei folgen der gemeinsamen Priorität; Pfad- oder Dateiänderungen lösen Core-Neuprüfung und Cacheinvalidierung aus. |
| U-G-000160 | GUI-Vorab-Build und relevante vollständige Regression gegen `Modelle-NEW` ausführen; alle elf Modelle, Fehlermatrix, Aktualisierungsaktion, Überwachung und Defaults prüfen. Ergebnisse und Inventar als Cutover-Vorbeleg dokumentieren. | A-G-00147, A-G-00148, A-G-00149, A-G-00150 | U-G-000156 bis U-G-000159; extern: C-0.21 / U-C-000135 bestanden | GUI-Build und alle relevanten Tests bestehen, Core-Pretest liegt vor und Modellinventar ist unverändert. Ohne diese Nachweise kein Cutover. |
| U-G-000161 | Nach dem Core-Cutover GUI-Build und vollständige relevante Regression gegen den finalen `Modelle`-Pfad wiederholen; alle elf Modelle und Pfad-/Watcher-/Cacheverhalten verifizieren. | A-G-00147, A-G-00150 | U-G-000160; extern: C-0.21 / U-C-000136 und U-C-000137 bestanden | GUI findet und prüft alle elf Modelle; Ansichts-/Diagnose-/Simulationsverhalten ist identisch zum Vorab-Gate; Core- und GUI-Posttests sind dokumentiert. |

## 3. Testfälle

| Test | Prüfung | Erwartung |
|---|---|---|
| G21-01 | Alle elf validen Modelle im 0.3-Katalog | Darstellung und Auswahl über Core; GUI liest die Modelldateien nicht selbst. |
| G21-02 | Schema-, Identitäts-, Ressourcen- und Pfadfehler | Core-Diagnose ist sichtbar; weder Modellgeometrie noch Simulation werden freigegeben. |
| G21-03 | Visualisierungs-Hash aktuell, anderer Bereichshash veraltet | Vollständige Ansicht bleibt verfügbar; Diagnose bleibt sichtbar und Simulation ist gesperrt. |
| G21-04 | Schema-valide Modelldateien, Visualisierungs-Hash veraltet/ungültig | Geometrie bleibt verborgen; die dafür vorgesehene Rechtsklickaktion ist verfügbar. Schema-ungültige Modelle erhalten keine Aktualisierungsaktion. |
| G21-05 | Rechtsklick-Aktualisierung bei Erfolg und absichtlich gescheitertem atomarem Replace | Erfolg aktualisiert ausschließlich `visualisation`, lädt Katalog und Geometrie neu und gibt Simulation frei. Fehler zeigt `CORE-FPR-0003`, lässt Validierungsdatei bytegleich und hält Ansicht/Simulation gesperrt; keine Erfolgsmeldung. |
| G21-06 | Jeweils Änderung und Wiederherstellung von Modelldatei, Validierungsdatei, referenzierter Geometrie sowie `schema_data.json` und `schema_validation.json` | Echter `QFileSystemWatcher`-Pfad löst Core-Neuprüfung, Status-/Diagnosewechsel und erneute Freigabe nach Reparatur aus; nur temporäre Testkopien werden geändert. |
| G21-07 | `defaults.json` fehlt, ist syntaktisch ungültig oder nicht lesbar; Datei wird neu angelegt/geändert; unbekannter Defaults-Schlüssel ist vorhanden | Änderung bzw. Ausfall wird sichtbar gemeldet, es gibt keinen stillen Ersatzpfad; der beobachtete Elternordner erkennt Wiederherstellung. Unbekannte Schlüssel bleiben zulässig. Relative und absolute Werte werden mit den Core-Defaults-Tests geprüft. |
| G21-08 | Expliziter GUI-Ordner, `SEGELPHYSIK_TEST_MODEL_ROOT`, `SEGELPHYSIK_MODEL_ROOT` und Defaults-Datei konkurrieren; Schema-Override/Umgebungsvariable/Defaults ebenfalls konkurrieren | Reihenfolge explizit → Test-Override → Umgebungsvariable → Datei ist für Modell- und Schemapfad belegt. Ein ausdrücklich im GUI gewählter Ordner bleibt auch nach Defaults-/Umgebungsänderung aktiv; nach Rückkehr in den konfigurierten Modus bewirkt Defaults-Änderung den Katalogwechsel. |
| G21-09 | Alle Tests vor und nach Cutover | Vor dem Cutover wird `Modelle-NEW`, danach der aktive Root `Modelle` genutzt; Legacy-Kompatibilitätstests verwenden `Modelle_alt`, ohne die Originaldaten zu verändern. |

Tests, die Dateien aktualisieren, verwenden ausschließlich temporäre
Modellkopien. Die elf Originalmodelle werden vor/nach der jeweiligen
Regression inventarisiert und per SHA-256 verglichen.

## 4. Meilensteine und projektübergreifende Gates

| Meilenstein | Eintritt | Abnahmekriterium |
|---|---|---|
| G1 – Core-Katalogintegration | Core U-C-000131/132 geliefert | G21-01 und G21-02 bestehen; kein GUI-eigener Datenleser. |
| G2 – Nutzung und Aktion | Core U-C-000133 geliefert | G21-03 bis G21-05 bestehen; Kontextaktion verwendet denselben Core-Funktionsvertrag. |
| G3 – Laufzeitpfade und Beobachtung | Core U-C-000134 geliefert | G21-06 bis G21-08 bestehen; gemeinsame Konfigurationssemantik nachgewiesen. |
| G4 – Vorab-Cutover-Gate | U-G-000156 bis U-G-000159 sowie Core U-C-000135 | GUI-Build und vollständige relevante GUI-Tests gegen `Modelle-NEW` bestehen; keine Änderung am Modellbestand. |
| G5 – Post-Cutover-Abnahme | Core U-C-000136 und U-C-000137 | GUI-Build und relevante vollständige Regression am finalen `Modelle`-Pfad bestehen; Core-/GUI-Verhalten und Inventare sind dokumentiert. |

Kein GUI-Schritt darf den physischen Ordnerwechsel selbst durchführen. Ist
das Core-Vorab-Gate nicht grün, wird G4 nicht abgenommen. Scheitert ein
Post-Cutover-Test, bleibt `Modelle_alt` erhalten und der dokumentierte
Core-Rückfallpfad ist zu verwenden; kein GUI- oder Core-Ordner wird
überschrieben oder gelöscht.

## 5. Validierung

Die GUI verwendet ihre vorhandene native CMake-/CTest-Konfiguration und
vorhandene Testselektoren. Der konkrete Buildpfad und Testumfang werden vor
der Umsetzung anhand der aktuellen Projektkonfiguration festgehalten. Für
G4 und G5 werden sowohl zielgerichtete Katalog-/GUI-Tests als auch die
vollständige relevante GUI-Regression ausgeführt. Fehlgeschlagene Tests
blockieren den jeweiligen Cutover-Gate; Python/pytest ist kein GUI-Testpfad.
Für das Vorab-Gate wird `SEGELPHYSIK_MODELFORMAT03_TEST_ROOT` explizit auf
`Modelle-NEW` gesetzt; nach dem Cutover zeigt es auf das aktive `Modelle`.

## 6. Umsetzungsstatus

### Vorläufige Ladephasen-Entscheidung

Auf ausdrückliche Nutzerentscheidung liefert die erste Auswahlphase für
Format 0.3 zuerst Core-Struktur und Darstellung, ohne Physikinspektion.
Anschließend lädt der Core die Physik ausschließlich für die ausgewählte
Yacht in einem Hintergrundthread; erfolgreich geladene Physik darf im
Modellcache wiederverwendet werden. Es gibt kein vorsorgliches Laden.
Referenzmarker `V`, `S`, `D` und `A` zeigen nur das Vorhandensein einer
Referenz an. Vollständige Aerodynamikprüfung/-Hash und Windfelddaten bleiben
vorläufig ausgenommen. Dies dokumentiert die Umsetzungsentscheidung, ändert
aber nicht die finale GUI-Spezifikation.

| Schritt | Tatsächlicher Zwischenstand |
|---|---|
| U-G-000156 | Katalog-/Inspektionsdaten über Core angebunden; isolierter Verbraucher- und 0.3-Katalogtest bestanden. Vollständiger Release-Build einschließlich aller GUI-Testziele bestanden. |
| U-G-000157 | Getrennte Freigaben implementiert. MainWindow-Integration bestätigt: veralteter Visualisierungs-Hash hält Geometrie in allen Ansichten verborgen, Diagnose sichtbar und Simulation gesperrt; eine schema-ungültige Modell-, Validierungs- oder Ressourcendatei sperrt nach Watcher-Neuprüfung und wird nach Reparatur erneut freigegeben. |
| U-G-000158 | Kontextaktion und Worker verwenden ausschließlich die Core-API mit `visualisation`. Integration bestätigt Aktionsangebot nur für schema-valide Modelle mit veraltetem Hash, erfolgreichen Worker-Update mit neuer Katalog-/Geometrieladung und Freigabe. Unter Windows erzwingt ein Dateihandle einen Replace-Fehler: `CORE-FPR-0003` ist sichtbar, Validierungsdatei bytegleich, Simulation/Geometrie bleiben gesperrt. |
| U-G-000159 | MainWindow-Integration bestätigt Watcher-Neuprüfung und Freigabe nach Reparatur für Modelldaten, Validierungsdatei, referenzierte Geometrie, beide Schema-Dateien sowie Defaults-Datei. Geprüft sind Defaults-Wechsel des Katalogroots, fehlende/ungültige/unlesbare Konfiguration mit sichtbarer Fehlermeldung ohne stillen Ersatzpfad und Wiederherstellung, exakte Modell-/Schema-Priorität sowie Vorrang eines explizit gewählten GUI-Ordners. Core-Tests decken relative/absolute Pfade und unbekannte Defaults-Schlüssel ab. |
| U-G-000160 | Technischer Vorab-Gate am 12.11.2026: vollständiger GUI-Release-Build; im jüngsten vollständigen CTest-Lauf bestanden 59/62 Tests. Die drei dort auffälligen DPI-Render-/Workspace-Tests bestanden in isolierten Wiederholungen. `gui_sail_configurator_420`, Montage, beide Format-0.3-Gates sowie Core-Integration bestehen. Der Montagetest läuft auf der gefilterten lokalen Kopie; die Format-0.3-Tests verwenden temporäre Kopien. |
| U-G-000161 | Nach dem dataset-only Core-Cutover GUI-Release-Build und vollständige CTest-Regression gegen `Modelle` bestanden: 62/62. Die GUI testet Legacy-Kompatibilität aus `Modelle_alt`; die Format-0.3-Tests verwenden den aktiven Root und temporäre Kopien. |

### Bisherige Nachweise und Buildabhängigkeiten

- Ein isolierter Qt-Core-Verbraucher baut die echten
  `model_catalog.cpp`-/`model_catalog_tests.cpp`-Quellen gegen die aktuelle
  Core-Bibliothek. `--model-format-03-single` mit einer temporären
  bleistift-Kopie: **bestanden**, auch nach der letzten Core-Neukompilierung.
  Er belegt Katalogvertrag, Ansicht bei veraltetem Physikfingerprint,
  gesperrte Ansicht bei Visualisierungs-/JSON-Fehler, selektive
  Visualisierungsaktualisierung ohne Physikfreigabe sowie Watcher-Pfadlisten
  einschließlich wiederherstellbarer fehlender Defaults. Er belegt nicht
  die vollständige GUI oder das tatsächliche Ereignis-/Workerverhalten.
- Zusätzlich wurden die geänderten `main_window.cpp`- und
  `main.cpp`-Quellen mit echten Qt-/VTK-Quell- und generierten
  VTK-Modulheadern als Objektziel erfolgreich kompiliert
  (`/W4 /WX /permissive- /utf-8`). Keine Ersatzheader oder API-Stubs
  verwendet. Dieser Compile-Nachweis ersetzt keinen gelinkten GUI-Build.
- Nach erfolgreicher VTK-Wiederherstellung fand die Konfiguration in
  `build\g021` Qt und VTK. Die Widgets-Tests `gui_navigation` und
  `gui_graphics_dialog` liefen zunächst in Timeouts, weil Qt-DLLs nicht im
  Prozesssuchpfad lagen. Die CMake-Testumgebung ergänzt jetzt die vcpkg-
  Laufzeitverzeichnisse; beide Tests bestehen anschließend ohne manuelle
  `PATH`-Änderung.
- `vcpkg-configuration.json` bindet deshalb **nur für gl2ps** den offiziellen
  vcpkg-Registry-Stand
  `c93431c51b046bd9f74356e8ac6cbf10586f7310` ein
  ([Upstream-Korrektur](https://github.com/microsoft/vcpkg/commit/c93431c51b046bd9f74356e8ac6cbf10586f7310)).
  gl2ps bleibt bei 1.4.2, Portrevision #5; andere Pakete behalten die
  bisherige Manifest-Baseline. Keine Prüfsumme wurde umgangen.
  gl2ps wurde damit erfolgreich gebaut und installiert.
- Nach den protokollierten vcpkg-Fehlern wurde der serielle Installationslauf
  erfolgreich abgeschlossen; VTK ist für Debug und Release installiert.
  CMake fand Qt und VTK. Der vollständige GUI-Release-Build einschließlich
  `segelphysik_gui.exe` und aller GUI-Testziele bestand. CMake meldete dabei
  Warnungen aus VTKs `vtkcompiletools`-Cross-Compilation-Erkennung und
  `dumpbin`-Prüfung für GLEW; sie verhinderten Konfiguration, Kompilierung
  oder Linken nicht.
- Zuletzt erneut ausgeführt am 12.11.2026: vollständiger GUI-Release-Build
  und CTest mit GPU-Gates an. Der Gesamtlauf führte alle 62 Tests aus;
  zunächst scheiterten drei DPI-Inspektionsläufe an einer Testannahme und
  daran, dass eine gewählte Elementzeile ihre abgeleiteten Meshes nicht in
  die Ansicht übernahm. `BoatInspection::updateMesh` berücksichtigt jetzt
  ausgewählte normale Elementzeilen; der Test wählt gezielt das Element mit
  abgeleitetem Hydrostatikvolumen. Die drei Tests bestanden anschließend
  einzeln bei 100 %, 150 % und 200 % Skalierung. Alle übrigen 59 Tests
  bestanden bereits im vollständigen Lauf; damit sind alle 62 Tests nach
  der Korrektur nachgewiesen.
- Die zuvor offenen interaktiven Gates bestehen: `gui_sail_configurator_420`
  (ca. 311 s), `gui_unified_montage` (ca. 32 s),
  `gui_model_format_03_interactions` (ca. 14 s) und `gui_model_format_03`.
  Der Montagetest hasht nur Visualisierungsdaten und vorhandene
  `modells_data.json`-Dateien in seiner isolierten Arbeitskopie; die
  Format-0.3-Gates verwenden temporäre Modellkopien.
- Der zuvor fehlschlagende `gui_model_catalog`-Test scheiterte nicht an
  abweichender Segelgeometrie: die direkte Schema-3-Migration des 0.2-Modells
  `420` übernahm das Legacy-Metadatenobjekt `modell` unverändert.
  `konvertiere_modell_v3` normalisiert das 0.2-Dokument jetzt vor der
  Schema-3-Validierung. Core-Regression und GUI-Katalogtest bestehen nach
  der Korrektur.
- Beim erneuten vollständigen Lauf bestanden 59/62 Tests. `gui_graphics_render_100`
  meldete einen QSettings-Schreibfehler, `gui_graphics_render_150` einen
  Watcher-/Modellladefehler und `gui_workspace_tabs_200` erreichte sein
  zuvor geltendes 90-s-Ctest-Zeitlimit. Das Zeitlimit der Workspace-Tests
  wurde angesichts der gemessenen Laufzeit auf 180 s angehoben. Alle drei Tests bestanden danach isoliert
  (100 %: 97,71 s; 150 %: 106,84 s; 200 %: 52,84 s). Damit sind die
  Einzelgates nachgewiesen, aber dieser Lauf war nicht vollständig grün.
- Nach dem dataset-only Cutover wurden GUI-Release-Build und vollständige
  CTest-Suite gegen die endgültigen Pfade erneut ausgeführt: **62/62 Tests
  bestanden**.
- Vor und nach dem vollständigen GUI-Vorab-Testlauf wurde das
  `Modelle-NEW`-Dateiinventar mit Pfad, Größe und SHA-256 verglichen:
  **153 Dateien, unverändert**. Das Format-0.3-Testroot zeigte damals auf
  `Modelle-NEW`; Legacy-Tests verwendeten den damaligen Root `Modelle`.
- Manueller Smoke-Test vor dem Cutover in der gestarteten Anwendung am
  11.11.2026: Modellordner `Modelle-NEW` gewählt, alle elf Modelle sichtbar; beim 420er
  waren die Segel in Bootsauswahl, statischer und dynamischer Ansicht
  sichtbar. Nutzer meldete „alles soweit OK“. Die anschließenden G21-
  Worker-/Watcher-/Prioritätstests sind zusätzlich automatisiert bestanden.
- `gui_model_format_03_interactions` besteht mit interaktivem Windows-/VTK-
  Backend (1/1). Der Test arbeitet ausschließlich mit temporären 0.3-Kopien.
  Er belegt Erfolg und atomaren Fehler des Workerpfads, unveränderte
  Validierungsdaten und sichtbare Core-Diagnose; Änderungen und Reparaturen
  an Modelldaten, Validierungsdatei, Ressource und beiden Schemas; Defaults-
  root-Wechsel, fehlende/ungültige/unlesbare Defaults samt sichtbarer Meldung
  und Wiederherstellung; Modell-/Schema-Priorität und Vorrang des expliziten
  GUI-Ordners. Core-Priorität und relative Pfadauflösung sind zusätzlich
  durch Defaults-Tests abgedeckt. Der Test ist bei
  `SEGELPHYSIK_GRAPHICS_GPU_TEST` registriert und benötigt den interaktiven
  OpenGL-Desktop.
- Die frühere 420-Testaussage „keine Bewegung“ widersprach den expliziten
  `Bewegung`-Einträgen für `Schwert` und `Ruder` in der Modelldatei.
  Der Test prüft nun diese deklarierten Bewegungen; `gui_420_model_02`
  besteht. Der ursprüngliche Testfehler wurde ebenfalls auf unverändertem
  `HEAD` reproduziert.

Der Nutzer hat die einmalige Aktualisierung der elf
`modells_validation.json` ausdrücklich freigegeben. Core hat diese Dateien
über die gemeinsame API aktualisiert; die vorherigen Dateien wurden im
Session-Backup anhand des SHA-256-/Größenmanifests verifiziert. Beim
dataset-only Cutover blieb die `Modelle`-Git-Wurzel samt `.git` am Ort;
Legacy-Ordner liegen in `Modelle_alt`, die elf neuen Modelle und beide
Schemas aktiv in `Modelle`. Defaults-Datei und gespeicherter GUI-Modellordner
zeigen auf die neue Wurzel. Die selektiven SHA-256-Werte der elf
`modells_data.json`-Dateien und der Visualisierungsdateien blieben vor und
nach den Tests unverändert. Keine Git-Veröffentlichung vorgenommen.
