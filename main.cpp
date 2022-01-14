//program needs to run on win32 platform

#include <iostream>
#include <algorithm>
#include <list>
using namespace std;

//Accessing existing code for accessing computer audio hardware. Source: Javidx9 on Github for header
#include "soundBox.h"
#define FREQTYPE double



namespace synth {
    //Convert Freq to angular velocity

    FREQTYPE fConvert(const FREQTYPE hertz) {
        return hertz * 2.0 * PI;
    }
    struct note {
        int noteID;
        FREQTYPE on; 
        FREQTYPE off;
        bool active;
        int channel;

        note() {
            on = 0.0;
            off = 0.0;
            noteID = 0;
            active = false;
            channel = 0;
        }
    };

    const int oSin = 0;
    const int oSq = 1;
    const int oSawA = 2;
    const int oSawD = 3;
    const int rando = 4;

    FREQTYPE oscillator(const FREQTYPE hertz, const FREQTYPE time, const int type = oSin,
                        const FREQTYPE LFOFreq = 0.0, const FREQTYPE LFOAmp = 0.0, FREQTYPE counter = 50.0) {
        
        FREQTYPE freq = fConvert(hertz) * time + LFOAmp * hertz * sin(fConvert(LFOFreq) * time);

        switch (type) {
        
        case oSin: //sin wave
            return sin(freq);
        
        case oSq: //square wave
            return sin(freq) > 0 ? 1.0 : -1.0;
         
        //case 2: //triangular wave
        //   return asin(sin(freq)) * 2.0 / PI);
        
        case oSawA: //saw wave (warm)
        {
            FREQTYPE out = 0.0;

            for (FREQTYPE i = 1.0; i < counter; i++)
                out += (sin(i * freq)) / i;

            return out * (2.0 / PI);
        }
        
        case oSawD: //optimized saw wave (harsh)
            return (hertz * PI * fmod(time, 1.0 / hertz) - (PI / 2.0)) * (2.0 / PI);

        case rando: //pseudo random
            return ((FREQTYPE)(rand()) / (FREQTYPE)(RAND_MAX)) - 1.0;

        default:
            return 0.0;
        }
    }

    const int DEFAULT_SCALE = 0;

    FREQTYPE scale(const int noteID, const int scaleID = DEFAULT_SCALE) {
        switch (scaleID) {
        case DEFAULT_SCALE: default:
            return 256 * pow(1.0594639094, noteID);
        }
    }

    struct envelope {
        virtual FREQTYPE getAmp(const FREQTYPE time, const FREQTYPE timeOn, const FREQTYPE timeOff) = 0;
    };

    struct envelopeADSR : public envelope {
        FREQTYPE attackTime;
        FREQTYPE decayTime;
        FREQTYPE releaseTime;
        FREQTYPE sustainedAmp;
        FREQTYPE startAmp;


        //Attack, Decay, Sustained, Release
        envelopeADSR() {
            attackTime = 0.01;
            decayTime = 0.1;
            releaseTime = 1.0;
            sustainedAmp = 0.2;
            startAmp = 1.0;
        }

        virtual FREQTYPE getAmp(const FREQTYPE time, const FREQTYPE timeOff, const FREQTYPE timeOn) {
            FREQTYPE amp = 0.0;
            FREQTYPE releaseAmp = 0.0;


            if (timeOn > timeOff) { //note on
                FREQTYPE total = time - timeOn;

                //A
                if (total <= attackTime)
                    amp = (total / attackTime) * startAmp;
                
                //D
                if (total > attackTime && total <= (attackTime + decayTime))
                    amp = ((total - attackTime) / decayTime) * (sustainedAmp - startAmp) * startAmp;

                //S
                if (total > (attackTime + decayTime)) {
                    amp = sustainedAmp;
                }
            }

            else { //note off
                //R
                FREQTYPE total = timeOff - timeOn;

                if (total < attackTime)
                    releaseAmp = (total / attackTime) * startAmp;

                if (total > attackTime && total <= (attackTime + decayTime))
                    releaseAmp = ((total - attackTime) / decayTime) * (sustainedAmp - startAmp) + startAmp;

                if (total > (attackTime + decayTime))
                    releaseAmp = sustainedAmp;
                
                    amp = ((time - timeOff) / releaseTime) * (0.0 - releaseAmp) + releaseAmp;
            }

            //epsilon check for positive
            if (amp <= 0.1) {
                amp = 0.0;
            }

            return amp;
        }
    };

    FREQTYPE env(const FREQTYPE time, envelope &env, const FREQTYPE timeOn, const FREQTYPE timeOff) {
        return env.getAmp(time, timeOn, timeOff);
    }

