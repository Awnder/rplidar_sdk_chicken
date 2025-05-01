/*
 *  SLAMTEC LIDAR
 *  Ultra Simple Data Grabber Demo App
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2020 Shanghai Slamtec Co., Ltd.
 *  http://www.slamtec.com
 *
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <chrono>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <wiringPi.h>

#include "sl_lidar.h" 
#include "sl_lidar_driver.h"
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
static inline void lidarDelay(sl_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}
#endif

using namespace sl;

void print_usage(int argc, const char * argv[])
{
    printf("Usage:\n"
           " For serial channel\n %s --channel --serial <com port> [baudrate]\n"
           " The baudrate used by different models is as follows:\n"
           "  A1(115200),A2M7(256000),A2M8(115200),A2M12(256000),"
           "A3(256000),S1(256000),S2(1000000),S3(1000000)\n"
		   " For udp channel\n %s --channel --udp <ipaddr> [port NO.]\n"
           " The T1 default ipaddr is 192.168.11.2,and the port NO.is 8089. Please refer to the datasheet for details.\n"
           , argv[0], argv[0]);
}

bool checkSLAMTECLIDARHealth(ILidarDriver * drv)
{
    sl_result     op_result;
    sl_lidar_response_device_health_t healthinfo;

    op_result = drv->getHealth(healthinfo);
    if (SL_IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
        printf("SLAMTEC Lidar health status : %d\n", healthinfo.status);
        if (healthinfo.status == SL_LIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, slamtec lidar internal error detected. Please reboot the device to retry.\n");
            // enable the following code if you want slamtec lidar to be reboot by software
            // drv->reset();
            return false;
        } else {
            return true;
        }

    } else {
        fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
        return false;
    }
}

// BUZZER VARIABLES AND FUNCTION

// hardware capable PWM pins include 12, 13, 18, and 19
// raspberry pi pin configuration is necessary before using the pin
const int PWM_PIN = 18;
const int PWM_PIN_U = 19;

void playFrequency(int frequency, int pin) {
    // calculate clock for desired frequency
    int clockDivisor = 192;
    int pwmRange = 19200000 / (clockDivisor * frequency);

    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(clockDivisor);
    pwmSetRange(pwmRange);
    pwmWrite(pin, pwmRange / 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Sustain tone
    pwmWrite(pin, 0); // Stop after delay
}

// LIDAR VARIABLES

// debouncing timer - to prevent many quick bursts of sound
// on many object detection making the piezo click
auto lastDetectionTime = std::chrono::steady_clock::time_point();
const int debounceDurationsMs = 250;
// track last sound time in case object remains in lidar range
auto lastSoundTime = std::chrono::steady_clock::time_point();
const int timeUntilNextSoundMs = 1000; // duration of sound in ms
// track buzzer state and objects
bool isBuzzerActive = false;
bool objectDetectedInCurrentScan = false;
// distance measured in mm
const float minDistance = 1.0f;
const float maxDistance = 2000.0f;

bool ctrl_c_pressed;
void ctrlc(int)
{
    ctrl_c_pressed = true;
}


int main(int argc, const char * argv[]) {
	const char * opt_is_channel = NULL; 
	const char * opt_channel = NULL;
    const char * opt_channel_param_first = NULL;
	sl_u32         opt_channel_param_second = 0;
    sl_u32         baudrateArray[2] = {115200, 256000};
    sl_result     op_result;
	int          opt_channel_type = CHANNEL_TYPE_SERIALPORT;

	bool useArgcBaudrate = false;

    IChannel* _channel;

    // COMMAND LINE ARGUMENTS TO INITIALIZE LIDAR

    printf("Ultra simple LIDAR data grabber for SLAMTEC LIDAR.\n"
           "Version: %s\n", SL_LIDAR_SDK_VERSION);

	 
	if (argc>1)
	{ 
		opt_is_channel = argv[1];
	}
	else
	{
		print_usage(argc, argv);
		return -1;
	}

	if(strcmp(opt_is_channel, "--channel")==0){
		opt_channel = argv[2];
		if(strcmp(opt_channel, "-s")==0||strcmp(opt_channel, "--serial")==0)
		{
			// read serial port from the command line...
			opt_channel_param_first = argv[3];// or set to a fixed value: e.g. "com3"
			// read baud rate from the command line if specified...
			if (argc>4) opt_channel_param_second = strtoul(argv[4], NULL, 10);	
			useArgcBaudrate = true;
		}
		else if(strcmp(opt_channel, "-u")==0||strcmp(opt_channel, "--udp")==0)
		{
			// read ip addr from the command line...
			opt_channel_param_first = argv[3];//or set to a fixed value: e.g. "192.168.11.2"
			if (argc>4) opt_channel_param_second = strtoul(argv[4], NULL, 10);//e.g. "8089"
			opt_channel_type = CHANNEL_TYPE_UDP;
		}
		else
		{
			print_usage(argc, argv);
			return -1;
		}
	}
	else
	{
		print_usage(argc, argv);
        return -1;
	}

	if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
	{
		if (!opt_channel_param_first) {
#ifdef _WIN32
		// use default com port
		opt_channel_param_first = "\\\\.\\com3";
#elif __APPLE__
		opt_channel_param_first = "/dev/tty.SLAB_USBtoUART";
#else
		opt_channel_param_first = "/dev/ttyUSB0";
#endif
		}
	}

    // CREATE DRIVER INSTANCE AND CONNECT TO LIDAR
    // on failure, exit gracefully to on_finished
    
    // create the driver instance
	ILidarDriver * drv = *createLidarDriver();

    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }

    sl_lidar_response_device_info_t devinfo;
    bool connectSuccess = false;

    if(opt_channel_type == CHANNEL_TYPE_SERIALPORT){
        if(useArgcBaudrate){
            _channel = (*createSerialPortChannel(opt_channel_param_first, opt_channel_param_second));
            if (SL_IS_OK((drv)->connect(_channel))) {
                op_result = drv->getDeviceInfo(devinfo);

                if (SL_IS_OK(op_result)) 
                {
	                connectSuccess = true;
                }
                else{
                    delete drv;
					drv = NULL;
                }
            }
        }
        else{
            size_t baudRateArraySize = (sizeof(baudrateArray))/ (sizeof(baudrateArray[0]));
			for(size_t i = 0; i < baudRateArraySize; ++i)
			{
				_channel = (*createSerialPortChannel(opt_channel_param_first, baudrateArray[i]));
                if (SL_IS_OK((drv)->connect(_channel))) {
                    op_result = drv->getDeviceInfo(devinfo);

                    if (SL_IS_OK(op_result)) 
                    {
	                    connectSuccess = true;
                        break;
                    }
                    else{
                        delete drv;
					    drv = NULL;
                    }
                }
			}
        }
    }
    else if(opt_channel_type == CHANNEL_TYPE_UDP){
        _channel = *createUdpChannel(opt_channel_param_first, opt_channel_param_second);
        if (SL_IS_OK((drv)->connect(_channel))) {
            op_result = drv->getDeviceInfo(devinfo);

            if (SL_IS_OK(op_result)) 
            {
	            connectSuccess = true;
            }
            else{
                delete drv;
				drv = NULL;
            }
        }
    }


    if (!connectSuccess) {
        (opt_channel_type == CHANNEL_TYPE_SERIALPORT)?
			(fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
				, opt_channel_param_first)):(fprintf(stderr, "Error, cannot connect to the specified ip addr %s.\n"
				, opt_channel_param_first));
		
        goto on_finished;
    }

    // SUCCESSFULLY CONNECTED TO LIDAR -> START PROGRAM

    // print out the device serial number, firmware and hardware version number..
    printf("SLAMTEC LIDAR S/N: ");
    for (int pos = 0; pos < 16 ;++pos) {
        printf("%02X", devinfo.serialnum[pos]);
    }

    printf("\n"
            "Firmware Ver: %d.%02d\n"
            "Hardware Rev: %d\n"
            , devinfo.firmware_version>>8
            , devinfo.firmware_version & 0xFF
            , (int)devinfo.hardware_version);

    
    // CHECK LIDAR HEALTH AND SETUP PWM PIN

    // check health...
    if (!checkSLAMTECLIDARHealth(drv)) {
        goto on_finished;
    }

    // setup wiringPi and PWM pin
    if (wiringPiSetupGpio() < 0) {
        printf("Failed to initialize GPIO\n");
        goto on_finished;
    }
    pinMode(PWM_PIN, PWM_OUTPUT);
    pinMode(PWM_PIN_U, PWM_OUTPUT);

    // setup listener to exit program on Ctrl-C
    signal(SIGINT, ctrlc);
    
	if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
        drv->setMotorSpeed();
    // start scan...
    drv->startScan(0,1);

    // fetch result and print it out, continues until Ctrl-C
    while (1) {
        sl_lidar_response_measurement_node_hq_t nodes[8192];
        size_t   count = _countof(nodes);

        op_result = drv->grabScanDataHq(nodes, count);

        if (SL_IS_OK(op_result)) {
            drv->ascendScanData(nodes, count);
            objectDetectedInCurrentScan = false; // reset for scan

            for (int pos = 0; pos < (int)count ; ++pos) {
                float angle = nodes[pos].angle_z_q14 * 90.f / (1 << 14); // bit shift is the same as dividing by 16384
                // distance is measured in mm and angle is measured in degrees
                // min/max angle and min/max distance to detect must be specified,
                // otherwise the raspberry pi has trouble keeping up with the data stream
                if ((angle >= 0.0f && angle < 360.0f) && (nodes[pos].dist_mm_q2 / 4.0f > minDistance && nodes[pos].dist_mm_q2 / 4.0f < maxDistance)) {
                    printf("CLOSE! Dist: %08.2f Angle: %02.2f\n", nodes[pos].dist_mm_q2 / 4.0f, angle);
                    objectDetectedInCurrentScan = true;
                    break; // no check further as object already detected
                }
            }

            auto currentTime = std::chrono::steady_clock::now();

            // if object detected for more than debounceDurationsMs, play sound
            if (objectDetectedInCurrentScan) {
                auto elapsedMsSinceDetection = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastDetectionTime).count();
                auto elapsedMsSinceSound = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastSoundTime).count();
        
                // Object has been detected for long enough
                if (elapsedMsSinceDetection >= debounceDurationsMs) {
                    // plays sound if buzzer is not active or if enough time has passed since last sound
                    if (!isBuzzerActive || elapsedMsSinceSound >= timeUntilNextSoundMs) {
                        printf("CLOSE! Object detected for %d ms\n", debounceDurationsMs);
                        playFrequency(2000, 18);
                        playFrequency(40000, 19);
                        lastSoundTime = currentTime;
                        isBuzzerActive = true;
                    }
                } else {
                    // Update detection time if object is still detected
                    lastDetectionTime = currentTime;
                }
            } else {
                // reset buzzer state if no object detected for debounce duration
                isBuzzerActive = false;
            }
        }

        if (ctrl_c_pressed){ 
            break;
        }
    }

    drv->stop();
	lidarDelay(200);
	if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
        drv->setMotorSpeed(0);
    // done!
on_finished:
    if(drv) {
        delete drv;
        drv = NULL;
    }
    pwmWrite(PWM_PIN, 0);
    pwmWrite(PWM_PIN_U, 0);
    return 0;
}