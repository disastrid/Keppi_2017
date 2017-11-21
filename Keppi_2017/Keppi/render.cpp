/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
  Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
  Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/


#include <Bela.h>
#include <cmath>
#include <SampleLoader.h>
#include <SampleData.h>
#include <algorithm>
#include <iterator>
#include <vector>
#include <Scope.h>
#include <deque>
#include <array>
#include <WriteFile.h>
#include "defs.hpp"		// Definitions that all member files need
#include "I2C_MPR121.h"	// Library for cap touch
#include "accelerometer.hpp" // Code that takes in and filters accelerometer data and sets lights
#include "coeffs.hpp"	// Code that calculates filter coefficients
#include "piezos.hpp"	// Code to handle and filter piezo data§
#include "play.hpp" 	// code to play samples

using namespace std;

Scope scope;

// To log piezo values, uncomment this:
// WriteFile piezoValues;


int gSampleCount = 0; // Master sample counter, ticks up every time audio updates

// --------------------------------
// Declare external functions:

void calculateCoeffs();
// --------------------------------

// --------------------------------
// STARTUP ROUTINE
// At startup, the lights should flash on and off three times.
// Then, on 1 sec button press, the lights should flash twice and the instrument becomes active.
// To turn off, hold the button for 2 seconds.

// 

int gIsAudioMuted = 0;

/* ========
	LIGHTS
   ========
These are the variables for controlling the lights.
*/

extern int gLightState;
int ledPins[5] = {0, 2, 4, 6, 7};

int gGlobalLightState = 0;
int gLightsOn = 0;

/* ========
	STARTUP
   ========
These are the variables for controlling the startup routine.
Lights flash three times, and system is ready and online.
*/

int gSystemState = 0;
int gLightFlashCount = 0;


/* ========
	SAMPLE VARIABLES
   ========
These are the variables for sample handling.
*/
vector<string> gFilenames = {"clay2.wav", "clay1.wav", "clay3.wav", "clay4.wav"};
int gEndFrame;
int gStartFrame = 0;
SampleData gSampleData[NUM_SAMPLES];



/* ========
	CAPACATIVE TOUCH
   ========
*/
#undef DEBUG_MPR121 		// Define this to print data to terminal
int readInterval = 200;		// Change this to change how often the MPR121 is read (in Hz)
int threshold = 20;			// Change this threshold to set the minimum amount of touch
int sensorValue[NUM_TOUCH_PINS];// This array holds the continuous sensor values
I2C_MPR121 mpr121;			// Object to handle MPR121 sensing
AuxiliaryTask i2cTask;		// Auxiliary task to read I2C
int readCount = 0;			// How long until we read again...
int readIntervalSamples = 0; // How many samples between reads
void readMPR121();

int gPrevTouchState[NUM_TOUCH_PINS] = { 0 };
int gTouchState[NUM_TOUCH_PINS] = { 0 };
int gSensorPlayState[NUM_TOUCH_PINS] = { 0 }; // keeps track of which is playing

// Cap touch state machine:
// States: 
// 0: Waiting for touch.
// 1: Touched - triggers piezo buffering and return of variable, then goes back to 0.
int gSensorState[NUM_TOUCH_PINS] = { 0 };
int gSensorDebounce[NUM_TOUCH_PINS];

/* ========
	VOICE STEALING
   ========
These variables keep track of:
- All read pointers, and where they are in their buffers 
- If they're active (1 for active, 0 for waiting)
- What samples they are playing (aka buffer ID - a number from 0-3)
- The velocity the sample is being played at (returned by piezos)
- How old the pointer is (so we can steal the oldest)
*/

int gReadPointers[NUM_VOICES] = { 0 };
int gPointerState[NUM_VOICES] = { 0 };
int gReadPointerBufferID[NUM_VOICES] = { 0 };
float gSampleVelocity[NUM_VOICES] = { 0 };
int gReadPointerAge[NUM_VOICES] = { 0 };


/* ========
	PIEZOS
   ========
	Here we have:
	- Piezo state (are we just buffering, or are we looking for a peak)?
	- An array of 4 deques used to buffer samples 
	- The peak value returned over buffered samples
	- How many samples we have in our deque (should be 100, but we count in case it's less)
	- The current filtered and cleaned piezo sample (gCurrentSampleDCBlocked)
	- Scalers to correct the velocity value, in case piezos are too sensitive/not sensitive enough
*/

int gPiezoState[NUM_TOUCH_PINS] = { 0 }; //0: wait and buffer; 1: look for peak

