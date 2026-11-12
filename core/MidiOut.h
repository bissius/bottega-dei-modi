#pragma once
// MIDI in uscita per altezze non temperate: ogni nota ha il suo canale e il suo pitch bend.
// Canali 2-9 e 11-16 per le note (uno per nota, come MPE), canale 10 per le percussioni (General MIDI).
// Il synth che riceve deve avere la stessa estensione del pitch bend (default 48 semitoni, standard MPE).
#include <array>
#include <cmath>
#include <vector>

namespace bottega
{
struct MidiNote
{
    int channel = 2;     // 1..16
    int note = 60;       // 0..127
    int bend = 8192;     // 0..16383, 8192 = centro
    int id = -1;
};

class ChannelAllocator
{
public:
    // altezza in frequenza -> nota MIDI più vicina + pitch bend del resto
    static void split (double hz, int bendRange, int& note, int& bend)
    {
        double m = 69.0 + 12.0 * std::log2 (hz / 440.0);
        note = (int) std::lround (m);
        if (note < 0) note = 0;
        if (note > 127) note = 127;
        double semis = m - note;
        bend = (int) std::lround (8192.0 + semis / bendRange * 8192.0);
        if (bend < 0) bend = 0;
        if (bend > 16383) bend = 16383;
    }

    // sceglie il canale libero da più tempo; se sono tutti occupati ruba il più vecchio
    MidiNote noteOn (double hz, int bendRange, int& stolenId)
    {
        stolenId = -1;
        MidiNote n;
        split (hz, bendRange, n.note, n.bend);
        int best = -1; long oldest = 0;
        for (int i = 0; i < (int) members.size(); ++i)
            if (owner[(size_t) i] < 0 && (best < 0 || lastUse[(size_t) i] < oldest)) { best = i; oldest = lastUse[(size_t) i]; }
        if (best < 0)
        {
            for (int i = 0; i < (int) members.size(); ++i)
                if (best < 0 || lastUse[(size_t) i] < oldest) { best = i; oldest = lastUse[(size_t) i]; }
            stolenId = owner[(size_t) best];
        }
        n.channel = members[(size_t) best];
        n.id = ++counter;
        owner[(size_t) best] = n.id;
        lastUse[(size_t) best] = counter;
        return n;
    }

    void noteOff (int id)
    {
        for (size_t i = 0; i < members.size(); ++i) if (owner[i] == id) { owner[i] = -1; lastUse[i] = ++counter; }
    }

    void reset() { owner.fill (-1); lastUse.fill (0); }

    static int drumNote (int inst)  // indice di Inst::Kick.. -> nota GM
    {
        static const int gm[] = { 36, 38, 37, 42, 70, 45, 39, 56 }; // kick, snare, ghost(side stick), hat, maracas, low tom (dum), clap (tek), cowbell
        return gm[inst];
    }

private:
    std::array<int, 14> members { 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16 };
    std::array<int, 14> owner { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    std::array<long, 14> lastUse {};
    long counter = 0;
};
} // namespace bottega
