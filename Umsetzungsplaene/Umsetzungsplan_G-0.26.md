# Umsetzungsplan G-0.26: Bewegungs- und Kraftdiagnose

**Stand: 20.11.2026 · Status: GUI-Implementierung begonnen; Integration und
Nutzerabnahme offen.** Dieser Plan gehoert zur bestehenden
[Kraft- und Momenten-Spezifikation](../Spezifikationen/Spezifikation_GUI_Kraft_und_Momentendarstellung.md),
Abschnitt 4, und ihren offenen
[Entwurfspunkten](../Spezifikationen/Entwurfspunkte/GUI_Kraft_und_Momentendarstellung.json).
Die Anforderungen A-G-00173 bis A-G-00177 werden mit U-G-000185 bis
U-G-000191 umgesetzt. Dies ist der einzige Umsetzungsplan fuer G-0.26.
Die fachliche Finalisierung ist keine Git-/Release-Freigabe.

## Ziel

Eine ComboBox soll zwischen der bisherigen freien Vektor-/Momentenauswahl
und einer neuen, fachlich zusammenhaengenden Bewegungsanalyse umschalten.
Die GUI visualisiert Core-Zustaende, Bezugssysteme und Kraft-/Momenten-
beitraege; sie fuehrt keine neue Physik ein. Die bestehende freie Ansicht
und ihre Einstellungen bleiben erhalten.

## Umfang und Voraussetzungen

- **Frei (wie bisher)** bleibt der Standard und verhaelt sich wie vor der
  Erweiterung. Die ComboBox ist keine neue Physikoption.
- **Bewegungsanalyse** zeigt nur Werte, Bezuege und Gueltigkeitsstatus,
  die zum gleichen Core-Ergebniszustand gehoeren.
- Zu Beginn die vorhandenen statischen und dynamischen Datenvertraege fuer
  Weltgeschwindigkeit, modellbezogene Winkelgeschwindigkeit,
  Kurvenfahrtstatus, Kraefte, Momente um S und LDP/SDP verifizieren.
- Die Bewegungsanalyse gilt fuer statische und dynamische Arbeitsbereiche.
  Die Modusauswahl wird je Arbeitsbereich getrennt gespeichert; ein
  fehlender Moduswert startet mit „Frei“. Die bisherige freie
  Vektor-/Momentenauswahl bleibt nach ihrem bestehenden Einstellungsvertrag
  unveraendert.
- Fuer die stationaere Kurvenfahrt die Einschraenkung auf eine Drehung um
  die Weltvertikale sichtbar erhalten. Modellkomponenten dieses Vektors
  sind keine unabhaengigen Euler- oder Bootsdrehraten.
- Schwerpunktbeschleunigung und Winkelbeschleunigung gehoeren zum
  Anzeigeziel, muessen aber aus einem Core-Ergebnis desselben Zustands
  stammen. Der aktuelle Ergebnisvertrag serialisiert sie nicht. Vor den
  abhaengigen GUI-Arbeiten ist daher ein separates, freigegebenes
  Core-Spezifikations-/Vertragspaket erforderlich; dieser GUI-Plan aendert
  CORE nicht. Ohne dieses Gate werden die Werte als nicht verfuegbar
  angezeigt und nicht aus Zeitreihen, Kraeften oder Momenten approximiert.
- Die allgemeine 3D-Schraubachse und Schraubsteigung duerfen nur aus
  gleichzeitig gueltiger Schwerpunktgeschwindigkeit und
  Winkelgeschwindigkeit im gemeinsamen Bezugssystem abgeleitet werden.
- Die GUI erfindet keine Achsen, Angriffspunkte oder Kraefte. Bewegungen
  einzelner Modellgruppen folgen ausschliesslich den Definitionen aus
  Modell-JSON und Core.

## Externe Abhaengigkeit: Core-Beschleunigungsdaten

Der GUI-Ergebnisvertrag enthaelt aktuell `geschwindigkeit_welt_m_s` und
`omega_modell_rad_s`, aber keine Schwerpunkt- oder
Winkelbeschleunigung (siehe [Core-Serializer](../../CORE/Code/src/simulation.cpp)).
Die Codebasis berechnet Beschleunigungen intern in Teilen des dynamischen
Solvers, serialisiert sie jedoch nicht im GUI-Ergebniszustand. Vor
U-G-000188 muss das Core-Projekt die noetigen
Beschleunigungsvektoren, Bezugsrahmen, Einheiten, Gueltigkeitsstatus und
Snapshot-Zuordnung in einer eigenen freigegebenen Spezifikation und einem
Ergebnisvertrag festlegen, implementieren und testen. Dieser GUI-Plan
beauftragt keine Core-Aenderung.

