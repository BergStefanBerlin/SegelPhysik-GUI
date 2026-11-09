# GUI-Spezifikation: Segelkonfigurator in der Bootsauswahl

**Status:** FINAL fachlich spezifiziert auf Nutzerauftrag vom 06.11.2026.
**Arbeitszielversion:** G-0.13. Keine Implementierung oder Nutzerabnahme
behauptet; kein Commit, Tag, Push oder Release beauftragt.
**Plan:** [G-0.13](../Umsetzungsplaene/Umsetzungsplan_G-0.13.md).
**Herkunft:** Anforderungen und Entscheidungen dieser Sitzung vom 06.11.2026;
kein vorheriger Spezifikationsentwurf mit IDEE-Kennungen.

Diese Datei ist die einzige thematische Spezifikation des Segelkonfigurators.
Nutzerauftrag vom 06.11.2026, 21:32 Uhr: einheitliche, datengetriebene
Montagereferenzen statt bootsbezogener Sonderfaelle. Jedes vorhandene
Objekt erscheint genau einmal in seiner aktuellen Lage; technische
Quell-/Ableitungsnetze erzeugen keine zusaetzlichen Objektbaumreferenzen.
Mehrdeutige aktuelle Darstellung wird diagnostiziert, nicht willkuerlich
ausgewaehlt. Der massgebliche Ordner `C:\_DATEN\Code\Cpp\Segelphysik\Modelle`
ist fuer diesen Auftrag strikt schreibgeschuetzt zu behandeln.
Speicher- und Bedienungstests verwenden ausschliesslich separate Kopien.
Der bestehende [Bootsauswahlvertrag](Spezifikation_GUI_Bootsauswahl.md)
bleibt bestehen. Dessen bisheriger rein lesender, fester Inspektionsstand
wird nur fuer ausdrueckliche Segelkonfigurationsaktionen erweitert:
Vorschau und bestaetigte Konfiguration sind vom unveraenderlichen
Quell-Snapshot getrennt. Keine allgemeine Meshbearbeitung oder automatische
Ueberwachung von Modelldateien.

Liefervertrag: [Core-Segelkonfigurator C-0.12](../../CORE/Spezifikationen/Spezifikation_Segelkonfigurator.md).
GUI und Core bleiben separate Projekte mit eigenen Kennungen, Dokumenten,
Builds und Git-Staenden. Die GUI liest oder schreibt keine Modell-, Segel-
oder Lookup-Dateien selbst.

## 1. Fachlicher Umfang

Nur der Reiter **Bootsauswahl** wird erweitert. Start- und Abnahmeboot ist
der **420er mit den neu hinzugefuegten Segeln**. Bearbeitbar sind exakt die
Element-IDs **Grosssegel in Originalschreibweise `Großsegel`** und **`Fock`**,
nicht gleichnamige Gruppen oder beliebige Anzeigenamen.

Rechtsklick auf das eindeutig zugeordnete Element im Objektbaum oder auf
dessen dargestellte Segelflaeche bietet:

| Element-ID | Menueeintraege |
|---|---|
| `Großsegel` | Automatisches Setzen des Großsegels; Manuelles Verschieben |
| `Fock` | Automatisches Setzen der Fock; Manuelles Verschieben |

Im manuellen Fock-Modus kommt **Hoehenversatz entlang des Vorlieks (cm)**
als Rechtsklickaktion hinzu. Andere Objekte bekommen keine Segelaktionen.
Auswahl oder Menueoeffnung allein aendern keine Einbaulage.

## 2. Automatik

- Grosssegel: vorderes unteres Horn, fachlich **Segelhals**, an den
  definierten Schnitt-/Anschlagpunkt von Mast und Grossbaum setzen.
  Unterliek entlang des Grossbaums, Vorliek entlang des Masts ausrichten.
- Fock: Segelhals an den unteren Vorstag-Anschlagpunkt auf dem Deck setzen;
  Vorliek entlang des Vorstags Richtung Mast ausrichten.
