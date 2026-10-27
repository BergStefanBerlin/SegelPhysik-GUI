# Umsetzungsplan G-0.5: Grafikprofile, Setup und Fahrtdarstellung

**Status: technische Implementierung einschließlich dauerhafter
CORE-/Folkeboot-Materialien integriert und automatisiert geprüft;
persönliche Sicht- und Release-Leistungsabnahme getrennt.
Erweiterung Testantrieb/Fahrtdarstellung technisch implementiert und gezielt nachgewiesen
und ohne Implementierungsfreigabe.**  
**Stand:** 27.10.2026 · **Zielversion:** G-0.5 (korrigierte Nutzerentscheidung)  
**Referenzmodell:** Folkeboot (Nutzerentscheidung)

**Maßgebliche Spezifikationen nach Thema:**
[Grafikprofile und Setup](../Spezifikationen/Spezifikation_GUI_Grafikprofile_und_Setup.md)  
[Testantrieb und Fahrtdarstellung](../Spezifikationen/Spezifikation_GUI_Testantrieb_und_Fahrtdarstellung.md)
(finalisiert; neuer Umfang in Abschnitt 9 dieses Plans)  
**Entwurfspunkte:**
[GUI_Grafikprofile_und_Setup.json](../Spezifikationen/Entwurfspunkte/GUI_Grafikprofile_und_Setup.json)

Dieser Plan ist der einzige maßgebliche Umsetzungsplan für G-0.5.
Er enthält sämtliche hier geplanten Arbeiten, Abhängigkeiten, Meilensteine
und Nachweisstände. Die Implementierung wurde nach dem Planauftrag erneut vollständig beauftragt.
Die Anforderungs- und Umsetzungsschrittzuordnung steht in Abschnitt 6.
Die Bezeichnungen der Arbeitspakete unten werden durch diese Zuordnung
eindeutig referenziert. Offene Abnahmeverträge bleiben sichtbar.
Die frühere Implementierungsfreigabe betrifft den damaligen Grafikumfang,
nicht automatisch die neue Erweiterung in Abschnitt 9. Deren Auftrag vom
27.10.2026 gilt ausschließlich der Planung.

G-0.2 und dessen offene Abnahmen bleiben im
[bestehenden Plan](Umsetzungsplan_G-0.2.md).
CORE und Modelle sind getrennte Lieferbereiche. Nach erneutem ausdrücklichem
Fortsetzungsauftrag wird der verbliebene Materialvertrag dort gezielt und
getrennt ergänzt; der GUI-Plan ersetzt keine Core-Spezifikation und
autorisiert weiterhin keine Commits oder Veröffentlichungen.

Die zunächst gewählte Zielversion G-0.3 wurde am 27.10.2026 auf
Nutzerhinweis zu G-0.5 korrigiert: G-0.3 und G-0.4 sind bereits als
GUI-Tags vorhanden. Der noch unveröffentlichte Planentwurf wurde umbenannt,
nicht ein historischer Versionsplan geändert.

## 1. Ziel und verbindliche Umfangsquelle

Ziel ist der vollständige Umfang der Grafik-Spezifikation: Profile Technik
und Optik am vorhandenen Platz bei der Vektorauswahl, Setup → Grafik als
erster Setup-Eintrag, 28 separat schaltbare Features, profilbezogene Parameter,
dauerhafte Einstellungen und vollständige Effektvorführung am Folkeboot.
Die Defaultwerte und fachlichen Verträge stehen ausschließlich in der
Spezifikation; dieser Plan dupliziert sie nicht.

Vorhandene PBR-/Wasser-/Himmels-/Material-/SSAO-/FXAA-Funktionen werden
integriert und schaltbar gemacht, nicht parallel neu implementiert.
Maximalumfang bedeutet insbesondere auch neue Normalstrukturen,
Texturmaterialien, Schatten und Bootsspiegelung zusätzlich zu IBL.
Teilumsetzung oder nicht verfügbare Effekte bleiben als solche sichtbar.

### Mitprüfung noch offener Bestandspunkte

Am 27.10.2026 wurden die vorhandenen Entwurfspunktelisten und offenen
G-0.2-Nachweise auf sinnvolle Überschneidungen geprüft. Die folgenden
Teile werden in die bestehenden Arbeitspakete integriert, nicht als
zweiter Plan geführt. Ursprungskennungen und historische Status bleiben
erhalten; Aufnahme in den Plan bedeutet keine Erfüllung oder Abnahme.

| Herkunft | In G-0.5 sinnvoll mitbehandeln | Zuständiges Arbeitspaket / Nachweis |
|---|---|---|
| IDEE-G-00003: Darstellungsqualität und Wasser-Schnitt | Qualitätskriterien für Schnittkontur, Wasserüberlagerung und Sichtbarkeit eingetauchter Teile in beiden Profilen konkretisieren; Auflösung/Bildrate gemeinsam mit IDEE-G-00019 festlegen. Kein zusätzlicher Volumenschnitt oder neue Einfärbung ohne fachliche Entscheidung. | Wasser und Umgebung; Gesamtintegration und Leistungsnachweis; Spezifikation Abschnitte 5/7/9 |
| IDEE-G-00014; A-G-00012/A-G-00014/A-G-00015 | Offene DPI-, Picking- und Hebelarmnachweise mit der neuen Renderpipeline kombinieren: 100/150/200 %, beide Profile, Verdeckungsarten und überlagerte Vektoren prüfen. | Setup und Profilumschalter; Schatten und kombinierte Renderpipeline; Gesamtintegration und Leistungsnachweis |
| IDEE-G-00014; A-G-00018 | Laufzeitvergleich Anzeige ein/aus um Technik/Optik ergänzen; ausstehende Grenzwerte zusammen mit IDEE-G-00019 entscheiden, bestehende Verträge nicht abschwächen. | Freigabe und Verträge; Gesamtintegration und Leistungsnachweis |
| IDEE-G-00005; Fehler-/Abbruchnachweise aus A-G-00006 | Vorhandene Stop-/Fehler-/Teilaufzeichnungspfade als Regression während Profilwechsel und geöffnetem Setup prüfen; Teilzustände und Laufstatus dürfen nicht durch Grafikänderungen verloren gehen. Keine neue Abbruch- oder Sitzungs-API. | Gesamtintegration und Leistungsnachweis; Spezifikation Abschnitt 9 |
| IDEE-G-00006: Lizenzprüfung | Herkunft und Lizenzbeigaben neuer Grafikassets sowie Änderungen des tatsächlichen Abhängigkeitsumfangs nachführen. Keine juristische Freigabe als Grafikarbeit behaupten. | Folkeboot-Materialien; Fachliche Abnahme und Abschluss; Spezifikation Abschnitt 9 |

**Bewusst nicht erweitert:** IDEE-G-00001 (Elementauswahl/Mehrmodellansicht),
IDEE-G-00002 (neue Lauf-/Wiedergabeparameter), IDEE-G-00007 (neue
Laufzeit-Diagnoseoberfläche) und IDEE-G-00011 (Wirkungslinie mit Restmoment).
Diese Themen sind keine Voraussetzung der Grafikprofile und würden eigene
fachliche Verträge erfordern. Aus IDEE-G-00004 werden keine vermeintlich
neuen Momenten-/Filterfunktionen abgeleitet: Die spätere
Kraft-/Momentenspezifikation beschreibt bereits deren Umsetzung; hier
werden nur die betroffenen Darstellungsfunktionen regressionsgeprüft.

## 2. Voraussetzungen und Freigabesperren

| Voraussetzung | Tatsächlicher Stand | Wirkung |
|---|---|---|
| Zielversion G-0.5 | entschieden | Planung möglich; keine Implementierungsfreigabe |
| Folkeboot als Referenz für alle Effekte | entschieden | Keine Ersatzabnahme ausschließlich an einfachen Körpern |
| Spezifikation zur Implementierung angenommen, A-/U-Zuordnung erstellt | erneuter vollständiger Umsetzungsauftrag; Zuordnung unten | Implementierung begonnen, keine behauptete Abnahme |
| Defaultmatrix und maximaler Reflexionsumfang | mit erneutem vollständigen Auftrag zur Implementierung angenommen, IDEE-G-00021 | Effekt-/Sichtnachweise weiterhin erforderlich |
| Referenzhardware, Auflösung, Szenen und messbare Leistungsgrenzen festgelegt | AMD Radeon 760M / Ryzen 5 8600G; 1280 × 720; Grenzwerte offen, IDEE-G-00019 | Werte messen; kein ungeprüftes 30-Hz-Versprechen oder behauptete Leistungsabnahme |
| Folkeboot-Material-/UV-Vertrag und benötigte externe Lieferungen abgegrenzt | getrennte rückwärtskompatible Core-Oberflächenwerte und explizite Folkeboot-Darstellungszuordnung implementiert und geprüft; prozedurale lokale Projektion statt allgemeinem UV-Import | Keine erratene Materialzuordnung oder Physikänderung |
| Herkunft und Rechte neuer Assets | noch kein freigegebenes Assetpaket | Ungeklärte Assets nicht übernehmen oder verteilen |
| GUI-/CORE-Basis und vorhandene Tests dokumentiert | GUI `8671f2e`, CORE `61e78de`; native Release-Suite einschließlich Profil-/Dialogtests 19/19 erfolgreich | Änderungen und zusätzliche Rendernachweise separat bewerten |

Die bestehende Lizenzfreigabe IDEE-G-00006 bleibt separat offen. Sie
blockiert keine lokale technische Entwicklung, aber eine davon betroffene
Verteilung darf nicht als freigegeben dargestellt werden.
Fehlende fachliche Core-Abnahmen werden durch diesen Plan nicht geschlossen.

