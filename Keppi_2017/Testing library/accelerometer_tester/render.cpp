#include <Bela.h>
#include <Scope.h>

Scope scope;

void readAccelerometer(BelaContext *context, int currentFrame);
double getMaxValue();
void lights(BelaContext *context, int currentFrame, int peak);
void assign_state(int peak);

double inputValues[3] = { 0 };
double outputValues[3] = { 0 };
double accelDC[3] = { 0 };

float gCurrentPeak = 0;
float gThresh = 0.1;
float gRolloff = 0.99995;
int gSampleCount = 0;

int gPeak;
int gPrevPeak;

int gLightState;
int gPrevLightState;

int gTickDown;

int gMostPeak = 0;

bool setup(BelaContext *context, void *userData)
{
	scope.setup(4, context->audioSampleRate);
	// Set up LED pins:
	for (int i = 0; i < 6; i++) { // pins 0-5
		pinMode(context, 0, i, OUTPUT);
	}
	return true;
}

void render(BelaContext *context, void *userData)
{
	for (int n = 0; n < context->audioFrames; n++) {
		if (!(n % 2))  {
			readAccelerometer(context, n);
		}
		
		gSampleCount++;

		
		lights(context, n, gLightState);
		scope.log(accelDC[0], accelDC[1], accelDC[2], gCurrentPeak);
	}

}

void cleanup(BelaContext *context, void *userData)
{
	rt_printf("the most peak: %d", gMostPeak);
}

void readAccelerometer(BelaContext *context, int currentFrame) {
	for (int i = 0; i < 3; i++) {
				float accelInput = analogRead(context, currentFrame/2, i);
				
				accelDC[i] = accelInput - inputValues[i] + 0.9998 * outputValues[i];
				inputValues[i] = accelInput;
				outputValues[i] = accelDC[i];
				if (accelDC[i] < 0) {
					accelDC[i] *= -1;
				}
		
		
	}
	double maxAccelReading = getMaxValue(); // From the three current readings, find the biggest.

	if (maxAccelReading > gCurrentPeak) { // If it's bigger than our last peak ...
		gCurrentPeak = maxAccelReading;
		gPeak = int(gCurrentPeak * 1000);
		// rt_printf("new peak! It's %d\n", gPeak)
		assign_state(gPeak);
	} else {
		
		gCurrentPeak *= gRolloff;
		gTickDown++;
		if (gTickDown >= 22050 && gLightState >= 0) {
			gTickDown = 0;
			gLightState -= 1;
			// rt_printf("current state: %d\n", gLightState);
		} else if (gLightState == -1) {
			gTickDown = 0;
		}
	}
	if (gSampleCount >= 22050) {
		rt_printf("current max reading: %d\n", gPeak);
		gSampleCount = 0;
	}
			
}

double getMaxValue() {
	double maxVal = 0;
	for (int i = 0; i < 3; i++) {
		if (accelDC[i] > maxVal) {
			maxVal = accelDC[i];
		}
	}
	return maxVal;
}

void assign_state(int peak) {
	gPrevLightState = gLightState;
	if ((peak > 10) && (peak <= 90)) {
		gLightState = 0;
	} else if ((peak > 90) && (peak <= 180)) {
		gLightState = 1;
	} else if ((peak > 180) && (peak <= 270)) {
		gLightState = 2;
	} else if ((peak > 360) && (peak <= 450)) {
		gLightState = 3;
	} else if ((peak > 450) && (peak <= 540)) {
		gLightState = 4;
	} else if (peak > 540) {
		gLightState = 5;
	} else {
		gLightState = -1;
	}
	if (gPrevLightState != gLightState) {
		rt_printf("current state: %d\n", gLightState);
	}
}

void lights(BelaContext *context, int currentFrame, int gLightState) {

	int lightsOff;
	if (gLightState > -1) {
		lightsOff = gLightState + 1; // turn lights off that are on pins ABOVE the state.
		// If we're in state 2, light up 0, 1, 2 and turn off 3, 4, 5
			for (int i = lightsOff; i < 6; i++) {
		digitalWrite(context, currentFrame, i, LOW);
	}
		// Turn on the lights that indicate state:
		for (int i = 0; i <= gLightState; i++) {
			digitalWrite(context, currentFrame, i, HIGH);
		}
	} else {
		lightsOff = 5;
	}
	

	

}