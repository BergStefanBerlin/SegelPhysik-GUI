# Umsetzungsplan G-0.2: Kraft- und Momentendarstellung und Modellkatalog-Anbindung

**Status: umgesetzt, Abnahme offen** (ctest 17/17 grün; DPI-Prüfung, Laufzeitgrenzen und Nutzerabnahme stehen aus).

**Stand:** 26.10.2026 · **Version:** G-0.2

**Maßgebliche Spezifikationen:**

- [Kraft- und Momentendarstellung](../Spezifikationen/Spezifikation_GUI_Kraft_und_Momentendarstellung.md) (A-G-00009 bis A-G-00018, Nachträge A-G-00023, A-G-00024, A-G-00026)
- [Modellkatalog-Anbindung](../Spezifikationen/Spezifikation_GUI_Modellkatalog_Anbindung.md) (A-G-00019 bis A-G-00022)
- Nachtrag [GUI-Erstausbau](../Spezifikationen/Spezifikation_GUI_Erstausbau.md) (A-G-00025)

**Entscheidungen:** [GUI_Kraft_und_Momentendarstellung.json](../Spezifikationen/Entwurfspunkte/GUI_Kraft_und_Momentendarstellung.json),
[GUI_Modellkatalog_Anbindung.json](../Spezifikationen/Entwurfspunkte/GUI_Modellkatalog_Anbindung.json)

Dieser Plan ist der einzige maßgebliche Plan für den Versionsschritt G-0.2.
Er enthält die Schritte U-G-000011 bis U-G-000023 (U-G-000021 bis
U-G-000023 als Nachträge vom 27.10.2026). Der Planauftrag ist keine
Freigabe für Commit, Push, Tag oder Release.

## 1. Zielbild und Umsetzungsgrenze

Die einfache Kraftanzeige aus G-0.1 wird durch eine skalierte Darstellung
von Kraftpfeilen und Momentenbögen mit Wertefenster ersetzt, und die GUI
bezieht Modellkatalog und Darstellungsgeometrie über den Core. Die GUI
rechnet keine Physik und liest das Modellformat nicht mehr selbst.

## 2. Voraussetzungen

1. Die Spezifikationen sind finalisiert (erfüllt am 26.10.2026).
2. **Externe Voraussetzungen aus dem Core** (eigenständig im Core-Projekt
   geplant, Versionsschritt C-0.2):
   - Ergebnisvertrag Version 2 (A-C-00034 bis A-C-00041, Core-Meilenstein M2)
     für U-G-000017.
   - Modellkatalog-Schnittstelle (A-C-00042 bis A-C-00046, Core-Meilenstein
     M3) für U-G-000018 und U-G-000019.
   Ohne diese Lieferungen sind die betroffenen Schritte **blockiert**; die
   V1-Schritte U-G-000011 bis U-G-000016 sind davon unabhängig.
3. Build und Tests wie in G-0.1 (`C:\spdeps\guibuild`, ctest mit
   `QT_QPA_PLATFORM=offscreen`).

## 3. Umsetzungsschritte

