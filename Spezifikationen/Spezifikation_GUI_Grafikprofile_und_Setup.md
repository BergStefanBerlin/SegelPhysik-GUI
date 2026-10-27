# Spezifikation GUI: Grafikprofile und Setup

**Status: zur Implementierung freigegeben durch erneuten vollständigen
Umsetzungsauftrag vom 27.10.2026; technische Implementierung geprüft,
fachliche Abnahmen offen.**  
**Stand:** 27.10.2026 · **Zielversion:** G-0.5 (korrigierte Nutzerentscheidung)  
**Entwurfspunkte:** [Grafikprofile und Setup](Entwurfspunkte/GUI_Grafikprofile_und_Setup.json)

**Maßgeblicher Umsetzungsplan:** [Umsetzungsplan G-0.5](../Umsetzungsplaene/Umsetzungsplan_G-0.5.md)

### Anforderungszuordnung bei Umsetzungsbeginn

Die nachfolgenden Kennungen führen die fachlichen Verträge der Abschnitte
zusammen, ohne deren Inhalt zu duplizieren. Sie werden projektweit
fortlaufend nach A-G-00027 vergeben.

| Kennung | Vertrag / maßgeblicher Abschnitt | Status |
|---|---|---|
| A-G-00028 | Profilwechsel ohne Fachzustandsverlust, Abschnitt 2 | implementiert; Hauptfenster-/Live-/Rückblicktest erfolgreich |
| A-G-00029 | Setup und Dialogtransaktionen, Abschnitt 3 | implementiert; Dialog- und Laufabbruchtests erfolgreich |
| A-G-00030 | Unabhängige Featurematrix und Defaults, Abschnitt 4 | implementiert; 28 Einzelzustände, Defaults und Abhängigkeiten geprüft |
| A-G-00031 | Wasser, Normalstruktur und Szenenreflexion, Abschnitt 5 | implementiert; GPU-Kombination und Zeitreproduzierbarkeit geprüft; Sichtabnahme offen |
| A-G-00032 | Materialdetails und Folkeboot-Vorführung, Abschnitt 6 | dauerhafter expliziter Core-/Folkeboot-Oberflächenvertrag und zusätzliche GUI-Materialvorschau implementiert; natürliche Materialien, Rücksetzen und Physikinvarianz geprüft; Nutzer-Sichtabnahme getrennt |
| A-G-00033 | Kombinierte Renderqualität und messbare Leistung, Abschnitt 7 | GPU-Kombination gemessen; verbindliche Leistungsabnahme offen |
| A-G-00034 | Persistenz, Migration und Fehlerbehandlung, Abschnitt 8 | implementiert; Prozessneustart, Migration, Schreib-/Datenfehler geprüft |
| A-G-00035 | Grafikbezogene Bestandsregressionen, Abschnitt 9 | Live-/Abbruch-/Ergebnisregression und tatsächliches Picking/Überlagerung/Hebelarm bei 100/150/200 % geprüft; fachliche Bildlesbarkeit offen |
| A-G-00036 | Vollständigkeits-, Herkunfts- und fachlicher Abnahmenachweis, Abschnitt 9 | Abnahme offen |

Der Umsetzungsauftrag wird nicht als bestandene Sicht-/Leistungsabnahme
behandelt. Nach erneutem ausdrücklichem Auftrag zur vollständigen autonomen
Fortsetzung wird die verbleibende Materialsemantik im getrennten
Core-Darstellungsvertrag und in den Folkeboot-Darstellungsmetadaten ergänzt
(Abschnitt 6). Die Implementierungen und Nachweise bleiben projektbezogen
getrennt; der aktive Arbeitsbereich bleibt GUI. Materialien werden
GUI-intern prozedural auf geeigneten vorhandenen Oberflächen erzeugt,
nicht aus zufälligen Elementnamen erraten.

