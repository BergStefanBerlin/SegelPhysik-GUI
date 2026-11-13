# GUI-Umsetzungsplan G-0.23: Winddarstellung und Wasseransicht

**Stand:** 13.11.2026 · **Status:** Umsetzung begonnen; Gesamtumsetzung, Tests und
Sichtabnahme offen. Vorgesehener nächster GUI-Versionsschritt nach G-0.22;
Umsetzung am 13.11.2026 beauftragt. Kein Tag oder Release. Dies ist der einzige
GUI-Plan für G-0.23.

Maßgeblich sind [GUI-Windbedienung und Darstellung](../Spezifikationen/Spezifikation_GUI_Winddarstellung.md),
[GUI-Referenzpunkte](../Spezifikationen/Spezifikation_GUI_Referenzpunkte.md)
und [Wasser-Darstellungsfehler](../Spezifikationen/Spezifikation_GUI_Grafikprofile_und_Setup.md#51-a-g-00163-und-a-g-00164-darstellungsfehler-an-der-wasseroberfläche).
Der getrennte [Core-Plan C-0.23](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.23.md)
liefert den Windvertrag.

## 1. Umfang und Grenzen

Die GUI bindet die bestätigten Core-Winddaten für Eingabe, Mastpfeile,
Kompass, Profil und Wiedergabe an. Das abgesetzte Spiegelungsfehlbild wird
beseitigt und die Sicht von unter Wasser verbessert. Die beiden Themen
bleiben in ihren jeweiligen Spezifikationen; dieser Plan bündelt die
versionsbezogene Ausführung.

GUI-Code rechnet keine eigene Windphysik und liest keine Modelldateien als
Ersatz für den Core. Ein fehlender oder mehrdeutiger Mast wird gemeldet;
keine erfundene Spitze oder SDP. Fremdkraft bleibt bestehen. Die GUI
verwendet die gemeinsame Core-Punktesammlung für ihre bereits benötigten
Anzeigen; zusätzliche Werte müssen nicht neu dargestellt werden. Keine
Segelkraft, kein Eingriff in Core oder Modellrepository, keine Git-Aktion.

Projektweit vor Vergabe geprüft: höchste bisherige Anforderungen
A-G-00156 und Schritte U-G-000168. Dieser Plan verwendet A-G-00157 bis
A-G-00165 und U-G-000169 bis U-G-000177.

**Externe Abhängigkeit:** Wind-Integration benötigt den umgesetzten,
getesteten Core-Vertrag aus C-0.23 einschließlich Punktesammlung und eindeutiger Mastspitze,
Profil, zeitbezogener Einstellungen, Vektoren und Fehlerstatus. Bestehende
Core-Modellstruktur enthält keinen expliziten Mastspitzenvertrag. Bei
fehlenden Daten oder fehlendem Contract-Gate nicht mit Namensheuristiken,
GUI-Formel oder Ersatzdaten weiterarbeiten; Blocker dokumentieren.
Die GUI-Wasserarbeiten sind davon unabhängig.

## 2. Umsetzungsschritte und Abhängigkeiten

| U-ID | Schritt und fachlicher Umfang | Anforderungen | Abhängigkeiten | Nachweis |
|---|---|---|---|---|
| U-G-000169 | Core-Vertrag und tatsächliche GUI-Anbindung für Modellidentität, Mastpunkt, Profil, Windstatus, Änderungszeit und historische Snapshots prüfen; Build gegen den beauftragten Core verifizieren. Fehlendes explizit als externen Blocker melden. | A-G-00158 bis A-G-00162 | Core C1/C3, zur Abschlussintegration C4 | G23-01: Vertragstest, Datenherkunft und verwendete Core-Fassung dokumentiert. |
| U-G-000177 | Bestehende Verbraucher für Mastspitze, Kräfte, LDP und Ersatz-SDP an die gemeinsame Core-Punktesammlung anbinden; Identität, Status und jeweilige bisherige Anzeige bewahren. Neue Geschwindigkeiten oder Punktarten müssen nicht zusätzlich dargestellt werden; gültige Positionen nicht wegen fehlender Geschwindigkeit ausblenden. | A-G-00165 | U-G-000169, Core C3/C4 | G23-11 und G23-12; kein Ersatz durch lokale Punktberechnung. |
| U-G-000170 | Windeingaben unter Fremdkraft ergänzen: Defaults, Modi, Liveänderungen, nur vor Start änderbares `h0`, Begrenzung und kurzer Feldhinweis; ungültige Zahlen behalten den alten Wert. Ungeänderte gerundete Anzeige darf gespeicherte Präzision nicht zerstören. | A-G-00157, A-G-00161 | U-G-000169 | G23-02, G23-03, G23-09. |
| U-G-000171 | Zwei gleich geformte blaue/orange 3D-Pfeile mit Legende und gemeinsamem Schalter an der Core-Mastspitze darstellen; Längenmaßstab und getrennte räumliche Schwelle verwenden. Fehlenden/mehrdeutigen Mast explizit behandeln. | A-G-00158, A-G-00162 | U-G-000169 | G23-04, G23-05. |
| U-G-000172 | Kompass mit großem transparentem und kleinem ausgefülltem Dreieck erweitern; Herkunftswinkel und horizontale getrennte Schwelle verwenden. Bei gleicher Richtung mittig schachteln; Lesbarkeit für gleiche und nahe Richtungen bei DPI-Skalierung prüfen. | A-G-00159, A-G-00161 | U-G-000169 | G23-06 und G23-09. |
| U-G-000173 | Geschwindigkeitsfeld halbieren und Core-Profil daneben darstellen; Werteformat, Achsen, strikt nächster 5-m-Schritt, Referenzpunkt außerhalb und fehlender Mast gemäß Spezifikation. Keine zweite Profilformel. | A-G-00157, A-G-00160, A-G-00162 | U-G-000169 | G23-02, G23-05, G23-07. |
| U-G-000174 | Abgesetztes Spiegelungsfehlbild untersuchen und beseitigen; von unter Wasser die Sicht auf den oberhalb liegenden Yachtteil verbessern. Wasser, echte Unterwassergeometrie, übrige Szenenreflexion und bestehende Schalter nicht pauschal entfernen. | A-G-00163, A-G-00164 | keine Windabhängigkeit | G23-08: Vergleich von 420 und Optimist, Blickwinkeln und Spiegelung ein/aus; Fehlbild und Sicht prüfen, persönliche Sichtabnahme getrennt. |
| U-G-000175 | Bestätigte Daten für Live, Pause und Wiedergabe zustandskonsistent anbinden; Zustands-/Modellwechsel, abgelehnte Eingaben und veraltete oder fehlende Daten explizit behandeln. Bestehende Bootsdaten und Fremdkraft unverändert lassen. | A-G-00157 bis A-G-00162, A-G-00165 | U-G-000170 bis U-G-000173, U-G-000177; Core C4 | G23-03, G23-05, G23-09, G23-12. |
| U-G-000176 | Relevante GUI-Tests, Release-Build und vollständige GUI-CTest-Suite ausführen; Rückverfolgbarkeit, technische Ergebnisse, Core-Version und persönliche Sichtabnahme getrennt dokumentieren. | A-G-00157 bis A-G-00165 | U-G-000174, U-G-000175, U-G-000177 | G23-10; alle Anforderungen mit tatsächlichen Nachweisen oder offener Abweichung. |

## 3. Testanforderungen

| Fall | Prüfung und erwartetes Ergebnis |
|---|---|
| G23-01 Vertrag | Tatsächlich verlinkte Core-Fassung, alle benötigten Wind-/Mast-/Zeitdaten und Fehlerstatus prüfen; fehlender Vertrag blockiert die Integration, GUI liefert keinen Ersatz. |
| G23-02 Eingabe | Defaults Nord/5,0 m/s/10 m/weltfest; ungültiger Text behält den alten Wert, begrenzte Werte zeigen einen Hinweis. `v0=0` wird intern 0,01 m/s und als 0,0 m/s dargestellt; unbeändertes Übernehmen und ein Simulationsneustart verlieren 0,01 nicht. `v0` und `h0` an beiden Grenzen und außerhalb testen. |
| G23-03 Laufbedienung | Beide Dynamikmodi, kontinuierlicher Umschaltwinkel, Liveänderungen von Richtung und `v0`, Sperre von `h0` im Lauf und bestätigte Werte bei Neustart. Fremdkraft bleibt vorhanden; historische Zustände bleiben unverändert. |
| G23-04 3D | Bei 5/10 m/s 0,5/1 m lange, gleich geformte blaue/orange Pfeile samt Legende; Richtung zeigt Strömung, nicht Herkunft. Gemeinsamer Schalter sowie getrennte Schwellen bei 0,1 und knapp darüber; vertikaler scheinbarer Vektor und gekrängtes Boot. |
| G23-05 Mast und Fehler | Eindeutige Mastspitze, kein Mast, Hauptmast bei mehreren Masten und fehlende eindeutige Zuordnung; bei fehlendem Mast keine Pfeile und kein Profil, bei Mehrdeutigkeit sichtbarer Hinweis. Ungültiges Profil ergibt Diagnose, kein gültig aussehendes Leerbild. |
| G23-06 Kompass | N/O/S/W und 0°/360°; Wahr-Markierung größer/transparent, Scheinbar kleiner/gefüllt, Spitzen nach innen. Gleichrichtung mittig verschachtelt mit 1–2 sichtbaren Pixeln Abstand, nahe Richtungen und 100/150/200 % DPI prüfen. Horizontale Schwelle unabhängig von der räumlichen 3D-Schwelle. |
| G23-07 Diagramm | Gleiche Feldbreite nach Lücke; links eine Nachkommastelle. Höhen 2,4/8/10/10,5 m ergeben Obergrenzen 5/10/15/15 m; Profilwerte stammen aus Core. Referenzhöhe innerhalb und außerhalb prüfen; schmale Fenster und DPI ohne abgeschnittene Beschriftung. |
| G23-08 Wasser | 420 und Optimist bei laufender sowie pausierter Dynamik; von oben, flach über und unter Wasser, mit/ohne Spiegelung. Keine abgesetzte zweite Rumpfform; echte Geometrie, Wasser und natürliche oberflächenbezogene Reflexion erhalten. Sicht auf den oberhalb liegenden Yachtteil unter Wasser gegenüber vorher beurteilen; Bildbelege und Nutzer-Sichtabnahme getrennt festhalten. |
| G23-09 Synchronität | Liveänderung, Pause und Rückblick vor/nach Änderung: Pose, Wind, Punkt, Profil, Kompass und 3D aus demselben Zustand; keine nachträgliche Änderung historischer Ergebnisse. Alte Ergebnisse ohne Winddaten zeigen nicht verfügbar statt aktuelle Livewerte. |
| G23-10 Regression | GUI-Release-Build und vollständige CTest-Suite einschließlich Navigation, Grafik, Renderer, Core-Integration und bestehender Modell-/Kraftdarstellung; keine Änderungen an Core- oder Modellrepository im GUI-Schritt. |
| G23-11 Punktverbrauch | Mastspitze, angezeigte Kraftangriffspunkte, LDP und Ersatz-SDP über die gemeinsamen Core-Einträge zuordnen. Bisherige Positionen, Beschriftungen, Schalter und fachliche Zustände bleiben erhalten; zusätzliche Einträge/Geschwindigkeiten erzwingen keine neue Anzeige. |
| G23-12 Punktstatus und Historie | Gültige Position mit nicht verfügbarer Geschwindigkeit bleibt für die bestehende Positionsanzeige nutzbar. Fehlender Punkt erzeugt keine erfundene Position; Live und Rückblick verwenden ihren eigenen Ergebnisstand, alte Ergebnisse bleiben lesbar. |

## 4. Meilensteine und Prüfungen

| Gate | Eintritt | Abnahme |
|---|---|---|
| G1 Core-Vertrag | U-G-000169; Core C1/C3 | G23-01 bestanden; Fehlendes blockiert G2/G3, nicht G4. |
| G2 Windbedienung und -darstellung | G1; U-G-000170–000173 | G23-02 bis G23-07 bestanden; UI und 3D sind mit tatsächlichen Core-Daten verbunden. |
| G3 Live, Punkte und Wiedergabe | G2, Core C4; U-G-000177 und U-G-000175 | G23-09, G23-11 und G23-12 bestanden; historische und aktuelle Daten unterscheidbar. |
| G4 Wasserbild | U-G-000174 | G23-08 technisch geprüft; Bildnachweise und offene persönliche Sichtabnahme dokumentiert. |
| G5 GUI-Abschluss | G3 und G4; U-G-000176 | G23-10 und vollständige Anforderungsnachweise bestanden; persönliche Sichtabnahme separat. |

Vor dem Build die CMake-Konfiguration auf den richtigen Core-Arbeitsbereich
`C:\_DATEN\Code\Cpp\Segelphysik\CORE` und die verwendete Fassung prüfen;
bei Abweichung korrekt konfigurieren, keine stillschweigende Altfassung.
Die bestehenden, bei 100/150/200 % registrierten Grafik-/Workspace-Tests
und gezielte neue Windtests zuerst ausführen, anschließend vollständig:

```powershell
cmake --build C:\spdeps\guibuild --config Release --parallel 6
ctest --test-dir C:\spdeps\guibuild -C Release -R "^gui_navigation|^gui_graphics_|^gui_dynamic_layout_|^gui_workspace_tabs_|^gui_core_integration$" --output-on-failure
ctest --test-dir C:\spdeps\guibuild -C Release --output-on-failure
```

Neu registrierte Windtests zusätzlich gezielt ausführen; genaue Selektoren,
Renderumgebung, Bildbelege und Ergebnisse nach der Umsetzung eintragen.
Ein fehlender GPU-/Sichtnachweis gilt nicht als bestanden.

## 5. Rückverfolgbarkeit und Status

| Anforderung | Schritte | Tests | Status |
|---|---|---|---|
| A-G-00157 | U-G-000170, U-G-000173, U-G-000175, U-G-000176 | G23-02, G23-03, G23-07, G23-09, G23-10 | offen |
| A-G-00158 | U-G-000169, U-G-000171, U-G-000175, U-G-000176 | G23-01, G23-04, G23-05, G23-09, G23-10 | offen |
| A-G-00159 | U-G-000169, U-G-000172, U-G-000175, U-G-000176 | G23-01, G23-06, G23-09, G23-10 | offen |
| A-G-00160 | U-G-000169, U-G-000173, U-G-000175, U-G-000176 | G23-01, G23-05, G23-07, G23-09, G23-10 | offen |
| A-G-00161 | U-G-000169, U-G-000170, U-G-000172, U-G-000175, U-G-000176 | G23-01, G23-03, G23-06, G23-09, G23-10 | offen |
| A-G-00162 | U-G-000169, U-G-000171, U-G-000173, U-G-000175, U-G-000176 | G23-01, G23-05, G23-09, G23-10 | offen |
| A-G-00163 | U-G-000174, U-G-000176 | G23-08, G23-10 | offen |
| A-G-00164 | U-G-000174, U-G-000176 | G23-08, G23-10 | offen |
| A-G-00165 | U-G-000177, U-G-000175, U-G-000176 | G23-11, G23-12, G23-10 | offen |

**Tatsächliche Nachweise:** GUI-Programm sowie Navigations-, Coreadapter-,
Kraftanzeige- und Grafiktestziel wurden gegen den aktuellen Core gebaut.
`gui_navigation`, `gui_navigation_camera`, `gui_core_integration` und
`gui_force_display` bestanden. Die Coreadapterprüfung umfasst eine
Live-Windänderung von Richtung, Geschwindigkeit und Modus sowie
unveränderte historische Zustände. Die Kraftanzeige liest Schwerpunkt
und Druckpunkte aus der gemeinsamen Sammlung; dies wurde auch mit
statischen und dynamischen Core-Snapshots geprüft. Das ist
keine vollständige Abnahme: Insbesondere Wasserfehler, DPI-/Sichtprüfungen,
Eingabegrenzen und die vollständige GUI-Regression bleiben offen. Der
gezielte GUI-Test für eine intern erhaltene Eingabe von `0,04 m/s` bei
Anzeige `0,0 m/s` und wiederholtem unverändertem Übernehmen bestand; die
angezeigten Mastpfeile stammen dabei aus einem statischen Core-Lauf. Der
Wasserbewegungs-Regressionstest bestand erneut für 420 und Folkeboot;
er ist kein Nachweis für die Behebung des hier beauftragten Wasserfehlers.
Vergleichsbilder von 420 und Optimist mit gleichem Dynamikzustand und
ein-/ausgeschalteter Spiegelung wurden für steile, flache und
Unterwasseransichten aufgenommen. Die tief liegenden Zusatzkonturen waren
auf die Spiegelgeometrie zurückzuführen. Diese blendet nun mit Abstand zur
Wasseroberfläche aus und ist unter Wasser nicht sichtbar; zugleich ist das
Wasser aus Unterwassersicht transparenter. `gui_water_reflection` prüft
Oberflächenspiegelung, abgesetzte Konturen und Unterwasseransicht für beide
Modelle; der Test bestand. Die Bilder zeigen die verbesserte Sicht auf die
oberhalb liegenden Bootsteile. Persönliche Sichtabnahme und weitere
Dynamik-/Perspektivprüfungen stehen noch aus.
Ein früherer vollständiger GUI-Lauf endete mit 55 von 64 bestandenen Tests. Grafik- und
Workspace-Tests scheiterten bei allen drei Skalierungen an der alten Erwartung
`0.00 °/s`; der Test wurde auf die neue einstellige Anzeige und auf den
neuen gemeinsamen Kartencontainer korrigiert. Die gezielten Workspace-Tests
bei 100 %, 150 % und 200 % bestehen danach.
`gui_dynamic_layout_150` meldete außerdem nicht
überwachbare Modellverzeichnisse und „Restart display restoration failed“;
eine gezielte Wiederholung bestand, die Ursache des ersten Fehlers bleibt
ungeklärt. `gui_boat_inspection_200` erreichte
außerdem das Zeitlimit von 300 Sekunden; ein Wiederholungsnachweis fehlt.
`gui_sail_configurator_420` scheiterte beim Warten auf das Neuladen des
Modell-Watchers; ein Wiederholungsnachweis fehlt. Eine spätere Wiederholung
von `gui_graphics_render_100` mit korrigierter Anzeigeerwartung erreichte
den 420er-Solver, scheiterte dort zunächst am Zeitlimit und bestand bei
erneuter Wiederholung. Der vollständige Lauf ist kein Nachweis für die
vollständige aktuelle Regression.
Nach dem jüngsten Core-Mastvertrag bestanden `gui_core_integration`,
`gui_water_reflection` und `gui_navigation_camera` erneut. Ein Referenzpunkt
außerhalb des Profilbereichs wird jetzt auch ohne Mauszeiger als Wert angezeigt;
`gui_navigation` prüft dies und bestand. `gui_wind_input` lief zunächst in
das Zeitlimit und meldete bei Wiederholung einen nicht gestarteten Lauf;
der Test wartet nun nach dem Arbeitsplatzwechsel erneut auf die Modellbereitschaft.
Die Wiederholung von `gui_wind_input` bestand; die vollständige aktuelle
Regression steht aus. Zusätzliche Prüfungen für Maßstab, Richtung,
gemeinsamen Schalter, getrennte Schwellen und fehlende Mastposition der
3D-Windpfeile sind ergänzt und im `--navigation-only`-Test bestanden.
Der Wasserbildtest lief bisher mit `Modelle_alt`. Bei Wiederholung mit
den aktuellen 0.3-Modellen trat das Fehlbild für den 420 erneut auf:
Bei texturiertem PBR-Material überschreibt VTK die zuvor gedämpfte
Deckkraft. Die Dämpfung wurde hinter die Material-/Texturberechnung
verschoben; `gui_water_reflection` und der neu registrierte Test
`gui_water_reflection_model03` bestanden danach mit 420 und Optimist.
Die erneute vollständige Regression bestand 64 von 67 Tests;
`gui_sail_configurator_420`, `gui_boat_inspection_200` und die
Layoutprüfungen bestanden dabei. Die drei
`gui_graphics_render`-Skalierungen meldeten zunächst einen nicht exakt
identischen Material-Reset. Die Diagnose bei 100 % fand drei abweichende
Pixel mit jeweils höchstens einer Farbstufe Unterschied. Der Vergleich
erlaubt nun nur solche vereinzelten Ein-Stufen-Abweichungen. Die
Wiederholungen erreichten anschließend den Pausenbildvergleich, der
ebenfalls drei Pixel mit je einer Farbstufe Unterschied zeigte. Derselbe
enge Vergleich gilt dort. Alle drei erneuten Skalierungstests scheiterten
anschließend am pixelgenauen Vergleich der statischen Vorschau ohne
Simulationszustand. Die Diagnose bei 100 % fand neun abweichende Pixel,
jeweils mit einer Farbstufe Unterschied. Auch hier gilt nun derselbe enge
Vergleich; bei einem weiteren 100-%-Lauf zeigte allerdings eines von sechs
abweichenden Pixeln zwei Farbstufen Unterschied. 16 weitere unveränderte
Aufnahmen wichen jeweils an 10–18 Pixeln um höchstens eine Farbstufe ab.
Der Vergleich begrenzt weiterhin die Gesamtzahl abweichender Pixel und
erlaubt höchstens zwei Farbstufen bei wenigen davon. Ein Modelldurchlauf
zeigte nach dem Zurückwechseln drei Ein-Stufen-Pixel; der 100-%-Test bestand
anschließend. Eine spätere 100-%-Wiederholung kam über die Bildvergleiche
hinaus, scheiterte aber an nicht überwachbaren Modellverzeichnissen.
`gui_graphics_render_150` und `gui_graphics_render_200` bestanden im
selben Lauf; der 100-%-Fehler bleibt trotz des früheren bestandenen Laufs
als intermittierende Abweichung offen. Eine erneute 100-%-Wiederholung
bestand. `gui_navigation` bestand
zusätzlich mit Rasterprüfung der verschachtelten Windmarker und ihrer
Sichtbarkeit bei nahen Richtungen für 100 %, 150 % und 200 %.
Die ergänzte Profilhöhenprüfung für 2,4/8/10/10,5 m bestand in
`gui_navigation_camera` nach Korrektur der Vergleichs-Testanordnung;
die Kurven werden mit Core-Profilwerten verglichen.
`gui_wind_input` bestand mit Defaults, beiden zulässigen Grenzwerten,
Begrenzung außerhalb des Bereichs und ungültiger Eingabe für `v0` und
`h0`; die interne Präzision bei wiederholt übernommener Anzeige `0,0`
wurde für `0,04 m/s` im tatsächlichen Statiklauf geprüft.
Ein weiterer vollständiger Regressionslauf endete mit 64 von 67 Tests.
`gui_sail_configurator_420` scheiterte erneut am Neuladen durch die
Modellüberwachung, `gui_workspace_tabs_200` an nicht überwachbaren
Modellverzeichnissen und `gui_graphics_dialog` am Zeitlimit. Die
modellunabhängige Wiederholung von `gui_graphics_dialog` bestand. Ein
Wiederholungslauf der anderen Tests wurde abgebrochen, nachdem festgestellt
wurde, dass der vorhandene Segelkonfigurator-Test Aerodynamikdateien aus
`Modelle_alt` liest und in eine Testkopie kopiert. Der Nutzer erlaubte
anschließend das Lesen der Originale, nicht aber deren Änderung. Die
ergänzte Live-/Wiedergabeprüfung
wurde anschließend gegen eine isolierte Kopie der 0.3-Folkeboot-Daten
ohne Aerodynamikdateien gebaut und bestanden. Sie prüft die Sperre von `h0`,
Liveänderung von Richtung, Geschwindigkeit und Modus sowie Rückblick und
Rückkehr zur Liveansicht während des Laufs und Wiedergabe nach dem Stopp.
Zusätzlich wurde nach Eingabe von `0`, unverändertem Übernehmen der Anzeige
`0,0` und zwei Statikstarts das angezeigte Profil jeweils mit dem
Core-Profil für intern `v0=0,01 m/s` und `h0=0,01 m` verglichen.
Das ist kein vollständiger Test des Originalmodellordners. Eine separate
GUI-Pausenbedienung wurde nicht gefunden oder geprüft; das Festhalten
eines Wiedergabezustands stoppt die laufende Simulation nicht. Die echte
Pause wird auf Nutzerwunsch vom 13.11.2026 für diese Umsetzung übergangen
und bleibt ohne Umsetzungs- oder Abnahmenachweis.
Die Wiederholung des 200-%-Arbeitsbereichstests mit einer isolierten
0.3-Modellkopie ohne Aerodynamikdateien bestand die Arbeitsbereichs- und
Layoutprüfungen und erreichte den 420er-Bewegungsteil, scheiterte dort
aber an fehlenden erwarteten Bewegungsreglern. Das ist
kein bestandener Ersatz für den Legacy-Test: Dieser erwartet die
Legacy-IDs `Ruder` und `Schwert`, das aktuelle 420-Modell verwendet
`Rudder` und `Centerboard` und beschreibt die Bewegung auf Gruppenebene.
Der im vollständigen Legacy-Lauf gemeldete Überwachungsfehler ist damit
nicht aufgeklärt.
Die separaten 0.3-Tests für Gruppenbewegung und UI-/Worker-/Watcher-Gates
bestanden mit isolierten Kopien der aktuellen Modelle ohne Aerodynamikdateien.
Auf ausdrücklichen Nutzerwunsch wurden `gui_sail_configurator_420` und
`gui_workspace_tabs_200` dauerhaft entfernt; ihre früheren Ergebnisse
bleiben hier als historische Nachweise stehen. Der 200-%-Grafiktest
enthält weiterhin gemeinsame Arbeitsbereichsprüfungen. Nach der Entfernung
bestanden die registrierten 0.3-Interaktions-, Gruppenbewegungs- und
Inspektionspointer-Tests; eine vollständige Regression der verbleibenden
Tests wurde danach noch nicht ausgeführt.
Die modellunabhängigen Wiederholungen `gui_navigation`, `gui_force_display`,
`gui_water_motion`, `gui_graphics_settings` und `gui_graphics_dialog`
bestanden anschließend gemeinsam. Die Nachweise für eine echte Pause und
die vollständige fehlerfreie GUI-Regression bleiben offen.
Beide Wasserbildtests bestanden außerdem für 420 und Optimist mit
fortlaufender Core-Simulation sowie anschließend festgehaltenem Zustand,
jeweils bei drei Blickwinkeln und mit/ohne Spiegelung. Die angehaltenen
Bildzustände blieben bei erneutem Rendern innerhalb der dokumentierten
engen Pixeltoleranz stabil; eine separate GUI-Pausenbedienung wurde dabei
nicht geprüft. Bildbelege liegen im verwendeten Build unter
`test-evidence/water-reflection` und `test-evidence/water-reflection-model03`;
die mit `_live_` bezeichneten Bilder stammen aus der laufenden Simulation.
Der Wasserbildtest bestand auch mit isolierten Kopien von 420 und Optimist
aus `Modelle` ohne Aerodynamikdateien; diese Wiederholung ersetzt keine
persönliche Sichtabnahme.
Persönliche Sichtabnahme bleibt ausstehend.

**Testmigration auf Modellformat 0.3:** `gui_wind_input` und
`gui_water_reflection_model03` sind auf den aktuellen Modellbestand
registriert und bestanden damit erneut. Der doppelte Wasserbildtest mit
Altmodellen wurde entfernt; der 0.3-Test bleibt ohne bedingtes Auslassen
registriert. Die Migration weiterer GUI-Tests und eine vollständige
Regression stehen noch aus.

**GUI-Anpassung auf Nutzerwunsch:** Bei Statikauswahl ist Wind weltfest
gesetzt und gesperrt. Das Profil nutzt den frei gewordenen Platz,
Zehnerschritte für die Geschwindigkeitsachse und feinere Stützstellen
nahe der unteren gültigen Profilgrenze. Das GUI-Releaseziel wurde gebaut;
auf Nutzerwunsch erfolgten keine weiteren Tests. Die ältere Core-Windspezifikation
beschreibt die Statik weiterhin als bootmitgeführt und wurde bei diesem
auf die GUI begrenzten Auftrag nicht geändert.
Auf anschließende Präzisierung beginnt die Profilkurve bei 0,5 m Höhe,
nicht bei einer Geschwindigkeit von 0,5 m/s; die Höhenachse beginnt bei 0 m.
Die Statikvorgabe wurde anschließend auf Nutzerwunsch auf
„Mit Boot mitgeführt“ korrigiert und bleibt gesperrt; sie stimmt damit wieder
mit der Core-Windspezifikation überein.