| ID | Arbeit | Anforderungen | Abhängig von | Version | Status |
|---|---|---|---|---|---|
| U-G-000011 | Skalierungsbaustein: linear/log, Referenz R je Größenart, Einfrieren im Lauf, „Neu einpassen“, Kleinwert-/Überskala-Markierung; reine Logik mit Unit-Tests. | A-G-00011 | – | G-0.2 | umgesetzt, getestet (`gui_force_display`) |
| U-G-000012 | Kraftpfeile neu: Angriffspunkt mit Pose, feste Bildschirmgrößen (Kopf 10 px, Strich 3 px, max. 180 px) DPI-unabhängig, Farben/Linienarten mit Legende, Actor-Wiederverwendung; ersetzt `setForceVisibility`-Längenlogik. | A-G-00009, A-G-00012, A-G-00013 | U-G-000011 | G-0.2 | umgesetzt; DPI-Prüfung offen |
| U-G-000013 | Momentenbögen: 240°-Bogen um Bezug, Rechte-Hand-Regel, Radius max. 48 px, stabile Bogenbasis. | A-G-00010, A-G-00012 | U-G-000011 | G-0.2 | umgesetzt; DPI-Prüfung offen |
| U-G-000014 | Sichtbarkeit und Auswahl: Modus „räumlich“/„durch Körper sichtbar“, Gruppen/Summen ein-/ausblenden, Picking mit 6-px-Trefferzone und Mehrfachtreffer, Hervorhebung, Hebelarm-Hilfslinie. | A-G-00014, A-G-00015 | U-G-000012, U-G-000013 | G-0.2 | umgesetzt; manuelle Picking-Prüfung offen |
| U-G-000015 | Wertefenster: QDockWidget mit QTreeView (Bilanzkarten, Hierarchie, Detail), 4 signifikante Stellen, Zustände als Text, Synchronisation mit 3D, Kopieren in die Zwischenablage; mit V1-Daten. | A-G-00016, A-G-00017 | U-G-000014 | G-0.2 | umgesetzt (QTreeWidget statt QTreeView, siehe 8.) |
| U-G-000016 | Einstellungen und Aktualisierung: QSettings für Skala/Sichtbarkeit/Layout, gebündelte Aktualisierung mit Live-Bildrate, Rückblick zeigt Pfeile und Werte des gewählten Zustands. | A-G-00018 | U-G-000015 | G-0.2 | umgesetzt; Laufzeitgrenzen offen |
| U-G-000017 | Ergebnisvertrag V2 anbinden: Version 1 und 2 lesen; Quelle/Gruppe für Farben, Datenzustände, Bilanzstatus und Gleichgewichtsangaben in Wertefenster und Bilanzkarten. | A-G-00013, A-G-00016, A-G-00017 | U-G-000015; extern Core M2 | G-0.2 | umgesetzt, getestet (`gui_core_integration`) |
| U-G-000018 | Katalog über Core: `model_catalog` auf `modellkatalog(ordner)` umstellen, Ordnerwahl/-überwachung bleiben, fehlerhafte Modelle ausgegraut mit Warnsymbol und Diagnose-Tooltip, nicht startbar; eigenes Lesen von `modell.json` entfernen. | A-G-00019, A-G-00020 | extern Core M3 | G-0.2 | umgesetzt, getestet (`gui_model_catalog`) |
| U-G-000019 | Geometrie über Core: `obj_mesh` durch `darstellungsgeometrie` ersetzen, Abruf erst bei Auswahl, Cache nach Modell-ID und Fassung; Reaktion auf veraltete Fassung (Katalog neu laden, Hinweis). | A-G-00021, A-G-00022 | U-G-000018 | G-0.2 | umgesetzt, getestet (`gui_display_mesh`, `gui_model_catalog`) |
| U-G-000020 | Integration und Abnahme: ctest, DPI-Prüfung 100/150/200 %, Laufzeitvergleich Live-Dynamik Anzeige ein/aus, Sichtabnahme durch die Nutzerin; Nachweise dokumentieren. | A-G-00009 bis A-G-00022 | U-G-000016, U-G-000017, U-G-000019 | G-0.2 | teilweise: ctest und Smoke-Test erledigt; DPI, Laufzeitvergleich, Nutzerabnahme offen |
| U-G-000021 | Nachtrag Modellfehler: Diagnose aus `modelldiagnose` und Fehlerkatalog im Wertefenster (Bilanzkarte, Fehlerzeilen, Detail, Kopieren). | A-G-00023 | U-G-000015, U-G-000018 | G-0.2 | umgesetzt, getestet (`gui_model_catalog`, Sichtprüfung) |
| U-G-000022 | Nachtrag Startzustand und Flutung: Zustand beim Moduswechsel an den Core übergeben (`runSimulation`/`LiveSimulation` mit `start`), Zeile „Vollgelaufen“ in der Bilanzkarte. | A-G-00024, A-G-00025 | U-G-000017; extern Core U-C-000031 | G-0.2 | umgesetzt, getestet (`gui_core_integration`, Sichtprüfung) |
| U-G-000023 | Nachtrag Vektorauswahl: zwei Rubriken Kräfte/Momente mit Gewicht, Auftrieb (gesamt/Teile) und Summe; `derivedContributions` bildet Gesamtwerte und Teilmomente aus Core-Beiträgen; Einstellungen gespeichert. | A-G-00026 | U-G-000014, U-G-000016 | G-0.2 | umgesetzt, getestet (`gui_force_display` `testDerived`, Sichtprüfung) |

## 4. Technische Leitlinien

- **Keine Physik in der GUI:** Momente, Summen und Zustände kommen aus dem
  Core. Die GUI berechnet nur Darstellungsgeometrie (Pfeil, Bogen,
  Hebelarm, Skalierung).
- **Bildschirmkonstante Größen:** Pfeile und Bögen werden pro Bild aus
  Kameraabstand und DPI auf Pixelgröße umgerechnet; Actors werden
  wiederverwendet, nicht pro Bild neu erzeugt.