Für Messungen dient zunächst die vorhandene AMD Radeon 760M / Ryzen 5 8600G
unter Windows als benannte Referenz. 1280 × 720 ist die reproduzierbare
Messfenstergröße. Numerische Leistungsgrenzen bleiben bis zur Auswertung und
fachlichen Entscheidung offen; fehlende Werte dürfen weder als bestanden
noch als pauschale Sperre für die Implementierung ausgegeben werden.

## 1. Ziel, Quellen und Abgrenzung

Die GUI erhält eine maximal ausgebaute, konfigurierbare Wasserdarstellung,
Bootsmaterialien und Szenenbeleuchtung sowie zwei unabhängige Grafikprofile:
**Technik** für durchsichtige, gut lesbare Analyse und **Optik** für die
grafisch aufwendige Präsentation. Ein Setup-Dialog macht alle nachfolgend
aufgeführten Darstellungsfeatures je Profil einzeln schaltbar. Einstellungen
werden über das Programmende hinaus gespeichert.

Quellen sind die Nutzerentscheidung vom 27.10.2026,
[Wasser](../../Ideen/Wasser.md) und [Yachtmaterialien](../../Ideen/yacht.md).
Die Ideenpapiere sind Quellen, keine zusätzlichen verbindlichen Spezifikationen.
„Maximum“ bedeutet den vollständigen hier beschriebenen Funktionsumfang,
nicht unbegrenzte Auflösung oder eine Wellensimulation.

Dieses Dokument ist die einzige Spezifikation für die neue Profil-/Setup-
Funktion und deren Grafik-Erweiterungen. Er ergänzt
[GUI-Erstausbau](Spezifikation_GUI_Erstausbau.md), insbesondere A-G-00027,
und [Kraft-/Momentendarstellung](Spezifikation_GUI_Kraft_und_Momentendarstellung.md),
insbesondere A-G-00014/A-G-00018. Deren bestehende fachliche Verträge werden
nicht dupliziert oder rückwirkend als geändert markiert. Neue Schatten und
Texturen gehen über den bisherigen Umfang hinaus; A-G-00027 schloss Schatten
für seinen damaligen Nachtrag ausdrücklich aus.

Keine Physikänderung, keine Wellenkräfte, keine Änderung der Wasserhöhe
`Z = 0`, Pose, Kräfte, Momente oder Flutung. Grafik darf keine
Simulationszustände erzeugen, ändern oder überspringen.
Eine nötige Erweiterung der Core-Darstellungsdaten benötigt eine eigene,
abgestimmte Core-Spezifikation und Freigabe. Diese GUI-Spezifikation autorisiert
keine Core-Änderung.

Vorhandene PBR-Materialien, Himmel/IBL, Wasser-/Tiefenfläche, FXAA und SSAO
sind wiederzuverwenden, nicht parallel neu aufzubauen.
Die Zielversion ist auf korrigierte Nutzerentscheidung G-0.5; G-0.4 ist
bereits als GUI-Tag vorhanden. Die Arbeiten stehen im
oben verlinkten einzigen Umsetzungsplan dieses Versionsschritts.
Die Anforderungs- und Umsetzungsschrittzuordnung wurde bei Beginn der
beauftragten Implementierung ergänzt; offene Abnahmen bleiben ausgewiesen.

## 2. Profilumschalter bei der Vektorauswahl

Der bestehende Auswahlplatz „Sichtmodus“ bei der Vektorpfeilauswahl bleibt
erhalten und wird zum klar beschrifteten Auswahlfeld **„Darstellung“** mit:

- **Technik – durchsichtig**
- **Optik – grafisch**

Es gibt keinen zweiten, unabhängig abweichenden Profilumschalter.
Ein Wechsel aktiviert das vollständige gespeicherte Profil und ändert
weder Modell, Kamerapose, Simulation, Wiedergabezeitpunkt noch ausgewählte
Kraft-/Momentenbeiträge oder deren Skalenreferenzen.

Die bisherigen Möglichkeiten „durch Körper sichtbar“ und „räumlich
(verdeckt)“ bleiben als profilbezogene Vektoroption im Setup erhalten.
Technik verwendet standardmäßig „durch Körper sichtbar“, Optik räumliche
Verdeckung. Benutzer dürfen dies für jedes Profil unabhängig ändern.
Kraft-/Momentengruppen, Wertefenster, Auswahl und Skalen bleiben die
bisherigen eigenständigen Bedienfunktionen; der Profilwechsel setzt sie
nicht zurück.

