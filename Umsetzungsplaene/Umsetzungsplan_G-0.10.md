# Umsetzungsplan G-0.10: Statische Anzeige und Dreifachansicht

## Nutzerabnahme 04.11.2026

**Nutzerabnahmen: done.** Ausdruecklicher Nutzerauftrag vom 04.11.2026,
09:16 Uhr: alle Nutzerabnahmen des gelieferten Stands abschliessen.
Dies schliesst Sicht-/Bedienabnahme, Kameraauslegung und die laufenden
Eingaben U-G-000145 ein und ersetzt alle aelteren offenen
Nutzerabnahmestaende in diesem Plan. Historische Nachweise bleiben erhalten.
Technische Testluecken und nicht finalisierte Zusatzumfaenge werden dadurch
nicht als nachgewiesen ausgegeben. Kein Commit, Tag oder Push beauftragt.

**Aktueller technischer Abschluss 04.11.2026:** Vollstaendiger
Release-Neubau und ungefilterter GUI-Gesamtlauf **45/45 bestanden**,
einschliesslich aller drei DPI-Stufen und laufender Eingaben.
Startuebergang/Live-Eingaben zusaetzlich dreifach wiederholt, **12/12
Ausfuehrungen bestanden**. Die aelteren offenen technischen Testfehler
weiter unten sind durch die Korrekturen und neueren Nachweise geschlossen,
nicht die nicht finalisierte erweiterte Physikmatrix.
[Massgeblicher aktueller Testnachweis](Umsetzungsplan_G-0.11.md#revisionsvorbereitung-04112026-vollstaendige-regression).

## Datierter fachlicher Nachtrag 04.11.2026: laufende Eingaben

Dieser Nachtrag fuehrt die weitere Arbeit am bestehenden Dynamikthema,
nicht eine nachtraegliche Aenderung historischer Abschlussnachweise oder
eine Erweiterung des separaten Performanceplans.

**U-G-000145 / A-G-00134:** Nach Nutzerfreigabe 08:52 Uhr:
1. Core-Vertrag CPPI-0119 stabilisieren (Abhaengigkeit U-C-000121).
2. Fremdkraft/Ansatzpunkt puffern und vor dem naechsten unberechneten
   Schritt uebernehmen; historische Zustaende unangetastet lassen.
3. Bestehende numerische Ruder-/Schwerteingaben im dynamischen Lauf
   freigeben und mit den vorhandenen Bewegungsereignissen verbinden.
4. Bestaetigte Core-Transformationen ueber die vorhandenen selektiven
   Modell-/Meshpfade darstellen; Fehler/Restanforderungen sichtbar melden.
5. Reale Eingabe im laufenden GUI-Versuch, Adapter-/Historien-/Geometrietest.

Technisch umgesetzt. Nachweise: `gui_core_integration` (29,92 s) prueft
laufende Kraft-/Ansatzpunkt-/Ruder-/Schwertwechsel, unveraenderte alte
JSON-Zustaende, neue Meshtransformationen und Abschaltung der Kraft.
`gui_dynamic_layout_100` (8,78 s) bestaetigt bedienbare Eingaben waehrend
eines realen Laufs sowie weitere Workspacewechsel. Performance-Regression
(17,76 s) bestaetigt die erhaltene Szenen-/Meshstilllegung.
Die Defaultpruefung des Layouttests wurde auf den separat am selben
Morgen beauftragten Zustand Analyseansichten AUS angepasst.
Normaler Release-Build erfolgreich. Fachliche Nutzerpruefung offen,
alle Aenderungen uncommittiert; kein Git-/Release-Auftrag.

**Stand:** 03.11.2026.
**Nachtrag Dynamik:** Abschnitt 12 erweitert den Anzeigeumfang nach
ausdruecklichem „go“ auf das Dynamische Modell; kein Core-Eingriff.
**Status:** Implementiert und technisch abgeschlossen; finaler
Wiederholungstest des Kraftmenues bestanden, persoenliche Nutzerabnahme offen.
**Nachtrag Kraftmenue:** Abschnitt 9 wurde am 03.11.2026, 16:15 Uhr
finalisiert und zur Implementierung freigegeben. Implementierung und
gezielte Nachweise laut Umsetzungsbericht vom 16:28 Uhr in Abschnitt 9;
finaler Retest nach Render-/Tooltipbereinigung laut Abschlussbericht
vom 16:31 Uhr bestanden. GUI uncommittet.
**Nachtrag:** Abschnitt 6 enthaelt die am 03.11.2026 um 13:42 Uhr
freigegebene neue Fensterbedienung und ihren gesonderten Nachweisstand.
Abschnitt 7 enthaelt die um 14:10 Uhr beauftragte Praezisierung fuer
linkes Verschieben und die feste schematische Darstellung.
**Zielversion:** G-0.10.
**Ausgangsstand:** veroeffentlichtes G-0.9.
**Projektumfang:** GUI ja, Core nein.

Massgeblich ist ausschliesslich die
[Spezifikation G-0.10](../Spezifikationen/Spezifikation_GUI_Statisches_Modell.md).
Die Herkunft steht in der
[Entwurfspunkteliste](../Spezifikationen/Entwurfspunkte/GUI_Statisches_Modell.json).
Dieser Plan ist der einzige Umsetzungsplan fuer G-0.10.
Er erweitert weder den fachlichen Umfang noch die Abnahmekriterien.

## 1. Eintrittsgate und Abgrenzung

Das Eintrittsgate wurde durch den ausdruecklichen Umsetzungsauftrag
erfuellt. Nach der
[Nummerierungsregel](../.github/instructions/nummerierung.instructions.md)
wurden A-G-00093 bis A-G-00097 sowie U-G-000091 bis U-G-000096 oberhalb
der geprueften bisherigen Hoechstwerte A-G-00092/U-G-000090 vergeben.
Die IDEE-Zuordnung bleibt in der Entwurfspunkteliste nachvollziehbar.

Nur Statisches Modell wird geaendert. Keine neue Physik, kein neuer
Core-Laufvertrag und kein separater Core-Umsetzungsplan sind vorgesehen.
Bootsauswahl, Dynamik, physikalische Eingaben und Ergebnistabelle bleiben
erhalten. Bestehende gespeicherte Einstellungen ausserhalb dieses
Anzeigeumfangs werden nicht geloescht oder migriert.
Commit, Push, Tag und GitHub-Release sind nicht beauftragt.

Die dokumentierte Ausrichtung Bug oben in der Draufsicht wurde mit dem
Umsetzungsauftrag uebernommen.

## 2. Arbeitspakete und Reihenfolge

Die Reihenfolge der Tabelle ist die geplante Integrationsreihenfolge.
Tatsaechliche Voraussetzungen stehen ausdruecklich in der jeweiligen Zeile.

| Schritt / Arbeitspaket | Anforderungen | Umsetzung und pruefbares Ergebnis | Voraussetzung | Status |
|---|---|---|---|---|
| U-G-000091: Bestand und Vertrag abgleichen | A-G-00093 bis A-G-00096 | Bestehende Workspace-, Modell-, Pose-, Druckpunkt- und Resetpfade geprueft. Bestehende VtkSceneWidget-Pipeline wiederverwendet; kein Core-Eingriff. | Eintrittsgate | technisch abgeschlossen |
| U-G-000092: Statische Anzeige reduzieren | A-G-00093 | Grafische Kraft-/Momentenbedienung nur statisch verborgen; auch Vektordurchsicht aus der statischen Techniktabelle entfernt. Kraft-/Momenten-/Kursrichtungspfeile und Hilfslinien aus. LDP/SDP bleiben unabhaengig von dynamischen Checkboxwerten sichtbar, soweit gueltig; physikalische Eingaben bleiben erhalten. | U-G-000091 | technisch abgeschlossen |
| U-G-000093: Zusatzspalte aufbauen | A-G-00094 | Eigene Spalte zwischen Hauptansicht und Rechenbedienung; drei gleiche Bildhoehen, Seitenverhaeltnis im echten Bildraster bei drei Fenstergroessen und drei DPI-Stufen nachgewiesen. | U-G-000091 | technisch abgeschlossen |
| U-G-000094: Orthogonale Kameras einrichten | A-G-00095 | Backbord, Heck, oben; feste Parallelprojektion und Weltachsen. Wasserlinienkameras auf Z=0, Draufsicht Bug oben; keine Mausrotation der Zusatzkameras. | U-G-000093 | technisch abgeschlossen |
| U-G-000095: Gemeinsamen Zustand anbinden | A-G-00093, A-G-00096 | Gleiche Modellteile, Elementgruppen, Core-Pose und Druckpunkte in allen vier Szenen. Gemeinsames Reset/Fehlerverhalten und bestehende Schutzgeneration; statisches Technikprofil wirkt sofort auch auf die Zusatzansichten. | U-G-000092; U-G-000094 | technisch abgeschlossen |
| U-G-000096: Integration und Nachweise abschliessen | A-G-00093 bis A-G-00097 | Bestehende Grafiktests erweitert, gezielte Tests und echter EXE-Start bestanden; Ergebnisse und Einschraenkung der Standard-EXE siehe Abschnitt 5. | U-G-000095 | technisch abgeschlossen; persoenliche Nutzerabnahme offen |

Einbauorte sind die bestehende Fenster-/Workspace-
Steuerung in [main_window.cpp](../Code/src/main_window.cpp), die
Szenen-/Kameralogik in [vtk_scene.cpp](../Code/src/vtk_scene.cpp) und
die vorhandenen [Grafiktests](../Code/tests/graphics_render_tests.cpp).
Die Zusatzansichten verwenden eigene Rendererinstanzen mit derselben
bestehenden Modell-/Zustandsdarstellung, keinen eigenen Simulationspfad.
Die feste Kamera wird vor jedem Rendern aus horizontaler Bootsrichtung
und Weltachsen aufgebaut. Bei exakt vertikaler Laengsachse dient die
horizontale Backbordachse zur eindeutigen Blickausrichtung.

## 3. Meilensteine

| Meilenstein | Eintritt | Ueberpruefbare Abnahme | Stand |
|---|---|---|---|
| Freigabefaehiger Plan | Spezifikation und Plan liegen vor | Fachlicher Umfang einschliesslich Draufsicht-Annahme finalisiert, A-/U-Zuordnung vollstaendig und Umsetzung freigegeben | erfuellt |
| Statisches Layout und Kameras bereit | Eintrittsgate erfuellt, Bestandsabgleich abgeschlossen | Reduzierte Anzeige, korrekt platzierte Zusatzspalte, drei Hochformat-Bildbereiche und spezifikationskonforme Kameraeigenschaften nachgewiesen | technisch geprueft |
| Zustandsintegration bereit | Layout/Kameras bereit | Gekraengter und gestampfter Core-Zustand samt Druckpunkten in allen Ansichten identisch; Reset und Reiterwechsel ohne veraltete Anzeige; kein zusaetzlicher Physiklauf | technisch geprueft |
| Technischer Abschluss | Zustandsintegration bereit | Build und gezielte Tests bestanden, DPI-/Fensternachweise vorhanden, jeder Spezifikationspunkt belegt und verbleibende Einschraenkungen sichtbar | fuer bisherigen Umfang erreicht; Standard-EXE nachtraeglich gebaut und geprueft |
| Fachliche Nutzerabnahme | Technischer Abschluss mit vorgelegten Nachweisen | Nutzer bestaetigt Darstellung und Bedienung oder dokumentiert verbleibende Korrekturen | offen |

## 4. Pruef- und Nachweisplan

Bestehende native CMake-/CTest-Ziele wurden verwendet und die betroffenen
Grafiktests erweitert. Tatsaechliche Befehle und Ergebnisse stehen in
Abschnitt 5; die nachfolgende Tabelle beschreibt die Prueffelder.

| Prueffeld | Geplanter Nachweis |
|---|---|
| Reduzierte statische Anzeige | Sichtbare Bedienelemente und Renderer-Akteure pruefen: keine Kraft-/Momentenpfeile, -boegen oder zugehoerigen Hilfslinien; LDP/SDP beschriftet. Fremdkraft/Angriffspunkt, Elementstellungen, Berechnung und Ergebnistabelle weiter bedienbar. |
| Layout und DPI | Echte Fenster bei 100/150/200 Prozent und unterschiedlichen Fenstergroessen pruefen. Reihenfolge/Spaltenposition und gleiche Bildhoehen messen; je Bildbereich Hoehe:Breite 4:3 mit hoechstens einem Geraetepixel Rundungsabweichung. Keine verzerrte Yacht oder ueberlagerten Rechenbedienelemente. |
| Kameravertrag | Kamera- und Zielkoordinaten, Blickrichtung und Parallelprojektion direkt pruefen. Wasserlinie in Seite/Heck horizontal; Backbord- und Heckrichtung aus horizontaler Laengsrichtung, nicht aus gekraengter Hochachse. |
| Core-Zustand und Druckpunkte | Einen echten gekraengten/gestampften Ergebniszustand mit Tiefenlage in allen vier Ansichten vergleichen. Identische Pose und Core-Punktkoordinaten nachweisen; bei ungueltigem/fehlendem Druckpunkt keinen Ersatz erfinden. |
| Lebenszyklus | Ergebniswechsel, Modellwechsel, Abbruch, Reiterreset und verspaetete Ergebnisse pruefen. Keine alten Punkte/Posen und kein zusaetzlicher Solverlauf durch Zusatzansichten. |
| Regression und Einstellungen | Bootsauswahl und dynamische Anzeige unveraendert pruefen. Statische Techniksettings und bestehende dynamische Speicherwerte erhalten; Anzeigewechsel veraendert keine Last-, Stellungs- oder Solverparameter. |
| Sichtpruefung | Bildnachweise mit erkennbarer Kraengung, horizontaler Welt und lesbaren Markierungen erstellen; technische Testfreigabe von persoenlicher Nutzerabnahme trennen. |

Bei Vertragsluecken vor einer Core-Erweiterung den Nutzer informieren.
Notwendige Umfangs- oder Abnahmeaenderungen nicht stillschweigend in
Code oder Tests festschreiben.

## 5. Umsetzungsstand, Ergebnisse und offene Punkte

### Tatsaechliche Pruefung am 03.11.2026

Native Release-Builds mit MSVC `/W4 /WX` erfolgreich:

```powershell
cmake --build C:\spdeps\guibuild --config Release --target segelphysik_gui_graphics_render_tests --parallel 4
ctest --test-dir C:\spdeps\guibuild -C Release -R '^gui_(workspace_tabs_(100|150|200)|boat_inspection_100|navigation_camera|graphics_settings|graphics_dialog|force_display)$' --output-on-failure
```

**8/8 bestanden, 138,21 s.** Nach abschliessender Korrektur der
Bedienzeilen die drei Workspace-DPI-Tests erneut frisch gebaut
und ausgefuehrt: **3/3 bestanden, 129,09 s**.
Der Kamerabereich ist von den dynamischen Skalenfeldern getrennt; die
Tests pruefen auch tatsaechlichen Textplatz und fehlende Ueberschneidung
mit der Navigationscheckbox.
Die vorherigen Kursrichtungspfeil- und Druckpunktcheckbox-Tests wurden
an den ausdruecklich geaenderten statischen Anzeigevertrag angepasst;
dynamische Optionen und freie Renderer bleiben unveraendert.

Die erweiterten Tests pruefen:

- Bildraster-Verhaeltnis innerhalb eines Geraetepixels, gleiche Bildhoehen,
  Spaltenposition und Reihenfolge bei 1500x940, 1200x800 und 1024x680
  logischen Pixeln sowie 100/150/200 Prozent DPI.
- Wasserlinienkamera-Z und Ziel-Z exakt null, Parallelprojektion und
  Blickrichtungen auch bei kombinierter Kraengung, Stampfen und Gieren.
- Vollstaendige identische Core-Pose und identische Druckpunktkoordinaten
  in allen vier Szenen; ungueltige/fehlende/alte Druckpunkte bleiben weg.
- Echte stationaere Folkeboot- und 420er-Ergebnisse, positive/negative
  sowie 24-Grad-Ruderfaelle, sofortiger Technikprofilwechsel, Reiterreset
  und Schutz vor verspaeteten stationaeren Ergebnissen.
- Erhaltene physikalische Bedienung, Ergebnistabelle, dynamische
  Anzeigeoptionen, gespeicherte Einstellungen und Bootsauswahl.

Bildnachweise liegen unter
`C:\spdeps\guibuild\graphics-evidence\workspace-tabs-{100,150,200}`,
insbesondere `stationary_result.bmp`, `turning_rudder_24.bmp` und
`graphics_configuration.bmp`. Die technische Sichtpruefung fuehrte zu
zentrierten, kleineren Druckpunktbeschriftungen in den Zusatzansichten.

### Anwendung und verbleibende Einschraenkung

Die laufenden Bestandsprozesse blockieren das Ersetzen von
`C:\spdeps\guibuild\Release\segelphysik_gui.exe`; Linkerfehler LNK1104.
Keine Zustimmung zum Schliessen erhalten, daher nicht beendet.
Stattdessen separater Anwendungsbuild:

```powershell
cmake --build C:\spdeps\guibuild --config Release --target segelphysik_gui --parallel 4 -- '/p:OutDir=C:\spdeps\guibuild\g010-preview\'
```

Die erzeugte EXE wurde als
`C:\spdeps\guibuild\Release\segelphysik_gui_g010.exe` neben den vorhandenen
Laufzeitbibliotheken bereitgestellt. Echter Start geprueft: Hauptfenster
reagiert, regulaeres Schliessen mit Exitcode 0 (Pruefprozess PID 14616).
Die alten Fenster bleiben unveraendert; der Standardname ist noch der
alte Build und kann erst nach deren Schliessen neu gebaut werden.

**Nachtrag 13:28 Uhr:** Auf ausdruecklichen Nutzerauftrag wurden die
Bestandsfenster geschlossen und die Standard-EXE
`C:\spdeps\guibuild\Release\segelphysik_gui.exe` erfolgreich neu gebaut.
Echter Start und regulaeres Beenden mit Exitcode 0 geprueft
(Pruefprozess PID 25168). Die obige Sperre ist damit behoben.

### Rueckverfolgbarkeit und Abnahme

| Anforderung | Umsetzung | Tatsaechlicher Nachweis |
|---|---|---|
| A-G-00093 | U-G-000092, U-G-000095, U-G-000096 | Workspace-Tests: reduzierte Rendereroptionen und Bedienung, feste Druckpunkte trotz dynamischer Checkboxwerte, reale Core-Laeufe und unveraenderte Dynamik |
| A-G-00094 | U-G-000093, U-G-000096 | Gemessene Bildgroessen und Spaltenpositionen bei drei Fenstergroessen und drei DPI-Stufen |
| A-G-00095 | U-G-000094, U-G-000096 | Direkte Kameraabfragen fuer alle drei Projektionen, kombinierte Pose und Bug-oben-Pruefung; Bildnachweise |
| A-G-00096 | U-G-000091, U-G-000095, U-G-000096 | Identische Posen/Punkte, direkte gemeinsame Zustandsanbindung ohne Solveraufruf, Profilwechsel, Reset und spaete Ergebnisse |
| A-G-00097 | U-G-000096 | Obige Testlaeufe, Bild-/EXE-Pruefung und dokumentierte verbleibende Einschraenkung |

Technische Umsetzung ist nicht gleich persoenliche fachliche Abnahme:
Diese bleibt offen. Kein Core-Quelltext geaendert. Keine Git-Aktion;
Code, Tests und Dokumentation bleiben uncommittet.

## 6. Vier benannte, bedienbare und tauschbare Ansichten

**Status:** Finalisiert und Umsetzung beauftragt am 03.11.2026,
13:42 Uhr; technisch umgesetzt und geprueft, persoenliche Nutzerabnahme
offen. Massgeblich ist
[Spezifikationsabschnitt 6](../Spezifikationen/Spezifikation_GUI_Statisches_Modell.md#6-vier-benannte-bedienbare-und-tauschbare-ansichten).
Die neuen Punkte sind IDEE-G-00062 bis IDEE-G-00067 in der bestehenden
Entwurfspunkteliste. A-G-00098 bis A-G-00103 und U-G-000097 bis U-G-000103
wurden oberhalb der geprueften Hoechstwerte A-G-00097/U-G-000096 vergeben.
Die abgeschlossenen Schritte
U-G-000091 bis U-G-000096 werden nicht rueckwirkend als Nachweis fuer die
neue Bedienung umgedeutet.

### Eintritt und Integrationsregeln

Das Eintrittsgate wurde durch den ausdruecklichen Umsetzungsauftrag
erfuellt; neue Anforderungen und Schritte sind zugeordnet.

Nur Statisches Modell wird erweitert; keine Core-Aenderung.
Bestehende Renderer-, Eingabe- und Workspace-Helfer wiederverwenden.
Die vier bestehenden Szenen behalten ihre Identitaet; Fensterplatz,
Ansichtsname und Kameraart duerfen nicht laenger allein aus festem
Widgetnamen oder der Position im Layout abgeleitet werden.
Keine neue Simulation fuer Kameraereignisse oder Tausch.

### Arbeitspakete und Abhaengigkeiten

| Schritt / Arbeitspaket | Anforderungen | Ergebnis / geplante Abnahme | Voraussetzung | Stand |
|---|---|---|---|---|
| U-G-000097: Eingabe- und Platzzuordnung abgleichen | A-G-00098 bis A-G-00103 | Bestehende Mausgesten, Kamera-Neueinpassen pro Render, Navigation/Profil-/Resetpfade und feste Szenenzuordnungen geprueft. | Freigabe | abgeschlossen |
| U-G-000098: Gebundene Kameraeingaben ermoeglichen | A-G-00099 bis A-G-00101 | Zoom/Pan und Rotationssperre; gespeicherte Kameraskala und Bildebenenverschiebung statt Neueinpassen bei jedem Render. | U-G-000097 | technisch abgeschlossen |
| U-G-000099: Defaultansicht je gebundener Ansicht anbieten | A-G-00099 bis A-G-00101 | Drei zugeordnete Buttons setzen nur Kameraoffset und Zoom zurueck; neu eingepasste Standardkamera des aktuellen Zustands. | U-G-000098 | technisch abgeschlossen |
| U-G-000100: Benannte Ansichten und Fensterplaetze trennen | A-G-00098, A-G-00103 | Vier bestehende Szenen als Ansichten; kleine Slots mit austauschbarem Inhalt. Namen unten rechts, alte obere Titel entfernt; kleine Bildformate erhalten. | U-G-000097 | technisch abgeschlossen |
| U-G-000101: Doppelklicktausch integrieren | A-G-00102, A-G-00103 | Kleine Szene mit aktueller grosser Szene umhaengen, keine Kopie/Neuberechnung; Kamera und Funktion bleiben erhalten. | U-G-000099; U-G-000100 | technisch abgeschlossen |
| U-G-000102: Zustands- und Workspacepfade integrieren | A-G-00096, A-G-00098 bis A-G-00103 | Gemeinsame Modell-/Zustandsanbindung unveraendert; Darstellungsseite kehrt zur aktuellen grossen Ansicht zurueck. Reiterwechsel stellt Ausgangsanordnung fuer isolierte Dynamik wieder her. | U-G-000101 | technisch abgeschlossen |
| U-G-000103: Interaktion, DPI und Regression nachweisen | A-G-00098 bis A-G-00103 | Echte Maus-/Buttonereignisse, Kamera- und Platzabfragen, Bild-/EXE-Nachweise sowie freie/dynamische Regression. | U-G-000102 | technisch abgeschlossen; persoenliche Nutzerabnahme offen |

### Meilensteine und Nachweise

| Meilenstein | Eintritt | Abnahmekriterium | Stand |
|---|---|---|---|
| Freigabe des Nachtrags | Spezifikation und Plan liegen vor | Finalisierung, Implementierungsauftrag und vollstaendige A-/U-Zuordnung | erfuellt |
| Bedienbare Kameras | Eingabepaket und Defaultbuttons integriert | Zoom/Pan veraendern reale Kamera; Blickbindung bleibt auch nach Render/Resize erhalten; Default setzt nur diese Kamera zurueck | technisch geprueft |
| Tauschbare Ansichten | Kamerabedienung und Platzzuordnung integriert | Alle kleinen Plaetze tauschen mit dem grossen, auch Frei im kleinen Platz; Kameraparameter und Funktion bleiben beim mehrfachen Tausch erhalten | technisch geprueft |
| Technischer Abschluss des Nachtrags | Zustandsintegration abgeschlossen | Neue Interaktions- und DPI-Tests bestanden, Bild-/EXE-Pruefung vorhanden, Spec 6.1-6.5 gegen Nachweise abgeglichen | erreicht |
| Fachliche Nutzerabnahme | Technische Nachweise vorgelegt | Nutzer bestaetigt die neue Bedienung oder offene Korrekturen bleiben dokumentiert | offen |

Bestehende native CMake-/CTest-Grafiktests erweitern, keine neue
Testarchitektur. Insbesondere:

- Zoom und Verschieben ueber echte Mausereignisse ausloesen, anschliessend
  Render und Resize pruefen. Bei Seite/Heck nach vertikalem Pan nur
  horizontale Blickrichtung verlangen; exakte Z=0-Werte nach Default.
- Default jeder gebundenen Ansicht vor und nach Vergroesserung pruefen;
  andere Kameras und Core-Pose/Punkte unveraendert.
- Alle drei kleinen Plaetze mehrfach tauschen; Frei im kleinen Platz
  weiterhin frei bedienbar, gebundene Ansicht im grossen weiterhin
  gebunden. Doppelklick gross und auf Buttons ohne Platztausch.
- Namen unten rechts, Bedienbarkeit und fehlende Ueberdeckung bei
  100/150/200 Prozent DPI und verschiedenen Fenstergroessen pruefen.
- Bisherige Layouttests von fester Ansichtsposition auf Ausgangsanordnung
  und tatsaechliche Identitaet umstellen. Den geaenderten Kameravertrag
  ausdruecklich testen statt alte Z=0-Pruefungen pauschal abzuschwaechen.
- Reset, Modellwechsel, spaete Ergebnisse, Techniksettings,
  Bootsauswahl und Dynamik mit getauschten Ansichten regressionspruefen.

### Tatsaechliche Ergebnisse des Nachtrags am 03.11.2026

Release-Builds von `segelphysik_gui_graphics_render_tests` und
`segelphysik_gui` mit MSVC `/W4 /WX` bestanden. Die abschliessende
Regression wurde auf demselben Quellstand in zwei gezielten CTest-Laeufen
ausgefuehrt, insgesamt **8/8 bestanden**:

| Test | Ergebnis | Dauer |
|---|---|---|
| gui_workspace_tabs_100 | bestanden | 42,99 s |
| gui_workspace_tabs_200 | bestanden | 43,55 s |
| gui_graphics_settings | bestanden | 0,37 s |
| gui_navigation_camera | bestanden | 10,83 s |
| gui_boat_inspection_100 | bestanden | 19,84 s |
| gui_workspace_tabs_150 | bestanden | 45,71 s |
| gui_graphics_dialog | bestanden | 0,10 s |
| gui_force_display | bestanden | 0,02 s |

Die beiden CTest-Laeufe dauerten insgesamt 86,55 s beziehungsweise
76,87 s. Ausgefuehrte Selektoren:

```powershell
ctest --test-dir C:\spdeps\guibuild -C Release -R '^gui_workspace_tabs_(100|200)$' --output-on-failure
ctest --test-dir C:\spdeps\guibuild -C Release -R '^gui_(workspace_tabs_150|boat_inspection_100|navigation_camera|graphics_settings|graphics_dialog|force_display)$' --output-on-failure
```

Der neue `testViewInteraction` in den bestehenden Grafiktests prueft
reale Rad-/Mausereignisse und Buttons, Kamerawerte, mehrfachen Tausch,
Resize ohne Neueinpassen, Default im kleinen/grossen Platz,
Namensposition und ungekappte Buttontexte. Er laeuft innerhalb der drei
Workspace-DPI-Tests mit dem echten 420-Kurvenfahrtergebnis bei
24 Grad Ruder. Gemeinsame Pose/Druckpunkte, Rueckkehr aus Darstellung
zur aktuellen grossen Ansicht und Workspace-Reset nach Tausch sind
eingeschlossen. Die freie Kamera bleibt auch im kleinen Platz drehbar.

Ein zuerst bei 200 Prozent entdeckter zu schmaler Defaultbutton wurde
behoben: Textmetriken bestimmen seine Breite; die Mindesthoehe der
Zusatzspalte folgt den tatsaechlichen Mindestmassen ihrer Szenen und
erhaelt zugleich das 4:3-Hochformat. Aenderungen durch Layout/Schrift
werden erneut beruecksichtigt. Fruehere fehlgeschlagene Prueflaeufe
sind nicht als Abschlussnachweis gezaehlt.

Bildbelege liegen lokal unter
`C:\spdeps\guibuild\graphics-evidence\workspace-tabs-{100,150,200}\`:
`swapped_views.bmp` und `turning_rudder_24.bmp`.
Die Standarddatei `C:\spdeps\guibuild\Release\segelphysik_gui.exe`
wurde danach aus diesem Stand gebaut und gestartet:
PID 35868, sichtbares reagierendes Hauptfenster, normal ueber
`CloseMainWindow` beendet, Exitcode 0. Kein Testprozess bleibt offen.

| Anforderung | Umsetzung | Tatsaechlicher Nachweis |
|---|---|---|
| A-G-00098 Frei | U-G-000100 bis U-G-000103 | Freie Rotation im kleinen Platz, Kamerazustand bei Tausch/Resize erhalten; Navigation/Dynamik-Regression bestanden |
| A-G-00099 Seite | U-G-000098, U-G-000099, U-G-000103 | Rad/rechter Drag zoomt, mittlerer/Shift-linker Drag verschiebt; horizontal und Welt-Oben erhalten; Default Z=0 |
| A-G-00100 Heck | U-G-000098, U-G-000099, U-G-000103 | Gleiche Maus-/Defaultpruefung mit Heckrichtung, ohne Rotation oder Kraengungsuebernahme |
| A-G-00101 Draufsicht | U-G-000098, U-G-000099, U-G-000103 | Senkrechte Projektion und Bug oben bei Zoom/Pan; Default setzt nur eigene Kamera zurueck |
| A-G-00102 Tausch | U-G-000101 bis U-G-000103 | Jeder kleine Platz und Frei mehrfach getauscht; aktuelle grosse Ansicht als Gegenseite, Doppelklick gross ohne Wirkung |
| A-G-00103 Namen | U-G-000100, U-G-000101, U-G-000103 | Vier exakte Namen unten rechts und mitwandernde Controls bei 100/150/200 Prozent; keine gegenseitige Ueberdeckung |
| A-G-00096 gemeinsame Daten | U-G-000102, U-G-000103 | Identische Pose/Druckpunkte vor/nach Tausch; Darstellungsrueckkehr und Reiterreset geprueft |

**Offen:** persoenliche Nutzerabnahme der Darstellung und Bedienung.
Technische Pruefung ersetzt diese nicht. Core bleibt unveraendert;
GUI-Aenderungen sind uncommittet. Keine Git-Veroeffentlichung beauftragt.

## 7. Linkes Verschieben und feste schematische Ansichten

**Freigabe:** Direkter Umsetzungsauftrag vom 03.11.2026, 14:10 Uhr.
Massgeblich sind Spezifikation 6.2 und 7; ausschliesslich GUI.
IDEE-G-00068 praezisiert A-G-00099 bis A-G-00101;
IDEE-G-00069 ist A-G-00104 zugeordnet. Neue Kennungen oberhalb
der geprueften bisherigen Hoechstwerte A-G-00103/U-G-000103.

| Schritt | Anforderungen | Umsetzung / Nachweis | Voraussetzung | Stand |
|---|---|---|---|---|
| U-G-000104: Linkes Verschieben anbinden | A-G-00099 bis A-G-00101 | Normales linkes Ziehen nutzt vorhandenen Bildebenen-Pan; Zoomgesten bleiben erhalten. Keine Rotation, keine Aenderung von Auge-Ziel-Differenz oder Welt-Oben. | Nutzerfreigabe | technisch abgeschlossen |
| U-G-000105: Schematische Darstellung festlegen | A-G-00104 | Gebundene Kamera erzwingt festes Profil mit Wasserlinie, FXAA und geglaetteten Normalen; alle anderen Features aus. Matte deckende Yacht auch fuer Glas/Stoff, heller Hintergrund. Darstellungsseite als ausschliesslich Frei gekennzeichnet. | Nutzerfreigabe | technisch abgeschlossen |
| U-G-000106: Darstellung und Bedienung verifizieren | A-G-00099 bis A-G-00104 | Reale linke Mausgeste klein/gross, Kameravergleich, nicht uebersteuerbares Profil, tatsaechliche Actor-Deckkraft/Materialwerte und drei DPI-Stufen; freie Kamera und bestehende Oberflaechen regressionspruefen. | U-G-000104; U-G-000105 | technisch abgeschlossen; persoenliche Nutzerabnahme offen |

Eintrittsgate erfuellt. Kamera- und Rendererimplementierung liegen in
der bestehenden VtkSceneWidget-Pipeline, ohne neuen Simulationspfad.
Ansichtsidentitaet statt Fensterposition steuert das feste Profil.
Die vormals gemeinsame konfigurierbare statische Optik gilt jetzt
nur fuer Frei; Abschnitt 5/6 beschreibt den vorherigen Nachweisstand.
Die gezielten Grafiktests pruefen den neuen Vertrag, statt die alte
Profilgleichheit oder die bisher wirkungslose linke Geste beizubehalten.

**Abschlussgate:** Native Builds, drei DPI-Interaktionstests und gezielte
Regression bestanden; dokumentierte Ergebnisse und EXE-Nachweis.
Persoenliche Nutzerabnahme wird gesondert offengehalten. Kein Commit,
Push, Tag oder Release beauftragt.

### Tatsaechliche Pruefung des schematischen Nachtrags

Native Release-Builds von `segelphysik_gui_graphics_render_tests`,
`segelphysik_gui_graphics_dialog_tests` und `segelphysik_gui` erfolgreich.
Gezielter CTest-Lauf am 03.11.2026: **8/8 bestanden**, insgesamt 146,58 s.

```powershell
ctest --test-dir C:\spdeps\guibuild -C Release -R '^gui_(workspace_tabs_(100|150|200)|boat_inspection_100|navigation_camera|graphics_settings|graphics_dialog|force_display)$' --output-on-failure
```

| Test | Ergebnis | Dauer |
|---|---|---|
| gui_graphics_settings | bestanden | 0,36 s |
| gui_navigation_camera | bestanden | 9,87 s |
| gui_workspace_tabs_100 | bestanden | 38,50 s |
| gui_boat_inspection_100 | bestanden | 17,34 s |
| gui_workspace_tabs_150 | bestanden | 38,81 s |
| gui_workspace_tabs_200 | bestanden | 41,53 s |
| gui_graphics_dialog | bestanden | 0,13 s |
| gui_force_display | bestanden | 0,02 s |

`testViewInteraction` belegt normales linkes Verschieben fuer jede
gebundene Ansicht im kleinen und grossen Platz: Ziel bewegt sich;
Auge-Ziel-Differenz, Kamera-Oben und Zoom bleiben erhalten.
Der freie linke Rotationsdrag bleibt im selben Test funktionsfaehig.
Default, Zoom, Tausch und Reiterwechsel bleiben regressionsgeprueft.

`testStaticViews` prueft A-G-00104 gegen ein absichtlich abweichendes
Optikprofil mit Transparenz und HDR-Pfad: nur Wasserlinie, Smooth und
FXAA bleiben aktiv; kein HDR wird uebernommen. Renderer ohne IBL/SSAO
und Hintergrundverlauf sowie reale sichtbare Yacht-Actors mit Deckkraft
1, ohne PBR und ohne Spiegelglanz sind nachgewiesen. Die Tests laufen
auch nach Aenderung der frei konfigurierbaren statischen Einstellungen.
Der Test weist unveraenderte gemeinsame Pose/Druckpunkte weiterhin nach.

Bildbelege `swapped_views.bmp` und `turning_rudder_24.bmp` unter
`C:\spdeps\guibuild\graphics-evidence\workspace-tabs-{100,150,200}\`
wurden auf diesem Stand neu erzeugt. Sichtpruefung des
100-Prozent-Tauschbilds: heller ruhiger Hintergrund, deckende Yacht,
Wasserlinienkontur, keine Wasserflaeche oder Reflexion; Namen und
Defaultbuttons sichtbar. Vergroessern allein passt die Kamera
weiterhin bewusst nicht neu ein; dafuer dient Defaultansicht.

Mit ausdruecklicher Zustimmung wurde die laufende alte Standard-GUI
regulaer geschlossen und
`C:\spdeps\guibuild\Release\segelphysik_gui.exe` neu gebaut
(Dateizeit 03.11.2026, 14:19:14). Neustart um 14:19:31:
PID 20852, Hauptfenster vorhanden und reagierend. Die zur Nutzerpruefung
geoeffnete GUI wurde anschliessend normal beendet (Exitcode 0);
sie wird nicht ungefragt erneut gestartet.

**Abschlussgate technisch erfuellt.** Dokumentlinks/JSON und
`git diff --check` geprueft. Core-Arbeitsbaum sauber; keine Core-Aenderung.
Persoenliche Nutzerabnahme offen, GUI-Aenderungen uncommittet.

### Quick-Fix vom 03.11.2026, 14:24 Uhr

Direkt beauftragte Praezisierung von A-G-00104 und der bestehenden
Druckpunktdarstellung A-G-00096, keine Aenderung von Frei/Core.
Wasserebene wieder fest aktiv, matt und hellblau mit 14 Prozent
Deckkraft; keine Reflexion, Normalstruktur oder Animation.
Seite/Heck zeichnen fuer den exakt kantenparallelen Blick eine feine
weltfeste Wasserhoehenlinie Z=0. Gueltige LDP-/Ersatz-SDP samt Text
werden im vorhandenen Overlay ohne Uebernahme des Szenentiefenpuffers
gezeichnet; Yacht und Wasser koennen sie nicht verdecken.
Die Yacht bleibt deckend.

| Schritt | Anforderungen | Abhaengigkeit | Nachweis / Stand |
|---|---|---|---|
| U-G-000107: Dezente Wasserebene und unverdeckte Druckpunkte | A-G-00096, A-G-00104 | U-G-000105 | technisch abgeschlossen; Build und kurzer gezielter Test bestanden |

Wie gewuenscht keine ausfuehrliche Abschlussregression: Release-Builds
von Grafiktest und Standard-EXE bestanden; nur
`gui_workspace_tabs_100` ausgefuehrt und bestanden (41,46 s).
Der bestehende Test prueft jetzt auch feste Wasserdeckkraft und
ausgeschaltete Tiefenpufferuebernahme im Druckpunkt-Overlay.
`git diff --check` und JSON-Parse bestanden. Die laufende GUI wurde
mit Zustimmung regulaer geschlossen; Standard-EXE aktualisiert.
Nutzer-Sichtabnahme bleibt offen, Aenderungen uncommittet.

## 8. Antreibende Momentenanteile

Direkter Umsetzungsauftrag vom 03.11.2026, 14:54 Uhr;
Spezifikation Abschnitt 8, A-G-00105, Herkunft IDEE-G-00070.
Kennungen oberhalb der bisherigen Hoechstwerte vergeben.

| Schritt | Anforderung | Abhaengigkeit | Umsetzung / Stand |
|---|---|---|---|
| U-G-000108: Antreibende Momente darstellen | A-G-00105 | U-G-000107 | Core-Einzelmomente der externen/Antriebsquellen ohne Doppelzaehlung; wasserfeste Ansichtskomponente, gerichteter Bogen und projizierter Bezug S, transparente Wertlegende; technisch umgesetzt |

Keine Reaktionsmomente, keine neue Windphysik und keine zusaetzlichen
Kraftpfeile. Die vorhandene Bogen- und Overlaypipeline wird benutzt.
Frei bleibt ohne Zusatzdarstellung, auch beim Tausch in kleine Plaetze.

Wie beauftragt nur Release-Build und ein gezielter
`gui_workspace_tabs_100`-Lauf. Der Test prueft die echte stationaere
Fremdkraft in allen drei Ansichten sowie einen gekraengten numerischen
Kontrollfall: korrektes Momentenvorzeichen und Projektion; ein grosser
hydrostatischer Gegenbeitrag darf den angezeigten Antriebswert nicht
veraendern. Bestehende Kamera-/Tauschpruefungen laufen dabei mit.
Abschluss: beide Release-Ziele erfolgreich gebaut;
`gui_workspace_tabs_100` bestanden (40,31 s), keine breite Testsuite.
Numerischer Kontrollfall und realer Core-Fremdkraftfall bestanden.
Bildnachweis `turning_rudder_24.bmp` im vorhandenen Workspace-100-
Nachweisordner erzeugt und kurz gesichtet; anschliessend die
Wertlegende transparent an den unteren Bildrand versetzt, damit sie
keine Yacht-/Druckpunktbereiche mit einer Flaeche abdeckt.
Abschliessender Test umfasst diesen Endstand.
Standard-EXE unter `C:\spdeps\guibuild\Release\segelphysik_gui.exe`
aktualisiert. JSON-Parse und `git diff --check` bestanden,
Core-Arbeitsbaum unveraendert sauber.
Persoenliche Sichtabnahme offen, keine Git-Aktion beauftragt.

### Praezisierung Gesamtmoment und Kraftpfeile, 03.11.2026, 15:12 Uhr

Nutzerauftrag ersetzt die reine Antriebsmomentanzeige in A-G-00105
durch die vollstaendige Core-Momentensumme; neue Druckpunktpfeile
A-G-00106, Herkunft IDEE-G-00071. Spec Abschnitt 8 aktualisiert.

| Schritt | Anforderungen | Abhaengigkeit | Umsetzung / Nachweis |
|---|---|---|---|
| U-G-000109: Gesamtmoment und Druckpunktkraefte anbinden | A-G-00105, A-G-00106 | U-G-000108 | technisch abgeschlossen: Core-Gesamtmoment um S statt Quellenauswahl; externe Kraft am Ersatz-SDP, signierte Core-Y-Querkraft am LDP; Projektion in Bildebene und bestehende Skalierung |

Frei unveraendert. Keine neue Physik und keine Rekonstruktion des
Gesamtmoments aus nur zwei Kraftpfeilen. Fehlende Druckpunktkraftdaten
erzeugen keinen erfundenen Pfeil. Der Parser uebernimmt die optionale
Core-Querkraft mit expliziter Pruefung auf endliche Zahlen.

Nur Release-Builds fuer Standard-EXE und bestehenden Grafiktest sowie
`gui_workspace_tabs_100`: bestanden, 43,12 s. Der numerische
Kontrollfall prueft ausdruecklich die Core-Summe statt der einzelnen
Antriebsbeitraege; reale stationaere Daten und Kamera/Tausch laufen mit.
Kein breiter Testlauf. Standard-EXE aktualisiert; zuvor laufende GUI
mit Zustimmung regulaer geschlossen. Nutzer-Sichtabnahme offen,
uncommittet und keine Core-Aenderung.

### Schwerpunkt S statt Momentenanzeige, 03.11.2026, 15:29 Uhr

Auf Nutzerauftrag A-G-00105 zurueckgezogen und unter seiner Kennung
in der Spezifikation archiviert. Neuer Schwerpunktpunkt A-G-00107
(IDEE-G-00072); Punktfarben fuer A-G-00106 praezisiert.

| Schritt | Anforderungen | Abhaengigkeit | Umsetzung / Nachweis |
|---|---|---|---|
| U-G-000110: Schwerpunkt und Kraftpfeile bereinigen | A-G-00107, A-G-00106; Ruecknahme A-G-00105 | U-G-000109 | technisch abgeschlossen: alle schematischen Momentenboegen und Legenden entfernt; blauer Schwerpunktpunkt S aus stationaerem Core-Bezug, unverdeckt; SDP-/LDP-Pfeile exakt in Punktfarben |

Frei bleibt unveraendert. Nur Bildebenenkomponenten werden gezeichnet,
keine Ersatzsymbole fuer senkrechte Kraefte. Der gezielte Kontrollfall
prueft nichtverschwindende Kraftpfeile und exakte Farben in allen drei
Ansichten, ebenso ihr Verschwinden bei fehlenden/ungueltigen Punkten
und die ausgeschaltete Tiefenpufferuebernahme im Overlay.
Release-Builds von Standard-EXE und Grafiktest bestanden.
Nur `gui_workspace_tabs_100` abschliessend ausgefuehrt: bestanden,
40,42 s. Bildnachweis `turning_rudder_24.bmp` kurz gesichtet: S in
allen drei Ansichten, kein Momentenbogen. JSON-Parse und
`git diff --check` bestanden; Core sauber/unveraendert.
Die alte GUI wurde mit Zustimmung regulaer geschlossen;
Standard-EXE aktualisiert. Nutzer-Sichtabnahme offen, uncommittet.

### Auftrieb A und resultierende Vertikalkraefte, 03.11.2026, 15:42 Uhr

Direkter Nutzerauftrag, finalisierte A-G-00108/IDEE-G-00073;
Farbpraezisierung fuer A-G-00106 und A-G-00107. Spezifikation 8.1.
Neue Kennungen oberhalb der bisherigen Hoechstwerte vergeben.

| Schritt | Anforderungen | Abhaengigkeit | Umsetzung / Nachweis |
|---|---|---|---|
| U-G-000111: Auftriebspunkt und Gewicht/Auftrieb ergaenzen | A-G-00108, A-G-00107, A-G-00106 | U-G-000110 | technisch abgeschlossen: vorhandene Core-Beitragsaggregation wiederverwendet, A samt Auftriebssumme und Gewichtssumme an S; feste gemeinsame Punkt-/Pfeilfarben, Welt-/Modellframes und Bildebenenprojektion erhalten |

Feste Palette nur gebunden: A `#55b6f0`, S `#1a4dcc`,
SDP `#176b38`, LDP `#63cf79`. Beschriftungen in derselben Farbe.
Ungültige oder fehlende Einzelbeitraege unterdruecken die betroffene
Summenkraft, statt eine Teilsumme als vollstaendig auszugeben.
Frei unveraendert. Keine neuen Momente oder Core-Aenderungen.

Kurze Pruefung: Release-Builds von Grafiktest und Standard-EXE
bestanden; `gui_workspace_tabs_100` bestanden, 39,72 s.
Kontrollfall mit gekraengter Pose und weltfesten Vertikalkraeften:
A/S-Punkte in allen drei Ansichten, Auftriebs-/Gewichtspfeile mit
passenden Blauwerten nur in Seite/Heck, keine senkrechten Ersatzpfeile
in Draufsicht. Gruene SDP-/LDP-Pfeile, unverdecktes Overlay und
fehlende/ungueltige Druckpunkte ebenfalls im selben Test geprueft.
Keine breite Regression. Die laufende GUI wurde mit Zustimmung
regulaer geschlossen; Standard-EXE aktualisiert.
Persoenliche Nutzerabnahme offen; GUI uncommittet.

## 9. Gemeinsames statisches Kraftpfeil-Menue

**Stand:** Entwurf vom 03.11.2026, 16:12 Uhr; durch ausdrueckliches
„bitte umsetzen“ um 16:15 Uhr finalisiert und zur Implementierung
freigegeben. Stand laut Abschlussbericht vom 16:31 Uhr: Schritte
implementiert und getestet; finaler Retest bestanden, alle technischen
Gates abgeschlossen. Persoenliche Nutzerabnahme offen.
Massgeblich ist
[Spezifikationsabschnitt 9](../Spezifikationen/Spezifikation_GUI_Statisches_Modell.md#9-gemeinsames-kraftpfeil-menue-im-statischen-modell).
Herkunft IDEE-G-00074 bis IDEE-G-00079, zugeordnet zu A-G-00109 bis
A-G-00114. U-G-000112 bis U-G-000117 sind die zugehoerigen Schritte.
Keine weitere Spezifikation und kein paralleler Plan.

### Eintrittsgate und Abgrenzung

Eintrittsgate durch den ausdruecklichen Nutzerauftrag vom 03.11.2026,
16:15 Uhr erfuellt. Aktive und archivierte Kennungen geprueft:
bisherige Hoechstwerte A-G-00108/U-G-000111, neue Kennungen strikt
darueber vergeben. IDEE-zu-A-Zuordnung in der Entwurfspunkteliste
erhalten. Freigabe und Planung sind kein Implementierungsnachweis.

Nur GUI im Statischen Modell, kein Core-Release oder Core-Codeauftrag.
Die Erweiterung auf Frei gilt ausschliesslich fuer das neue gemeinsame
Kraftmenue und den Richtungspfeil; keine freie Kamera umgestalten.
Bestehende Codeaenderungen und Nachweise aus Abschnitten 1 bis 8 sind
kein Nachweis fuer diese neue Menue-/Persistenz-/Summenfunktion.

### Arbeitspakete und tatsaechliche Voraussetzungen

| Umsetzungsschritt | Anforderungen | Umsetzung / pruefbares Ergebnis | Voraussetzung | Stand |
|---|---|---|---|---|
| U-G-000112: Bestand und Kraftinventar pruefen | A-G-00109 bis A-G-00114 | Bestehende Core-Inspektion fuer Modellrollen und Quellen verwendet; keine Core-Aenderung. Gueltige Nullwerte von fehlendem/ungueltigem Inventar getrennt. | Erfuelltes Eintrittsgate | implementiert/getestet; gui_force_display und Workspace 100/200, siehe Nachweise unten |
| U-G-000113: Stationaere Bedienung reduzieren | A-G-00109 | Kurvenfahrt und modellfest=true im Laufauftrag festgesetzt, entsprechende Widgets entfernt; Langtext/Druckpunktstatus nur statisch verborgen. Andere physikalische Eingaben erhalten. | U-G-000112 | implementiert/getestet; Workspace 100/200 |
| U-G-000114: Summen fuer die vollstaendige Auswahl bereitstellen | A-G-00111 | Ruder/Schwert enthalten alle hydrodynamischen Komponenten; Modell-/Weltframes umgerechnet. Core-Gesamtkraft exklusiv, keine Addition von Anzeigezeilen; keine Teilsummen bei ungueltigem/unvollstaendigem Inventar. | U-G-000112 | implementiert/getestet; gui_force_display und Workspace 100/200 |
| U-G-000115: Gemeinsames Menue und Speicherung aufbauen | A-G-00110, A-G-00112, A-G-00114 | Gemeinsame statische Auswahl/Farben, kurze Zeilen mit Tooltips, Farbpicker mit Persistenz und reiner Farbreset implementiert. Initial einfach ein/vollstaendig aus. | U-G-000112; Kraftinventar dort festgelegt | implementiert/getestet; Workspace 100/200 und finaler Tooltip-Retest bestanden |
| U-G-000116: Vier Ansichten und Richtungspfeil anbinden | A-G-00110 bis A-G-00114 | Auswahl/Farben fuer alle vier Ansichten; vorhandene A/S/SDP/LDP-Texte an einfache Auswahlen gebunden. Vollstaendige Resultierende mit explizitem Bezug-S-Tooltip statt zusaetzlicher ueberlagernder Bildnamen. Richtungspfeil gemeinsam Frei/Draufsicht, kompassunabhaengig, bei Nullrate gerade wie bisher in Frei. | U-G-000114; U-G-000115 | implementiert/getestet; gui_force_display, Workspace 100/200 und finaler Render-Retest bestanden |
| U-G-000117: Gezielte Nachweise und Dokumentabschluss | A-G-00109 bis A-G-00114 | Standard-GUI als Release mit /W4 /WX gebaut; gezielte Tests bestanden, Regression an festen Kurvenfahrtvertrag angepasst. Ergebnisse und offene Abnahme unten dokumentiert. | U-G-000113; U-G-000116 | technisch abgeschlossen; finaler Release-Neubau und CTest 3/3 bestanden, 82,59 s; Nutzerabnahme offen |

Die stationaere Bedienreduktion und die Arbeit an Summen/Menue koennen
nach dem gemeinsamen Bestandsabgleich unabhaengig erfolgen. Die
Renderintegration braucht sowohl die zugeordneten Resultierenden als
auch den gemeinsamen Einstellungszustand.

### Integrationsregeln

- Bestehende VtkSceneWidget-, force_display-, navigation- und
  QSettings-Pfade wiederverwenden; keine zweite Simulationspipeline.
- Ein statischer Auswahl-/Farbzustand fuer alle Ansichten; Szenen
  behalten ihre Identitaet beim Tauschen. Kein Speichern in dynamische
  Profile und keine neue Konfiguration der schematischen Grundoptik.
- Rolle/Quelleninventar bestimmt angebotene Eintraege, nicht ein
  Zufallsname oder nur der gerade von null verschiedene Kraftbetrag.
  Keine stillschweigende physikalische Ersatzloesung bei Vertragsluecken.
- Allgemeine Summen ohne belegten Angriffspunkt am Bezug S kenntlich
  machen; keine unbeauftragte Momentendarstellung oder neue Physik.
- Vorherige Regeln „Frei unveraendert“ und „feste Farben“ nur in dem
  ausdruecklich beschriebenen neuen Ueberlagerungsumfang erweitern.

### Meilensteine und minimale zielgerichtete Nachweise

| Meilenstein | Eintritt | Abnahmekriterium | Stand |
|---|---|---|---|
| Freigabe und Traceability | Spec-/Planentwurf liegt vor | Ausdrueckliche Finalisierung/Umsetzungsfreigabe, neue A-/U-Zuordnung vollstaendig | erfuellt am 03.11.2026, 16:15 Uhr; keine technische Abnahme |
| Daten und Menue bereit | Bestandsabgleich | Alle zulaessigen Resultierenden korrekt, Tabelle/Defaults/Farbfelder/Picker und statische Persistenz vorhanden | technisch abgeschlossen; finaler Retest laut Bericht vom 16:31 Uhr bestanden |
| Vier-Ansichten-Integration | Daten/Menue bereit | Gemeinsames Schalten/Faerben, korrekte Projektion und Richtungspfeil in Frei/Draufsicht, fester stationaerer Laufvertrag | technisch abgeschlossen; finaler Retest laut Bericht vom 16:31 Uhr bestanden |
| Technischer Abschluss | Integration fertig | Release-Build und gezielte vorhandene Tests bestanden; alle aktiven Anforderungen belegt, Einschraenkungen dokumentiert | erreicht am 03.11.2026 laut Bericht 16:31 Uhr; Release-Neubau und finaler CTest 3/3 bestanden |
| Persoenliche Nutzerabnahme | Technische Belege vorgelegt | Nutzer bestaetigt Menue, Lesbarkeit und Darstellung | offen |

Geplante Tests in vorhandenen Runnern, keine neue Testarchitektur:

1. Reine Darstellungs-/Aggregationstests fuer alle Summen, gemischte
   Frames, Ruder/Schwert, Nullwerte und fehlende/ungueltige Beitraege.
   Gesamtkraft bleibt unabhaengig von Checkboxauswahl und ueberlappenden
   Gruppen; kein additives Zaehlen von Anzeigeaggregaten.
2. Isolierte Settings-/Dialogtests fuer Defaults, Picker-Bestaetigung
   und -Abbruch, Neustartpersistenz, ungueltige Farben und Reset nur
   der Farben. Keine persoenlichen Registrywerte ueberschreiben.
3. Ein gezielter echter Workspace-Test fuer beide Menuegruppen,
   Tooltips/Farbfelder, vier Ansichten, gemeinsame Punkt-/Pfeilwahl,
   Tauschen, Daten-/Modellwechsel und Richtungspfeil +/-/0.
   Den tatsaechlichen Core-Laufauftrag auf feste Werte pruefen.
4. Vorhandene betroffene Build-/Testziele ausfuehren; nur bei einem
   konkreten Fehler breiter eskalieren. Lesbarkeit der erweiterten
   Auswahl bei vorhandenen DPI-Stufen gezielt pruefen.

### Rueckverfolgbarkeit des freigegebenen Nachtrags

| Aktive Anforderungs-ID | Spezifikation/Abschnitt | Umsetzungsschritt(e) | Test / Abnahmekriterium und gemeldeter Nachweis | Status |
|---|---|---|---|---|
| A-G-00109 | 9.1, 9.5 | U-G-000112, U-G-000113, U-G-000117 | Feste Werte und reduzierte statische Bedienung: Workspace 100/200 bestanden | implementiert/getestet; Nutzerabnahme offen |
| A-G-00110 | 9.2, 9.3, 9.5, 9.6 | U-G-000112, U-G-000115, U-G-000116, U-G-000117 | Gemeinsames Menue und vier Ansichten: Workspace 100/200 bestanden; Bild-/Tooltipentscheidung unten | implementiert/getestet; finaler Render-/Tooltip-Retest bestanden, Nutzerabnahme offen |
| A-G-00111 | 9.2, 9.4, 9.5, 9.6 | U-G-000112, U-G-000114, U-G-000116, U-G-000117 | Rollen/Frames, vollstaendige Summen, Core-Gesamtwert, Nullwerte/Datenluecken: gui_force_display und Workspace 100/200 bestanden | implementiert/getestet; finaler Tooltip-Retest bestanden, Nutzerabnahme offen |
| A-G-00112 | 9.2, 9.3, 9.5 | U-G-000112, U-G-000115, U-G-000116, U-G-000117 | Farbpicker/Persistenz und reiner Farbreset: Workspace 100/200 bestanden | implementiert/getestet laut Umsetzungsbericht; Nutzerabnahme offen |
| A-G-00113 | 9.3, 9.5 | U-G-000112, U-G-000116, U-G-000117 | Richtungspfeil +/-/0, Frei/Draufsicht kompassunabhaengig; Nullrate gerade nach Praezisierung 16:28 Uhr: gui_force_display und Workspace 100/200 bestanden | implementiert/getestet; finaler Render-Retest bestanden, Nutzerabnahme offen |
| A-G-00114 | 9.2, 9.3, 9.5 | U-G-000112, U-G-000115, U-G-000116, U-G-000117 | Initialauswahl und keine konkurrierenden alten statischen Auswahlen; Farbreset ohne Auswahlreset: Workspace 100/200 bestanden | implementiert/getestet laut Umsetzungsbericht; Nutzerabnahme offen |

### Gemeldete Implementierung und Nachweise, 03.11.2026, 16:28 Uhr

Die folgenden Ergebnisse wurden im Umsetzungsbericht mitgeteilt; bei
dieser ausschliesslichen Dokumentationsaktualisierung keine Builds oder
Tests erneut ausgefuehrt.

| Nachweis | Gemeldetes Ergebnis | Einordnung |
|---|---|---|
| Standard-GUI `C:\spdeps\guibuild\Release\segelphysik_gui.exe` | Release-Build mit `/W4 /WX` bestanden | Stand vor abschliessender Render-/Tooltipbereinigung |
| `gui_workspace_tabs_100` | bestanden, 40,65 s | Frueherer gezielter Integrationsnachweis, kein finales Retest-Ergebnis |
| `gui_force_display` | bestanden, 0,02 s | Frueherer gezielter Darstellungs-/Aggregationsnachweis |
| `gui_workspace_tabs_200` | bestanden, 41,20 s | Frueherer Integrationsnachweis bei 200 Prozent |
| Finaler Retest nach Render-/Tooltipbereinigung | Zum Berichtszeitpunkt 16:28 Uhr noch ausstehend; um 16:31 Uhr bestanden gemeldet | Finale Ergebnisse nachfolgend; fruehere Laufzeiten bleiben historisch |

Bestehende GUI-Regression an den fest vorgegebenen Kurvenfahrtvertrag
angepasst: die ausschliessliche Geradeaus-Fehlerfixture entfernt, weil
der entsprechende Umschalter nicht mehr existiert. Tatsaechliche
vorzeichenbehaftete Kurvenfahrt, grosse Ruderausschlaege und
GUI-Integrationstests bleiben erhalten. Keine breite Regression behauptet.

Dokumentierte Darstellungsentscheidung, keine neue Anforderung:
Die betreffenden Zeilentooltips lauten ausdruecklich
**„Resultierende, Bezug S: kein nachgewiesener Angriffspunkt und keine
vollständige Momentenäquivalenz.“** Vollstaendige Kraftnamen wie
„Name (an S)“ werden bewusst nicht zusaetzlich im Bild angezeigt:
bei gemeinsam eingeschalteten Kraeften ueberlagerten sie sich in
den kleinen Ansichten. Vorhandene A/S/SDP/LDP-Texte bleiben an
die einfachen Auswahlen gebunden. Siehe Spezifikation 9.6.
Die ausdrueckliche Nutzerpraezisierung vom 16:28 Uhr erhaelt den
geraden Richtungspfeil bei Nullrate wie bisher in Frei; die vorherige
Formulierung „Nullrate ohne Pfeil“ ist damit ersetzt. Gemeinsame
Anzeige in Frei/Draufsicht unabhaengig vom Kompass.

### Finaler Wiederholungstest und technischer Abschluss, 03.11.2026, 16:31 Uhr

Laut Abschlussbericht nach Render-/Tooltipbereinigung wurde die
Standard-EXE `C:\spdeps\guibuild\Release\segelphysik_gui.exe` erneut
fehlerfrei als Release gebaut. Ein gemeinsamer CTest-Lauf:

| Test | Finales Ergebnis |
|---|---|
| `gui_workspace_tabs_100` | PASS, 40,06 s |
| `gui_workspace_tabs_200` | PASS, 42,37 s |
| `gui_force_display` | PASS, 0,14 s |
| Gesamter CTest-Lauf | 3/3 bestanden, 82,59 s |

Der erweiterte Workspace-Test prueft die ausgewaehlte Farbe an echten
VTK-Punkt- **und** Pfeilakteuren in allen vier Ansichten, den echten
Farbpicker, Persistenz nach erneutem Oeffnen, unveraenderte Auswahlen
beim Farbreset, den kompassunabhaengigen Richtungspfeil in
Frei/Draufsicht und alle vorzeichenbehafteten Kurvenfahrtfaelle.
Der reine Test prueft zusaetzlich die Auftriebskraftaggregation mit
gemischten Welt-/Modellframes und die Unterdrueckung unvollstaendiger
Summen. Diese finalen Nachweise gelten fuer die Rueckverfolgbarkeit
oben und ersetzen das zuvor offene Retest-Gate; die frueheren
Laufzeiten bleiben als Historie erhalten. Keine breite Regression
oder persoenliche Sichtabnahme daraus ableiten.

**Tatsaechlicher Stand:** U-G-000112 bis U-G-000117 implementiert und
getestet, alle technischen Gates abgeschlossen. Finaler Wiederholungstest
bestanden gemaess gemeldetem Abschlussbericht; persoenliche
Nutzerabnahme bleibt offen. GUI uncommittet; keine Core-Aenderungen
und keine Git-Aktionen. Bei dieser Dokumentationsaktualisierung keine
Builds oder Tests erneut ausgefuehrt.

## 10. Statische Wertetabelle

Freigabe: Fortsetzungsauftrag vom 03.11.2026, 17:13 Uhr;
anschliessende Praezisierung Gewichtskraft vor Auftrieb.

| Schritt | Anforderungen | Umsetzung | Voraussetzung | Stand |
|---|---|---|---|---|
| U-G-000118 | A-G-00115 | Bestehenden Filterheader mit Bootsauswahl teilen; statische Tabelle mit zehn Spalten, numerischer Sortierung, Filtern, Mehrfachauswahl und Strg+C; bisherige Dynamikdarstellung erhalten. Modellframe und Bezugspunkte aus Core-Daten/Pose anzeigen. | Bestehende Kraftdaten und vier statische Ansichten | implementiert; Workspace 100/200 bestanden |
| U-G-000119 | A-G-00116 | Gewichtskraft und Auftrieb ausschliesslich in der sichtbaren Anordnung tauschen; Schluessel/Farben unveraendert. | Bestehendes gemeinsames Kraftmenue | implementiert; sichtbare Reihenfolge im Workspace-Test geprueft |
| U-G-000120 | A-G-00115, A-G-00116 | Release-Build, Workspace-Integration inklusive Filter-/Sortier-/Auswahl-/Reihenfolgetest; vorhandene Bootsauswahl pruefen. | U-G-000118, U-G-000119 | gezielte Tests 2/2 bestanden; separater Release-Neubau erfolgreich, Standard-EXE durch laufende GUI gesperrt |

Persoenliche Nutzerabnahme offen. Keine Core- oder Git-Aktionen;
Arbeitsstand uncommittet.

Tatsaechliche Nachweise:

- Finaler Lauf `gui_workspace_tabs_100`: bestanden, 50,55 s.
- Finaler Lauf `gui_workspace_tabs_200`: bestanden, 45,47 s.
  Insgesamt 2/2 bestanden, 96,04 s.
- Geprueft: zehn Spalten, Kraefte und Momente, expliziter Momentenbezug S,
  Vektorbetrag passend zu Komponenten, kombinierte Spaltenfilter,
  numerische Sortierung, veraenderbare Breiten, Mehrfachauswahl, alter
  Infoblock/Copy-Button statisch verborgen, Pfeilauswahl unabhaengig,
  Gewichtskraft vor Auftrieb. Die vorhandenen Bootsauswahl-/Dynamikfaelle
  im Workspace-Test bleiben bestanden.
- Tabellen- und Bootsauswahlfilter verwenden denselben Header.
  Keine breite Vollregression behauptet; Strg+C nicht separat automatisiert.
- Standard-Linkversuch meldete LNK1104: laufende Standard-GUI PID 42724
  sperrt ihre EXE. Ohne Schliessfreigabe bleibt dieses Fenster unberuehrt.
  Stattdessen unterstuetzten MSBuild-Ausgabenamen verwendet:
  `C:\spdeps\guibuild\Release\segelphysik_gui_updated.exe`.
  Release-Build erfolgreich. Separater Start/Normalabschluss des
  Neubaus vor abschliessender Summenabsicherung: bereit, Exitcode 0.
  Standard-EXE und bereits laufendes Fenster sind noch der alte Stand.

## 11. Navigation als eigene statische Spalte

| Schritt | Anforderung | Umsetzung | Voraussetzung | Stand |
|---|---|---|---|---|
| U-G-000121 | A-G-00117 | Vorhandene Navigation statisch in eigene Scrollspalte zwischen Auswahl und Hauptbild umhaengen; Dynamik-Overlay erhalten; Schalter und Ansichtstausch konsistent anbinden. | Bestehende Navigation und statisches Vieransichtenlayout | implementiert; Workspace 100/200 bestanden |
| U-G-000122 | A-G-00117 | Workspace-Tests mit Platzierung und Spaltenschalter, Instrumentwerten, Ansichtstausch und Dynamikrueckkehr; Release-Neubau. | U-G-000121 | gezielte Tests 2/2 bestanden; separater Release-Neubau erfolgreich |

Nutzerabnahme offen, keine Core-/Git-Aktionen. Laufende Standard-GUI
bleibt unangetastet; Neubau weiterhin separat, solange gesperrt.

Tatsaechliche Nachweise: `gui_workspace_tabs_100` bestanden, 50,00 s;
`gui_workspace_tabs_200` bestanden, 47,93 s; 2/2 in 97,95 s.
Platzierung zwischen Auswahl und Hauptbild, Schalter ohne Restspalte,
Instrumentwerte, bestehender Ansichtstausch und Workspacewechsel sind
im Integrationstest enthalten. Sichtpruefung des tatsaechlichen
Workspace-Testbildes bestaetigt separate Spalte ohne Bootueberdeckung;
bei zu geringer Hoehe steht eine vertikale Scrollleiste bereit.
`segelphysik_gui_updated.exe` als Release neu gebaut. Standard-EXE
und laufende Standard-GUI bleiben unveraendert. Keine breite Regression
oder persoenliche Abnahme behauptet; Arbeitsstand uncommittet.

### 11.1 Instrumentlayout und Lageansichten

| Schritt | Anforderungen | Umsetzung | Voraussetzung | Stand |
|---|---|---|---|---|
| U-G-000123 | A-G-00118 | Statischen Titel verbergen; Geschwindigkeitshoehe fixieren, restliche Hoehe auf drei Instrumente verteilen, Spaltenbreite anpassen, 6-Pixel-Abstaende. | U-G-000121 | implementiert |
| U-G-000124 | A-G-00119, A-G-00120 | Persistente Lageansichten-Checkbox direkt unter Navigation; rechte Spalte schalten; Wiedergabe statisch vollstaendig ausblenden. | Bestehende Workspace-/Vieransichtensteuerung | implementiert |
| U-G-000125 | A-G-00118 bis A-G-00120 | Resize-/Abstands-/Schalter-/Wiedergabetests, Workspace-Regression und Standard-EXE-Neubau. | U-G-000123, U-G-000124 | Workspace 100 bestanden, 49,23 s; Workspace 200 bestanden, 43,49 s; Standard-EXE erfolgreich neu gebaut |

Nutzerabnahme bleibt offen. Keine Core- oder Git-Aktionen.

Nachweise: Resize erhaelt Geschwindigkeitshoehe; Instrumentwidgets teilen
Resthoehe mit 6-Pixel-Abstaenden und unterem Geschwindigkeitspanel.
Navigationsueberschrift und statische Wiedergabezeile sind unsichtbar.
Lageansichten schaltet die rechte Spalte aus und wieder ein. Bestehende
Workspace-/Ansichtstauschtests bei 100/200 Prozent bestanden.
Sichtkontrolle des aktuellen Testbildes durchgefuehrt.
Standardausgabe `C:\spdeps\guibuild\Release\segelphysik_gui.exe` als
Release neu gebaut, nicht nur separate Updated-Ausgabe. Arbeitsstand
uncommittet, persoenliche Abnahme offen.

### 11.2 Sichtbare Kartenkanten

U-G-000126 zu A-G-00121: Karten-SVG auf sichtbare Kartenflaeche begrenzen,
Hoehen zuerst mit 6-Pixel-Abstaenden und fester Geschwindigkeit berechnen,
Breite daraus ableiten; Panelrundungen auf 12 Pixel angleichen.
Implementiert; finaler Abschlussnachweis nach Breitenkorrektur:
Workspace 100 bestanden, 50,30 s; Workspace 200 bestanden, 51,22 s.
Zusammen 2/2 bestanden, 101,54 s. Buendigkeit, Geschwindigkeitshoehe,
Abstaende und Kartenradius automatisiert geprueft.
Separater Release-Neubau `segelphysik_gui_updated.exe` erfolgreich.
Standard-Link nach letzter Breitenkorrektur durch erneut laufende GUI
gesperrt (LNK1104); laufendes Fenster nicht geschlossen.
Nutzerabnahme offen, keine Core-/Git-Aktionen, uncommittet.

### 11.3 Spaltentrenner und Resize-Cursor

U-G-000127 zu A-G-00122: Gemeinsamen FilterHeader fuer Bootsauswahl
und statische Wertetabelle um helle senkrechte Kopftrenner und
horizontalen Doppelpfeil-Cursor an interaktiven Grenzen ergaenzt.
Filterfelder lassen jeweils vier Pixel am Rand fuer das Ziehen frei.
Implementiert; auf ausdruecklichen Nutzerwunsch keine Tests ausgefuehrt.
Release-Neubau erfolgreich als `segelphysik_gui_updated.exe`.
Standard-Link durch laufende Standard-GUI gesperrt (LNK1104);
Standard-EXE unveraendert, laufendes Fenster nicht geschlossen.
Keine Tests ausgefuehrt. Nutzerabnahme offen, Arbeitsstand uncommittet.

Nachtrag 03.11.2026, 17:56 Uhr: Nutzer hat ausschliesslich Linien-/Cursor-
Tests nachtraeglich beauftragt. Eigener Modus
`--column-boundaries-only` prueft beide echten Tabellen ohne Modelllauf:
helle senkrechte Rasterlinie ueber Titel und Filterzeile, horizontaler
Doppelpfeil an zwei Grenzen jeweils im Titel-/Filterbereich und kein
Resize-Cursor abseits der Grenze. Bestanden. Isolierte temporaere
Einstellungen, keine anderen Testfaelle ausgefuehrt.
Die Trenner werden nach dem gesamten Header gezeichnet, damit der
Style sie nicht in der Titelzeile abschneidet.
Standard-EXE `C:\spdeps\guibuild\Release\segelphysik_gui.exe` erfolgreich
neu gebaut (17:55:58 Uhr); kein gesperrter Standardprozess vorgefunden.
Nutzerabnahme offen, uncommittet.

## 12. Dynamisches Modell an die statische Oberflaeche angleichen

Direkter Nutzerauftrag vom 03.11.2026, 17:59 Uhr, mit anschliessendem
„go“. Massgeblich ist Abschnitt 12 der bestehenden
[Layout-Spezifikation](../Spezifikationen/Spezifikation_GUI_Statisches_Modell.md).
Dieser Nachtrag erweitert die fruehere Abgrenzung „nur statisch“ auf
die gemeinsamen Anzeigeoberflaechen, nicht auf den stationaeren
Core-Laufvertrag. Kein neuer Plan, keine Core-Aenderung/Git-Aktion.
Neue IDs oberhalb A-G-00122/U-G-000127 nach Bestandspruefung.

| Schritt | Anforderungen | Voraussetzung | Umsetzung / Nachweis |
|---|---|---|---|
| U-G-000128: Gemeinsame Anzeige fuer Dynamik verdrahten | A-G-00123 | Freigegebener Nutzerauftrag; vorhandenes statisches Layout U-G-000127 | Gemeinsame Seitenleiste, Navigation, vier Ansichten, Kraftauswahl und Tabelle fuer beide Modellreiter; Legacy-Anzeige dynamisch verborgen; Snapshot-/Reset-/Tauschpfade erweitert. |
| U-G-000129: Laufbedienung und Kamerabezug anbinden | A-G-00124 | U-G-000128 | Bestehende Dauer/Schritt/Takt-, Antriebs- und Live-Steuerung erhalten; Koerper-/Weltkamera ohne Sprung, physikalischer Core-Auftrag unveraendert. Bedeutung ist offen offengelegte technische Interpretation, keine Nutzerbestaetigung. |
| U-G-000130: Gezielte Integration und Auslieferung pruefen | A-G-00123, A-G-00124, A-G-00122 | U-G-000128; U-G-000129 | Eigener Modus `--dynamic-layout-only`: echter kurzer Folkeboot-Lauf mit Testantrieb, Snapshotgleichheit, Tabelle/Filter, Sichtbarkeit, Ansichtstausch, Kamerabezug und Rueckwechsel. `--column-boundaries-only` auf alle drei Reiter erweitert. Release-Neubau der Standard-EXE vorgesehen. |

Der horizontale Resize-Cursor verwendet nun `Qt::SizeHorCursor` auf
Header und Viewport statt nur `Qt::SplitHCursor` auf dem Viewport.
Das ist eine optionale Nachbesserung des gemeldeten Cursorproblems;
ein bestandener Qt-Formtest ersetzt keine persoenliche Sichtabnahme
des nativen Mauszeigers.

Nachweise: Implementierung und erster gezielter Dynamiktest bestanden.
Erster Testversuch scheiterte ausschliesslich am PNG-Schreiben der
Testaufnahme; bestehendes BMP-Ausgabeformat wiederverwendet, Retest
bestanden. Abschliessender Release-Build von Standard-EXE und
Grafiktest erfolgreich; Standarddatei am 03.11.2026, 18:05:08 Uhr
aktualisiert. `--dynamic-layout-only` einschliesslich tatsaechlichem
Core-Testantrieb, Filter und vier Ansichten bei 100 und 200 Prozent
bestanden. `--column-boundaries-only` in allen drei Reitern bestanden.
Aktueller BMP-Bildnachweis gesichtet: dynamisches Layout, Testantrieb
und numerische Tabelle sichtbar. Keine breite Testsuite ausgefuehrt.
Damit U-G-000128 bis U-G-000130 technisch gezielt nachgewiesen.
Persoenliche Nutzerabnahme, insbesondere Bedeutung der
Koerper-/Welt-Auswahl, bleibt offen.

Zusaetzliche gezielte Regression nach Anpassung der alten dynamischen
Layoutannahmen: `gui_workspace_tabs_100` bestanden, 42,10 s.
Die Standard-EXE wurde anschliessend gestartet: sichtbares,
antwortendes Fenster nachgewiesen und regulaer geschlossen, Exit 0.
`git diff --check` ohne Fehler. Keine Commits, Tags oder Pushes;
Arbeitsstand bleibt uncommittet.

| Aktive Anforderung | Umsetzung | Tatsaechlicher Nachweis | Status |
|---|---|---|---|
| A-G-00123, Abschnitt 12 | U-G-000128; U-G-000130 | Dynamikmodus 100/200: echter Core-Lauf, vier gleiche Posen, Tabelle/Filter, grafik-unabhaengige Zahlen, Tausch und Reset; Workspace-Regression 100 | technisch nachgewiesen; Nutzerabnahme offen |
| A-G-00124, Abschnitt 12 | U-G-000129; U-G-000130 | Sichtbare Laufparameter/Testantrieb, kurzer dynamischer Core-Lauf, Weltkamera unveraendert bei Translation, Koerperkamera folgt ohne Sprung, statischer Rueckwechsel | technisch nachgewiesen; Bedeutung der Auswahl offengelegte Annahme |
| A-G-00122, erweiterter Umfang | U-G-000127; U-G-000130 | Helle Rasterlinien und horizontaler Resize-Cursor auf Header/Viewport in allen drei Reitern | Qt-Nachweis bestanden; native Sichtabnahme offen |

<a id="13-planentwurf-einheitliche-rechenbedienung-und-performancevergleich"></a>

## 13. Einheitliche Rechenbedienung und Performancevergleich

**Stand:** 03.11.2026, ausdruecklicher Planauftrag nach der
Praezisierung „zuerst GUI und Mindestanpassungen im Core, danach
ordentlich testen und mit aelteren Versionen vergleichen“.
**Status:** Durch Umsetzungsauftrag vom 03.11.2026, 18:43 Uhr
finalisiert. GUI/Core-Mindestintegration technisch geprueft und am
verbindlichen Haltepunkt zur persoenlichen GUI-Pruefung gestoppt.
Analyse-Go vom 03.11.2026, 19:23 Uhr hebt nur die Diagnosesperre auf;
Optimierung und weitere Produktionsumsetzung bleiben gesperrt.
Massgeblich ist
[Spezifikation Abschnitt 13](../Spezifikationen/Spezifikation_GUI_Statisches_Modell.md#13-identische-rechenbedienung-fuer-statik-und-dynamik).
Offene Punkte und Herkunft:
[Entwurfspunkteliste](../Spezifikationen/Entwurfspunkte/GUI_Dynamische_Rechenbedienung.json).
Gegenstueck:
[Core-Plan, vorgeschlagen C-0.9](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.9.md).
Dies bleibt der einzige GUI-Plan fuer G-0.10. Die Nachweise aus
Abschnitt 12 bleiben als Nachweise des damaligen Umfangs erhalten,
erfuellen aber nicht automatisch die hier geaenderte Bedienung.

### 13.1 Eintrittsgate und Grenzen

**Verbindlicher Haltepunkt, Nutzerpraezisierung 03.11.2026, 18:42 Uhr:**
Die naechste Umsetzung endet nach dem GUI-Umbau einschliesslich der
dafuer notwendigen Core-Mindestanpassungen und gezielten technischen
Funktionspruefung. Danach Stand zur persoenlichen GUI-Pruefung vorlegen
und stoppen. Ohne erneutes Nutzer-Go weder den historischen
Performancevergleich noch weitergehende Optimierungen oder weitere
Umsetzungspakete starten. Die technische Pruefung ersetzt diese
persoenliche Freigabe nicht.
Das erneute Go ist fuer die Performance-Ursachenanalyse inzwischen
erteilt (Abschnitte 13.6 ff.); die uebrigen Grenzen gelten unveraendert.

Die Spec ist fuer diesen Umfang finalisiert; die Core-Schnittstelle
verwendet unveraenderliche modellfeste Kraft-/Ansatzpunktdaten.
Mangels erreichbarem Nutzer bleibt die Modellwahl Kamerabezug:
Koerpermodell folgt Pose, Weltmodell folgt Position mit weltorientierter
Kamera. Diese offengelegte technische Auslegung ist bei der persoenlichen
Pruefung noch zu bestaetigen, keine bestaetigte fachliche Nutzerentscheidung.
Die bereits vorgeschlagene absolute Weltkamera ist keine Freigabe:
Bootsnachfuehrung und Zentrierung wie vor dem Umbau erhalten.
Keine neue Physik in der GUI, keine Windphysik und kein automatisch
vorgeschalteter stationaerer Solver.

Vor Vergabe wurden aktive und historische Kennungen geprueft:
bisherige Maxima A-G-00124 / U-G-000130.
Neue Anforderungen A-G-00125 bis A-G-00129 stehen ausschliesslich
in der Spezifikation; Schritte U-G-000131 bis U-G-000139 hier.
Finalisierung ist kein technischer oder persoenlicher Abnahmenachweis.

### 13.2 Arbeitspakete und tatsaechliche Abhaengigkeiten

| Arbeitspaket | Entwurfsquelle | Voraussetzung | Ergebnis und Abschlusskriterium | Status |
|---|---|---|---|---|
| U-G-000131: Vertrag und aktuellen Stand festhalten | A-G-00125 bis A-G-00129 | Eintrittsgate | Spec finalisiert, Core-API vereinbart, Kameraauslegung offengelegt. Kein automatischer Commit. | umgesetzt; persoenliche Auslegungsklaerung offen |
| U-G-000132: Rechte Rechenleiste gemeinsam verwenden | A-G-00125, A-G-00128 | U-G-000131 | Statische Leiste dynamisch nahezu 1:1, gruen statt blau; Fremdkraft/Ansatzpunkt/feste Stellungen identisch. Testantriebsgruppe und Live-Slider entfernt. | technisch geprueft, Sichtabnahme offen |
| U-G-000133: Darstellung strikt pro Modellmodus trennen | A-G-00125 | U-G-000131 | Gleiche Optionen; Darstellung, Auswahl und Farben nur im aktiven Modus, getrennt persistent. | technisch geprueft einschliesslich Neustart |
| U-G-000134: Startdialog einfuegen | A-G-00126 | U-G-000131 | Dauer/Schrittweite/Echtzeit nur beim Start, zuletzt bestaetigte Sitzungswerte; Abbruch ohne Datenverlust. | technisch geprueft |
| U-G-000135: Eingabeuebernahme und Laufabbruch absichern | A-G-00128 | U-G-000132 | Eingaben beidseitig erhalten; Lauf stoppen und spaete Ergebnisse abweisen. | technisch geprueft |
| U-G-000136: Dynamischen Core-Auftrag anbinden | A-G-00127, A-G-00128 | U-G-000132, U-G-000134; Core-Mindestvertrag | Unveraenderlicher Kraftfall und feste Stellungen ab Zustand null; V7-Ausgabe strikt lesen. Kein alter Testantrieb/Begrenzer. | technisch geprueft, Batch/Live identisch |
| U-G-000137: Kamera und vier Ansichten abgleichen | A-G-00125, A-G-00129 | U-G-000131, U-G-000136 | Beide Kameramodi folgen dem Boot, Pan/Zoom bleiben; echte Snapshots in vier Ansichten. | technisch geprueft; Auslegungsbestaetigung offen |
| U-G-000138: Gemeinsames Funktionsgate pruefen | A-G-00125 bis A-G-00129 | U-G-000132 bis U-G-000137; Core-Mindesttests | Gezielte Tests, Standard-EXE, ehrliche offene Punkte; persoenliche GUI-Abnahme getrennt. | Mindestintegrationsgate bestanden; persoenliche Abnahme und vollstaendige fachliche Matrix offen |
| U-G-000139: Historische GUI-/Core-Performance messen | A-G-00129 | Mindestintegration; Analyse-Go 03.11.2026 19:23 Uhr; Core-Vergleichsplan | Alle laufzeitrelevanten Diffs erfassen; statisch/dynamisch, einfache Darstellung ein/aus, getrennte Komponenten und Modellstruktur. Optimierungen erst nach Ergebnisfreigabe. | Ursacheninventar und begrenzte GUI-/Core-Versionsmessungen vorgelegt (13.7-13.9); nicht isolierte Anteile und offene Physiknachweise ausdruecklich benannt. Keine vollstaendige fachliche Abnahme; Optimierung gesperrt |

Rechenleiste, Darstellungstrennung und Startdialog koennen nach
Vertragsklaerung unabhaengig vorbereitet werden. Echter dynamischer
Start und Integrationsabnahme benoetigen die fertigen Core-Pfade.
Keine Implementierungsreihenfolge allein aus der Tabellenreihenfolge
ableiten.

### 13.3 Meilensteine und gezielte Abnahme

| Meilenstein | Eintritt | Erforderlicher Nachweis |
|---|---|---|
| Spec und Schnittstelle bereit | Entscheidungen geklaert | Finale Spezifikationen, A-/U-Zuordnung, Core-API und Modellwahlsemantik stehen fest. |
| GUI-Bedienung bereit | Rechenleiste, Dialog, Darstellung und Wechselpfad umgesetzt | Statik/Dynamik bei 100/200 Prozent direkt vergleichen; gleiche Geometrie, Felder, Einheiten, Grenzen, Defaults, Tastaturbedienung. Nur vereinbarte dynamische Unterschiede und Gruen/Blau. |
| Echter dynamischer Rechenfall korrekt | Core-Mindestanpassungen abgeschlossen | Nichttriviale XYZ-Kraft, Ansatzpunkt und von Defaults abweichende Stellungen erreichen den Core exakt; Kraefte/Momente und Posen erscheinen korrekt. Null/deaktiviert/ungueltig unterscheiden. |
| Nahtloser Wechsel und Darstellungstrennung korrekt | Bedienung und dynamischer Fall bereit | Eingabewerte in beide Richtungen erhalten; laufender Solver stoppt, keine spaete Ergebnisuebernahme. Statische Darstellungsaenderung laesst Dynamik unveraendert und umgekehrt, auch nach Neustart. |
| Integration auslieferbar | Fachliche Matrix und GUI-Tests bestanden | Kamera zentriert ohne Nutzergesten zu ueberschreiben; vier Ansichten, Tabelle/Filter/Sortierung/Kopieren, Stop und Rueckblick korrekt. Standard-EXE geprueft, offene Sichtabnahme benannt. |
| Historische Ursache eingegrenzt | Erst nach Integrationsgate | Messbericht mit Hashes, Rechenfaellen, Rohzeiten und Vergleichbarkeit; Core-Mehrphysik von GUI-Mehrarbeit und echten Regressionen unterschieden. |

Startdialog-Abbruch separat pruefen: kein neuer Lauf, kein Verlust
alter Ergebnisse, keine Aenderung zuletzt bestaetigter Parameter.
Nicht nur Widgets vorhanden pruefen, sondern tatsaechliche Wirkung.
Vorhandene gezielte Tests erweitern; keine breite Suite ohne Anlass.

### 13.4 Historischer Vergleich und Performance-Vorgehen

**Aktualisierung 03.11.2026, 19:23 Uhr:** ausdrueckliches Go fuer
vorrangige Ursachenanalyse. Das bisherige Eintrittsgate wird nur fuer
diese Analyse aufgehoben. Umsetzung von Optimierungen bleibt gesperrt;
persoenliche Abnahme und offene Physikmatrix sind nicht damit erledigt.
Nutzerhinweis vom 03.11.2026, 18:42 Uhr: Die Modellstruktur wurde
geaendert. Als Ursachenhypothesen sind komplexere abgeleitete Modelle
und haeufigere Neuberechnungen ausdruecklich zu untersuchen, nicht
als bereits nachgewiesene Ursachen darzustellen. Quell-/Ableitungsmeshes,
Element-, Dreiecks- und Panelzahlen sowie Ableitung, Laden,
Validierung, Cache-Nutzung und wiederholte Geometrie-/Kraftauswertungen
je Schritt und angezeigtem Snapshot erfassen. Historische und heutige
Modelldaten getrennt von Softwareaenderungen vergleichen; soweit
kompatibel dieselbe Geometrie verwenden. Keine Modelle still
vereinfachen, migrieren oder ueberschreiben.

Nutzerfestlegung vom 03.11.2026, 18:41 Uhr: Bevorzugte historische
Vergleichsbasis ist **Core-Tag C-0.7**, nicht GUI-Tag G-0.7.
Verifizierter Core-Commit:
`ebca994770b216a749e18917d2ae387de1612dff` vom 28.10.2026.
Zuerst nach funktionaler Mindestintegration die Core-Rechenzeit
gegen diese Basis vergleichen; passende GUI-/Modellkompatibilitaet
separat belegen. Die folgenden GUI-Staende sind nur Kandidaten zur
Einordnung, keine vom Nutzer ausgewaehlte GUI-Basis.

Historische GUI-Kandidaten vom lokalen Git-Verlauf:

| GUI-Stand | Datum | Einordnung |
|---|---|---|
| `67c2ce7` / G-0.9 | 02.11.2026 | Letzter veroeffentlichter Stand vor aktuellen Layoutaenderungen |
| `194dbbb` | 28.10.2026 | LDP und Ruder-/Schwertbedienung |
| `59f22d6` | 28.10.2026 | Live-Hydrodynamikkontext |
| `e54497f` | 27.10.2026 | Etwa eine Woche zurueck: Grafikprofile und koerperfeste Fahrtdarstellung |

Diese Liste ist eine Untersuchungsreihenfolge, kein bereits gemessener
Leistungsnachweis. Kompatible Core-Staende aus dem verlinkten Core-Plan
zuordnen. Aenderungen am dynamischen Aufrufpfad anhand Code-Diffs
nachvollziehen, nicht allein aus Commitbetreff/Datum schliessen.

- Alte Quellen separat bereitstellen und bauen, keine aktuellen
  Branches wechseln oder Dirty-Arbeit verlieren. Release-Builds,
  gleiche Hardware, Fenstergroesse/DPI, Renderprofil, Kamera,
  Modellgeometrie und Laufdaten soweit kompatibel.
- Zuerst isolierten Core messen (Core-Plan); dann JSON/Force-Parsing,
  Szenenaufbau, einzelne Snapshotdarstellung, Werteaufbereitung und
  Rendering getrennt. Schliesslich echten Livebetrieb mit Core-/GUI-
  Konkurrenz pruefen. Ladezeit und stationaere Solversuche nicht
  faelschlich als dynamische Schrittzeit rechnen.
- Drei Sichtfaelle vergleichen: freie Einzelansicht, vier sichtbare
  Ansichten, Zusatzansichten verborgen. Keine Verbesserung aus
  blossem Verbergen behaupten, solange unsichtbar weitergerechnet wird.
- Simulation unveraendert halten: gleiche Schritte und numerische
  Resultate. Echtzeit-Wartezeit fuer Durchsatzmessung ausschalten,
  die normale Echtzeitbedienung gesondert pruefen.
- Ein Warmlauf und mindestens fuenf Wiederholungen, Median/Streuung,
  Snapshot-ms, sichtbare Aktualisierungen/s, Core-ms/Schritt,
  Resultatgleichheit und Messkonfiguration protokollieren.
- Historische Schemas, Features und Modelle koennen abweichen:
  direkt vergleichbaren gemeinsamen Umfang getrennt vom damaligen
  vollstaendigen Umfang messen. Nicht vergleichbare Paarungen
  ausdruecklich kennzeichnen, keine unbelegten Beschleunigungsfaktoren.
- Bestehender Komponenten-Audit aus Spec 13.5 bleibt Vorbefund,
  kein historischer Vorher-nachhervergleich.
- Erst nach Ursachenbeleg gezielte GUI-Optimierungen priorisieren:
  identische Profilzuweisung, mehrfaches synchrones Rendering,
  verborgene Ansichten/alte Tabelle. Keine Core-Schritte auslassen,
  dt vergroessern oder Physik vereinfachen. Optimierung ist ein
  gesondert auszuweisender Folgeschritt, nicht Voraussetzung fuer
  einen ehrlichen Versionsvergleich.

### 13.5 Aktueller Stand

GUI-Umbau und notwendige Core-Mindestanpassungen implementiert.
Keine Commit-/Push-/Tag-/Release-Aktion; beide Arbeitsstaende bleiben
uncommittet. Standard-EXE in `C:\spdeps\guibuild\Release\segelphysik_gui.exe`
neu gebaut und auf antwortendes Fenster sowie normales Beenden mit Exit 0
geprueft. Screenshot der echten Dynamikoberflaeche visuell kontrolliert.

Finaler Release-Build erfolgreich. Gezielter CTest-Lauf am 03.11.2026:

| Test | Ergebnis | Dauer |
|---|---|---|
| gui_workspace_tabs_100 | bestanden | 52,75 s |
| gui_workspace_tabs_200 | bestanden | 52,78 s |
| gui_dynamic_layout_100 | bestanden | 9,43 s |
| gui_dynamic_layout_150 | bestanden | 9,67 s |
| gui_dynamic_layout_200 | bestanden | 9,05 s |
| gui_core_integration | bestanden | 36,37 s |
| gui_force_display | bestanden | 0,07 s |

7/7 bestanden, 170,15 s insgesamt; kein Performancevergleich.
Nachweise in `C:\spdeps\guibuild\Testing\Temporary\LastTest.log` und
`C:\spdeps\guibuild\graphics-evidence\dynamic-layout-100` beziehungsweise
`-150`, `-200`; Screenshots als BMP. Die neuen Dynamiktests sind dauerhaft
im vorhandenen CTest-GPU-Gate registriert, kein neues Testwerkzeug.

- A-G-00125: gemeinsame Leiste/Ansichten, dynamisches Gruen; echte
  Darstellungsaenderung, getrennte Kraftauswahl und Wiederherstellung
  nach Reiterwechsel sowie frischer MainWindow-Instanz.
- A-G-00126: echter Dialogstart und Abbruch vor sowie nach einem Lauf;
  letzter bestaetigter Parameter und vorhandenes Ergebnis bleiben erhalten.
- A-G-00127: reale XYZ-Kraft `[150,10,-2]` N und Modellpunkt `[1,0,3]` m
  im Core-Zustand exakt nachgewiesen. Moment jeder Achse gleich
  `(P-S) x F` mit Fehler kleiner `1e-10` Nm; Batch/Live-Endpose und
  Winkelgeschwindigkeit innerhalb `1e-12` gleich. V7-Kontext und
  veraltete/fehlerhafte Auswertung werden strikt validiert.
- A-G-00128: 420 mit Ruder 5 Grad und Schwert 50 Prozent ab Zustand null
  und in saemtlichen Snapshots unveraendert. Ungueltige 91 Grad werden
  explizit abgewiesen. GUI-Eingaben beidseitig erhalten; alte Live-Slider
  bleiben verborgen/gesperrt. Workspace-Regression prueft Laufabbruch
  und verworfene spaete Ergebnisse.
- A-G-00129: Tabellenfilter, vier echte Zustandsansichten, Ansichtstausch,
  Bootsnachfuehrung und weltorientierte Kamera auch bei Bootsrotation
  geprueft. Die nachstehend benannten offenen Nachweise bleiben offen.

Core-Nachweise und CPPI-Vertrag stehen ausschliesslich im
[Core-Plan](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.9.md):
A-C-00126 bis A-C-00128, U-C-000112 bis U-C-000114, CPPI-0105 bis
CPPI-0112. Core-Mindesttests `hydrodynamik_traegheit` und
`simulation_gesamtlauf` bestanden. Zusaetzlicher Core-Gesamtlauf 19/21:
`modellkatalog_schnittstelle` erwartet die nicht mehr vorhandene
`meshes\gui\rumpf.obj` statt `boot.obj`; `modell_geometrie_v2` verwendet
beim Schema-2-Import eine Schema-3-Ableitung `z_schnitt`.
Beide Fehler mit unveraendertem Core-HEAD `d6d57c7` und denselben
Modellen reproduziert; keine fachfremde Reparatur oder gruene Gesamtsuite
behauptet.

**Hier gestoppt:** persoenliche GUI-Pruefung und Bestaetigung der
offengelegten Koerper-/Weltkameraauslegung fehlen. Die komplette
fachliche Uebernahmematrix aus Abschnitt 13.5 der Spezifikation ist nicht
als vollstaendig nachgewiesen: insbesondere dt-Verfeinerung, volle
Ruderwinkelmatrix und weitergehende Kirchhoff-/Flutungsreferenzen
bleiben offen. Vor spaeterem Performancevergleich diese Nachweise
abschliessen; technische Mindestintegration ist kein Beleg fuer alle
physikalischen Referenzen. Historischer Vergleich mit C-0.7,
Modellstruktur-/Nachrechnungsanalyse und weitergehende Optimierung
blieben an diesem damaligen Haltepunkt gesperrt. Fuer die Analyse gilt
inzwischen der nachfolgende Auftrag; Optimierung bleibt gesperrt.

### 13.6 Analyseauftrag vom 03.11.2026, 19:23 Uhr

Auf ausdrueckliches Go jetzt Performance-Ursachen vor weiterer Umsetzung
untersuchen, einschliesslich aller laufzeitrelevanten Aenderungen in
der betrachteten Versionsspanne. A-G-00129 / U-G-000139 gelten weiter;
die Prioritaetsaenderung erweitert keine Produktionsimplementierung.
Persoenliche GUI-Abnahme und vollstaendige fachliche Matrix sind
weiterhin offen, aber keine Sperre fuer eine rein diagnostische Messung.
Nach Vorlage der Befunde wieder stoppen.

Analyse-Reihenfolge:

1. Versionsdiff-Inventar fuer GUI G-0.9/G-0.10 und vorherige Aenderungen
   seit 27.10.2026; Core separat C-0.7/C-0.8/C-0.9.
2. Gleiche eingefrorene Core-Snapshots mit 420/Folkeboot darstellen.
   Renderkosten unabhaengig vom Solver messen; keine Snapshotwerte
   zur Erreichung besserer Zeiten veraendern.
3. Ein/eine plus drei/verborgene Zusatzansichten, einfache Darstellung
   ein/aus, statischen und dynamischen GUI-Pfad getrennt auswerten.
   Tabellen, Ergebnisparser, erneute Profilzuweisung, Kraftmenue und
   gesamte Zustandsdarstellung gesondert messen.
4. Core-Rechenzeit, Geometriekomplexitaet, Modellaufbereitung, statische
   Solversuche und JSON getrennt gemaess Core-Plan auswerten.
5. Alle Befunde in Tabellen mit GUI/Core, Statisch/Dynamisch,
   Versions-/Quellbeleg, Messung/Verdacht, Einbussen-/Einsparpotenzial
   und physikalischem Risiko zusammenfuehren. Restluecken sichtbar
   lassen, keine Untersuchung jeder denkbaren Hardware versprechen.

**Unveraenderlich:** Produktionsquellen, Modelle, Integrator,
Core-Schritte, Toleranzen, dt und erforderliche Physik. Die Auslegung
statischer und dynamischer Kraefte darf durch die Analyse nicht
auseinanderlaufen. Kein Commit, Tag, Push, Release oder Standard-EXE-
Update fuer diesen Auftrag.

Isolierte Messprogramme im Sitzungsordner duerfen bestehende Methoden
aufrufen. Fuer getrennte Methodenzeiten wird nur im isolierten
MainWindow-Header die Zugriffssichtbarkeit fuer den Harness geoeffnet,
keine Berechnungsanweisung geaendert. Das ist kein produktiver Patch.
Abgebrochene Entwicklungs-/Diagnoselaeufe gelten nicht als Messbeleg;
nur vollstaendig beendete Rohmessungen werden uebernommen.

### 13.7 Vollstaendiges GUI-Diff-Inventar und Ursachenbefunde

Versionsumfang: G-0.10 `6f2f0b782fb4d52277c62201ab3f22b7fc0d9780`
gegen G-0.9 `67c2ce7534126525cf0c75d27c72338276f4d356`;
Urspruenge seit `e54497f` vom 27.10.2026 anhand des Dateiverlaufs.
Alle 14 geaenderten produktiven C++-Dateien/Headers einschliesslich der
neuen Filterheaderdatei im GUI-Diff sind unten zugeordnet. Test-, CMake- und
Dokumentationsaenderungen laufen nicht in der produktiven Simulation.
`graphics_settings.cpp`, `water_motion.cpp`, `display_geometry.cpp`
und `core_adapter.hpp` sind zwischen G-0.9/G-0.10 unveraendert;
aeltere Aenderungen werden deshalb als Altbestand markiert.

Potenzial bezeichnet vermutete Einsparbarkeit ohne Funktionsverlust,
nicht bereits erreichte Beschleunigung. Hoch/mittel/niedrig ist eine
Priorisierung, keine Prozentangabe. Zeilen ueberlappen und duerfen
nicht addiert werden. Quellen beziehen sich auf G-0.10.

| Aenderung / Befund | GUI / Core | Statisch / Dynamisch | Quelle / Version / Belegart | Einbussen- bzw. Einsparpotenzial | Physikalisches Risiko / Grenze |
|---|---|---|---|---|---|
| Eine freie Szene wird durch vier vollstaendige Szenen ergaenzt; jede erhaelt `setState` und rendert synchron | GUI | beide; statisch pro Ergebnis/Bedienung, dynamisch fortlaufend | G-0.10; [displayState](../Code/src/main_window.cpp#L2440), [setState](../Code/src/vtk_scene.cpp#L1538); Codebeleg, Messung 13.8 | hoch: wiederholte Geometrieaufbereitung und Renderarbeit | Keine Ansicht/Ergebnisfunktion entfernen; nur redundante Arbeit waere kandidat |
| Ausgeblendete Zusatzansichten werden weiterhin bedient; `requestRender` prueft nur vorhandene Koerper, nicht Sichtbarkeit | GUI | beide | [displayState](../Code/src/main_window.cpp#L2490), [requestRender](../Code/src/vtk_scene.cpp#L2356); Codebeleg, Hidden-Messfall | hoch; Ausblenden alleine ist keine Optimierung | Bei spaeterem Einblenden aktuellen Snapshot korrekt herstellen |
| Unveraendertes Profil der drei gebundenen Ansichten umgeht den Gleichheitsabbruch der freien Ansicht; Styling und Render erneut | GUI | beide | neu G-0.10; [setGraphicsProfile](../Code/src/vtk_scene.cpp#L1830); Codebeleg und Messung | hoch fuer redundante Profilrunden | Feste schematische Profile, Transparenz und ThroughBody muessen identisch bleiben |
| Profilrunde schon am Beginn des 33-ms-Live-Ticks und erneut nach `setState`; auch ohne neuen Snapshot kann sie arbeiten | GUI | dynamisch; Profilwechsel auch statisch | [updateLiveProgress](../Code/src/main_window.cpp#L2303), [displayState](../Code/src/main_window.cpp#L2499), [applyGraphicsProfile](../Code/src/main_window.cpp#L1452); neue Vervielfachung, Timer Altbestand | hoch; Zielintervall ist kein garantierter Frame-Durchsatz | Core-Takt und vollstaendige Zustandshistorie nicht beschneiden |
| Wasserlinienschnitt, Stripper, Trocken-/Flutungsloops, Normalen und lokale Meshtransformation fuer jeden Koerper pro Szene/Snapshot | GUI | beide | Altbestand u.a. G-0.7/0.9, jetzt vierfach; [setState](../Code/src/vtk_scene.cpp#L1384) | hoch bei komplexen Modellen; genaue Teilzeit noch nicht isoliert | Feste Elementstellung ist nicht feste Bootspose; Flutung und Welt-Wasserebene korrekt invalidieren |
| Bewegungsmatrizen im Snapshot fuehren auch bei unveraenderten Elementstellungen zu DeepCopy, Matrixanwendung und neuen Normalen | GUI | beide, dynamisch besonders haeufig | [setState](../Code/src/vtk_scene.cpp#L1385); vor G-0.10 vorhanden, vierfach multipliziert | hoch als Cache-Kandidat; kein Prozentbeleg | Matrix/Modellfassung/Topologie vollstaendig als Cache-Schluessel, nicht nur Element-ID |
| `geometryDirty` erzwingt Umwelt-, Wasser-, Raster-, Kraft- und Kursgeometrie-Neuaufbau | GUI | beide | [setState](../Code/src/vtk_scene.cpp#L1535), [Renderpfad](../Code/src/vtk_scene.cpp#L2860), [rebuildEnvironment](../Code/src/vtk_scene.cpp#L3495); ueberwiegend Altbestand | hoch/mittel, im Gesamt-State-Aufwand enthalten | Kameraabhaengige Ausschnitte und Flutung duerfen nicht veralten |
| Wasserflaeche samt Farbfeldern, Trockenmaske, tiefer Kopie und Raster wird pro Szene aufgebaut; kleine Ansichten nutzen dieselbe Infrastruktur | GUI | beide | [rebuildEnvironment](../Code/src/vtk_scene.cpp#L3505); Altbestand, neue schematische Zweige G-0.10 | mittel/hoch, nicht separat gemessen | Kein Wasser im trockenen Bootsinneren; Wasserlinie und Ebenenprojektion erhalten |
| Fuenf einfache Darstellungen erzeugen Resultierende, Marker, Pfeilgeometrie und Labels | GUI | beide | neu G-0.10; [rebuildSchematicForces](../Code/src/vtk_scene.cpp#L2980), [Kurs](../Code/src/vtk_scene.cpp#L2923); Ein/Aus-Messmatrix | Messung entscheidet; kein pauschales Hauptursachenurteil | Abschalten der Anzeige darf niemals Kraftauswertung im Core aendern |
| Selbst bei allen einfachen Darstellungen AUS wird `staticResultants` vor den `enabled`-Checks berechnet | GUI | beide | [rebuildSchematicForces](../Code/src/vtk_scene.cpp#L3036); Codebeleg | mittel; AUS beseitigt nicht den gesamten Auswahl-/Aggregationsaufwand | Numerische Tabelle und Verfuegbarkeitsdiagnosen muessen vollstaendig bleiben |
| Resultierende und abgeleitete Summen mehrfach: jede Szene, Kraftmenue und neue Tabelle | GUI | beide | [setState](../Code/src/vtk_scene.cpp#L1505), [Menue](../Code/src/main_window.cpp#L1529), [Tabelle](../Code/src/main_window.cpp#L2659), [staticResultants](../Code/src/force_display.cpp#L545); neu G-0.10 | mittel; unveraenderliche gemeinsame Snapshotauswertung waere Kandidat | Modell-/Weltframe und Momentbezug S nicht vermischen |
| Neue 10-Spalten-Tabelle wird formatiert, sortiert und vollstaendig gefiltert, unabhaengig von grafischen Checkboxen | GUI | beide | [updateStaticValues](../Code/src/main_window.cpp#L2643), [filterStaticValues](../Code/src/main_window.cpp#L2631); neu G-0.10 | mittel; messbare eigene Teilzeit | Alle Zahlen, Auswahl, Sortierung, Kopieren und Fehlerstatus erhalten |
| Verborgene alte Werteansicht wird zusaetzlich zur neuen Tabelle gepflegt | GUI | beide | [updateValues](../Code/src/main_window.cpp#L2743); G-0.10 ruft neue Tabelle vor altem Baum auf | mittel, AllValues minus VisibleTable nur grobe Aufwandsdifferenz | Versteckte Darstellung darf nicht weiterhin fachliche Datenquelle sein muessen; derzeit keine Aenderung |
| Kraftmenue setzt jedes Mal Stylesheets, Texte und Verfuegbarkeit auch bei unveraenderten Werten | GUI | beide | [updateStaticForceMenu](../Code/src/main_window.cpp#L1527); neu G-0.10 | niedrig/mittel, separat gemessen | Moduslokale Farben und korrekte Verfuegbarkeit erhalten |
| Kamera-/Navigationswerte werden bei Rendern auch fuer unsichtbare Overlays abgeleitet/formatiert; SVG wird nur bei geaenderten Bytes neu geladen | GUI | beide | [Renderpfad](../Code/src/vtk_scene.cpp#L2905), [Instrument](../Code/src/navigation.cpp#L196); Karte/Docking neu, Gleichheitsabbruch Altbestand | niedrig/mittel; vierfache Ableitung, kein pauschal vierfacher SVG-Reload | Weltkurs, Roll/Stampf und Kameraindikator nicht vertauschen |
| Koerper-/Weltkamera und gebundene Pan/Zoom-Projektionen; freie Ansicht folgt Pose, Weltmodus nur Position | GUI | beide; Auswahl dynamisch | [setState](../Code/src/vtk_scene.cpp#L1480), neue Orthografiemethoden ab Zeile 1921 | niedrig, konstante Vektor-/Matrixarbeit; Renderfolgen separat oben | Keine Physikframe-Aenderung; persoenliche Kameraabnahme offen |
| Live-Snapshot serialisiert Core-Zustand erneut und parst JSON; Historienrueckblick kann Bewegungshistorie neu aufbauen | GUI / Core | dynamisch; statische Ausgabe einmalig | [stateJson](../Code/src/core_adapter.cpp#L654), [displayState](../Code/src/main_window.cpp#L2468); Historie/Live Altbestand, V7 neu | mittel/hoch beim 420; Core-JSON-Arbeit separat im Core-Plan | Unveraenderliche Historie und gleiche Beitraege zwingend; keine gemessenen Frames als Solver-Schritte ausgeben |
| V7-Fremdkraftpruefung und feste Eingaben kommen hinzu; neuer Kraftfall ist nicht identisch zum alten Bug-Testantrieb mit Begrenzer | GUI / Core | dynamisch | [create](../Code/src/core_adapter.cpp#L450), [Parser](../Code/src/force_display.cpp#L235); G-0.10/C-0.9 | Validierung klein/konstant; Physikmehrarbeit im Core-Plan | Korrektheit nicht durch Weglassen von Kontext/Validierung erkaufen; Laufvergleiche nur bei gleichem Kraftfall |
| Modellinspektion wird seit G-0.9 als gemeinsame Quelle benutzt; G-0.10 fragt Rollen ueber Objekteigenschaften ab | GUI / Core | Laden fuer beide | [loadDisplayData](../Code/src/model_catalog.cpp#L179); c97c439/67c2ce7/6f2f0b7 | Rollenabfrage einmalig, nicht pro Zeitschritt; moegliches Ladepotenzial ungemessen | Source-/Darstellungs-/Physikmesh und Einzelquelle erhalten |
| Vier `setModel`-Pfade erzeugen VTK-Actors, Cutter, Normalen/Materialdaten und getrennte Renderfenster | GUI | Laden fuer beide | [modelOpened](../Code/src/main_window.cpp#L1939), [setModel](../Code/src/vtk_scene.cpp#L1201); neu G-0.10 | mittel/hoch fuer Ladezeit/Speicher; kein belegter laufender Core-Mehrimport | Nur immutable Meshdaten teilbar; Scene-/Kameraidentitaet erhalten |
| Tabellenfilterheader, Separatoren, Cursor, Rundungen, Docklayout und SVG-Kartenzuschnitt | GUI | beide / Bootsauswahl | `filter_header.hpp`, `boat_inspection.cpp`, `navigation.cpp`, `main_window.cpp`; G-0.10 | niedrig/ereignisabhaengig; kein belegter Integratorschritt-Aufwand | UX erhalten; nicht als primaere Rechenursache behaupten |
| Modusgetrennte Grafikprofile/Checkboxen werden bei Bedienung gespeichert; Startdialog fuehrt keine stationaere Suche vor Dynamik aus | GUI | beide / Start | [saveDisplaySettings](../Code/src/main_window.cpp#L1330), [startSimulation](../Code/src/main_window.cpp#L2140), `graphics_dialog.cpp`; neu G-0.10 | niedrig bei Bedienung; kein Beleg fuer pro-Schritt-Dateischreiben | Erhalt physikalischer Eingaben und getrennte Darstellung sind Vertragsbestandteil |
| Live-Hydrodynamikkontext, ITTC/Daempfung und spaetere stationaere Fahrt/Kursdrehrate wurden bereits vor G-0.10 eingefuehrt | GUI / Core | jeweils dynamisch bzw. statisch | GUI `59f22d6`, `194dbbb`, `67c2ce7`; Core-Versionen im Gegenplan | notwendige Mehrphysik, nicht automatisch Regression | C-0.7 und heutige stationaere Fahrt sind nicht ohne Umfangskontrolle gleichwertige Benchmarks |

**Abdeckung / Negativbefund:** `graphics_settings.hpp` und
`graphics_dialog.hpp` sind im letzten Versionsdiff nicht geaendert;
`main_window.hpp`, `vtk_scene.hpp`, `navigation.hpp`, `force_display.hpp`,
`model_catalog.hpp` liefern die zugeordneten Daten/Felder, keine
zusaetzliche versteckte Berechnung. Die Auslagerung des Filterheaders
ersetzt vorhandenen Bootsauswahlcode; sie dupliziert keinen Core-Lauf.
Farbgebung, Spaltenlinien, Cursor oder Dialogtexte sind ohne Messbeleg
keine Erklaerung fuer groessere Physik-Rechenzeiten.

#### Physikalische Gleichheit: GUI-Vertrag und Messgrenzen

| Pruefpunkt | Statisch | Dynamisch | Befund / verbleibender Nachweis |
|---|---|---|---|
| Aktivflag, Kraftvektor, Ansatzpunkt, feste Elementstellungen | dieselben rechten Eingabefelder | dieselben rechten Eingabefelder | [startSimulation](../Code/src/main_window.cpp#L2201) baut ein gemeinsames Objekt; nur Schluessel `stationaer`/`dynamische_fremdkraft` und Startpfad wechseln |
| Bezugsrahmen | modellfest | modellfest | `modellfest=true` in derselben Uebergabe; Kameraauswahl ist kein Kraftkoordinatenwechsel |
| Bewegung/Trimm | stationaeres Gleichgewicht und Kurvenfahrt werden gesucht | zeitliche Fortbewegung mit Core-Integrator | Absichtlich andere Aufgabe: gleiche Grundgesetze bedeuten nicht gleiche Laufzeit, Endpose oder Trajektorie |
| dt und Dauer | stationaere Suche, kein Lauf ueber die dynamische Dauer | bestaetigte Dialogwerte, Echtzeittakt optional | Kein Beleg fuer neu verfeinertes Standard-dt: weiterhin 0,01 s; Echtzeitwarten ist keine Solverzeit |
| Hydrodynamik | stationaere Core-Auswertung | bei Rumpfrolle `hydrodynamik_start` mit Defaultkonfiguration | [LiveSimulation::create](../Code/src/core_adapter.cpp#L483); Vollstaendigkeit und Faktoren separat im Core-Plan pruefen |
| Physik waehrend Display-Messung | identische eingefrorene dynamische Snapshots im statischen Workspace | dieselben Snapshots im dynamischen Workspace | Nachweis fuer GUI-Kosten, **kein** Nachweis fuer eine geloeste stationaere Gleichgewichtslage |
| Rueckwirkung der einfachen Darstellung | keine Core-Eingabe | keine Core-Eingabe | Die Checkboxen steuern nur VTK-Darstellung; AUS darf keine physikalischen Beitraege entfernen |
| Fachliche Gesamtparitaet | Referenzen/Residualnachweise teilweise vorhanden | Mindestintegration fuer Fremdkraft/CG/Traegheit vorhanden | Vorhandene Tests sind kein Ersatz fuer die weiter offene dt-/Rudermatrix und Kirchhoff-/Flutungsabnahme |

Besonders wichtig fuer den spaeteren historischen Core-Vergleich:
200-N-Testantrieb mit Geschwindigkeitsbegrenzer, freie Dynamik ohne
Fremdkraft und neue modellfeste Fremdkraft sind drei verschiedene
Rechenfaelle. Nur der gleiche Fall darf eine Regressionszahl liefern.
Die Modell-Schemaumstellung kann Importkosten, Elementzahl und
Geometriekomplexitaet veraendern; eine nicht ladbare Schema3-Datei auf
einer Schema2-Version ist eine Kompatibilitaetsgrenze, kein gemessener
Geschwindigkeitsfaktor. Kein Modell wird fuer die Analyse vereinfacht.

### 13.8 GUI-Messnachweis: G-0.10 und G-0.9

Gueltiger Lauf `gui-perf-final3`: beide isolierten Release-Harnesses
erfolgreich gebaut und vollstaendig beendet, Exit 0. Keine Core-
Benchmarks parallel. MSVC 19.44, Qt6/VTK9.3, Windows, Scale 1,
Ryzen 5 8600G / Radeon 760M. Modellfassungen und SHA256 der exakten
Snapshots: [Manifest](Nachweise/Performance_GUI_2026-11-03/manifest.json).
Rohwerte (jeweils alle fuenf Wiederholungen und Min/Max):
[G-0.10](Nachweise/Performance_GUI_2026-11-03/current.json),
[G-0.9](Nachweise/Performance_GUI_2026-11-03/G-0.9.json).
Eingaben:
[420](Nachweise/Performance_GUI_2026-11-03/snapshots/420.json),
[Folkeboot](Nachweise/Performance_GUI_2026-11-03/snapshots/folkeboot.json).

**Methode:** ein Warmlauf, fuenf Wiederholungen mit je sechs
identischen eingefrorenen Snapshots (0,08 bis 0,48 s des gleichen
0,5-s-Laufs, dt 0,01 s, Testantrieb 200 N mit altem Begrenzer).
Median/Min/Max beziehen sich auf die sechs-Snapshot-Mittel der fuenf
Wiederholungen, nicht auf jedes einzelne Frame. Kein Echtzeitwarten,
keine Physik waehrend der Renderzeitmessung. Alle Werte Millisekunden
je Snapshot, gerundet; Rundung kann Nachkommastellen verdecken.

Das isolierte Szenenfenster nutzt 720x480 plus dreimal 180x160,
gleiche Defaultprofile und versteckte eingebettete Navigation.
Die fuenf einfachen Flags sind gemeinsam EIN gemaess Default bzw.
alle AUS; vollstaendige Kraefte und Hebelarme sind in beiden Faellen
AUS. Nicht verfuegbare Druckpunkte werden nicht kuenstlich erzeugt.
Die Szenenklasse hat keinen statisch/dynamisch unterschiedlichen
Geometriealgorithmus; die erste Tabelle ist deshalb **ein gemeinsamer
Szenenpfad**, keine doppelte Physikmessung.

| Modell | Ansichten | Einfach | setState alle Szenen: Median (Min-Max) | Unveraendertes Profil erneut allen Szenen zuweisen |
|---|---|---|---:|---:|
| 420 | eine | ein | 120,2 (110,6-135,5) | <0,002 |
| 420 | eine | aus | 125,1 (112,0-138,7) | <0,002 |
| 420 | vier | ein | 373,1 (357,0-380,1) | 35,4 |
| 420 | vier | aus | 358,5 (352,9-368,4) | 32,4 |
| 420 | drei Zusatzansichten verborgen | ein | 364,0 (351,6-410,9) | 34,3 |
| 420 | drei Zusatzansichten verborgen | aus | 458,7 (443,7-481,9) | 41,9 |
| Folkeboot | eine | ein | 9,5 (7,4-11,2) | <0,002 |
| Folkeboot | eine | aus | 9,2 (7,5-9,9) | <0,002 |
| Folkeboot | vier | ein | 50,2 (49,1-51,3) | 27,6 |
| Folkeboot | vier | aus | 47,3 (45,0-49,2) | 26,2 |
| Folkeboot | drei Zusatzansichten verborgen | ein | 49,4 (47,8-50,4) | 28,6 |
| Folkeboot | drei Zusatzansichten verborgen | aus | 49,2 (47,3-50,0) | 26,7 |

Die **wirklichen MainWindow-Workspaces** werden zusaetzlich im Fenster
1600x1000 vermessen, jeweils mit derselben eingefrorenen Historie.
`displayState` enthaelt bereits SetState, Profil, Menue und Werte.
Die daneben gemessenen Methoden wurden danach separat aufgerufen:
**nicht zu `displayState` addieren**. AllValues enthaelt VisibleTable.
Kein Core-Worker aktiv; `stateAt` trifft die bereits vorhandenen
JSON-Objekte, nicht die teurere Live-Serialisierung.

| Modell | Workspace | Einfach | displayState: Median (Min-Max) | VisibleTable | AllValues inkl. VisibleTable | Kraftmenue | Weitere Profilrunde |
|---|---|---|---:|---:|---:|---:|---:|
| 420 | statisch | ein | 486,3 (462,0-517,3) | 0,237 | 0,409 | 1,116 | 42,5 |
| 420 | statisch | aus | 430,3 (407,5-477,2) | 0,225 | 0,377 | 1,021 | 38,3 |
| 420 | dynamisch | ein | 466,5 (426,1-489,6) | 0,260 | 0,480 | 1,093 | 38,7 |
| 420 | dynamisch | aus | 439,8 (433,5-471,8) | 0,260 | 0,400 | 1,112 | 39,4 |
| Folkeboot | statisch | ein | 93,9 (91,0-119,4) | 0,192 | 0,320 | 1,041 | 27,9 |
| Folkeboot | statisch | aus | 90,0 (87,7-108,5) | 0,169 | 0,291 | 1,048 | 27,7 |
| Folkeboot | dynamisch | ein | 101,7 (92,6-110,6) | 0,210 | 0,352 | 1,025 | 29,8 |
| Folkeboot | dynamisch | aus | 94,0 (86,3-100,2) | 0,187 | 0,327 | 0,946 | 28,0 |

Parser der fertigen Kraft-JSON-Objekte: 0,12-0,17 ms/Snapshot;
Zugriff auf bereits gecachte MainWindow-JSON-Objekte rund 0,001 ms.
Dies ist **nicht** die Core-Kraftauswertung/JSON-Erzeugung.
SVG-/Qt-Repaints aus der Eventqueue und Frame-Pacing werden hier
nicht vollstaendig erfasst; synchrone VTK-Renderaufrufe dagegen schon.
Statisch bedeutet Workspace mit dynamischem Snapshot: die Messung
enthaelt keine stationaere Solversuche, keine stationaeren Residuen
und keine abweichende stationaere Beitragsliste.

#### Historischer GUI-Komponentenvergleich

G-0.9-Szenenquellen gegen **denselben heutigen Core C-0.9** gebaut.
Die Einzelszene mit allen vergleichbaren Kraeften/Markern/Kurs AUS ist
der gemeinsame Darstellungsumfang. G-0.9 kennt das neue Fuenfermenue
nicht: sein EIN-Fall zeigt alte vollstaendige Kraefte und ist deshalb
kein aequivalenter Einfach-EIN-Vergleich.

| Modell | Gemeinsamer Umfang | G-0.9 setState Median (Min-Max) | G-0.10 setState Median (Min-Max) | Einordnung |
|---|---|---:|---:|---|
| 420 | eine Szene, Darstellung aus | 109,4 (105,5-134,1) | 125,1 (112,0-138,7) | Median +14,3 %, Bereiche ueberlappen; kein Beleg fuer Faktorregression der Einzelszene |
| Folkeboot | eine Szene, Darstellung aus | 8,8 (8,3-9,0) | 9,2 (7,5-9,9) | Median +5,4 %, Bereiche ueberlappen |

Keine historische vollstaendige EXE und kein Vorher/Nachher derselben
Core-Physik wird damit vorgetaeuscht. Der grosse sichere Struktur-
unterschied ist die vierfache Szenenversorgung und deren Profilzweig,
nicht ein nachgewiesener mehrfach langsamerer einzelner Renderer.

#### Auswertung und Prioritaeten

| Befund | GUI / Core | Statisch / Dynamisch | Gemessener Effekt | Potenzial / Urteil |
|---|---|---|---|---|
| Vier statt einer Szene | GUI | beide | Einfach EIN: 120,2 -> 373,1 ms beim 420; 9,5 -> 50,2 ms beim Folkeboot, ohne weitere Profilrunde | hoch; ca. 3,1x / 5,3x in diesem Komponentenfall, keine realisierte Ersparnis |
| Redundante Profilrunde | GUI | beide, dynamisch mehrfach pro Tick | freie Szene <0,002 ms; vier Szenen 26-42 ms, auch bei Einfach AUS | hoch; klarer vermeidbarer Kandidat ohne Physikabbau |
| Zusatzansichten nur verbergen | GUI | beide | Aufwand bleibt aehnlich oder hoeher; kein Sichtbarkeitsabbruch im Code | hoch; Verbergen alleine hilft nicht |
| Einfache Darstellung abschalten | GUI | beide | Vieransicht ohne Profil: 373,1 -> 358,5 bzw. 50,2 -> 47,3 ms; Workspace-Zahlen siehe oben | kleiner als Szenen-/Profilstruktur; kein verlaesslicher Alleinhebel |
| Tabelle plus verborgener Altbaum | GUI | beide | VisibleTable 0,17-0,26 ms, AllValues 0,29-0,48 ms | hier niedrig; sinnvoller Bereinigungskandidat, nicht Hauptursache |
| Kraftmenue | GUI | beide | rund 0,95-1,12 ms | hier niedrig; redundante Stylesheets nicht als hunderte Millisekunden ausgeben |
| Fertiges JSON parsen/kopieren | GUI | beide | 0,12-0,17 ms / ca. 0,001 ms | hier niedrig; Live-Core-JSON gesondert untersuchen |
| Gemeinsame Geometrie/Wasserlinie und Modellkomplexitaet | GUI / Core | beide | 420-Einzelszene trotz AUS 125,1 ms, Folkeboot 9,2 ms; verschiedene Modelle, kein isolierter einzelner Geometriefaktor | hoch plausibel; genaue Kostenverteilung/Cachetreffer noch nicht instrumentiert |

**Streuung nicht verstecken:** Der 420-Hidden-AUS-Fall ist deutlich
langsamer als Hidden-EIN. Auch die einzelne 420-Szene wird durch AUS
nicht schneller. Reihenfolge war fest, nicht randomisiert; thermische,
GPU-/Betriebssystemeffekte oder weitere Zustandskosten sind nicht
isoliert. Daraus weder „AUS macht langsam“ noch garantierte
Prozentsaetze fuer Checkboxen ableiten. Fuer eine spaetere Optimierungs-
abnahme gepaarte/reihenfolgeinvertierte Messung vorsehen. Der im Code
bewiesene fehlende Sichtbarkeits-/Profilabbruch bleibt davon unberuehrt.

**Grenzen:** Kein End-to-End-Liveprofil unter gleichzeitiger Core-
Vollauslastung, kein GPU-Hardwaretrace, keine Zaehlung jedes Cutters/
Cachetreffers und keine historische GUI-Komplettmessung bis 27.10.
Die aelteren GUI-Commits sind per Diff auf Ursachen zugeordnet, nicht
alle binär vermessen. „Alle Aenderungen erfasst“ bedeutet das belegte
Diff-Inventar, nicht dass jede der aufgelisteten Hypothesen bereits
kausal in Millisekunden isoliert ist. Die Physikmatrix bleibt offen.

### 13.9 Gemeinsame GUI-/Core-Befunduebersicht und Haltepunkt

Core-Quellen, Modellinventar, Versionsmatrix und Messmethodik:
[einziger Core-Plan, 4.1-4.3](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.9.md#41-modellinventar-und-vergleichsgrenzen).
Die Tabellen sind keine umgesetzten Optimierungen. Unterschiedliche
Einheiten (pro Snapshot / 50 Schritte / Ladeaufruf) nicht addieren.

| Befund | GUI / Core | Statisch / Dynamisch | Messung / Evidenz | Potenzieller Hebel |
|---|---|---|---|---|
| Vierfache Szenenarbeit und mehrfaches Rendern | GUI | beide | 420 120,2 -> 373,1 ms, Folkeboot 9,5 -> 50,2 ms je Snapshot (einfach EIN) | hoch; Geometrieaufbereitung teilen, nur erforderliche sichtbare Aktualisierungen; Funktionen erhalten |
| Unveraendertes Profil rendert Zusatzansichten erneut | GUI | beide, dynamisch fortlaufend | weitere Profilrunde 26-42 ms; freie Szene <0,002 ms | hoch; korrekter Gleichheitsabbruch/gebuendelte Renderanforderung, keine Core-Aenderung |
| Einfache Anzeige AUS beseitigt Hauptlast nicht | GUI | beide | vier Szenen 420 373,1 -> 358,5 ms; Folkeboot 50,2 -> 47,3 ms; Streuung beachten | geringerer Hebel als Szenen-/Profilstruktur; nicht automatisch Physik auslassen |
| Tabellen, Menue, fertiger JSON-Parser | GUI | beide | Tabelle+Altbaum <0,5 ms, Menue ca. 1 ms, Parser 0,12-0,17 ms | hier niedrig; kein Hauptgrund fuer hunderte Millisekunden |
| Bewegtes physisches Modell wiederholt aufbereitet | Core | beide; zusaetzliche Integratoraufbereitung dynamisch | Archiv-420, free, 50 Schritte C-0.7/0.8/0.9: 87,55/168,24/221,39 ms; Versionsgesamtzeiten, kein isolierter Funktionsanteil | hoch plausibel; exakt stellungs-/modellgebundene Wiederverwendung statt Physik-Rueckbau |
| Hydro/Testantrieb-Pfad mit korrekter bewegter Traegheit | Core | dynamisch | Schema3-420 C-0.8 -> C-0.9 1.221,16 -> 1.553,53 ms je 50 Schritte; Folkeboot 17,92 -> 23,14 ms | hoch plausibel bei wiederholter Modellvorbereitung; keine Fremdkraft-Alleinschuld belegt |
| Geometriekomplexitaet | Core / GUI | beide | aktuelle Quellmeshes 420 94.129 Dreiecke, Folkeboot 486; keine identische Modellaufgabe | hoch fuer exakte Wiederverwendung; keine autorisierte Meshvereinfachung |
| Lade-/Inspektionswiederholungen | Core / GUI | Laden fuer beide | Schema3-Modell laden C-0.9: 420 1.160 ms, Folkeboot 5,17 ms; wiederholte Hash-/Meshpfade Codebefund, Inspektionszeit nicht isoliert | mittel/hoch fuer Start/Bedienung, nicht als Kosten jedes Integratorschritts ausgeben |
| Core-JSON wertet Kraefte erneut aus | Core | beide, dynamisch je abgefragtem Snapshot | Schema3-V6-Einzelzustand C-0.9: 420 7,08 ms, Folkeboot 0,372 ms; ohne Dateischreiben | mittel; fertige passende Kraftauswertung wiederverwenden, Ergebnisvollstaendigkeit erhalten |
| Statische Gleichgewichts-/Kurvenfahrtsuche | Core | statisch | Residuen, Jacobi, Liniensuche und Geometrieschnitte im Code nachgewiesen; keine Solverzeitmessung dieses Audits | potenziell hoch; nicht in dynamischen Zeitschritt kopieren, Abnahmekriterien unveraendert |
| Kein grosser neuer CLI-Gesamtzeit-Sprung auf aktuellen Modellen | Core | dynamisch, free-CLI ohne Hydroanforderung | C-0.8 -> C-0.9: 420 2,930 -> 2,940 s; Folkeboot 0,071 -> 0,073 s, inkl. Laden/51 Ausgaben | Gesamtzeit verdeckt Teilpfade; keine Aussage gegen gemessenen Hydro-Pfad-Anstieg |

**Historische Grenze:** C-0.7 kann die heutigen Schema3-Modelle
nicht laden. Sein direkter Vergleich nutzt unveraenderte archivierte
0.1-Modelle; diese sind Entwuerfe ohne Simulationsfreigabe. Die
Schema3-Zeitreihe C-0.8/C-0.9 ist deshalb separat. Gleiche Endwerte
einzelner Kurzlaufkomponenten sind keine umfassende Physikparitaet.

**Abschluss dieser Untersuchung:** Spezifikationen und die beiden
bestehenden Plaene sind auf die priorisierte Analyse ausgerichtet;
Versions-, Modellstruktur-, Wiederholungs- und Darstellungsbefunde
sind tabellarisch mit Potentialen, Belegen und Grenzen erfasst.
Nicht gemessen bleiben insbesondere statische Solverzeiten, der
Inspektionspfad als eigener Zeitblock, laengere Horizonte, dynamische
Cache-/Aufrufzaehler und Livekonkurrenz. Damit wird weder jeder
Hypothese ein gemessener Kostenanteil zugeschrieben noch vollstaendige
statisch/dynamische Physikgleichheit behauptet.

Keine Produktionsquelle, kein Modell und keine Standard-EXE fuer
diese Analyse geaendert. Keine neuen funktionalen Tests behauptet;
isolierte Release-Harnesses ausgefuehrt und Rohwerte/JSON,
Dokumentlinks sowie Diff-Whitespace geprueft. Dokumentations- und
Nachweisdateien verbleiben uncommittet. **Hier stoppen:** Weitere
Messvertiefung, Optimierung und Git-Veroeffentlichung nur nach Auftrag.
