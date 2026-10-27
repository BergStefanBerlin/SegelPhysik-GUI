# Navigation: Bedienung, Koordinatenvergleich und Herkunft

**Stand:** 27.10.2026. Technische Dokumentation des direkten Nutzerauftrags,
keine neue Spezifikation und keine Lizenz- oder Releasefreigabe.

## Bedienung

Kompass, Krängung und Stampfen stehen untereinander als Qt-Overlay innerhalb
der 3D-Ansicht. An **Navigation · ziehen** lässt sich die Gruppe verschieben;
sie bleibt beim Verkleinern der Ansicht innerhalb des Viewports.
An der **Größenecke rechts unten** lässt sich das Overlay in Breite und Höhe
vergrößern oder verkleinern; die SVG-Instrumente skalieren proportional in
ihren jeweiligen Flächen. Es bleibt ein eingebettetes Overlay, kein externes
Fenster. Eine vorübergehend kleinere 3D-Ansicht begrenzt die tatsächliche
Größe; bei erneutem Vergrößern kehrt die gewählte Größe zurück.
**Darstellung → Kompass und Lageanzeigen** blendet sie ein oder aus.
Dieser Schalter wird benutzerbezogen über das Programmende gespeichert.
Position und Größe der Gruppe werden nicht über das Programmende gespeichert.

Alle drei Instrumente verwenden denselben aktuell ausgewählten
Ergebniszustand wie das Modell, auch beim Zurückblättern im laufenden Versuch.
Ein Modell ohne Ergebniszustand zeigt **FEHLER**, nicht die Beispielwerte
der ursprünglichen SVGs oder eine erfundene Nulllage.

- Kompass: Bugrichtung, im Uhrzeigersinn ab lokalem Nord, 000–359°,
  mit 16-Punkt-Himmelsrichtung.
- Krängung: positiv, wenn Steuerbord steigt; negativ, wenn Backbord steigt.
- Stampfen: positiv, wenn der Bug steigt.
- Senkrechter Bug: Kompass und Krängung sind singulär und zeigen FEHLER;
  Stampfen bleibt ±90°. Das entspricht der alten Winkelzerlegung.
- Fehlender/ungültiger Primärwert: rote Innenscheibe und FEHLER.
- Gültige Krängung über ±90°: roter Außenring, weiterhin gültiger Zahlenwert.
  Genau ±90° löst keinen Gefahrenring aus. Die normale Stampfzerlegung
  liefert nur Werte zwischen −90° und +90°.
- Gelbe Kamerasymbole: tatsächliche Kameraposition relativ zum Blickziel,
  bei Krängung/Stampfen in die jeweilige vertikale Ebene projiziert.
  Mausdrehen aktualisiert die Marker. Eine undefinierte Projektion blendet
  nur den betreffenden Marker aus, nicht den gültigen Physikwert.
- Für die Segelseite existiert im aktuellen Snapshot kein Winkelvertrag.
  Deshalb bleibt das Original-Bootssymbol mit Segel auf Backbord bestehen;
  es behauptet keine aus der Physik gelieferte aktuelle Segelstellung.

## Koordinatenvergleich

Verglichen wurde der Altbestand von `BergStefanBerlin/SegelPhysik`,
Commit `c18114546c13cfe6709064c33492893ed8f0785e`, mit dem aktuellen
[CORE-Modellvertrag](../../CORE/Spezifikationen/Spezifikation_Modellbeschreibung.md)
und den [GUI-Posetransformationen](../Code/src/force_display.cpp).

