# GUI-Umsetzungsplan G-0.13: Segelkonfigurator Bootsauswahl

**Stand:** 06.11.2026. **Status:** GUI-Umsetzung und die erweiterten
synthetischen/Core-/420er-Integrationsmatrizen sind durch Release-Builds
verifiziert. Eine eng begrenzte echte-420er-Projektionsluecke sowie die
persoenliche Abnahme bleiben offen.
Einziger Plan fuer den Arbeitsversionsschritt G-0.13.
Massgeblich: [GUI-Spezifikation](../Spezifikationen/Spezifikation_GUI_Segelkonfigurator.md).
Core-Lieferung: [getrennter C-0.12-Plan](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.12.md).

Kennungen projektweit vor Vergabe geprueft: bisher A-G-00134 und
U-G-000145; neu A-G-00135 bis A-G-00144 und U-G-000146 bis U-G-000153.
Ergaenzung auf Nutzerfreigabe vom 06.11.2026, 10:11 Uhr:
A-G-00145 und U-G-000154 fuer den einheitlichen Winkelvertrag.
Weitere Nutzerfreigabe vom 06.11.2026: A-G-00146 und U-G-000155
fuer Original-Buglage und bedingte Bauchdarstellung.
Historische Anforderungen, Plaene und offene Nachweise bleiben erhalten.

Nutzerkorrektur vom 06.11.2026, 21:32 Uhr: einheitliche Montagereferenz
fuer alle Objekte und ausschliesslich die aktuelle Objektreferenz in
Baum/Darstellung. Keine bootsspezifischen Core-Freigaben, keine technischen
Quell-Duplikate als Unterobjekte. Der Original-Modellordner bleibt
unveraendert; Bedienung und Speicherung ausschliesslich an Testkopien.
Finale Core-/GUI-Validierung bestanden: 5/5 CTests, 420er-Bedienmatrix
182,10 s, neue Allboot-GUI-Montage 384,94 s, Segelvertrag 23,52 s,
Mittschiffs-Basis 4,67 s und alle Objektmontagen 47,51 s.
Die neue GUI-Matrix prueft elf Modelle mit genau einer Baumreferenz je
Objekt, Automatik/Handmodus und echte Eckrotation bei allen fuenf Segeln,
Fock-cm-Speicherung bei beiden Focken sowie Wiederladen. Das SHA-256-
Inventar des Originalordners ist vor/nach dem Test identisch, einschliesslich
Dateibestand. Core prueft alle 38 Objekte und 22 persistierte Aktionen
an separaten Kopien. Release-GUI am gewohnten Buildpfad neu erstellt.
Der bekannte, unabhaengige `modellinspektion_c08`-Fehler
`missing tree type hydrodynamik` bleibt offen; keine persoenliche Abnahme
oder vollstaendige Gesamtregression behauptet.

Weiterer Nutzerauftrag vom 06.11.2026, 15:40 Uhr: Alle vorhandenen Boote
speichern die mittschiffs ausgerichtete Grundposition fuer Segel und Baum.
Der Core kennt dieselbe fachliche 0-Grad-Referenz, ohne Mast-/Vorstagneigung
oder Segelwoelbung zu nivellieren. Die GUI uebernimmt die Core-Geometrie;
keine eigene Winkelnormalisierung. Die isolierte Segeltestkopie behaelt
Nicht-Segel-Settings, insbesondere die neutrale Baumstellung.
Umstellung abgeschlossen: 420er, Folkeboot und Optimist speichern die
mittschiffs-Grundstellung. Core-Messmatrix fuer alle drei Boote bestanden
(4,54 s), alle elf Modelle verwendbar. Quellenkonflikte bei Folkeboot/Opti
werden als `CORE-SEG-0030` gemeldet; keine Physikfreigabe.
Nach Nutzerfreigabe zwischenzeitlich verschobene Quellpfade uebernommen,
ohne Quellbytes oder Posen zu veraendern. GUI neu gebaut, echte 420er-
Integration mit Speichern/Neustart und Fehlermatrix bestanden (194,93 s).
Die Handmodus-Testfolge setzt ihren gewuenschten Zustand ausdruecklich,
statt abhaengig vom vorherigen Zustand umzuschalten. Modellimport und
Core-Konfigurationsvertrag ebenfalls bestanden. Bekannte weitere
Regressionsfehler und persoenliche Abnahme bleiben offen.

