# Developer Guide

Welcome! This instruction file assumes you've set up the [project environment](./instructions.md). This file describes changes made to the SDK to work with the lidar so future developers can modify these as necessary for their project or for debugging.

Modifications apply to ultra_simple (audible sound) and ultra_simple_u (ultrasonic sound).

### Wiring Pi

1) Including wiringPi in the header:
```cpp
#include <wiringPi.h>
```

2) Adding wiringPi to the Makefile:
```txt
LD_LIBS += -lstdc++ -lpthread -lwiringPi
```

3) Renaming all instances of `delay` to `lidarDelay`:

> If you're using Visual Studio Code, you can right-click and 'Rename Symbol' to search and rename throughout the whole SDK.

> The Slamtec SDK and Wiring Pi share the method name `delay`. However, they are different methods which throws an error. We need to change one to differentiate them, so we're renaming the method name in the SDK.

```cpp
static inline void lidarDelay(sl_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}
```

4) Initializing Wiring Pi and Pin

> It's important to use `wiringPiSetupGpio()` and not `wiringPiSetup()` as this project uses GPIO pins.

```cpp
if (wiringPiSetupGpio() < 0) {
      printf("Failed to initialize GPIO\n");
      goto on_finished;
   }

pinMode(PWM_PIN, PWM_OUTPUT);
```

5) Stop PWM on program end
```cpp
on_finished:
   if(drv) {
      delete drv;
      drv = NULL;
   }
   pwmWrite(PWM_PIN, 0);
   return 0;
```

### PWM Function

> Why `19200000`? 19.2 MHz is the clock source oscillator used by the Wiring Pi GPIO library. [Source](https://youngkin.github.io/post/pulsewidthmodulationraspberrypi/)

To calculate PWM frequency use the formula:
```
PWM Frequency = 19200000 / (divisor * range)
```

For example, to achieve a 100 kHz PWM frequency, divide the 19,200,000 Hz clock by 192 (divisor), resulting in desired frequency.

```cpp
const int PWM_PIN = 18;

void playFrequency(int frequency) {
    // calculate clock for desired frequency
    int clockDivisor = 192;
    int pwmRange = 19200000 / (clockDivisor * frequency);

    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(clockDivisor);
    pwmSetRange(pwmRange);

    // start pwm with 50% duty cycle
    pwmWrite(PWM_PIN, pwmRange / 2);

    // delay(500); // play for 500ms

    // pwmWrite(PWM_PIN, 0);
}
```

### Main Scanning Loop

The lidar scans 360 degrees and detects objects within 2 meters (2000 mm).
```cpp
if (nodes[pos].dist_mm_q2 / 4.0f < 2000.0f) {
```

And `playFrequency` sounds either 2kHz or 40kHz (ultrasonic) for the `ultra_simple` or `ultra_simple_u` file respectively.

```cpp
playFrequency(40000);
```

```cpp
while (1) {
   sl_lidar_response_measurement_node_hq_t nodes[8192];
   size_t   count = _countof(nodes);

   op_result = drv->grabScanDataHq(nodes, count);

   if (SL_IS_OK(op_result)) {
      drv->ascendScanData(nodes, count);
      for (int pos = 0; pos < (int)count ; ++pos) {
            // distance is measured in mm
            // play a tone or stop if no object detected
            if (nodes[pos].dist_mm_q2 / 4.0f < 2000.0f) {
               printf("close!\n");
               playFrequency(40000); // 40kHz which is ultrasonic
            } else {
               pwmWrite(PWM_PIN, 0);
               printf("Dist: %02.2f \n", nodes[pos].dist_mm_q2 / 4.0f);
            }
      }
   }

   if (ctrl_c_pressed){ 
      break;
   }
}
```