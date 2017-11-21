#include <Bela.h>
#include <cmath>
#include <rtdk.h>
#include "I2C_MPR121.h"

// How many pins there are
#define NUM_TOUCH_PINS 4

// Define this to print data to terminal
#undef DEBUG_MPR121

// Change this to change how often the MPR121 is read (in Hz)
int readInterval = 50;

// Change this threshold to set the minimum amount of touch
int threshold = 40;

// This array holds the continuous sensor values
int sensorValue[NUM_TOUCH_PINS];

// This array holds the trigger status of each button
int gTouchState[NUM_TOUCH_PINS] = { 0 };

// This holds the PREVIOUS trigger status of each button
int gPrevTouchState[NUM_TOUCH_PINS] = { 0 };

// ---- internal stuff -- do not change -----

I2C_MPR121 mpr121;          // Object to handle MPR121 sensing
AuxiliaryTask i2cTask;      // Auxiliary task to read I2C

int readCount = 0;          // How long until we read again...
int readIntervalSamples = 0; // How many samples between reads

void readMPR121();

bool setup(BelaContext *context, void *userData)
{
	for (int i = 0; i < 8; i++) {
		pinMode(context, 0, i, OUTPUT);
	}
	
	if(!mpr121.begin(1, 0x5A)) {
        rt_printf("Error initialising MPR121\n");
        return false;
    }
    
    i2cTask = Bela_createAuxiliaryTask(readMPR121, 50, "bela-mpr121");
    readIntervalSamples = context->audioSampleRate / readInterval;
    
	
	rt_printf("pins ready, starting ...\n");
	return true;
}

void render(BelaContext *context, void *userData)
{
	for(int n = 0; n < context->audioFrames; n++) {
        // Keep this code: it schedules the touch sensor readings
        if(++readCount >= readIntervalSamples) {
            readCount = 0;
            Bela_scheduleAuxiliaryTask(i2cTask);
        }
        
       	for (int i = 0; i < 4; i++) {
       		digitalWrite(context, n, i, gTouchState[i]);
       	}
        
	}

}

void cleanup(BelaContext *context, void *userData)
{

}

// Auxiliary task to read the I2C board
void readMPR121()
{
    for(int i = 0; i < NUM_TOUCH_PINS; i++) {
        sensorValue[i] = -(mpr121.filteredData(i) - mpr121.baselineData(i));
        sensorValue[i] -= threshold;
        if(sensorValue[i] < 0)
            sensorValue[i] = 0;
    }
// #ifdef DEBUG_MPR121
//         rt_printf("%d ", sensorValue[i]);
// #endif
//     }
// #ifdef DEBUG_MPR121
//     rt_printf("\n");
// #endif
    
    
    //You can use this to read binary on/off touch state more easily
    // rt_printf("Touched: %x\n", mpr121.touched());
    
    // Reading for multi-touch. This allows more than one switch to be active at once.
    // Step 1: Save previous states
    // Step 2: Loop through all electrodes and do a comparison
    // Step 3: If there
    
	// 1. Save previous states:
    for (int i = 0; i < 4; i++) {
    	gPrevTouchState[i] = gTouchState[i];
    }
    
    // 2. Loop through the electrodes and bitwise compare:
    for (int i = 0; i < 4; i++) {
		if (mpr121.touched() & (1 << i))  {
		    gTouchState[i] = 1;
		    // rt_printf("0 is pressed\n");
		} else {
			gTouchState[i] = 0;
		}
    }

	// 3. Do something based on the previous state and current state.
   	for (int i = 0; i < 4; i++) {
   		if (!gPrevTouchState[i] && gTouchState[i]) {
   			rt_printf("Electrode %d is touched!\n", i);
   		}
   		if (gPrevTouchState[i] && !gTouchState[i]) {
   			rt_printf("Electrode %d has been released!\n", i);
   		}
   	}
}