Die Profilnamen bezeichnen Voreinstellungen, keine unveränderlichen Zwänge:
Auch Technik darf Schatten und Optik transparente Körper verwenden.
Der aktive Profilname ist stets erkennbar. Beim Erststart ist Technik aktiv;
später startet die Anwendung mit dem zuletzt erfolgreich gespeicherten Profil.

**Abnahme:** Beide bisherigen Vektor-Verdeckungsarten sind erreichbar.
Wiederholte Profilwechsel während Lauf und Rückblick verändern nur die
Darstellung; sämtliche fachlichen Auswahl- und Ergebnisdaten bleiben gleich.

## 3. Setup-Funktion und Bedienung

Die Hauptoberfläche erhält den Menüeintrag **Setup**. Dessen erster Eintrag
ist **Grafik …**. Er öffnet einen Dialog mit getrennten Bereichen
**Technik** und **Optik**, identischer Featuregliederung und unabhängigen Werten.

Je Feature gibt es einen eindeutig beschrifteten Ein-/Ausschalter, eine
kurze Wirkungserklärung und gegebenenfalls zugehörige Qualitätsparameter.
Das Bearbeiten des inaktiven Profils aktiviert es nicht.
Es gibt je Profil „Alle einschalten“, „Alle ausschalten“ und
„Profilstandard wiederherstellen“.
„Alle ausschalten“ entfernt optionale Effekte und Hilfen, nicht die
notwendige Grunddarstellung des Modells oder die Bedienbarkeit.

- **Übernehmen:** Änderungen prüfen, aktiv anwenden und dauerhaft speichern;
  Dialog bleibt geöffnet. Einstellungen des inaktiven Profils werden ebenfalls
  gespeichert.
- **OK:** wie Übernehmen, danach schließen.
- **Abbrechen/Fenster schließen:** Änderungen seit dem letzten erfolgreichen
  Übernehmen verwerfen, weder speichern noch anwenden.
- **Profilstandard wiederherstellen:** setzt nur den bearbeiteten
  Dialogentwurf auf die Defaults aus Abschnitt 4; erst Übernehmen/OK speichert.
- **Alle einschalten:** setzt alle aktivierbaren Featurewünsche auf Ein;
  Konfliktgruppen bleiben in einer gültigen Kombination. Nicht verfügbare
  Features erhalten einen sichtbaren Grund und werden nicht als wirksam angezeigt.

Für abhängige Features werden benötigte Optionen vor Übernahme angeboten
oder die abhängige Option mit Begründung gesperrt. Keine stumme
Teilaktivierung. Gegensätzliche Optionen sind Auswahlfelder statt zweier
widersprüchlicher Schalter.

**Abnahme:** Änderung beider Profile, Übernehmen, anschließende weitere
Änderung und Abbrechen ergeben exakt den zuletzt übernommenen Zustand.
Das Zurücksetzen eines Profils verändert das andere nicht.

## 4. Vollständige Feature- und Default-Matrix

„Ein“ bedeutet aktiviert, soweit die Funktion im dargestellten Modell und
auf der Hardware anwendbar ist. Optik hat **alle kombinierbaren Features Ein**.
Fehlende Modellteile werden nicht erfunden. Ohne Segel gibt es beispielsweise
keinen Segelschatten, obwohl Schatten eingeschaltet sind.