- **Threading:** Wie in G-0.1 – Core-Aufrufe (auch Katalog und Geometrie)
  außerhalb des GUI-Threads; die Anzeige bremst den Core nicht.
- **Kompatibilität:** Bis zur Lieferung des Core C-0.2 arbeitet die GUI mit
  Vertragsversion 1; V2-Felder werden optional ausgewertet.
- **Bestehende Tests** (14 ctest-Fälle G-0.1) bleiben grün;
  `gui_model_catalog` wird in U-G-000018 auf die Core-Schnittstelle angepasst.

## 5. Meilensteine

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| A – Darstellung V1 | Spezifikationen final | U-G-000011 bis U-G-000016 umgesetzt, ctest grün, Sichtprüfung mit V1-Daten |
| B – Vertrag V2 | Meilenstein A, Core M2 geliefert | U-G-000017 umgesetzt, Datenzustände und Bilanzstatus sichtbar |
| C – Katalog über Core | Core M3 geliefert | U-G-000018, U-G-000019 umgesetzt; keine eigenen Dateilesezugriffe mehr |
| D – Abschluss G-0.2 | B, C | U-G-000020, Nutzerabnahme |

A kann sofort beginnen; B und C hängen vom Core ab und sind voneinander
unabhängig.

## 6. Rückverfolgbarkeit

| Anforderung | Umsetzungsschritte | Nachweis | Status |
|---|---|---|---|
| A-G-00009 | U-G-000012 | Geometrietest Pfeil, Sichtprüfung | umgesetzt; Sichtprüfung offen |
| A-G-00010 | U-G-000013 | Bogentest | umgesetzt; Sichtprüfung offen |
| A-G-00011 | U-G-000011 | Unit-Tests Skalierung | erfüllt (Unit-Tests) |
| A-G-00012 | U-G-000012, U-G-000013, U-G-000020 | DPI-Prüfung | umgesetzt; DPI-Prüfung offen |
| A-G-00013 | U-G-000012, U-G-000017 | Zuordnungstest, Sichtprüfung | umgesetzt (Zuordnungstest); Sichtprüfung offen |
| A-G-00014 | U-G-000014 | Picking-Test | umgesetzt; manuelle Prüfung offen |
| A-G-00015 | U-G-000014 | Hebelarm-Test | umgesetzt; manuelle Prüfung offen |
| A-G-00016 | U-G-000015, U-G-000017 | Synchronisationstest | umgesetzt; Synchronisation manuell offen |
| A-G-00017 | U-G-000015, U-G-000017 | Zustands-/Formattest | umgesetzt (Kopieren geprüft) |
| A-G-00018 | U-G-000016, U-G-000020 | Laufzeitvergleich, Rückblicktest | umgesetzt; Laufzeitgrenzen offen |
| A-G-00019 | U-G-000018 | Ordnerwechseltest | umgesetzt (Ordnerwechsel manuell offen) |
| A-G-00020 | U-G-000018 | Katalogtest | erfüllt (Katalogtest) |
| A-G-00021 | U-G-000019 | Geometrievergleich | erfüllt (Geometrietest) |
| A-G-00022 | U-G-000019 | Test geänderte Modelldatei | erfüllt (Test geänderte Datei) |
| A-G-00023 | U-G-000021 | Katalogtest, Sichtprüfung Fehlermodell | umgesetzt; Nutzerabnahme offen |
| A-G-00024 | U-G-000022 | Sichtprüfung Optimist-Dynamik | umgesetzt; Nutzerabnahme offen |
| A-G-00025 | U-G-000022 | `testStartState`, Sichtprüfung Statik → Dynamik | umgesetzt; Nutzerabnahme offen |
| A-G-00026 | U-G-000023 | `testDerived`, Sichtprüfung Optimist-Statik | umgesetzt; Nutzerabnahme offen |

## 7. Offene Punkte

- Laufzeitgrenzen für A-G-00018 legt die Nutzerin nach der ersten Messung
  fest (IDEE-G-00014).
- Wirkungslinie zurückgestellt (IDEE-G-00011).
- Aus G-0.1 weiter offen: Lizenzprüfung Qt/VTK (IDEE-G-00006),
  Fehler-/Abbruch-Referenzfall für A-G-00006.

## 8. Tatsächliche Ergebnisse

Stand 26.10.2026, nicht committet.