Nutzerauftrag vom 06.11.2026, 14:15 Uhr: Die Speicherung zu U-G-000146,
U-G-000152 und U-G-000153 wird auf den allgemeinen Konfigurationsblock in
`bootsmodell.json` umgestellt. Praezisierung 14:45 Uhr: `setting` direkt
am bestehenden Element statt zentraler zweiter Elementliste; keine neuen
IDs, redundante Zuordnungen und gespeicherter Fockversatz entfallen.
Katalog, Startargumente, Dateibeobachtung
und Testfixtures verwenden den neuen Dateinamen. Der isolierte 420er-Test
entfernt nur Einstellungen und Speichermetadaten seiner Kopie, vergleicht alle anderen
Modellangaben und prueft Geometrie-/Lookup-Dateien weiterhin bytegenau.
Build, Neustart- und Fehlermatrix werden nach der Umstellung erneut
ausgefuehrt; deren Nachweis steht bis dahin aus. Keine Erweiterung der
bearbeitbaren GUI-Komponenten.

Tatsaechlicher Migrationsnachweis: 38 vorhandene Objekte in elf Modellen
haben `setting` direkt am Element, ohne neue IDs. Beide gespeicherten
420er-Segellagen und Revision 12 sind erhalten; alle Geometrie-/Lookup-
Dateien SHA-256-identisch. Alle elf Modelle laden nativ, 420er-Konfiguration
ist bereit. Finaler GUI-Release-Build und `gui_sail_configurator_420`
bestanden (165,68 s), inklusive Speichern, Neustart und Fehlerverhalten.
Core-Konfigurationsvertrag und Geometriematrix bestanden. Erweiterte
Regressionen bleiben sichtbar: `gui_core_integration` scheitert an der
stationaeren Ruder-Bewegungsdefinition des Referenzmodells;
`gui_model_catalog` an der fehlenden Bewegungszuordnung im Neutral-ID-Test.
Das sind keine Belege fuer einen abgeschlossenen Gesamttest oder eine
persoenliche Abnahme. Der zusaetzliche Core-Importtest ist nach typensicherer
Diagnosebildung fuer numerische Element-IDs ebenfalls bestanden (0,29 s).
Die finale Core-Bibliothek ist in GUI und Vertragstests neu verlinkt.
Abschliessender gemeinsamer Lauf: 4/4 bestanden, GUI-420er-Integration
166,27 s, Modellimport 0,22 s, Geometriematrix 68,97 s und
Konfigurationsvertrag 21,49 s. Die oben genannten erweiterten
Regressionsfehler und offenen persoenlichen Abnahmen bleiben bestehen.

## 1. Bestand und Grenzen

- `Code/src/boat_inspection.*`: Objektbaum, Auswahl, Werte und eigener
  Inspektions-Snapshot. Hier liegt der Konfigurator, nicht im Simulationsreiter.
- `Code/src/vtk_scene.*`: Picking, Darstellung und Interaktion.
- `Code/src/navigation.*` und `main_window.*`: vorhandene Kamera-Presets
  und Kameraauswahl des Statischen Modells als wiederverwendbares Vorbild.
- `Code/src/core_adapter.*`: bestehenden Adapterstil fuer Core-Kommunikation
  verwenden, soweit fuer Inspektion passend; kein Dateiformatparser in GUI.
- Bestehende QSettings fuer Anzeige/Navigation sind kein Ablageort
  der fachlichen Segel-Einbaulage.

Keine neuen Physikfunktionen, keine Umbauten fremder Reiter und keine
GUI-eigenen Rigg-/Lookup-Heuristiken. Core und GUI werden separat gebaut.

## 2. Geordnete Umsetzung

Die GUI-Integration ist implementiert, aber noch nicht vollstaendig nachgewiesen.
Der aktuelle Stand je Schritt steht in der Statusspalte; offen gebliebene
Abnahmekriterien sind kein Abschlussnachweis.