U-G-000188 ist bis zur bestaetigten Core-Lieferung blockiert; bis dahin
zeigt das GUI die Beschleunigungswerte als nicht verfuegbar. U-G-000191
darf erst nach bestandener Core-Integration und Ergebnisparitaet
abgeschlossen werden. Ersatzberechnungen aus Zeitreihen oder aus
Kraft-/Momentenwerten sind ausgeschlossen.

## Umsetzungsschritte und Abhaengigkeiten

| U-ID | Paket | Anforderungen | Abhaengigkeit | Fertigkriterium |
|---|---|---|---|---|
| U-G-000185 | GUI/Core-Datenvertrag pruefen | A-G-00174 bis A-G-00177 | Keine | Statische und dynamische Snapshots, Quellenbeitraege, Momente um S, Winkelgroessen, Status und Bezuege inventarisiert. Fehlende Beschleunigungsfelder samt benoetigtem Core-Gate sind dokumentiert. |
| U-G-000186 | ComboBox und Persistenz | A-G-00173 | Keine | ComboBox schaltet zwischen Frei und Bewegungsanalyse; Moduswahl wird statisch/dynamisch getrennt gespeichert. Erststart ist Frei. Freie Einzelauswahl bleibt nach Hin- und Rueckwechsel unveraendert. |
| U-G-000187 | Kinematik und 3D-Schraubachse | A-G-00174, A-G-00175, A-G-00177 | U-G-000185 | Geschwindigkeit und Winkelgeschwindigkeit werden korrekt transformiert. Schraubachse und Steigung stimmen fuer gueltige Faelle; ungueltige Werte werden begruendet ausgelassen. |
| U-G-000188 | Beschleunigungsanzeige anbinden | A-G-00174, A-G-00177 | U-G-000185; bestaetigte Core-Beschleunigungsdaten als externe Voraussetzung | Schwerpunkt- und Winkelbeschleunigung stammen aus demselben Core-Snapshot und zeigen korrekte Frames, Einheiten und Status. Keine Differenzbildung in der GUI. |
| U-G-000189 | Quellenbezogene Kraft-/Momentenbilanz | A-G-00176, A-G-00177 | U-G-000185 | Kraefte, Momente um S und LDP/SDP entsprechen dem Core-Snapshot. Einzelbeitraege und Summen sind unterscheidbar und werden nicht doppelt gezaehlt. |
| U-G-000190 | Ansichten integrieren | A-G-00173 bis A-G-00177 | U-G-000186 bis U-G-000189 | Beide Modi funktionieren in statischem und dynamischem Arbeitsbereich; Live, Pause, Wiedergabe und Modellwechsel zeigen konsistente Snapshots, ohne Simulation oder freie Einstellungen zu veraendern. |
| U-G-000191 | Regression, Release-Build und Sichtabnahme | A-G-00173 bis A-G-00177 | U-G-000190; Core-Integration fuer Beschleunigungsabnahme | Tests und Build bestehen; synthetische Kinematik-, Schraubachsen- und Lastfaelle sowie 100/150/200-%-DPI-Sichtpruefung sind dokumentiert. Bestehende freie Kraft-/Momenten-, Wind- und Druckpunktdarstellung bleibt regressionsfrei. |

## Tatsaechlicher Umsetzungsstand

