---
description: "Use when preparing or implementing any project version or release: create or update its single specification and single implementation plan, organize requirements, milestones, evidence and acceptance. Arbeitsanweisung fuer die Releaseumsetzung aller Versionsschritte."
---
# Arbeitsanweisung Releaseumsetzung

## Zweck und Geltungsbereich

Diese Instruction regelt die fachliche Vorbereitung und Umsetzung eines
Versionsschritts im Revisionslog. „Releaseumsetzung“
meint hier die Arbeit an Spezifikation, Umsetzungsplan, Code und Nachweisen,
nicht das Erstellen eines GitHub-Releases.

Sie gilt ab Inkrafttreten für den aktuell bearbeiteten und alle künftigen
Versionsschritte. Abgeschlossene historische Versionsschritte werden
dadurch nicht nachträglich geändert oder umstrukturiert.

Spezifikationen und Umsetzungspläne bleiben je Versionsschritt getrennte
Dokumente. Die Kennungen ihrer Anforderungen und Umsetzungsschritte sind
jedoch projektbezogen und laufen über Releases hinweg fort: In SegelPhysik-GUI
gelten `A-G-<laufende-nummer>` und `U-G-<laufende-nummer>`. Ein neuer
Versionsschritt setzt diese Zähler nicht zurück. Für Core gilt entsprechend
`A-C-<laufende-nummer>` und `U-C-<laufende-nummer>`.

## Verbindliche Dokumentstruktur

- Pro sachlogischem Themenpunkt gibt es genau **eine maßgebliche
  Spezifikationsdatei**. Keine thematisch unabhängigen Anforderungen in
  einer Sammelspezifikation bündeln. Eine Spezifikation darf mehrere
  kohärente Anforderungen enthalten, wenn sie denselben fachlichen
  Gegenstand beschreiben.
- Eine Spezifikation kann zunächst als klar markierter Entwurf geführt
  werden. Offene fachliche Punkte stehen in einer getrennten
  Entwurfspunkteliste unter `Spezifikationen/Entwurfspunkte/`. Entwürfe
  und ihre Listen erhalten keine A- oder U-Kennungen; sie sind nicht
  freigegeben und autorisieren keine Umsetzung.
- Erst nach ausdrücklicher Nutzerfreigabe zur Finalisierung werden
  angenommene Entwurfspunkte als Anforderungen in der finalen Spezifikation
  übernommen und nach der Nummerierungs-Instruction mit neuen A-Kennungen
  versehen. Die IDEE-zu-A-Zuordnung und der Status aller übrigen Punkte
  bleiben in der Entwurfspunkteliste nachvollziehbar.
- Für jeden Versionsschritt gibt es genau **einen maßgeblichen
  Umsetzungsplan**. Alle Umsetzungsschritte, Abhängigkeiten, Meilensteine
  und Umsetzungsstände dieses Versionsschritts stehen dort.
- Themen, Komponenten oder Arbeitspakete sind als Abschnitte, Tabellen und
  Meilensteine in den jeweiligen Dokumenten zu gliedern. Keine zusätzlichen
  Teil-, Neben- oder Ersatzspezifikationen für denselben sachlogischen
  Punkt und keine parallelen Umsetzungspläne für denselben Versionsschritt
  anlegen.
- Vor dem Anlegen oder Ändern prüfen, welche Dateien in den passenden
  Spezifikations- oder Versionsordnern bereits existieren. Eindeutige
  Hauptdateien nicht stillschweigend ersetzen, aufteilen oder umbenennen.
  Bei unklarer Abgrenzung oder konkurrierenden Kandidaten zuerst den Nutzer
  fragen.
- Unterstützende Nachweise wie Testergebnisse, Messdaten oder große
  maschinenlesbare Inventare dürfen bei Bedarf separat abgelegt werden.
  Sie sind keine zusätzlichen Spezifikationen oder Umsetzungspläne; die
  verbindlichen Anforderungen und Umsetzungsschritte bleiben in den beiden
  Hauptdateien und verlinken auf ihre Belege.
- Beispieldokumente sind nicht automatisch freigegebene Anforderungen oder
  verbindliche Vorlagen. Maßgeblich für den fachlichen Umfang ist
  ausschließlich die Spezifikation des jeweiligen Versionsschritts; für
  die Umsetzungsschritte ist es der zugehörige Umsetzungsplan.
  Unterstützende Nachweise belegen Ergebnisse, enthalten aber keine
  eigenständigen zusätzlichen Anforderungen oder parallelen Pläne.
  Übergreifende Projektregeln und die referenzierten Instructions gelten
  weiterhin.