| Schritt | Arbeit und Ergebnis | Anforderungen | Voraussetzung | Status/Nachweis |
|---|---|---|---|---|
| U-G-000146 | Core-C-0.12-Liefervertrag in BootInspection integrieren: Referenzen, semantische Ecken, Vorschau und bestaetigte Konfiguration trennen; keine Dateizugriffe. | A-G-00135, A-G-00137, A-G-00142, A-G-00143 | Core-Meilenstein C1 | Implementiert; nativer GUI-Build und echter 420er-Aufruf der Core-Vorschau-/Bestaetigungs-/Lesepfade bestanden. |
| U-G-000147 | Kameraauswahl oberhalb der Modellauswahl aus vorhandenen Presets aufbauen; orthografische Achsansichten und Bearbeitungssperre fuer freie/schraege Kamera. | A-G-00138 | keine | Implementiert; sechs Achsen-/Sperrtest und DPI-Pointertest 100/150/200 % bestanden. Layoutabnahme offen. |
| U-G-000148 | Eindeutiges Baum-/Flaechen-Picking und ID-genaue Kontextmenues; Automatik an Core anbinden, gespeicherten Stand darstellen. | A-G-00136, A-G-00137, A-G-00143 | U-G-000146; Core-Meilenstein C2 | Echter 420er: beide Kontextmenue-Automatiken erfolgreich und per Core-Revision nachgewiesen. |
| U-G-000149 | Handmodus und Segelflaechen-Translation in aktiver Modellachsebene; Vorschau, Maus-Loslassen, Abbruch und Navigation sauber trennen. | A-G-00139, A-G-00142 | U-G-000147, U-G-000148 | Release-Matrix: synthetische Pointerabbildung und Core-Vorschau pruefen XY/XZ/YZ samt Normalanteil; realer 420er persistiert Flaechendrag in XZ. Escape, Kamerawechsel, Tab, Hide und Ungrab stellen im echten 420er jeweils bestaetigte Bounds ohne Save wieder her. Echte Oberflaechen-Persistenz in XY/YZ bleibt wegen der kantenstaendigen 420-Projektion nicht nachgewiesen. |
| U-G-000150 | Semantische Drei-/Viereckgriffe, Winkel aus projiziertem Pivothebel, starre Rotation und Hals-/Kopfpivot; Projektionsentartung melden. | A-G-00140, A-G-00142 | U-G-000149 | Synthetisches Dreieck routet alle drei semantischen Handles, synthetischer Vierpunktgriff auch KopfAchterliek. Echter 420er: Grosssegel hat 4 Ecken und rotiert in beiden Pivotfaellen; Fock hat 3 Ecken und bleibt nach +10 cm am Core-Positionspunkt bedienbar. |
| U-G-000151 | Manuellen Fock-Rechtsklickdialog mit absoluter vorzeichenbehafteter cm-Eingabe und Erhalt der seitlichen/drehenden Korrektur integrieren. | A-G-00141, A-G-00142 | U-G-000149 | Echter 420er: +10, wiederholt +10, -10 cm jeweils Core-bestaetigt; Dialogabbruch und Nichtzahl-Eingabe erhalten Revision und dargestellte bestaetigte Geometrie. |
| U-G-000152 | Jede abgeschlossene Aktion transaktional bestaetigen; Speicherfehler/Versionskonflikte sichtbar behandeln; Neustart-/Modellwechsel-Wiederherstellung testen. | A-G-00142, A-G-00143 | U-G-000150, U-G-000151; Core-Meilenstein C3 | Neuer GUI-Prozess stellt beide Lagen wieder her; Lockverweigerung und kaputte Konfiguration erzeugen sichtbare Fehler und behalten den letzten bestaetigten Renderstand. |
| U-G-000153 | 420er-Endintegration, synthetisches Dreieck, DPI-/Bedienregression, Quellen-/Lookup-Pruefsummen und Nutzerabnahme dokumentieren. | A-G-00135 bis A-G-00144 | U-G-000152; Core-Meilenstein C4 | Finaler Release-GUI-Build und echter 420er-/Restartlauf bestanden; synthetische Pointer-/Semantikmatrix, SHA-256-Kopienachweis und Parent40/40-Quell-/Lookup-Pruefung vorhanden. Nutzerabnahme offen; reale-420er-Projektionsgrenze in U-G-000149 dokumentiert. |
| U-G-000154 | Einheitlichen Winkelvertrag im Adapter-/Modelldatenvertrag pruefen: unveraenderte delta-Bedeutung, getrennte Einbaulage/Nullstellung und Lookup-alpha; keine Stellregler implementieren. | A-G-00145 | U-G-000146; Core U-C-000129 | Core-Vertrag eingebunden; vollstaendige Winkelvertragspruefung offen. |
| U-G-000155 | Rechtsklick-Auswahl Original-Buglage je Segel integrieren, Core-Bestaetigung und Neustart pruefen; bedingte Bauchgeometrie/Picking konsistent darstellen, ohne Zielseitenfeld oder Stellregler. | A-G-00146 | U-G-000148, U-G-000152, U-G-000154; Core U-C-000130 | Echter 420er: Grosssegel Backbord/Fock Steuerbord getrennt gewaehlt und nach echtem Prozessneustart bestaetigt; Spiegel-Faelle ueber Stellwinkel bleiben ohne GUI-Stellwinkeltest. |

