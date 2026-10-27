# Spezifikation GUI: Temporärer Testantrieb und Fahrtdarstellung

**Status: finalisiert, technisch implementiert und gezielt nachgewiesen; persönliche Abnahme offen.**  
**Stand:** 27.10.2026 · **Zielversion:** G-0.5  
**Entwurfspunkte:** [Testantrieb und Fahrtdarstellung](Entwurfspunkte/GUI_Testantrieb_und_Fahrtdarstellung.json)

Dieses Dokument ist die einzige fachliche Spezifikation dieses Themas.
Es beschreibt den Nutzerauftrag für einen temporären Testantrieb und die
grafischen Optionen **2: Kielwasser/Bugwelle** sowie **3: technische Fahrspur**.
Die Auswahl beider Darstellungen erfolgt im Setup. Eine neue numerische
Bewegungsanzeige ist ausdrücklich nicht enthalten.

Die Nutzerantworten auf die Entscheidungsfragen sind in der verlinkten
Entwurfspunkteliste dokumentiert, einschließlich abgelöster Vorschläge.
Die Anforderungen werden nach A-G-00036 projektweit fortlaufend vergeben.
Finalisierung ist keine Implementierungsfreigabe. Der bestehende
[einzige Plan G-0.5](../Umsetzungsplaene/Umsetzungsplan_G-0.5.md)
wurde auf den Planauftrag vom 27.10.2026 in Abschnitt 9 um
U-G-000037 bis U-G-000045 ergänzt. Auf ausdrücklichen Auftrag vom
27.10.2026, 22:16 wurden CORE und GUI gemeinsam umgesetzt.
Tatsächliche technische Nachweise stehen im Plan, Abschnitte 9.6/9.7;
sie ersetzen keine persönliche fachliche/Sicht- oder Leistungsabnahme.

| Kennung | Maßgeblicher Vertrag | Herkunft | Status |
|---|---|---|---|
| A-G-00037 | Umfang, Optionen 2/3, keine neue Zahlenanzeige; Abschnitte 1/5/9 | IDEE-G-00022 | technisch nachgewiesen |
| A-G-00038 | Bug-Zugpunkt und horizontale Kraft ohne Kursregelung; Abschnitt 2 | IDEE-G-00023 | CORE implementiert und gezielt geprüft |
| A-G-00039 | Horizontaler Testbegrenzer und Eingriffsnachweis; Abschnitt 2 | IDEE-G-00024 | CORE/GUI-Vertrag integriert und geprüft |
| A-G-00040 | Temporäre Bedienung, Parameter und Live-Aktivflag; Abschnitt 2 | IDEE-G-00025 | technisch nachgewiesen |
| A-G-00041 | Wasserbezug und grafisches Kielwasser; Abschnitte 3/4 | IDEE-G-00026 | technisch nachgewiesen; persönliche Sichtabnahme offen |
| A-G-00042 | Technische Fahrspur und rotationsunabhängiger Weg; Abschnitt 5 | IDEE-G-00026 | technisch nachgewiesen |
| A-G-00043 | Setup-Auswahl, Profildefaults und Persistenz; Abschnitt 6 | IDEE-G-00027 | technisch nachgewiesen |
| A-G-00044 | Konfigurierbare Spurgrenzen und reproduzierbare Wiedergabe; Abschnitte 6/7 | IDEE-G-00028 | technisch nachgewiesen; keine allgemeine Performancezusage |
| A-G-00045 | Gezielte Abnahme und Lieferbereichstrennung; Abschnitte 1/8/9 | IDEE-G-00028 | Planzuordnung und technische Belege vollständig; Nutzerabnahme offen |

## 1. Ziel und Abgrenzung

Die Bewegung eines Modells durch das Wasser soll bei körperfester Kamera
deutlich sichtbar werden. Ein zuschaltbarer Testantrieb erzeugt hierfür
tatsächliche simulierte Bewegung. Er ersetzt weder Segelantrieb noch
Wasserwiderstand oder eine spätere vollständige Fahrphysik.

Der Nutzer hat festgelegt:

- Zusätzliche Kraft in Bugrichtung für das jeweils simulierte Modell.
- Kraftbetrag und Geschwindigkeitsobergrenze sind Parameter.
- Ein Flag schaltet die Testkraft ein beziehungsweise aus.
- Die Funktion ist temporär zum Testen.
- Die Kamera bleibt relativ zum Körper konstant; die Welt bewegt sich
  dadurch optisch um das Boot.
