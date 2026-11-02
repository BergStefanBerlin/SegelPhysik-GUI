# Spezifikation GUI: Darstellung des Lateraldruckpunkts

**Status:** Fachlich finalisiert am 28.10.2026 (Anforderungen A-G-00052 bis A-G-00056). Umsetzung und Abnahme sind offen. Keine Commit-, Push-, Tag- oder Release-Freigabe.  
**Zielversion:** G-0.7 (laufender Versionsschritt) · **Umsetzungsplan:** [Umsetzungsplan G-0.7](../Umsetzungsplaene/Umsetzungsplan_G-0.7_Ruder_Schwertsteuerung.md), Teil B  
**Maßgeblicher Core-Vertrag:** [Spezifikation Lateraldruckpunkt](../../CORE/Spezifikationen/Spezifikation_Lateraldruckpunkt.md) (A-C-00093 bis A-C-00097)  
**Herkunft:** [Entwurfspunkteliste](Entwurfspunkte/GUI_Lateraldruckpunkt.json) (IDEE-G-00029 bis IDEE-G-00033)

Die GUI zeichnet den vom Core gelieferten Lateraldruckpunkt (CLR). Sie
berechnet ihn nicht, leitet ihn nicht aus Kraftvektoren ab und verwendet
keine Geometrieheuristiken. Der Segeldruckpunkt ist nicht Teil dieses
Umfangs; die Darstellung wird aber so angelegt, dass er später ergänzt
werden kann.

## 1. Zuständigkeit

| Bereich | Zuständigkeit |
|---|---|
| Core | Berechnung, Status, Position (`druckpunkte.lateral`, in Metern) |
| GUI | Lesen des Feldes, Marker, Beschriftung, Ein-/Ausblenden, spätere Umrechnung z. B. in Prozent der Wasserlinienlänge |

## 2. Anforderungen

### A-G-00052: Datenübernahme

*Herkunft: IDEE-G-00029.*

Der Adapter liest `druckpunkte.lateral` aus dem Core-Ergebnis (live und
Wiedergabe). Fehlt das Feld (Altstand, keine Hydrodynamik), erscheint kein
Marker und es entsteht kein Fehler.

**Abnahme:** Test mit und ohne Feld; Altergebnisse laden unverändert.

### A-G-00053: Darstellung

*Herkunft: IDEE-G-00030.*

- Senkrechter Strich in der Mittschiffsebene an der vom Core gelieferten
  Längsposition x, Höhe um die Wasserlinie.
- Eigene Farbe, deutlich von Kraftvektoren und Wasserlinie unterscheidbar;
  Kurzbeschriftung „LDP“, keine Zahlenanzeige.
- Der Marker folgt der Core-Pose des Bootes (Modellrahmen → Welt).

**Abnahme:** Lage des Markers entspricht der Core-Position bei gekrängtem und
bewegtem Boot; Sichtprüfung durch den Nutzer.

### A-G-00054: Status

*Herkunft: IDEE-G-00031.*

- `gueltig`: Marker sichtbar.
- `nicht_bestimmbar` oder Feld fehlt: Marker ausgeblendet, ohne erfundene
  Position.

**Abnahme:** Tests für beide Status und das fehlende Feld.

### A-G-00055: Bedienung

*Herkunft: IDEE-G-00032.*

Ein- und Ausblenden über eine Schaltfläche im vorhandenen Overlay der
Kraftdarstellung. Standard: **an**. Die Einstellung wird wie die anderen
Anzeigeoptionen gehalten.

**Abnahme:** Test der Schaltfläche und des Standardzustands.

### A-G-00056: Nachweise

*Herkunft: IDEE-G-00033.*

- Tests: Marker an Core-Position, ausgeblendet bei `nicht_bestimmbar` und
  fehlendem Feld, Schaltfläche, Live-Lauf und Wiedergabe.
- Bestehende GUI-Tests bleiben unverändert grün.
- Integrierter GUI/Core-Lauf und Nutzer-Sichtprüfung als getrennte Nachweise.

**Abnahme:** Alle genannten Tests und der integrierte Lauf bestehen.

## 3. Entscheidungen zur Finalisierung

- Zielversion: laufender Versionsschritt G-0.7 (Nutzer, 28.10.2026).
- Standard: Marker an, keine Zahlenanzeige (Nutzer).
- Prozentdarstellung und Umrechnung folgen später in der GUI.

## 4. Abhängigkeit

Die Umsetzung setzt voraus, dass der Core das Feld liefert. Ohne den Core
bleibt die GUI ohne Marker und läuft unverändert.

## 5. LDP und Ersatz-SDP als Punkte (02.11.2026)

