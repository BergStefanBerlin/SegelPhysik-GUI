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