- Die grafischen Optionen 2 und 3 sind im Setup auswählbar.
- Die numerische Darstellung aus Option 3 entfällt. Eine übergreifende
  numerische Darstellung wird später separat behandelt.

„Je Körper“ wird hier als je ausgewähltes, simuliertes Gesamtmodell
verstanden, nicht als zusätzliche Einzelkraft auf jedes Rumpf-, Mast- oder
Segelbauteil. Eine neue Mehrmodell-/Mehrkörpersimulation gehört nicht dazu.

Maßgebliche bestehende Verträge:

- [Grafikprofile und Setup](Spezifikation_GUI_Grafikprofile_und_Setup.md):
  vorhandener Dialog, Technik/Optik, unabhängige Profile und Persistenz.
- [Kraft-/Momentendarstellung](Spezifikation_GUI_Kraft_und_Momentendarstellung.md):
  vorhandene Kraftbeiträge und Diagnosewege, keine neue Zahlenoberfläche.
- [Navigation und Kamera](../Dokumentation/Navigation.md):
  aktuelle körperfeste Kamera und Wasserhöhen-Vorgaben.

Die neue Spezifikation erweitert den Gegenstand um Testbewegung und deren
Darstellung; die ältere Grafik-Spezifikation autorisiert keine Physikänderung.
GUI verwaltet Bedienung und Darstellung. Die Erzeugung und Begrenzung
simulierter Bewegung gehört in den getrennten CORE-Laufvertrag. Dieses
GUI-Dokument ersetzt keine CORE-Spezifikation und gibt keine CORE-Änderung
frei. Konkrete Vertragsänderungen sind dort vor Umsetzung abzustimmen.
Der inzwischen finalisierte und implementierte
[CORE-Vertrag](../../CORE/Spezifikationen/Spezifikation_Temporaerer_Testantrieb.md)
führt A-C-00052 bis A-C-00060; der
[CORE-Plan C-0.5](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.5.md)
enthält die getrennten Umsetzungs- und Testnachweise.

## 2. Temporärer Testantrieb

### Bedienung und Parameter

Ein klar abgesetzter Bereich **Testantrieb (temporär)** bietet:

| Bedienung | Bedeutung |
|---|---|
| Testantrieb aktiv | Startwert aus; im laufenden Versuch schaltbar |
| Testkraft [N] | Default 200 N; Bereich 0 bis 100000 N |
| Maximale Fahrt [m/s] | Default exakt 10 kn = 463/90 m/s (ca. 5,14444 m/s); Bereich 0,01 bis 20 m/s |

Die Zahlenfelder sind **Eingabeparameter**, keine neue numerische
Ergebnisdarstellung. Ihre Einheiten bleiben sichtbar.
Ungültige Eingaben werden mit Diagnose zurückgewiesen, nicht still geklemmt
oder durch Standardwerte ersetzt. Anzeigerundung der Geschwindigkeits-
Eingabe darf den exakten Default von 10 kn intern nicht verändern.

Der temporäre Bereich wird als kompakter Bedienblock/Overlay im
Simulationspanel integriert, nicht als weiterer Setup-Menüpunkt oder
über den 3D-Instrumenten. Er darf bestehende Lauf-/Stop-Bedienung nicht
verdecken. Antriebsparameter gelten für den Simulationslauf, nicht je
Grafikprofil. Ein Profilwechsel darf den Antrieb weder aktivieren noch
seine Parameter verändern. Keine Speicherung über das Programmende;
bei Neustart gelten wieder die obigen Defaults und Aktivflag aus.

### Verhalten

- Die Kraft dreht sich mit der Yacht und erzeugt echte Positionsänderungen
  im Simulationszustand. Reines Verschieben von Wassertexturen reicht nicht.
- Der Angriffspunkt liegt an der geometrisch vordersten Stelle des
  Gesamtmodells in Körper-+X-Richtung, unabhängig von der Wasserlinie.
  Maßgeblich ist die tatsächliche Modellgeometrie, kein Boundingbox-
  Fantasiepunkt und keine Namensheuristik. Bei mehreren gleich weit
  vorn liegenden Geometriepunkten wird deren Mitte verwendet; die
  konkrete Geometriequelle und Gleichheitstoleranz dokumentiert der
  getrennte CORE-Vertrag. Der körperfeste Punkt wird mit der Pose transformiert.
  Sein Hebelarm zum Schwerpunkt und damit entstehende Momente bleiben wirksam.