- Der Core liefert die fachlichen Bezugspunkte, Richtungen, Eckrollen und
  berechnete starre Transformation. Die GUI schaetzt keine Bezugspunkte aus
  Bildschirmpositionen oder Bounding-Boxen.
- Automatik ersetzt die bisherige Einbaulage durch die definierte
  Grundposition; bei Fock gilt danach 0 cm Versatz. Keine Skalierung,
  Spiegelung, Netzverformung oder Aenderung der intrinsischen Segelform.
- Fehlende/mehrdeutige Referenzen oder geometrisch nicht gemeinsam
  erfuellbare Ausrichtungen werden sichtbar gemeldet; keine angenaeherte
  Platzierung als Erfolg.

## 3. Kamera und manuelle Bedienung

Oberhalb der Modellauswahl wird eine **Kameraauswahl** aehnlich derjenigen
im Statischen Modell eingebaut. Vorhandene Kamerapresets und deren
Benennungen werden wiederverwendet; Einstellungen der anderen Reiter
bleiben unangetastet.

Nur achsenfeste Ansichten erlauben manuelle Bearbeitung:
Backbord/Steuerbord (XZ), Bug/Heck (YZ), Oben/Unten (XY), im Modellbezug.
Freie und Schraegansichten bleiben zur Betrachtung moeglich, erlauben aber
kein Ziehen. Eine sichtbare Erklaerung fordert dort eine achsenfeste Ansicht.
Fuer die Bearbeitung wird eine orthografische Projektion verwendet.

**Manuelles Verschieben** aktiviert den Handcursor und sichtbare Eckgriffe.
Linke Maustaste gedrueckt halten und ziehen:

- Auf der Segelflaeche: gesamtes Segel parallel zur Ansichtsebene
  oben/unten/links/rechts verschieben. Orientierung, Form, Groesse und
  Koordinate senkrecht zur Ebene bleiben unveraendert.
- Auf einem Eckgriff: gesamtes Segel als starren Koerper um die senkrechte
  Modellachse drehen. Segelhals ist der feste Drehpunkt. Wird der Segelhals
  gezogen, ist stattdessen der Segelkopf fest.
- Der Eckgriff bewegt sich auf einem Kreisbogen um den Drehpunkt;
  radialer Mausabstand skaliert oder verformt das Segel nicht.
- Drei- und viereckige Segel folgen derselben Regel. Nur semantische
  Ecken werden zu Griffen, keine Zwischenpunkte des Achterlieks.
  Bei viereckigem Kopf ist der vordere Kopfpunkt am Vorliek der Segelkopf.
- Eckgriffe haben Vorrang vor Flaechen-Picking. Bei in der aktuellen
  Projektion zusammenfallenden Griffen oder verschwindendem Drehhebel
  wird kein beliebiger Griff gewaehlt; Hinweis auf eine andere Achsansicht.
- Zoomen und Kameraverschieben bleiben moeglich, ohne gleichzeitig das
  Segel zu bewegen. Kamerawechsel, Modell-/Reiterwechsel oder Escape
  brechen eine noch nicht abgeschlossene Ziehvorschau ab. Verlassen des
  Modus stellt normalen Cursor und Navigation wieder her.

## 4. Fock: absolute cm-Eingabe

Der Dialog zeigt den aktuellen **absoluten** Versatz mit Einheit cm.
0 cm bezeichnet den Deck-Anschlagpunkt der automatischen Grundposition.
Positive Werte fuehren entlang des Vorstags Richtung Mast, negative
Richtung Deck. Es handelt sich nicht um eine Verschiebung allein entlang Z.
Endliche positive und negative Werte sind erlaubt; keine stillen Grenzen.
Ungueltige Zahlen werden mit Erklaerung abgewiesen.

Die bestaetigte Eingabe verschiebt das gesamte Segel. Vorhandene manuelle
Drehung und seitliche Korrektur bleiben erhalten. Der Core verwendet dazu
die feste Vorstag-Bezugsrichtung der Grundposition, nicht eine nach manueller
Drehung veraenderte Bildschirm- oder Segelrichtung. Derselbe Wert zweimal
eingegeben darf sich nicht aufsummieren. Abbrechen aendert nichts.