## 3. Meilensteine

| Meilenstein | Eintritt | Abnahmekriterium |
|---|---|---|
| G1: Vertrag und Kamera | U-G-000146/147; Core C1 | GUI baut mit typisiertem Vertrag; Kameraauswahl korrekt platziert; nur sechs orthografische Achsansichten bearbeitbar. |
| G2: Platzierung und Bedienung | G1; U-G-000148 bis 151; Core C2 | Beide Automatikaktionen, Flaechenziehen, alle Eck-/Pivotfaelle und Fock-cm-Eingabe am 420er sowie Dreieckreferenz funktionieren ohne Formveraenderung. |
| G3: Dauerhafter Stand | G2; U-G-000152; Core C3 | Echter Prozessneustart erhaelt alle bestaetigten Lagen; Fehler/Abbruch/Konflikt ohne Datenverlust oder stillen Reset. |
| G4: Abschluss | G3; U-G-000153; Core C4 | Alle untenstehenden Anforderungen technisch nachgewiesen; bestehende Ansichten unveraendert; persoenliche 420er-Abnahme separat protokolliert. |

## 4. Rueckverfolgbarkeit und Tests

Die Anforderungstexte stehen ausschliesslich in der verlinkten Spezifikation,
Abschnitt 6. Der dokumentierte technische Zwischenstand ist kein Ersatz fuer
die noch offenen Pivot-, Eingabe-, Pruefsummen- und Nutzerabnahmen.

| Anforderung | Umsetzung | Geplanter Nachweis |
|---|---|---|
| A-G-00135 | U-G-000146, U-G-000153 | Architekturpruefung, Regression statisch/dynamisch und Inspektions-Snapshot |
| A-G-00136 | U-G-000148, U-G-000153 | Kontextmenue-/Pickingtests in Baum und Szene; Negativauswahl |
| A-G-00137 | U-G-000146, U-G-000148, U-G-000153 | Core-Transformationswerte gegen gerenderte 420er-Lage |
| A-G-00138 | U-G-000147, U-G-000153 | Sechs Achsansichten, Sperre bei freier/schraeger Kamera; Layout 100/150/200 % |
| A-G-00139 | U-G-000149, U-G-000153 | Mausereignisse in XY/XZ/YZ; Tiefe/Orientierung; Escape/Wechsel/Kamera |
| A-G-00140 | U-G-000150, U-G-000153 | Drei-/Viereckgriffe, Abstand/Pivot, zusammenfallende Projektionen |
| A-G-00141 | U-G-000151, U-G-000153 | Dialog 0/+10/-10, wiederholte Eingabe, Nichtzahl und Abbruch |
| A-G-00142 | U-G-000146, U-G-000149 bis 152, U-G-000153 | Prozessneustart nach jeder Aktionsart; Modellwechsel ohne Speicherknopf |
| A-G-00143 | U-G-000146, U-G-000148, U-G-000152, U-G-000153 | Schreibfehler, kaputte/veraltete Konfiguration; SHA-256 Quell-/Lookupvergleich |
| A-G-00144 | U-G-000153 | Reale 420er-Bedienmatrix plus Dreieckfixture und Nutzerabnahmeprotokoll |
| A-G-00145 | U-G-000154 | Vertragspruefung fuer 0/+90/-90 Grad bei Grosssegel/Fock; keine Vorzeichenumrechnung oder neuen Stellregler |
| A-G-00146 | U-G-000155 | Getrennte Originalseiten, unbekannter Wert, Persistenz/Schreibfehler, Nullstellung original; positive/negative Teststellungen ueber Core-Fixture, Geometrie/Normalen/Picking |

G4 setzt zusaetzlich U-G-000154 voraus. Der Schritt prueft nur die
freigegebene Kompatibilitaetskonvention; spaetere Segelsteuerung bleibt
ausserhalb dieses Plans.
G4 setzt ausserdem U-G-000155 voraus. Die Nullstellung behaelt ihren
Originalbauch; Teststellungen fuer die Spiegelpruefung fuehren keine
neuen Bedienelemente oder Simulationssteuerung ein.