**Code:** neu `force_display`, `model_catalog` (Core-Katalog),
`display_geometry` (Core-Darstellungsgeometrie), `vtk_scene` (Pfeile,
Bögen, Overlay-Renderer, Picking, Hebelarm), `main_window` (asynchroner
Katalog, Wertefenster-Dock, Darstellungsoptionen, QSettings);
`obj_mesh` und das eigene Lesen von `modell.json` entfernt.

**Tests:** `ctest -C Release` in `C:\spdeps\guibuild`: 17/17 grün
(13 Core, `gui_core_integration`, `gui_model_catalog`, `gui_display_mesh`,
`gui_force_display`).

**Smoke-Test (Quader, Referenzmodelle):**

- Katalog: 11 Modelle, 8 verwendbar; 420er, Folkeboot, Optimist grau mit ⚠.
- Statik: Gewicht/Auftrieb je 74560 N, ΣF 1,455e-11 N, Bilanzkarte V2
  vollständig, konvergiert; R_F 74560 N, R_M 129100 N·m (Ersatzwert).
- Live-Dynamik in Echtzeit, Pfeile an: 10 s simuliert, 1001 Zustände,
  Core-Rechenzeit 13,56 ms (0,1 % der Echtzeit); Wiedergabe-Slider und
  Werte folgen live.
- „Werte kopieren“ liefert TSV mit Bilanzkarte und allen Beiträgen.

**Abweichungen:**

- Zahlenformat auf Nutzerwunsch (26.10.2026) geändert: N und N·m auf 1,
  m auf 0,01, Prozent ganzzahlig, keine Exponentenschreibweise, jede
  Komponente mit Einheit (A-G-00017 angepasst; Tests in `gui_force_display`).
- Wertefenster nutzt `QTreeWidget` (Item-basiert, an Ort und Stelle
  aktualisiert) statt reinem `QTreeView`-Modell.
- R_M: erster gültiger Zustand max |M|; ist dieser ≈ 0, Ersatzwert
  R_F × halbe Hüllquader-Diagonale (mind. 1 N·m).
- Das Modellformat liefert keine Farben (`farbe_rgb` immer null);
  Standardfarben der GUI.

**Offen:** DPI-Prüfung 100/150/200 %, Laufzeitvergleich Anzeige ein/aus
mit Grenzwerten (IDEE-G-00014), manuelle Prüfung Picking/Auswahl-
Synchronisation (UI-Automation erreicht die Qt-Baumauswahl nicht),
Ordnerwechsel, Sicht- und Nutzerabnahme.

**Nachträge 27.10.2026 (U-G-000021, U-G-000022):**

- Modellfehler im Wertefenster: Sichtprüfung mit fehlerhaften
  Referenzmodellen (Fehler-ID, Kurztext, Fundstelle, Abhilfe sichtbar).
- Startzustand beim Moduswechsel (`core_adapter`: `start`-Parameter,
  `startState`; `main_window`: `lastRunDynamic_`). Test `testStartState`
  in `gui_core_integration`. Sichtprüfung Optimist: Statik konvergiert
  trocken; danach Dynamik bleibt in Ruhelage (ΣF = 0); Dynamik ab
  Initialwerten zeigt „Vollgelaufen: Rumpf“ und sinkt; danach Statik
  „unloesbar … geflutet: Rumpf“. (Historischer Nachweis mit dem Optimisten
  vor der Anhebung seiner Geometrie; siehe unten.)
- ctest 17/17 grün.

**Nachtrag 27.10.2026 (U-G-000023, Optimist angehoben):**

- Vektorauswahl in zwei Rubriken (A-G-00026): `force_display`
  (`derivedContributions`, `colorForContribution`, Label „gesamt“),
  `vtk_scene` (`VectorSelection`, `contributionVisible`), `main_window`
  (Raster Kräfte/Momente, Settings `darstellung/kraft_i`, `moment_i`).
  Test `testDerived` (Summen, Schwerpunkt, Teilmomente, ungültige Beiträge
  ausgeschlossen). Eigenentscheidung: Gesamtwerte nur in 3D und Detail,
  nicht in der Tabelle; Test-/Fremdbeiträge immer sichtbar.
- Sichtprüfung Optimist-Statik: Gesamtgewicht und Gesamtauftrieb liegen
  auf einer Wirkungslinie; Teilmomente um S sichtbar.
- Der Optimist (Core-Referenzmodell) wurde auf Nutzerauftrag um +1,354 m
  in z angehoben und schwimmt ab Initialpose aufrecht (Bordrand 0,31 m
  über Wasser, nicht geflutet). Der Flutungsnachweis A-G-00024 braucht
  daher künftig eine Startlage mit eingetauchtem Bordrand.
- ctest 17/17 grün.
