# GUI-Umsetzungsplan G-0.22: Gruppen- und Segelbewegungsvorschau

**Stand:** 12.11.2026. **Status:** Umsetzung und Regression bestanden;
fachliche Sichtabnahme offen.
Einziger maßgeblicher GUI-Plan für G-0.22.
Maßgeblich ist die [GUI-Spezifikation](../Spezifikationen/Spezifikation_GUI_Bootsauswahl.md#12-finalisiert-am-12112026-bewegungs-und-segelvorschauen),
insbesondere A-G-00151 bis A-G-00156.

## 1. Projektgrenze und Umfang

Dieser Plan verantwortet ausschließlich SegelPhysik-GUI: GUI-Code,
GUI-Tests und GUI-Abnahme im Repository SegelPhysik-GUI. Es werden keine
Dateien, Tests, Spezifikationen, Pläne oder Git-Stände des Core-Projekts
geändert. Es werden keine Core-Änderungen in diesem Plan beauftragt.

Die GUI verwendet ausschließlich Bewegungsdefinitionen, Identitäten,
Gruppenmitgliedschaften, Posen und Spiegelinformationen, die über den
vorhandenen Core-Vertrag bereitgestellt werden. Die GUI berechnet weder
Bewegungsposen noch Segelspiegelungen selbst. Fehlt im vorhandenen Vertrag
eine für die Abnahme erforderliche Gruppen- oder Segelpose, wird der
betroffene GUI-Schritt blockiert und als externe Core-Abhängigkeit
ausgewiesen. Es gibt keinen GUI-seitigen Ersatz, keine Namensheuristik und
keine Änderung der Core-Schnittstelle im Rahmen dieses Plans.

Der Umfang umfasst:

- Bewegungsdefinitionen auf ihrem tatsächlichen Core-Definitionsinhaber
  abfragen und anzeigen: Gruppe oder Element;
- gruppendefinierte Drehungen am Gruppenknoten darstellen und die ganze
  bewegte Gruppe in den Core-Posen zeigen;
- einheitliche Neutral-, vorhandene Minimal- und Maximalvorschauen für alle
  gültigen Bewegungsdefinitionen einschließlich einheitlicher Beschriftung,
  Farbzuordnung und expliziter fehlender Grenzen;
- beide vom Core gelieferten Segelschotvarianten für denselben Ausschlag
  darstellen und die seitenabhängige Bauch-/Spiegeldarstellung beachten;
- unveränderte Quell-, Auswahl-, Kamera- und Simulationszustände.

Nicht enthalten sind Bewegungsregler, persistente Trimm- oder
Montageänderungen, neue Bewegungsphysik, Änderungen an Core-Dateien oder
Core-Tests sowie Git-Commit, Push, Tag oder Release.

Projektweit vor Vergabe geprüft: höchste bisherige Anforderung
A-G-00150 und Umsetzung U-G-000161. Dieser Plan verwendet A-G-00151 bis
A-G-00156 und vergibt U-G-000162 bis U-G-000168.

## 2. Geordnete Umsetzung

| U-ID | Umsetzungsschritt | Anforderungen | Abhängigkeiten | Nachweis/Abnahmekriterium |
|---|---|---|---|---|
| U-G-000162 | Den vorhandenen GUI/Core-Vertrag und seine GUI-Aufrufpfade für Gruppenbewegungen und Segelposen prüfen. Verifizieren, dass stabile Gruppen-IDs, betroffene Elemente, Neutral-/Grenzposen, beide vorzeichenbehafteten Segelvarianten und die für den Bauch erforderlichen Core-Daten abrufbar sind. | A-G-00151 bis A-G-00155 | Keine | Ein GUI-seitiger Vertragstest belegt die benötigten Daten und ihre Herkunft. Fehlt eine Pose oder Gruppenzuordnung, wird der abhängige Schritt als blockiert dokumentiert; keine Core-Änderung und kein GUI-Ersatzalgorithmus. |
| U-G-000163 | Bewegungsdaten am jeweiligen Definitionsinhaber anbinden. Gruppendefinierte Drehungen am Gruppenknoten zeigen und jede Gruppenpose mit allen vom Core betroffenen Elementen darstellen; Elementdefinitionen bleiben am Element. | A-G-00151, A-G-00153 | U-G-000162 bestanden | Test mit Gruppen- und Elementdefinition: Vorschau am korrekten Baumknoten; Gruppenpose enthält alle betroffenen Elemente und nur die Core-bestätigten Transformationen. |
| U-G-000164 | Die Vorschau für alle gültigen Bewegungsarten vereinheitlichen: Neutralstellung, vorhandene Minima/Maxima, Achse soweit definiert, gemeinsame Bezeichnungen und Farben (Neutral Cyan, Minimum Orange, Maximum Violett, Achse Gelb). Fehlende Grenzen explizit anzeigen. | A-G-00152 | U-G-000162 bestanden | Testmatrix über gültige Bewegungsarten bestätigt identische Bedienung und Farbzuordnung; Pose-/Achsdaten stammen aus dem Core; fehlende Grenzen werden nicht geschätzt. |
| U-G-000165 | Die vom Core gelieferten Backbord- und Steuerbordschotvarianten für denselben Segelausschlag in der Vorschau unterscheidbar darstellen, auch wenn die Bewegungsdefinition nur 0–90° angibt. Die bestehende Vorzeichenkonvention bleibt unverändert. | A-G-00154 | U-G-000162, U-G-000164 | GUI zeigt beide Core-Posen für gleiche Ausschlagsbeträge. Tests belegen die Zuordnung positiv = Backbord, negativ = Steuerbord und keine GUI-eigene Drehung. |
| U-G-000166 | Die Core-seitige Bauch-/Spiegeldarstellung je Segelvariante übernehmen und mit Original-Buglage/Zielseite nach Abschnitt 9 der Spezifikation verknüpfen. Unbekannte Original-Buglage sichtbar als nicht bestimmbar behandeln. | A-G-00155 | U-G-000162, U-G-000165 | Für beide Original-Buglagen werden gleiche und abweichende Zielseite geprüft; Spiegelung und Geometrie stimmen mit den Core-Daten überein. Unbekannte Herkunft erzeugt eine Diagnose statt einer geratenen Spiegelung. |
| U-G-000167 | Vorschau-Interaktion und Seiteneffektfreiheit regressionssicherstellen: Ein-/Ausschalten, Auswahl und Wechsel zwischen Gruppen-/Elementvorschauen dürfen nur temporäre Vorschaugeometrie ändern. | A-G-00152, A-G-00153, A-G-00156 | U-G-000163 bis U-G-000166 | Integrationstests belegen unveränderte Quelle, Auswahlidentität, Inspektionskamera und Simulation; beim Ausschalten verschwinden nur die Vorschauüberlagerungen. |
| U-G-000168 | GUI-Abschlussintegration, vollständigen relevanten Build/Testlauf, Spezifikations-Rückverfolgung und Nachweise durchführen. | A-G-00151 bis A-G-00156 | U-G-000167 | Release-Build und relevante CTests bestehen; alle Anforderungen sind mit Tests und Ergebnissen belegt. Offene Core-Vertragslücken oder Nutzer-Sichtabnahme bleiben ausdrücklich offen. |

## 3. Testfälle

| Test | Prüfung | Erwartung |
|---|---|---|
| G22-01 | Gruppen- und Elementbewegungen im selben Modell | Jede Vorschau hängt am Core-Definitionsinhaber; Gruppendrehungen erscheinen am Gruppenknoten, Elementbewegungen am Element. |
| G22-02 | Gruppendrehung in Neutral-, Minimum- und Maximumpose | Alle bewegten Gruppenelemente folgen gemeinsam den vom Core gelieferten Posen; kein Element wird ausgelassen oder als alleinige Bewegung dargestellt. |
| G22-03 | Verschiedene gültige Bewegungsarten und fehlende Grenzen | Einheitliche Labels/Farben; Achse nur gemäß Definition; fehlende Grenzen werden angezeigt, nicht erfunden. |
| G22-04 | Segelausschlag 0–90° | Backbord- und Steuerbordpose desselben Ausschlags sind beide sichtbar und anhand der bestehenden Vorzeichenkonvention korrekt beschriftet. |
| G22-05 | Segelbauch für gleiche und abweichende Original-/Ziel-Buglage | Unveränderte beziehungsweise Core-bestätigte Spiegelpose entspricht Abschnitt 9; unbekannte Originalseite wird diagnostiziert. |
| G22-06 | Vorschau schalten, Pose auswählen, Baumknoten wechseln | Quelle, Auswahlidentität, Inspektionskamera und Simulationszustand bleiben unverändert; Ausblenden entfernt nur temporäre Vorschaugraphik. |
| G22-07 | Modelle ohne Bewegung, ungültige Definitionen und bestehende Elementvorschauen | Keine unbegründete Vorschau; bisheriges Verhalten beweglicher Elemente bleibt erhalten, sofern es dem finalen Vertrag entspricht. |

Die neuen GUI-Tests werden in den vorhandenen Bootinspektions-/
Grafikregressionen ergänzt, insbesondere in `graphics_render_tests.cpp`.
Testfixtures und Modelle werden nicht im Core-Arbeitsbereich geändert.

## 4. Meilensteine und Gates

| Meilenstein | Eintritt | Abnahmekriterium |
|---|---|---|
| G1 – Datenvertrag | U-G-000162 | Der bestehende Core-Vertrag liefert alle für die GUI-Abnahme nötigen Gruppen-, Segel-, Pose- und Spiegelinformationen. Bei Lücken ist G1 blockiert; keine GUI-Implementierung mit Ersatzdaten. |
| G2 – Gruppen- und Standardvorschau | G1; U-G-000163 und U-G-000164 | Definitionsinhaber, gesamte Gruppenpose, Neutral/Min/Max, Achse, Farben und fehlende Grenzen sind getestet. |
| G3 – Segelvarianten | G1; U-G-000165 und U-G-000166 | Beide Seitenvarianten und die seitenabhängige Bauchdarstellung stimmen mit den Core-Daten und Abschnitt 9 überein. |
| G4 – GUI-Abschluss | G2, G3; U-G-000167 und U-G-000168 | Seiteneffektfreiheit, Regression, Release-Build und relevante GUI-Tests bestanden; Anforderungsnachweise vollständig dokumentiert. |

## 5. Validierung

Der native GUI-Build und CTest verwenden die bestehende Konfiguration aus
der [README-Bauanleitung](../README.md#configure-and-build):

```powershell
cmake --build C:\spdeps\guibuild --config Release --parallel 6
ctest --test-dir C:\spdeps\guibuild -C Release -R "^gui_boat_inspection_|^gui_graphics_render_|^gui_core_integration$" --output-on-failure
ctest --test-dir C:\spdeps\guibuild -C Release --output-on-failure
```

Die gezielte Regression umfasst alle konfigurierten
`gui_boat_inspection_*`- und `gui_graphics_render_*`-DPI-Ziele sowie
`gui_core_integration`; anschließend wird die vollständige CTest-Suite
ausgeführt. Es werden keine Abnahmetests im Core-Repository gestartet oder
geändert. Fehlgeschlagene GUI-Builds oder Tests blockieren G4.

## 6. Rückverfolgbarkeit

| Anforderung | Umsetzungsschritte | Abnahmen |
|---|---|---|
| A-G-00151 | U-G-000162, U-G-000163, U-G-000167, U-G-000168 | G22-01, G22-06 |
| A-G-00152 | U-G-000162, U-G-000164, U-G-000167, U-G-000168 | G22-03, G22-06 |
| A-G-00153 | U-G-000162, U-G-000163, U-G-000167, U-G-000168 | G22-02, G22-06 |
| A-G-00154 | U-G-000162, U-G-000165, U-G-000168 | G22-04 |
| A-G-00155 | U-G-000162, U-G-000166, U-G-000168 | G22-05 |
| A-G-00156 | U-G-000167, U-G-000168 | G22-06 |

## 7. Umsetzungsstatus

**U-G-000162 bis U-G-000168 – umgesetzt; technische Abnahmekriterien
bestanden (12.11.2026):** Der Core-Gruppenvertrag ist implementiert. Die GUI
ordnet die Vorschau der Gruppe zu und belässt bestehende Elementvorschauen
an ihrem Definitionsinhaber. Der isolierte Test
`gui_group_movement_preview` besteht einschließlich Seitenposen, fehlender
Modellgrenzen, Achse, Kamera- und Snapshot-Konstanz, unbekannter Buglage
sowie verborgenem Gruppenmitglied. Nicht darstellbare Posen werden als
nicht bestimmbar ausgewiesen und nicht zur Auswahl angeboten.

Der GUI-Release-Build und der Core-Release-Build bestehen. Die vollständige
GUI-CTest-Suite besteht mit 63/63 Tests; die separate vollständige Core-
CTest-Suite besteht mit 29/29 Tests. Zusätzlich bestanden die gezielten
Gruppen-, Performance-, GUI-Inspektions- und 100/150/200-%-Renderregressionen.
Eine frühere fehlgeschlagene Vollserie lief zeitgleich mit der Core-Regression;
der vollständige GUI-Lauf ohne konkurrierende Tests bestand anschließend.

Für Format-0.3-Segelgruppen enthält das aktive Schema kein
Ausschlags-/Grenzfeld. ±90° ist eine Vorschau-Referenz aus dem bisherigen
Core-Stellbereich, keine Modell- oder Simulationsgrenze. Diese Umsetzung
hat keine Modelldaten oder Schema/Fingerprints beschrieben. Rund 50 Sekunden
nach Abschluss der vollständigen GUI-CTest-Suite wurde im parallel genutzten
Modelle-Repository eine uncommittete Änderung an
`420/modells_validation.json` (Aerodynamik-Fingerprint) und neue
Aerodynamik-Lookup-Ausgaben beobachtet. Sie wurden nicht angefasst.
GUI- und Core-Änderungen bleiben uncommittet; Commit, Push, Tag und Release
waren nicht beauftragt. Die technische Umsetzung ist abgeschlossen; eine
etwaige persönliche Sichtabnahme bleibt getrennt.