Vorhandene native CMake-/CTest-Struktur verwenden. Zuerst passende
Inspektions-/Navigation-/Adaptertests und neue Konfiguratortests ausfuehren,
anschliessend Integrations-/Grafiktests bei 100/150/200 % DPI mit echtem
Windows/OpenGL. Kein pytest-/venv-Testpfad.
Buildvorbild: `cmake --build C:\spdeps\guibuild --config Release --parallel 3`.
Laufzeit-DLLs unter `C:\spdeps\installed\x64-windows\bin` bereitstellen;
CTest nach vorhandener Projektkonfiguration verwenden.

Automatisierte Tests arbeiten in isolierten temporaeren Modellkopien,
nicht schreibend am Original-420er oder an dessen Lookups.
Testergebnisse, Befehle und Source-/Buildbezug im Nachweisabschnitt dieses
Plans nachtragen. Gruene Tests sind keine persoenliche Abnahme.

## 5. Zwischenstand und Nachweise

- Release-Build im isolierten GUI-Buildverzeichnis
  `C:\spdeps\guig013build-off`: `segelphysik_gui` und
  `segelphysik_gui_graphics_render_tests` erfolgreich gebaut.
- `segelphysik_gui_graphics_render_tests --inspection-pointer-only` mit
  `QT_QPA_PLATFORM=windows` und `QT_SCALE_FACTOR=1`, `1.5`, `2`: alle drei
  Laeufe erfolgreich (`inspectionCameraAxesAndPointerRouting=true`); umfasst
  sechs orthografische Achsenebenen, freie/schraege Kamerasperre, DPI-bewusste
  Eckgriff-/Rechtsklickweiterleitung und Ablehnung projiziert ueberlappender
  Griffe.
- `segelphysik_gui_graphics_render_tests --sail-configurator-integration
  <Modelle> <Nachweisordner>`: erfolgreich. Die Test-CLI kopiert den echten
  420er isoliert, laesst `segelkonfiguration.json`/`.lock` aus und ueberspringt
  Lookup-Produktdateien ausser `lookup.json`. Phase 1 bedient die echte GUI:
  beide Automatikaktionen, Flaechendrag, semantische Rotation, +10/+10/-10 cm,
  zwei getrennte Originalbuglagen, freie-Kamera-Sperre und exklusive Lock-
  Verweigerung. Phase 2 startet einen neuen GUI-Prozess und vergleicht beide
  vollstaendigen Core-Lagen/Revision; prueft reale Meshbounds bei Auto/Preview/
  Commit, Fockgriff nach +10 cm sowie Beibehalt der bestaetigten Darstellung
  bei kaputter Konfiguration. Beide Phasen bestanden.
- `segelphysik_gui_graphics_render_tests --inspection-pointer-only` nach den
  Hide/Ungrab-Aenderungen erneut bei DPI 1/1.5/2: bestanden. Das Widget-
  Eventfilter-Testfixture sendet aktiven Drag, Ungrab bzw. Hide und danach
  Release; es wird kein Finish emittiert. Die echte 420er-Integration prueft
  zusaetzlich die Vorschau-Ruecknahme und unveraenderte Core-Revision.
- `--inspection-only <Modelle> <Nachweisordner>` bleibt auf dem Legacy-
  Folkeboot-Fixture rot. Konkreter Befund: Wertetabelle mit 118 Root-Eintraegen,
  Filter `Bereich=Bewegung` zeigt 0 Eintraege, weil das Folkeboot-Fixture 0
  Bewegungseigenschaften liefert. Die Assertion verlangt `visible > 0` und
  `visible < rootValues`. Vorhandene Bereiche sind Masse, Geometrie, Model,
  Darstellung und Hydrostatik. Kein Filterfehler; weder GUI-Filter noch diese
  Legacy-Testannahme wurde veraendert.
- Der echte Ungrab-Nachweis deckte einen Cachefehler beim Abbruch auf:
  `cancelSailDrag()` entfernte die Preview, ohne die Mesh-Content-Generation
  zu erhoehen. Die Generation wird jetzt vor dem Neurendern erhoeht; der
  420er-Test bestaetigt die wiederhergestellte letzte bestaetigte Darstellung,
  unveraenderte Revision und keinen verzoegerten Finish-Save.
