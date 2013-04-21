					jw, Sat Mar 30 17:18:48 CET 2013
Buzzer
======
Zwei Stück gebaut, einmal mit 4m Kabel, einmal mit 10m Kabel.

Material
--------
  * Arcade Taster Jamma Mame Aktionstaster Neu Push Button Mikroschalter
    http://www.ebay.de/itm/220923514291, 1.60 EUR
    Durchmesser: Kappe 22mm, Gewinde 28mm, Ring 36mm
    Länge: Gewinde unter Ring 31mm
  * 3x LED Lumitronix#13601 orange, 1.7V 20mA
  * 3x LED Lumitronix#13001 weiss, 3.1V 18mA
  * Telefonkabel 4polig 
  * Sperrholz 35mm dick, Rohzuschnitt 105mm x 105mm
  * Sperrholz 6mm dick, Rohzuschnitt 80mm x 80mm
  * Schrumpflschlauch, Kabelbinder, Heisskleber


Anschlussbelegung
-----------------
 1 ws: Taster
 2 bn: Taster und (-) LEDs
 3 gn: (+) LEDs weiss, 1 3 5
 4 ge: (+) LEDs orange, 2 4 6

LED Anordnung
-------------

        1
    6       2
       (O)
    5 	    3
        4

	|
	\____Kabel


Stromaufnahme
-------------
 gn: 390ohm an 5V: 5mA
 ge: 390ohm an 5V: 7mA

Aufbau
------
  Drehteil aus Sperrholz 35mm, mit 
  eingesetztem Boden aus Sperrholz 6mm.
  Der Mittelbereich der Bodenscheibe wird auf 3mm ausgedünnt, damit der Taster
  Platz hat.

  FreeCAD Konstruktion: buzzer.fcstd
  Abmessungen siehe: buzzer_round.png
  Diese Skizze zeigt eine alternative Konstruktion aus zwei Teilen je 21mm
  dick.  Die LED-Bohrungen werden 4mm gebohrt, der Rand der LEDs wird teilweise
  abgezwickt, damit sie sich leicht reinschieben lassen. 

  Die Kabelbohrung 4.5mm bohren. 4cm Schrumpfschlauch auf das Kabel
  aufschrumpfen. Kabel mit Schrumpfschlauch einschieben, innen einen kleinen
  Kabelbinder als Zugentlastung um Kabel und Schrumpfschlauch festzurren.

  Die Anschlussdrähte der LEDs werden ca 8mm unter dem LED-Gehäuse seitlich 45°
  gebogen, so daß der kürzere Draht den engeren Knickradius hat. In die Löcher
  einschieben, so daß die LED-Linse plan auf der Oberfläche und am 28mm Radius
  erscheint. Danach mit Heisskleber sichern. Die kürzeren (-) Beinchen (innen)
  alle jweils nach links biegen, und das Beinchenende auf die Biegung der
  Nachbar-LED löten, so dass ein möglichst großer Kreis entsteht. Braunen Draht
  anlöten.

  Grünen Draht an das hochstehende (+) Beinchen der orangen LED nahe der
  Kabeldurchführung anlöten. Dann mit Kabelbrücken die (+) Beinchen der beiden
  anderen orangen LEDs verbinden.  Gelben Draht an das nächstgelegen freie (+)
  Beinchen einer weissen LED anlöten, die (+) Beinchen der beiden anderen
  weissen LEDs verbinden.

  Weissen Draht auf einen Schaltkontakt des Tasters löten.  Den anderen
  Schaltkontakt mit dem (-) Ring verbinden.

