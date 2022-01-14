//program needs to run on win32 platform

#include <iostream>
using namespace std;

//Accessing existing code for accessing computer audio hardware. Credit to Javidx9 on Github
#include "soundBox.h"



double fConvert(double hertz) {
    return hertz * 2.0 * PI;
}

double oscillator(double hertz, double time, int type, double LFOFreq = 0.0, double LFOAmp = 0.0) {
    double freq = fConvert(hertz) * time + LFOAmp * hertz * sin(fConvert(LFOFreq) * time);
    
    switch (type) {
    case 0: //sin wave
        return sin(freq);
    case 1: //square wave
        return sin(freq) > 0.0 ? 1.0 : -1.0;
    //case 2: //triangular wave
    //   return asin(sin(freq)) * 2.0 / PI);
    case 2: //saw wave (warm)
    {
        double out = 0.0;

        for(double i = 1.0; i < 100.0; i++)
            out += (sin(i * freq)) / i;
        
        return out * (2.0 / PI);
    }
    case 3: //optimized saw wave (harsh)
        return (hertz * PI * fmod(time, 1.0 / hertz) - (PI / 2.0)) * (2.0 / PI);
    
    case 4: //pseudo random
        return 2.0 * ((double)(rand()) / (double)(RAND_MAX)) - 1.0;
    
    default: 
        return 0.0;
    }
}

struct envelopeADSR {
    double attackTime;
    double decayTime;
    double releaseTime;
    double sustainedAmp;
    double startAmp;

    double triggerOnTime;
    double triggerOffTime;

    bool noteOn;

    //Attack, Decay, Sustained, Release
    envelopeADSR() {
        attackTime = 0.01;
        decayTime = 1.0;
        releaseTime = 1.0;
        sustainedAmp = 0.0;
        startAmp = 0.1;

        triggerOnTime = 0.0;
        triggerOffTime = 0.0;
        noteOn = false;
    }

    double GetAmplitude(double time) {
        double amp = 0.0;
        double timeTotal = time - triggerOnTime;

        if (noteOn) {
            //A
            if (timeTotal <= attackTime)
                amp = (timeTotal / attackTime) * startAmp;
            //D
            if(timeTotal > attackTime && timeTotal <= (attackTime+decayTime))
                amp = ((timeTotal-attackTime)/decayTime) * (sustainedAmp - startAmp) * startAmp;

            //S
            if (timeTotal > (attackTime + decayTime)) {
                amp = sustainedAmp;
            }
        }
        else {
            //R
            amp = (time - triggerOffTime)/releaseTime;
        }

        //epsilon check
        if (amp <= 0.0001) {
            amp = 0.0;
        }

        return amp;
    }

    void NoteOn(double timeOn) {
        triggerOnTime = timeOn;
        noteOn = true;
    }
    void NoteOff(double timeOff) {
        triggerOffTime = timeOff;
        noteOn = false; 
    }
};


struct instrument {
    double volume;
    envelopeADSR env;

    virtual double sound(double time, double freq) = 0;
};

//double that isnt hindered by multiple threads
atomic<double> frequencyOut = 0.0;
//12 notes per octave. 
double octaveBaseF = 110.0; //440hz. When you double your frequency, you move up on octave
double twoRoot12 = pow(2.0, 1.0 / 12.0);
envelopeADSR envelope;

//instrument* voice = nullptr;
instrument* voice = nullptr;

struct bell : public instrument {
    bell() {
        env.attackTime = 0.01;
        env.decayTime = 1.0;
        env.startAmp = 1.0;
        env.sustainedAmp = 0.0;
        env.releaseTime = 1.0;
    }

    double sound(double time, double freq) {
        double output = envelope.GetAmplitude(time) * (
            + 1.0 * oscillator(freq * 2.0, time, 0, 5.0, 0.001)
            + 0.5 * oscillator(freq * 3.0, time, 0)
            + 0.25 * oscillator(freq * 4.0, time, 0)
            );
        return output;
    }
};

struct harmonica : public instrument {
    harmonica() {
        env.attackTime = 0.01;
        env.decayTime = 1.0;
        env.startAmp = 1.0;
        env.sustainedAmp = 0.0;
        env.releaseTime = 1.0;
    }

    double sound(double time, double freq) {
        double output = envelope.GetAmplitude(time) * (
            +1.0 * oscillator(freq * 2.0, time, 0, 5.0, 0.001)
            + 0.5 * oscillator(freq * 3.0, time, 0)
            + 0.25 * oscillator(freq * 4.0, time, 4)
            );
        return output;
    }
};

double MakeSound(double duration) {
    //square wave function implementation of sin wave
    double output = voice->sound(duration, frequencyOut);
    return output * 0.4; //Set amplitude/volume
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

    voice = new bell();

    //MakeSound function for sound instance of soundBox
    sound.SetUserFunction(MakeSound);
     
    bool keyPress = false;
    int currKey = -1;

    while (1) {
        //keyboard initialization. 0x8000 highest bit.

        //piano keyboard implementation
        keyPress = false;

        for (int i = 0; i < 16; i++) {
            //Mapping piano keys onto computer keys
            if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe"[i])) & 0x8000) {
                frequencyOut = octaveBaseF * pow(twoRoot12, i);
                voice->env.NoteOn(sound.GetTime());
                wcout << "\rNote On: " << sound.GetTime() << "s, " << frequencyOut << "hz";
                currKey = i;
            }
            keyPress = true;
        }

        if (!keyPress) {
            if (currKey != -1) {
                wcout << "\rNote Off: " << sound.GetTime() << "s";
                voice->env.NoteOff(sound.GetTime());
                currKey = -1;
            }
        }
    }

    return 0;
}
