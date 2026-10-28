# Umsetzungsplan G-0.7: Dynamische Ruder- und Schwertsteuerung

**Status:** Umsetzung teilweise abgeschlossen; technische und fachliche Abnahmen offen.  
**Stand:** 28.10.2026 · **Zielversion:** G-0.7 (nach vorhandenem Tag G-0.6)

Maßgebliche GUI-Grundlage ist die
[Spezifikation GUI: Dynamische Ruder- und Schwertsteuerung](../Spezifikationen/Spezifikation_GUI_Ruder_und_Schwertsteuerung.md).
Der Core-Bewegungsvertrag steht in Abschnitt 13 der
[Core-Hydrodynamik-Spezifikation](../../CORE/Spezifikationen/Spezifikation_Hydrodynamik.md#13-dynamische-verstellung-von-ruder-und-schwert);
seine Umsetzung und Lieferung wird getrennt im
[Core-Plan C-0.7, Teil B](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.7_Hydrodynamik.md)
geführt.

Dieser Plan ist der einzige Umsetzungsplan für die GUI-Arbeiten des
Umfangs G-0.7. Er betrifft ausschließlich GUI-Code, -Tests und -Dokumentation
im GUI-Repository. Er erteilt keinen Implementierungs-, Commit-, Push-,
Tag- oder Release-Auftrag. Die Planversion folgt auf den vorhandenen Tag
G-0.6; dadurch werden keine Tags oder Releases erstellt.

## 1. Umfang und Rückverfolgbarkeit

Die GUI bietet während einer dynamischen Simulation ein Steuer-Overlay
für die im Modell gültig deklarierten beweglichen Ruder und Schwerter.
Sie übermittelt Eingaben an den Core und stellt nur Core-bestätigte
Stellzustände und Posen als simuliert dar. Modellvalidierung,
Bewegungsgeometrie, Ereigniszeit, Hydrodynamik, Masse und Hydrostatik
bleiben Core-Verantwortung.

| GUI-Anforderung | GUI-Gegenstand |
|---|---|
| A-G-00046 | Bewegliche Elemente aus Core-Vertrag erkennen und passende Steuerelemente anbieten |
| A-G-00047 | Ruderwinkel -90° bis +90° und Schwertstellung 0–100 % bedienen |
| A-G-00048 | Laufende Simulation, Stellereignisse, Pause/Fortsetzen/Reset/Abbruch integrieren |
| A-G-00049 | Core-bestätigte Pose und vollständige sichtbare Baugruppe darstellen |
| A-G-00050 | Fehlerbehandlung und Bestandsverhalten erhalten |
| A-G-00051 | GUI/Core-Integration, Build und Abnahmenachweise |

Die nächsten freien GUI-Umsetzungsschrittkennungen nach U-G-000045 sind
U-G-000046 bis U-G-000051:

| U-ID | Anforderung(en) | Arbeitspaket und Ergebnis | Abhängigkeit |
|---|---|---|---|
| U-G-000046 | A-G-00046, A-G-00051 | Gelieferten Core-Modell-/Bewegungsvertrag und Simulations-Eingabepfad untersuchen; GUI-Adapter, Element-IDs, Core-Lauf-/Ergebnisbindung und Integrationstestgrenzen festlegen | Core C-0.7 (Teil B): U-C-000061–000063 geliefert; vorhandenen GUI/Core-Buildstand feststellen |
| U-G-000047 | A-G-00046, A-G-00047 | Overlay mit dynamischer Erkennung und eindeutigen Reglern pro beweglichem Element implementieren; projektweite Bereiche, Einheiten, Werteanzeigen und Grenzwerte abbilden | U-G-000046 |
| U-G-000048 | A-G-00048 | Regleränderungen über den Core-Eingabepfad senden; bestätigte/ausstehende Werte darstellen; Lauf-, Simulationszeit-, Pause-, Fortsetzen-, Reset- und Abbruchzustände anbinden | U-G-000046, U-G-000047; Core C-0.7 (Teil B): U-C-000063–000065 geliefert |
| U-G-000049 | A-G-00049 | Vollständige Darstellung auf Core-bestätigte Posen aktualisieren; Bewegungsgruppen und Darstellungsmash-Bindung ohne GUI-seitige Physik sicherstellen | U-G-000046, U-G-000048; Core C-0.7 (Teil B): U-C-000062–000064 geliefert |
| U-G-000050 | A-G-00050, A-G-00051 | Ablehnungen, Fehler und veraltete Modellelemente sichtbar behandeln; Bestandsverhalten und bestehende Laufsteuerung regressionsprüfen | U-G-000047–000049 |
| U-G-000051 | A-G-00046 bis A-G-00051 | Core/GUI-Live- und Wiedergabeintegration, vollständige GUI-Tests, Build, Nutzersichtprüfung und Abschlussmatrix dokumentieren | U-G-000050; alle benötigten Core-C-0.7-Lieferungen |

Die GUI-Planung beginnt unabhängig, die eigentliche Integration der
Regler- und Simulationsereignisse aber erst mit einer benannten und
prüfbaren Core-Lieferung. Nicht verfügbare Core-Funktionen werden nicht
durch GUI-Geometrie, lokale Wandzeitstempel oder lokale Kraftberechnung
ersetzt.

## 2. Voraussetzungen und Grenzen

- Vor Implementierungsbeginn den vorhandenen GUI-Arbeitsbaum, Branch,
  CMake-Konfiguration, Testbaseline und die tatsächlich eingebundene
  Core-Revision dokumentieren. Im Arbeitsbaum existieren bereits
  uncommittete Änderungen; sie sind fremd beziehungsweise außerhalb
  dieses Planumfangs und dürfen weder überschrieben noch verworfen oder
  pauschal gestagt werden.
- Core C-0.7 (Teil B) muss mindestens den validierten Bewegungsvertrag,
  stabile Modell-/Element-IDs, den Stellereingabepfad, bestätigte
  Stellwerte/Pose, zeitgeordnete Laufereignisse und für Rendering
  erforderliche Posen bereitstellen. Das genaue API- und Datenformat
  muss vor GUI-Implementierung gegen die tatsächlich integrierte
  Core-Lieferung geprüft werden.
- Die GUI nutzt den Core-Modellkatalog beziehungsweise eine dafür
  vorgesehene Core-Schnittstelle. Sie liest keine Modell-JSONs und
  hydrodynamischen Simulationsmeshes selbst und leitet keine
  Beweglichkeit aus Namen, Meshgeometrie oder Gruppenheuristiken ab.
- Regler erscheinen ausschließlich für gültige Bewegungsdefinitionen.
  Ruder: -90° bis +90°, negativ Backbord und positiv Steuerbord.
  Bewegliches Schwert: 0 % bis 100 %. Projektweite Endstellungen
  und Null-/Vollausfahrbedeutungen werden nicht GUI-seitig aus der
  Geometrie neu bestimmt. Ein Hub-Kiel wird nicht angeboten.
- Ein Sliderwert allein beweist keine angenommene Core-Eingabe. Die
  GUI muss angeforderten, bestätigten und dargestellten Zustand
  unterscheiden, bis die Core-Antwort die Übernahme bestätigt.
- Eingaben richten sich nach Simulationszeit und Core-vergebener
  Ereignisreihenfolge. Wandzeitstempel, eigener GUI-Tie-break und
  Stell-/Aktuatordynamik sind unzulässig.
- Darstellung basiert auf der Core-bestätigten Bauteilpose. Die GUI
  rendert vollständige   Darstellungsmeshes und bewegt alle
  zugeordneten sichtbaren Baugruppenteile gemeinsam; sie verwendet
  kein reduziertes hydrodynamisches Simulationsmesh als Ersatz.
- Die GUI ändert keine Masse, keinen Schwerpunkt, keine Trägheit,
  keine Hydrostatik, keinen Auftrieb und keinen Flutungszustand.
  Sie berechnet auch keine Hydrodynamik oder Kollision mit Rigg,
  Großbaum und Segeln.
- G-0.7 ist ein eigenständiger GUI-Lieferumfang nach dem vorhandenen
  Tag G-0.6. Es werden keine historischen GUI-Pläne umgeschrieben.
  Core-Tests und GUI-Tests sowie deren Abnahmen sind getrennt
  auszuweisen.

## 3. Abhängigkeiten und Integrationsreihenfolge

| Stufe | Ergebnis | Eintritts-/Abnahmekriterium |
|---|---|---|
| A. Core/API-Übergabe | Lieferumfang und Buildbindung bekannt | Core-Modellkatalog, Event-Eingabe, Zustandsbestätigung und Posevertrag mit konkreter API-Version/Fassung dokumentiert; GUI baut dagegen |
| B. Steuerelemente | Overlay und Werteingabe | Ausschließlich gültige Core-Bewegungen erhalten passende Regler; Endwerte und Anzeigewerte stimmen |
| C. Live-Steuerung | Ereignisse an laufende Simulation | Core-Bestätigung und Simulationszeit sind sichtbar; Pause, Fortsetzen, Reset und Abbruch haben definierte Zustände |
| D. Pose/Rendering | vollständige sichtbare Baugruppen | Rendering folgt ausschließlich bestätigten Core-Posen; keine GUI-seitige Bewegungsphysik |
| E. Fehler/Regression | Fehler und Altmodelle | Ablehnungen explizit angezeigt; unkonfigurierte Modelle und vorhandene Laufsteuerung unverändert |
| F. Abschluss | Integration und Abnahmenachweis | GUI-Build, gezielte Tests, Integrations- und Sichtprüfung sowie A-/U-Matrix abgeschlossen |

**Gesperrte Abhängigkeit:** U-G-000047 bis U-G-000049 dürfen nicht als
integriert oder abgeschlossen markiert werden, solange die dafür
benötigten C-0.7-APIs und Zustandsverträge nicht konkret geliefert und
mit der GUI-Buildkonfiguration geprüft sind. Bei Änderungen des
Core-Vertrags bleibt die Anpassung in der Core-Spezifikation und
Core-Lieferung nachvollziehbar.

## 4. Meilensteine

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| M1 – Core-Übergabe | Core-Vertrag in U-G-000046 verfügbar | Modellmetadaten, Stellereignis, bestätigter Zustand/Pose und verknüpfte Core-Revision dokumentiert; GUI baut gegen diese Schnittstelle |
| M2 – Overlay | M1 bestanden | dynamisch erzeugte Steuerelemente, Identität, Wertebereiche, Einheiten und Grenzfälle getestet |
| M3 – Live-Eingaben | M2 sowie Core-Ereignislieferung bestanden | bestätigte/ausstehende Eingaben, Zeitschrittwirkung, Pause/Fortsetzen/Reset/Abbruch und keine Übertragung auf neuen Lauf geprüft |
| M4 – Darstellung | Core-Posevertrag verfügbar | vollständige bewegte Ruderbaugruppen stimmen in End- und Zwischenstellungen mit Core-Pose überein |
| M5 – Fehler/Regression | M3 und M4 bestanden | Fehlerdiagnosen sichtbar; Modelle ohne Bewegung sowie bestehende Simulationsbedienung regressionsfrei |
| M6 – G-0.7-Abschluss | M1–M5 bestanden | Release-GUI-Build und CTest, Integrationsmatrix, Sicht-/Bedienstatus und offene Punkte dokumentiert |

## 5. Prüfstrategie

1. **Modellkatalog:** gültige Ruder-/Schwertdefinitionen, Hub- und
   Drehschwert sowie Modell ohne Bewegung; ungültige und fehlende
   Bewegungsdaten; mehrfach vorhandene Elemente mit stabilen IDs.
2. **Bedienung:** beide Ruderwinkelgrenzen und 0°, Schwert 0 %, 100 %
   und Zwischenwert; sichtbare Werte, Einheiten und Backbord-/
   Steuerbordvorzeichen.
3. **Live-Ereignisse:** mehrere Eingaben, Core-Ablehnung, verzögerte
   Bestätigung, Pause/Fortsetzen, Reset, Stop und Abbruch; keine
   Wandzeitsteuerung, doppelten Ereignisse oder unbestätigt dargestellten
   Simulationsergebnisse.
4. **Darstellung:** Core-bestätigte Pose an End- und Zwischenstellungen;
   vollständige Baugruppenbewegung, keine Skalierung, kein Ersatz durch
   Simulationsmesh, Wiedergabe derselben Ereignisfolge.
5. **Regression:** Modelle ohne Bewegungsblock, Simulationsergebnisse,
   Lauf-/Pause-/Stopp-Bedienung sowie Anzeige hydrostatischer und
   massenbezogener Daten bleiben unverändert.
6. **Integration:** vorhandener GUI-Build und vollständige CTest-Suite
   einschließlich gezielter neuer Core-Integrationsfälle. GUI-
   Bedienbarkeit und Sichtprüfung werden getrennt von automatisierten
   Tests protokolliert.

Projektvorgabe für Windows-Build und CTest ist die
[GUI-README](../README.md#configure-and-build). Die verwendete
Core-Checkout-Fassung ist für Build und Test ausdrücklich zu notieren;
eine erfolgreiche GUI-Kompilierung gegen einen anderen oder alten
Core-Stand ist kein Integrationsnachweis.

## 6. Rückverfolgbarkeit und tatsächlicher Stand

| A-ID | GUI-Arbeitspaket(e) | Nachweis | Status |
|---|---|---|---|
| A-G-00046 | U-G-000046, U-G-000047, U-G-000050, U-G-000051 | Modellkatalog-/Overlaycode vorhanden; gezielte Live-Core-Ereignistests bestanden, Overlayabnahme offen | teilweise |
| A-G-00047 | U-G-000047, U-G-000051 | Reglerbereiche und Werteanzeige implementiert; UI-Bedien- und Grenzwertabnahme offen | teilweise |
| A-G-00048 | U-G-000048, U-G-000050, U-G-000051 | Laufender Core-Eingabepfad und bestätigte Werte implementiert; Pause/Reset/Abbruch-Integration offen | teilweise |
| A-G-00049 | U-G-000049, U-G-000051 | Core-Matrizen auf vollständige Darstellungsmeshes angewandt; gezielter Bewegungstest bestanden, vollständige Sichtabnahme offen | teilweise |
| A-G-00050 | U-G-000050, U-G-000051 | Eingabefehler werden zurückgewiesen und angezeigt; Altmodell- und vollständige Laufsteuerungsregression offen | teilweise |
| A-G-00051 | U-G-000046, U-G-000051 | Release-Build und integrierte GUI/Core-CTest-Suite bestanden (29/29); Nutzer-Sichtprüfung und fachliche Abnahme offen | teilweise |

| U-ID | Tatsächlicher Stand | Nachweis / offener Punkt |
|---|---|---|
| U-G-000046 | teilweise umgesetzt | Core-Modellmetadaten, Ereignisse und Ergebnisvertrag V4 angebunden; GUI-Release-Build und integrierter CTest-Lauf 29/29 bestanden |
| U-G-000047 | teilweise umgesetzt | Dynamische Ruder-/Schwertregler, Bereiche und angefordert/bestätigt-Anzeige implementiert; MainWindow-Integrationstest prüft 420er-Regler und Wertebereiche; UI-Sichtprüfung ausstehend |
| U-G-000048 | teilweise umgesetzt | Live-Regler senden Ruder- und Schwertwerte an Core; GUI-Test prüft Core-Bestätigung 20°/50 %; Adapter- und vollständige Integrationstests bestanden |
| U-G-000049 | teilweise umgesetzt | Core-Elementmatrizen auf vollständige Darstellungsmeshes angewandt; Renderingtests bei 100/150/200 % und integrierte CTest-Suite bestanden; fachliche Sichtabnahme offen |
| U-G-000050 | teilweise umgesetzt | Ungültige Stellwerte werden explizit abgelehnt; weitere Fehler-/Regressionstests offen |
| U-G-000051 | teilweise umgesetzt | GUI/Core-Release-Build und vollständige CTest-Suite bestanden: 29/29; Bewegungs-Overlay wird im dynamischen MainWindow-Test einschließlich Core-bestätigter Eingaben geprüft. Nutzer-Sichtprüfung, Pivot-/Modellabnahme und fachliche Integration bleiben offen |

Der Planstatus wird anhand der tatsächlich abgeschlossenen Arbeit und
Belege fortgeschrieben. Nutzer-Sichtabnahme, automatisierte Tests,
Core-Abnahme und technische Integration sind getrennte Status. Core-Arbeiten
sind Bestandteil des beauftragten Gesamtumfangs. Commit, Push, Tag und
Release sind nicht beauftragt.

# Teil B: Darstellung des Lateraldruckpunkts

Der Versionsschritt G-0.7 umfasst zusätzlich die Darstellung des
Lateraldruckpunkts. Fachliche Grundlage ist die
[Spezifikation GUI Lateraldruckpunkt](../Spezifikationen/Spezifikation_GUI_Lateraldruckpunkt.md)
(A-G-00052 bis A-G-00056); der Core-Vertrag steht in der
[Core-Spezifikation Lateraldruckpunkt](../../CORE/Spezifikationen/Spezifikation_Lateraldruckpunkt.md)
und wird im Core-Plan C-0.7, Teil D, geliefert. Dieser Teil ist kein
eigener Plan, sondern Teil des einen Plans für G-0.7. Der Segeldruckpunkt
ist ausgeschlossen.

## B.1 Umsetzungsschritte

| U-ID | Anforderung(en) | Arbeitspaket und Ergebnis | Abhängigkeit |
|---|---|---|---|
| U-G-000052 | A-G-00052, A-G-00054, A-G-00056 | Adapter liest `druckpunkte.lateral` (Status, Position, Querkraft); fehlendes Feld und `nicht_bestimmbar` ergeben keinen Wert; Adaptertests | Core C-0.7: U-C-000076 geliefert |
| U-G-000053 | A-G-00053, A-G-00054, A-G-00055 | Marker (senkrechter Strich, Farbe, Beschriftung „LDP“) in der Szene nach Core-Pose; Schaltfläche im Kraft-Overlay, Standard an; Ausblenden bei fehlendem Wert | U-G-000052 |
| U-G-000054 | A-G-00052 bis A-G-00056 | GUI-Tests (Marker, Status, Schaltfläche, Live, Wiedergabe), Regression, integrierter GUI/Core-Lauf, Nutzer-Sichtprüfung, Rückverfolgbarkeit | U-G-000053; Core C-0.7: U-C-000077 geliefert |

## B.2 Voraussetzungen und Grenzen

- Die GUI zeigt nur den Core-Wert und berechnet keine Physik.
- Prozentdarstellung und Zahlenanzeige sind nicht Teil dieses Umfangs.
- Fremde, uncommittete Änderungen werden nicht überschrieben.

## B.3 Rückverfolgbarkeit und tatsächlicher Stand

| A-ID | Arbeitspaket(e) | Nachweis | Status |
|---|---|---|---|
| A-G-00052 | U-G-000052, U-G-000054 | Adaptertests | offen |
| A-G-00053 | U-G-000053, U-G-000054 | Darstellungstest, Sichtprüfung | offen |
| A-G-00054 | U-G-000052, U-G-000053, U-G-000054 | Statustests | offen |
| A-G-00055 | U-G-000053, U-G-000054 | Schaltflächentest | offen |
| A-G-00056 | U-G-000052 bis U-G-000054 | Gesamttests, integrierter Lauf | offen |

| U-ID | Tatsächlicher Stand | Nachweis / offener Punkt |
|---|---|---|
| U-G-000052 | umgesetzt | `parseForceState` liest `druckpunkte.lateral` (`ForceState::lateral`); Test `testLateralCenter` |
| U-G-000053 | umgesetzt (Sichtprüfung offen) | Marker (senkrechte Linie, „LDP“) in `vtk_scene.cpp`, Schaltfläche „Lateraldruckpunkt (LDP)“, Standard an; magenta = kraftgewichtet, gedämpft = geometrisch |
| U-G-000054 | teilweise | Adaptertest ok; Testfixtures `graphics_render_tests` für Folkeboot-Bewegung angepasst; 3 Render-Tests schlagen aus nicht LDP-bedingtem Grund fehl |