- Die Kraft wirkt horizontal entlang der normierten Projektion der aktuellen
  Körper-+X-Achse auf die Welt-XY-Ebene. Eine undefinierte Projektion erzeugt
  Diagnose und keine erfundene Kraftrichtung. Fehlender Wasserkontakt
  allein schaltet den Testantrieb nicht ab.
- „Gerade nach vorn“ bedeutet diese Kraftrichtung, nicht eine erzwungene
  Geradeausbahn. Keine künstliche Kursregelung, kein Unterdrücken von
  Drehung oder seitlicher Abdrift. Zug am Bug garantiert keine Kursstabilität.
- Die Obergrenze ist kein sofortiger Sollwert. Unterhalb beschleunigt das
  Modell entsprechend Masse und resultierenden Kräften.
- Nur bei aktivem Testantrieb wird nach jedem Physikschritt der Betrag
  der horizontalen Wasserrelativgeschwindigkeit auf den Grenzwert begrenzt.
  Dazu wird bei Überschreitung der XY-Vektor richtungstreu skaliert.
  Vertikal- und Winkelgeschwindigkeit bleiben vom Begrenzer unberührt.
  Bei ausgeschaltetem Testantrieb wirken weder Zusatzkraft noch Begrenzer.
  Ohne Strömung ruht das Wasser im Weltsystem; eine neue Strömung wird
  durch diesen Auftrag nicht eingeführt.
- Ausschalten entfernt die zusätzliche Antriebskraft. Es ist kein
  Stoppknopf und erzwingt nicht von selbst Geschwindigkeit null.
- Der Begrenzungseingriff ist ausdrücklich künstlich und darf nicht als
  physikalischer Widerstand oder Energieerhaltung ausgegeben werden.
- Testkraft und gegebenenfalls Begrenzungseingriff müssen im CORE-Ergebnis
  beziehungsweise dessen vorhandenen Diagnosewegen nachvollziehbar sein.
  Es wird kein normaler, vollständiger Kräftebilanzschluss vorgetäuscht,
  falls der Begrenzer die Geschwindigkeit außerhalb der Kraftintegration ändert.

Kraftbetrag und Geschwindigkeitsgrenze werden vor Laufbeginn übernommen
und bleiben während des Laufs gesperrt. Nur das Aktivflag ist live schaltbar.
Steuerereignisse werden am nächsten noch nicht berechneten Physikschritt
übernommen und mit wirksamer Schrittkennung, Simulationszeit und Flagzustand
aufgezeichnet. Keine rückwirkende Änderung bereits gespeicherter Zustände.
Im Rückblick wirkt der Schalter auf die weiterlaufende Simulation, nicht
auf den gerade betrachteten historischen Zustand; die Bedienung erklärt dies.
Mehrere Ereignisse erhalten eine deterministische Reihenfolge.
Dieser Ablauf benötigt einen separaten CORE-Laufvertrag vor Implementierung.

Durchgehend ausgeschalteter Testantrieb muss den bisherigen Physiklauf
unverändert lassen. Abschalten nach aktiver Fahrt macht deren entstandene
Position/Geschwindigkeit nicht rückgängig.

## 3. Gemeinsame Grundlage der Fahrtdarstellung

Die Grafik verwendet die tatsächlichen ausgewählten Simulationszustände:
Pose, Zeit, Geschwindigkeit relativ zum Wasser und gegebenenfalls Historie.
Ohne Strömung ist die Wasserrelativgeschwindigkeit gleich der Geschwindigkeit
im ruhenden Weltsystem. Eine spätere Strömung darf nicht stillschweigend
ignoriert werden; fehlende Daten benötigen einen klar dokumentierten Vertrag.

Die Kamera bleibt körperfest. Weder Darstellungsoption noch Antriebsschalter
versetzen sie, erzwingen eine neue Kamera oder bewegen die Physikwelt.
Wasserreferenzen und bereits erzeugte Spuren werden im Weltsystem gehalten,
nicht als mitbewegte Dekoration an die Yacht oder Kamera geheftet.

Darstellungsobjekte liegen auf beziehungsweise mit kleinem rein grafischem
Tiefenversatz an der Wasserfläche Z=0. Sie ändern weder die physikalische
Wasserhöhe noch Auftrieb, Kräfte oder Momente. Das Mast-/Gesamtgeometriezentrum
ist kein zulässiger Punkt für die Fahrspur.

