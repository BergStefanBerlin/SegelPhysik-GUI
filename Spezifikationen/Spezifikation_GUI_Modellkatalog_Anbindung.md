# Spezifikation GUI: Anbindung an die Modellkatalog-Schnittstelle

**Status: FINAL – freigegeben am 26.10.2026, in G-0.2 umgesetzt, Abnahme offen (siehe Umsetzungsplan G-0.2).**
**Anforderungen:** A-G-00019 bis A-G-00022
**Umsetzungsplan:** [Umsetzungsplan G-0.2](../Umsetzungsplaene/Umsetzungsplan_G-0.2.md)
**Entwurfspunkte:** [Modellkatalog-Anbindung](Entwurfspunkte/GUI_Modellkatalog_Anbindung.json) (IDEE-G-00015 bis IDEE-G-00017 und IDEE-G-00089 bis IDEE-G-00090)

## 1. Zweck und Abgrenzung

Die GUI liest Modelle heute selbst: `model_catalog.cpp` durchsucht den Ordner
und liest `modell.json`, `obj_mesh.cpp` liest die OBJ-Dateien. Künftig bezieht
die GUI Katalog, Struktur, Geometrie und Diagnosen ausschließlich über die
Modellkatalog-Schnittstelle des Cores („Spezifikation Modellkatalog-
Schnittstelle Core → GUI“ im Core-Projekt, A-C-00042 bis A-C-00046, C-0.2).

Bei der GUI bleiben Ordnerwahl, Ordnerüberwachung, Anzeige, Auswahl und
Zwischenspeicher.

**Bezug zu G-0.1:** Das sichtbare Verhalten von A-G-00002
([GUI-Erstausbau](Spezifikation_GUI_Erstausbau.md)) bleibt erhalten; nur die
Datenquelle ändert sich.

## 2. Anforderungen

| ID | Kurztitel | Version | Status |
|---|---|---|---|
| A-G-00019 | Ordnerwahl und Überwachung bleiben in der GUI | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00020 | Modellkatalog aus dem Core | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00021 | Struktur und Geometrie auf Abruf | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00022 | Verhalten bei geänderten Modellen | G-0.2 | umgesetzt, Abnahme offen |
| A-G-00147 | Schema- und fingerprintabhängige Ansicht und Simulation | G-0.21 | fachlich finalisiert, Umsetzung offen |
| A-G-00148 | Ausdrückliche Aktualisierung des Visualisierungs-Fingerprints | G-0.21 | fachlich finalisiert, Umsetzung offen |
| A-G-00149 | Gemeinsame Laufzeit-Defaults und Modellpfade | G-0.21 | fachlich finalisiert, Umsetzung offen |
| A-G-00150 | GUI-Abnahme des Modellordner-Cutovers | G-0.21 | fachlich finalisiert, Umsetzung offen |

### A-G-00019 – Ordnerwahl und Überwachung bleiben in der GUI

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** IDEE-G-00017

Die GUI wählt, speichert und überwacht den Modellordner wie in G-0.1. Bei
Ordnerwechsel oder gemeldeter Änderung ruft sie `modellkatalog(ordner)` neu
ab. Bei einer späteren Web-GUI legt der Server die zulässigen Ordner fest.

**Abnahme:** Ordnerwechsel und Dateiänderung aktualisieren die Liste wie
in G-0.1.

### A-G-00020 – Modellkatalog aus dem Core

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen · **Herkunft:** IDEE-G-00015, -00016

Die Modellliste zeigt die Hauptobjekte aus dem Core-Katalog mit Name und
Status. Fehlerhafte Modelle sind ausgegraut und mit Warnsymbol markiert,
bleiben als „nur Ansicht“ wählbar, sind aber nicht startbar. Ihre Diagnosen
erscheinen als Tooltip und im Statusbereich. Die GUI wird direkt umgestellt;
ein paralleler eigener Dateileser bleibt nicht bestehen.

**Abnahme:** Dieselben Modelle wie in G-0.1 erscheinen; ein fehlerhaftes
Modell ist sichtbar, aber nicht startbar. Im GUI-Code gibt es keine eigenen
Lesezugriffe auf `modell.json` mehr.

### A-G-00021 – Struktur und Geometrie auf Abruf

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen

Struktur und Darstellungsgeometrie werden erst beim Auswählen eines Modells
vom Core angefordert und für die Sitzung zwischengespeichert (Schlüssel:
Modell-ID und Fassung). Das eigene Lesen der OBJ-Dateien entfällt.

**Abnahme:** Die 3D-Ansicht der Referenzmodelle ist unverändert; im
GUI-Code gibt es keine eigenen OBJ-Lesezugriffe mehr.

