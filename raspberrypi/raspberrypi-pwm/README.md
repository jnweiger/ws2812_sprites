Raspberry Pi PWM via DMA. Based on https://github.com/richardghirst/PiBits/tree/master/ServoBlaster

Overview
--------
raspberrypi-pwm provides an interface to drive multiple servos via the GPIO 
pins. You control the servo postions by sending commands to the driver saying
what pulse width a particular servo output should use. The driver maintains
that pulse width until you send a new command requesting some other width.

Currently is it configured to drive 8 servos.  Servos typically need an active
high pulse of somewhere between 0.5ms and 2.5ms, where the pulse width controls
the position of the servo.  The pulse should be repeated approximately every
20ms, although pulse frequency is not critical.  The pulse width is critical,
as that translates directly to the servo position.

The driver creates a device file, /dev/rpio-pwm, in to which you can send
commands.  The command format is "`servo-number`=`sero-position`", where servo
number is a number from 0 to 7 inclusive, and servo position is the pulse width
you want in units of 10us.  So, if you want to set servo 3 to a pulse width of
1.2ms you could do this at the shell prompt:

    echo 3=120 > /dev/rpio-pwm

**120 is in units of 10us, so that is 1200us, or 1.2ms.**

If you set a servo width to 0 it turns off the servo output, without changing
the current servo position.

The code supports 8 servos, the control signals of which should be connected
to P1 header pins as follows:

     Servo number    GPIO number   Pin in P1 header
          0               4             P1-7
          1              17             P1-11
          2              18             P1-12
          3              21             P1-13
          4              22             P1-15
          5              23             P1-16
          6              24             P1-18
          7              25             P1-22

When the driver is first loaded the GPIO pins are configure to be outputs, and
their pulse widths are set to 0.  This is so that servos don't jump to some
arbitrary position when you load the driver.  Once you know where you want your
servos positioned, write a value to /dev/rpio-pwm to enable the respective
output.  When the driver is unloaded it attempts to shut down the outputs
cleanly, rather than cutting some pulse short and causing a servo position to
jump.


Terminology
-----------

A servo has a specific period width (usually 20ms; see datasheet), and knows
its position based on the pulse width inside this period. Usually the minimum 
position is at 1 ms pulse-width, and the maximum position at 2 ms.

         |<--- Period Width (20ms) ------>|

         +--------+                       +--------+
         |        |                       |        |
    -----+        +-----------------------+        +------

      -->|        |<-- Pulse Width (1..2 ms)

Under The Hood
--------------
The driver allocates a timeslot of 2.5ms to each output (8 servos resulting in
a cycle time of 20ms).  A servo output is set high at the start of its 2.5ms
timeslot, and set low after the appropriate delay.  There is then a further
delay to take us to the end of that timeslot before the next servo output is
set high.  This way there is only ever one servo output active at a time, which
helps keep the code simple.

In the following description it refers to using the PWM peripheral.  For the
user space implementation it can instead use the PCM peripheral, see below
for details.  Using PCM is typically a better option, as the 3.5mm jack also
uses the PWM peripheral, so rpio-pwm can interfere with sound output.


**DMA**

The driver works by setting up a linked list of DMA control blocks with the
last one linked back to the first, so once initialized the DMA controller
cycles round continuously and the driver does not need to get involved except
when a pulse width needs to be changed.  

For a given servo there are four DMA control blocks; the first transfers a 
single word to the GPIO 'set output' register, the second transfers some 
number of words to the PWM FIFO to generate the required pulse width time, 
the third transfers a single word to the GPIO 'clear output' register, 
and the fourth transfers a number of words to the PWM FIFO to generate a 
delay up to the end of the 2.5ms timeslot.


**PCM**

While the driver does use the PWM peripheral, it only uses it to pace the DMA
transfers, so as to generate accurate delays. The PWM is set up such that it
consumes one word from the FIFO every 10us, so to generate a delay of 1.2ms the
driver sets the DMA transfer count to 480 (1200/10*4, as the FIFO is 32 bits
wide). The PWM is set to request data as soon as there is a single word free
in the FIFO, so there should be no burst transfers to upset the timing.

Richard Hirst used Panalyzer running on one Pi to mointor the servo outputs
from a second Pi. The pulse widths and frequencies seem very stable, even under
heavy SD card use. This is expected, because the pulse generation is effectively
handled in hardware and not influenced by interrupt latency or scheduling
effects.

The driver uses DMA channel 0, and PWM channel 1. It makes no attempt to
protect against other code using those peripherals. It sets the relevant GPIO
pins to be outputs when the driver is loaded, so please ensure that you are not
driving those pins externally.

I would of course recommend some buffering between the GPIO outputs and the
servo controls, to protect the Pi. That said, I'm living dangerously and doing
without :-) If you just want to experiment with a small servo you can probably
take the 5 volts for it from the header pins on the Pi, but I find that doing
anything non-trivial with four servos connected pulls the 5 volts down far
enough to crash the Pi!


Using the daemon
----------------

    $ sudo ./rpio-pwm
    Using hardware:        PWM
    Number of servos:      8
    Servo cycle time:      20000us
    Pulse width units:     10us
    Maximum width value:   249 (2490us)

The prompt will return immediately, and servod is left running in the
background. You can check it is running via the "ps ax" command.
If you want to stop servod, the easiest way is to run:

    $ sudo killall rpio-pwm

Note that use of PWM will interfere with 3.5mm jack audio output.  Instead
of using the PWM hardware, you can use the PCM hardware, which is less likely
to cause a conflict.  Please be aware that the PCM mode is very lightly tested
at present.  To use PCM mode, invoke servod as follows:

    $ sudo ./rpio-pwm --pcm
    Using hardware:        PCM
    ...
