---
description: "Use when committing, merging, tagging, pushing, publishing or preparing a release, Tagesstart, Tagesende or vor grosser Aenderung. Projektregeln fuer Git-Freigaben und Versionssicherung."
---
# Git-, Commit- und Release-Regeln

Diese Instruction enthält die vollständige projektweite Regelung für
Git-/Release-Aktionen; eine separate Release-Anweisung wird nicht parallel
gepflegt. Release-Regeln dürfen Entwicklungsarbeit nicht blockieren.

Antworte kurz und auf Deutsch. Unterscheide Entwicklungsaufträge von
ausdrücklich beauftragten Git-/Release-Aktionen.

## Geltungsbereich

- Jeder Release-Ablauf gilt ausschließlich für dieses Repository
  SegelPhysik-GUI. Core hat einen strikt getrennten Versions- und
  Release-Verlauf; Aktionen hier lösen keinerlei Release-Aktion dort aus.
- Entwicklungsaufträge erlauben Code-, Test- und Dokumentationsänderungen
  auf dem aktuell ausgecheckten benannten Branch, einschließlich
  Entwicklungsbranches. Bestehende Projektregeln und fachliche
  Abnahmekriterien gelten weiter.
- Ein Entwicklungsauftrag ist keine Freigabe für Commit, Push, Tag oder
  Release. Diese Aktionen nur auf ausdrücklichen Auftrag ausführen.
  Die unten genannten Stichwörter gelten nur als Auftrag, nicht bei ihrer
  bloßen Erwähnung in einer Frage, einem Zitat oder einer Dokumentation.
- Git-/Release-Abläufe gelten nur für die jeweils beauftragte Aktion.
  Sie dürfen normale Entwicklungsarbeit nicht blockieren.
- Nicht eigenmächtig den Branch wechseln, mergen oder rebasen.
  Bei unklaren oder widersprüchlichen Aufträgen gezielt nachfragen.

## Versionsschema

- GUI-Releases verwenden Tags `G-X.Y` (ohne `v`), z. B. `G-0.3`,
  `G-0.4`, `G-1.0`, `G-1.1`. Core verwendet unabhängig davon `C-X.Y`.
- Jeder normale GUI-Tag erhöht `Y` um 1: `G-0.3 → G-0.4`.
- Ein GUI-Release erhöht `X` um 1 und setzt `Y` auf 0:
  `G-0.7 → G-1.0`, `G-1.4 → G-2.0`.
- Aktuellen Stand finden:
  `git tag --list --sort=-v:refname | Where-Object { $_ -match '^G-[0-9]+\.[0-9]+$' }`
  (erster Treffer; berücksichtigt auch mehrstellige Werte wie `G-0.10`).
  Unpräfixierte Tags des alten SegelPhysik-Repositories und Tags mit drei
  Versionsstellen sind Historie und werden ignoriert.
- Commits tragen keine eigene Versionsnummer.
- Der bisherige Versionsstartpunkt `0.3` wird für GUI als `G-0.3` geführt.
  Ältere Tags (`0.2`, `F0`, `F1`, `v4.0-…`, Drei-Stellen-Tags) sind
  Historie.

## Grundregeln

1. Änderungen nur auf ausdrücklichen Auftrag committen. Ein uncommitteter
   Arbeitsstand ist während der Entwicklung erlaubt; am Ende verbleibende
   Änderungen benennen. Fremde oder nicht freigegebene Änderungen weder
   übernehmen noch verwerfen.
2. Commit-Nachricht kurz im Stil des Repos, am Ende immer:
   `Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`.
   Inhalt der Einträge siehe „Aussagekräftige Einträge“.
3. Vor jedem Commit Tests ausführen:
   `.\.venv\Scripts\python.exe -m pytest -q`,
   mit `QT_QPA_PLATFORM=offscreen`. Schlagen sie fehl: nicht committen.
   Bei Entwicklungsaufträgen Fehler im Auftragsumfang untersuchen und beheben.
4. Nie `--force`, nie `--no-verify`, nie Verlauf umschreiben und nie
   Änderungen verwerfen.
5. Entwicklung, Commit und Branch-Push sind auf dem aktuellen benannten
   Branch erlaubt. Vor Git-Aktionen Branch und Arbeitsbaum prüfen.
   Bei Detached HEAD vor Änderungen abbrechen und einen Zielbranch klären.
   Versionstags und GitHub-Releases bleiben auf `main` beschränkt; auf
   Entwicklungsbranches diese Aktionen auslassen und melden. Eine ausdrücklich
   gewünschte Ausnahme vor dem Taggen klären.

## Abläufe nach ausdrücklichem Stichwort

**„commit“**

Status und Änderungen prüfen, Tests ausführen und nur die zum Auftrag
gehörenden, freigegebenen Dateien gezielt stagen und committen. Kein pauschales
`git add -A` bei fremden oder nicht freigegebenen Änderungen. Gibt es nichts
zu committen, melden und enden. Kein Push und kein Tag.

**„Tagesstart“**

