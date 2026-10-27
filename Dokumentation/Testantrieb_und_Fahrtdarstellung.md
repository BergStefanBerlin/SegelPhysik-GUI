# Temporärer Testantrieb und Bewegung durchs Wasser

Stand: 27.10.2026, G-0.5. Technisch implementiert und gezielt getestet;
persönliche fachliche/Sichtabnahme bleibt offen. Maßgeblich sind die
[GUI-Spezifikation](../Spezifikationen/Spezifikation_GUI_Testantrieb_und_Fahrtdarstellung.md),
der [GUI-Plan, Abschnitt 9](../Umsetzungsplaene/Umsetzungsplan_G-0.5.md)
und der getrennte
[CORE-Vertrag C-0.5](../../CORE/Spezifikationen/Spezifikation_Temporaerer_Testantrieb.md).

## Bedienung

Im rechten **Simulationspanel**, Modus **Dynamischer Verlauf**, steht der
Bereich **Testantrieb (temporär)** unter den Lauf-/Stop-Schaltern.

- Startwert: **aus**, Kraft **200 N**, Grenzwert intern exakt
  **10 kn = 463/90 m/s**. Anzeige des Eingabefelds wird gerundet.
- Vor Laufbeginn Kraft von 0 bis 100000 N und Grenzwert von 0,01 bis
  20 m/s einstellen. Beide Zahlen sind während des Laufs gesperrt.
- Das Aktivflag bleibt im laufenden Versuch bedienbar, auch im Rückblick.
  Es beeinflusst den nächsten unberechneten Schritt des laufenden Versuchs,
  nicht den angezeigten historischen Zustand. Die Bestätigung nennt das
  tatsächlich übernommene Ereignis und seinen Schrittübergang.
- Aus entfernt Zusatzkraft **und** künstlichen Begrenzer, stoppt das Boot
  aber nicht. Aktiv bei 0 N lässt den Begrenzer wirksam.
- Keine Speicherung der Testbedienung über Programmende oder in Grafikprofilen.
  Ein Grafikprofilwechsel aktiviert den Antrieb nicht.

Die Kraft wirkt am vordersten tatsächlichen Punkt der **physischen**
Massen-/Volumennetze einschließlich Punktmassen; bei mehreren gleich weit
vorn liegenden deduplizierten Punkten an deren Mitte. Die sichtbare
Darstellungsgeometrie ist dafür keine Physikquelle. Die horizontale
Bugprojektion bestimmt die Richtung. Hebelarmmoment, Rotation und Abdrift
bleiben echt; es gibt keine versteckte Kursstabilisierung.

Der CORE begrenzt nur die horizontale Endgeschwindigkeit nach jedem
aktiven Schritt. Keine nachträgliche Wegkorrektur, keine Änderung von
Vertikal- oder Winkelgeschwindigkeit durch den Begrenzer.
Korrekturimpuls und Energieänderung sind künstliche Testeingriffe, keine
Wasserwiderstandskraft. Das bestehende Bilanzfenster weist einen wirksamen
Begrenzungseingriff ausdrücklich aus, ohne neue Bewegungsinstrumente einzuführen.

## Setup → Grafik

Je Profil Technik/Optik gibt es **Bewegung durchs Wasser**:

| Einstellung | Technik-Default | Optik-Default |
|---|---|---|
| Kielwasser/Bugwelle | aus | ein |
| Wasserraster | ein | aus |
| Technische Fahrspur | ein | aus |
| Distanzmarken ohne Beschriftung | ein | aus |

Alle Optionen sind kombinierbar. Der Wasserraster-Schalter und der bisherige
Bezugsraster-Schalter bearbeiten denselben Wert und denselben Actor.
Explizit gespeicherte Rasterwünsche bleiben bei der Migration erhalten.
Distanzmarken sind bei ausgeschalteter technischer Fahrspur unwirksam;
der gespeicherte Wunsch bleibt erhalten.
Die alte 3D-Kursspur bleibt separat und wird nicht automatisch eingeschaltet.

