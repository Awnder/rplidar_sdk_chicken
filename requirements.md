# Requirements Documentation

This project uses a LIDAR sensor to detect objects in a 360-degree 2-meter radius. When detecting an object, it produces an ultrasonic or audible sound from a Piezo horn. A Raspberry Pi fascilitates communication between sensor and actuator in a C++ program.

### Functional Requirements
1. The system must provide a minimal example to demonstrate basic LIDAR functionality.
2. The system must initialize the LIDAR device and retrieve scan data.
3. The system must display the scan data in a human-readable format.

### Non-Functional Requirements
1. The example shall be lightweight and require minimal dependencies.
2. The example shall execute within 1 second for a single scan operation.
3. The example shall include comments to explain the code for educational purposes.

### Acceptance Criteria
1. The example must successfully initialize the LIDAR device without errors.
2. The example must retrieve and display scan data in a readable format.
3. The example must terminate gracefully after execution.

## Feature 1: Sensor LIDAR Data Collection

### Functional Requirements
1. The system must detect objects across a full 360-degree field of view.
2. The system must detect objects within a 2-meter range.
3. The system must check the health of the LIDAR device.

### Non-Functional Requirements
1. The system shall provide feedback for debugging.
2. The system shall process LIDAR data within 1 second.
3. The system shall ensure consistent operation without crashes 99.9% of the time.
4. The system shall turn off the LIDAR automatically on error.

### Acceptance Criteria
1. The system must detect objects as small as a 9 1/2 x 11 piece of paper.
2. The system must detect objects across a full 360-degree field of view.
3. The system must detect objects within 2 meters.
4. The system must handle LIDAR health checks and report errors.

## Feature 2: Actuator Piezo Horn Sound

### Functional Requirements
1. The system must produce a 40kHz ultrasonic tone.
2. The system must produce a 2kHz audible tone as an alternative.

### Non-Functional Requirements
1. The system shall produce the chosen audible or ultrasonic tone 99.9% of the time.
2. The system shall provide the ability to change audible and ultrasonic tones.

### Acceptance Criteria
1. The system must allow switching between ultrasonic and audible tones without errors.

## Feature 3: Raspberry Pi Integration

### Functional Requirements
1. The system must allow remote access to the Raspberry Pi for program execution and debugging.
2. The system must use hardware pulse width modulation via GPIO pins to control the Piezo horn.
3. The system must collect data from the LIDAR.
4. The system must activate the Piezo horn in response to data.

### Non-Functional Requirements
1. The system shall be compatible with Raspberry Pi OS Lite (64-bit) and WiringPi library.
2. The codebase shall contain contributor, developer, and quick-start guides to explain codebase and aid debugging.

### Acceptance Criteria
1. The system must handle program execution without any errors.
2. The system must provide a way to shutdown the program.

