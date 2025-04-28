import gpiod
import subprocess

# global var to store process ref
process = None

# Configuration (BCM numbering)
BUTTON_PIN = 17  # GPIO 17 (BCM)
SCRIPT_PATH = "./ultra_simple_u"
HOLD_TIME = 3  # Seconds to hold for exit
CHIP_NAME = "gpiochip4" # Pi 5 uses this

chip = gpiod.Chip(CHIP_NAME)
button_line = chip.get_line(BUTTON_PIN)

# Configure button as input with pull-down
button_line.request(
    consumer="button",
    type=gpiod.LINE_REQ_EV_BOTH_EDGES,  # Detect presses and releases
    flags=gpiod.LINE_REQ_FLAG_BIAS_PULL_DOWN
)

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

print("Press button to run script (CTRL+C to exit)")
try:
    while True:
        if button_line.event_wait(1):
            event = button_line.event_read()
            if event.event_type == gpiod.LINE_EVENT_RISING_EDGE:
                run_script()
            elif event.event_type == gpiod.LINE_EVENT_FALLING_EDGE:
                # Check if button is held for exit
                if event.timestamp - event.timestamp > HOLD_TIME:
                    exit_script()
        pass  # Keep program running
except KeyboardInterrupt:
    button_line.release()