1. Offene, freigegebene Änderungen committen (wie oben).
2. Den aktuellen Branch mit seinem konfigurierten Upstream vergleichen.
   Ungepushte Commits auf diesem Branch pushen. Fehlt ein Upstream,
   Remote/Zielbranch vor der Veröffentlichung klären.
3. Nur auf `main` den Tag `G-X.(Y+1)` auf HEAD setzen (`git tag -a`) und
   gezielt pushen, sofern seit dem letzten Tag neue Commits existieren.
   Auf Entwicklungsbranches ohne Tag weiterarbeiten. Kein Release.

**„Tagesende“**

Offene freigegebene Änderungen committen, dann den aktuellen Branch an seinen
konfigurierten Upstream pushen. Fehlt ein Upstream, Remote/Zielbranch vorher
klären. Nur auf `main` die zum Auftrag gehörenden Versionstags gezielt
mitpushen. Danach melden, was gepusht wurde.

**„vor großer Änderung“**

Offene freigegebene Änderungen committen und den aktuellen Branch wie bei
„Tagesende“ pushen. Nur auf `main` den Tag `G-X.(Y+1)` setzen und gezielt
pushen, sofern seit dem letzten Tag neue Commits existieren. Auf
Entwicklungsbranches genügt die Sicherung per Commit und Branch-Push.
Danach freigeben.

**„Release“**

Nur auf ausdrücklichen Zuruf. Vor Beginn `main` prüfen. Auf einem anderen
Branch den Release-Ablauf abbrechen und melden; keine Teilveröffentlichung
und kein automatischer Branchwechsel.

1. Offene freigegebene Änderungen committen, Tests ausführen und pushen.
2. Neuen GUI-Tag `G-(X+1).0` setzen und pushen.
3. Notizen aus
   `git log <letzter Release-Tag>..HEAD --format="- %s (%h)"` erstellen.
   Letzter Release-Tag = letzter Tag mit `.0`; gibt es keinen, den gesamten
   Verlauf verwenden.
4. GitHub-Release anlegen:
   `gh release create <Tag> --title <Tag> --notes-file ...`.
   Ist `gh` nicht installiert, Notizen ausgeben und den Link
   `https://github.com/BergStefanBerlin/SegelPhysik-GUI/releases/new?tag=<Tag>`
   nennen.

## Aussagekräftige Einträge

Vor jedem Commit und Tag `git status --short` und
`git diff --cached --stat` lesen und daraus den Eintrag ableiten.
Nie Platzhalter wie „Update“, „Änderungen“ oder nur einen Dateinamen verwenden.

**Commit**

- Betreff (max. 72 Zeichen, Deutsch): `<Bereich>: <was wurde erreicht>`,
  z. B. `Hydrostatik: exakter Ebenenschnitt, 25-39x schneller`.
- Bereiche: `Hydrostatik`, `Solver`, `GUI`, `Echtzeit`, `Test`, `Doku`,
  `Werkzeug`, `Release`.
- Bei mehreren Dateien oder Gründen: Leerzeile, dann 1–4 Stichpunkte mit
  *Was* und *Warum* sowie bei Messungen den Zahlenwert (vorher → nachher).
- Mehrere unabhängige Themen im Arbeitsbaum: getrennte Commits, nicht ein
  Sammelcommit.
- Keine Versionsnummer im Betreff.

**Tag** (`git tag -a`)

- Erste Zeile: `<Tag>: <Hauptergebnis seit dem letzten Tag>`.
- Darunter 2–5 Stichpunkte aus
  `git log <letzter Tag>..HEAD --format="- %s"`, ohne Duplikate.
- Einen Tag ohne neue Commits nicht setzen.

**Release-Notizen**

Nach Bereichen gruppieren (Neu, Verbessert, Behoben, Doku), nicht als reine
Commit-Liste. Messwerte und bekannte Einschränkungen nennen.

## Abbruch bei Git-/Release-Aktionen

Bei fehlgeschlagenen Tests nicht committen. Push-Ablehnung (z. B. Remote ist
weiter), bereits anderweitig vergebener Tag oder fehlender GitHub-Zugang:
nichts erzwingen, insbesondere nicht force-pushen, mergen oder Verlauf
umschreiben. Ein fehlender GitHub-Zugang blockiert keine lokale
Entwicklungsarbeit.

## Abschlussmeldung

Bei Git-/Release-Aktionen immer drei Zeilen:

- Was wurde committet, getaggt, gepusht (Hashes, Tag).
- Aktueller Stand: `git describe --tags --always --dirty`.
- Nächster Tag bzw. nächstes Release, falls jetzt gewünscht.

Danach die Tonfolge abspielen:
`[console]::Beep(262,350); Start-Sleep -Milliseconds 80; [console]::Beep(330,350); Start-Sleep -Milliseconds 80; [console]::Beep(392,350); Start-Sleep -Milliseconds 80; [console]::Beep(523,600)`

Bei Entwicklungsaufträgen stattdessen Umsetzung, tatsächliche Testergebnisse,
offene Punkte und uncommittierten Stand zusammenfassen. Keine Release-
Abschlussmeldung oder Tonfolge erforderlich.