## 3. Arbeitspakete und Abhängigkeiten

Der aktuelle Ausführungsstand steht in Abschnitt 8. Abhängigkeiten sind vollständige
fachliche Voraussetzungen, nicht bloß eine Reihenfolge der Tabellenzeilen.
Mit „Freigabe“ ist der Abschluss des ersten Pakets gemeint.

| Arbeitspaket | Ergebnis und Spezifikationsbezug | Abhängig von | Nachweis / Abschluss |
|---|---|---|---|
| Freigabe und Verträge | Offene Entwurfspunkte entscheiden, Spezifikation finalisieren, A-/U-Zuordnung in diesem Plan herstellen; externe Core-/Modelllieferungen und deren Abnahmekriterien festhalten. Abschnitte 1, 4, 6, 7, 9. | Nutzerentscheidungen; getrennte Abstimmung externer Verträge | Finale Spezifikation, eindeutige Rückverfolgbarkeit, freigegebene Messgrenzen |
| Ausgangsstand und Referenzdaten | Bestehende Tests/Build erfassen; Folkeboot-Geometrie, Materialflächen, Simulierbarkeit, trockene Innenräume und benötigte Details prüfen; feste Kameras und Messfälle erfassen. Abschnitte 5–7, 9. | Freigabe | Reproduzierbarer Ausgangsstand, sichtbare Lücken, keine erfundenen Simulationsergebnisse |
| Profilmodell und Renderzustand | Zwei unabhängige typisierte Profile mit vollständiger Feature-/Parametermatrix, Validierung, Abhängigkeiten und getrenntem Wunsch-/Wirksamzustand; vorhandene Effekte anbinden. Abschnitte 2, 4, 7. | Ausgangsstand und Referenzdaten | Tests aller Defaultwerte, Konflikte, nicht unterstützter Features und Zustandswechsel |
| Persistenz und Migration | Versioniertes QSettings-Schema für beide Profile und aktives Profil; Alt-Sichtmodus einmalig migrieren, Fehler und Schreibstatus explizit behandeln. Abschnitt 8. | Profilmodell und Renderzustand | Prozessneustart mit isolierten Einstellungen, Migration, Schreib-/Lesefehler, keine Änderung bestehender Nutzerwerte |
| Setup und Profilumschalter | Bestehenden Auswahlplatz erhalten; Setup → Grafik, zwei unabhängige Profilbereiche, Einzelschalter/Parameter, Sammelaktionen und Dialogtransaktionen implementieren. Abschnitte 2–4, 8. | Persistenz und Migration | Bedien-/Dialogtests für Übernehmen, OK, Abbrechen, Reset und Wechsel ohne Verlust von Vektorauswahl oder Laufzustand |
| Wasser und Umgebung | Vorhandene Flächen, Fresnel, Himmel/IBL und Dunst über Profile steuern; Normalstruktur mit stabiler Weltabbildung, Filterung und reproduzierbarer Animation ergänzen. Abschnitte 4–5. | Profilmodell und Renderzustand | Ober-/Unterwasser, Nah/Fern, Krängung, trockene Innenräume und Zeit-/Rückblicktests |
| Folkeboot-Materialien | Lack, Stoff, Holz und Metall samt Textur-/Normal-/ORM-/Anisotropiefunktionen integrieren; geeignete UVs/Tangenten und eindeutige Zuordnung verwenden. Abschnitt 6. | Profilmodell und Renderzustand; freigegebene Core-/Modelllieferungen aus Freigabe und Verträge | Sichtvergleich aller Materialarten am Folkeboot; vorhandene Modelle ohne neue Angaben bleiben darstellbar |
| Szenenreflexion | Zusätzliche ebene Bootsspiegelung getrennt von IBL, mit Clipping und Ausschluss rekursiven Wassers sowie fachlicher Hilfen integrieren. Abschnitt 5. | Wasser und Umgebung; Folkeboot-Materialien | Reflexion folgt Pose/Kamera; keine Wasserfüllung trockener Bereiche, keine gespiegelt verfälschten Vektoren |
| Schatten und kombinierte Renderpipeline | Sonnenlicht und Schatten integrieren; Zusammenwirken mit SSAO, FXAA, Transparenz, Reflexion und Overlays prüfen; abschaltbare Effekte verursachen keine vermeidbaren Passkosten. Abschnitte 4, 7. | Szenenreflexion; Folkeboot-Materialien | Kombinierte Bild-/Interaktionstests statt bloß isolierter Effekttests |
| Qualitätssteuerung und Fehleranzeige | Qualitätsparameter und sichtbare Hardware-/Assetdiagnosen integrieren; optionale Live-Reduktion gemäß freigegebener Regel, ohne Wunschprofile zu überschreiben. Abschnitte 3–4, 7–8. | Setup und Profilumschalter; Schatten und kombinierte Renderpipeline | Gewünschter/wirksamer Zustand unterscheidbar, keine stille Deaktivierung und keine Änderung der Core-Aufzeichnung |
| Gesamtintegration und Leistungsnachweis | Profilwechsel im Lauf/Rückblick, alle Features einzeln und gemeinsam, Parameter und Persistenz prüfen; Messungen gegen freigegebene Grenzen durchführen. Abschnitte 2–9. | Qualitätssteuerung und Fehleranzeige | Bestehende Tests plus neue Prüfungen bestanden; Messbericht mit Abweichungen |
| Fachliche Abnahme und Abschluss | Folkeboot-Vorführung für Technik/Optik, vollständigen Anforderungs-/Nachweisabgleich und Herkunftsbelege erstellen; offene Punkte sichtbar lassen. Abschnitt 9. | Gesamtintegration und Leistungsnachweis | Getrennte Nutzerabnahme beider Profile; Abschluss nur für tatsächlich belegte Anforderungen |

Nach Profilmodell/Persistenz können die Bedienoberfläche und die
Render-Erweiterungen unabhängig voranschreiten. Ein blockierter
Texturdatenvertrag darf keine GUI-eigene Modelldatei-Auswertung als
Abkürzung auslösen.

## 4. Meilensteine

| Meilenstein | Eintritt | Überprüfbare Abnahme | Stand |
|---|---|---|---|
| Freigegebener Umfang | erneuter vollständiger Umsetzungsauftrag | GUI-Verträge angenommen, A-/U-Verweise eingetragen; externe Lieferungen und Leistungsabnahme nicht als freigegeben behandeln | GUI-Ausführung begonnen; Detail-/Abnahmeverträge offen |
| Gespeicherte Profile und Bedienung | Freigegebener Umfang; unveränderte Ausgangsbasis erfasst | Profilmodell, Setup, Umschalter, echte Neustartpersistenz und Fehlertests bestanden | automatisierte Nachweise erfolgreich |
| Vollständige Folkeboot-Effekte | Freigegebener Umfang; erforderliche externe Lieferungen vorhanden | Wasser, Materialien, Schatten und zusätzliche Objektreflexion einzeln und kombiniert nachgewiesen | dauerhafte Materialien und kombinierte GPU-Prüfung erfolgreich; persönliche Sichtabnahme getrennt |
| Technische Integration | Gespeicherte Profile und Bedienung; vollständige Folkeboot-Effekte | Regressionstests, Hardware-/Assetfälle und Leistungsgrenzen bestanden; keine Physik-/Ergebnisänderung | 22/22 Tests einschließlich GPU/DPI/Picking und Live-Ergebnisvergleich erfolgreich; verbindliche Leistungsgrenzen offen |
| Fachliche Abnahme G-0.5 | Technische Integration | Nutzer bestätigt Technik und Optik; aktive Anforderungen vollständig mit tatsächlichen Belegen abgeglichen | offen |

Eine nachgewiesene Hardwareeinschränkung wird dokumentiert, ersetzt aber
nicht die Vorführung aller Features auf der vereinbarten Referenzhardware.
Wenn Grenzen nicht erreichbar sind, erst optimieren und erneut messen.
Eine Umfangs- oder Grenzänderung benötigt eine Nutzerentscheidung, keine
stille Reduktion der Abnahmekriterien.

## 5. Prüf- und Nachweisstrategie

### Automatisierte Prüfungen

- Vorhandene native GUI-Tests über die bestehende CMake-/CTest-Konfiguration
  nutzen; Qt-Tests bei Bedarf mit `QT_QPA_PLATFORM=offscreen`.
  GPU-bezogene Bildnachweise zusätzlich in einer echten unterstützten
  OpenGL-Umgebung durchführen; offscreen Unit-Tests beweisen keine GPU-Qualität.
- Zuerst gezielte Tests für Profilmodell, Settings und Dialogzustände,
  danach die betroffenen GUI-Integrationstests und zum Integrationsmeilenstein
  die vorhandene native Testsuite ausführen.
- Alle 28 Features einzeln Ein/Aus, Defaults und Parameter testen;
  profilübergreifende Unabhängigkeit und Grunddarstellung bei „Alle aus“ prüfen.
- Abhängigkeiten/Konflikte sowie alle teuren Effekte zusammen testen.
  Eine vollständige Aufzählung aller möglichen Schalterkombinationen ist
  kein praktikabler Ersatz für gezielte Interaktionsfälle.
- Einstellungen in einem isolierten Testbereich speichern; zwei getrennte
  Prozesse für Schreiben und Lesen, Migration, ungültige Daten, fehlende
  Assets und nicht schreibbaren Speicher prüfen.
- Bei Profilwechsel Auswahl, Kraft-/Momentenwerte, Skalenreferenzen,
  Kamerapose und Wiedergabezeitpunkt vergleichen.