    struct instrument {
        FREQTYPE volume;
        synth::envelopeADSR env;

        virtual FREQTYPE sound(const FREQTYPE time, synth::note n, bool &noteFinished) = 0;
    };

    struct bell : public instrument {
        bell() {
            env.attackTime = 0.01;
            env.decayTime = 1.0;
            env.sustainedAmp = 0.0;
            env.releaseTime = 1.0;

            volume = 1.0;
        }

        virtual FREQTYPE sound(const FREQTYPE time, synth::note n, bool &noteFinished) {
            FREQTYPE amp = synth::env(time, env, n.on, n.off);
            
            if (amp <= 0.0)
                noteFinished = true; 

            FREQTYPE sound = + 1.00 * synth::oscillator(n.on - time, synth::scale(n.noteID + 12), synth::oSin, 5.0, 0.001)
                             + 0.50 * synth::oscillator(n.on - time, synth::scale(n.noteID + 24))
                             + 0.25 * synth::oscillator(n.on - time, synth::scale(n.noteID + 36));

            return amp * sound * volume;
        }
    };

    struct harmonica : public instrument {
        harmonica() {
            env.attackTime = 0.05;
            env.decayTime = 1.0;
            env.sustainedAmp = 0.95;
            env.releaseTime = 0.1;

            volume = 1.0;
        }

        virtual FREQTYPE sound(const FREQTYPE time, synth::note n, bool& noteFinished) {
            FREQTYPE amp = synth::env(time, env, n.on, n.off);

            if (amp <= 0.0)
                noteFinished = true;

            FREQTYPE sound = + 1.00 * synth::oscillator(n.on - time, synth::scale(n.noteID), oSq, 50.0, 0.00001)
                             + 0.20 * synth::oscillator(n.on - time, synth::scale(n.noteID + 12), oSq)
                             + 0.0001 * synth::oscillator(n.on - time, synth::scale(n.noteID + 24), rando);
    
            return amp * sound * volume;
         }
    };
}

vector<synth::note> vecNotes;
mutex muxNotes; 
synth::bell bellInst;
synth::harmonica harmInst;

// Credit to Javidx9 for following vector overload fix
typedef bool(*lambda)(synth::note const& item);
template<class T>
void safe_remove(T &v, lambda f) {
    auto n = v.begin();
    while (n != v.end()){
        if (!f(*n))
            n = v.erase(n);
        else
            ++n;
    }   
}
    

 FREQTYPE MakeSound(int channel,FREQTYPE time) {
    unique_lock<mutex> lm(muxNotes);
    FREQTYPE mixOut = 0.0;

    for (auto &note : vecNotes) {
        bool noteFinished = false;
        FREQTYPE sound = 0;

        if (note.channel == 1)
            sound = harmInst.sound(time, note, noteFinished) * 0.1;
        if (note.channel == 2)
            sound = bellInst.sound(time, note, noteFinished);
        mixOut += sound;

          if (noteFinished && note.off > note.on)
            note.active = false;
    }

    safe_remove<vector<synth::note>>(vecNotes, [](synth::note const& item) { return item.active; });

    return mixOut * 0.2; //Set final volume
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
    //12 notes per octave. When you double your frequency, you move up on octave

    soundBox<short> sound(devices[0], 44100, 1, 8, 512);

    //MakeSound function for sound instance of soundBox
    sound.SetUserFunction(MakeSound);

    while (1) {
        //0x8000 highest bit
        for (int i = 0; i < 16; i++) {
            //Mapping piano keys onto computer keys
            short keyState = GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[i]));
            double currTime = sound.GetTime();

            //Check if note already exists
            muxNotes.lock();
            auto foundNote = find_if(vecNotes.begin(), vecNotes.end(), [&i](synth::note const& item) {
                return item.noteID == i; });

            if (foundNote == vecNotes.end()) {//Note not found

                if (keyState & 0x8000) {
                    //Create new note for new key press
                    synth::note n;
                    n.noteID = i;
                    n.on = currTime;
                    n.channel = 1;
                    n.active = true;

                    //Add this new note to vector
                    vecNotes.emplace_back(n);
                }
                else {}
            }
            else {//Note found in vector
                if (keyState & 0x8000) {//Held key
                    if (foundNote->off > foundNote->on) {
                        //Key has been pressed again
                        foundNote->off = currTime;
                        foundNote->active = false;
                    }
                    else {
                        foundNote->on = currTime;
                    }
                }
                else {//Released key
                    if (foundNote->off < foundNote->on) {
                        foundNote->off = currTime;
                    }
                    else {
                    }
                }

            }

            muxNotes.unlock();
        }
    }

    return 0;
}
