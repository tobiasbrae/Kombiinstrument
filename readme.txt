======================================= [Kombiinstrument] =========================================

Dieses Projekt ist dazu entworfen worden, ein Kombiinstrument mit einer RGB-Beleuchtung
zu versehen und dessen Farbe abhängig von der Drehzahl entsprechend des austauschbaren
Datensatzes zu steuern. Das vorliegende Dokument soll dabei helfen, die Funktionsweise
zu verstehen und das Projekt selber zu verwenden.

Autor: Tobias Brächter
Letzte Änderung: 2019-05-30

========================================== [Umfang] ===============================================

Das Projekt umfasst den Schaltplan und die Software für den Controller, der im Fahrzeug
verbaut werden soll. Weiterhin ist eine Software enthalten, die Frequenzen/Drehzahlen
erzeugen kann, um den Datensatz zu testen, ohne dafür den Motor zu belasten. Außerdem
ist eine Software enthalten, mit der auf dem Computer Datensätze bearbeitet und auf den
Controller übertragen werden können. Sämtliche Software ist in C programmiert.

======================================== [Controller] =============================================

Als Controller für das Fahrzeug kommt ein Atmel ATmega8 zum Einsatz, der mit einer Frequenz
von 8 MHz betrieben wird. Der Schaltplan befindet sich in der Datei
"/Controller/schaltplan.png".

Funktionen:
-Messen der Motordrehzahl
-PWM Ansteuerung von 3 Kanälen (Rot, Grün, Blau...)
-Zwei (gleichzeitig geschaltete) Freigabeausgänge (gedacht für einen Starterknopf mit
 Freigabe-LED)
-Serielle Kommunikation über UART (19200 baud/s) zum Übertragen von Datensätzen
-Dauerhaftes Speichern des Datensatzes im EEPROM
-Automatisches Laden des Datensatzes aus dem EEPROM beim Einschalten

Die Struktur des Datensatzes sowie der Aufbau der Kommunikation werden im folgenden erläutert.

======================================== [kombiData] ==============================================

Der Datensatz wurde in diesem Projekt auf den Namen "kombiData" getauft. Die dazugehörigen
Structs werden in der Datei "kombiData.h" deklariert. In kombiData sind 4 Strukturen
untergebracht: breakpoint, dimmer, rpmStarter und hysteresis

Breakpoint:
-Der wesentliche Teil der Farbgebung wird über 3 Lookup-Tabellen für die PWM-Kanäle realisiert
-Alle 3 Tabellen teilen sich dieselben Stützstellen (breakpoints)
-Als Messgröße für die Lookup-Tabellen dient die gemessene Motordrehzahl
-Die Motordrehzahl wird in Umdrehungen pro Minute angegeben und ist für 4-Zylinder Motoren berechnet
-Jeder breakpoint muss eine höhere Drehzahl aufweisen, als der vorherige, um eine einwandfreie
 Funktion zu gewährleisten
-Zwischen den breakpoints wird linear interpoliert
-Das Tastverhältnis wird im Bereich von 0 bis 100 angegeben

Dimmer:
-Die Dimmer dienen dazu, einen Blink- bzw. An-/Abschwilleffekt zu erzeugen
-Ein Dimmer durchläuft periodisch die 4 Phasen: Rise, High, Fall, Low
-In High bzw. Low sind alle LEDs an (entsprechend eingestellter Helligkeit) bzw. aus
-Während Rise bzw. Fall werden die LEDs heller bzw. dunkler
-Die Zeiten der jeweiligen Phasen können seperat verändert werden
-Die eingestellten Zeiten sind vielfache von 100us (bis max. 65535, also ca. 6.5s)
-Der jeweilige Dimmer wird immer aktiv, wenn die Drehzahl sich im eingestellten Bereich befindet

rpmStarter:
-Die Starterfreigabe erfolgt, wenn die Drehzahl unter rpmStarterOn fällt
-Die Starterfreigabe wird zurückgenommen, wenn die Drehzahl über rpmStarterOff steigt

Hysteresis:
-Sowohl für die Breakpoints, als auch für die Dimmer lässt sich jeweils ein Hysteresewert einstellen
-Der nächste/vorherige Breakpoint wird erst aktiviert, wenn die jeweilige Stützstelle plus den
 angegebenen Hysteresewert überschritten wurde
-Für die Dimmer gilt dasselbe
-Die Hysterese soll verhindern, dass häufig zwischen zwei Effekten umgeschaltet wird, wenn sich die
 Drehzahl im Bereich der Grenze befindet (wird z.B. durch Zündzeitpunktverstellung oder leicht
 schwankende Programmlaufzeiten hervorgerufen)

======================================= [Kommunikation] ===========================================