- Vorhandene Stop-, Fehler- und Teilaufzeichnungspfade mit reproduzierbaren
  Eingaben prüfen: Profil vor/während/nach Abbruch wechseln, Setup offen
  halten und anschließend verfügbare Zustände samt Status im Rückblick
  vergleichen. Fehlende Referenzfälle bleiben explizit offen; Fehler nicht
  durch fingierte Erfolgsergebnisse oder neue Core-Verträge umgehen.

### Folkeboot-Vorführung und Bildnachweise

Für Technik und Optik dieselben Modellfassungen, Zustände, Kameras,
Fenstergrößen und DPI-Werte benutzen. Mindestens Gesamtansicht, Nahaufnahme
von Lack/Stoff/Holz/Metall, flacher Horizontblick, Draufsicht, Unterwasseransicht,
gekrängte Pose, trockener Innenraum und dynamischer Rückblick prüfen.
Für jedes Feature einen Ein-/Aus-Vergleich samt wirksamen Parametern ablegen.
„Nicht anwendbar“ wegen fehlender Referenzgeometrie gilt als offener Nachweis.

Bei 100/150/200 % Skalierung die bestehenden festen Vektorgrößen und
Trefferzonen gemäß Kraft-/Momentenspezifikation messen; Auswahl,
Mehrfachtreffer, Hebelarm und Synchronisation zum Wertefenster in beiden
Profilen prüfen. Reflexion, Schatten und transparente Körper dürfen keine
zusätzlichen auswählbaren Vektorkopien oder falschen Treffer erzeugen.
Eine erfolgreiche Sichtprüfung ersetzt weder Größenmessung noch
die separate Nutzerabnahme.

Der tatsächliche Folkeboot-Zustand entscheidet über mögliche dynamische
Prüfungen. Ist das Modell nicht simulierbar, bleibt die Folkeboot-Live-Abnahme
blockiert; eine separat freizugebende Modell-/Core-Korrektur ist erforderlich.
Ein anderes Modell darf frühere technische Tests unterstützen, ersetzt aber
nicht die beauftragte Folkeboot-Gesamtvorführung.

### Leistung und Reproduzierbarkeit

Hardware/GPU, Treiber, Betriebssystem, Buildkonfiguration, GUI-/CORE-Revision,
Modellfassung, Auflösung, Profilwerte und Kamerafahrt protokollieren.
Nach Aufwärmung mehrere Wiederholungen mit identischen Eingaben messen:
Median/P95 der Bildzeiten, Lade-/Startzeit, Speicher sowie Core-Laufzeit mit
und ohne Darstellung. Messdauer, Wiederholungszahl und numerische Grenzen
vor Finalisierung festlegen. Protokollierte Rohdaten dürfen separat abgelegt
werden; Anforderungen, Arbeitspakete und deren Status bleiben in Spezifikation
und diesem Plan.

Der Vergleich umfasst dieselbe Simulation ohne Darstellung, mit Technik
und mit Optik einschließlich aller Effekte; eine gegebenenfalls eingeschaltete
Live-Reduktion wird als eigener Messfall geführt. Die bisherigen getrennten
Zeitbegriffe bleiben unverändert. Eine neue Diagnoseoberfläche ist nicht
Teil dieses Nachweises.

## 6. Vorläufige Rückverfolgbarkeit

Fortlaufende Zuordnung bei Umsetzungsbeginn (nach U-G-000024):

| Umsetzungsschritt | Arbeitspaket aus Abschnitt 3 | Anforderungen |
|---|---|---|
| U-G-000025 | Freigabe und Verträge | A-G-00030, A-G-00032, A-G-00033, A-G-00036 |
| U-G-000026 | Ausgangsstand und Referenzdaten | A-G-00032, A-G-00033, A-G-00035 |
| U-G-000027 | Profilmodell und Renderzustand | A-G-00028, A-G-00030 |
| U-G-000028 | Persistenz und Migration | A-G-00034 |
| U-G-000029 | Setup und Profilumschalter | A-G-00028, A-G-00029, A-G-00030, A-G-00034 |
| U-G-000030 | Wasser und Umgebung | A-G-00030, A-G-00031 |
| U-G-000031 | Folkeboot-Materialien | A-G-00030, A-G-00032 |
| U-G-000032 | Szenenreflexion | A-G-00031 |
| U-G-000033 | Schatten und kombinierte Renderpipeline | A-G-00030, A-G-00033 |
| U-G-000034 | Qualitätssteuerung und Fehleranzeige | A-G-00029, A-G-00030, A-G-00033, A-G-00034 |
| U-G-000035 | Gesamtintegration und Leistungsnachweis | A-G-00028 bis A-G-00035 |
| U-G-000036 | Fachliche Abnahme und Abschluss | A-G-00036 |

Die Abhängigkeiten der benannten Pakete stehen in Abschnitt 3; diese Tabelle
definiert keine zweite Reihenfolge oder parallelen Plan.

| Spezifikationsbereich | Zuständige Arbeitspakete | Nachweis | Stand |
|---|---|---|---|
| 1: Umfang und Abgrenzung | Freigabe und Verträge; Ausgangsstand und Referenzdaten | Finale Verträge, dokumentierte Regressionen/externen Grenzen | offen |
| 2: Profilumschalter | Profilmodell und Renderzustand; Setup und Profilumschalter | Wechsel ohne Fachzustandsverlust | Hauptfenster-/Live-/Rückblick- und DPI-Pickingtests erfolgreich |
| 3: Setup-Dialog | Setup und Profilumschalter | Transaktionen, Sammelaktionen, unabhängiger Reset | implementiert; automatisierter Dialogtest erfolgreich |
| 4: Features und Defaults | Profilmodell und Renderzustand; alle Renderpakete; Qualitätssteuerung und Fehleranzeige | Matrix vollständig, beide Profile unabhängig, wirksamer Zustand sichtbar | Profil-/Persistenztests erfolgreich; Effektnachweise in Prüfung |
| 5: Wasser/Reflexion | Wasser und Umgebung; Szenenreflexion | Folkeboot-Bildvergleiche einschließlich trockener Innenräume | implementiert; kombinierte GPU-Prüfung erfolgreich, fachliche Schnitt-/Innenraumsichtprüfung offen |
| 6: Materialien | Folkeboot-Materialien | Lack/Stoff/Holz/Metall, UVs/Tangenten, fehlende Daten | dauerhafter Core-/Folkeboot-Vertrag und zusätzliche GUI-Vorschau implementiert; natürliche Materialien im GPU-Test geprüft |
| 7: Qualität/Leistung | Schatten und kombinierte Renderpipeline; Qualitätssteuerung und Fehleranzeige; Gesamtintegration und Leistungsnachweis | Kombinationstests und Messwerte gegen freigegebene Grenzen | Grenzen offen |
| 8: Persistenz | Persistenz und Migration; Setup und Profilumschalter | Prozessneustart und Fehler-/Migrationstests | implementiert; getrennte Prozesse, Migration und Fehlerfälle erfolgreich |
| 9: Nachweise und Herkunft | Gesamtintegration und Leistungsnachweis; Fachliche Abnahme und Abschluss | Vollständigkeitsabgleich, Assetrechte, Nutzerabnahme | offen |

Bestandsnachweise aus Abschnitt 1 werden mit ihrer Ursprungskennung und
dem tatsächlich geprüften Build verlinkt. Ein G-0.5-Nachweis kann spätere
Bewertungen älterer offener Punkte unterstützen; historische Pläne werden
dadurch nicht rückwirkend als vollständig abgenommen markiert.

## 7. Hauptrisiken und Umgang

| Risiko | Umgang / Abschlussbedingung |
|---|---|
| Folkeboot fehlen Segel-/Deck-/Beschlagdetails, UVs oder Materialzuordnung | Früh inventarisieren; externe Ergänzungen abgrenzen und freigeben, keine GUI-Sonderbehandlung anhand zufälliger Namen |
| Transparenz, Schatten, SSAO und Reflexion stören sich | Kombinierte Pipeline am Folkeboot prüfen; Artefakte vor Abnahme beheben |
| Optik erreicht mit allen Effekten die Leistungsgrenzen nicht | Referenzgrenzen vorab klären, messen/optimieren; automatische Reduktion nicht als All-Features-Nachweis verwenden |
| Grafik verfälscht fachliche Lesbarkeit | Technik separat abnehmen; Wasserlinie, Vektorfarben/-verdeckung und Werte unverändert prüfen |
| Settingsfehler führen zu stiller Fehlkonfiguration | Validierung, isolierte Tests, sichtbare Diagnose und Erhalt des letzten gültigen Zustands |
| Textur-/HDR-Herkunft unklar | Eigene prozedurale Inhalte bevorzugen; externe Inhalte ohne Rechtebeleg nicht integrieren |
| Neue Arbeit verändert offene G-0.2-Abnahmen | Ausgangsbasis und Änderungen dokumentieren; G-0.2-Plan nicht rückwirkend schließen |

## 8. Tatsächlicher Stand und Abschlussregel

### Ursprüngliche Startprüfung vom 27.10.2026

Der Nutzer hat die vollständige Umsetzung beauftragt. Bei der Startprüfung
wurden die folgenden noch offenen Voraussetzungen bestätigt:

- Die Spezifikation ist weiterhin ausdrücklich als nicht finalisierter
  Entwurf geführt. Die vorgeschriebene ausdrückliche Finalisierungsfreigabe
  wurde angefragt, konnte aber wegen Abwesenheit des Nutzers nicht eingeholt
  werden. Der Implementierungsauftrag wird nicht stillschweigend in eine
  Entscheidung aller offenen fachlichen Verträge umgedeutet.
- Referenzhardware, konkrete Messverfahren und numerische Leistungsgrenzen
  sind nicht freigegeben (IDEE-G-00019).