### A-G-00022 – Verhalten bei geänderten Modellen

**Version:** G-0.2 · **Status:** umgesetzt, Abnahme offen

Meldet der Core eine veraltete Modellfassung, lädt die GUI den Katalog neu
und weist darauf hin. Laufende Simulationen bleiben unberührt.

**Abnahme:** Test mit geänderter Modelldatei während geöffnetem Modell.

## GUI-Betrieb mit Modellformat 0.3

**Status:** Fachlich finalisiert am 10.11.2026; Zielversion G-0.21.
GUI-Umsetzung und technische Abnahme sind offen.
**Anforderungen:** A-G-00147 bis A-G-00150.
**Core-Grundlage:** [Modellbeschreibung, Modellformat 0.3](../../CORE/Spezifikationen/Spezifikation_Modellbeschreibung.md#modellformat-03-fingerprints-und-modellpfade);
maßgebliche Schemaquellen sind
[schema_data.json](../../Modelle/Modelle-NEW/schema_data.json) und
[schema_validation.json](../../Modelle/Modelle-NEW/schema_validation.json).
Der einzige Plan für den Schritt ist der
[Umsetzungsplan G-0.21](../Umsetzungsplaene/Umsetzungsplan_G-0.21.md).
Die IDEE-zu-A-Zuordnung bleibt in der
[Entwurfspunkteliste](Entwurfspunkte/GUI_Modellkatalog_Anbindung.json)
nachvollziehbar.

### A-G-00147: Schema- und fingerprintabhängige GUI-Nutzung

- Die GUI verwendet neue Modellordner ausschließlich über den Core-
  Modellkatalog und die Core-Modellprüfung. Sie implementiert weder einen
  eigenen JSON-Schema-Validator noch einen zweiten Parser oder
  Fingerprint-Algorithmus.
- Ein Modell, dessen Dateien oder dateiübergreifende Angaben die Core-
  Schema-Validierung nicht bestehen, wird mit Core-Diagnosen kenntlich
  gemacht und weder dargestellt noch simuliert.
- Die GUI darf die vollständige Darstellungsgeometrie eines Modells anzeigen,
  wenn beide Modelldateien schema-valide sind und der Core den gespeicherten
  `visualisation`-Hash als aktuell bestätigt. Fehler anderer Bereiche werden
  weiterhin angezeigt und sperren die Simulation, aber nicht diese Ansicht.
- Bei einem vom Core als veraltet oder ungültig gemeldeten
  `visualisation`-Hash zeigt die GUI
  keine Geometrie. Am Modelllisteneintrag bietet sie per Rechtsklick den
  Befehl „Visualisation-Hash erneuern“ an. Dieser ruft die öffentliche
  Core-C++-Funktion ausschließlich für den `visualisation`-Bereich auf.
  Nach erfolgreicher Aktualisierung lädt bzw. prüft die GUI den Katalog neu;
  die Ansicht erscheint nur, wenn Schema- und Ressourcenprüfung erfolgreich
  sind. Die Diagnose bleibt bei Fehlern sichtbar.
- Für valide Modelle verwendet die GUI die vom Core gelieferten Identitäten,
  Struktur, Geometrie, Status und Modellfassung. Sitzungscaches werden mit
  Modell-ID und Fingerprint verknüpft; ein geänderter Fingerprint löst eine
  erneute Katalogabfrage und Cache-Invalidierung aus.
- Die bestehende Ordnerüberwachung wird auf Änderungen an
  `modells_data.json`, `modells_validation.json`, den referenzierten
  Ressourcen und beiden Schema-Dateien erweitert. Eine Änderung am Schema
  aktualisiert damit ebenfalls den Katalogstatus. Außer dem ausdrücklich
  gewählten Kontextmenübefehl schreibt die GUI keine Fingerprints.
- Core-Diagnosen werden wie bisher in Modellliste, Tooltip und Statusbereich
  sichtbar gemacht. Absolute Pfade oder interne Parserdetails werden von der
  GUI nicht ergänzt.

**Abnahme**

- Ein valides 0.3-Modell wird über den Core-Katalog angezeigt und wie ein
  gleichwertiges vorhandenes Modell geladen.
- Schema- und Identitätsfehler erscheinen als Modellfehler; vor erfolgreicher
  Core-Prüfung werden keine Modellgeometrie und keine Simulation verwendet.
- Ein veralteter Visualisierungs-Hash sperrt zunächst die Ansicht. Der
  Rechtsklickbefehl aktualisiert nur diesen Hash und ermöglicht die Ansicht
  erst nach erfolgreicher erneuter Core-Prüfung. Fehler anderer
  Fingerprint-Bereiche bleiben sichtbar und sperren die Simulation.
- Änderungen an Daten-, Validierungs-, Ressourcen- oder Schema-Dateien
  erneuern den Katalog und verhindern die Nutzung eines veralteten
  GUI-Caches. Ein externer Fingerprint-Aufruf aktualisiert den sichtbaren
  Status nach einer Dateiänderung.
- Der GUI-Build und die GUI-Tests belegen, dass die GUI keine eigene
  Auswertung der neuen Modell-JSON-Dateien oder Schemata enthält.

### A-G-00148: Ausdrückliche Visualisierungs-Hash-Aktualisierung

Sind beide Modelldateien schema-valide und meldet der Core einen veralteten
oder ungültigen Visualisierungs-Hash, bietet die GUI am Modelle-Listeneintrag
per Rechtsklick ausschließlich die Aktion „Visualisation-Hash erneuern“ an.
Sie ruft die öffentliche Core-
Funktion nur für `visualisation` auf. Nach Erfolg lädt und prüft die GUI
den Katalog neu; die Ansicht wird erst nach erfolgreicher Schema-,
Identitäts- und Ressourcenprüfung freigegeben. Fehlermeldungen bleiben
sichtbar. Kein anderer Bereich und kein anderer Auslöser darf
`modells_validation.json` ändern.

**Abnahme:** Bei gültigem Visualisierungs-Hash oder schema-ungültigen
Modelldateien ist der Befehl nicht verfügbar. Bei Ausführung ändert er
ausschließlich diesen Bereich und
aktualisiert die Ansicht erst nach erneutem Core-Erfolg. Scheitert der Core-
Aufruf, bleiben GUI- und Dateizustand konsistent und die Diagnose sichtbar.

### A-G-00149: Gemeinsame Laufzeit-Defaults und Modellpfade

GUI, Core und Modellwerkzeuge beziehen Modellbasis, Schema-Wurzel und
veränderbare Standardpfade aus derselben zur Laufzeit gelesenen Datei
`%APPDATA%\SegelPhysik\defaults.json`. Die Felder `modelRoot`,
`schemaRoot` und das erweiterbare Objekt `defaults` haben in allen
Verbrauchern dieselbe Bedeutung. `defaults` ist offen; unbekannte Schlüssel
werden nicht abgelehnt. Standardwerte lassen sich ohne Neu-Build ändern oder
ergänzen. Relative Pfade werden relativ zur Datei aufgelöst. Explizite
Benutzerauswahl hat Vorrang, danach Test-Override,
Umgebungsvariable und Datei. Fehlerhafte oder nicht lesbare Werte erzeugen
eine konkrete sichtbare Diagnose und keinen stillen Ersatzpfad. Änderungen
erfordern keinen Neu-Build.

**Abnahme:** Tests belegen gemeinsame Feldsemantik, Vorrangreihenfolge,
explizite Modellauswahl, Laufzeitänderung und sichtbare Fehler. Startwert,
Überwachung und Katalog verwenden keinen hartcodierten aktiven Modellpfad.

### A-G-00150: GUI-Abnahme des Modellordner-Cutovers

Die GUI stellt nicht vor bestandenem Vorab-Gate auf den aktiven
`Segelphysik\Modelle`-Pfad um. Physische Umbenennung und Verschiebung erfolgen
einmalig im getrennten Core-Plan; die GUI überschreibt, löscht oder verschiebt
keine Modellordner. Vor dem Cutover müssen Core- und GUI-Builds und
vollständige Tests gegen `Modelle-NEW` bestanden sein. Danach verifiziert
die GUI erneut alle elf Modelle, Core-Prüfstatus, Ansicht, Diagnosen,
Simulationssperren, Überwachung und Cacheinvalidierung gegen den finalen
`Modelle`-Pfad. Ein Fehler stoppt die Abnahme; Rückfall erhält
`Modelle_alt` unverändert.

**Abnahme:** Pre-Cutover- und Post-Cutover-Testnachweise sind im
G-0.21-Plan dokumentiert; die Post-Cutover-Prüfung hängt vom erfolgreichen
Core-Cutover und Core-Posttest ab.

### G-0.21 Nachverfolgbarkeit

| Anforderung | Umsetzungsschritte |
|---|---|
| A-G-00147 | U-G-000156, U-G-000157, U-G-000159, U-G-000160, U-G-000161 |
| A-G-00148 | U-G-000158, U-G-000160 |
| A-G-00149 | U-G-000159, U-G-000160 |
| A-G-00150 | U-G-000160, U-G-000161 |