| Gruppe | Einzeln schaltbares Feature | Technik | Optik |
|---|---|---|---|
| Wasser | Wasserfläche bis zum Horizont | Ein | Ein |
| Wasser | Fresnel-Effekt / blickwinkelabhängige Oberfläche | Aus | Ein |
| Wasser | Tiefenfarbe und dunkle Tiefenebene | Aus | Ein |
| Wasser | Feine Wasser-Normalstruktur | Aus | Ein |
| Wasser | Langsame Animation der Normalstruktur | Aus | Ein |
| Wasser | Spiegelung der Umgebung (IBL-Anteil des Wassers) | Aus | Ein |
| Wasser | Zusätzliche Spiegelung von Szenenobjekten im Wasser | Aus | Ein |
| Umgebung | Sichtbarer Himmel / Skybox | Aus | Ein |
| Umgebung | Horizontdunst / Luftperspektive | Aus | Ein |
| Beleuchtung | Image-Based Lighting für Modellmaterialien | Aus | Ein |
| Beleuchtung | Erweitertes gerichtetes Lichtset | Aus | Ein |
| Beleuchtung | Sonnenschatten | Aus | Ein |
| Materialien | PBR statt einfacher Grundmaterialien | Aus | Ein |
| Materialien | Klarlack / Clearcoat für geeignete Oberflächen | Aus | Ein |
| Materialien | Materialbezogene Base-Color-Texturen | Aus | Ein |
| Materialien | ORM-Texturen / örtliche Rauheitsvariation | Aus | Ein |
| Materialien | Materialbezogene Normal-Maps | Aus | Ein |
| Materialien | Anisotropie für gebürstetes Metall | Aus | Ein |
| Materialien | Leichte Stofftransparenz für geeignete Segel | Aus | Ein |
| Bildqualität | Kantenglättung (FXAA) | Ein | Ein |
| Bildqualität | Umgebungsverdeckung (SSAO) | Aus | Ein |
| Bildqualität | Transparenz-Tiefenschälung | Ein | Ein |
| Geometrie | Geglättete Normalen mit Erhalt scharfer Kanten | Ein | Ein |
| Hilfen | Wasserlinie / Schnittkontur | Ein | Ein |
| Hilfen | Bezugsraster | Ein | Ein |
| Hilfen | Achsenkreuz | Ein | Ein |
| Hilfen | Kursspur des angezeigten Laufs | Aus (vorläufig) | Aus (vorläufig) |
| Transparenz | Analytische Körperdurchsicht | Ein | Ein |

Nutzerentscheidung vom 27.10.2026 nach Sichtprüfung des dynamischen 420ers:
Die Kursspur bleibt vorerst in beiden Profilen ausgeschaltet, als Ausnahme
zum ursprünglichen Optik-Default „alle Features ein“. Implementierung und
Setup-Schalter bleiben erhalten; eine spätere Überarbeitung wird nicht
vorweggenommen. Keine Projektion auf die Wasserfläche und keine Änderung
der bisherigen 3D-Spurgeometrie.

Der Eintrag „Analytische Körperdurchsicht“ ist in Optik ebenfalls verfügbar
und nach „alle Features“ zunächst eingeschaltet, aber mit
**Körperdeckkraft 1,0** ohne globale Aufhellung/Durchsicht wirksam.
Technik verwendet Körperdeckkraft **0,35**.
Das ist eine transparente, im Setup sichtbare Parameterentscheidung,
kein zweites verstecktes Transparenzsystem. Materialeigene Glas- und
Stofftransparenz bleiben getrennt konfigurierbar; die Kombination muss
eindeutig dokumentiert sein.

Zusätzliche profilbezogene Werte:

| Parameter / Auswahl | Technik-Default | Optik-Default |
|---|---|---|
| Vektorverdeckung | durch Körper sichtbar | räumlich verdeckt |
| Wasserdeckkraft ohne Fresnel | 0,25 | 0,75 |
| Wasser-Grundfarbe | zurückhaltendes Blaugrün | dunkles Petrol |
| Umgebungstyp | prozedural | prozedural |
| HDR-Datei als alternative Umgebung | keine | keine |
| Wasserstruktur-Periode | 0,35 m, einstellbar 0,02–20 m | 0,35 m, einstellbar 0,02–20 m |
| Normalstruktur-Stärke | 0,06, einstellbar 0–1 | 0,06, einstellbar 0–1 |
| Normalstruktur-Animation | deaktiviert | dezent, ohne Geometrieverformung |
| Schatten-/Reflexionsqualität | ausgewogene feste Stufe | höchste implementierte, nachgewiesene Stufe |
| Automatische Reduktion im Live-Lauf | Aus | Aus |