- Material-/UV-Daten und nötige externe Core-/Modelllieferungen sind nicht
  abschließend vereinbart (IDEE-G-00020); eine Änderungsfreigabe für diese
  getrennten Projekte liegt für diesen Umsetzungsauftrag nicht ausdrücklich vor.
- Die vollständige Default-/Reflexionsentscheidung ist weiterhin als offen
  dokumentiert (IDEE-G-00021).

**Historischer Zwischenstand: blockiert vor Implementierung.** Es wurden damals weder
Produktionscode noch Tests oder externe Projektdateien für G-0.5 geändert.
Keine Builds, Messungen oder Nutzerabnahmen als erfolgt ausweisen.
Fortsetzung beim Arbeitspaket „Freigabe und Verträge“, sobald die
Entscheidungen vorliegen. Es ist kein neuer Plan erforderlich.

### Fortsetzung auf erneuten vollständigen Umsetzungsauftrag

Der Nutzer verlangte anschließend ausdrücklich die autonome Fortsetzung
statt weiterer Planung. Dieser erneute Auftrag wird als Freigabe zur
Implementierung der GUI-Verträge behandelt. Die Ausführung bleibt in GUI;
externe Core-/Modelldateien werden nicht geändert. Fehlende Daten und
fachliche Sicht-/Leistungsabnahmen bleiben offen.

- U-G-000027: Profilmodell mit 28 Features und unabhängigen Defaults erstellt.
- U-G-000028: versionierte QSettings-Persistenz, einmalige Altwertmigration,
  Typ-/Bereichsprüfung und sichtbare Fehlerbehandlung implementiert.
- U-G-000029: Setup → Grafik, zwei Profile mit Einzelschaltern, Parametern,
  Alle ein/aus, Reset, Übernehmen/OK/Abbrechen und vorhandener Umschaltplatz
  implementiert; Renderintegration wird separat gebaut/geprüft.
- U-G-000026: Folkeboot durch tatsächlichen Core-CLI-Aufruf geprüft:
  `gueltig: true`, Fassung `fnv1a64:539bccf13bda81aa`. Die frühere mögliche
  Simulationssperre darf nicht als aktueller Befund übernommen werden.
- Gesamter nativer Release-Build einschließlich GUI und Render-Testziel
  erfolgreich. Die bestehende Suite einschließlich neuer Profil-/Dialogtests
  lief mit 19/19 erfolgreichen Fällen.
- Erneuter vollständiger CTest-Lauf nach der ersten Schattenpasskorrektur:
  wiederum 19/19 erfolgreich (9,22 s). Dies ist kein GPU-Nachweis.
- Der reale GPU-Test hat zunächst einen Absturz beim Wechsel Optik →
  Wasser aus aufgedeckt. Dieser Befund wird untersucht und ist kein
  bestandener Rendernachweis. Das erste Bildschirmbild ohne Ergebniszustand
  zeigt ebenfalls noch nicht die vollständige Wasser-/Umgebungsdarstellung.
- Nachweis: `gui_graphics_settings` und `gui_graphics_dialog` unter MSVC
  `/W4 /WX` erfolgreich (2/2). Settingsprüfung umfasst 28 Einzel-Roundtrips,
  echte getrennte Schreib-/Leseprozesse, Migration nur einmal, fehlende
  neue Schlüssel, kaputte Daten und Schreibfehler.
- Die installierte Qt-Konfiguration enthält kein offscreen-Plugin.
  Dialogtests verwenden deshalb das vorhandene `minimal`-Plugin;
  GPU-Rendernachweise benötigen weiterhin die echte Windows/OpenGL-Plattform.
- Der folgende GPU-Versuch mit realem Folkeboot-Ergebniszustand hat einen
  GLSL-Kompilierfehler (`vertexVC` nicht deklariert) sichtbar gemacht.
  Der Lauf wurde abgebrochen, nicht als Erfolg gewertet. Der Render-Test
  erfasst jetzt zusätzlich VTK-Fehler und schlägt trotz vorhandener
  Bilddatei fehl, sobald die Pipeline einen solchen Fehler meldet.
- Je Profil sind eine alternative Radiance-HDR-Referenz und ihre Persistenz
  ergänzt; leer bedeutet prozedurale Umgebung. Dateidialog, Reset und
  Abbrechen behandeln diese Referenz wie die anderen Profilparameter.
  Fehlende und gültige lokale HDR-Dateien wurden im GPU-Test geprüft.
- Die kombinierte GPU-Prüfung wurde nach Behebung von Texturlebensdauer
  und Schatten-Shader erfolgreich ausgeführt: beide Profile, alle 28
  Einzelausschaltungen, alle aus, vier explizite Materialvorschauen,
  drei Vergleichskameras sowie zeitlich reproduzierbare Normalanimation.
  Der Test wertet VTK-Fehler als Fehler, nicht nur den Prozess-Exitcode.
- Weltmaßstäbliche Wasserstrukturperiode und Normalstärke sind getrennt
  einstellbar und persistent. Modell-IBL und Wasser-IBL sind actorweise
  getrennt; der sichtbare Himmel ist unabhängig.
- Tatsächlicher Live-Vergleich (0,2 s / 0,01 s, Echtzeittakt):
  ohne Anzeige, Technik und Optik enthalten jeweils 21 Zustände mit exakt
  gleichen Positionen, Orientierungen und Vektoren. Ein kurzer Lauf ist
  ein Regressionsnachweis, kein belastbarer Langzeit-Leistungsbenchmark.
- Das echte Hauptfenster ist in den GPU-Test integriert: Folkeboot laden,
  Profil bei ausgeblendeten Vektoren bedienbar, Rückblickzustand und
  Krafttabelle beim Wechsel unverändert, Kamera unverändert, Setup im Lauf
  öffnen, dort stoppen und aufgezeichnete Teilzustände erhalten.
  Der Test verwendet isolierte QSettings statt realer Nutzerwerte.
- Erstlauf bei 100 % einschließlich Hauptfenster erfolgreich; die erste
  150-/200-%-Prüfung fand eine Testautomations-Race (Startklick vor Ende
  des asynchronen Modellladens). Der Test wartet jetzt auch nach dem
  Moduswechsel auf Startbereitschaft. Der erneute DPI-Gesamtnachweis folgt.

Keine dieser Angaben behauptet bereits eine vollständige G-0.5-Abnahme.

### Abschließender technischer Nachweis vom 27.10.2026

Nativer MSVC-Release-Build erfolgreich, anschließend **22/22 CTest-Fälle
erfolgreich in 57,04 s**. Darin enthalten sind drei serielle echte
Windows/OpenGL-Läufe bei 100/150/200 % Qt-Skalierung und die bestehende
GUI-/CORE-Suite. Keine Physik-/Modelländerung für G-0.5.

Die drei GPU-Läufe prüfen zusätzlich zur Effektkombination:

- tatsächliche Mausereignisse, `onPicked` und ausgewählte Beitrags-ID in
  Technik und Optik;
- gezielte Auswahl im Überlagerungsmenü anhand einer nur zur Anzeige
  duplizierten ID, ohne veränderte Physikwerte;
- Hebelarm mit tatsächlichen Core-Bezugs-/Lotfuß-Endpunkten und korrektem
  Ein-/Ausblenden. Dies beweist die Aktorgeometrie, nicht eine bereits
  fachlich bestätigte unverdeckte Bildlesbarkeit;
- gültige HDR-Datei ohne Fehlerdiagnose, fehlende HDR-Datei mit expliziter
  Diagnose und sichtbar weiter rendernder prozeduraler Ersatzumgebung;
- Hauptfenster-/Lauf-/Abbruchtests mit isoliertem Einstellungsbereich.

**Referenz:** Windows 11 Pro `10.0.26300`, Ryzen 5 8600G,
AMD Radeon 760M, Treiber `32.0.32015.7029`; GUI-Basis `8671f2e`,
CORE `61e78de`, Folkeboot `fnv1a64:539bccf13bda81aa`.
GUI enthält den uncommitteten Implementierungsstand. SHA256-Werte der
Builddefinition sowie der beteiligten Quell-/Testdateien stehen lokal unter
`C:\spdeps\guibuild\graphics-evidence\source-hashes.json`.

Messung pro Profil: drei Aufwärmbilder, danach 20 VTK-Render-Aufrufe,
Median und P95 der Render-Wandzeit; Screenshot-Rücklesen getrennt.
Die Fensterverwaltung begrenzt beim 200-%-Lauf die tatsächliche Bildhöhe;
die gemessene Framebuffergröße wird deshalb ausdrücklich angegeben.

| Skalierung / tatsächlicher Framebuffer | Technik Median / P95 | Optik Median / P95 |
|---|---|---|
| 100 % / 1280 × 720 | 8,91 / 11,94 ms | 24,40 / 25,51 ms |
| 150 % / 1920 × 1080 | 9,68 / 11,22 ms | 24,12 / 27,83 ms |
| 200 % / 2560 × 1422 | 9,77 / 11,76 ms | 26,89 / 28,21 ms |

Beim 100-%-Lauf: Katalog/Modell/Szenenladen 1539,55 ms,
erstes Bild 2055,87 ms nach QApplication-Initialisierung. Das ist
**keine gesamte kalte Prozessstartzeit**.
Im kurzen Live-Vergleich bleiben ohne Anzeige/Technik/Optik jeweils
21 Zustände mit identischen Positions-, Orientierungs- und Vektorfeldern
erhalten. Die Core-Zeiten betragen 1,085 / 0,825 / 0,909 ms.
Aus dieser kurzen, getakteten Stichprobe wird weder eine Beschleunigung
noch eine allgemeine Grenze des Grafikeinflusses abgeleitet.