Simulation und Darstellung bleiben getrennt: Effekte wirken niemals auf
die Physik zurück und werden auch bei anderweitig erzeugter Fahrt angezeigt.
Der Schaum darf nicht lediglich dem Aktivflag folgen: Entscheidend ist die
tatsächliche Fahrt, auch nach Abschalten des Antriebs.

## 4. Option 2: Kielwasser, Bugwelle und Schaum

Die optische Darstellung umfasst:

- Dezente, auseinanderlaufende Kielwasser-/Schaumkanten hinter dem Körper.
- Eine kleine visuelle Bugwelle im vorderen Wasser-Kontaktbereich.
- Eine zurückbleibende, mit dem Alter ausblendende Spur im Wasser.

Intensität und Ausdehnung orientieren sich an der horizontalen
Wasserrelativgeschwindigkeit. Bei fehlender Fahrt entsteht keine neue
Fahrspur allein durch Krängung, Stampfen, Gieren oder Wellenanimation.
Vorhandene Spur klingt weiter anhand der Simulationszeit ab.

Bug-/Heckanker für die Grafik werden aus den bestehenden Wasserlinien-
Konturen in Körper-+X/-X-Richtung gewonnen, nicht aus dem unabhängigen
Antriebs-Zugpunkt. Keine Namensheuristik und keine neuen Modell-Pflichtfelder.
Bei Abdrift oder Rückwärtsfahrt folgt die zurückbleibende Spur der
tatsächlichen horizontalen Bewegungsrichtung. Kontaktwellen werden am
jeweils angeströmten Kontaktbereich dargestellt, nicht blind am nominellen Bug.
Das gilt auch bei gekenterten Lagen, sofern verwertbare Konturen vorliegen.
Bei einfachen Körpern dient dieselbe Kontur als ausdrücklich geometrische
Näherung. Ohne Kontakt entsteht keine neue Spur; bei fehlenden/ungeeigneten
Konturdaten explizite Diagnose statt erfundener Anker.

Es handelt sich um **grafische Fahrteffekte**, nicht um berechnete
Wellenhydrodynamik. Kelvinwinkel, Wellenhöhe oder Widerstand dürfen nicht
als physikalisch nachgewiesen ausgegeben werden.

Die Darstellung muss ohne dominante weiße Flächen, Schlaufen am Segel,
Flackern an der Wasserfläche oder Sichtverlust der bestehenden Kraftpfeile
auskommen. Wassertransparenz, Schatten, Reflexion und Unterwasseransicht
sind bei der Integration zu berücksichtigen.

## 5. Option 3: Technische Fahrspur ohne numerische Anzeige

Die technische Darstellung besteht aus unabhängig schaltbaren Bestandteilen:

- **Weltfestes Wasserraster** als räumlicher Bewegungsbezug.
- **Technische Fahrspur** auf Wasserhöhe, aus der tatsächlichen
  horizontalen Translation eines festgelegten Körperbezugspunkts.
- **Distanzmarken** entlang der Spur in gleichmäßigen Wegabständen.

Distanzmarken sind rein geometrische Striche/Punkte, **ohne Zahlen oder
Beschriftung**. Sie werden nach zurückgelegtem Weg, nicht nach Anzahl
der Renderbilder gesetzt. Die Markenschrittweite ist im Setup konfigurierbar
(Abschnitt 6); der vorhandene Rastermaßstab wird wiederverwendet.

Nicht enthalten sind neue Anzeigen für m/s, kn, Strecke, Beschleunigung,
Begrenzerwerte oder eine neue HUD-/Instrumentenkarte. Bestehende
numerische Oberflächen bleiben unverändert; die Gesamtkonzeption
numerischer Darstellung ist ausdrücklich ein späterer Auftrag.
Ein einfacher beschrifteter Ein-/Aus-Schalter für den Testantrieb bleibt
als Bedienung zulässig und ist keine Ergebnis-Zahlenanzeige.

Die alte, vorerst deaktivierte 3D-Kursspur wird nicht umgedeutet oder
automatisch reaktiviert. Die neue wasserbezogene technische Spur erhält
einen eindeutigen Namen und verwendet keinen mitrollenden Mast-/Meshmittelpunkt.
Maßgeblich ist der horizontale Schwerpunktweg, auf Wasserhöhe projiziert.
Rotation um einen ruhenden Schwerpunkt erzeugt keine Wegstrecke.
Kontaktpunkte gestalten die Wasser-/Schaumeffekte, sind aber kein
Streckenzähler. Änderungen eines Schwerpunktbezugs allein, etwa durch
geänderte Massenverteilung, dürfen nicht als Fahrweg interpretiert werden;
eine solche Diskontinuität unterbricht das Spursegment mit Diagnose.