Die Kommunikation erfolgt seriell über UART.

Parameter: 19200 baud/s, 8 Datenbits, 1 Stopbit, keine Parität

Generell besteht jede ausgetauschte Nachricht aus mindestens 2 Elementen: Ein einleitendes Symbol,
dass den Befehl darstellt, sowie ein 'e' als letztes Symbol der Nachricht, um das Ende zu
signalisieren. Dazwischen wird abhängig vom Befehl eine definierte Anzahl von Zeichen erwartet.
Beschreibt das einleitende Symbol keinen bekannten Befehl, stimmt die Menge der übetragenen Zeichen
nicht mit der Erwartung überein oder fehlt der Terminator, so wird die Nachricht verworfen. So wird
sichergestellt, dass nur bekannte und vollständig übertragene Befehle ausgeführt werden.

Befehle, die an den Controller gesendet werden könnnen:
-"se": Veranlasst den Controller, den Datensatz im Cache im EEPROM zu speichern.
-"re": Veranlasst den Controller, den Datensatz aus dem EEPROM in den Cache zu laden.
-"l<kombiData>e": Übetragt den Datensatz in den Cache des Controllers
-"ge": Fordert den Datensatz aus dem Cache an 
-"te": Veranlasst den Controller, den Datensatz aus dem Cache als aktiven Datensatz zu setzen
-"de": Lädt einen Demodatensatz in den Cache
-"a<0/1>e": (De-)Aktviert ein Echo bei unbekannten Befehlen (zu Debug-Zwecken)

Befehle, die der Controller sendet:
-"s<0/1/2>e": Wird nach jeder empfangenen Nachricht zurückgesendet und gibt den Status an:
	0: Befehl erfolgreich ausgeführt
	1: Unbekannter Befehl
	2: Befehl nicht korrekt übertragen
-"d<kombiData>e": Im Cache gespeicherter Datensatz (nach Aufforderung, diesen zu senden)

======================================== [Interface] ==============================================

Das Interface ist konsolenbasiert und dient dazu, den Datensätze zu bearbeiten und an den Controller
zu übertragen. Es besteht die Möglichkeit, Datensätze in Dateien zu speichern und diese wieder zu
laden, um verschiedene Datensätze probieren oder mit anderen austauschen zu können. Generell ist
das Programm so aufgebaut, dass man für jeden Breakpoint/Dimmer etc. jeweils alle Daten in einem
Befehl ändert. Da diese Art zu arbeiten schnell aufwendig wird, wenn man einen oder mehrere Parameter
iterativ in kleinen Schritten ändern möchte, wurde noch die Möglichkeit geschaffen, Skripte zu
erstellen und auszuführen. Dazu erstellt man einfach eine beliebige Datei, in die man die Befehle
schreibt, die ausgeführt werden sollen. Anschließend kann man dieses Skript ausführen lassen. Für
eine Übersicht der Befehle kann man im Interface "help" eingeben.

Es ist zu beachten, dass man unter Linux die entsprechenden Rechte benötigt, um auf die seriellen
Schnittstellen zuzugreifen. Zu diesem Zweck kann man das Programm entweder mit Root-Rechten starten
oder seinen Nutzer zu der Gruppe "dialout" hinzufügen.

====================================== [Frequenzgenerator] ========================================

Der Frequenzgenerator ist ebenso wie der Controller für einen Atmel ATmega8 entworfen worden, der
mit 8 MHz läuft.
Der Frequenzgenerator kommuniziert ebenso wie der Controller über UART mit denselben Einstellungen.
Jedoch unterscheidet sich die Kommunikation etwas vom Controller, da sie für den Einsatz mit Putty
entworfen wurde. Der Frequenzgenerator unterstützt sowohl das Einstellen einer Frequenz, als auch
einer Drehzahl. Weiterhin verfügt er über einen Modus für die Frequenzvorgabe durch ein Potentiometer.

Befehle, die an den Frequenzgenerator übermittelt werden können:
-"f<xxxxx>e": Stellt die Frequenz auf <xxxxx> Hz ein
-"r<xxxxx>e": Stellt die Frequenz auf <xxxxx> RPM ein
-"m<0/1/2/3>e": Stellt den Modus ein:
	0: Normal (eingestellte Frequenz)
	1: Analog In - Mit Hilfe des Potentiometers wird die Ausgangsfrequenz zwischen 1 Hz/RPM und
		dem mit "f..." bzw. "r..." eingstellten Wert variiert.
	2: On (Permanent an)
	3: Off (Permanent aus)
	
Die vom Frequenzgenerator gesendeten Nachrichten sind im Klartext verfasst.


Für tiefergehende Informationen sind die Quellcodes zu studieren.