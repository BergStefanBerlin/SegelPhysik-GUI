# Umsetzungsplan GUI-Erstausbau

**Status: FINAL – umgesetzt in Version G-0.1.**

**Stand:** 26.10.2026 · **Version:** G-0.1

**Maßgebliche Spezifikation:** [GUI-Erstausbau](../Spezifikationen/Spezifikation_GUI_Erstausbau.md) (A-G-00001 bis A-G-00008)

**Entwurfspunkte und Restfragen:** [Entwurfspunkte GUI-Erstausbau](../Spezifikationen/Entwurfspunkte/GUI_Erstausbau.json)

Dieser Plan führt die Umsetzungsschritte U-G-000001 bis U-G-000010 für den
ersten GUI-Ausbau. Alle Schritte sind in Version G-0.1 umgesetzt; die
Zuordnung zu den Anforderungen steht in Abschnitt 3 und in der
Rückverfolgbarkeitsmatrix (Abschnitt 6). Die Umsetzung begann auf
ausdrücklichen Nutzerauftrag vor der formalen Finalisierung; die Kennungen
wurden bei der Finalisierung am 26.10.2026 vergeben.

## 1. Zielbild und Umsetzungsgrenze

Ein Desktop-Programm mit Qt 6 Widgets und VTK ermöglicht die Auswahl und
Prüfung eines Modells, startet statische oder dynamische Core-Läufe außerhalb
des GUI-Threads und zeigt passende Ergebnisse in einer interaktiven 3D-Szene:
Körperpose, Wasseroberfläche und Wasserlinie, Ergebnisstatus, Zeitangaben und
Kraftpfeile. Dynamische Läufe werden live und schrittweise vom Core
berechnet, optional im Echtzeit-Takt, und alle Zustände bleiben für den
Rückblick gespeichert (A-G-00005). Die Modelle werden automatisch aus dem
Modellordner aufgelistet (A-G-00002).

Der Core bleibt für Physik und Ergebniszustände zuständig. Ein GUI-seitiger
lokaler Adapter verbindet dessen C++-Schnittstelle mit Qt/VTK. JSON ist für
diese lokale Verbindung nicht vorgeschrieben. Die GUI ruft den Core aus ihrer
eigenen Bedienoberfläche über den Adapter auf, um Modellprüfungen sowie
statische und dynamische Simulationen selbst zu starten und Ergebnisse
zurückzuerhalten. Ein separater manueller CLI-Aufruf oder externer Prozess ist
nicht vorausgesetzt; vorbereitete Referenzdateien allein reichen zur Abnahme
nicht aus. Visuelles Clipping darf die Core-Geometrie oder -Berechnung nicht
verändern.

## 2. Voraussetzungen

Ursprünglich vorgesehene Voraussetzungen; Stand G-0.1 jeweils in Klammern:

1. Die Spezifikation ist vom Nutzer ausdrücklich finalisiert (erfüllt am
   26.10.2026).
2. Die Entwurfspunkte sind entschieden, übernommen, zurückgestellt oder
   verworfen (erfüllt; Lizenzprüfung IDEE-G-00006 weiterhin offen).
3. Jede angenommene Anforderung besitzt ein prüfbares Abnahmekriterium und
   ist mindestens einem Umsetzungsschritt zugeordnet (erfüllt, Abschnitt 6).
4. Die Umsetzungsschritte tragen U-G-Kennungen (erfüllt).

Die Referenzhinweise aus der Core-Übergabe belegen einen startfähigen Quader
und Ergebnisbeispiele. Sie begründen keine Simulierbarkeit aller beigefügten
Modelle. Für die Integration wird deshalb zunächst nur ein durch die
Core-Modellprüfung akzeptierter Referenzfall vorausgesetzt.

## 3. Umsetzungsschritte