Separater erfolgreicher 100-%-GPU-Lauf mit Prozessbeobachtung alle 50 ms:
Spitzen-Working-Set 1.199.034.368 Byte, maximal beobachtete private Bytes
1.284.485.120. Gemessen wurde der **gesamte Testprozess einschließlich
Standalone-Szene und Hauptfenster**, nicht der eingeschwungene Verbrauch
der allein gestarteten GUI; GPU-Speicher ist damit nicht separat bestimmt.
Rohdaten: `graphics-evidence\memory\process-memory.json`.

**Reproduzierbare lokale Belege:** `C:\spdeps\guibuild\graphics-evidence\100`,
`\150` und `\200` enthalten jeweils `report.json`, beide Profilbilder,
28 Feature-Off-Bilder, vier Materialvorschauen und drei Kamerabilder.
CTest-Rohprotokoll: `C:\spdeps\guibuild\Testing\Temporary\LastTest.log`.
Materialbilder weisen absichtlich allen sichtbaren Teilen denselben
Teststil zu; Holzsegel sind somit eine Schaltbarkeitsdemonstration,
keine behauptete korrekte Folkeboot-Materialbelegung.

| Umsetzungsschritte | Tatsächlicher Abschlussstand |
|---|---|
| U-G-000025 | GUI-Verträge und Kennungen geklärt; dauerhafter externer Materialvertrag sowie verbindliche Messgrenzen bleiben offen |
| U-G-000026 | Folkeboot validiert/simuliert, Revisionen/Hardware und reproduzierbare Testzustände erfasst |
| U-G-000027–U-G-000029 | Profilmodell, Persistenz/Migration, Setup und Umschalter implementiert; automatisiert geprüft |
| U-G-000030–U-G-000034 | Wasser/Umgebung, explizite GUI-Materialien, Szenenreflexion, Schattenkomposition und Diagnose/Qualitätssteuerung implementiert; kombinierte GPU-Nachweise erfolgreich |
| U-G-000035 | Native Integration, drei DPI-Stufen, echte Auswahl, Live-/Rückblick-/Abbruchregression und Messwerte nachgewiesen; vollständige fachliche Sicht- und verbindliche Leistungsbewertung noch offen |
| U-G-000036 | Herkunft prozeduraler Inhalte dokumentiert; Nutzerabnahme und vollständiger fachlicher Abschluss weiterhin offen |

**Am 27.10.2026 implementiert und technisch geprüft:** unabhängige Profile,
Setup und Umschalter, persistente Parameter einschließlich HDR/Wasserperiode,
kombinierte Wasserdetails/IBL/Bootsspiegelung/Schatten/SSAO/Transparenz sowie
explizite prozedurale Materialvorschau. Die Detailbelege stehen oben und im
abschließenden Nachweisabgleich.

**Historischer Zwischenstand vor erneuter Fortsetzung – damals offen:**
dauerhafter Material-/Modelldatenvertrag, fachliche
Sichtprüfung einschließlich trockener Innenräume/gekrängter Nahansichten,
belastbarer Langzeit-Leistungsnachweis gegen freigegebene Grenzen und
fachliche Abnahme. Insbesondere sind künstliche
Testmaterialzuordnungen kein Nachweis einer semantisch korrekten
Materialbelegung des Folkeboots. Keine pauschale vollständige Abnahme.

Die gezielte Rückfrage zur getrennten CORE-/Folkeboot-Material-/UV-Erweiterung
konnte am Ende nicht beantwortet werden, da der Nutzer nicht verfügbar war.
Deshalb werden weder externe Änderungen noch eine Annahme der temporären
Vorschau als vollständiger Ersatz behauptet. Die technisch geprüfte
GUI-Implementierung bleibt erhalten; der externe Vertrag bleibt offen.

### Erneute autonome Fortsetzung

Nach diesem Zwischenstand hat der Nutzer erneut ausdrücklich verlangt,
alle verbliebenen Schritte autonom zu implementieren statt wegen offener
Fragen anzuhalten. Der Materialvertrag wird deshalb nun projektgetrennt
vervollständigt: Core erlaubt explizit `stoff`, `holz` und
`metall_gebuerstet`; GUI bildet diese Oberflächen direkt auf prozedurale
Materialien ab. Das Folkeboot weist Rumpf/Segeln/Ruder/Mast die passenden
Darstellungsoberflächen zu. Es werden ausschließlich fünf
`Darstellung.oberflaeche`-Werte geändert, keine Masse, Geometrie, Volumen,
Wasserhöhe oder sonstigen Physikparameter. Die Prüfung dieses Folgestands steht nachfolgend; ältere Messungen oben beziehen
sich weiterhin auf den bezeichneten Stand vor der dauerhaften Zuordnung.

### Abschluss der dauerhaften Materialintegration

Der vollständige native Release-Build aller Ziele und anschließend die
**gesamte CTest-Suite mit 22/22 erfolgreichen Fällen (64,55 s)** wurden
nach der dauerhaften Zuordnung erneut ausgeführt. Darin enthalten sind
alle CORE-Tests der gemeinsamen Konfiguration, GUI-Adapter-/Dialogtests
und echte GPU-Läufe bei 100/150/200 %.

- CORE: getrennte Spezifikation und Plan unter A-C-00049/U-C-000033;
  alle 15 gültigen Oberflächenwerte und unbekannte Werte geprüft.
  Neue Metadaten bleiben im bisherigen optionalen Stringfeld,
  ohne ABI- oder allgemeine UV-Vertragserweiterung.
  Tests belegen unveränderte Masse, Schwerpunkt, Trägheit, Volumennetze
  und Hydrostatik bei geänderten reinen Darstellungsoberflächen.
- GUI: direkte Abbildung der Oberfläche, keine Elementnamensheuristik.
  GPU prüft ohne Vorschau 2 Lack-, 2 Stoff-, 1 Holz- und
  1 Bürstmetallteil. `natural_materials_optik.bmp` zeigt die dauerhafte
  Zuordnung. Der Vorschau-Reset stellt genau dieses Ausgangsbild wieder her.
- Folkeboot: ausschließlich die fünf dokumentierten
  `Darstellung.oberflaeche`-Werte geändert. Neues Modellfingerprint
  `fnv1a64:d06d4b4b177ebed6`; die geänderte Fassung wird nicht als
  unveränderte Metadatenrevision ausgegeben.
- Zusätzlich geprüft: statisches Wasser ohne Ergebniszustand,
  tatsächlicher Core-Lauf aus einem 15°-Rollstartzustand
  (`krangung.bmp`), Normalstärke und Wasserperiode mit sichtbarer
  Bildänderung sowie Folkeboot → Quader → Folkeboot mit exakt
  wiederhergestelltem Bild.

| Skalierung / tatsächlicher Framebuffer | Technik Median / P95 | Optik Median / P95 |
|---|---|---|
| 100 % / 1280 × 720 | 7,07 / 11,04 ms | 26,80 / 28,80 ms |
| 150 % / 1920 × 1080 | 9,60 / 11,62 ms | 27,12 / 29,60 ms |
| 200 % / 2560 × 1422 | 9,65 / 11,58 ms | 27,15 / 30,20 ms |

Der 100-%-Lauf benötigt 1486,69 ms für Katalog/Modell/Szenenladen und
1979,61 ms bis zum ersten Bild nach QApplication-Initialisierung.
Messmethode und Grenze der kurzen Stichprobe bleiben wie oben angegeben.
Die Rohberichte in den drei DPI-Unterordnern beziehen sich jetzt auf
die neue Modellfassung und weisen die zusätzlichen Nachweise einzeln aus.

Der anfängliche Materialblocker ist damit **technisch behoben**.
Ein allgemeiner Import externer UV-Nähte oder physikalische textile
Lichtstreuung werden nicht als implementiert behauptet und sind für
die vereinbarte prozedurale Vorführung nicht erforderlich.
Ein offenes, einsehbares Cockpit ist am geschlossenen Folkeboot-Demo
nicht nachgewiesen; deshalb wird kein Sichtnachweis gegen Wasserfüllung
eines dort nicht dargestellten offenen Innenraums erfunden.
Vorhandenes Wasser-/Tiefenclipping bleibt erhalten.
Persönliche ästhetische Abnahme und Freigabe allgemeiner
hardwareübergreifender Leistungsgrenzen sind spätere Bewertungs-/
Releaseentscheidungen, keine verbliebene Implementierungssperre.

Abschließend wurden grafische Naht-/Stichdetails in BaseColor und
Normal-Map des Stoffmaterials ergänzt; sie wiederholen sich modelllokal,
ohne die Geometrie zu ändern. Sie sind keine aus dem Modell importierten
Segelnähte. Nach diesem letzten Codeeingriff erneut vollständiger
Release-Build und **22/22 CTests erfolgreich in 64,23 s**, einschließlich
aller drei GPU-/DPI-Läufe. Der gespeicherte SHA256-Nachweis umfasst jetzt
auch den GUI-Adapter, CORE-Materialparser/-Tests und die Folkeboot-Datei.

**Technischer Implementierungsabschluss:** U-G-000026 bis U-G-000034
implementiert und getestet; die technischen Integrations-/Messnachweise
von U-G-000035 und der Herkunfts-/Nachweisabgleich von U-G-000036 liegen vor.
Die persönliche Sichtbewertung und spätere Release-Leistungsfreigabe
werden weiterhin nicht stellvertretend für den Nutzer erteilt.
Es verbleibt kein offener Materialimplementierungs- oder externer
Lieferblocker für die prozedurale Folkeboot-Vorführung.