**Status:** am 02.11.2026 um 19:00 Uhr durch Nutzerauftrag finalisiert
und zur Umsetzung freigegeben. Historische Abschnitte 1-4 nicht
rueckwirkend umschreiben. Entwurfspunkte IDEE-G-00056 und IDEE-G-00057
in der [bestehenden Liste](Entwurfspunkte/GUI_Lateraldruckpunkt.json).
Zuordnung: laufender Schritt G-0.9 und dessen
[einziger Plan](../Umsetzungsplaene/Umsetzungsplan_G-0.9.md).
Dieser Umfang ersetzt die alte Strichdarstellung. Neue Anforderungen:
A-G-00088 (Abschnitt 5.1) und A-G-00089 (Abschnitte 5.2/5.3).
Die alten Bedienvertraege bleiben als Historie erkennbar.

### 5.1 Zwei beschriftete Kraftangriffspunkte

**A-G-00088**, Herkunft IDEE-G-00056.

- Bisherige senkrechte LDP-Linie vollstaendig entfernen.
- LDP als kompakten magentafarbenen Punkt exakt an
  `druckpunkte.lateral.position_modell_m` zeichnen; Position X/Y/Z
  vollstaendig aus dem Core uebernehmen, keine Projektion auf Wasser
  oder Rumpf. Die Pose transformiert den Punkt Modell -> Welt.
- Ersatz-SDP als zweiten, klar unterscheidbaren orangefarbenen Punkt
  am von Core gelieferten Ansatzpunkt der Fremdkraft zeichnen.
  Beschriftung `SDP (Ersatz)`, Herkunft und aktive Ersatzlast im
  Tooltip/Detailbereich erklaeren. Kein echter aerodynamischer SDP.
- Beide Marker mit gut lesbarer Beschriftung und weitgehend
  bildschirmkonstanter Punktgroesse; 100/150/200 Prozent testen.
  Unterwasserpunkt muss in der Analyseansicht erkennbar bleiben.
  Kein an die Wasserlinie versetztes LDP-Schild als anderer Ort.
- Die bestehende Druckpunktbedienung erweitern: `Druckpunkte`
  mit getrennten Schaltern `LDP` und `SDP (Ersatz)`, standardmaessig
  an. Sichtbarkeit unabhaengig von Kraft-/Momentenpfeilen und
  Navigationsinstrumenten; Einstellungen dauerhaft speichern.
  Navigation oder Richtungspfeil sind kein Ersatz fuer diese Schalter.

Keine Verbindungslinie und keine automatische Luv-/Leegierigkeitsdiagnose
im beauftragten Umfang: zwei Punkte statt der bisherigen Linie.
Die Gleichheit ihrer Laengspositionen ist eine Anschauung, kein
Konvergenztest oder Auftrag zur Veraenderung von Kraeften/Stellungen.

### 5.2 Verbindliche Datenquelle und Zustaende

**A-G-00089**, Herkunft IDEE-G-00057.

Abgestimmter Vertrag im
[Core-Nachtrag Abschnitt 5](../../CORE/Spezifikationen/Spezifikation_Lateraldruckpunkt.md#5-c-08-korrekter-ldp-und-ersatz-sdp).
Die GUI berechnet weder LDP noch SDP und liest keine physikalischen
Mesh-/Modelldaten parallel zum Core.

Kraftbasierter LDP nur bei gueltigem Core-Status; geometrischer Ersatz
wird nicht unter der Beschriftung LDP gezeigt. Fehlend/nicht bestimmbar:
Marker aus, Grund im Detailbereich sichtbar. Ein Punkt ausserhalb des
Boots ist zulaessig und darf nicht stillschweigend geklemmt werden.
Alte Ergebnisdaten ohne Kennzeichnung der korrigierten Auswertung
bleiben lesbar, gelten aber nicht als nachgewiesen korrigierter LDP.

Ersatz-SDP nur aus dem eingefrorenen Ergebnis der aktiven Fremdkraft;
inaktiv/null/fehlender Kontext: aus mit nachvollziehbarem Grund.
Nach Eingabeaenderung nicht den alten LDP mit einem neuen Eingabe-SDP
kombinieren. Bestehende Veraltet-/Naeherungskennzeichnung erhalten.
Beim Reiterreset und Modellwechsel beide Marker entfernen; spaete
Resultate abgebrochener Laeufe duerfen keinen Marker wiederherstellen.
In dynamischen Zustaenden nur tatsaechlich gelieferte Punkte zeigen;
keine stationaere Fremdkraft in die Dynamik erfinden.

### 5.3 Abnahme

Gemeinsame Core-Referenzen mit exakter LDP- und Ersatz-SDP-Position,
gedrehte/gekraengte/verschobene Pose, beide Kraftframes, Null/inaktiv,
Gegenkraefte mit zulaessigem Aussenpunkt, unbestimmbar/geometrisch/alt,
Naeherung/veraltet, Einzelschalter und persistenter Neustart.
Echte 420er-Ergebnisse und Bildnachweise bei 100/150/200 Prozent.
Keine alte senkrechte LDP-Linie mehr; keine Physikaenderung durch
Marker oder Grafikeinstellungen. Technische Tests und persoenliche
Sichtabnahme getrennt ausweisen.
