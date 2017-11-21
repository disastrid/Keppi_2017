#include <Bela.h>

bool setup(BelaContext *context, void *userData)
{
	pinMode(context, 0, P8_07, OUTPUT);
	return true;
}

void render(BelaContext *context, void *userData)
{
	for (int n = 0; n < context->digitalFrames; n++) {
		digitalWrite(context, n, 0, HIGH);
	}

}

void cleanup(BelaContext *context, void *userData)
{

}