## 5. Automatisch und dauerhaft speichern

Alle bestaetigten Konfigurationsaenderungen werden **ohne Speicherknopf**
sofort vom Core dauerhaft gespeichert: Automatik, Maus-Loslassen nach
einem gueltigen Ziehvorgang und bestaetigte cm-Eingabe.
Waehrend des Ziehens wird eine fluessige, noch nicht gespeicherte Vorschau
gezeigt. Eine erfolgreich gespeicherte Aenderung darf bei Neustart,
Modellwechsel oder Reiterwechsel nicht verloren gehen; sie hat kein
Ablaufdatum. Das ersetzt keine Sicherung gegen Loeschen oder Hardwareausfall.

Die GUI zeigt einen neuen Stand erst nach Core-Speicherbestaetigung als
verbindlich an. Bei Schreibfehler bleibt der letzte dauerhaft bestaetigte
Stand aktiv, die fehlgeschlagene Vorschau wird zurueckgenommen und der
Fehler sichtbar gemeldet. Keine Erfolgsmeldung oder stiller Reset.

Auf ausdruecklichen Nutzerauftrag vom 06.11.2026, 14:15 Uhr wird die
Konfiguration in `modells_information.json` integriert; der bisherige Dateiname
`modell.json` und die separate `segelkonfiguration.json` werden abgeloest.
Die Nutzerpraezisierung vom 06.11.2026, 14:45 Uhr legt die Einstellungen
direkt am jeweiligen bestehenden Element fest: `setting` steht neben
`Darstellung`, enthaelt Translation, Quaternion und Konfigurationsstatus.
Kein paralleler Elementbestand, keine neuen Objekt-IDs. Bei Segeln kommen
Original-Buglage und Quellenpruefsumme hinzu; Dateireferenzen werden einmal
am Element gefuehrt, Fockversatz wird aus der Lage abgeleitet. Der Block
`konfiguration` enthaelt nur gemeinsame Speichermetadaten. Alle Objekte
koennen solche Einstellungen fuehren, bekommen dadurch aber keine neuen
GUI-Bedienaktionen. Betriebswinkel bleiben von der gespeicherten Montage
getrennt. Schema, Revision, Quellpruefung und atomare
Core-Speicherung gelten weiter; eigene Speicheraktionen veraendern nicht
die Modellgrundfassung. Die bisherigen Segellagen werden verlustfrei
migriert. Original-OBJ, intrinsische Segel-JSON, Auftragsdateien und Lookups
bleiben unveraendert.

## 6. Anforderungen und pruefbare Abnahme

Alle Anforderungen sind **fachlich final**. Der Umsetzungs- und Nachweisstand
wird im [G-0.13-Umsetzungsplan](../Umsetzungsplaene/Umsetzungsplan_G-0.13.md)
fortgeschrieben; technische Teilnachweise ersetzen keine Abnahme.

