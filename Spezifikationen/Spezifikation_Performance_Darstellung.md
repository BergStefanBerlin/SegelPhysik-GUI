# GUI: Performance ohne unnoetige Darstellungsaufbereitung

**Status:** Finalisiert und zur Umsetzung freigegeben am 03.11.2026,
20:45 Uhr. Versionsschritt G-0.11 bestaetigt.
Einzige Spezifikation dieses Optimierungsthemas.
[Entwurfspunkteliste](Entwurfspunkte/Performance_Darstellung.json).
Arbeitszielversion: G-0.11, ohne Git-/Release-Auftrag.
Einziger zugehoeriger [Plan](../Umsetzungsplaene/Umsetzungsplan_G-0.11.md).

## 1. Ziel und Umfang

Die vier Ansichten bleiben erhalten und optional abschaltbar.
Nicht die Vieransicht als Funktion optimieren oder entfernen, sondern
unnuetze Arbeit an abgeschalteten Ansichten und wiederholte unveraenderte
Profil-/Geometrieaufbereitung verhindern. Fuer statischen und dynamischen
Workspace gleichermassen, mit und ohne einfache Darstellungen.

Grundlage sind [GUI-Befunde G-0.10, 13.7-13.9](../Umsetzungsplaene/Umsetzungsplan_G-0.10.md#137-vollstaendiges-gui-diff-inventar-und-ursachenbefunde).
Physik-/Modellaufbereitung gehoert in die
[Core-Spezifikation](../../CORE/Spezifikationen/Spezifikation_Performance_Modellaufbereitung.md).
Hier keine Kraefte, Integration, Physikmeshes oder statischen
Solververfahren aendern. Keine neue Kamera, Bedienung oder Darstellung,
kein Weglassen von Tabellenwerten, Historie oder Core-Schritten.
Tabellen-/Menuebereinigung und allgemeine Katalogoptimierung sind keine
eigenen Arbeitspakete dieses Plans.

## 2. Abgeschaltete Ansichten

**A-G-00130:** Abgeschaltete Ansichten ohne laufende Szenenarbeit und
mit korrekter Reaktivierung gemaess diesem Abschnitt.

Ist die Zusatzansicht durch die vorhandene Option abgeschaltet,
duerfen ihre Szenen keine laufende `setState`-Geometrieaufbereitung,
Profilanwendung, Wasser-/Kraft-/Navigationsaktualisierung und keine
Renderaufrufe ausloesen. Vorhandene Actors duerfen erhalten bleiben;
pro Snapshot ist hoechstens ein Verweis/eine Markierung auf den
neuesten darzustellenden Zustand noetig.

Keine Ausnahmen fuer indirekte Timer-, Callback- oder Renderpfade.
Der Core und seine vollstaendige Historie laufen unveraendert weiter.
Ein-/Ausschalten und Reiterwechsel bleiben bedienbar.
Die freie aktive Ansicht darf nicht durch einen falsch gesetzten
Sichtbarkeitstest stillgelegt werden.

Bei erneutem Einschalten genau den aktuell ausgewaehlten Snapshot mit
aktuellen moduslokalen Optionen, Profilen, Kamera und Modellfassung
herstellen, nicht alle ausgelassenen Zwischenbilder nachrendern.
Im Rueckblick ist der gewaehlte historische Zustand massgeblich, nicht
automatisch der neueste Core-Zustand. Modellwechsel waehrend AUS muss
alte Szenendaten vor Wiederanzeige ungueltig machen.

## 3. Unveraenderte Profile und Renderanforderungen

**A-G-00131:** Idempotente effektive Profile und keine redundanten
profilbedingten Render gemaess diesem Abschnitt.

Profile nach ihrem effektiven Ergebnis vergleichen, einschliesslich
orthografischer Festwerte, Transparenz und ThroughBody.
Bei gleichem effektivem Profil kein Styling-/Pipeline-Neuaufbau und
kein dadurch ausgeloester Render. Fuer abgeschaltete Ansichten nur
das spaeter benoetigte Profil vormerken.

Zustandsdarstellung und Live-Timer duerfen nicht dieselbe unveraenderte
Profilrunde mehrfach rendern. Notwendige sichtbare Aktualisierungen
pro Ereignis buendeln; Modellladen, Kamera-, Auswahl-, Profil- oder
Fensteraenderungen muessen trotzdem ein aktuelles Bild erzeugen.
Kein Wechsel zu einer neuen Renderarchitektur allein fuer diesen Auftrag.

## 4. Gemeinsame und ansichtsabhaengige Geometrie

**A-G-00132:** Exakte gemeinsame Modell-/Stellungsaufbereitung ohne
ungueltige ansichtsuebergreifende Wiederverwendung.

Unveraenderliche Meshgrunddaten und geeignete lokale Vorarbeiten je
Modell-/Stellungsfassung wiederverwenden statt fuer jede Ansicht und
jeden Snapshot erneut tief kopieren oder identisch transformieren.
Veroeffentlichte gemeinsam gelesene Daten nicht nachtraeglich mutieren;
Modellwechsel und Core-Stellungsereignisse muessen korrekt invalidieren.
Komplexe Meshes exakt erhalten, kein LOD oder reduzierter Physikumfang.

Kamera, Projektion, ansichtsspezifische Ausschnitte, Actors und
veraenderliche VTK-Pipelines bleiben getrennt, soweit ihre Semantik es
erfordert. Pose-/wasserlinienabhaengige Schnitte nur bei gleichen
wirksamen Eingaben teilen. Nicht blind ein aus einer Ansicht
abgeleitetes Ergebnis in eine andere uebernehmen.
Keine Neuentwicklung der vier Kameras oder ihrer Anordnung.

## 5. Nachweise und Abnahme

**A-G-00133:** Aufruf-, numerische, visuelle und Performance-Nachweise
gemaess diesem Abschnitt; Ergebnisse statt Beschleunigungsversprechen.

| Fall | Zu pruefendes Ergebnis |
|---|---|
| Zusatzansichten AUS, mehrere neue Snapshots und Live-Ticks | Null State-Aufbereitungen, Profil-Neuaufbauten und Renderaufrufe in den abgeschalteten Szenen |
| EIN nach AUS | Aktueller gewaehlter Snapshot einmal vollstaendig hergestellt, keine Zwischenbild-Nachholschleife |
| AUS mit Modell-/Stellungs-/Profilwechsel | Keine veralteten Meshes, Profile oder Szenen beim Einschalten |
| Wiederholtes unveraendertes effektives Profil | Null profilbedingte Neuaufbauten/Render |
| Geaendertes Profil/Kamera/Fenster | Notwendige Aktualisierung weiterhin sichtbar |
| Modell/Konfiguration unveraendert | Keine erneute tiefe Meshvorbereitung je Ansicht/Snapshot; notwendige Posearbeit bleibt |
| Statik/Dynamik und Reiterwechsel | Moduslokale Darstellung, gemeinsame physische Eingaben, Stopverhalten und Rueckblick unveraendert |

Zaehlerlaeufe von Release-Timing trennen. Messung vor/nach bei gleichem
420/Folkeboot, Snapshots, Kamera, DPI und Profil; ein Warmup plus mindestens
fuenf Wiederholungen mit Rohwerten/Median/Min/Max. Zusatzansichten EIN/AUS
und einfache Darstellungen EIN/AUS jeweils in beiden Workspaces.
EIN dient als Regression, nicht als Auftrag zum Umbau der Vieransicht.

An den bestehenden eingefrorenen Snapshots aus G-0.10 anschliessen.
Ergaenzend einen echten dynamischen Live-Lauf pruefen; Core-Zeiten,
JSON und GUI-Zeiten getrennt messen. Vorbereitungszeit, `displayState`,
Profilrunden und Renderaufrufe ausweisen, keine ueberlappenden Zeiten
addieren. Reihenfolge variieren, Streuung offenlegen.

Numerische GUI-Tabellen und ausgegebene Core-Zustaende unveraendert;
sichtbare Inhalte, Wasserlinien, Kraft-/Druckpunkte und Kamera per
bestehenden Grafik-/Workspace-Tests und gezielten Bildbelegen pruefen.
Fuer Performance keine Renderqualitaet oder fachliche Toleranz senken.
Keinen Beschleunigungsfaktor zusagen: strukturelle Null-Aufruf-Kriterien
und tatsaechliche Laufzeitverbesserung getrennt belegen.

## 6. Freigabegate

Finalisierung und Umsetzung sind am 03.11.2026, 20:45 Uhr freigegeben.
IDEE-G-00085 bis IDEE-G-00088 sind in der Entwurfspunkteliste eindeutig
A-G-00130 bis A-G-00133 zugeordnet; Plan U-G-000140 bis U-G-000144.
Offene fachliche Physiknachweise aus vorherigen Arbeiten bleiben offen.
Die Freigabe erlaubt Codeumsetzung, aber keinen Commit, Push, Tag oder Release.