| Gegenstand | Altprojekt | Aktueller CORE / GUI |
|---|---|---|
| Körperachsen | Rechtshändig: +X Bug, +Y Backbord, +Z oben in Referenzlage | Identisch; kein Achsentausch erforderlich |
| Weltachsen | Rechtshändig: +X Nord, +Y West, +Z oben | CORE schreibt +Z oben und Wasserfläche Z=0 vor, aber keine geographische X/Y-Richtung |
| Lokales Nord der Anzeige | +X; Ost ist −Y | Auf ausdrücklichen Nutzerentscheid ebenfalls +X Nord / +Y West, **ausschließlich GUI-Konvention** |
| Quaternion-Reihenfolge | wxyz | xyzw; keine ungeprüfte Übernahme der vier Arraypositionen |
| Anzeigewinkel | `Rz(-Heading) Ry(-Stampfen) Rx(-Krängung)` | Gleiche Anzeigevorzeichen, aus der aktuellen xyzw-Rotation abgeleitet |
| Punkte / Vektoren | Lage und Richtung unterscheiden | Punkte rotieren und verschieben; Richtungen nur rotieren |
| Bezugspunkte | Konstruktionsursprung, Schwerpunkt und weitere Punkte unterscheiden | Unverändert verschiedene Punkte, keine zusätzlichen Weltkoordinatensysteme |

Es wurden weder CORE-Weltachsen noch Quaternionvertrag, Modellursprung,
Schwerpunkt, physikalische Daten oder Simulationsrechnung geändert.
„Nord“ bezeichnet hier keine georeferenzierte Himmelsrichtung.

Die Ableitung steht in [navigation.cpp](../Code/src/navigation.cpp):
`Heading = atan2(-Bug.y, Bug.x)`, auf [0°, 360°) gewickelt;
`Stampfen = atan2(Bug.z, hypot(Bug.x, Bug.y))`;
`Krängung = atan2(-Backbord.z, Oben.z)` bei nicht senkrechtem Bug.
Alle drei Körperrichtungen entstehen aus derselben validierten Pose.

## Kamera-Dropdown

Unter **Darstellung → Kamera** gibt es die 14 Originalansichten in
Originalreihenfolge, zusätzlich **Freie Ansicht (Maus)** als Ausgangseintrag:

1. Oben Backbord Bug
2. Oben Backbord Heck
3. Oben Steuerbord Bug
4. Oben Steuerbord Heck
5. Unten Backbord Bug
6. Unten Backbord Heck
7. Unten Steuerbord Bug
8. Unten Steuerbord Heck
9. Backbord (Seite)
10. Steuerbord (Seite)
11. Bug
12. Heck
13. Oben
14. Unten

**Korrigierter Nutzerauftrag:** Die Kamera ist jetzt an das **Körpersystem
der Yacht** gekoppelt, nicht an die festen Weltachsen. Eine Auswahl setzt
die körperrelative Ansicht; freies Drehen, Verschieben und Zoomen per Maus
bleibt möglich und legt anschließend die neue körperrelative Kamera fest.
Der gewählte Eintrag bezeichnet die zuletzt aktivierte Vorgabe.
Das Umschalten des Grafikprofils verändert die Kamera nicht.
Die Auswahl wird nicht als Startkamera gespeichert.

Die Namen beziehen sich auf die aktuellen Yachtachsen:
Bug +X, Heck −X, Backbord +Y, Steuerbord −Y, Oben +Z, Unten −Z.
Für die senkrechten Ansichten ist Kamera-Up körperlich +Y, sonst +Z.
Die acht Eckrichtungen benutzen die Vorzeichen der ursprünglichen
`(±1.7r, ±1.7r, ±1.5d)`-Vorgaben.

**Wasserhöhen-Präzisierung nach Nutzerfoto:** Backbord, Steuerbord, Bug und
Heck setzen beim Anwählen sowohl Kameraposition als auch Blickziel auf
**Welt-Z=0**. Die Blickrichtung hat damit **0° Elevation**, Kamera-Up ist
Welt-Z. Die aktuelle Yachtachse wird dazu horizontal projiziert; ihre
Richtung bleibt erhalten, statt auf die hohe Geometriemitte inklusive Mast
zu zielen. Der Abstand berücksichtigt weiterhin die gesamte Modellgeometrie.
Eine exakt senkrechte Yachtachse hat keine horizontale Projektion und wird
mit expliziter Diagnose abgelehnt, nicht durch eine erfundene Richtung ersetzt.
Anschließend bleibt auch diese Ansicht körperfest: Spätere Bootsdrehungen
können die Kamera aus der Wasserhöhe bewegen. Dauerhaft weltfestes Z=0 würde
der ausdrücklich gewünschten körperfesten Kamera widersprechen.