HDR und prozedurale Umgebung sind Alternativen, nicht gleichzeitig
aktivierbare Effekte. Eine externe HDR-Datei ist keine Voraussetzung
für den Optik-Default. Einzelne Feature-Schalter steuern den gewünschten
Effekt; notwendige gemeinsame Renderressourcen dürfen intern weiterbestehen.

**Abnahme:** Automatisierte Tests prüfen sämtliche Defaultwerte und die
Vollständigkeit der Featureliste in beiden Dialogbereichen. Optik besitzt
keinen standardmäßig ausgeschalteten kombinierbaren Effekt. Die unterschiedlichen
Transparenz-/Verdeckungswerte erzeugen dennoch klar verschiedene Startansichten.

## 5. Wasser und Spiegelungen

Die Wasseroberfläche bleibt geometrisch eben bei `Z = 0`.
Fresnel, Normalstruktur, Reflexion und Farbe ändern ausschließlich das Bild.
Es gibt keine Wellengeometrie, Wellenphysik oder veränderte Wasserlinienlage.

Die Normalstruktur wird vorzugsweise prozedural erzeugt, weltmaßstäblich
abgebildet und in der Ferne gefiltert, damit weder Kameramitführung noch
Netzvergrößerung die sichtbare Struktur mitskalieren oder springen lassen.
Benötigte UVs/Tangenten der Wasserfläche sind GUI-intern.

Die Animation folgt im Lauf/Rückblick der angezeigten Simulationszeit,
bleibt im angehaltenen Zustand stehen und ist bei identischem Zeitpunkt
reproduzierbar. Ohne Simulation ist die Oberfläche statisch.

IBL zeigt die Umgebung, nicht automatisch das Boot. Der maximale Umfang
enthält deshalb einen separaten Effekt für ebene Szenenreflexionen:
mindestens opake Rumpf-/Deck-/Segelgeometrie oberhalb der Wasserfläche,
ohne rekursive Wasserspiegelung. Auswahlmarkierungen, Vektoren, Raster und
Achsen werden nicht mitgespiegelt. Transparente Objekte und Reflexionsgrenzen
müssen nachvollziehbar behandelt und dokumentiert werden.

Tiefenebene und Reflexion dürfen die bestehenden Aussparungen trockener
Bootsinnenräume nicht mit sichtbarem Wasser füllen. Unterwassergeometrie
bleibt im Technikprofil erkennbar. Keine zusätzliche feste „Bodenplatte“,
die bei größeren Modellen ungewollt durch deren Geometrie schneidet.

**Abnahme:** Ansichten von oben, flach über Wasser und unter Wasser,
Modellwechsel, Kameraentfernung und Krängung prüfen. Kein Wasser im trockenen
Innenraum; Reflexion folgt Bootspose und Kamera. Deaktivierte
Normalanimation ergibt ein zeitlich unverändertes Bild bei fester Kamera.

## 6. Bootsmaterialien und Texturen

Vorhandene Oberflächenkennungen und Farben bleiben gültig.
Neue Materialdetails bauen darauf auf:

- GFK/Lack: dielektrische Basis mit separat schaltbarer Klarlackschicht.
- Segelstoff: matte helle Oberfläche, Gewebestruktur/Nahtdetails und optional
  geringe Transparenz. Keine Behauptung echter Lichtstreuung im Stoff.
- Holz/Teak: gerichtete Maserung, Fugen, örtliche Rauheit und Normalstruktur.
- Edelstahl/Beschläge: Metallmaterial und bei geeigneter Zuordnung gerichtete
  Anisotropie; polierter Stahl wird nicht zwangsweise gebürstet.
- Matte Plane/Persenning: diffuse Oberfläche ohne erzwungenen Glanz.