| Parameter je Profil | Default | Technische Eingabegrenze |
|---|---|---|
| Spuralter | 30 s | endlich, > 0, höchstens 86400 s |
| Räumliche Abtastung | 0,25 m | endlich, > 0, höchstens 10000 m |
| Distanzmarkenabstand | 5 m | endlich, > 0, höchstens 10000 m |
| Stützpunktbudget je Spur | 4096 | ganzzahlig, 2 bis 65536 |

Punkt und Komma sind in den drei neuen Dezimalfeldern erlaubt. Ungültige
Werte verhindern Übernehmen/OK und werden im Dialog diagnostiziert, nicht
still geklemmt. Profilstandard setzt nur das bearbeitete Profil zurück;
Abbrechen verwirft Änderungen seit der letzten Übernahme.
Diese technischen Grenzen sind keine Performancegarantie.
Für den konkreten Fahrweg numerisch nicht darstellbare Abtastung oder
Markenabstände werden zusätzlich mit Grafikdiagnose ausgesetzt, statt
unendliche Koordinaten oder endlose Schleifen zu erzeugen.

## Daten- und Darstellungsverhalten

- Die Quelle sind alle gespeicherten Physikzustände, nicht die Anzahl
  gezeichneter Bilder. Die Live-GUI holt dafür leichte Pose-/Schwerpunkt-/
  Geschwindigkeitsdaten ohne erneute Hydrostatikauswertung in Paketen
  von höchstens 256 Zuständen.
- Die technische Strecke entsteht aus dem horizontalen Schwerpunktweg.
  Abtastpunkte werden wegbezogen zwischen aufeinanderfolgenden Zuständen
  interpoliert. Rotation um ruhenden Schwerpunkt und reine Vertikalbewegung
  erzeugen keinen Fahrweg. Ein geänderter Schwerpunktbezug unterbricht das
  Segment ausdrücklich mit Diagnose.
- Raster, technische Spur und vorhandenes Kielwasser sind weltfest an
  Z=0, mit 0,012 m rein grafischem Tiefenversatz der Linien. Die Kamera
  bleibt körperfest; kein Grafikschalter versetzt die Physikwelt.
- Kontaktanker stammen aus den vorhandenen Wasserlinien-Schnitten der
  Darstellungsmeshes. Anström- und Nachlaufseite orientieren sich an der
  tatsächlichen horizontalen Bewegung, auch bei Abdrift/Rückwärtsfahrt.
  Sie sind nicht identisch mit dem physischen Antriebs-Zugpunkt.
- Kielwasser besteht aus dezenten Linienkanten und einer kleinen
  Kontaktwelle, ohne große weiße Flächen. Ausdehnung/Intensität hängen von
  der Fahrt ab, Alterung ausschließlich von Simulationszeit.
  Kein neuer Kontakt bedeutet keine neue Kontaktspur; vorhandene Spuren
  altern weiter. Fehlende geeignete Konturen werden diagnostiziert.
- Unterstützungen werden nach Alter und Budget von alt nach neu entfernt.
  Zusätzliche Distanzmarken sind ebenfalls auf das Punktbudget begrenzt,
  bei Überschreitung bleiben die jüngsten. Technische Linien einschließlich
  Marken höchstens etwa zweimal Budget, Schaumlinien höchstens zweimal
  Budget plus zwei Kontaktwellenlinien. Kein unbeschränkter GPU-Aufbau.
- Rückblick löscht Zukunftsgeometrie und rekonstruiert aus der vorhandenen
  Aufzeichnung; Vorwärtssprünge berücksichtigen übersprungene Zustände.
  Geänderte Abtastparameter bauen die Historie erneut auf. Modellwechsel
  und neuer Lauf löschen fremde Historie. Große Rücksprünge können deshalb
  Rechenzeit benötigen; es wird nicht heimlich die Physik neu simuliert.
- Fehler ausschließlich in Fahrtdaten deaktivieren die betroffene
  Fahrtdarstellung mit Grafikdiagnose; eine ansonsten gültige Modellansicht
  bleibt benutzbar. Ungültige eigentliche CORE-Zustände bleiben Fehler.

