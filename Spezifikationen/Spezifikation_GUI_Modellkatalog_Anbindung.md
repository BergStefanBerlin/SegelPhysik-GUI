# Spezifikation GUI: Anbindung an die Modellkatalog-Schnittstelle

**Status: FINAL – freigegeben am 26.10.2026, in G-0.2 umgesetzt, Abnahme offen (siehe Umsetzungsplan G-0.2).**
**Anforderungen:** A-G-00019 bis A-G-00022
**Umsetzungsplan:** [Umsetzungsplan G-0.2](../Umsetzungsplaene/Umsetzungsplan_G-0.2.md)
**Entwurfspunkte:** [Modellkatalog-Anbindung](Entwurfspunkte/GUI_Modellkatalog_Anbindung.json) (IDEE-G-00015 bis IDEE-G-00017)

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