Base-Color-Texturen werden im passenden Farbraum behandelt; ORM und Normaldaten
als lineare Daten. Tangenten werden auf Basis geeigneter UVs und Normalen
berechnet, nicht allein durch Einschalten eines Filters behauptet.
Texturmaßstab und Richtung dürfen nicht von Zoom oder Modellpose abhängen.
UV-Nähte und harte Geometriekanten bleiben kontrolliert.

Der Core-Vertrag liefert keine importierten UVs und Texturreferenzen.
GUI-generierte Projektion ist für eindeutig geeignete Flächen möglich.
Eine allgemeine Lösung für modellgebundene UVs/Materialzuordnung benötigt
einen separat freigegebenen Core-/Modellvertrag. Kein erneutes direktes
Lesen von Modelldateien durch die GUI als Umgehung des Katalogvertrags.

Materialien werden nicht anhand zufälliger Elementnamen geraten.
Zur expliziten Vorführung gibt es nach „Grafik“ einen Setup-Eintrag
„Materialvorschau“. Dort wählt der Nutzer einen tatsächlich vorhandenen
Darstellungsteil und weist ihm Original, Lack, Segelstoff, Holz/Teak oder
gebürstetes Metall als vorübergehende GUI-Vorschau zu. Die Zuordnung gilt
bis zum nächsten Modellladen, verändert weder Modell-/Core-Dateien noch
die Physik und ist ausdrücklich keine automatisch erkannte Materialsemantik.
Sie ermöglicht zusätzliche Materialvergleiche unabhängig von der
dauerhaften Modellzuordnung.
Auf erneuten ausdrücklichen Fortsetzungsauftrag wird der getrennte
Core-Darstellungsvertrag rückwärtskompatibel um die expliziten
Oberflächenwerte `stoff`, `holz` und `metall_gebuerstet` erweitert.
Das Folkeboot weist diese Materialien in seinen Darstellungsmetadaten
aus: Rumpf `lack`, Segel `stoff`, Ruder `holz`, Mast `metall_gebuerstet`.
Massen, Volumennetze, Wasserhöhe und Simulationsparameter bleiben
unverändert. Diese modellseitige Zuordnung ersetzt keine Physikgeometrie
und benutzt keine GUI-Heuristik anhand von Elementnamen.
Für diese prozeduralen Materialien genügt die stabile lokale Projektion
auf vorhandener Geometrie; ein allgemeines externes UV-/Texturimportformat
ist kein zusätzlich erforderlicher Lieferumfang. Eine spätere solche
Erweiterung bleibt ein eigenständiger Vertrag.
Nicht texturierbare oder nicht zugeordnete Teile bleiben mit dem vorhandenen
Grundmaterial sichtbar; die fehlende Detailanwendbarkeit wird im Setup
erkennbar. Das ist kein Nachweis einer vollständigen Texturimplementierung.

**Referenzmodell auf Nutzerentscheidung vom 27.10.2026: Folkeboot.**
Am Folkeboot sind alle Features aus Abschnitt 4 vorzuführen und einzeln
zwischen Ein/Aus zu vergleichen: insbesondere Lack am Rumpf, Stoff an den
Segeln, Holz an geeigneten Deck-/Holzteilen, Metall an Beschlägen sowie
Wasserstruktur, Szenenreflexion, Schatten und beide Grafikprofile.
Benötigte Darstellungsdetails und Materialzuordnungen sind am Modell
nachzuweisen oder als noch fehlende Voraussetzungen zu dokumentieren.
Sie dürfen nicht nur für ein Bild als vorhanden behauptet werden.
Eine nötige Ergänzung des Darstellungsmodells darf dessen Physik nicht
unbemerkt verändern; fehlende Simulierbarkeit ist separat zu klären und
darf nicht durch erfundene Live-Ergebnisse verdeckt werden.

**Abnahme:** Das Folkeboot deckt Lack, Stoff, Holz und Metall ab.
Texturorientierung, Maßstab, Rückseiten, Kanten und fehlende Texturdaten werden
geprüft. Keine Änderung der für die Physik verwendeten Geometrie.

## 7. Szenenqualität, technische Lesbarkeit und Grenzen