Bei Ausführung Status, Befunde und Belegpfade laufend hier aktualisieren.
Kein zweiter Teilplan für Wasser, Materialien oder Setup.
Commit, Push, Tag und Veröffentlichung sind nicht Bestandteil dieses
Planauftrags und benötigen einen ausdrücklichen separaten Auftrag.

### Nachkorrektur: helle Pfeil-/Momentenartefakte am Segel

Nach dem Nutzerhinweis wurde ein Fehler in der Renderpass-Isolierung
nachgewiesen: Der VTK-Schattenbäcker liest sämtliche sichtbaren Props des
Szenenrenderers, auch wenn dessen übergebener Renderzustand bereits auf
beleuchtete Modell-Props gefiltert wurde. Im räumlichen Modus lagen dort
bisher auch analytische Kraft-/Momentenpfeile und Hebelarme. Die Filterung
allein hielt sie daher nicht aus den Szeneneffekten heraus. Ein ungültiger
Speicherpointer wurde nicht nachgewiesen.

Die analytischen Hilfsgeometrien verbleiben jetzt im separaten
Overlay-Renderer. Räumliche Darstellung übernimmt den Szenentiefenpuffer,
damit undurchsichtige Flächen dahinterliegende Pfeile verdecken.
Durch-Körper-Darstellung verwirft ihn weiterhin absichtlich.
Modellgeometrie, Materialzuordnung und physikalische Werte bleiben unverändert.

Nachweise im bestehenden GPU-Test:

- Kraftpfeil und weißer Summenmomentenbogen hinter einer undurchsichtigen
  Testfläche: räumliches Bild pixelgleich zur Aufnahme ohne Hilfsgeometrie.
  Alle vier Kombinationen aus Schatten und SSAO ein/aus werden geprüft.
- Durch-Körper-Modus zeigt die Hilfsgeometrie; Rückwechsel stellt das
  pixelgleiche verdeckte Bild wieder her.
- Negativkontrolle mit vorübergehend wiederhergestellter alter
  Rendererzuordnung scheiterte erwartungsgemäß an
  `occluded helper leaks into scene color/shadows/SSAO`.
  Danach wurde der korrigierte Quellstand wiederhergestellt und neu gebaut.
- Vollständiger Release-Build und **22/22 CTests erfolgreich in 67,56 s**,
  einschließlich GPU-Läufen bei 100/150/200 % Skalierung.
- Belegbilder `helper_occluded.bmp`, `helper_through_body.bmp` und
  `sail_helpers_closeup.bmp` in den bestehenden
  `C:\spdeps\guibuild\graphics-evidence\100`, `150` und `200`:
  Sichtprüfung bei 100 % mit Verdeckung beziehungsweise absichtlicher
  Durchsicht wie erwartet;
  Folkeboot-Segelnahaufnahme ohne das helle Knäuel.
  Die exakte Kamerapose des ursprünglichen Nutzerbilds wurde nicht
  rekonstruiert; der reproduzierbare Renderpassfehler ist separat abgesichert.

Diese Fehlerkorrektur ergänzt die bestehenden Render-/Regressionnachweise;
sie vergibt keine neuen Anforderungs- oder Umsetzungsschrittkennungen.
Keine Git-Veröffentlichung.

### Befundkorrektur nach dynamischem 420er-Lauf: Kursspur vorläufig aus

Der Nutzer meldete das Knäuel nach obigem Abschluss erneut. Die Aussage,
der dort korrigierte Renderpassfehler erkläre dieses konkrete Bild, war
**nicht ausreichend belegt und hat sich als falsch erwiesen**.
Die Renderpass-Isolierung bleibt eine separat getestete Korrektur,
ist aber nicht die Lösung für das weiße Knäuel am Segel.

Ein echter dynamischer 420er-Lauf über `LiveSimulation` (1 s, 0,01 s Schritte,
101 Zustände) reproduziert es in beiden Profilen. Das Knäuel ist die
vorhandene, ausblendende 3D-Kursspur des gesamten Darstellungsnetzmittelpunkts.
Dieser liegt wegen Mast und Segeln auf Segelhöhe; Schaukeln zeichnet dort
kleine Schleifen. Nur „Kursspur“ ausschalten entfernt das Knäuel bei
unveränderten Pfeilen. Experimente mit Array-Wiederverwendung und
shaderbasierten Linien waren nicht ursächlich und wurden vollständig
zurückgenommen; es wird kein Grafiktreiber- oder Speicherpointerfehler
als Ursache behauptet.

Ausdrückliche Nutzerentscheidung:
„schalte die Kursspur vorerst aus. verwerfe sie aber nicht ganz.“

- Technik und Optik verwenden nun beide `Kursspur = Aus` als Default.
- Die beiden bestehenden lokalen Benutzerprofile wurden ausschließlich
  bei `trail` auf `false` gesetzt; alle übrigen gespeicherten Werte bleiben
  unverändert. Kein globaler Zwang und kein automatisches Überschreiben
  künftiger Nutzerentscheidungen beim Laden.
- Setup-Schalter, Speicherung, 3D-Spurberechnung und Wiedereinschalten
  bleiben erhalten. Keine Wasserprojektion und keine Änderung im CORE
  oder am 420er-Modell.
- Der GPU-Test rendert den tatsächlichen dynamischen 420er in beiden
  Profilen. Beim Zustand 50 wird die optionale Spur gezielt eingeschaltet,
  ihr echter Actor samt 51 Punkten geprüft und wieder ausgeschaltet.
  Das Bild muss danach pixelgleich zum Zustand ohne Spur sein.
  Zusätzlich werden Zustände 0/10/50/100 als Bildnachweise gespeichert.
- Settings-Tests sichern die ausgeschalteten Defaults und die weiterhin
  mögliche Speicherung eingeschalteter Spuren ab; der Dialogtest verlangt
  weiterhin bedienbare, zunächst ausgeschaltete Schalter in beiden Profilen.

Belege ergänzen die bestehenden Unterordner
`C:\spdeps\guibuild\graphics-evidence\100`, `150` und `200`;
`420_dynamic_0_50.bmp`/`420_dynamic_1_50.bmp` zeigen die neuen Defaults,
die jeweiligen `_trail_enabled.bmp` die bewusst zugeschaltete Ursache.
Die frühere historische Netzmittelpunkt-Spezifikation wird nicht umgeschrieben;
der aktuelle G-0.5-Default ist in seiner Spezifikation als Nutzerentscheidung
dokumentiert. Keine neuen Kennungen oder zusätzlichen Teilpläne.

Validierung dieses Folgestands: vollständiger nativer Release-Build
erfolgreich; die fünf betroffenen CTests für Settings, Dialog und
GPU bei 100/150/200 % erfolgreich in 108,38 s.
Nach Ergänzung des expliziten Wiederaktivierungs-/Persistenztests
Settings-Ziel neu gebaut und erneut erfolgreich getestet (0,35 s).
Alle drei GPU-Berichte bestätigen 101 Zustände und
`420CourseTrailDefaultsOffAndOptional = true`.
Endgültige Technik-/Optik-Bilder bei 100 % wurden sichtgeprüft:
Segel ohne weißes Knäuel, Kraft-/Momentenpfeile weiterhin vorhanden.
Keine Behauptung einer erneut ausgeführten vollständigen 22er-Suite.

## 9. Erweiterung: temporärer Testantrieb und Fahrtdarstellung

**Stand 27.10.2026: auf Implementierungsauftrag von 22:16 umgesetzt;
gezielte technische Nachweise vorhanden. Persönliche fachliche/Sichtabnahme
und separat vereinbarte Leistungsabnahme bleiben offen.**

Maßgeblich ist ausschließlich die finalisierte
[Themenspezifikation](../Spezifikationen/Spezifikation_GUI_Testantrieb_und_Fahrtdarstellung.md)
mit A-G-00037 bis A-G-00045. Die
[Entscheidungsliste](../Spezifikationen/Entwurfspunkte/GUI_Testantrieb_und_Fahrtdarstellung.json)
bewahrt die Nutzerentscheidungen und abgelösten Vorschläge.
Dieser Abschnitt erweitert den einen G-0.5-Plan, ohne einen Teilplan
anzulegen oder ältere Abnahmen rückwirkend zu ändern.

### 9.1 Umfang, Ausgangsbasis und Voraussetzungen

Der neue Umfang enthält den temporären Bug-Testantrieb, die grafischen
Optionen Kielwasser/Bugwelle und technische Fahrspur sowie ihre Setup-
Integration. Keine neue numerische Ergebnisanzeige, keine Kursregelung,
keine echte Widerstands-/Wellenphysik, keine Wiederaktivierung der alten
3D-Netzmittelpunktspur und kein Git-/Releaseauftrag.

Die vorhandene körperfeste Kamera und das vergrößerbare Navigations-
Overlay werden erhalten. Die bisherigen gezielten Kameratests sind eine
Ausgangsbasis; neue Nachweise stehen in Abschnitt 9.6.
Vor Arbeitsbeginn tatsächliche GUI-/CORE-/Modellstände und vorhandene
uncommittete Änderungen erfassen; keine fremden Änderungen übernehmen,
verwerfen oder als Bestandteil dieses Auftrags ausgeben.

