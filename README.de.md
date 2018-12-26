# K,O.Y-Tastaturlayout: Umschaltung auf QWERTZ, wenn Strg, Alt und/oder Win gedrückt wird (Linux)

*Read this in English (README.md)*

Dieses Projekt ist ein Fork von https://github.com/tbocek/dvorak (Umschaltung auf QWERTY im Dvorak-Layout). Dort findest du auch Informationen zur Entstehungsgeschichte sowie hilfreiche Links.

---

Wer das KOY-Layout verwendet weiß, dass häufig verwendete Shortcuts wie Strg-c, Strg-v oder Strg-z darin nicht sehr einfach zu erreichen sind. Deshalb habe ich mir gewünscht, ich könnte zwar das KOY-Layout beim normalen Schreiben verwenden, aber bei Shortcuts würden die aufgedruckten Zeichen (QWERTZ) gelten. Thomas Bocek hat dieses Problem für Dvorak gelöst und ich habe seinen Code einfach für KOY angepasst.

## Probleme

In der Beschreibung des Originalprojekts steht, dass Eclipse das eingestellte Tastaturlayout ignoriert und deshalb auch QWERTZ gilt, wenn Dvorak (oder KOY) eingestellt ist. Wenn dieses Programm aktiv ist, kommt Eclipse durcheinander.

## Installation

 * Kompiliere das Programm mit ```make```
 * Optional: Verschiebe das Programm nach /usr/local/sbin/: ```sudo mv koy /usr/local/sbin/```
 
### Anwendungsszenario 1:

Wer regelmäßig zwischen QWERTZ- und KOY-Layout wechselt, kann das Programm manuell oder in einem Skript wie folgt starten:
```sudo /usr/local/sbin/koy /dev/input/by-path/platform-i8042-serio-0-event-kbd keyb```
Hierbei muss die verwendete Tastatur (/dev/input/...) eingesetzt werden.
Ich benutze in meinem Skript zum Layout-Wechsel tmux, um das Programm in den Hintergrund zu schicken:
```tmux new-session -d 'sudo /usr/local/sbin/koy /dev/input/by-path/platform-i8042-serio-0-event-kbd keyb'```
(/usr/local/sbin/koy muss in die /etc/sudoers eingetragen werden, damit die Passwort-Abfrage entfällt.)

### Anwendungsszenario 2:

Das KOY-Layout ist dauerhaft aktiv und die Shortcut-Umschaltung soll für jede eingesteckte Tastatur automatisch gestartet werden. Installiere hierfür den entsprechenden Service mit:
 * ```sudo make install```
(Dies kopiert die drei Dateien: koy, 80-koy.rules und koy@.service)