Sonne, Himmel und Beleuchtung haben eine konsistente Richtung.
Schatten, SSAO, Transparenz, Reflexion und FXAA müssen gemeinsam funktionieren;
getrennte erfolgreiche Einzeltests reichen nicht aus.
SSAO-Radius, Schattenparameter und Tiefenpräzision berücksichtigen den
Modellmaßstab. Vektorfarben, Trefferzonen und Wertefenster bleiben unverfälscht.

Wird ein Effekt ausgeschaltet, werden auch seine vermeidbaren Renderkosten
abgeschaltet; nicht bloß sein sichtbarer Anteil auf null gesetzt.
Das Deaktivieren von Lichtset/PBR lässt eine einfache, lesbare
Grundbeleuchtung/Grundfarbe bestehen. Bei abgeschaltetem Himmel wird ein
neutraler Hintergrund gezeigt.

Nicht unterstützte Features sind mit Begründung markiert. Persistierter
Wunsch und tatsächlich wirksamer Zustand werden unterschieden.
Keine stumme Hardware-Deaktivierung, kein Absturz und kein automatisches
Zurückschreiben abgeschwächter Werte in das gespeicherte Wunschprofil.

Die optionale automatische Live-Reduktion ist selbst je Profil schaltbar,
standardmäßig Aus. Bei Aktivierung muss die Reduktionsregel vorab sichtbar
sein und die Abweichung von den Wunschwerten anzeigen. Sie darf nur
Grafikeffekte reduzieren, niemals Core-Schritte oder Aufzeichnungen.

„Optik: alle“ ist keine unbewiesene 30-Hz-Garantie.
Referenzhardware, Modelle und Auflösung sind für Messungen zu dokumentieren;
Framezeitgrenzen und zulässiger Einfluss auf den Core sind vor einer
erfolgreichen Leistungsabnahme festzulegen.
Zu messen sind mindestens Median und 95. Perzentil der Bildzeiten,
Start-/Ladezeit, Speicherbedarf und Core-Laufzeit mit und ohne Darstellung.
33,3 ms pro Bild entsprechen einem 30-Hz-Ziel, sind aber noch kein
freigegebenes Abnahmelimit für jede Hardware.

## 8. Persistenz und Migration

Die vorhandene QSettings-Infrastruktur wird verwendet. Zu speichern sind:
beide vollständigen Profile einschließlich ausgeschalteter Features und
Parameter, Umgebungswahl/Assetreferenzen, Profilversion und aktives Profil.
Die Daten sind benutzerbezogen und von Modell- und Ergebnisdateien getrennt.
Beide Profile bleiben auch dann erhalten, wenn eines längere Zeit inaktiv ist.

Profilwechsel in der Hauptoberfläche wird gespeichert; Dialogänderungen nur
nach erfolgreichem Übernehmen/OK. Ein Neustart ohne Modell lädt die
Einstellungen ebenfalls korrekt. Bestehende Ordner-, Fenster-, Vektorgruppen-
und Skalierungseinstellungen werden nicht überschrieben.

Eine Migration übernimmt den bisherigen Sichtmodus in das entsprechende
Startprofil, ohne alte Vektoreinstellungen zu verlieren:
„durch Körper sichtbar“ entspricht Technik, „räumlich“ Optik.
Ohne brauchbaren Altwert gilt Technik. Nach Migration gelten die getrennten
Profile; alte Werte dürfen nicht bei jedem Start erneut darübergeschrieben werden.

Fehlende neue Schlüssel erhalten dokumentierte profilbezogene Defaults.
Ungültige Werte, nicht lesbare Einstellungen und Speicherfehler werden
sichtbar gemeldet. Bei Fehler bleibt der letzte gültige Zustand erhalten;
„gespeichert“ darf erst nach erfolgreicher Synchronisation gemeldet werden.
Fehlende HDR-/Texturdateien führen zu sichtbarer Diagnose und einer
kenntlich gemachten prozeduralen/Grundmaterial-Ersatzdarstellung, nicht zu
einem unbemerkten Erfolg.

