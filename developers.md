# Developer Guide

Welcome! This instruction file assumes you've set up the [project environment](./instructions.md). This file describes changes made to the SDK to work with the lidar so future developers can modify these as necessary for their project or for debugging.

Modifications apply to ultra_simple (audible sound) and ultra_simple_u (ultrasonic sound).

### Making the Program
If you make changes to the files and want to compile the binaries, navigate to the root of the sdk and type `make` to build the sdk and code files. This will compile the code into `output/Linux/`.

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

void playFrequency() {
    // calculate clock for desired frequency
    int frequency = 2000; // Frequency in Hz
    // int frequency = 40000; for ultrasonic 
    int clockDivisor = 192;
    int pwmRange = 19200000 / (clockDivisor * frequency);

    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(clockDivisor);
    pwmSetRange(pwmRange);
    pwmWrite(PWM_PIN, pwmRange / 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Sustain tone
    pwmWrite(PWM_PIN, 0); // Stop after delay
}
```

### Main Scanning Loop

This loop uses a lot of variables. They are declared above main().

First grab scan data from the lidar:
```cpp
sl_lidar_response_measurement_node_hq_t nodes[8192];
size_t   count = _countof(nodes);

op_result = drv->grabScanDataHq(nodes, count);
```

This program uses data from 360 degrees and in between 1 mm and 2 meters (2000 mm).
```cpp
if (
   (angle >= 0.0f && angle < 360.0f) 
   && (nodes[pos].dist_mm_q2 / 4.0f > minDistance 
   && nodes[pos].dist_mm_q2 / 4.0f < maxDistance)
) {
   printf("CLOSE! Dist: %08.2f Angle: %02.2f\n", nodes[pos].dist_mm_q2 / 4.0f, angle);
   objectDetectedInCurrentScan = true;
   break; // no check further as object already detected
}
```

And `playFrequency` sounds either 2kHz or 40kHz (ultrasonic) for the `ultra_simple` or `ultra_simple_u` file respectively.

After detecting an object, use debouncing to prevent frequent and often overlapping start/stop pwmWrites.
```cpp
if (elapsedMsSinceDetection >= debounceDurationsMs) {
```

Finally, play a frequency every so often (timeUntilNextSoundMs is set at 1 sec)
```cpp
if (!isBuzzerActive || elapsedMsSinceSound >= timeUntilNextSoundMs) {
   printf("CLOSE! Object detected for %d ms\n", debounceDurationsMs);
   playFrequency();
   lastSoundTime = currentTime;
   isBuzzerActive = true;
}
```