std::array< std::deque<float>, NUM_TOUCH_PINS > gPiezoBufferDeques; // make 4 deques for buffering.

float gPiezoPeak[NUM_TOUCH_PINS] = { 0 }; // This is the MAX VALUE in our deque of 300 collected samples.
int gNumSamplesInDeque[NUM_TOUCH_PINS] = { 0 }; // We count the number of buffered samples in the back buffer - in case we happento gather less than 100.
extern float gCurrentSampleDCBlocked[NUM_TOUCH_PINS];
float gScalerValues[4] = {4.0, 6.0, 6.0, 4.0};


bool setup(BelaContext *context, void *userData)
{
	// Uncomment these to log piezo values.
	// piezoValues.init("piezo0.txt"); //set the file name to write to
	// piezoValues.setFileType(kText);
	// piezoValues.setFormat("%.4f\n");
	
	// Set LED pin modes
	for (int i = 0; i < 5; i++) {
		pinMode(context, 0, ledPins[i], OUTPUT);
	}
	
    scope.setup(4, context->audioSampleRate);
    
    // Init I2C stuff:
	i2cTask = Bela_createAuxiliaryTask(readMPR121, 50, "bela-mpr121");	
	readIntervalSamples = context->audioSampleRate / readInterval;
	if(!mpr121.begin(1, 0x5A)) {
		rt_printf("Error initialising MPR121\n");
		return false;
	}
    
	// Get the sample data:
    for (int i = 0; i < NUM_SAMPLES; i++) {
    	gSampleData[i].sampleLen = getNumFrames(gFilenames.at(i));
    	gEndFrame = gSampleData[i].sampleLen;
	    
	    // gSampleData[i][ch].sampleLen = getNumFrames(gFilenames.at(i));
	    gSampleData[i].samples = new float[gSampleData[i].sampleLen];
	    getSamples(gFilenames.at(i),gSampleData[i].samples,0,gStartFrame,gEndFrame);
    }
    
	// Get filter values:
	calculateCoeffs();
	
	return true;
	
}