## 6. Auswahl im Setup und Persistenz

Unter dem vorhandenen **Setup → Grafik** wird die Gruppe
**Bewegung durchs Wasser** je Profil Technik/Optik ergänzt.
Option 2 und Option 3 sind unabhängig und gemeinsam nutzbar, nicht
gegenseitig ausschließende Modi.

| Schalter | Default Technik | Default Optik |
|---|---|---|
| Kielwasser/Bugwelle (Option 2) | aus | ein |
| Wasserraster (Option 3) | ein | aus |
| Technische Fahrspur (Option 3) | ein | aus |
| Distanzmarken (Option 3) | ein | aus |

Distanzmarken benötigen die technische Fahrspur. Bei ausgeschalteter Spur
wird ihre Wirkung ausgesetzt; der gespeicherte Wunsch bleibt erhalten,
wie bei vorhandenen abhängigen Grafikfeatures.
Das vorhandene Wasserraster wird wiederverwendet, nicht doppelt gezeichnet.
Wasserraster hier und sein bisheriger Setup-Schalter bearbeiten denselben
Einstellungswert. Die Defaults gelten bei neuen Profilen und explizitem
Reset; bestehende ausdrücklich gespeicherte Rasterwünsche bleiben erhalten.

Zusätzlich sind je Profil folgende Werte im Setup einstellbar und persistent:

| Parameter | Default | Gültigkeit |
|---|---|---|
| Spuralter | 30 s | endlich, größer 0 |
| Räumliche Abtastung | 0,25 m | endlich, größer 0 |
| Abstand der Distanzmarken | 5 m | endlich, größer 0 |
| Maximalzahl Stützpunkte je Spur | 4096 | ganze Zahl, mindestens 2 |

Die Grenzen gelten für optische und technische Spuren. Sobald Alter oder
Punktbudget erreicht ist, werden die ältesten Abschnitte entfernt; das
Punktbudget kann also vor Ablauf des Alters greifen. Zusätzliche erzeugte
Schaumgeometrie bleibt proportional zum Punktbudget begrenzt.
Nicht darstellbare/allokierbare Konfigurationen werden explizit abgelehnt,
nicht still reduziert. Konkrete technische Eingabeobergrenzen werden mit
dem späteren Umsetzungsplan dokumentiert und sind kein erfundener
Leistungsabnahmenachweis. Parameterfelder sind keine neue Ergebnisanzeige.

Es gelten die bestehenden Transaktionen: Übernehmen/OK speichern,
Abbrechen verwirft Änderungen seit der letzten Übernahme, Reset betrifft
nur das bearbeitete Profil. Bestehende Einstellungen bleiben bei der
Schemaerweiterung erhalten. Ein Neustart stellt die Darstellungswahl wieder her.
Antriebsflag und -parameter gehören ausdrücklich nicht in die Grafikprofile.

## 7. Zeit, Wiedergabe, Grenzen und Fehler

- Gleicher Zustand mit gleicher Historie und denselben Einstellungen
  ergibt dieselbe Fahrtdarstellung, unabhängig von Renderfrequenz und
  Echtzeit-Takt.
- Pause friert Spur, Schaum und Altersausblendung ein.
- Beim Zurückspringen dürfen keine zukünftigen Spuren sichtbar bleiben.
  Beim Vorwärtsspringen werden übersprungene Zustände aus der verfügbaren
  Historie berücksichtigt, ohne die Physik neu oder anders zu berechnen.
- Die vollständige Aufzeichnung ist die Quelle; eine Darstellungsreduktion
  darf sie nicht verändern. Historienausdünnung oder Caches brauchen
  definierte Fehlergrenzen, keine unbegründete Ergebnisveränderung.
- Spurlebensdauer, räumliche Abtastung und Geometriebudget sind endlich
  und dokumentiert. Lange Läufe dürfen keinen unbegrenzten GPU-/RAM-Aufbau
  verursachen. Die konfigurierbaren Grenzen stehen in Abschnitt 6.
- Modellwechsel und neuer Lauf löschen fremde Fahrthistorie.
- Fehlende/ungültige Zustands- oder Kontaktdaten werden ausdrücklich
  diagnostiziert; kein erfundener Nullwert, keine zufällige Ersatzspur.
  Die gültige Modellansicht soll unabhängig vom betroffenen Effekt weiter
  benutzbar bleiben.