**Abnahme:** Echter Prozessneustart nach Speichern beider unterschiedlich
konfigurierter Profile, Wiederherstellung des aktiven Profils, unabhängiger
Reset, Migration, neue Schlüssel, ungültige Werte, fehlende Assets und
simulierter Schreibfehler. Testeinstellungen dürfen reale Nutzerwerte nicht ändern.

## 9. Nachweise, Lizenz und offene Freigaben

Erforderliche Nachweisbereiche nach Finalisierung:

| Bereich | Prüfergebnis |
|---|---|
| Profilmodell/Defaults | Jedes Feature unabhängig gespeichert und angewandt; keine Profilvermischung |
| Bedienung | Schalter bleibt bei Vektorauswahl; Setup → Grafik zuerst; Übernehmen/OK/Abbrechen korrekt |
| Persistenz | Neustart, Migration und Fehlerfälle gemäß Abschnitt 8 |
| Visualisierung | Feste Vergleichskameras für beide Profile, Nah-/Fernansicht, Transparenz und Wasserausschnitte |
| Maximum | Alle Features am Folkeboot, einschließlich Lack, Stoff, Holz, Metall, Wasserdetails, Objektreflexion und Schatten, mit Ein-/Aus-Vergleichen nachgewiesen |
| Integration | Bestehende GUI-/Core-Integrationstests unverändert erfolgreich; keine Physikänderung |
| Leistung | Gemessene Werte gegen vorab freigegebene Grenzen, keine bloßen Fremdbenchmarks |
| Fachliche Abnahme | Nutzer bestätigt technische Lesbarkeit und Optik getrennt |
| Herkunft/Lizenzen | Eigene prozedurale Inhalte bzw. geprüfte Assetquellen und erforderliche Beigaben |

### Ergänzende Bestandsnachweise

Die Grafikabnahme umfasst auch die unmittelbar betroffenen offenen
Bestandsprüfungen, ohne neue Physik- oder Wiedergabefunktionen einzuführen:

- Wasser-Schnittqualität aus IDEE-G-00003: konsistente Schnittkontur und
  erkennbare Unterwasserteile, insbesondere im Technikprofil. Ein zusätzlicher
  abgeschnittener oder eingefärbter Volumenkörper wird damit nicht beauftragt.
- DPI-/Auswahlprüfung aus IDEE-G-00014 und der Kraft-/Momentenspezifikation:
  feste Vektorgrößen, Trefferzonen, Mehrfachtreffer, Hebelarm und
  Wertefenstersynchronisation bei 100/150/200 % in beiden Profilen.
- Leistungsprüfung: identische Simulation ohne Darstellung sowie mit Technik
  und Optik vergleichen; gemeinsame Klärung der offenen Grenzwerte aus
  IDEE-G-00014 und IDEE-G-00019.
- Abbruch-/Fehler-/Teilaufzeichnungspfade aus IDEE-G-00005: Grafikwechsel
  und Setup dürfen vorhandene Laufstatus und verfügbare Ergebniszustände
  nicht verfälschen. Reproduzierbare Regressionen sind nachzuweisen;
  keine Erweiterung des Core-Laufvertrags.

Die ursprünglichen Entwurfspunkte werden dadurch weder vollständig
übernommen noch geschlossen. Die konkrete Zuordnung zu Arbeitspaketen
steht ausschließlich im G-0.5-Plan.

Keine neuen externen Assets sind zwingender Default. Externe HDRs/Texturen
werden nur mit belegter Herkunft und geeigneten Nutzungsrechten übernommen;
„kostenlos herunterladbar“ reicht nicht aus.
Die bestehende Lizenzprüfung bleibt unabhängig offen.

Die entschiedene Zielversion, Defaultmatrix und dauerhafte
Folkeboot-Materialentscheidung sowie die offene fachliche Leistungsabnahme
stehen in der separaten Entwurfspunkteliste.
Die oben vergebenen A-Kennungen bezeichnen Verträge, keine bestandenen
Nachweise. Keine behauptete vollständige Umsetzung und keine automatische
GitHub-Veröffentlichung.
