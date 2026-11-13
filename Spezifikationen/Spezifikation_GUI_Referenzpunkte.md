# Spezifikation: GUI-Verwendung gemeinsamer Referenzpunkte

**Stand:** 13.11.2026 · **Status:** Fachlich freigegeben; Umsetzung, Tests
und Abnahme offen. Maßgebliche fachliche Quelle ist der
[Core-Referenzpunktvertrag](../../CORE/Spezifikationen/Spezifikation_Referenzpunkte.md);
die Ausführung steht im [Plan G-0.23](../Umsetzungsplaene/Umsetzungsplan_G-0.23.md).
Die Umsetzung ist ausdrücklich beauftragt.

## A-G-00165 – Bestehende Anzeigen aus dem gemeinsamen Vertrag

Die GUI verwendet die vom Core übergebene Referenzpunkt-Sammlung für
Punkte, die sie bereits benötigt, einschließlich Mastspitze, dargestellten
Kraftangriffspunkten, Lateraldruckpunkt und Ersatz-SDP. Sie übernimmt
Identität, Bezugssysteme, Status und Ergebnisstand; sie erfindet keine
Punkte und ersetzt fehlende Werte nicht durch Null. Bestehende
Darstellungen, Beschriftungen, Schalter und fachliche Bedeutungen bleiben
gemäß ihren jeweiligen Spezifikationen erhalten.

Die Sammlung kann zusätzlich vom Core gelieferte Positionen und
Geschwindigkeiten enthalten. Die GUI muss **nur die von ihr benötigten
Werte darstellen**; zusätzliche Punktarten oder Geschwindigkeitswerte
erfordern jetzt keine neuen Anzeigen. Ein fehlender Geschwindigkeitswert
darf eine vorhandene Positionsanzeige nicht unnötig unterdrücken.

Liveansicht und Wiedergabe nutzen den zugehörigen Ergebnisstand.
Bestehende historische Ergebnisse bleiben gemäß bisherigem Vertrag lesbar;
fehlen neue Angaben, werden sie nicht aus aktuellen Daten ergänzt.
Die fachliche Berechnung der Punkte und ihrer Bewegung bleibt im Core.

**Abnahme:** Mastspitzen-, Kraft-, LDP- und Ersatz-SDP-Anzeigen verwenden
die korrekten Core-Punkte und behalten ihr bisheriges Verhalten.
Fehlender Punkt bzw. nicht verfügbare Geschwindigkeit werden unabhängig
behandelt; zusätzliche Sammlungseinträge erzeugen keine neuen Pflichtanzeigen.
Live- und historische Ergebnisse bleiben konsistent.
