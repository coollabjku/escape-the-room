# Escape Room Koffer Program

Hier befindet sich das Arduino Programm des Escape Room Koffers. 

Das Spiel besteht derzeit aus 5 Rätseln:
* Start: Zum Start muss der rote Taster ca. 2s lang gedr ̈uckt werden um den
Mikrocontroller zu starten.
* Startcode: Zu Beginn muss mit dem Keypad ein Startcode eingegeben werden.
Der Startcode lautet:”42069” Mit der * Taste wird die Eingabe best ̈atigt, mit der
\# können Zeichen gel ̈oscht werden. Wird ein falscher Code eingegeben werden 10s
abgezogen.
* Drahtbr ̈ucken: Mit vier Drahtbr ̈ucken (Jumper-Cable) m ̈ussen vier der sieben
Jumper-Pins verbunden werden. F ̈ur jede richtige Drahtbr ̈ucke leuchtet eine rote
LED.
* Potentiometer: Die drei Potentiometer m ̈ussen an die richtige Stellung gedreht
werden. Für jeden richtigen Poti leuchtet eine der gr ̈unen LEDs.
* Quiz: Beim Quiz ist eine beliebe Anzahl von auf der SD Karte gespeicherten Fragen
richtig zu beantworten. Für jede falsche Antwort, werden 10s Zeit abgezogen.
* Farben: Die Farbkarten m ̈ussen in folgender Reihenfolge in den ”Kartenleser”
eingef ̈uhrt werden: rot, grün blau. F ̈ur jede richtige Karte leuchtet eine der gr ̈unen
LEDs auf. Sobald eine falsche Karte zwischendurch eingef ̈uhrt wird, muiss erneut
von Anfang mit der roten Karte begonnen werden.
* Ende: Nach Beendigung wird der Mikrocontroller automatisch abgeschaltet.


## Quiz 
Die Quizes liegen in Form einer CSV Datei auf der SD Karte vor. Welche CSV Datei
geladen werden soll, kann im Setup Men ̈u eingestellt werden. Bei der Durchf ̈uhrung des
Quizes hat man beliebig lange Zeit die Frage zu lesen. Mit der 0 Taste auf dem Keypad gelangt man zu den Antwortm ̈oglichkeiten. Sobald man eine Antwort am Keypad eingibt
wird die Auswertung der angezeigt, sowie bei falscher Antwort auch die richtige Antwort.
Durch Dr ̈ucken der 0 Taste gelangt man zu der n ̈achsten Frage. Dies wiederholt sich so
lange bis zum Ende vom Quiz, oder bis die Zeit aus ist.

###
Die Fragen sowie die Antworten m ̈ussen in CSV Dateien auf der SD Karte zu speichern.
Der Dateiname muss nach folgendem Schema aufgebaut sein: n.csv gespeichert werden,
wobei n eine Zahl zwischen 0 und 255 sein muss. Die CSV Dtaei muss wie folgt aufgebaut
sein:
Frage;Antwort 1;Antwort 2;Antwort 3;Antwort 4;richtige Antwort
Wer erand Microsoft?; Steve Jobs; Bill Gates; Alan Turing; Photoshop Phillip;1
...
Wichtig ist, dass die erste Zeil der CSV Datei den Titel beinhaltet ( Frage;Antwort1,...),
da ansonsten die erste Frage nicht ausgegeben wird. Die Frage darf dabei nicht mehr als
80 Zeichen umfassen. Eine Antwort darf nicht l ̈anger als 17 Zeichen sein.

## Setup Menü
In dem Setup Men ̈u kann jederzeit die Zeit eingestellt werden.
Dazu muss w ̈ahrend des Einschaltens die 5 auf dem Keypad gedr ̈uckt und gehalten
werden bis das Setup Men ̈u erscheint.
Nun kann mit dem Keypad die Zeit eingestellt werden. # dient dabei als Backspace und
\* als Enter. Nach einem Druck auf \* wird die eingestellte Zeit nochmals angezeigt. Um
die CSV Datei einzustellen muss nochmals auf * gedr ̈uckt werden. nun kann die CSV
Datei eingestellt werden:
Dzu mit dem Keypad die entsprechende Zahl der Datei eingeben. F ̈ur ”2.csv” w ̈are das
”2”. Nach einem weiteren Druck auf die \* Taste wird die Eingabe best ̈atigt.
Die Einstellungen sind auf dem EEPROM gespeichert. Nach einem erneuerten Starten
Mikrocontrollers, sind die neuen Einstellungen bereits geladen.