Bei jedem ausgewählten Ergebniszustand werden Kameraposition und Blickziel
aus der bisherigen Pose ins Körperkoordinatensystem und von dort mit der
neuen Pose zurück in die Welt transformiert. Kamera-Up wird nur rotiert.
Damit bleiben Abstand, Blickrichtung, Ziel und Bildschirmorientierung relativ
zum Boot konstant, auch bei Krängung, Stampfen, Gieren, Translation und
Rückwärtswiedergabe. Die Welt bewegt sich optisch um das Boot; die Physik
selbst bleibt im unveränderten Weltsystem.

Das Laden eines anderen Modells setzt die Kamera wie bisher neu.
Das Ende einer Aufzeichnung passt dagegen nicht mehr die gesamte Weltbahn
ein; auch automatisches Nachzoomen aufgrund weltfester Begrenzungen entfällt,
weil beides die körperfeste Kamera überschreiben würde.
Die Kamerasymbole der Neigungsanzeigen bleiben vorhanden und verwenden
weiterhin die tatsächliche Kameraprojektion.

**Bewusste Anpassung:** Statt der alten festen Beckengröße `r` und
Wassertiefe `d` wird die aktuelle Modellgeometrie zentriert und vollständig
eingepasst. Die Eckrichtung benutzt das Verhältnis 1.7 : 1.7 : 1.5.
Es gibt hier keinen entsprechenden festen Beckenvertrag; alte absolute
Kameradistanzen würden kleine Modelle unsichtbar oder große abgeschnitten
darstellen. Es entsteht kein neuer Physik- oder Beckenparameter.

## Übernommene Quellen

Alle Links sind auf denselben Commit des Altprojekts festgelegt:

