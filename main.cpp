//program needs to run on win32 platform

#include <iostream>
using namespace std;

//Accessing existing code for accessing computer audio hardware. Credit to Javidx9 on Github
#include "soundBox.h"

//double that isnt hindered by multiple threads
atomic<double> frequencyOut = 0.0;

double MakeSound(double duration) {
    //square wave function implementation of sin wave
    double sqOut = 1.0 * sin(frequencyOut * 2 * 3.14159 * duration);
    if (sqOut > 0.0)
        return 0.2;
    else
        return -0.2;

}

int main() {
    //Fetch all audio hardware devices
    vector<wstring> devices = soundBox<short>::Enumerate();

    //Read all fetched displays 
    for (auto device : devices) wcout << "Identified Display: " << device << endl;

    //soundBox uses short to get 16bits. This gives us a minimum accurate representation of a sin wave using bits.
    //Sample rate must be double the maximum frequency output, which is a maximum of 20khz for human hearing. 
    //1 channel, as it is one speaker
    //512 mystery number for latency management
    soundBox<short> sound(devices[0], 44100, 1, 8, 512);

    //MakeSound function for sound instance of soundBox
    sound.SetUserFunction(MakeSound);

    //12 notes per octave. 
    double octaveBaseF = 110.0; //440hz. When you double your frequency, you move up on octave
    double twoRoot12 = pow(2.0, 1.0 / 12.0);
     
    while (1) {
        //keyboard initialization. 0x8000 highest bit.

        //piano keyboard implementation
        bool keyPress = false;

        for (int i = 0; i < 15; i++) {
            //Mapping piano keys onto computer keys
            if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe"[i])) & 0x8000) {
                frequencyOut = octaveBaseF * pow(twoRoot12, i);
                keyPress = true;
            }
        }

        if (!keyPress) {
            frequencyOut = 0.0;
        }
    }

    return 0;
}