## Anforderungen und Nachverfolgbarkeit

- Kennungen, Vergabe, Archivierung, Abhängigkeiten und Rückverfolgbarkeit
  richten sich nach der
  [Nummerierungs-Instruction](nummerierung.instructions.md). Sie ist die
  vollständige projektweite Regelung; Nummern nicht neu erfinden,
  wiederverwenden oder eigenständig umnummerieren.
- Entwurfskennungen `IDEE-G-…` markieren offene Entwurfspunkte, keine
  Anforderungen. Keine A- oder U-Kennung vorwegnehmen. In GUI erhalten
  finalisierte Anforderungen `A-G-…` und Umsetzungsschritte `U-G-…`.
- Die A- und U-Zähler laufen projektweit fort und werden nicht je
  Versionsschritt neu begonnen. Eine finale Spezifikation enthält die
  Anforderungen ihres sachlogischen Themas; der versionsbezogene
  Umsetzungsplan enthält die Umsetzungsschritte für diesen Versionsschritt.
- Jede aktive Anforderung steht in der einen Spezifikation und erhält ein
  prüfbares Ergebnis oder Abnahmekriterium. Jeder Umsetzungsschritt steht
  im einen Plan und verweist auf die zugehörigen Anforderungskennungen.
- Der Plan weist Abhängigkeiten und eine umsetzbare Reihenfolge aus.
  Meilensteine erhalten überprüfbare Eintritts- und Abnahmekriterien.
  Am Ende müssen alle aktiven Anforderungen Umsetzungsschritten und
  tatsächlichen Nachweisen zugeordnet sein.
- Zurückgezogene oder verworfene Anforderungen nicht stillschweigend
  löschen: nach der Nummerierungs-Instruction unter ihrer ursprünglichen
  Kennung archivieren. Übergänge ins Backlog während laufender Umsetzung
  nur mit Nutzerzustimmung.
- Technische Abweichungen, fehlende Nachweise und offene Punkte ausdrücklich
  dokumentieren. Umsetzung, Test, Integration und fachliche Abnahme nicht
  gleichsetzen; Status nur mit passenden Belegen ändern.

## Vorlagen und Quellen

- Für Kennungen und Rückverfolgbarkeit gilt ausschließlich die
  [Nummerierungs-Instruction](nummerierung.instructions.md).
- Ideenpapiere und Architekturentwürfe sind Quellen für die Ausarbeitung,
  aber nicht automatisch freigegebene Anforderungen. Nur ausdrücklich
  finalisierte Spezifikationen legen den verbindlichen fachlichen Umfang
  fest.
- Historische Dokumente sind Beispiele, keine zusätzliche Spezifikation
  oder paralleler Umsetzungsplan. Frühere Freigaben, Ausnahmen,
  Messgrenzen oder offene Punkte gelten nicht automatisch weiter.
- Schnittstellenregister nur dann pflegen, wenn das jeweilige Projekt ein
  solches Register eingerichtet hat; dessen projektlokale Regelung ist
  maßgeblich.

## Umsetzung und Abschluss

- In der Reihenfolge des freigegebenen Plans arbeiten. Den Plan fortlaufend
  mit tatsächlichem Status, Testergebnissen, Belegen und offenen Punkten
  aktualisieren; keine geplanten oder teilweise erledigten Arbeiten als
  abgeschlossen darstellen.
- Vor einer Erweiterung des Umfangs, Änderung fachlicher Verträge oder
  Änderung geltender Abnahmekriterien die nötige Nutzerentscheidung
  einholen.
- Zum Abschluss die aktive Anforderungsliste gegen Plan, Tests und Belege
  abgleichen. Fehlende Nachweise, bekannte Abweichungen und nicht erledigte
  Punkte sichtbar lassen; keine Vollständigkeit ohne Beleg behaupten.
- Git-Aktionen wie Commit, Push, Tag und Veröffentlichung folgen
  ausschließlich der
  [Release-Instruction](release.instructions.md) und benötigen den dort
  festgelegten ausdrücklichen Auftrag. Diese Arbeitsanweisung autorisiert
  keine Git-Veröffentlichung.
