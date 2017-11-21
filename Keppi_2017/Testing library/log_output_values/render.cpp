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
#include <WriteFile.h>

WriteFile piezoValues;

bool setup(BelaContext *context, void *userData)
{
	piezoValues.init("piezo0.txt"); //set the file name to write to
	piezoValues.setFileType(kBinary);
	
	
	return true; 
}

void render(BelaContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->analogFrames; ++n) {
		
		piezoValues.log(); // log an array of values
		
	}
}

void cleanup(BelaContext *context, void *userData)
{
}


/**
\example logging-sensors/render.cpp

Logging Sensor Data
---------------------------

This sketch demonstrates how to log sensor data for later processing or analysis.
The file can be written as either a binary file (using 32-bit floats) or
as a formatted text file (again, only using floats).
An optional header and footer can be added to text files, to make your file 
immediately ready to be opened in your favourite data analysis program.

Make sure you do not fill up your disk while logging.
Current disk usage can be obtained by typing at the console:

    $ df -h .

While the space occupied by a given file can be obtained with

    $ ls -lh path/to/filename

To learn how to obtain more space on your filesystem, check out the [wiki](https://github.com/BelaPlatform/Bela/wiki/Manage-your-SD-card#resizing-filesystems-on-an-existing-sd-card).


If you write text files, it is a good idea keep your lines short,
as this usually makes it faster for other programs to process the file.
The text file in this example breaks a line after each sample is logged (note the
`\n` at the end of the format string).

If you plan to write large amount of data, binary files are preferrable as they
take less space on disk and are therefore less resource-consuming to write.
They are also generally faster to read and parse than text files.

Binary files can be opened in GNU Octave or Matlab, with:

    fid=fopen([filename],'r');
    A = fread(fid, 'float');
    fclose(fid);
    A = vec2mat(A, numChannels);

It is safe to call WriteFile::log() from the audio thread, as the data is added 
to a buffer which is then processed in the background and written to disk from 
a lower-priority non-realtime task.

*/