CORE-Testläufe liefern Ergebnisvertrag **3** mit unveränderlichem
historischem Kontext; Altaufrufe ohne Testkontext bleiben bei Version **2**.
Die GUI liest Version 1/2/3 und weist unbekannte Versionen beziehungsweise
fehlenden V3-Testkontext zurück.
Es gibt keine neue numerische Geschwindigkeits-/Streckenkarte oder
beschriftete Distanzmarken und keine behauptete Wellenhydrodynamik.

## Tatsächliche Nachweise

Releasebuild mit MSVC `/W4 /WX /permissive- /utf-8` einschließlich
produktiver GUI erfolgreich. Acht gezielt ausgewählte CTests:

```powershell
ctest --test-dir C:\spdeps\guibuild -C Release -R '^gui_(water_motion|water_motion_render|graphics_settings|graphics_dialog|core_integration|force_display|navigation|navigation_camera)$' --output-on-failure
```

Diese Auswahl prüft die reine Historienlogik, echte Live-Flagwechsel und
V3-Historie, Parameterpersistenz/-migration, Setup-Transaktionen und
Abhängigkeiten, vorhandene Kraftdarstellung sowie die körperfeste Kamera.
Die GPU-Prüfung läuft nur bei 100 %, keine vollständige GUI-/DPI-Suite.
Letzter gemeinsamer Lauf: **8/8 bestanden, 22,98 s**. Die reine
Historienprüfung übt das exakte Standardbudget von 4096 Punkten und
die Altersgrenze von 30 s einschließlich Überschreitung tatsächlich aus.

Der fokussierte GPU-Nachweis verwendet je 101 echte Zustände des 420ers
und Folkeboots bei 200 N und 0,1 m/s Testgrenze, übt die Begrenzung aus,
prüft Profil-/Kamera-/Physikinvarianz und Replay. Bei 0,005 m grafischer
Testabtastung entstehen 20 beziehungsweise 12 Stützpunkte, 23/13
technische Linien einschließlich Marken und 38/22 Kielwasserlinien.
Zusätzliche Bilder einer kontrollierten 5-m-Translation dienen nur der
grafischen Sichtprüfung, ausdrücklich nicht als Physiknachweis.
Ein separater Test in der echten Hauptfensterbedienung prüft Startdefaults,
Zahlensperren, Live-Flag und Rückblick.
Ein einfacher VTK-Quader prüft ohne Modellnamensheuristik den geometrischen
Konturersatz, Rotation bei festem Schwerpunkt, Drift und Rückwärtsfahrt.
Sein 2-m-Weg ergibt acht 0,25-m-Spursegmente und vier 0,5-m-Marken;
reine Rotation erzeugt keine neue Fahrstrecke.
Ein abschließender GPU-Regressionslauf prüft zusätzlich einen numerisch
nicht darstellbaren Markenabstand: explizite Diagnose, gültige Modellansicht
und acht endliche Spursegmente bleiben erhalten (bestanden, 15,53 s).
Die elf gezielten CORE-Regressionen wurden anschließend ebenfalls erneut
bestanden (1,82 s).

Lokale maschinenlesbare Mess-/Bildbelege:
`C:\spdeps\guibuild\graphics-evidence\water-motion\report.json`,
daneben `420_technik.bmp`, `420_optik.bmp`, entsprechende Folkeboot-Bilder
und die ausdrücklich benannten `*_synthetic_motion.bmp`.
Diese erzeugten Dateien gehören zum lokalen Build, nicht zur Spezifikation.
CORE führt seine gezielten Physik-/Vertragsnachweise und Referenzdaten
im eigenen Plan.

Persönliche fachliche/Sichtabnahme und eine separat vereinbarte
Leistungsabnahme sind offen. Keine globale Performancezusage aus einer
Testlaufzeit und keine rückwirkende Abnahme älterer G-0.5-Grafikthemen.