| U-ID | Stand | Nachweis und offen |
|---|---|---|
| U-G-000185 | Abgeschlossen | Statische/dynamische Ergebnisfelder und Bezuege wurden geprueft. Der Core-Ergebnisvertrag liefert Weltgeschwindigkeit und modellbezogene Winkelgeschwindigkeit, aber keine Schwerpunkt- oder Winkelbeschleunigung; das externe Gate bleibt dokumentiert. |
| U-G-000186 | Implementiert | ComboBox, getrennte Persistenz und Erhalt der freien Auswahl sind integriert. Die neuen MainWindow-Moduspruefungen liefen im fokussierten GUI-Test vor einem spaeteren, unabhaengigen Navigations-Layoutfehler durch. |
| U-G-000187 | Implementiert, Sichtabnahme offen | Frame-Transformation und Schraubachsenberechnung sind integriert; `gui_force_display` besteht mit 776 Checks. Der Release-Build des GUI-Targets ist erfolgreich; 3D-Overlays und DPI-Stufen sind noch nicht visuell abgenommen. |
| U-G-000188 | Blockiert | Core-Beschleunigungsfelder, Bezugsrahmen, Einheiten, Status und Snapshot-Zuordnung fehlen weiterhin. Die GUI weist die Werte als nicht verfuegbar aus und berechnet sie nicht selbst. |
| U-G-000189 | Implementiert, Integration offen | Quellenkraefte und -momente sowie die bestehende Druckpunkt-/Wertetabelle werden fuer die Analyseansicht eingebunden. Der vollstaendige MainWindow-Integrationslauf ist vor der Analyse-Label-Pruefung an der bestehenden Navigations-Layoutpruefung abgebrochen. |
| U-G-000190 | Teilweise integriert | Modus und Analyseausgabe sind in die Arbeitsbereiche eingebunden. Dynamische Live-/Pause-/Wiedergabe- und Modellwechsel-Regressionspruefung ist noch offen. |
| U-G-000191 | Offen | Der Release-Build sowie `gui_force_display` und `gui_graphics_settings` bestehen. Der vollstaendige GUI-Testlauf und die Sichtabnahme sind wegen vorhandener Fixture-/Navigationsfehler nicht abgeschlossen. |

Der konfigurierte CTest-Modellpfad `Modelle_alt` kann das Referenzmodell nicht
laden. Mit dem aktuellen Modellordner erreicht der fokussierte MainWindow-Test
die bestehende Layoutpruefung, die eine vertikale Anordnung von Kursrate und
Geschwindigkeitsbetrag erwartet, waehrend die GUI beide in einer Zeile
platziert. Dieser unabhaengige Navigationsfehler wurde nicht geaendert. Weitere
bekannte, hier nicht behobene GUI-Testfehler betreffen die Windreferenz-
Sichtbarkeit, eine fehlende 420-Fixture und die Core-Ablehnung des
Referenzmodells.

## Nachweis und Abnahme

Nach Abschluss der jeweiligen Arbeitsschritte den GUI-Release-Build und
die fokussierten sowie relevanten bestehenden CTest-Tests ausfuehren.
Die Sichtabnahme an 420, Folkeboot und Optimist fuer beide Modi, beide
Arbeitsbereiche und die DPI-Stufen 100/150/200 % dokumentieren.

Synthetische Kinematikfaelle umfassen reine Translation, reine Rotation,
Schraubbewegung, kombinierte Bewegung, gekraengte Pose, Nullwinkel-
geschwindigkeit sowie ungueltige und fehlende Werte. Beschleunigung und
Winkelbeschleunigung sind sowohl mit gueltigen Core-Snapshots als auch
mit fehlendem/ungueltigem Status zu pruefen; GUI-seitige Differenzen
zwischen Zeitschritten gelten nicht als Ersatztest.

Kraft-/Momentenfaelle umfassen Einzelquelle, Summe, reines Moment,
Kraftangriffspunkt mit Hebelarm, LDP/SDP und einen nicht verfuegbaren
Angriffspunkt. Sichtbarkeit und ComboBox duerfen weder Core-Aufruf noch
physikalisches Ergebnis veraendern. Eine GUI-Fixture prueft die
Darstellung, ersetzt jedoch keine reale Core-Integration. Die fehlenden
Core-Beschleunigungsfelder muessen bis zur Abnahme der jeweiligen
GUI-Anforderung durch einen getrennten Core-Beleg geschlossen sein.

Lokale Validierung nach der Implementierung:

```powershell
cmake --build C:\spdeps\guibuild --config Release
ctest --test-dir C:\spdeps\guibuild -C Release --output-on-failure
```

## Abschlusskriterien

- A-G-00173 bis A-G-00177 sind den U-G-000185 bis U-G-000191 und
  tatsaechlichen Testergebnissen zugeordnet.
- Akzeptanztests, Sichtpruefung, bekannte Einschraenkungen und gegebenenfalls
  fehlende Core-Daten sind mit Belegen dokumentiert.
- Das externe Core-Gate fuer Beschleunigung und Winkelbeschleunigung ist
  durch eine getrennte Spezifikation, einen bestaetigten Ergebnisvertrag
  und reale Integration geschlossen. Ohne diese Belege bleiben die
  abhaengigen GUI-Arbeiten offen.
- Abweichungen zwischen spezifiziertem und implementiertem Umfang bleiben
  sichtbar; Umsetzungs-, Test- und Abnahmestatus werden nicht gleichgesetzt.
- Git-Freigaben folgen separat und ausschliesslich der Release-Instruction.