| ID | Verbindliche Anforderung | Abnahme |
|---|---|---|
| A-G-00135 | Nur Bootsauswahl; GUI/Core-Trennung und Core als einziger Datenleser/-schreiber. | Keine neuen Bedienfunktionen oder Physikaenderungen in statischem/dynamischem Reiter; GUI-Dateizugriffe auf Konfiguration ausgeschlossen. |
| A-G-00136 | ID-genaue Kontextmenues fuer Grosssegel/Fock in Baum und Darstellung. | Beide Menues/Wege pruefen; Fremdobjekt, Gruppe und uneindeutige Auswahl erhalten keine Aktion. |
| A-G-00137 | Automatik mit Core-Referenzen, ohne Formveraenderung. | 420er: Hals und Liek-Ausrichtungen entsprechen Core-Pruefkriterien; Fock-Versatz danach 0 cm; Fehler sichtbar. |
| A-G-00138 | Kameraauswahl oberhalb der Modellauswahl; manuell nur achsenfest. | Alle sechs Achsansichten und freie/schraege Ansichten; Position bei 100/150/200 % DPI; andere Reiter unveraendert. |
| A-G-00139 | Handcursor und reine Flaechentranslation in Ansichtsebene. | Linksziehen in XY/XZ/YZ: Orientierung/Tiefe erhalten; kein gleichzeitiges Kameraziehen; Abbruch stellt letzten Stand wieder her. |
| A-G-00140 | Starre Eckrotation fuer drei-/viereckige Segel mit Hals-/Kopfpivot. | Alle semantischen Ecken, beide Pivotfaelle und Projektionsentartung pruefen; keine Skalierung/Verformung; Zwischenpunkte ohne Griff. |
| A-G-00141 | Absolute vorzeichenbehaftete Fock-cm-Eingabe im manuellen Rechtsklickmenue. | 0, +10, -10 cm, Wiederholung desselben Werts und Abbruch; Drehung/seitliche Korrektur erhalten; Nichtzahlen abweisen. |
| A-G-00142 | Sofortige dauerhafte Speicherung jeder bestaetigten Aenderung, kein Speicherknopf. | Automatik, Translation, Rotation und cm-Eingabe nach echtem Prozessneustart und Modellwechsel identisch wiederherstellen. |
| A-G-00143 | Sichtbare Fehler und unveraenderte Lookup-/Quellgeometrie. | Schreibverweigerung, veraltete Fassung und kaputte Konfiguration ohne stillen Reset; Quell-/Lookup-Pruefsummen unveraendert. |
| A-G-00144 | 420er-Endabnahme mit neuen Segeln und Regression. | Grosssegel/Fock vollstaendig bedienen, Neustart pruefen; synthetische Dreieckreferenz ergaenzen; bestehende Bootinspektion/Navigation erhalten. |
| A-G-00145 | Einheitlichen Segelstellwinkel nach Abschnitt 8 im GUI/Core-Vertrag erhalten; keine zweite Winkelkonvention. | Vertragspruefung: 0 mittschiffs, +90 Backbord, -90 Steuerbord fuer beide Segel; kein Vorzeichenwechsel an der GUI-Grenze und keine neuen Stellregler in diesem Umfang. |
| A-G-00146 | Original-Buglage je Segel per Rechtsklick auswaehlen und automatisch persistent speichern; Bauchdarstellung gemaess Abschnitt 9. | Grosssegel/Fock getrennt waehlen, Neustart und Speicherfehler pruefen; bei 0 Grad Originalbauch, bei bekannter Zielseite nur bei abweichender Originalseite spiegeln. |

## 7. Nicht beauftragt

Keine neue Aerodynamik, Lookup-Interpolation, Mehrsegelinterferenz,
Kraftberechnung, Segelverformung, allgemeine Modellreparatur oder
Uebernahme der Einbaulage in laufende Simulationen. Der bestehende
Simulationsfreigabestatus des 420ers wird nicht geaendert.
Die spaetere Physik muss dieselbe gespeicherte Einbaulage nutzen; sie
bekommt keine GUI-eigene zweite Transformation.

## 8. Verbindliche Winkelkonvention fuer die spaetere Segelsteuerung