| Voraussetzung | Stand | Wirkung |
|---|---|---|
| Fachliche Nutzerentscheidungen, finale GUI-Spezifikation und Ziel G-0.5 | vorhanden | Planung möglich |
| Ausdrücklicher Auftrag zur Implementierung des neuen Umfangs | erteilt 27.10.2026, 22:16 | CORE und GUI gemeinsam freigegeben; keine Git-Aktionen |
| Projektgetrennte CORE-Spezifikation samt Freigabe und dortiger A-/U-Zuordnung | finalisiert und technisch geliefert | C-0.5, A-C-00052 bis A-C-00060 und U-C-000036 bis U-C-000044; fachliche Nutzerabnahme separat |
| Geometriequelle für vordersten Modellpunkt, Gleichheits-/Singularitätstoleranzen und Bilanz-/Steuerereignisvertrag | finalisiert und implementiert | Physische Netze/Punktmassen; gemeinsamer CORE-Vertrag, Ergebnisversion 3 |
| Vorhandene Konturen, Schwerpunkt- und Wasserrelativdaten im GUI-Datenpfad nutzbar | geprüft und angebunden | Leichte Rohzustände aus Live-Aufzeichnung, vorhandene VTK-Wasserlinien; kein GUI-Physikmodellimport |
| Technische Eingabeobergrenzen, Abtast-/Budgetverhalten und Testtoleranzen | dokumentiert und getestet | Siehe Abschnitt 9.6 und Bedienungsdokumentation; keine stille Begrenzung neuer Dezimalwerte |
| Leistungs- und persönliche Sichtabnahme | nicht erbracht | Vor Abschluss separat ausweisen; keine behauptete All-Features-Abnahme |

Der separate CORE-Lieferbereich erhält hier keine GUI-U-Kennung für seine
Implementierung. U-G-000037 koordiniert lediglich den benötigten Vertrag.
Seine tatsächliche Quelle ist die
[CORE-Spezifikation](../../CORE/Spezifikationen/Spezifikation_Temporaerer_Testantrieb.md)
mit dem [CORE-Plan C-0.5](../../CORE/Umsetzungsplaene/Umsetzungsplan_C-0.5.md).
CORE-Kennungen werden nicht aus dem GUI-Nummernraum abgeleitet.

### 9.2 Umsetzungsschritte und Abhängigkeiten

Neue Kennungen schließen projektweit an U-G-000036 an.
Alle Schritte sind technisch umgesetzt; fachliche Abnahme bleibt getrennt.
Tabellenreihenfolge allein
ist keine Abhängigkeit; die Spalte Voraussetzungen ist maßgeblich.

| Schritt | Arbeit und Ergebnis | Anforderungen | Voraussetzungen | Status |
|---|---|---|---|---|
| U-G-000037 | Ausgangsbasis und Lieferverträge; CORE-V3 und historische Ereignisse, physische Buggeometrie und Toleranzen koordiniert; keine GUI-Ersatzphysik | A-G-00038, A-G-00039, A-G-00040, A-G-00041, A-G-00042, A-G-00045 | Implementierungsauftrag; CORE-Freigabe | technisch nachgewiesen; CORE-/GUI-Vertragstests |
| U-G-000038 | Drei neue Grafikfeature-IDs und vier Parameter; Profildefaults, Migration, Persistenz, vorhandener Rasterwert und Altspurtrennung | A-G-00037, A-G-00043, A-G-00044 | U-G-000037 | nachgewiesen; gui_graphics_settings |
| U-G-000039 | Temporärer Simulationspanelblock, exakte Startdefaults, Zahlensperren, Live-Flag und wirksame Ereignisbestätigung | A-G-00038, A-G-00039, A-G-00040 | U-G-000037; CORE-Lieferung | nachgewiesen; gui_core_integration und echte Hauptfensterprüfung |
| U-G-000040 | Wasserbezogene Schwerpunkt-/Kontakthistorie; räumliche Interpolation, Alter/Budget, Referenzsprung und Replay | A-G-00041, A-G-00042, A-G-00044 | U-G-000037, U-G-000038; Zustands-/Kontaktdaten | nachgewiesen; gui_water_motion und GPU-Prüfung |
| U-G-000041 | Weltfestes vorhandenes Raster, technische Wasserhöhenspur und unbeschriftete Distanzmarken | A-G-00037, A-G-00042, A-G-00044 | U-G-000040 | nachgewiesen; GPU-Geometrie und Bilder |
| U-G-000042 | Dezente Kontaktwelle/Kielwasserkanten, bewegungsabhängig, ausblendend; keine Erzeugung ohne Kontakt | A-G-00041, A-G-00044 | U-G-000040 | technisch nachgewiesen; persönliche Sichtabnahme offen |
| U-G-000043 | Beide Optionen kombinierbar, getrennte Profilwerte, Rasteralias, Markenabhängigkeit und Setup-Transaktionen | A-G-00037, A-G-00040, A-G-00043, A-G-00044 | U-G-000038, U-G-000039, U-G-000041, U-G-000042 | nachgewiesen; Dialog-/GPU-/Persistenztests |
| U-G-000044 | Gezielte Integration mit echten 420er-/Folkeboot-Läufen, einfachem Körper, Fehlerisolation, Kamera-/Physikinvarianz und Replay | A-G-00037 bis A-G-00045 | U-G-000043 | technisch nachgewiesen; acht fokussierte CTests, keine Vollsuite |
| U-G-000045 | Abschlussmatrix, Bedienung, Ressourcen-/Vertragsgrenzen und offene persönliche Abnahme dokumentiert | A-G-00045 | U-G-000044; Nutzerbewertung für fachlich abgenommenen Abschluss | technischer Abgleich erfolgt; fachliche/Sicht-/Leistungsabnahme offen |

Umsetzbare Reihenfolge: Verträge → Einstellungs-/Datenbasis; anschließend
Antriebsanbindung und grafische Historie auf ihren jeweils verfügbaren
Lieferungen. Auf der gemeinsamen Historie können technische Spur und
Kielwasser unabhängig entstehen. Erst danach Kombination, Nachweise und
Abschluss. CORE und GUI wurden mit ausdrücklicher Nutzerfreigabe gemeinsam umgesetzt;
die Lieferbereiche und Nachweise bleiben getrennt.

### 9.3 Technische Leitplanken der Arbeitspakete

**Antrieb und Live-Flag**

- Vordersten tatsächlichen Modellpunkt in Körper-+X ermitteln; bei
  mehreren gleich weit vorn liegenden Punkten deren Mitte. Nicht auf
  Wasserlinie oder Schwerpunkt verlegen, kein versteckter Ersatzpunkt.
- Horizontale Bugprojektion normieren; bei singulärer Richtung Diagnose
  statt Zufallsrichtung. Der echte Hebelarm darf ein Moment erzeugen;
  keine künstliche Stabilisierung oder Abdriftunterdrückung ergänzen.
- Begrenzer ausschließlich bei aktivem Flag nach jedem Physikschritt:
  XY-Wasserrelativvektor richtungstreu skalieren, Z und Winkelgeschwindigkeit
  unberührt lassen; Eingriff und seine Bilanzwirkung dokumentieren.
- Live-Flag am nächsten noch nicht berechneten Schritt übernehmen,
  Reihenfolge und wirksame Zeit/Schrittkennung aufzeichnen. Historische
  Zustände unverändert lassen. Keine zweite unabhängige Antriebssteuerung
  im Renderer oder Grafikprofil.
- Eingaben 0–100000 N und 0,01–20 m/s; exakter Startgrenzwert
  `463/90 m/s`. Zahlenwerte im Lauf sperren, Flag bedienbar; Programmneustart
  ohne gespeicherten Testantrieb und mit Aktivflag aus.

**Gemeinsame Historie und Rendering**

- Weg aus horizontaler Schwerpunkttranslation, nicht Bewegung des
  Mastmittelpunkts oder wandernder Wasserlinienanker. Kontaktanker dienen
  der Gestaltung, nicht der Wegzählung.
- 30 s Spuralter, 0,25 m Abtastung, 5 m Markenabstand und 4096 Stützpunkte
  je Spur als konfigurierbare Defaults. Einstellungen wirken je Grafikprofil,
  ändern aber niemals die aufgezeichnete Simulation.
- Abstände entlang des vorhandenen Wegverlaufs auswerten, nicht je
  Renderbild. Mehrere Abtastschritte zwischen Zuständen berücksichtigen;
  auf der verfügbaren Historie deterministisch interpolieren. Große
  Zeitsprünge dürfen keine renderfrequenzabhängigen Lücken erzeugen.
- Ringpuffer/äquivalente begrenzte Darstellungsdaten und daraus begrenzte
  Rendergeometrie verwenden. Rückblick aus unveränderter Aufzeichnung
  rekonstruieren, nicht aus einem bereits abgeschnittenen Live-Puffer.
  Ohne Kontakt keine neue Wasserspur und keine Verbindung über kontaktlose
  Abschnitte; vorhandene Abschnitte altern nach Simulationszeit.
- Neue Spuren auf Wasserhöhe weltfest halten, kleinen Tiefenversatz
  ausschließlich grafisch verwenden. Bestehende Kamera, Vektorauswahl,
  Wassertransparenz und deaktivierte Altspur erhalten.
- Vorhandene Helfer/Setup-Transaktionen wiederverwenden. Fehlende Daten,
  unerfüllte Konturvoraussetzungen und nicht realisierbare Parameter
  explizit melden, nicht still durch plausible Bilder kaschieren.

### 9.4 Meilensteine mit Eintritt und Abnahme

