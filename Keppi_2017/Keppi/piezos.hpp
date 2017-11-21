/***** readPiezos.hpp *****/

void readPiezos(BelaContext *context, int frame);


// DC blocking variables:
float gPiezoX[NUM_TOUCH_PINS] = { 0 };
float gPiezoY[NUM_TOUCH_PINS] = { 0 };

// Piezo handling variables:
float gPiezoInput[NUM_TOUCH_PINS] = { 0 };
float gCurrentSampleDCBlocked[NUM_TOUCH_PINS] = { 0 };



void readPiezos(BelaContext *context, int frame) {
	for (int i = 0; i < NUM_TOUCH_PINS; i++) {
		gPiezoInput[i] = analogRead(context, frame/2, i);
			    
		// DC Offset Filter    y[n] = x[n] - x[n-1] + R * y[n-1]
		gCurrentSampleDCBlocked[i] = gPiezoInput[i] - gPiezoX[i] + (0.995 * gPiezoY[i]);
		gPiezoX[i] = gPiezoInput[i];
		gPiezoY[i] = gCurrentSampleDCBlocked[i];
			
		// Full wave rectify
		if(gCurrentSampleDCBlocked[i] < 0.0) {
			gCurrentSampleDCBlocked[i] *= -1.0;
		}
	}
}