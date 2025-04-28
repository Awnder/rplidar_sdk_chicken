from gpiozero import Button
import subprocess

# global var to store process ref
process = None

# Configuration (BCM numbering)
BUTTON_PIN = 17  # GPIO 17 (BCM)
SCRIPT_PATH = "./ultra_simple_u"
HOLD_TIME = 3  # Seconds to hold for exit

def run_script():
    print("Button pressed - executing script")
    process = subprocess.Popen(["sudo", SCRIPT_PATH, "--channel", "--serial", "/dev/ttyUSB0", "460800"], check=True)

def exit_script():
    global process
    if process and process.poll() is None:
        print("Button held - exiting script")
        process.terminate()
        process.wait()
        process = None
        print("Script terminated")
    else:
        print("No script running to terminate")
    
# Set up button with pull-down resistor
button = Button(BUTTON_PIN, pull_up=False, hold_time=HOLD_TIME)
button.when_pressed = run_script
button.when_held = exit_script

print("Press button to run script (CTRL+C to exit)")
try:
    while True:
        pass  # Keep program running
except KeyboardInterrupt:
    button.close()