| ID | Arbeit und Abhängigkeit | Anforderungen | Abhängig von | Version | Status |
|---|---|---|---|---|---|
| U-G-000001 | Projekt- und Buildbasis: CMake-Build für GUI und Core-Anbindung, Qt-/VTK-Abhängigkeiten (vcpkg), Qt-Hauptfenster mit VTK-Ansicht. | A-G-00001 | – | G-0.1 | umgesetzt; Lizenzprüfung offen (IDEE-G-00006) |
| U-G-000002 | Core-Adapter und Modellprüfung: lokale C++-Grenze für Prüfung, `simulieren`, Laufbericht und Diagnosen; Core-Aufrufe außerhalb des GUI-Threads; Modell-ID/-fassung und Auswahlwechsel. | A-G-00001, A-G-00002 | U-G-000001 | G-0.1 | umgesetzt, ctest |
| U-G-000003 | 3D-Grunddarstellung: Modellgeometrie in Ergebnispose, Wasserfläche bei Welt-`Z = 0`, Wasserlinie rein grafisch, Kamera. | A-G-00003 | U-G-000002 | G-0.1 | umgesetzt, Sichtprüfung |
| U-G-000004 | Statischer Lauf und Zustand: statischen Lauf über den Adapter starten, Status, Pose und Diagnose konsistent anzeigen. | A-G-00004 | U-G-000002, U-G-000003 | G-0.1 | umgesetzt, Smoke-Test |
| U-G-000005 | Dynamischer Lauf und Zustandsauswahl: Laufoptionen abbilden, Zustände per Zeitleiste auswählen, Status gemäß Laufbericht. | A-G-00005, A-G-00008 | U-G-000004 | G-0.1 | umgesetzt, Smoke-Test |
| U-G-000006 | Laufzeit und Messwerte: Simulationszeit, Echtzeit und Core-Rechenzeit getrennt mit Status anzeigen; keine Detailmessungen im Normalbetrieb. | A-G-00006 | U-G-000004, U-G-000005 | G-0.1 | umgesetzt; Fehler-/Abbruchpfad ohne Referenzfall |
| U-G-000007 | Kraftpfeile und Legende: Ansatzpunkte rotieren und verschieben, Vektoren nur rotieren; Gewicht und Auftrieb gekennzeichnet; Kraftbeitragstabelle. | A-G-00007 | U-G-000003, U-G-000004 | G-0.1 | umgesetzt, Sichtprüfung |
| U-G-000008 | Modellordner-Katalog: automatische Modellliste aus `modell.json`-Dateien, Ordnerbeobachtung, Hintergrundprüfung, „nur Ansicht“ für abgelehnte Modelle, Merken von Ordner und Modell. | A-G-00002 | U-G-000002 | G-0.1 | umgesetzt, ctest `gui_model_catalog`, Smoke-Test |
| U-G-000009 | Live-Dynamik: schrittweiser Core-Lauf im Worker (`dynamikschritt`), optionaler Echtzeit-Takt, Anzeige mit begrenzter Bildrate, Speicherung aller Zustände, Rückblick, „bis Abbruch“ und „Lauf stoppen“, mitwachsender Kameraausschnitt. | A-G-00005, A-G-00006 | U-G-000005, U-G-000006 | G-0.1 | umgesetzt, ctest Live-Tests, Smoke-Test |
| U-G-000010 | Integration und Abnahme: Modell- und Moduswechsel, statische und dynamische Läufe gemeinsam prüfen; Nachweise dokumentieren. | A-G-00001 bis A-G-00008 | U-G-000001 bis U-G-000009 | G-0.1 | technisch geprüft, Nutzer-Sichtabnahme 26.10.2026; Lizenzfreigabe offen |

## 4. Technische Leitlinien

### Threading und Zustandslebensdauer

- Der synchrone Core-Gesamtlauf wird in einem Worker außerhalb des
  GUI-Oberflächenthreads ausgeführt.
- Auswahl, Prüfstart, Laufstart und Ergebnisabruf werden von der GUI
  gesteuert. Der lokale Adapter ruft die Core-Funktionen direkt auf; CLI und
  vorbereitete Referenzaufzeichnungen sind weder Laufzeitvoraussetzung noch
  Ersatz für die GUI/Core-Integration.
- Qt-/VTK-Objekte verbleiben auf der GUI-Seite und werden nicht in den Core
  eingeführt.
- Der Worker übergibt abgeschlossene, konsistente Ergebnisdaten. Die
  Darstellung liest keine gerade veränderten Solverobjekte.
- Ein Modellwechsel oder ein neuer Lauf invalidiert bzw. trennt vorherige
  Zustände und Pfeile eindeutig anhand von Modellidentität/-fassung und
  Laufkontext.
- Live-Dynamik (U-G-000009): Der Worker ruft `dynamikschritt` schrittweise
  auf und legt nur rohe Zustände unter einem Mutex ab. Die GUI erzeugt
  Anzeigedaten nur für angezeigte Zustände mit eigener Modellkopie, sodass die
  Darstellung den Core nicht bremst.

### Darstellung und Koordinaten