void render(BelaContext *context, void *userData)
{

    for(unsigned int n = 0; n < context->audioFrames; n++) {
    	// First, count the samples.
    	gSampleCount = context->audioFramesElapsed;
    
		// Schedule the cap touch via MPR121:
    	if(++readCount >= readIntervalSamples) {
			readCount = 0;
			Bela_scheduleAuxiliaryTask(i2cTask);
		}
		
		// If we're on even numbered samples, read the accel and piezos.
		if(!(n % 2)) {
			readAccelerometer(context, n);
			readPiezos(context, n);
		}
		
		// TO DO:
		// Check all the sensors. Are any touched? If so, change the piezo state.
		// Check the piezo state. Either buffer away or return a value.
		// Write any audio that's needed.
		
		// CHECK SENSORS:
       	for (int s = 0; s < NUM_TOUCH_PINS; s++) { 
       		
	       	if (gSensorState[s] == 0) { // If we're not touched ..
	       		gPiezoState[s] = 0; // Keep the piezo buffering.
	       	
	       	} else if (gSensorState[s] == 1) { // If we're touched ...
	       		gNumSamplesInDeque[s] = gPiezoBufferDeques[s].size(); // Check how many samples we have in the buffer
	       		gPiezoState[s] = 1; // Move to piezo collecting state
	       		// gSensorDebounce[s] = 0; // Get ready to count debounce samples
	       		gSensorState[s] = 2; // Move to debounce state
	       		
	       	} else if (gSensorState[s] == 2) { // Debounce - delay a little before moving back to waiting.
	       		gSensorDebounce[s]++;
	       		if (gSensorDebounce[s] >= 5000) {
	       			gSensorState[s] = 0;
	       			gSensorDebounce[s] = 0;
	       			// gPiezoState[s] = 0;
	       		}
	       	}
       		
       		// PIEZOS STATES
       		// 0: Buffer away
       		// 1: Triggered - find highest value and return
	       	if (gPiezoState[s] == 0) {
				// If we're just buffering, store the value in our deque.
				gPiezoBufferDeques[s].push_back(gCurrentSampleDCBlocked[s]);
				//  If the deque has more than 100 items in it, pop the value off the front - it's too old, we don't need it.
				if (gPiezoBufferDeques[s].size() >= NUM_PIEZO_VALUES_BACK) {
					gPiezoBufferDeques[s].pop_front();
				}

			} else if (gPiezoState[s] == 1) {
				gPiezoBufferDeques[s].push_back(gCurrentSampleDCBlocked[s]);
				
				// If we have collected enough forward samples:
				if (gPiezoBufferDeques[s].size() >= NUM_PIEZO_VALUES_FRONT + gNumSamplesInDeque[s]) {
					// Save the peak value:
					gPiezoPeak[s] = *max_element(gPiezoBufferDeques[s].begin(), gPiezoBufferDeques[s].end());
					// Map the peak to pass it to the play function:
					gPiezoPeak[s] *= gScalerValues[s];
					float sampleVelocity = map(gPiezoPeak[s], 0.001, 0.2, 0.01, 1.6);
					if (sampleVelocity < 0.01) {
						sampleVelocity = 0.01;
					} else if (sampleVelocity > 1.6) {
						sampleVelocity = 1.6;
					}
					// Scale it if you need to:
					// sampleVelocity *= gScalerValues[s];
					// Pass value to play function. 
					
					// Evaluate if our play function returns 0. If it doesn't, it's started to play.
					// If it does, we just freed up a voice so we can run it again.
                    if (!gIsAudioMuted) {
                    	if (startPlayingSample(s, sampleVelocity) == 0) { 
                        	startPlayingSample(s, sampleVelocity); 
                        }
                    }
					// Clear the deque so we're ready to start buffering again:
					gPiezoBufferDeques[s].clear();
				
					rt_printf("The highest piezo value was %f!\n", gPiezoPeak[s]);
					rt_printf("the velocity was %f\n", sampleVelocity);
					gPiezoState[s] = 0;
					}
				}
	       	} // Finished checking all the sensors for their states.
       	
       
		// Start an output variable.
	    float out = 0;
		float sampleOutputs[NUM_TOUCH_PINS] = { 0 };
	 	float sampleScalers[NUM_TOUCH_PINS] = { 0 };
	 	float numberOfSamples[NUM_TOUCH_PINS] = { 1, 1, 1, 1 }; 
	 	// Count up number of samples on each sensor
	 	for (int j = 0; j < NUM_VOICES; j++) {
	 		if (gPointerState[j] == 1) {
	 			switch (gReadPointerBufferID[j]) {
	 				case 0: {
	 					numberOfSamples[0] += 1;
	 					sampleOutputs[0] += gSampleData[gReadPointerBufferID[j]].samples[gReadPointers[j]] * gSampleVelocity[j];
	 					break;
	 				}
	 				case 1: {
	 					numberOfSamples[1] += 1;
	 					sampleOutputs[1] += gSampleData[gReadPointerBufferID[j]].samples[gReadPointers[j]] * gSampleVelocity[j];
						break;
	 				}
	 				case 2: {
	 					numberOfSamples[2] += 1;
	 					sampleOutputs[2] += gSampleData[gReadPointerBufferID[j]].samples[gReadPointers[j]] * gSampleVelocity[j];
	 					break;
	 				}
	 				case 3: {
	 					numberOfSamples[3] += 1;
	 					sampleOutputs[3] += gSampleData[gReadPointerBufferID[j]].samples[gReadPointers[j]] * gSampleVelocity[j];
	 					break;
	 				}
	 				default: {
	 					break;
	 				}
	 			} // end switch
	 		} // end if
	 	} // end for
	 	// int numSensorsActive = 1;
	 	
	 	// calculate each sample's scaler
	 	// for (int i = 0; i < NUM_TOUCH_PINS; i++) {
	 	// 	if (numberOfSamples[i] != 1) {
	 	// 		numSensorsActive++;
	 	// 	}
	 	// }
	 	
	 	
	 	for (int i = 0; i < NUM_TOUCH_PINS; i++) {
	 		// sampleScalers[i] = 1/numberOfSamples[i];
	 		// sampleOutputs[i] *= sampleScalers[i];
	 		out += sampleOutputs[i] * 1.2;
	 	}
	 	
	 	// float masterScaler = 1 / float(numSensorsActive);
	 	// out *= masterScaler;
	    
	    // for (int i = 0; i < NUM_VOICES; i++) {
	    // 	if(gReadPointers[i] >= 0 && gPointerState[i] == 1) {
	    // 		float outputSample = gSampleData[gReadPointerBufferID[i]].samples[gReadPointers[i]] * gSampleVelocity[i];
	    // 		out += outputSample;
	    // 		// out *= outputScaler;
	    // 	}
	    // }

	   // Crackle of 0 has no effect. Crackle of 1 is silent. Crackle of 0.2 is crackle.
	    float crackle = 0;
	    if (gLightState == 0) { 
	    	crackle = 0.2;				// In state 0 we start buzzing
	    } else if (gLightState == -1) { 
	    	crackle = 1;				// In state -1 we go silent
	    } else {
	    	crackle = 0; 				// Otherwise, don't bother with crackle
	    }
	    
	    if (out > 0) {
			out -= crackle;
		    if (out < 0) {
		    	out = 0;
			}	    	
	    } else {
	    	out += crackle;
	    	if (out > 0) {
	    		out = 0;
			}
		}
	    	
	    // Are we in the inactive light state? If so, the output is silent.

	    // Write audio to both channels:
	    for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
	    }
	    
	    
	    // ADVANCE READ POINTERS. If they're at the end of the sample, make them inactive and reset.
		for(int j=0; j < NUM_VOICES; j++) {
		
			if (gPointerState[j] == 1 && gReadPointers[j] < gSampleData[gReadPointerBufferID[j]].sampleLen) {
    			gReadPointers[j]++;
			} else {
    			gPointerState[j] = 0;
    			gReadPointers[j] = 0;
			}
		}
	    
	    // Writing the piezo data for Jack:
	    
	    
	    // audioWrite(context, n, 0, gCurrentSampleDCBlocked[3]);
	    // audioWrite(context, n, 1, gCurrentSampleDCBlocked[3]);
	    
	    //scope.log();
    	
    }// end audio loop
} // end render