## 8. Vorgesehene Abnahmeszenarien

Diese Liste definiert die Abnahmeszenarien. Tatsächlich erfolgte technische
Nachweise sind im Plan, Abschnitt 9.7, zugeordnet; die Liste allein
begründet keine fachliche Abnahme:

| Szenario | Erwarteter Nachweis |
|---|---|
| Testantrieb aus | Gleiche Physikzustände wie bisher; Grafikoptionen ändern keine Fachwerte |
| Beschleunigung und Obergrenze | Parameter wirken im CORE-Lauf; festgelegte Geschwindigkeitsgröße überschreitet die Grenze nicht außerhalb einer vereinbarten numerischen Toleranz |
| Ausschalten | Keine neue Testkraft; kein impliziter Sofortstopp; Fahrteffekt folgt weiter der tatsächlichen Bewegung |
| Live-Aktivflag | Schalter wirkt erst auf dokumentierten zukünftigen Schritt; Rückblick unverändert; Zahlenparameter im Lauf gesperrt |
| Zugpunkt | Vorderste Modellgeometrie unabhängig von Wasserlinie; tatsächliches Hebelarmmoment, keine versteckte Kursregelung |
| Unterschiedliche Modelle | Folkeboot für optische Bewertung, 420er für kurzen dynamischen Nachweis, einfacher Körper für geometrischen Ersatzfall |
| Körperfeste Kamera | Referenzen/Spuren bleiben weltfest; relative Kamera bleibt bei Fahrt und Rotation erhalten |
| Ruhe, Rollen und Stampfen | Keine künstliche Fahrtstrecke oder neue Schaumspur ohne horizontale Translation |
| Wasserbezug | Keine Spur am Mast/Segel, keine Kontaktwelle an einem vollständig aus dem Wasser gehobenen Körper |
| Setup-Kombinationen | Beide Optionen aus, nur 2, nur 3, beide; Raster, Spur und Marken getrennt; je Profil unabhängig |
| Spurparameter | Alle vier Werte je Profil speichern/resetten; 30 s / 0,25 m / 5 m / 4096 als Defaults; Alters- und Budgetbegrenzung tatsächlich geprüft |
| Wiederanlauf | Darstellungswahl erhalten, Testantrieb gemäß festgelegtem Sicherheitsstartwert aus |
| Pause und Rückblick | Gleicher Zeitpunkt/Historie reproduzierbar; keine Zukunftsspur, kein Weiteraltern im Stillstand |
| Darstellung | Kraftpfeile, Wassertransparenz und bestehende Modellansicht bleiben lesbar |
| Umfangsgrenze | Keine neue numerische Bewegungsanzeige oder Distanzbeschriftung hinzugefügt |

Tests zunächst gezielt auf die betroffenen Verträge begrenzen. Eine
vollständige GUI-/DPI-Suite ist nicht als Standardauftrag enthalten.
Bild- und Laufzeitbewertung, technische Tests und persönliche Sichtabnahme
sind getrennt auszuweisen. Leistungsgrenzen und Fehlertoleranzen werden
nicht ohne Nutzerfreigabe als erfüllt oder verbindlich behauptet.

## 9. Nicht enthalten und weiteres Vorgehen

Keine echte Wellen-/Widerstands-/Segelantriebsphysik, keine neue Strömung,
keine automatisch erzeugte Bojenlandschaft aus Option 1, keine neue
numerische Instrumentierung, keine Reaktivierung der alten 3D-Kursspur.

Die Nutzerentscheidungen sind finalisiert und G-0.5 zugeordnet.
Der vorhandene G-0.5-Plan enthält nun in Abschnitt 9 die U-/A-Zuordnung.
Implementierungsfreigabe und projektgetrennte CORE-Vertragsarbeit sind
erfolgt; beide Lieferbereiche wurden technisch umgesetzt und gezielt geprüft.
Konkrete technische Geometrie-/Eingabetoleranzen, Bedienung und Grenzen stehen
im CORE-Vertrag sowie der
[Bedienungsdokumentation](../Dokumentation/Testantrieb_und_Fahrtdarstellung.md).
Persönliche fachliche/Sichtabnahme und separate Leistungsabnahme bleiben
offen; technische Testergebnisse werden damit nicht gleichgesetzt.
Keine Commit-, Push-, Tag- oder Releasefreigabe.