- Ergaenzte Release-Matrix: drei Ebenen im synthetischen orthografischen
  Pointerpfad und in Core-Vorschauen; der Normalanteil bleibt jeweils null,
  beide In-Plane-Achsen sind erreichbar, und Preview veraendert keine Revision.
  Sieben Core-Rotationsvorschauen (Grosssegel vier Ecken, Fock drei Ecken)
  halten den jeweils vertraglichen Gegenpivot exakt fest; synthetische
  GUI-Pointerrouten decken drei/drei/vier Semantikgriffe ab.
  Im realen 420er wurde persistente Flaechentranslation in XZ ausgefuehrt.
  XY (Ansicht `oben`) liess sich im echten Modell nicht als Flaeche picken;
  persistente echte-420er-Drags in XY/YZ sind deshalb nicht behauptet. Das ist
  eine begrenzte Fixture-/Projektionsevidenzluecke, keine geaenderte Core- oder
  Mesh-Quelle.
- Rotationsmatrix: der echte 420er liefert ein 4-Eck-Grosssegel und ein
  3-Eck-Focksegel. Die echte Grosssegel-GUI-Rotation wurde erfolgreich mit
  Hals- und KopfVorliek-Gegenpivot bestaetigt; Fock-Rotation nach +10 cm
  ebenfalls. Synthetische Pointertests decken die drei bzw. vier semantischen
  Griffe ab.
- cm-Abbruch/Nichtzahl und fuenf echte 420er-Dragabbrueche (Escape, Kamera,
  Tab, Hide, Ungrab) bestehen jeweils mit unveraenderter Revision und
  wiederhergestellten bestaetigten Mesh-Bounds.
- Abschliessende GUI-Validierung im isolierten Build
  `C:\\spdeps\\guig013build-off`: Release-Build der Ziele
  `segelphysik_gui` und `segelphysik_gui_graphics_render_tests`; danach
  `--sail-configurator-integration <Modelle> <Nachweisordner>` erfolgreich
  (Automatik beider Segel, Translation, Rotation, absolute cm-Werte,
  getrennte Buglagen, Kamerasperre, Lockfehler, Neustart und fehlerhafte
  Konfiguration). `--inspection-pointer-only` erfolgreich bei
  `QT_SCALE_FACTOR=1`, `1.5` und `2`. Der Legacy-Aufruf
  `--inspection-only <Modelle> <Nachweisordner>` wurde separat wiederholt
  und scheitert weiterhin exakt mit dem oben dokumentierten Befund.
  Verwendete Testdatei:
  `C:\\spdeps\\guig013build-off\\Release\\segelphysik_gui_graphics_render_tests.exe`.
  Core-Library wurde im selben Release-Baum neu gebaut und durch den
  GUI-Testprozess geladen; der Core-Agent meldete separat den finalen
  `segelkonfiguration_contract` als 1/1 bestanden.
- Separater Production-Smoke des tatsaechlichen
  `C:\\spdeps\\guig013build-off\\Release\\segelphysik_gui.exe` mit
  `QT_QPA_PLATFORM=windows`, installiertem Qt-Pluginpfad und Modellkatalog-
  Argument: innerhalb des 60-s-Limits sichtbares Hauptfenster
  (`SegelPhysik – Hydrostatik`), `Responding=True`, kein stdout/stderr;
  `CloseMainWindow()` beendete denselben Prozess normal, ohne PID-Abbruch.
- Der finale Isolationskopie-Hashvergleich meldete 30/30 kopierte regulaere
  Dateien mit identischem SHA-256; 10 ausgelassene Lookup-Produktdateien
  wurden explizit gezaehlt. Zusaetzlich bestaetigt Parent40/40 unabhaengig
  unveraenderte Quell-/Lookup-Pruefsummen.
- Uebrige offene Nachweise: echte-420er-Oberflaechen-Persistenz XY/YZ (siehe
  Projektionsluecke) und persoenliche Nutzerabnahme. Parent40/40 deckt
  Quell-/Lookup-Pruefsummen ab. Ein Core-Fehler mitten im atomaren
  Tempfile-Replace ist nicht injizierbar und fuer diesen GUI-Nachweis nicht
  erforderlich; die GUI prueft die verfuegbare Schreibsperre.
- Keine Core-Dateien, Lookup- oder Quelldateien geaendert. Keine Git-Aktion.

G-0.13 ist daher noch nicht abnahmefertig. Eine spaetere Physik-/Lookup-
Integration bleibt ausserhalb dieses Plans.
