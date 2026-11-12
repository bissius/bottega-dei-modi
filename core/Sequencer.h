#pragma once
// Trasforma un ciclo del loop in eventi temporizzati (note e colpi), indipendenti dal motore audio.
// Gli stessi eventi vanno al suono interno e all'uscita MIDI.
#include "Theory.h"

namespace bottega
{
enum class Inst { Rhodes, Pad, Bass, Mel, MelRelease, Kick, Snare, Ghost, Hat, Shaker, DafDum, DafTek, Bell };

struct Event
{
    double time = 0;        // secondi dall'inizio del ciclo
    double dur = 0;         // secondi
    Inst inst = Inst::Rhodes;
    double cents = 0;       // altezza in cents dalla tonica (non usato per le percussioni)
    float vel = 1.0f;
    bool shortNote = false; // accordo staccato (stab) o arpeggio
    // solo melodia
    juce::String orn;
    double up = 0;          // nota superiore per il trillo
    bool legato = false;
};

const char* lane (Inst i);   // canale del mixer: mel, acc, bas, bat

std::vector<Event> realizeCycle (const Theory& th, int c, const std::vector<std::vector<double>>& voicings);
} // namespace bottega