- Modellpose und Quaternionreihenfolge `xyzw` werden gemäß Core-Vertrag
  interpretiert.
- Ansatzpunkte werden vom Modell- ins Weltsystem rotiert und verschoben;
  Kraftvektoren werden nur rotiert.
- Wasserfläche und -linie beziehen sich auf Welt-`Z = 0`; ihre Darstellung
  verändert keine Physik- oder Ergebnisdaten.
- Grafische Pfeilskalierung ist eine Visualisierung, keine Änderung der
  angezeigten SI-Werte. Nullvektoren erhalten keine erfundene Richtung.

### Zeitmessung

- Die Core-Gesamtlaufzeit umfasst laut Spezifikation laufbezogene Vorbereitung
  einschließlich Modellladen, Berechnung und Abschluss der Aufzeichnung;
  Prozessstart und -ende liegen außerhalb. Der GUI-Anzeigetext benennt die
  reale Gesamtlaufzeit und nicht Solver- oder Renderzeit.
- `zeit_s` ist Simulationszeit, die reale Laufzeit ist ein davon unabhängiger
  Messwert.
- Erfolgs-, Fehler- und Abbruchstatus werden zusammen mit der verfügbaren
  Gesamtdauer angezeigt.
- Detailmessungen bleiben im normalen Lauf deaktiviert. Messregister und
  Detailwerte werden nur dann GUI-seitig ergänzt bzw. angezeigt, wenn dies
  nach Finalisierung ausdrücklich im Umfang liegt.

## 5. Meilensteine und Abnahmekriterien

Stand G-0.1: Die Meilensteine A bis E sind technisch bestanden (Nachweise in
Abschnitt 6 und 7). Bei Meilenstein F sind die technische Abnahme und die
Sichtabnahme durch die Nutzerin am 26.10.2026 erfolgt; die Lizenzprüfung ist
noch offen. Meilenstein D wurde durch die Live-Dynamik (U-G-000009)
erweitert: Statt einer abgeschlossenen Aufzeichnung werden die live
gespeicherten Zustände zurückgeblickt.

### Meilenstein A – Freigegebene Grundlage

**Eintritt:** Entwurf und Entwurfspunkteliste liegen vor.

**Abnahme:** Die Spezifikation ist ausdrücklich finalisiert; alle aktiven
Anforderungen besitzen A-Kennungen und Abnahmekriterien. Der Plan enthält
zugeordnete U-Kennungen, Abhängigkeiten und freigegebene Qt-/VTK- und
Lizenzannahmen. Bis dahin keine Implementierung nach diesem Plan.

### Meilenstein B – Startfähige GUI und Modellansicht

**Eintritt:** Meilenstein A bestanden.

**Abnahme:** GUI-Build startet; ein gültiges Core-Referenzmodell kann geprüft
und ausgewählt werden. Körper, Wasserfläche bei `Z = 0` und sichtbare
Wasserlinie erscheinen in der Szene. Core-Prüffehler werden als Fehler und
nicht als erfolgreiche Auswahl angezeigt. Die GUI selbst stößt die Prüfung
über den lokalen Adapter an; kein separater CLI-Aufruf ist nötig.

### Meilenstein C – Statische Darstellung

**Eintritt:** Modellansicht und lokaler Core-Adapter integriert.

**Abnahme:** Statische Referenz wird mit passender Pose, Status, Diagnose,
Wasserlinie und den zugehörigen Kräften angezeigt. Ein nicht konvergierter,
unlösbarer oder abgebrochener Lauf wird nicht als erfolgreicher
Gleichgewichtszustand ausgegeben. Zusätzlich startet die GUI einen statischen
Core-Lauf selbständig und übernimmt dessen Laufbericht sowie Ergebniszustand.

### Meilenstein D – Dynamische Wiedergabe und Kräfte

**Eintritt:** Statische Darstellung bestanden.

**Abnahme:** Dynamische Referenzzustände lassen sich auswählen/wiedergeben.
Pose, Zeit, Schritt und Pfeile stammen bei jedem Wechsel aus demselben
Zustand. Transformierte Angriffspunkte und Vektoren stimmen mit dem
Ergebnisvertrag überein; Modell- und Moduswechsel lassen keine fremden
Zustände oder Pfeile zurück. Die GUI startet den dynamischen Core-Lauf selbst
und lädt dessen vollständige Aufzeichnung für die Wiedergabe; eine
vorgefertigte Referenzdatei allein besteht diesen Meilenstein nicht.

