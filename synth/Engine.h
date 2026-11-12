#pragma once
// Motore audio interno: porting dei suoni del prototipo web (Rhodes FM, pad, basso, bordone,
// percussioni, voce melodica continua con glissandi) con mixer a canali, mandate post-fader,
// ducking dalla cassa, riverbero, delay, lo-fi, saturazione e compressore.
// Tutte le altezze arrivano in Hz, quindi i microtoni suonano esatti.
#include <juce_dsp/juce_dsp.h>
#include "Sequencer.h"

namespace bottega
{
// Parametro automatizzabile nel tempo, con la stessa semantica di un AudioParam del Web Audio
// (setValue, rampa lineare/esponenziale, setTarget con costante di tempo).
class Param
{
public:
    explicit Param (double v = 0) : value (v) {}
    void set (double t, double v) { add ({ Set, t, v, 0 }); }
    void linear (double t, double v) { add ({ Lin, t, v, 0 }); }
    void expo (double t, double v) { add ({ Exp, t, v, 0 }); }
    void target (double t, double v, double tau) { add ({ Tgt, t, v, std::max (1e-4, tau) }); }
    void cancelFrom (double t);
    double tick (double t, double dt);
    double value;

private:
    enum Type { Set, Lin, Exp, Tgt };
    struct Ev { Type type; double time, v, tau; };
    void add (Ev e);
    std::vector<Ev> ev;
    size_t idx = 0;
    double lastT = 0, lastV = 0;
    bool inTarget = false; double tgt = 0, tau = 1;
};

class Biquad
{
public:
    enum Kind { LP, HP, BP };
    void set (Kind k, double f, double q, double sr);
    float process (float x)
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
private:
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
};

struct Voice;

class Engine
{
public:
    Engine();
    ~Engine();
    void prepare (double sampleRate, int maxBlock);
    // evento del sequencer a un tempo assoluto (secondi del motore). hzOf converte cents -> Hz.
    void schedule (const Event& e, double when, double hz);
    void setDrone (bool on, double hzTonic, double c0, double c1);
    void setVinyl (bool on);
    void setMelTimbre (const juce::String& timbro);
    void setMix (const Song& s, double bpm, double duck);
    void allNotesOff();
    void render (juce::AudioBuffer<float>& out, int start, int num);
    double now() const { return time; }

private:
    double sr = 44100, time = 0;
    std::vector<std::unique_ptr<Voice>> voices;
    std::unique_ptr<Voice> mel;
    std::vector<std::unique_ptr<Voice>> drone;
    bool vinylOn = false; float vinLp = 0;
    juce::Random noise;
    juce::String timbro { "arco" };

    // mixer
    std::map<juce::String, float> level, target;
    Param duckAcc { 1 }, duckBor { 1 };
    double duckAmount = 0.2;
    float revLevel = 0.5f; bool lofi = false;
    double delaySec = 0.55;

    juce::dsp::Reverb reverb;
    std::vector<float> dlL, dlR; int dlPos = 0; Biquad dlLpL, dlLpR, lofiL, lofiR, revLpL, revLpR;
    juce::dsp::Compressor<float> comp;
    double tremPhase = 0;
    juce::AudioBuffer<float> bus;    // 0-1: dry, 2-3: reverb send, 4-5: delay send
    friend struct Voice;
};
} // namespace bottega
