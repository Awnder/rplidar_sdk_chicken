import RPi.GPIO as GPIO
import subprocess
import sys
import time

# Configuration
BUTTON_PIN = 10  # GPIO pin for button
SCRIPT_PATH = "./ultra_simple_u"  # Path to the script to run

# Button press handler

def button_callback(channel):
    start_time = time.time()
    while GPIO.input(channel):  # While button held
        if time.time() - start_time > 3:  # 3-second hold
            print("Exiting...")
            GPIO.cleanup()
    
    # Normal press
    subprocess.run(["sudo", SCRIPT_PATH, "--channel", "--serial", "/dev/ttyUSB0", "460800"])

# Setup GPIO
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)
GPIO.setup(BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

# Attach event detectors
GPIO.add_event_detect(BUTTON_PIN, GPIO.RISING, callback=button_callback, bouncetime=300)

# Keep alive
print("Ready - Press button quickly to run script, press and hold for 3 seconds to exit")
try:
    while True:
        pass
except KeyboardInterrupt:
    GPIO.cleanup()