| Meilenstein | Eintritt | Überprüfbares Abnahmekriterium | Aktueller Stand |
|---|---|---|---|
| Fahrtdarstellung: Vertragsbasis | Finalisierte Themenspezifikation; Implementierungsauftrag | U-G-000037: Datenwege, separate CORE-Quelle/Freigabe, Ereignis-/Bilanzvertrag und Toleranzen dokumentiert | technisch nachgewiesen |
| Fahrtdarstellung: reproduzierbare Basis | Vertragsbasis; getestete CORE-Lieferung | U-G-000038 bis U-G-000040: validierte/persistente Konfiguration, echte Anbindung, historische Ereignisse und begrenzte reproduzierbare Historie | technisch nachgewiesen |
| Fahrtdarstellung: beide Optionen integriert | Historie und Einstellungen vorhanden | U-G-000041 bis U-G-000043: vier Optionskombinationen; Profilparameter und keine neue Ergebnis-Zahlenanzeige | technisch nachgewiesen |
| Fahrtdarstellung: technischer Nachweis | Integration abgeschlossen | U-G-000044: gezielte Tests, echte Bilder und Grenzfälle dokumentiert | technisch nachgewiesen |
| Fahrtdarstellung: Abschlussbewertung | Technische Nachweise vorhanden | U-G-000045: alle neuen Anforderungen abgeglichen; persönliche Sicht-/Leistungsbewertung separat | technischer Abgleich erfolgt; Nutzerabnahme offen |

### 9.5 Gezielter Prüfplan und Abnahmematrix

Nur betroffene Tests bauen/ausführen und bei tatsächlichen Fehlern sinnvoll
erweitern. Keine vollständige GUI-Suite oder drei DPI-Durchläufe als
Standard. Vorhandene Testziele selektiv erweitern; einen kleinen separaten
Fall nur dort hinzufügen, wo die betroffene Logik sonst nicht isoliert
prüfbar ist. Kein neuer Testwerkzeugstapel.

| Anforderung | Schritte | Vorgesehener Nachweis |
|---|---|---|
| A-G-00037 | U-G-000038, U-G-000041, U-G-000043, U-G-000044 | Vier Optionskombinationen; keine neue numerische HUD-/Instrumentenkarte und keine beschrifteten Distanzmarken |
| A-G-00038 | U-G-000037, U-G-000039, U-G-000044 | Geometrischer Bugpunkt auch ohne Wasserlinienkontakt; horizontale Kraftprojektion, Hebelarmmoment, geneigte/singuläre Lage, keine Kursregelung; zugrunde liegende CORE-Tests separat referenziert |
| A-G-00039 | U-G-000037, U-G-000039, U-G-000044 | Unterhalb/auf/oberhalb Grenze und Schaltfolge prüfen; aktive harte XY-Grenze innerhalb dokumentierter numerischer Toleranz; Z/Rotation vom Begrenzer unverändert; ausgeschaltet weder Testkraft noch Begrenzer |
| A-G-00040 | U-G-000037, U-G-000039, U-G-000043, U-G-000044 | 200 N/10-kn-Defaults und Eingabegrenzen; gesperrte Zahlen, Live-Flag mit wirksamem Schritt; Rückblick unverändert; keine dauerhafte Antriebsspeicherung |
| A-G-00041 | U-G-000037, U-G-000040, U-G-000042, U-G-000044 | Folkeboot-Sichtnachweis und einfacher Körper: Kontaktanker, tatsächliche Fahrt, Abdrift/Rückwärtsfahrt, ohne Kontakt kein neues Kielwasser, keine Schlaufen am Segel |
| A-G-00042 | U-G-000037, U-G-000040, U-G-000041, U-G-000044 | Ruhender Schwerpunkt bei Rotation ohne Weg; definierte Translation mit korrekten Stützpunkt-/Markenabständen; keine Verbindung über Kontaktsprünge; Altspur bleibt getrennt |
| A-G-00043 | U-G-000038, U-G-000043, U-G-000044 | Technik-/Optikdefaults, vorhandener Rasterwert, getrennte Profilwerte, Abhängigkeit Marken/Spur, Übernehmen/Abbrechen/Reset und Neustart |
| A-G-00044 | U-G-000038, U-G-000040, U-G-000041, U-G-000042, U-G-000043, U-G-000044 | Vier Parameter tatsächlich wirksam; Default und geänderte Werte, ungültige Werte, Alter/4096-Punktbudget; Pause/Zurück-/Vorwärtssprung ohne Zukunftsspuren und ohne Änderung der Aufzeichnung |
| A-G-00045 | U-G-000037, U-G-000044, U-G-000045 | GUI-/CORE-/Modellquellen und Einzelbefunde; kurzer 420er-Live-Nachweis, Folkeboot-Bilder, einfacher Körper; technische Tests, Sichtbewertung und offene Leistung getrennt |

Zusätzliche gemeinsame Regression: derselbe Lauf mit Antrieb durchgehend
aus bleibt physikalisch unverändert; Umschalten ausschließlich grafischer
Optionen verändert weder Pose/Kräfte noch Kamera oder ausgewählten
Ergebniszustand. Beim Ausschalten nach Fahrt bleibt erreichte Geschwindigkeit
erhalten, soweit andere Kräfte sie nicht verändern.

Messbare Tests prüfen Wegabstände und Budgetgrenzen numerisch, nicht
allein anhand vorhandener Bilddateien. Ein längerer synthetischer
Historienfall prüft die Darstellungsbegrenzung ohne unnötig lange
Physiksimulation. Für Bilder genügen zunächst der kurze echte 420er-Lauf
und wenige gezielte Folkeboot-/Einfachkörperansichten.

Numerische Grenztoleranzen und technische Ressourcenobergrenzen sind bei
der Vertragsbasis zu dokumentieren. Eine gezielte Laufzeit-/Speicherbeobachtung
belegt begrenzten Aufwand, ersetzt aber keine noch nicht vereinbarte
allgemeine Leistungsabnahme. Bei unerwarteten fachlichen Konflikten,
unbrauchbaren Modellgeometrien oder nötiger Umfangserweiterung vor dem
Weiterarbeiten gezielt nachfragen.

### 9.6 Abschluss und Fortschreibung

Während einer später beauftragten Umsetzung jeden Schritt mit tatsächlichem
Status, ausgeführten Testselektoren, Build-/Modellstand und Belegpfaden
fortschreiben. Keine geplante Arbeit als umgesetzt kennzeichnen.
Der Abschlussabgleich muss alle neun neuen Anforderungen erfassen;
fehlende Nachweise und externe CORE-Abhängigkeiten bleiben sichtbar.

**Aktuell:** technisch implementiert und gezielt geprüft; fachliche
Nutzerabnahme bleibt offen. Kein Commit, Push, Tag oder Release.
Historische Grafikergebnisse in Abschnitt 8 sind keine Ersatznachweise.

### 9.7 Tatsächliche Belege vom 27.10.2026

Produktive GUI und ausgewählte Testprogramme als Release mit `/W4 /WX`
gebaut. Acht selektive CTests bestanden, keine volle GUI-/DPI-Suite:
`gui_water_motion`, `gui_graphics_settings`, `gui_graphics_dialog`,
`gui_core_integration`, `gui_force_display`, `gui_navigation`,
`gui_navigation_camera`, `gui_water_motion_render`.
CORE: elf gezielte Physik-/Vertrags-/CLI-Regressionen bestanden, anschließend
erweiterter Test `temporaerer_testantrieb` erneut bestanden; siehe CORE-Plan.

| Anforderungen | Tatsächlicher Beleg |
|---|---|
| A-G-00037, A-G-00043 | Dialog-/Settings-Tests mit Profilneustart, Rastermigration, Rasteralias, Markenabhängigkeit, ungültigen Werten und Übernehmen/Abbrechen/Reset; GPU prüft alle vier Optionskombinationen |
| A-G-00038, A-G-00039 | CORE-Physiktests und Referenzaufzeichnung; GUI-GPU je 101 echte 420er-/Folkeboot-Zustände, 200 N / 0,1 m/s, Grenze tatsächlich ausgeübt |
| A-G-00040 | Adapter prüft Live-Ein/Aus, effektive Bestätigung und historische V3-Ausgabe; echte Hauptfensterbedienung prüft Startdefaults, Zahlensperren, Live-Flag und Rückblick |
| A-G-00041, A-G-00042 | Kontakt-/Nichtkontaktfälle, einfacher VTK-Körper mit festem Schwerpunkt bei Rotation sowie Drift/Rückwärtsfahrt; Welt-Wasserhöhenspur und keine Segelschlaufen; Bilder vorhanden |
| A-G-00044 | Reine Historientests prüfen räumliche Interpolation, Referenzsprung, Zeit-/Budgetgrenzen und große Schritte; GPU prüft proportional begrenzte Geometrie, eingefrorene Zeit, Replay und fehlerhafte Fahrtdaten ohne Verlust der Modellansicht |
| A-G-00045 | Diese Matrix, CORE-Lieferplan und Bedienungs-/Nachweisdokumentation; persönliche fachliche/Sichtabnahme und separate Leistungsabnahme offen |

Technische Grenzen: vier persistente Parameter je Profil; Spuralter
höchstens 86400 s, Abtastung/Markenabstand höchstens 10000 m, jeweils
endlich > 0; Budget 2 bis 65536. Ungültige Dezimalwerte werden diagnostiziert,
nicht still reduziert. Markengeometrie zusätzlich auf das Punktbudget
begrenzt; ältere Marken weichen jüngeren. Rohzustände werden in Paketen
von höchstens 256 übernommen; Renderhistorie selbst bleibt budgetbegrenzt.

Belege im lokalen Build:
`C:\spdeps\guibuild\graphics-evidence\water-motion\report.json` und
benachbarte BMPs. Echte Läufe und rein grafische 5-m-Translationsbilder
(`*_synthetic_motion.bmp`) sind ausdrücklich getrennt.
Die neuen [Bedienungs- und technischen Hinweise](../Dokumentation/Testantrieb_und_Fahrtdarstellung.md)
beschreiben Datenbezüge, Grenzen und Testselektor.
Keine allgemeine Performancezusage aus der Testlaufzeit.