void readMPR121()
{
#ifdef DEBUG_MPR121
	static int printCounter = 20;
#endif
	
	for(int i = 0; i < NUM_SAMPLES; i++) {
		sensorValue[i] = -(mpr121.filteredData(i) - mpr121.baselineData(i));
		sensorValue[i] -= threshold;
		if(sensorValue[i] < 0)
			sensorValue[i] = 0;
		// Turn on sensor 
		// if(sensorValue[i] > 0)
		// 	gSensorsTouched[i] = 120; // This is the number of samples to tick down to time out
#ifdef DEBUG_MPR121
		if(i == 0)
			printCounter--;
		if(printCounter == 0)
			rt_printf("%d/%d ", mpr121.filteredData(i), mpr121.baselineData(i));
#endif
	}
#ifdef DEBUG_MPR121
	if(printCounter == 0) {
		rt_printf("\n");
		printCounter = 20;
	}
#endif
	
	// You can use this to read binary on/off touch state more easily
	//rt_printf("Touched: %x\n", mpr121.touched());
	
	// Do multitouch.
	// 1. Save the current touch state as previous touch state.
	// 2. Then, determine if the sensor is currently touched.
    for (int i = 0; i < NUM_SAMPLES; i++) {
    	gPrevTouchState[i] = gTouchState[i];
		if (mpr121.touched() & (1 << i))  {
			//if(gTouchState[i] == 0)
			//	rt_printf("%d is pressed\n", i);
		    gTouchState[i] = 1;
		} else {
			gTouchState[i] = 0;
		}
    	if (!gPrevTouchState[i] && gTouchState[i]) {
   			rt_printf("Electrode %d is triggered!\n", i);
   			
   			gSensorState[i] = 1; // Set state to triggering 
   		}
   		if (gPrevTouchState[i] && !gTouchState[i]) {
   			//rt_printf("Electrode %d has been released!\n", i);
   			
   		}
   	}
}

// void checkButton(BelaContext *context, int frame, int buttonPin) {
// 	buttonCounter++;
	
// 	if (!(buttonCounter % 100)) {
// 		int status = digitalRead(context, frame, buttonPin);
// 		if ((prevButtonState == 0 && status == 1) || (prevButtonState == 1 && status == 1)) {
// 			counter++;
// 		} else if (prevButtonState == 1 && status == 0) {
// 			counter = 0;
// 		}
// 		if (counter >= 441) {
// 			rt_printf("Held for 1 second!\n");
// 			counter = 0;
// 		}
// 		prevButtonState = status;
		
// 	}
// }


void cleanup(BelaContext *context, void *userData)
{
	for (int i = 0; i < NUM_SAMPLES; i++) {
	    
	    	delete[] gSampleData[i].samples;
	 
	}
}


/**
\example sample-loader/render.cpp

Basic Sample Loader
--------------------------------

This example loads a specified range of samples from a file into a buffer using
helper functions provided in SampleLoader.h. This should ideally be used when working
with small wav files. See sampleStreamer and sampleStreamerMulti for more elaborate ways
of loading and playing back larger files.

*/
