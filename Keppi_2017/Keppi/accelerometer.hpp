/***** accelerometer.hpp *****/

// Function declarations:
void readAccelerometer(BelaContext *context, int currentFrame);
void setLights(BelaContext *context, int currentFrame, int lightState);
void lightState(BelaContext *context, int frame, float peakValue);

extern int ledPins[];

extern int gIsAudioMuted;

// Values for the filters. These are the past inputs:
double x_xValues[2] = { 0 };
double y_xValues[2] = { 0 };
double z_xValues[2] = { 0 };
// ... and these are the past outputs:
double x_yValues[2] = { 0 };
double y_yValues[2] = { 0 };
double z_yValues[2] = { 0 };

double scopeX, scopeY, scopeZ;

// Save the past values for the DC blocking filter:
double x_DC_in, x_DC_out, y_DC_in, y_DC_out, z_DC_in, z_DC_out = 0;

int gWritePointer = 0;

double gRolloff = 0.000029;
// double gRolloff = 0.1;

double gTotalMotion;
double gPeak = 2;

float thresh = 0.0000001; // adjust this


int gLightState;
int gPrevLightState = 0;

int gTickDown;

int gMostPeak = 0;

int gMuteTimeout = 0;

// External values for the coefficients (these are declared in coeffs.hpp):
extern double gLowA1, gLowA2, gHighA1, gHighA2, gLowB0, gLowB1, gLowB2, gHighB0, gHighB1, gHighB2;
extern double gW0_low;
extern double gW0_high;

int gInTimeout = 0;

void readAccelerometer(BelaContext *context, int frame) {
	
	
    double xIn = analogRead(context, frame/2, 4);
    double yIn = analogRead(context, frame/2, 5);
    double zIn = analogRead(context, frame/2, 6);
    
    // LPF!
    // y[n] = (B0 * x[n] + B1 * x[n-1] + B2 x[n-2] - A1 * x[n-1] - A2 * x[n-2])
    double x_LP = (gLowB0 * xIn) + (gLowB1 * x_xValues[(gWritePointer + 2 - 1) % 2]) + (gLowB2 * x_xValues[gWritePointer]) - (gLowA1 * x_yValues[(gWritePointer + 2 - 1) % 2]) - gLowA2 * x_yValues[gWritePointer];
    double y_LP = (gLowB0 * yIn) + (gLowB1 * y_xValues[(gWritePointer + 2 - 1) % 2]) + (gLowB2 * y_xValues[gWritePointer]) - (gLowA1 * y_yValues[(gWritePointer + 2 - 1) % 2]) - gLowA2 * y_yValues[gWritePointer];
    double z_LP = (gLowB0 * zIn) + (gLowB1 * z_xValues[(gWritePointer + 2 - 1) % 2]) + (gLowB2 * z_xValues[gWritePointer]) - (gLowA1 * z_yValues[(gWritePointer + 2 - 1) % 2]) - gLowA2 * z_yValues[gWritePointer];
    

    // Save the raw X values for next time:
    x_xValues[gWritePointer] = xIn;
    y_xValues[gWritePointer] = yIn;
    z_xValues[gWritePointer] = zIn;
    

    
    // Also, save the filtered Y values for next time:
    x_yValues[gWritePointer] = x_LP;
    y_yValues[gWritePointer] = y_LP;
    z_yValues[gWritePointer] = z_LP;

    // Perform DC filtering!
    double x_DC = (x_LP - x_DC_in + 0.9998 * x_DC_out);
    double y_DC = (y_LP - y_DC_in + 0.9998 * y_DC_out);
    double z_DC = (z_LP - z_DC_in + 0.9998 * z_DC_out);
    
    double x_motion = (x_DC - x_DC_out) * (x_DC - x_DC_out);
    
    double y_motion = (y_DC - y_DC_out) * (y_DC - y_DC_out);
    
    double z_motion = (z_DC - z_DC_out) * (z_DC - z_DC_out);
    
	gTotalMotion = sqrt( x_motion + y_motion + z_motion );
	// gTotalMotion = x_motion + y_motion + z_motion; // squared euclidean distance
	gTotalMotion -= thresh;
	if (gTotalMotion < 0) {
		gTotalMotion = 0;
	}
	
    
    if (gMuteTimeout == 0) {
	    gPeak += gTotalMotion;
	    if (gPeak > 2) {
	    	gPeak = 2;
	    }
	    gPeak -= gRolloff;
	    if (gPeak < 0) {
	    	gPeak = 0;
	    }
	    lightState(context, frame, gPeak);
    }
    
	
	//setLights(context, frame, gLightState);
    
    // Save the X and Y values for the DC filter (we only need the last one):
    x_DC_in = x_LP;
    x_DC_out = x_DC;
    
    y_DC_in = y_LP;
    y_DC_out = y_DC;
    
    z_DC_in = z_LP;
    z_DC_out = z_DC;
    
    // Advance the write pointer: 
    gWritePointer++;
    if (gWritePointer > 1) {
    	gWritePointer = 0;
    }

}


void lightState(BelaContext *context, int frame, float peakValue) {
	// Store the light state from last time we checked:
    gPrevLightState = gLightState;
    
    // Get the light state from right now:
	if (peakValue > 1.5) {
		gLightState = 4;
	} else if (peakValue <= 1.5 && peakValue > 1) {
		gLightState = 3;
	} else if (peakValue <= 1 && peakValue > 0.6) {
		gLightState = 2;
	} else if (peakValue <= 0.6 && peakValue > 0.3) {
		gLightState = 1;
	} else if (peakValue <= 0.3 && peakValue > 0.07) {
		gLightState = 0;
	} else if (peakValue <= 0.07) {
		gLightState = -1; // -1 is the silent state
		
	}
	
	if (gLightState == -1) {
		gIsAudioMuted = 1;
	} else {
		gIsAudioMuted = 0;
	}
	 
		
	// When count down is done, move to 0 and charge up again
	if (gLightState != gPrevLightState) {
		rt_printf("light state moved to %d\n", gLightState);
		// Blink the lights accordingly:
		setLights(context, frame, gLightState);
	}
}


void setLights(BelaContext *context, int currentFrame, int lightState) {
	if (gLightState >= 0) {
		for (int i = lightState; i >= 0; i--) {
			digitalWrite(context, currentFrame, ledPins[i], HIGH);
		}
		for (int j = lightState + 1; j <= 5; j++) {
			digitalWrite(context, currentFrame, ledPins[j], LOW);
		}
	} else {
		for (int j = 0; j < 5; j++) {
			digitalWrite(context, currentFrame, ledPins[j], LOW);
		}
	}
	
	
	
	
}