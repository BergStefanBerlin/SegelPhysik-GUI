# Lizenzdokumentation SegelPhysik-GUI

**Stand:** 27.10.2026 · **Geprüfter GUI-Commit:** `8671f2e`  
**Ergebnis:** technische Teilprüfung dokumentiert; **keine Verteilungsfreigabe**.  
**Bezug:** [IDEE-G-00006](../Spezifikationen/Entwurfspunkte/GUI_Erstausbau.json),
[GitHub-Issue #1](https://github.com/BergStefanBerlin/SegelPhysik-GUI/issues/1).

## 1. Umfang und Entscheidung

Geprüft wurden der [Lizenzentwurf](../../Ideen/Lizenz.md), das
[vcpkg-Manifest](../vcpkg.json), die [Builddefinition](../CMakeLists.txt)
und die unten bezeichneten lokalen Build-/Paketnachweise.
Diese Dokumentation ist ein Nachweis, keine neue Spezifikation und keine
eigene Lizenz des Projekts. Sie gilt ausschließlich für GUI. CORE und Modelle
werden durch diese GUI-Prüfung weder geändert noch lizenzrechtlich freigegeben.
Die spätere eigenständige CORE-Prüfung ersetzt diese GUI-Prüfung nicht.

### Lizenzentscheidung und Umsetzung am 27.10.2026

Der Nutzer hat für GUI folgende Lizenzrichtung ausdrücklich festgelegt:

- Private, nicht-kommerzielle Nutzung und kostenlose Weiterentwicklung.
- Schulen, Universitäten und gemeinnützige Vereine dürfen die Software auch
  bei Kursgebühren und bezahlten Forschungsprojekten nutzen; Originallink
  muss genannt werden.
- Bei Weitergabe müssen Software und weitergegebene Änderungen kostenlos
  bleiben; Quellcode, Lizenzbedingungen, Urheberhinweise und Originallink
  müssen erhalten bleiben. Dasselbe soll für nachfolgende Ableitungen gelten.
- Verkauf, betriebliche Nutzung für kommerzielle Arbeit und bezahlte
  Dienstleistungen außerhalb der Bildungs-/Vereinsausnahme sind verboten.

Das ist keine freie Open-Source-Lizenz. Die Richtung der eigenen
Lizenzentscheidung ist damit geklärt; ein wirksamer, kompatibler Lizenztext
ist noch nicht freigegeben. Der
[Bedingungenentwurf](Lizenzbedingungen_Entwurf.md) setzt die Entscheidungen
als projektbezogenen Formulierungsvorschlag um, ohne sie bereits in Kraft
zu setzen. Die Bezeichnung als Entwurf ist keine Freigabe für Nutzer.

**Prüfung bestehender Modelle:**

| Modell | Ergebnis für die gewünschten Bedingungen |
|---|---|
| MIT | Erlaubt kommerzielle Nutzung und geschlossene Ableitungen; ungeeignet. |
| GPL-3.0-or-later | Erlaubt Verkauf und kommerzielle Nutzung; trotz Quellcodepflicht bei Weitergabe ungeeignet. |
| PolyForm Noncommercial 1.0.0 | Erlaubt nicht-kommerzielle Zwecke und nennt Ausnahmen für Bildung/gemeinnützige Organisationen unabhängig von Finanzierung. Verlangt aber weder kostenlose Weitergabe noch vollständigen Quellcode noch dieselben Bedingungen für eigene Änderungen; keine vollständige Abdeckung. |

Der [Originaltext von PolyForm](https://polyformproject.org/licenses/noncommercial/1.0.0)
wurde geprüft, nicht verändert oder als eigene Lizenz übernommen.
Für die zusätzlichen Wünsche wird daher keine vermeintlich passende
Standardlizenz behauptet.

**GUI-spezifische Grenze:** Die bloße Herausnahme fremder Bibliotheken aus
dem Geltungsbereich der eigenen Bedingungen beweist keine Vereinbarkeit.
Insbesondere LGPLv3 § 4 verlangt auch für die Bedingungen des kombinierten
Werks, dass bestimmte Änderungs- und Reverse-Engineering-Rechte nicht
eingeschränkt werden. Die beabsichtigte nicht-kommerzielle Lizenzierung
der Kombination ist deshalb rechtlich zu prüfen. Installierter oder
übernommener GPL-Code könnte weitere Unvereinbarkeiten auslösen.
Der Entwurf darf solche Fremdrechte nicht begrenzen.

Der Entwurf bestätigt nicht, dass die Lizenznutzung bereits abschließend
geklärt ist: Er bezeichnet die eigene Lizenzentscheidung, konkrete Versionen,
transitive Lizenzen und Verteilungsbeigaben selbst als offen. Eine proprietäre
GUI kann mit LGPL-Bibliotheken grundsätzlich vereinbar sein; allein die
dynamische Verlinkung belegt jedoch nicht die Einhaltung aller Bedingungen.
Es wurde keine pauschale Unzulässigkeit der bisherigen Nutzung festgestellt.
Interne Nutzung und Weitergabe eines konkreten Pakets sind getrennt zu bewerten.

| Punkt | Prüfergebnis | Status |
|---|---|---|
| IDEE-G-00006: Qt-/VTK-Versionen und Lizenzfreigabe | Installierte Versionen ermittelt; Freigabe und vollständiger Paketnachweis fehlen | offen |
| U-G-000001: Projekt-/Buildbasis | Technische Umsetzung bleibt unverändert; Lizenzvorbehalt bleibt bestehen | umgesetzt; Lizenzprüfung offen |
| U-G-000010: Integration/Abnahme | Vorhandene technische und Sichtabnahme wird nicht geändert; Lizenzvorbehalt bleibt bestehen | Lizenzfreigabe offen |

Es wird kein Entwurfspunkt geschlossen und keine neue A-/U-Kennung vergeben.

### Grafikarbeit G-0.5

Die neuen Wasser-, Stoff-, Holz- und Metallstrukturen sowie die
Standardumgebung werden im GUI-Code prozedural erzeugt. Es wurde kein
externes Textur- oder HDR-Paket in das Repository übernommen. Die explizite
Materialvorschau verändert keine Modell-/Core-Dateien.
Die anschließend ergänzte dauerhafte Materialzuordnung benutzt
Oberflächenkennungen im Core-Darstellungsvertrag und im Folkeboot-Modell.
Auch dafür werden keine fremden Bilddateien übernommen: Wasser,
Gewebemuster, Holzmaserung und Metallstruktur entstehen prozedural im
GUI-Code. Die zugehörigen separaten Core-/Modelldateiänderungen sind
technische Vertrags-/Metadatenänderungen, keine neue Lizenzfreigabe.

Der HDR-Dateidialog referenziert eine vom Nutzer gewählte lokale Datei;
er kopiert oder veröffentlicht sie nicht. Für die spätere Weitergabe
einer solchen Datei ist ein eigener Herkunfts- und Rechtenachweis nötig.
Die HDR-Lesefunktion nutzt das Modul `VTK::IOImage` derselben bereits
installierten VTK-Version, kein neu aufgenommenes Fremdpaket.
Die obige Paketprüfung ist weiterhin keine Freigabe eines künftigen
G-0.5-Verteilpakets; dessen tatsächliche Dateiliste und Beigaben sind
erneut zu prüfen.

## 2. Tatsächlich verwendeter Build

### Ergänzung: Navigation aus dem Altprojekt

Für den anschließend beauftragten Nachbau von Kompass, Krängung und
Stampfen wurden drei Original-SVGs aus `BergStefanBerlin/SegelPhysik`
übernommen. Sie sind keine neu erzeugten prozeduralen Texturen.
Commit, Originalquellen, lokale Dateinamen und SHA256-Nachweise stehen in
der [Navigationsdokumentation](Navigation.md#übernommene-quellen).
Die C++-Bindung orientiert sich an der dort verlinkten Python-Implementierung;
eine Python-Laufzeit wird nicht eingebunden.

Das Manifest führt dafür zusätzlich `qtsvg`; der Build verlinkt
`Qt6::Svg` und `Qt6::Xml`. Die Pakete waren bereits lokal installiert,
werden jetzt aber unmittelbar von der GUI verwendet. Sie sind deshalb bei
Dateiliste, Modulhinweisen und Lizenzbeigaben eines späteren Verteilpakets
mitzuprüfen. Die Herkunft aus dem eigenen Altprojekt ist keine automatische
Lizenz- oder Verteilungsfreigabe. Der offene Lizenzstatus bleibt bestehen.

Der Cache `C:\spdeps\guibuild\CMakeCache.txt` verweist auf:

- GUI-Quellordner dieses Repositories und den separaten CORE-Checkout;
- `VCPKG_INSTALLED_DIR = C:\spdeps\installed`;
- `VCPKG_TARGET_TRIPLET = x64-windows`;
- Qt aus `C:\spdeps\installed\x64-windows\share\Qt6`;
- VTK aus `C:\spdeps\installed\x64-windows\share\vtk`.

Der repositorylokale Ordner `vcpkg_installed` ist deshalb nicht die
maßgebliche Paketquelle für diesen Build.
Das Manifest fixiert die Baseline
`3af1d1e60af2b2abf55760538cd607829029b07a`.
Die in der [README](../README.md) beschriebenen lokalen Overlay-Ports
für libaec/gl2ps gehören ebenfalls zur Herkunftsprüfung; die Baseline allein
beschreibt diese Abweichungen nicht vollständig.

Im Build-Ausgabeverzeichnis `C:\spdeps\guibuild\Release` liegen 70 DLLs,
darunter `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6OpenGL.dll`,
`Qt6OpenGLWidgets.dll` und `Qt6Widgets.dll`.
Die Plugins `platforms\qwindows.dll` und
`styles\qmodernwindowsstyle.dll` liegen separat vor.
Das stützt die dynamische Qt-Einbindung. Ein Versuch mit kompatibel
modifizierten Ersatz-DLLs wurde nicht durchgeführt.
Ein freigegebenes Verteilpaket samt vollständiger Dateiliste liegt dieser
Prüfung nicht zugrunde. Im Release-Ausgabeverzeichnis wurden keine unmittelbar
erkennbaren LICENSE-/COPYING-/NOTICE-Beigaben gefunden; daraus folgt keine
Aussage über ein anderweitig zusammengestelltes Paket.

## 3. Versionsinventar und Grenzen der Lizenzmetadaten

Quelle: `C:\spdeps\installed\vcpkg\status`, ausschließlich Basiseinträge mit
`Status: install ok installed`, ohne Feature-Doppelzählung und ohne
deinstallierte Pakete. Ergebnis: **41 installierte Basispakete**, nicht 28.
`#n` bezeichnet die vcpkg-Portrevision, nicht eine zusätzliche Upstream-Version.

Die dritte Spalte übernimmt `licenseConcluded` des zum Paket gehörenden
Eintrags aus `share\<paket>\vcpkg.spdx.json`. Das ist ein Metadatenbefund,
keine eigenständige Lizenzfreigabe. `NOASSERTION` und `LicenseRef-vcpkg-null`
werden nicht als Lizenz oder Erlaubnis interpretiert; Copyright-Dateien,
Unterkomponenten und tatsächlicher Einsatz müssen zusätzlich geprüft werden.

| Paket | Installierte Version | SBOM: licenseConcluded |
|---|---|---|
| vcpkg-cmake-config | 2024-05-23 | MIT |
| vcpkg-cmake | 2024-04-23 | MIT |
| sqlite3 | 3.51.0 | blessing |
| egl-registry | 2024-01-25 | NOASSERTION |
| opengl-registry | 2024-02-10#1 | NOASSERTION |
| opengl | 2022-12-04#3 | LicenseRef-vcpkg-null |
| double-conversion | 3.3.1 | NOASSERTION |
| zlib | 1.3.1 | Zlib |
| pcre2 | 10.47 | BSD-3-Clause |
| qtbase | 6.9.1#1 | LicenseRef-vcpkg-null |
| qtsvg | 6.9.1 | LicenseRef-vcpkg-null |
| qtshadertools | 6.9.1 | LicenseRef-vcpkg-null |
| qtlanguageserver | 6.9.1 | LicenseRef-vcpkg-null |
| qtdeclarative | 6.9.1 | LicenseRef-vcpkg-null |
| vtk-compile-tools | 9.3.0-pv5.12.1#1 | LicenseRef-vcpkg-null |
| verdict | 1.4.2 | LicenseRef-vcpkg-null |
| utfcpp | 4.0.6 | BSL-1.0 |
| liblzma | 5.8.1 | LicenseRef-vcpkg-null |
| libjpeg-turbo | 3.1.2 | BSD-3-Clause |
| tiff | 4.7.1 | libtiff |
| pugixml | 1.15#1 | MIT |
| pegtl | 3.2.8 | MIT |
| nlohmann-json | 3.12.0#1 | MIT |
| lz4 | 1.10.0 | BSD-2-Clause |
| libiconv | 1.18#3 | LicenseRef-vcpkg-null |
| libxml2 | 2.15.0 | MIT |
| libpng | 1.6.50 | libpng-2.0 |
| jsoncpp | 1.9.6 | MIT |
| libaec | 1.1.3#1 | BSD-2-Clause |
| hdf5 | 1.14.6 | BSD-3-Clause |
| glew | 2.2.0#6 | NOASSERTION |
| gl2ps | 1.4.2#4 | NOASSERTION |
| bzip2 | 1.0.8#6 | bzip2-1.0.6 |
| brotli | 1.2.0 | MIT |
| freetype | 2.13.3 | (FTL OR GPL-2.0-or-later) |
| fmt | 12.1.0 | MIT |
| fast-float | 8.1.0 | (Apache-2.0 OR BSL-1.0 OR MIT) |
| exprtk | 0.0.3 | MIT |
| expat | 2.7.3 | MIT |
| eigen3 | 3.4.1#1 | MPL-2.0 |
| vtk | 9.3.0-pv5.12.1#12 | LicenseRef-vcpkg-null |

Installiert bedeutet nicht automatisch verlinkt oder verteilt. Insbesondere
sind Buildwerkzeuge, Headerbibliotheken, Laufzeit-DLLs und mitgelieferte
Werkzeuge getrennt einzuordnen. In Bibliotheken eingebetteter Drittcode wird
durch diese Portliste nicht vollständig erfasst.

### Zusätzliche Befunde

- **Qt:** Die im Build vorgefundenen Qt-Base-Laufzeitmodule können unter
  LGPLv3 genutzt werden, sofern deren Bedingungen erfüllt sind. Weitere
  Qt-Pakete sind tatsächlich installiert; `default-features: false` im
  GUI-Manifest verhindert nicht sämtliche transitiven Qt-Abhängigkeiten.
  Deren Lizenzierung und etwaige Weitergabe sind separat zu prüfen.
- **VTK:** `share\vtk\copyright` enthält die BSD-3-Clause-Bedingungen des
  VTK-Hauptprojekts und verweist zusätzlich auf `share\vtk\licenses`.
  BSD-3-Clause allein deckt somit nicht automatisch sämtlichen Drittcode ab.
- **libiconv:** Die lokale Copyright-Datei unterscheidet LGPL-Bibliotheken
  (`COPYING.LIB`, LGPL 2.1) und GPL für Programm/Dokumentation.
  Eine installierte GPL-Anwendung beweist keine GPL-Pflicht für die GUI.
- **gl2ps:** Die lokale Copyright-Datei nennt die GNU Library General Public
  License sowie eine alternative GL2PS-Lizenz. Die gewählte Alternative und
  deren Bedingungen sind vor einer betroffenen Verteilung festzuhalten.
- **Eigen:** MPL-2.0 ist nicht pauschal als permissive Lizenz zu behandeln.
  Relevant sind die tatsächlich betroffenen Dateien und deren Weitergabe.
- **FreeType/fast-float:** Lizenzalternativen sind sichtbar; die für die
  Verteilung verwendete Alternative darf nicht stillschweigend angenommen werden.

## 4. Korrekturen und Ergänzungen zum Lizenzentwurf

1. Dynamisches Linken ist ein geeigneter technischer Weg für LGPLv3,
   erfüllt aber allein weder Hinweis-, Lizenztext- noch Quellcodepflichten.
   Neben dem LGPLv3-Text ist auch der GPLv3-Text zu berücksichtigen.
2. Der Bezugsweg muss zum tatsächlich verwendeten Bibliotheksquellcode
   einschließlich angewandter Änderungen/Patches passen und die einschlägigen
   Bereitstellungspflichten erfüllen. Ein allgemeiner Qt-Downloadlink allein
   ist dafür kein vollständiger Nachweis.
3. Bedingungen für das Gesamtprodukt dürfen Änderungen an LGPL-Bestandteilen
   und Reverse Engineering zur Fehlersuche solcher Änderungen nicht untersagen.
   Austausch und Ausführung kompatibler modifizierter Bibliotheken sowie
   gegebenenfalls nötige Installationsinformationen sind zu berücksichtigen.
4. Statisches Linken ist nicht pauschal verboten. Es erfordert eine gesonderte
   Prüfung insbesondere der Relink-Pflichten; die vorgefundene dynamische
   Einbindung wird hier nicht geändert.
5. GPL-Buildwerkzeuge, GPL-Bibliotheken im Produkt und mehrfach lizenzierte
   Module sind zu unterscheiden. Die pauschale Modulliste im Entwurf ersetzt
   keine versionsbezogene Prüfung; insbesondere darf Qt WebEngine nicht ohne
   Prüfung seiner eigenen und seiner Drittkomponenten-Lizenzen pauschal als
   ausschließlich GPLv3 eingeordnet werden.
6. Eine noch fehlende eigene Projektlizenz wird nicht durch Fremdlizenzen
   ersetzt. Die Rechteinhaber müssen die beabsichtigten GUI-Nutzungs- und
   Verteilungsbedingungen entscheiden. Für CORE ist dies separat zu klären.

## 5. Noch offene Abschlusskriterien

Diese Liste konkretisiert die fehlenden Nachweise zu IDEE-G-00006 und enthält
keine neue Anforderung oder parallelen Umsetzungsplan.

- [x] Gewünschte eigene GUI-Lizenzrichtung mit dem Nutzer klären und dokumentieren:
  nicht-kommerziell, Bildungsausnahme, kostenlose Weitergabe mit Quellcode.
- [ ] Rechteinhaber/Zustimmungen bestätigen, Bedingungenentwurf rechtlich prüfen
  und wirksame, mit den Fremdlizenzen vereinbare Nutzungsbedingungen freigeben;
  keine Entscheidung für CORE vorwegnehmen.
- [ ] Konkretes Verteilpaket mit Versionen, Herkunft, DLLs, Plugins, eingebettetem
  Drittcode und gegebenenfalls Werkzeugen inventarisieren; Build-only-Anteile
  abgrenzen.
- [ ] Lizenzalternativen und Pflichten sämtlicher betroffener Komponenten
  prüfen und freigeben, einschließlich Qt-/VTK-Drittcode und unklarer SBOMs.
- [ ] Erforderliche Lizenztexte, Copyright- und NOTICE-Hinweise beilegen;
  bei LGPLv3 auch den GPLv3-Text berücksichtigen.
- [ ] Passenden Quellcode einschließlich Patches und einen rechtlich geeigneten
  Bereitstellungsweg nachweisen.
- [ ] Austausch/Start mit kompatibel modifizierten Qt-DLLs prüfen, zulässige
  Nutzerrechte sicherstellen und gegebenenfalls Installationsinformationen liefern.
- [ ] Verteilpaket abschließend prüfen und freigeben; erst danach
  IDEE-G-00006 und die zugehörigen Lizenzvorbehalte konsistent schließen.

Der [GitHub-Eintrag](https://github.com/BergStefanBerlin/SegelPhysik-GUI/issues/1)
führt diese offenen Kriterien. Bei seiner Anlage existierten keine anderen
Issues im GUI-Repository. Die übrigen offenen GUI-Abnahmepunkte bleiben
unverändert. Diese Prüfung umfasst keine fachliche Core-Abnahme.

## 6. Quellen und Nachweisgrenzen

- [GNU LGPLv3](https://www.gnu.org/licenses/lgpl-3.0.html), insbesondere §§ 3/4.
- [Qt-Lizenzübersicht](https://doc.qt.io/qt-6/licensing.html).
- [Qt: LGPL-Verpflichtungen](https://www.qt.io/development/open-source-lgpl-obligations).
- Lokaler CMake-Cache, Paketstatus, paketbezogene `vcpkg.spdx.json`- und
  `copyright`-Dateien unter den in Abschnitt 2/3 genannten Pfaden;
  Dateiinventar des Release-Ausgabeverzeichnisses am Prüftag.

Die lokalen Dateien sind veränderliche Nachweise, kein archiviertes und
gehashtes Verteilpaket. Die Port-SBOMs ersetzen keine vollständige
Produkt-SBOM. Dokumentation und Verweise wurden strukturell geprüft;
kein neuer Build, keine Laufzeittests und keine juristische Gesamtabnahme.
Diese technische Prüfung ist keine Rechtsberatung.