### Meilenstein E – Zeit- und Fehlerpfade

**Eintritt:** Statische und dynamische Laufpfade integriert.

**Abnahme:** Erfolgs-, Fehler- und Abbruchfälle zeigen endliche,
nichtnegative Gesamtdauer mit korrektem Status. Simulationszeit und reale
Gesamtlaufzeit sind getrennt benannt. Normale GUI-Läufe aktivieren keine
Detailmessungen; Messwerte werden weder erfunden noch aus verschachtelten
Messpunkten falsch aufsummiert.

### Meilenstein F – Gesamtabnahme

**Eintritt:** Meilensteine B bis E bestanden.

**Abnahme:** Freigegebene Anforderungen sind einzeln gegen ihre Tests und
Nachweise abgeglichen. Referenzläufe, Modus-/Modellwechsel, Fehlerbehandlung,
Transformationsregeln und Verteilungs-/Lizenzprüfung sind dokumentiert.
Technische Fertigstellung und fachliche Nutzerabnahme werden getrennt
festgehalten.

## 6. Nachweise und Statusführung

Für die spätere Umsetzung sind mindestens folgende Nachweise einzuplanen:

- Build- und Startnachweis für Qt-GUI, VTK-Ansicht und lokale Core-Anbindung.
- Testnachweis der Modellprüfung sowie korrekter Zuordnung von Modell-ID und
  Modellfassung.
- Bild-/Sichtprüfung für Körper, Wasserfläche, Wasserlinie und Kraftpfeile.
- Referenzvergleich statischer und dynamischer Zustände einschließlich
  Punkt-/Vektortransformationen.
- Tests für Moduswechsel, Modellwechsel, fehlerhafte Läufe, Abbruch und
  unvollständige Aufzeichnungen.
- Nachweis der getrennten Anzeige von Simulationszeit und realer
  Gesamtlaufzeit sowie der deaktivierten Detailmessung im Normalbetrieb.
- Dokumentation verwendeter Qt-/VTK-Versionen und ihrer vor Verteilung
  geprüften Lizenzbedingungen.

Der Status jedes Schritts bleibt „geplant“, bis Implementierung,
zugehöriger Test und Integration mit tatsächlichen Ergebnissen belegt sind.
Ein bestandener Test ersetzt nicht die fachliche Abnahme. Fehlende Nachweise,
Abweichungen und nicht abgeschlossene Punkte bleiben sichtbar.

### Rückverfolgbarkeitsmatrix G-0.1

| Anforderungs-ID | Spezifikation/Abschnitt | Umsetzungsschritt(e) | Test/Abnahmekriterium | Status |
|---|---|---|---|---|
| A-G-00001 | Abschnitt 1, 2 | U-G-000001, U-G-000002, U-G-000010 | ctest `gui_core_integration`; Läufe aus der GUI ohne CLI | umgesetzt in G-0.1 |
| A-G-00002 | Abschnitt 2 | U-G-000002, U-G-000008, U-G-000010 | ctest `gui_model_catalog`; Smoke-Test: 11 Modelle, 3 als „nur Ansicht“ | umgesetzt in G-0.1 |
| A-G-00003 | Abschnitt 2 | U-G-000003, U-G-000010 | ctest `gui_display_mesh`; Sichtprüfung Körper, Wasserfläche, Wasserlinie | umgesetzt in G-0.1 |
| A-G-00004 | Abschnitt 2 | U-G-000004, U-G-000010 | Smoke-Test Quader: konvergiert, Gewicht = Auftrieb = 74 556 N | umgesetzt in G-0.1 |
| A-G-00005 | Abschnitt 2 | U-G-000005, U-G-000009, U-G-000010 | ctest Live-Tests (ungetaktet = `simulieren`, Takt, Stopp); Smoke-Test Echtzeit 4,710 s / 4,72 s, Rückblick | umgesetzt in G-0.1 |
| A-G-00006 | Abschnitt 2 | U-G-000006, U-G-000009 | Smoke-Test: Simulations-, Echt- und Core-Rechenzeit getrennt | umgesetzt in G-0.1; Fehler-/Abbruchpfad ohne Referenzfall |
| A-G-00007 | Abschnitt 2 | U-G-000007, U-G-000010 | Sichtprüfung Pfeile und Legende; Kraftbeitragstabelle | umgesetzt in G-0.1 |
| A-G-00008 | Abschnitt 2 | U-G-000005, U-G-000010 | Smoke-Test Modus- und Modellwechsel | umgesetzt in G-0.1 |