- [Navigationswidget-Spezifikation v0.7](https://github.com/BergStefanBerlin/SegelPhysik/blob/c18114546c13cfe6709064c33492893ed8f0785e/doc/WegZuV1.0/Revisionslog/zuV0.7/Spezifikationen/Spezifikation_Navigationswidgets_v0.7.md)
- [Weltsystem- und Bewegungsspezifikation v0.7](https://github.com/BergStefanBerlin/SegelPhysik/blob/c18114546c13cfe6709064c33492893ed8f0785e/doc/WegZuV1.0/Revisionslog/zuV0.7/Spezifikationen/Spezifikation_Weltsystem_Bewegung_v0.7.md)
- [Python-SVG-Bindung](https://github.com/BergStefanBerlin/SegelPhysik/blob/c18114546c13cfe6709064c33492893ed8f0785e/code/python/segelphysik/gui/navigationswidgets.py)
- [Winkelableitung](https://github.com/BergStefanBerlin/SegelPhysik/blob/c18114546c13cfe6709064c33492893ed8f0785e/code/python/segelphysik/core/navigationsanzeigen.py)
- [Original-Kameravorgaben](https://github.com/BergStefanBerlin/SegelPhysik/blob/c18114546c13cfe6709064c33492893ed8f0785e/code/python/segelphysik/grafik_optimierung.py)
- [Original-SVG-Ordner](https://github.com/BergStefanBerlin/SegelPhysik/tree/c18114546c13cfe6709064c33492893ed8f0785e/doc/WegZuV1.0/Revisionslog/zuV0.7/Widgets)

Die drei SVGs liegen als eingebettete Ressourcen
[hier](../Code/resources/navigation).
Ihr Inhalt wurde **bytegleich** übernommen. Nur der lokale Dateiname
`Widget_Krängung.svg` wurde zu `Widget_Kraengung.svg` transliteriert,
weil MSBuild die Unicode-Abhängigkeit falsch decodierte.

| Lokale Datei | SHA256 |
|---|---|
| [Widget_Kompass.svg](../Code/resources/navigation/Widget_Kompass.svg) | `19EE51F631317886ED0E10AD9FFDE4B36F5FB1EA65A74670A68F572E002FA318` |
| [Widget_Kraengung.svg](../Code/resources/navigation/Widget_Kraengung.svg) | `74A32917516F67980B6AC242E7BD011F4F007FAC50E06F7DC31D8A27727EE49A` |
| [Widget_Stampfen.svg](../Code/resources/navigation/Widget_Stampfen.svg) | `D6FF611A933E1E42B44A6AAA286D4FA3A5D5C1605E021121EB1240D52D39610A` |

Zur Laufzeit werden DOM-Kopien mit Zahlen, Rotationen, Sichtbarkeit,
Fehlerscheibe und Gefahrenring gebunden. Die Originaldateien bleiben
unverändert. QtSvg übernahm die Klassen-Schriftgrößen der numerischen
Anzeigen nicht zuverlässig: Die Originalzahl hatte im installierten
Windows-Renderer nur etwa 18 × 12 statt etwa 101 × 64 SVG-Einheiten.
Deshalb werden die ursprünglichen Schriftgrößen der Wert-, Richtungs- und
HDG-Texte in der DOM-Kopie explizit gesetzt. Der Test prüft die tatsächlichen
Renderer-Textgrenzen, nicht nur den vorhandenen Zahlentext.

Das ursprüngliche separate Fenster wurde auf Nutzerentscheid durch das
verschiebbare Viewport-Overlay ersetzt. Die C++-Implementierung verwendet
QtSvg und QtXml statt einer Python-Laufzeit. Kameraableitungen unterstützen
auch die übernommenen Unterwasseransichten und die tatsächlich freie Kamera.

Nicht behauptet wird eine vollständige Umsetzung aller darüber hinaus
enthaltenen Altprojekt-Optionen: zusätzliche Geschwindigkeits-/Windkarten,
umfangreiche Titel-/Analog-/Numerik-Konfiguration oder ein neuer
Segelwinkelvertrag gehören nicht zu diesem Auftrag.
Die Herkunftsangaben ersetzen keine noch offene
[Verteilungs- und Lizenzprüfung](Lizenzdokumentation.md).

## Prüfbarkeit

- `gui_navigation`: xyzw-Vorzeichen, Himmelsrichtungen, fehlende/ungültige
  Pose, senkrechter Bug, Kamera-Singularitäten, Unterwasserkamera,
  DOM-Bindung, FEHLER/Gefahrenring, Textgröße und Drag/Resize.
- `gui_graphics_render_100`, `_150`, `_200`: echte Windows/OpenGL-Prüfung,
  dynamischer 420er mit ausgewähltem Snapshot in beiden Profilen,
  Folkeboot-Hauptfenster mit allen 14 Kameravorgaben, Markern bei tatsächlicher
  Kameraänderung, Sichtbarkeitsschalter und unveränderten Physikwerten.
- GPU-Nachweise unter
  `C:\spdeps\guibuild\graphics-evidence\<Skalierung>\report.json`,
  `420_dynamic_*.bmp` und `navigation_mainwindow.bmp`.
- Optionale eigenständige Instrumentenbilder:
  Umgebungsvariable `SEGELPHYSIK_NAVIGATION_EVIDENCE` auf einen Zielordner
  setzen und `segelphysik_gui_navigation_tests` ausführen.
  Für echte Schriften `QT_QPA_PLATFORM=windows` verwenden; `minimal`
  prüft Bindungen und Geometrie, nicht die tatsächlichen Windows-Glyphen.

Diese Tests ersetzen keine persönliche gestalterische Abnahme.

**Historischer Nachweis vor der Kamera-/Größenkorrektur:** MSVC-Release-Anwendung und betroffene
Testziele mit `/W4 /WX /permissive- /utf-8` erfolgreich gebaut.
Die vier oben genannten CTest-Fälle bestanden im abschließenden Lauf
gemeinsam in **121,60 s** (4/4). Dabei wurden auch das erneute Aktivieren
derselben Kameravorgabe nach einer freien Kameraänderung sowie das Speichern
und Wiederherstellen des Sichtbarkeitsschalters im nächsten Hauptfenster
geprüft. Original-SVG-Prüfsummen wurden gegen die heruntergeladenen Quellen
verglichen. Windows-Instrumentenraster und das Folkeboot-Hauptfenster wurden
nach der Schriftkorrektur zusätzlich visuell kontrolliert.
Es wurden keine Commits, Tags, Pushes oder Veröffentlichungen ausgeführt.

### Gezielte Prüfung der Kamera-/Größenkorrektur

Auf Nutzerwunsch nur die beiden betroffenen Tests, **keine vollständige
GUI-Suite und keine drei DPI-Läufe**:

```powershell
ctest --test-dir C:\spdeps\guibuild -C Release -R '^gui_navigation(_camera)?$' --output-on-failure
```

- `gui_navigation`: tatsächliche Mausereignisse am Größen-Griff; nur Overlay,
  nicht Hauptfenster vergrößert; Position bleibt erhalten; Größe nach
  temporärer Viewport-Verkleinerung und bei kleinem Startviewport korrekt.
- `gui_navigation_camera`: kurzer echter 420er-Lauf mit 11 Zuständen,
  zusätzliche Translation und 120°-Rotation jeweils um X/Y/Z; Kamera-
  Position, Ziel und Up im Körperrahmen mit Toleranz `1e-8` verglichen;
  alle 14 Vorgaben bei gedrehter Yacht; freie Kameraänderung, Rücksprung,
  Zustandslöschen und Abschluss-Einpassen ohne Verlust der relativen Kamera.
  Zusätzlich Größen-Griff im tatsächlichen VTK-Viewport von 180 × 564 auf
  280 × 764 Pixel geprüft und fotografisch kontrolliert.

**Ergebnis im letzten Lauf: 2/2 bestanden in 6,03 s.** Die ursprünglichen Beispiel-CSS-
Rotationen der SVG-Posegruppen werden beim dynamischen Binden entfernt,
wie in der Altimplementierung, damit sie den tatsächlichen Winkel nicht
überschreiben. Die drei Original-SVG-Dateien bleiben unverändert.

Die Hauptanwendung wird wegen laufender Nutzerinstanzen separat unter
`C:\spdeps\guibuild\navigation-preview\segelphysik_gui.exe` gebaut;
die laufende Release-EXE wird nicht überschrieben und kein Nutzerprozess
beendet. Quellstand bleibt uncommittet.

### Wasserhöhe: gezielter Buildnachweis

Nach der Wasserhöhen-Präzisierung wurden ausschließlich der fokussierte
`gui_navigation_camera`-Test und der GUI-Release-Build ausgeführt.
**1/1 Test bestanden in 6,25 s.** Geprüft werden für die vier Ansichten
Kamera-Z und Ziel-Z gleich 0, horizontal projizierte Yachtachse,
anschließende körperfeste Bewegung sowie vollständige Sichtbarkeit der
transformierten Modell-Boundingbox-Ecken innerhalb des Bildausschnitts.
Der Abstand wird anhand der gesamten Geometrie um das Wasserziel und des
kleineren horizontalen/vertikalen Sichtwinkels gewählt, damit der Mast beim
Verschieben des Blickziels auf Wasserhöhe nicht abgeschnitten wird.
Bildnachweise: `graphics-evidence\navigation-camera\water_level_*.bmp`.
Die Backbordansicht wurde zusätzlich visuell kontrolliert.
Aktuelle startbereite GUI weiterhin unter `navigation-preview` (siehe oben).