**Nutzerfreigabe:** 06.11.2026, 10:11 Uhr. Dies legt den gemeinsamen
Vertrag fest, beauftragt aber keine Segelsteuerung oder Windberechnung.
Massgeblich ist [Core, Abschnitt 8](../../CORE/Spezifikationen/Spezifikation_Segelkonfigurator.md#8-einheitlicher-segelstellwinkel).

Grosssegel und Fock haben jeweils einen eigenen Stellwinkel delta in Grad,
mit identischer Bedeutung in GUI, Core, Modelldaten und Speicherung:

| delta | Bedeutung in Normallage |
|---|---|
| 0 Grad | Grundsegelebene mittschiffs, Schothorn achterlich |
| +90 Grad | Nach Backbord geoeffnet |
| -90 Grad | Nach Steuerbord geoeffnet |

Die positive Stell-Drehachse zeigt vom Kopf zum Hals, also entgegen
der lokalen Segel-Z-Achse. Dadurch stimmt das Bedienvorzeichen unmittelbar
mit der mathematischen Rechte-Hand-Regel ueberein. Es gibt keinen zweiten
mathematischen Stellwinkel und keine Vorzeichenumrechnung an Schnittstellen.

Nutzerpraezisierung vom 06.11.2026, 15:40 Uhr: Das Modell speichert die
Grundposition mit Grosssegel, Fock und Grossbaum mittschiffs, Schothorn
achterlich; zugehoerige Riggteile werden konsistent ausgerichtet.
Mast-/Vorstagneigung, Anschlagpunkte, Segelwoelbung und Original-Buglage
bleiben erhalten. 0 Grad bezeichnet diese fachliche Grundstellung, nicht
eine Identitaetsquaternion bei geneigtem Vorliek. Eine beliebig geoeffnete
manuelle Einbaulage darf nicht als mittschiffs umgedeutet werden.
Eine spaetere Stellbewegung wird separat darauf angewendet; das freie Eckdrehen im
Konfigurator ist keine Segelstellwinkel-Eingabe. Der Lookup-Anstroemwinkel
alpha beschreibt dagegen den Wind relativ zum Segel und bleibt ein anderer
fachlicher Wert. Bestehende Lookup-Achsen und -Winkel werden nicht geaendert.

## 9. Original-Buglage und gespiegelte Bauchdarstellung

**Nutzerfreigabe:** 06.11.2026. Im Rechtsklickmenue von `Großsegel` und
`Fock` steht unabhaengig vom manuellen Modus ein Auswahlfeld
**Original gezeichnet fuer** mit **Backbordbug** und **Steuerbordbug**.
Es beschreibt ausschliesslich die unveraenderte Quelldatei, nicht die
gewuenschte aktuelle Darstellung. Beide Segel werden getrennt konfiguriert.
Fehlt die Angabe, zeigt die GUI **Nicht festgelegt**, ohne eine Seite
zu erraten. Eine bestaetigte Auswahl wird wie jede andere Aenderung
sofort ohne Speicherknopf vom Core dauerhaft gespeichert.

| Buglage | Definition in Normallage |
|---|---|
| Backbordbug | Baum/Schothorn Backbord, Wind von Steuerbord |
| Steuerbordbug | Baum/Schothorn Steuerbord, Wind von Backbord |

Die spaetere Ziel-Buglage folgt aus dem Stellwinkel: delta > 0 bedeutet
Backbordbug, delta < 0 Steuerbordbug. Bei delta = 0 oder unbekannter
Ziel-Buglage wird der Originalbauch unveraendert angezeigt; keine
automatische Wahl oder Uebernahme der zuletzt verwendeten Seite.
Die manuelle Einbaukorrektur gilt nicht als Stellwinkel.
Es wird weder eine zweite Zielseiten-Auswahl noch ein Stellregler eingefuehrt.

Stimmen Original- und Ziel-Buglage ueberein, bleibt die Form unveraendert.
Sind sie verschieden, spiegelt der Core die Darstellung lokal quer zur
Grundsegelebene (Y -> -Y), vor Einbaulage und Stellrotation.
Die GUI verwendet dessen Geometrie/Spiegelkennzeichnung und fuehrt keine
eigene Vorzeichenheuristik anhand Bildschirm-, Welt- oder Boots-Y aus.
Eine Spiegelung allein verschiebt das Segel nicht auf die andere Baumseite.

Da der aktuelle Konfigurator die Null-Einbaulage bearbeitet, aendert die
Auswahl dort den sichtbaren Bauch zunaechst nicht. Sie liefert die
verbindliche Originalseiten-Metainformation fuer die spaetere Stellung.
Eine benoetigte, aber mangels Originalangabe nicht bestimmbare Spiegelung
wird sichtbar diagnostiziert. Physikalische Nutzung gespiegelter Lookups
ist weiterhin nicht implementiert oder als nachgewiesen auszugeben.