## 7. Ist-Stand der technischen Umsetzung (26.10.2026, Version G-0.1)

Die Umsetzung erfolgte auf ausdrücklichen Nutzerauftrag vor der formalen
Finalisierung der Spezifikation. Die A-/U-Kennungen wurden bei der
Finalisierung am 26.10.2026 vergeben; alle Schritte gehören zu G-0.1.

**Nachweise:**

- Abhängigkeiten: vcpkg (`x64-windows`, 28 Pakete inkl. Qt 6 und VTK) in
  `C:\spdeps\installed`. Overlay-Ports für libaec (HTTP 429 am Ursprungsserver)
  und gl2ps (HTML statt Archiv) mit geprüften SHA512-Hashes, siehe README.
- Build: Visual Studio 2022, Release, `/W4 /WX` ohne Fehler und Warnungen.
- Tests: `ctest` 13/13 bestanden (`gui_core_integration`, `gui_display_mesh`
  und 11 Core-Tests).
- Statischer Smoke-Test, Quader-Referenzmodell: „konvergiert · 1 Zustände“,
  Gewicht = Auftrieb = 74 556 N, Wasserfläche, Wasserlinie und Kraftpfeile
  sichtbar, Rechenzeit ca. 1–3 ms.
- Dynamischer Smoke-Test: Dauer 2 s, Schrittweite 0,01 s ergeben
  201 Zustände in ca. 8 ms Rechenzeit. Wiedergabe per Slider; Zustand 61 bei
  0,600 s: Auftrieb 67 285 N. Simulationszeit und Rechenzeit werden getrennt
  angezeigt.
- Die GUI ruft den Core selbständig als gelinkte Bibliothek auf. Ein CLI oder
  externer Prozess wird nicht benötigt.

**Beobachtungen und offene Punkte:**

- Die Core-Dynamik des Quaders ist ungedämpft. Der Körper überschwingt und ist
  bei 2 s vollständig eingetaucht (78 480 N). Das ist Core-Verhalten, kein
  GUI-Fehler; eine Dämpfung ist Core-Thema.
- Der UI-Ablauf wurde nur manuell bzw. per Skript mit Screenshots geprüft;
  automatisierte UI-Tests fehlen.
- Abbruch-, Fehler- und Teilaufzeichnungspfade sind implementiert, aber nicht
  mit einem Referenzfall belegt.
- Die Lizenzprüfung für Qt und VTK vor einer Verteilung steht aus.
- Die Spezifikation ist zu finalisieren (A-/U-Kennungen), die fachliche
  Nutzerabnahme steht aus.

### 7.1 Ergänzung: Modellordner und Live-Dynamik (U-G-000008, U-G-000009)

Umgesetzt nach Nutzerangaben zu IDEE-G-00001 und IDEE-G-00002 (jetzt
A-G-00002 und A-G-00005).

- Modellordner: Die linke Liste zeigt automatisch alle `modell.json` des
  gewählten Ordners und seiner Unterordner (11 Core-Beispielmodelle). Der
  Ordner wird beobachtet, Änderungen erscheinen ohne Neustart. Die Modelle
  werden im Hintergrund geprüft; vom Core abgelehnte Modelle (420er,
  Folkeboot, Optimist: CORE-MOD-0004) erscheinen grau als „nur Ansicht“.
  Ordner und Modell werden gemerkt.
- Live-Dynamik: Der Core rechnet im Worker-Thread Schritt für Schritt
  (`dynamikschritt`), ohne Vorausrechnung. Die Anzeige holt etwa alle 33 ms
  den neuesten Zustand. Mit „Echtzeit“: 4,710 s Simulationszeit bei 4,72 s
  Wanduhr; Core-Rechenzeit 8,27 ms (0,2 %). Lauf von 10 s ergibt
  1001 Zustände; Rückblick per Slider während und nach dem Lauf. Ein Lauf ohne
  Dauer läuft bis „Lauf stoppen“.
- Kamera: Der Ausschnitt wächst bei Bedarf mit, damit bewegte Körper im Bild
  bleiben.
- Tests: `ctest` 14/14 bestanden (neu `gui_model_catalog`; Live-Tests:
  ungetaktet identisch mit dem Dateilauf, getaktet ≥ 0,19 s für 0,2 s,
  endloser Lauf stoppt).
