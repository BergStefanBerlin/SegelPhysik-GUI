# Umsetzungsplan G-0.9: Statisches Modell und stationaere Fahrt

**Stand:** 02.11.2026.
**Status:** Geradliniger Umfang, Kurvenfahrt, Druckpunkte und integrierte Grafikkonfiguration implementiert und gezielt integrationsgeprueft. Persoenliche Nutzerabnahme offen. Bekannte unabhaengige Bestandsfehler siehe Abschnitt 5.
**Ziel:** G-0.9; der veroeffentlichte Tag G-0.8 bleibt unveraendert.

Massgeblich:
[GUI-Spezifikation im veroeffentlichten G-0.9-Stand](https://github.com/BergStefanBerlin/SegelPhysik-GUI/blob/G-0.9/Spezifikationen/Spezifikation_GUI_Statisches_Modell.md).
Physikalische Voraussetzung:
[Core-Spezifikation](../../CORE/Spezifikationen/Spezifikation_Stationaeres_Fahrtgleichgewicht.md)
und [einziger Core-Plan C-0.8](../../CORE/Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md#stationaeres-fahrtgleichgewicht).
Dieser Plan ist die einzige Umsetzungsplanung fuer G-0.9; weitere Arbeiten
dieses Versionsschritts hier aufnehmen, keine parallelen Themenplaene.

## 1. Umfang und Ausgangslage

### Druckpunkte und integrierte Grafikkonfiguration (02.11.2026)

#### Aktueller Bediennachtrag: ausschliesslich statische Techniktabelle

Ausdruecklicher Korrektur- und Umsetzungsauftrag vom 02.11.2026, 20:14 Uhr.
Die Anforderungen A-G-00090/00091/00092 wurden im selben fachlichen
Gegenstand angepasst; Kennungen bleiben erhalten. Dieser Nachtrag ersetzt
die fruehere gemeinsame Profilwahl und Uebernehmen-/Abbrechen-Bedienung.
Die nachfolgenden historischen Beschreibungen und Testzahlen dokumentieren
den vorherigen Stand und sind kein Nachweis fuer den neuen Bedienvertrag.

| Bestehender U-Schritt | Aktueller Umfang | Abhaengigkeit | Stand |
|---|---|---|---|
| U-G-000087 | Button nur im statischen Modell; Technik-Tabelle ohne Optikreiter; Checkboxspalte ohne Titel, Beschreibung, CPU Last, GPU Last | Nutzerkorrektur A-G-00090 | implementiert |
| U-G-000088 | Wirkung/Voraussetzungen im Beschreibungstext, qualitative CPU-/GPU-Last je Tabellenzeile, keine Prozentmesswerte | U-G-000087; A-G-00091 | implementiert |
| U-G-000089 | jede gueltige Aenderung sofort speichern/anwenden; Fehler mit Bedienwertruecknahme; vorhandenes QSettings unter statisch/grafik/profile_v1, dynamische Altwerte unveraendert | U-G-000087; A-G-00092 | implementiert |
| U-G-000090 | echte Profilanwendung und dynamische Isolation, Prozessneustart, Tabelle bei drei DPI-Stufen, Standard-EXE | U-G-000088, U-G-000089 | technisch abgeschlossen; persoenliche Sichtabnahme offen |

Tatsaechliche gezielte Pruefung: frisch gebaute `gui_graphics_settings`,
`gui_graphics_dialog`, `gui_workspace_tabs_100/150/200`: **5/5, 93,10 s**.
Geprueft: jede Featurezeile mit beiden Lastspalten, keine Optik-/Apply-/
Cancelbedienung, sofortige Speicherung/Anwendung auf echte Szene,
Schreibfehler mit Checkbox-/Zahlenwertruecknahme, PBR-Abhaengigkeiten,
Wiederherstellung in neuer Fensterinstanz und echtem Unterprozess,
unveraenderte dynamische Grafikwerte, Reset und bisherige Druckpunkte.
Bildnachweis: `graphics_configuration.bmp` in den drei bestehenden
`C:\spdeps\guibuild\graphics-evidence\workspace-tabs-{100,150,200}`-Ordnern.
Persoenliche Sichtabnahme bleibt getrennt; kein Core-Eingriff.

Abschliessend mit entfernter Materialvorschau und exakter Behandlung
kleiner positiver Spurparameter erneut frisch gebaut:
**5/5, 93,44 s**. Zusaetzlich bestaetigt: `0,00001` wird nicht geklemmt,
ungueltiger Wert wird gemeldet/zurueckgestellt; sofortige Aenderung im
echten stationaeren 420er-Ergebnis erhaelt Werte, Kursrate, Kamera und
dynamischen Speicherstand. `Zurueck zum Boot` zeigt denselben Zustand.
Standard-EXE `C:\spdeps\guibuild\Release\segelphysik_gui.exe` frisch
gebaut (02.11.2026, 20:25 Uhr), tatsaechlich gestartet, Hauptfenster
reagiert, normal beendet mit Exitcode 0 (Pruefprozess PID 44500).
Keine Git-Aktion; Arbeitsstand bleibt uncommittet.

Bedienpraezisierung 21:00 Uhr, gleicher Umfang U-G-000088/000089/000090:
Status je Effekt direkt in der Beschreibung, aus dem aktiven Renderer
abgefragt und sofort aktualisiert. Gespeicherte Schalter gelten fuer alle
unterstuetzten Teile; fehlende Materialdaten sind kein Fehler und loeschen
die Einstellung nicht. Durchsicht zeigt konkrete Deckkraft/Transparenz,
Wasser Basisdeckkraft, Animation den statischen Zeitbezug. Bestehende
Nutzerwerte und dynamische Profile bleiben unveraendert. Keine neue Physik.
Abschlusspruefung dieser Praezisierung: Standard-EXE und beide betroffenen
GUI-Testziele frisch gebaut; Settings, Tabellenseite und Workspace-DPI
100/150/200 **5/5, 94,12 s** bestanden. Statusaktualisierung nach Checkbox-
und Deckkraftaenderung explizit geprueft. Tatsaechlicher Standard-EXE-Start
reagiert, normaler Exit 0 (Pruefprozess PID 48108).

**Status:** am 02.11.2026 um 19:00 Uhr finalisiert und Umsetzung beauftragt.
Aktive Schritte und Status siehe nachfolgende Tabelle; keine
bestandenen Tests vor Ausfuehrung behaupten.
Massgebliche Themen bleiben getrennt:
[Druckpunkte Abschnitt 5](../Spezifikationen/Spezifikation_GUI_Lateraldruckpunkt.md#5-ldp-und-ersatz-sdp-als-punkte-02112026)
und [Grafikkonfiguration Abschnitt 10](../Spezifikationen/Spezifikation_GUI_Grafikprofile_und_Setup.md#10-darstellung-im-ansichtsfenster-konfigurieren-02112026).
Physikalische Voraussetzung ist der
[Core-Plan](../../CORE/Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md#korrekter-ldp-und-ersatz-sdp).
Kein paralleler GUI-Plan und keine Sammelspezifikation mit zwei
unabhaengigen Gegenstaenden anlegen.

Dokumentierte Annahmen nach nicht verfuegbarer Nutzerantwort:
gleicher neuer Darstellungs-Button in Statisch und Dynamisch,
Technik/Optik und gemeinsamer bestehender Grafikstand bleiben
innerhalb der Seite erhalten. Grafik einschliesslich temporaerer
Materialvorschau zieht aus Setup in die Seite um; Bootsauswahl
bleibt unveraendert. Aufwand qualitativ erklaeren, keine erfundenen
FPS/Prozentangaben und keine neue Benchmarkpflicht.

#### Eintrittsgate und Abhaengigkeiten

Gate durch den vollstaendigen Umsetzungsauftrag erfuellt.
Neue A-G-/U-G-Kennungen wurden oberhalb der geprueften Hoechstwerte
vergeben; IDEE-Zuordnung und ersetzte Bedienvertraege sind dokumentiert.
Die folgende Tabelle dokumentiert ausschliesslich den historischen
Planentwurf vor Freigabe. Ihre Statuswerte sind keine aktuellen
Umsetzungsstaende; massgeblich ist nur die U-Tabelle weiter unten.

| Arbeitspaket | Entwurfsbezug | Ergebnis / Abnahme | Voraussetzung | Status |
|---|---|---|---|---|
| Vertragsabgleich | IDEE-G-00056, IDEE-G-00057 | Core-CPPI-0031/0101/0102, Modell-XYZ, Snapshot/Altstatus und Ersatzlast-Herkunft pruefen | Finalisierungsgate und Core-Meilenstein Vertrag bereit | geplant |
| Marker | IDEE-G-00056, IDEE-G-00057 | zwei beschriftete Punkte, alte Linie entfernen, Einzelschalter unabhaengig von Navigation/Vektoren; fehlend/geometrisch/null/alt korrekt | Vertragsabgleich und Core-Meilenstein Daten bereit | geplant |
| Konfigurationsseite | IDEE-G-00058, IDEE-G-00060 | Button statt Dropdown, Seite statt Boot, Technik/Optik innen, dauerhaft erreichbares Uebernehmen rechts unten; Abbrechen | Finalisierungsgate | geplant |
| Featureerklaerungen | IDEE-G-00059 | jeden vorhandenen Schalter/Parameter mit Wirkung, Voraussetzungen und begruendeten CPU/GPU/Speicherkosten erklaeren | Finalisierungsgate | geplant |
| Transaktion und Umzug | IDEE-G-00058, IDEE-G-00060 | bestehende Settings validieren/speichern, Fehler sichtbar, Profile migrieren, Setup-Grafik entfernen und Materialvorschau integrieren | Konfigurationsseite und Featureerklaerungen | geplant |
| Integration und Deployment | IDEE-G-00056..00060 | gemeinsame echte Core-Fixtures, DPI, Reset, Physikinvarianz, Neustart/Fehlerpfade, Standard-EXE | Marker, Transaktion und Umzug, Core-Meilenstein Integration bereit | geplant |

Konfigurationsseite und Featuretexte benoetigen keine neue
Core-Grafikschnittstelle und koennen unabhaengig von der
Panelkorrektur vorbereitet werden. Marker nicht vor Core-Lieferung
mit einer GUI-eigenen LDP-Schaetzung ueberbruecken.

#### Meilensteine und Abnahme

- **Vertrag bereit:** Einheiten, Status, Kennzeichnung der korrigierten
  Auswertung und Ersatz-SDP fuer denselben Lauf festgelegt.
  Historische geometrische Daten nicht zu kraftbasiertem LDP umdeuten.
- **Marker bereit:** exakte Core-X/Y/Z mit Pose transformiert,
  LDP unter Wasser erkennbar, zwei unterscheidbare Punkte und keine
  alte vertikale Linie. Zulaessigen Aussenpunkt nicht klemmen.
  Eingabeaenderung erzeugt keine Mischung aus altem und neuem Lauf.
- **Konfiguration bereit:** tatsaechlicher Ersatz des Dropdowns,
  kein modales Fenster, vollstaendige Features und sichtbare
  Erklaerungen/Kostenklassen, Profile erhalten. Uebernehmen speichert
  und kehrt erst bei Erfolg zum selben Boot zurueck; Abbrechen
  verwirft. Schreib-/Validierungsfehler lassen Seite und Entwurf offen.
- **Integration bereit:** 100/150/200 Prozent mit kleiner/grosser
  Ansicht, Tastatur und Checkboxhaken. Reiterwechsel im Lauf
  stoppt/resetet weiterhin, verwirft unbearbeitete Konfiguration
  und stellt keine spaeten Marker wieder her. Navigation aus
  darf Druckpunkte nicht verstecken. Dynamik, Bootsauswahl,
  Kamera, Zeitauswahl, Kraefte und Loesungen bleiben bis auf
  ausdruecklich beauftragten Reiterreset fachlich unveraendert.
- **Deployment bereit:** Standard-EXE frisch bauen, lokale Plugins,
  tatsaechlicher Start und echte Punktdarstellung pruefen.
  Nutzer-Sichtabnahme bleibt eigener offener Schritt.

Kleinste passende Testziele frisch bauen: `gui_force_display`,
`gui_core_integration`, `gui_graphics_settings`,
`gui_graphics_dialog` (bestehenden Test passend zur eingebetteten
Seite umbauen/umbenennen), `gui_workspace_tabs_100/150/200` und
gezielte Grafik-/Navigationsregression bei den geaenderten Oberflaechen.
Testnamen vor Ausfuehrung gegen CTest-Inventar pruefen; keine
behaupteten bestandenen Tests aus dieser reinen Dokumentarbeit.

#### Historische Entwurfsrueckverfolgbarkeit vor Freigabe

| Entwurfspunkt | Arbeitspakete | Verlangter Nachweis | Stand |
|---|---|---|---|
| IDEE-G-00056 | Vertragsabgleich, Marker, Integration | zwei Punkte statt Linie, Position/DPI, getrennte Schalter | nicht implementiert |
| IDEE-G-00057 | Vertragsabgleich, Marker, Integration | Core-Snapshot, Status/Frames/Pose/alt, Reset/Fehlwerte | nicht implementiert |
| IDEE-G-00058 | Konfigurationsseite, Transaktion und Umzug, Integration | Button, eingebettete Seite, Setup entfernt, beide Reiter erreichbar | nicht implementiert |
| IDEE-G-00059 | Featureerklaerungen, Integration | jede vorhandene Option, Kosten/Herkunft/Abhaengigkeit sichtbar | nicht implementiert |
| IDEE-G-00060 | Konfigurationsseite, Transaktion und Umzug, Integration | Profile erhalten, Speicherfehler, Abbrechen, Neustart und Rueckkehr | nicht implementiert |

Dokumentierte Annahmen sind mit dem anschliessenden vollstaendigen
Umsetzungsauftrag angenommen. Historische Entwurfstabelle oben bleibt
zur Herkunft erhalten; massgeblich sind jetzt diese U-Schritte.
Hoechstwerte vor Vergabe A-G-00087 und U-G-000084 geprueft.

| U-Schritt | Anforderungen | Umsetzung | Voraussetzung | Status |
|---|---|---|---|---|
| U-G-000085 | A-G-00088, A-G-00089 | Core-CPPI-/JSON-Abgleich, neuer Markerparser und Legacy-/Fehlerstatus | Core-Vertrag bereit | technisch umgesetzt und gezielt geprueft |
| U-G-000086 | A-G-00088, A-G-00089 | zwei beschriftete Punkte, Linie entfernen, persistente Einzelschalter und Reset | U-G-000085; Core-Daten bereit | technisch umgesetzt und gezielt geprueft |
| U-G-000087 | A-G-00090, A-G-00092 | eingebettete Konfigurationsseite statt Dropdown/Setup, zwei Profile und Abbrechen/Uebernehmen | Finalisierung | technisch umgesetzt und gezielt geprueft |
| U-G-000088 | A-G-00091 | sichtbare Wirkungs-/Abhaengigkeits-/Kostenbeschreibung fuer alle Features und Parameter | Finalisierung | technisch umgesetzt und gezielt geprueft |
| U-G-000089 | A-G-00090, A-G-00092 | vorhandene Persistenz, Validierung und Fehlertransaktion, Materialvorschau umziehen | U-G-000087, U-G-000088 | technisch umgesetzt und gezielt geprueft |
| U-G-000090 | A-G-00088..00092 | Core-Fixtures, echte GUI/DPI, Neustart/Reset/Fehlerregression und Standard-Deployment | U-G-000086, U-G-000089; Core-Integration bereit | technisch abgeschlossen; persoenliche Sichtabnahme offen |

Finalisierte Rueckverfolgbarkeit:
A-G-00088/00089 -> U-G-000085/000086/000090;
A-G-00090 -> U-G-000087/000089/000090;
A-G-00091 -> U-G-000088/000090;
A-G-00092 -> U-G-000087/000089/000090.
Technische und persoenliche Abnahme getrennt. Kein Commit/Push/Tag/Release.

#### Tatsaechliche Nachweise der Druckpunkt-/Darstellungserweiterung

- Frisch gebaute Ziele `gui_force_display`, `gui_graphics_settings`,
  `gui_graphics_dialog`, `gui_core_integration` und
  `gui_workspace_tabs_100/150/200`: **7/7 bestanden, 148,55 s**.
  Nach Ergaenzung der echten Zweifarben-Rasterpruefung und expliziter
  Technikansicht nochmals alle drei Workspace-Faelle: **3/3, 100,15 s**.
  Abschliessend mit Tastatur-Uebernehmen und expliziter Profilwiederherstellung
  beide betroffenen Ziele neu gebaut: Workspace-DPI plus Seitentest
  **4/4, 100,06 s**. Nach Core-Abschluss Standard-EXE und Core-Test neu
  gelinkt; Core-Integration, Parser und Persistenz nochmals **3/3, 31,45 s**.
- Ergaenzende Core-Verifikation abgeschlossen: sechs gezielte CTests
  **6/6, 40,41 s**, einschliesslich stationaerer Kurvenfahrt und
  Gesamtsimulation. Anschliessend beide echten Seitenkraftmethoden
  fuer Kiel/Schwert und Ruder in acht analytischen Rotations-/
  Gegenkraftfaellen geprueft: Q/B/X/Z aus unabhaengiger Dreiecksgeometrie
  und Stroemung, Toleranz 1e-8, jeweils vom alten Betragsschwerpunktfehler
  unterscheidbar. Vier betroffene Core-Tests nochmals **4/4, 3,12 s**.
  Bestehende Bilanzregression ist kein expliziter Vorher-nachher-
  Physikinvarianznachweis. Letzter Nachtrag aendert nur Core-Tests und
  den bestehenden Core-Plan; ausgelieferter Produktcode unveraendert.
- Parser: Version 2, exakte XYZ einschliesslich Aussenpunkt,
  Legacy-Kennzeichnung, Ersatzlast gueltig/inaktiv/null und explizite
  Fehler bei falscher Version, Herkunft oder fehlender Position.
  Echte Core-Kurvenfahrt liefert den Ersatz-SDP aus dem eingefrorenen
  Ansatzpunkt bei allen bereits vorhandenen Ruder-Referenzfaellen.
- Darstellung: echte 420er-Ergebnisse mit 1, 8, 24 Grad Ruder,
  getrennte Schalter und Navigation aus/an; beide Markerfarben im
  tatsaechlichen Analyse-Raster nachgewiesen. Zusaetzliche unabhaengige
  Pose-Referenz prueft Translation/Kraengung und LDP bei x=9 m ohne
  Klemmen. Geometrisch/alt/fehlend sowie Reiterreset liefern keine
  falschen oder spaet wiederhergestellten Marker.
- Konfiguration: gleiche Bootsinstanz nach Uebernehmen,
  unveraenderte Ergebniswerte/Zeitauswahl/Kamera, kein zweiter Setup-Weg,
  Entwurf beim Modell-/Reiterwechsel verworfen. Uebernehmen bleibt auch
  bei 1280 x 720 logischen Pixeln und allen drei DPI-Stufen voll im
  sichtbaren Seitenrechteck. Profile und Druckpunktschalter werden in
  einer neuen Fensterinstanz wiederhergestellt.
- Eingebetteter Seitentest: sichtbare Erklaerungen fuer alle Features,
  PBR-Abhaengigkeiten, beide Profile, Standard/alle/keine,
  Tastatur-Uebernehmen, ungueltiger Wert und abgelehnter Commit ohne Ruecksprung.
  Bestehender Settings-Test deckt Migration und echten Schreibfehler ab.
- Bildnachweise unter
  `C:\spdeps\guibuild\graphics-evidence\workspace-tabs-{100,150,200}`:
  `graphics_configuration.bmp` und `turning_rudder_{1,8,24}.bmp`.
  Persoenliche Sichtabnahme und hardwareuebergreifende Leistungsabnahme
  bleiben offen; qualitative Kosten sind keine neu gemessenen Garantien.
- Standard-Deployment:
  `C:\spdeps\guibuild\Release\segelphysik_gui.exe` frisch gelinkt;
  lokale Plugins `platforms\qwindows.dll` und `imageformats\qsvg.dll`
  vorhanden. Tatsaechlicher EXE-Start mit reagierendem Hauptfenster
  geprueft (Testprozess PID 27176); normal beendet, Exitcode 0.
  Kein Testprozess zurueckgelassen, keine Git-Veroeffentlichung.

### Richtungspfeil und Reiterreset (Nutzerfreigabe 02.11.2026, 18:30 Uhr)

Direkte Umsetzungsfreigabe nach drei Renderentwuerfen, einschliesslich
des zuvor vorgemerkten Abbruchs/Resets bei jedem Reiterwechsel.
A-G-00079 entsprechend ausdruecklich geaendert; A-G-00087 neu vergeben
nach Pruefung der aktiven/historischen Hoechstwerte 00086/000081.
Keine neue Core-Schnittstelle oder Core-Physik.

| Umsetzungsschritt | Anforderungen | Umsetzung / Abnahme | Voraussetzung | Status |
|---|---|---|---|---|
| U-G-000082: Richtungspfeil | A-G-00084, A-G-00087 | Core-Rate direkt nutzen, Welt-XY-Pfeil vor dem Bug, Null/Vorzeichen/Staerke/Saettigung; mit Navigation ein-/ausblenden | U-G-000079 | implementiert; Geometrie und echte 420-Anzeige getestet |
| U-G-000083: Reiterreset | A-G-00079 | Abbruch, Timer stoppen, Ergebnisse/Historie/Kamera zuruecksetzen, alte statische Rueckmeldungen per Laufgeneration verwerfen; Neustart erst nach Workerende aus Modellinitialwerten | keine | technisch abgeschlossen; drei Workspace-DPI-Tests bestanden |
| U-G-000084: Integration und Deployment | A-G-00079, A-G-00087 | Null/Vorzeichen/Saettigung, Navigation aus/an, echter statischer/dynamischer Abbruch und Neustart bei 100/150/200 Prozent; Standard-EXE aktualisieren | U-G-000082, U-G-000083 | technisch abgeschlossen; persoenliche Nutzerabnahme offen |

Erster Nachweis vor abschliessender Kamera-/Wasserlinienbereinigung:
`gui_force_display` und `gui_workspace_tabs_100/150/200` **4/4 bestanden**,
109,75 s. Tatsaechlichen 24-Grad-Corefall mit neuem Pfeil visuell geprueft.
Fruehere Nachweise des unveraenderten Weiterlaufens bei Reiterwechsel
gelten ausschliesslich historisch und sind durch die neue Freigabe ersetzt.

Abschliessender Neubau von Standard-EXE und Tests nach Kamera-/Wasserlinienreset:
`gui_force_display` und `gui_workspace_tabs_100/150/200`
**4/4 bestanden**, 107,81 s. Geometrie prueft Null, beide Vorzeichen,
zunehmende Kruemmung, Saettigung, Bootsrichtungswechsel und horizontale Lage.
GUI prueft Navigation aus/an, echte Core-Kurvenloesungen mit Vorzeichen,
Pfeil ueber Wasser, dynamischen Reset/Neustart und das Verwerfen spaeter
statischer Ergebnisse auch nach Rueckkehr in denselben Reiter.
Standard-EXE unter `C:\spdeps\guibuild\Release\segelphysik_gui.exe`
neu gebaut und mit lokalen Qt-Plugins gestartet/reagierend geprueft;
Testinstanz normal geschlossen. `git diff --check` bestanden.
Keine Aenderung an Core-Physik oder V6; kein Commit, Tag oder Push.

### Umsetzung fuer Kurvenfahrt und Drehratenanzeige (02.11.2026)

**Status:** ausdruecklich finalisiert und Umsetzung beauftragt.
Die bisherigen U-G-Schritte beziehen sich auf die geradlinige Lieferung.
Massgeblich ist
[Spezifikation Abschnitt 6 im G-0.9-Stand](https://github.com/BergStefanBerlin/SegelPhysik-GUI/blob/G-0.9/Spezifikationen/Spezifikation_GUI_Statisches_Modell.md#6-stationaere-kurvenfahrt-und-drehratenanzeige).
Keine parallele Spezifikation und kein weiterer Versionsplan.
Ziel G-0.9 zusammen mit Core C-0.8 bestaetigt.

#### Freigabegate

Nutzerfreigabe liegt vor. A-G-00083 bis A-G-00086 und U-G-000078 bis
U-G-000081 nach Pruefung der projektweiten Hoechstwerte vergeben.
Alte Nullraten-/Nullkraftvertraege nicht umdeuten.
V6 und gemeinsame numerische Grenzen aus dem
[Core-Plan](../../CORE/Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md).
Freigegeben: Drehratenfehler <=0,01 Grad/s, Roll-/Stampfwinkelfehler
<=0,1 Grad, vertikale Schwerpunktgeschwindigkeit <=0,001 m/s und
Roll-/Stampfraten <=0,01 Grad/s.
Fahrtfehler bleibt <=0,1 m/s; alle sechs Bewegungsresiduen <=0,1 N/Nm.
Keine zusaetzliche GUI-Physik zur Ueberbrueckung fehlender Core-Felder.

#### Arbeitspakete und Abhaengigkeiten

| Umsetzungsschritt | Anforderungen | Umsetzung / Nachweis | Voraussetzung | Status |
|---|---|---|---|---|
| U-G-000078: Core-Vertragsabgleich | A-G-00083, A-G-00084, A-G-00085 | CPPI-0088..0100 abgeglichen; `kurvenfahrt` explizit, V6 mit `kursdrehrate_rad_s` und `kursdrehrate_status`. Fehlendes Eingabefeld bleibt `false`; Alt-Ergebnisversionen unveraendert lesbar. | Freigabegate und Core-Meilenstein Vertrag bereit | technisch abgeschlossen |
| U-G-000079: Drehratenzeile | A-G-00084, A-G-00086 | Fahrtbetrag -> signierte Grad/s -> Welt-X/Y/Z; rad/s nur umgerechnet, exakte Tooltips/Wertebaum, Null/fehlend/Singularitaet/Naeherung. | U-G-000078 | technisch abgeschlossen; Navigation und drei DPI-Stufen bestanden |
| U-G-000080: Stationaerer Start und Ergebnis | A-G-00083, A-G-00085 | Sichtbarer, standardmaessig aktiver Kurvenmodus; ausschaltbarer geradliniger Altpfad. Eingaben eingefroren; Loesungstyp/Radius, sechs Bewegungsresiduen und aeussere Summen getrennt. | U-G-000078; Core-Lieferung bereit | technisch abgeschlossen; echte Kreis-/Dreh-/420-End-to-End-Faelle bestanden |
| U-G-000081: Integration, DPI und Dokumentation | A-G-00083 bis A-G-00086 | Gemeinsame Core-Fixtures, Navigation, Adapter, 100/150/200-Prozent-Tests, README und Istnachweise; Standard-EXE und lokale Qt-Plugins geprueft. | U-G-000079, U-G-000080 sowie Core-Lieferung bereit | technisch abgeschlossen; gezielte GUI-Abnahme gruen, Nutzerabnahme offen |

Drehratenzeile und Ergebnisverdrahtung sind nach Vertragsabgleich
unabhaengig vorbereitbar; kein produktiver Attrappen-/Altvertragsersatz.
Die GUI meldet unvereinbare weltfeste Lasten aus Core ausdruecklich,
statt Kraftframe oder Laufart heimlich zu aendern.

#### Abnahme und Abschluss

- **Vertragsgate:** Alle angezeigten Werte und Status auf Core-Felder
  abgebildet; alte V5-Nullraten bleiben alte geradlinige Ergebnisse.
- **Anzeige bereit:** Exakte Umrechnung +/-pi/2 rad/s -> +/-90 Grad/s,
  Schraeglage gegen omega-Modell-Z unterscheiden; Zeilenreihenfolge
  Fahrtbetrag -> Kursdrehrate -> Weltachsen bei 100/150/200 Prozent
  und verkleinerter/vergroesserter Karte geprueft. Bestehende
  Achsenwerte und deren Weltbezug bleiben erhalten.
- **Integration bereit:** Echte Core-Referenzen fuer Geradeaus,
  positive/negative Kurvenfahrt und reine Drehung; aeussere
  Zentripetalkraft darf bei erfuellter Bewegungsbilanz kein falsches
  Nichtkonvergenzlabel ausloesen. Fehlende Werte, Singularitaet,
  Naeherung, veraltet, Nichtkonvergenz, Abbruch und Flutung pruefen.
- **Regression:** Bootsauswahl, dynamischer Lauf samt neuer Kursrate,
  Reiterwechsel im Lauf, feste Eingaben, Kraftframes, Checkbox-Haken
  und Standard-Deployment mit lokalen Qt-Plugins pruefen.
- Plan mit tatsaechlichen Testbefehlen, Messwerten, Modellfassungen und
  offenen Punkten fortschreiben. Keine technische Vollstaendigkeit ohne
  Nachweis; fachliche Nutzerabnahme separat.

Alle vier GUI-Anforderungen sind zugeordnet. Umsetzung ist beauftragt;
Commit/Tag/Push sind nicht beauftragt.

#### Tatsaechliche Nachweise der Erweiterung

- Nachkorrektur des Nutzerfalls mit Ruder 24 Grad: modellfeste Kraft
  [150,10,0] N, gemeinsamer Ansatzpunkt [-0,05,0,3] m, Schwert 100 Prozent.
  Kein GUI-Physik- oder Eingabeumbau: Core-Initialisierung korrigiert;
  alle Lasten/Stellungen bleiben konstant. Aus Ruhe jetzt 4 statt 100
  erfolgloser Iterationen, Fahrt ca. 2,124 m/s und Kursdrehrate
  -17,28 Grad/s. Alle sechs Residuen innerhalb 0,05 N/Nm.
  `gui_core_integration` prueft zusaetzlich -24 Grad sowie unveraenderte
  Last, Ansatzpunkt und Stellungen. Die richtige Loesungsbranche wurde
  zuvor unabhaengig numerisch gefunden; keine analytische
  Genauigkeitsbehauptung aus diesem Regressionswert ableiten.
- Nach Neubau von Standard-EXE, Adapter- und Grafiktests:
  **4/4 gezielte GUI-Tests bestanden**, 125,96 s
  (`gui_core_integration`, `gui_workspace_tabs_100/150/200`).
  Screenshotfall in jeder DPI-Stufe mit sichtbarer signierter Rate und
  unveraendertem 24-Grad-Ruder geprueft; Bilder
  `graphics-evidence\workspace-tabs-*\turning_rudder_24.bmp`.
  Core prueft dazu +/-12, +/-24 und +/-36 Grad sowie Bestandsfaelle;
  **6/6** gezielte Coretests nach Neubau bestanden (35,47 s).
  Fruehere Laufzeit-/Iterationswerte unten dokumentieren den damaligen
  Stand, nicht die neue Initialisierung.

- Release-Build der GUI und geaenderten Tests im Standardordner
  `C:\spdeps\guibuild`; MSVC `/W4 /WX`, keine Compilerwarnungen.
- Gezielter CTest mit
  `-R '^gui_(navigation|force_display|core_integration|workspace_tabs_(100|150|200)|boat_inspection_(100|150|200)|model_catalog|display_mesh)$'`:
  **11/11 bestanden**, 141,59 s. Nach ergaenzten expliziten
  Kurvenfahrt-Screenshots und reinem Drehfall die sechs direkt betroffenen
  Tests erneut: **6/6 bestanden**, 90,93 s.
- `gui_core_integration`: gemeinsames importiertes Core-Fixture
  `stationaere_kurvenreferenz`, kompletter Startzustand mit Omega,
  unveraenderte Laufoptionen-Defaults. 2 m/s, r=0,4 rad/s,
  Roll=0,2 rad, Radius=5 m; aeussere Seitenkraft 3200 N und aeusseres
  Kreiselmoment 62,3069347694 Nm, Bewegungsresiduen
  max|F|=1,14e-13 N, max|M|=8,65e-12 Nm. Dies prueft ein bereits
  vorbesetztes Gleichgewicht ohne Widerstand, keine eindeutige Suche
  aus Ruhe. Separater reiner Drehfall r=-0,4 rad/s bestanden;
  Radius bleibt null mit Status `reine_drehung`.
- Zusaetzlich echtes Modell 420 **aus Ruhe**, Kraft [100,0,0] N,
  modellfester Punkt [1,0,0] m, Schwert 100 Prozent, feste Ruder:

  | Ruder | Ergebnis | Kursrate rad/s | Iterationen | gemessene Core-Zeit |
  |---|---|---|---|---|
  | 0 Grad | konvergiert | +0,000132566 | 5 | 2,891 s |
  | +1 Grad | konvergiert | -0,0108649 | 5 | 3,219 s |
  | -1 Grad | konvergiert | +0,0111162 | 5 | 3,699 s |

  Alle sechs Residuen <=0,1 N/Nm, keine Ruderoptimierung. Laufzeiten sind
  Messwerte dieser Maschine, keine zugesicherte Zeitgrenze.
- `gui_workspace_tabs_100/150/200`: tatsaechliche positive/negative
  420-Kurvenloesung ueber Hauptfenster, sichtbare Drehratenzeile,
  Eingabesperre, Dynamik, geradlinige Nichtkonvergenzwarnung und
  Checkbox-Haken bestanden. Screenshots in
  `graphics-evidence\workspace-tabs-*\turning_*_rudder.bmp`.
- `gui_navigation`: +/-pi/2 rad/s -> +/-90 Grad/s, numerische Null,
  Nichtverfuegbarkeit und Naeherungszeichen, Zeilenreihenfolge und
  vergroesserte/verkleinerte Karte. Zusaetzlicher direkter Lauf bei
  `QT_SCALE_FACTOR=1/1.5/2` jeweils bestanden.
- `gui_force_display`: V1/V2/V4/V5 bleiben lesbar; V6-Felder werden
  strukturell validiert. Ungueltige/fehlende Kursrate wird abgewiesen,
  `nicht_bestimmbar` bleibt ohne Zahlenwert. Bewegungssummen/-residuen
  werden getrennt beschriftet.
- Standalone-Pluginpruefung des Hauptfenstertests mit
  `QT_PLUGIN_PATH=C:\spdeps\guibuild\Release` bestanden, ohne installierten
  Entwicklungs-Pluginpfad. Die echte Standard-EXE anschliessend gestartet,
  nach sechs Sekunden responsiv und normal geschlossen.
- Kein globaler Volltest-Erfolg behauptet: Core hat weiterhin die zwei
  bekannten fachfremden Katalog-/Schemafehler, der bekannte breite
  Grafik-Kursspurtest bleibt unabhaengig offen. Fachliche Nutzerabnahme
  und Git-Veroeffentlichung sind nicht erfolgt.

Keine GUI-Parallelphysik. Der statische Reiter bestimmt den Laufmodus.
Unter Lauf stoppen folgen eine Fremdkraft und danach getrennte feste
Ruder-/Schwertstellungen. Core liefert stationaere Lage und Fahrt sowie
die vollstaendige Kraft-/Momentenbilanz.

Bereits im Arbeitsbaum: Modellname ausserhalb der Bereiche auf Reiterhoehe
und Darstellungssteuerung links. Vorhandener Nachweis: sechs
Workspace-/Bootsauswahltests bei 100/150/200 Prozent bestanden.
Dies belegt ausschliesslich die Layoutaenderung, nicht A-G-00079 bis
A-G-00082 oder die neue stationaere Berechnung.

## 2. Arbeitspakete und Abhaengigkeiten

| Schritt | Anforderungen | Umsetzung und erwarteter Nachweis | Voraussetzung | Status |
|---|---|---|---|---|
| U-G-000073 | A-G-00079, A-G-00080, A-G-00081 | Finalen Core-Vertrag mit GUI-Adapter abgleichen; Laufdaten/Ergebniszuordnung und Einheiten explizit festlegen. Bestehende Layoutaenderung pruefen und dokumentieren; keine neuen Exports in der GUI erfinden. | Core U-C-000096 | technisch abgeschlossen; Typed-API und V5-End-to-End getestet |
| U-G-000074 | A-G-00079 | Moduswahl im statischen Reiter entfernen; Startpfad zwingend statisch/stationaer binden. Dynamischen Reiter, laufende Auftraege und Reiterwechsel ohne unbeabsichtigten Start erhalten. | U-G-000073 | technisch abgeschlossen; drei Workspace-DPI-Tests |
| U-G-000075 | A-G-00080 | Eine aktivierbare Fremdkraft unter Lauf stoppen mit modellfestem Angriffspunkt, modell-/weltfestem Vektor und Einheiten anbieten. Direkt darunter je Ruder/Schwert die Core-Stellung mit Einheit und Validierung; fehlende Grenzen explizit behandeln. Eingaben fuer den Lauf einfrieren/sperren; Aenderungen danach als neuer Eingabestand kennzeichnen. | U-G-000073 | technisch abgeschlossen; Workspace-/Adaptertests |
| U-G-000076 | A-G-00079, A-G-00080, A-G-00081 | Adapter und statischen Start an Core-Lieferung binden; Snapshot, Abbruch und Modellfassung erhalten. Lage, Fahrt/Abdrift, sechs Residuen, Toleranzen und Referenzkurs darstellen; Fremdkraft/Hydrodynamik in vorhandene Auswahl und Tabelle integrieren. | U-G-000074, U-G-000075; Core U-C-000099 und gepruefter Solver C-S2 | technisch abgeschlossen; echte Core-Ergebnisse getestet |
| U-G-000077 | A-G-00079, A-G-00080, A-G-00081, A-G-00082 | Adaptertests fuer beide Kraftvektorrahmen, feste Stellungen, unveraenderliche Eingaben, Wechsel dynamisch/statisch und Fehlerstatus. End-to-End mit realem Core: analytisches Fahrtfixture, Nullast, exzentrische Last, verschiedene Elementstellungen, freier Kurs und Abbruch. DPI-/Bootsauswahl-/Dynamikregression, README und Nachweise aktualisieren. | U-G-000076; Core U-C-000100 | technisch abgeschlossen; 10/10 GUI-Integrationstests, ergaenzende Core-Nachweise und Grenzen in Abschnitt 5 |

U-G-000074 und U-G-000075 sind nach U-G-000073 unabhaengig.
Produktiver neuer Solveraufruf erst mit nachgewiesener Core-Funktion;
eine Attrappe oder hydrostatischer Ersatz gilt nicht als Integration.

## 3. Meilensteine

- **G-S1 Vertrag:** Eintritt Core-Vertrag C-S1; Abnahme U-G-000073:
  jeder Eingabe-/Ergebniswert eindeutig zugeordnet, vorhandenes Layout
  regressionsgeprueft, keine offene Bedeutung von Bezugssystemen.
- **G-S2 Bedienung:** Eintritt G-S1; Abnahme U-G-000074/75:
  statischer Modus unabhaengig von alter Dropdown-Auswahl; Reihenfolge
  Laufbuttons -> Fremdkraft -> Ruder-/Schwertstellungen; feste Eingaben,
  Einheiten und fehlende Grenzen getestet.
- **G-S3 Integration:** Eintritt G-S2 und Core C-S3; Abnahme U-G-000076/77:
  echte Core-Ergebnisse mit allen sechs Residuen und ehrlichem Status;
  0,1 N / 0,1 Nm als Obergrenzen, analytischer Fahrtfehler <= 0,1 m/s.
  Engere erreichte Genauigkeit und verwendete Toleranzen anzeigen.
  100/150/200-Prozent-Tests, Bootsauswahl und dynamische Steuerung gruen.
  Keine Gleichsetzung technischer Tests mit persoenlicher Nutzerabnahme.

## 4. Rueckverfolgbarkeit und Nachweise

| Anforderung / Spezifikationsabschnitt | Schritte | Nachweis | Status |
|---|---|---|---|
| A-G-00079 / 2 | U-G-000073, U-G-000074, U-G-000076, U-G-000077, U-G-000083, U-G-000084 | G-S2/G-S3 und neue Verhaltensfreigabe: drei Workspace-DPI-Tests mit statischem/dynamischem Abbruch, Reset, Neustart und Verwerfen alter Ergebnisse | technisch nachgewiesen |
| A-G-00080 / 3 | U-G-000073, U-G-000075, U-G-000076, U-G-000077 | G-S2/G-S3: Reihenfolge, beide Frames, Ruder 20 Grad / Schwert 50 Prozent, Snapshot und Eingabesperre | technisch nachgewiesen |
| A-G-00081 / 4 | U-G-000073, U-G-000076, U-G-000077 | G-S3: echter V5-Kontext mit sechs Residuen, Fahrt, Referenzkurs; Fehler-/Abbruchtests | technisch nachgewiesen |
| A-G-00082 / 5 | U-G-000077 | G-S3: analytische Fahrt ueber GUI-Adapter und drei DPI-Stufen; ergaenzende Core-Last-/Frame-/Flutungsnachweise im C-0.8-Plan | technisch nachgewiesen; Nutzerabnahme offen |
| A-G-00083 / 6.1 | U-G-000078, U-G-000080, U-G-000081 | echte 420-Kurvensuche aus Ruhe, eingefrorene Kraft/Stellungen, deaktivierbarer Altpfad | technisch nachgewiesen |
| A-G-00084 / 6.2 | U-G-000078, U-G-000079, U-G-000081 | Navigationseinheiten, Kursrate bei Schraeglage, V6 dynamisch/stationaer und drei DPI-Layouts | technisch nachgewiesen |
| A-G-00085 / 6.3 | U-G-000078, U-G-000080, U-G-000081 | Kreis mit 3200 N aeusserer Seitenkraft trotz erfuellter Bewegungsbilanz; reiner Drehfall, Nullradiusstatus, Parserfehler/Altvertraege | technisch nachgewiesen |
| A-G-00086 / 6.4 | U-G-000079, U-G-000081 | 11 gezielte GUI-Tests, sechs direkt betroffene nochmals nach letzten Test-Ergaenzungen, Standard-Deployment | technisch nachgewiesen; Nutzerabnahme offen |
| A-G-00087 / 6.5 | U-G-000082, U-G-000084 | Geometrieeinheitstest, echte 420-Kursraten in drei Workspace-DPI-Tests, Navigation aus/an, Reset und Standard-Deployment | technisch nachgewiesen; Nutzerabnahme offen |

## 5. Grenzen und Abschluss

Nachweise im Standard-Build `C:\spdeps\guibuild`:

Die folgenden aelteren 420-Nichtkonvergenznachweise gelten fuer den
geradlinigen Modus mit erzwungener Null-Drehrate. Die freigegebene
Kurvenfahrterweiterung und ihre erfolgreichen Istwerte stehen oben.

- Ergaenzung zum zweiten 420er-Screenshot (Angriff X = 1 m):
  der unveraenderte Fall Ruder 0 / Schwert 100 bleibt bei
  Fy = 0,102321154 N und Mz = 0,128377476 Nm stehen. Eine reine
  Geschwindigkeitsrundung behebt diese Restbilanz nicht.
  Separate diagnostische Rudervariation -0,1 / +0,1 Grad kehrt die
  Vorzeichen der Rest-Seitenkraft und des Rest-Giermoments um.
  Ein unabhaengig vorgegebener Vergleichsfall mit Ruder +0,0114 Grad
  konvergiert dagegen in 6 Iterationen: v = 1,987109512 m/s,
  max|F| = 0,000429064 N, max|M| = 0,000538233 Nm.
  Dieser Winkel ist ein Testeingang fuer das aktuelle Modellinventar,
  keine automatische Stellungsoptimierung und keine Aenderung des
  Nutzerfalls. Keine Aussage, dass fuer Ruder 0 global keine Loesung
  existiert; nachgewiesen ist nur der lokale Suchstillstand.
  Beide festen Lastfaelle sind im Adaptertest enthalten.
  Der Fehlerdialog zeigt jetzt alle sechs Restkomponenten und Toleranzen,
  die Bilanzkarte bezeichnet Fahrt/Lage ausdruecklich als Naeherung.
  4/4 Adapter-/Workspace-Tests bei 100/150/200 Prozent bestanden
  (66,29 s), einschliesslich echter Nichtkonvergenzmeldung.
- Nachpruefung des gemeldeten 420er-Falls: siehe
  [Core-Nachweis](../../CORE/Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md#aktueller-validierungsstand).
  Modellfeste 100 N bei [2,0,0] m, Ruder 0 Grad / Schwert 100 Prozent:
  bisher 100 Iterationen / 15,36 s; nach Korrektur der freien Kursrichtung
  6 Iterationen / 1,99 s bis zur Stagnationsdiagnose. Versuchsfahrt
  1,98711 m/s bleibt wegen Rest-Fy 0,10232 N und Rest-Mz 0,12838 Nm
  ausdruecklich nicht konvergiert. Keine Geschwindigkeitstoleranz als
  Ersatz fuer die Kraft-/Momentenbilanz. Ohne aktivierte Kraft ist derselbe
  Fall in 1,18 s konvergiert.
  Neue Regression prueft beide Faelle, Referenzkurs und begrenzte Iterationen.
  5/5 gezielte Core-/GUI-Tests bestanden (23,29 s).
  Die laufende Standard-EXE war beim Linken gesperrt; der neue Release-Build
  ist unter `C:\spdeps\gui-workspace-tabs\Release\segelphysik_gui.exe`
  erfolgreich erstellt. Keine fremden Prozesse beendet.
- Abschliessender Release-Build erfolgreich; 10/10 gezielte Tests bestanden
  (118,79 s), einschliesslich analytischer Fahrt ueber den Adapter.
- Sieben Tests bestehen (84,40 s): `gui_workspace_tabs_100/150/200`,
  `gui_core_integration`, `gui_model_catalog`, `gui_display_mesh`,
  `gui_force_display`. Die Workspace-Tests starten einen echten stationaeren
  Core-Lauf, pruefen Eingabesperre, V5-Ergebnisanzeige und Markierung
  nachtraeglich geaenderter Eingaben; anschliessend pruefen sie den
  dynamischen Lauf einschliesslich laufendem Reiterwechsel.
- Drei Bootsauswahltests bei 100/150/200 Prozent bestehen (49,49 s).
- Der Adaptertest prueft Nullast, expliziten stationaeren Ergebniskontext,
  fehlerhafte Eingaben und vorab angeforderten Abbruch. Der Parser prueft
  V5 positiv sowie die Zurueckweisung eines fehlenden stationaeren
  Kontextes; die V1/V2/V4-Bestandspruefungen bleiben gruen.
- Die anfangs fehlende V5-Freigabe in der Adapter-Zustandspruefung ist
  behoben. Der erweiterte Adaptertest prueft beide Kraftframes samt
  modellfestem Angriffspunkt, unveraenderliche fruehere Snapshots,
  Ruder 20 Grad / Schwert 50 Prozent und reale Elementtransformationen.
  Nullast, Stellungsfall und analytische Fahrt pruefen alle sechs
  Komponenten explizit gegen 0,1 N beziehungsweise 0,1 Nm.
- Die gemeinsame analytische Core-Fixture unter
  `CORE/Code/tests/testdata/stationaere_referenz` laeuft ueber den
  GUI-Adapter mit 1500 N Welt-X bei Angriff [0,0,-0,4] m.
  Sollfahrt 1,118033989 m/s; der Test prueft Fehler <= 0,1 m/s.
  Core-Nachweis: Ist 1,118035272 m/s, maximale Residuen 0,003443046 N
  und 0,001032914 Nm bei internen Solltoleranzen 0,05 N/Nm.
- Ungueltiger Vektor, unbekannte Elementstellung und stationaere Daten
  im dynamischen Auftrag werden explizit zurueckgewiesen.
  Eine unmoegliche aufwaerts gerichtete Last von 1e8 N liefert
  Nichtkonvergenz mit Diagnose statt Erfolg; Nutzerabbruch bleibt erhalten.
- Die drei abschliessenden Workspace-Tests bestehen auch nach Umstellung
  der Normalwerte/Einheiten auf Core-Metadaten (42,70 s).
  Fehlende Metadaten sperren den Lauf mit Diagnose, fehlende physikalische
  Grenzen werden kenntlich gemacht. Ergebnisstatus und Referenzkurs stehen
  in einer scrollbar erreichbaren Bilanzkarte; stationaere Iterationen
  werden nicht als physikalischer Zeitverlauf bezeichnet.
- Ergaenzende Core-Abnahme: 4/4 gezielte Tests, Build erfolgreich.
  Galileiverschiebung aendert relative Fahrt um 1,281878563e-6 m/s;
  90-Grad-Framewechsel, exzentrisches Moment [0,450,0] Nm,
  mehrere Stellungen und eine Flutungswiederholung sind nachgewiesen.
  Vollstaendiger Core-Lauf 18/20: `modellkatalog_schnittstelle`
  erwartet eine fehlende Quader-Inventardatei; `modell_geometrie_v2`
  belaesst bei Schema-2-Konvertierung das Schema-3-Feld `ableitung`.
  Ursachen und Reproduktion im einzigen C-0.8-Plan; keine
  durch den Stationaersolver verursachten Fehler.
  Der bekannte allgemeine Grafiktest zur exakten Wiederherstellung der
  420-Kursspur ist nicht Teil dieser gezielten Abnahme und bleibt offen.

G-S1 und G-S2 sind technisch nachgewiesen. G-S3 ist fuer den beauftragten
stationaeren Vertrag gezielt nachgewiesen; keine Behauptung einer fehlerfreien
Gesamtsuite oder persoenlichen Nutzerabnahme. U-G-000073 bis U-G-000077
sind implementiert und durch die obigen Nachweise technisch abgeschlossen.

Keine Windphysik, Ruderoptimierung, automatische Kreisfahrt oder
Geschwindigkeitsbegrenzung als Bilanzersatz. Vor fachlichen
Vertragserweiterungen Nutzerentscheidung einholen. Bei fehlender Loesung
oder nicht bestimmtem Kurs keinen Erfolg vortaeuschen.

Alle aktiven Anforderungen vor Abschluss gegen Tests und Core-Lieferung
abgleichen; offene Defekte und Nutzerabnahme sichtbar lassen. Commit,
Push, Tag und GitHub-Release benoetigen einen separaten Auftrag.
