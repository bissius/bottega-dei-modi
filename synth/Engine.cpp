#include "Engine.h"
#include <cmath>

namespace bottega
{
static constexpr double TWO_PI = 6.283185307179586;

// ======================= Param =======================
void Param::add (Ev e)
{
    // gli eventi arrivano quasi sempre in ordine; inserimento ordinato per sicurezza
    auto it = std::upper_bound (ev.begin() + (long) idx, ev.end(), e.time, [] (double t, const Ev& x) { return t < x.time; });
    ev.insert (it, e);
}
void Param::cancelFrom (double t)
{
    while (ev.size() > idx && ev.back().time >= t) ev.pop_back();
}
double Param::tick (double t, double dt)
{
    while (idx < ev.size() && ev[idx].time <= t)
    {
        auto& e = ev[idx];
        if (e.type == Tgt) { inTarget = true; tgt = e.v; tau = e.tau; }
        else { value = e.v; inTarget = false; }
        lastT = e.time; lastV = value;
        ++idx;
    }
    if (idx > 64) { ev.erase (ev.begin(), ev.begin() + (long) idx); idx = 0; }
    if (idx < ev.size() && (ev[idx].type == Lin || ev[idx].type == Exp))
    {
        auto& e = ev[idx];
        if (inTarget) { lastV = value; lastT = t; inTarget = false; }
        double span = e.time - lastT, x = span > 0 ? juce::jlimit (0.0, 1.0, (t - lastT) / span) : 1.0;
        if (e.type == Lin || lastV <= 0 || e.v <= 0) value = lastV + (e.v - lastV) * x;
        else value = lastV * std::pow (e.v / lastV, x);
        return value;
    }
    if (inTarget) value += (tgt - value) * (1.0 - std::exp (-dt / tau));
    return value;
}

// ======================= Biquad (RBJ) =======================
void Biquad::set (Kind k, double f, double q, double sr)
{
    f = juce::jlimit (10.0, sr * 0.45, f);
    q = std::max (0.05, q);
    double w = TWO_PI * f / sr, cs = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
    double nb0, nb1, nb2;
    if (k == LP) { nb0 = (1 - cs) / 2; nb1 = 1 - cs; nb2 = (1 - cs) / 2; }
    else if (k == HP) { nb0 = (1 + cs) / 2; nb1 = -(1 + cs); nb2 = (1 + cs) / 2; }
    else { nb0 = al; nb1 = 0; nb2 = -al; }
    b0 = (float) (nb0 / a0); b1 = (float) (nb1 / a0); b2 = (float) (nb2 / a0);
    a1 = (float) (-2 * cs / a0); a2 = (float) ((1 - al) / a0);
}

// ======================= voci =======================
static inline double centsHz (double hz, double c) { return hz * std::pow (2.0, c / 1200.0); }
static inline void envAD (Param& g, double t, double peak, double att, double dec)
{
    g.set (t, 0.0001); g.linear (t + att, peak); g.target (t + att, 0.0001, dec);
}

struct Voice
{
    virtual ~Voice() = default;
    // restituisce il campione mono; il motore lo smista su canale e mandate
    virtual float tick (double t, double dt) = 0;
    double start = 0, end = 0;
    juce::String laneId { "acc" };
    float revSend = 0, dlSend = 0;
    bool tremolo = false, duck = false;
    juce::Random* rnd = nullptr;
    int laneK = -1;
    float noise() { return rnd->nextFloat() * 2.0f - 1.0f; }
};

struct Osc
{
    double ph = 0;
    float sine (double f, double dt) { ph += f * dt; ph -= std::floor (ph); return (float) std::sin (TWO_PI * ph); }
    float saw (double f, double dt) { ph += f * dt; ph -= std::floor (ph); return (float) (2 * ph - 1); }
    float sq (double f, double dt) { ph += f * dt; ph -= std::floor (ph); return ph < 0.5 ? 1.0f : -1.0f; }
    float tri (double f, double dt) { ph += f * dt; ph -= std::floor (ph); return (float) (ph < 0.5 ? 4 * ph - 1 : 3 - 4 * ph); }
};

// Rhodes: FM con barra metallica, brillantezza legata alla dinamica
struct Rhodes : Voice
{
    double f; Osc car, m, h2, tine; Param mg, h2g, tg, amp; Biquad lp;
    Rhodes (double hz, double t, double dur, double v, double sr) : f (hz)
    {
        start = t; end = t + dur + 1.4; laneId = "acc"; revSend = 0.28f; tremolo = true; duck = true;
        mg.set (t, f * (0.6 + v * 1.1)); mg.target (t, f * 0.06, 0.22);
        envAD (h2g, t, 0.12 * v, 0.002, 0.18);
        envAD (tg, t, 0.035 * v, 0.001, 0.03);
        lp.set (Biquad::LP, 900 + v * 2600, 0.5, sr);
        amp.set (t, 0); amp.linear (t + 0.004, 0.16 * v); amp.target (t + 0.004, 0.16 * v * 0.4, 0.7); amp.target (t + dur, 0, 0.22);
    }
    float tick (double t, double dt) override
    {
        double mod = m.sine (f, dt) * mg.tick (t, dt);
        float s = car.sine (f + mod, dt) + h2.sine (f * 2, dt) * (float) h2g.tick (t, dt) + tine.sine (f * 7.02, dt) * (float) tg.tick (t, dt);
        return lp.process (s) * (float) amp.tick (t, dt);
    }
};

struct Pad : Voice
{
    double f; Osc a, b, c; Param amp; Biquad lp;
    Pad (double hz, double t, double dur, double v, double sr) : f (hz)
    {
        start = t; end = t + dur + 2.5; laneId = "acc"; revSend = 0.45f; duck = true;
        lp.set (Biquad::LP, std::min (1400.0, f * 2.5), 0.5, sr);
        double at = std::min (0.6, dur * 0.3);
        amp.set (t, 0); amp.linear (t + at, 0.045 * v); amp.target (t + dur, 0, std::max (0.05, at));
    }
    float tick (double t, double dt) override
    {
        float s = a.tri (centsHz (f, -6), dt) + b.tri (centsHz (f, 6), dt) + 0.18f * c.saw (f, dt);
        return lp.process (s) * (float) amp.tick (t, dt);
    }
};

struct Bass : Voice
{
    double f, sr; Osc sub, saw; Param cut, amp; Biquad lp; int n = 0;
    Bass (double hz, double t, double dur, double v, double sampleRate) : f (hz), sr (sampleRate)
    {
        start = t; end = t + dur + 0.3; laneId = "bas";
        cut.set (t, f * 6); cut.target (t, f * 2.2, 0.09);
        amp.set (t, 0); amp.linear (t + 0.006, 0.5 * v); amp.target (t + 0.006, 0.375 * v, 0.3); amp.target (t + dur, 0, 0.04);
    }
    float tick (double t, double dt) override
    {
        double c = cut.tick (t, dt);
        if ((n++ & 15) == 0) lp.set (Biquad::LP, c, 1.2, sr);
        float s = sub.sine (f, dt) + lp.process (0.22f * saw.saw (f, dt));
        return s * (float) amp.tick (t, dt);
    }
};

struct Perc : Voice
{
    Inst kind; double sr; Osc o; Param freq, g1, g2; Biquad f1, f2;
    Perc (Inst k, double t, double v, double sampleRate) : kind (k), sr (sampleRate)
    {
        start = t; laneId = "bat";
        switch (k)
        {
            case Inst::Kick: end = t + 0.8; freq.set (t, 155); freq.expo (t + 0.13, 46); envAD (g1, t, 0.95 * v, 0.002, 0.16); envAD (g2, t, 0.12 * v, 0.001, 0.006); f2.set (Biquad::HP, 2500, 0.7, sr); break;
            case Inst::Snare: case Inst::Ghost:
            {
                bool gh = k == Inst::Ghost;
                end = t + 0.6; f1.set (Biquad::BP, gh ? 2400 : 1900, 0.7, sr); envAD (g1, t, (gh ? 0.25 : 0.55) * v, 0.001, gh ? 0.05 : 0.13);
                freq.set (t, 190); envAD (g2, t, gh ? 0 : 0.35 * v, 0.001, 0.05); revSend = gh ? 0 : 0.5f; break;
            }
            case Inst::Hat: end = t + 0.15; f1.set (Biquad::HP, 8200, 0.7, sr); envAD (g1, t, 0.16 * v, 0.001, 0.025); break;
            case Inst::Shaker: end = t + 0.2; f1.set (Biquad::BP, 6500, 1.2, sr); envAD (g1, t, 0.2 * v, 0.012, 0.035); break;
            case Inst::DafDum: end = t + 1.0; freq.set (t, 110); freq.expo (t + 0.25, 72); envAD (g1, t, 0.6 * v, 0.003, 0.2); f2.set (Biquad::LP, 500, 0.7, sr); envAD (g2, t, 0.25 * v, 0.001, 0.03); break;
            case Inst::DafTek: end = t + 0.5; f1.set (Biquad::BP, 3200, 1.6, sr); envAD (g1, t, 0.35 * v, 0.001, 0.035); f2.set (Biquad::HP, 9000, 0.7, sr); envAD (g2, t, 0.09 * v, 0.004, 0.09); revSend = 0.1f; break;
            default: end = t + 0.5; envAD (g1, t, 0.22 * v, 0.001, 0.09); envAD (g2, t, 0.07 * v, 0.001, 0.04); break; // campana
        }
    }
    Osc o2;
    float tick (double t, double dt) override
    {
        switch (kind)
        {
            case Inst::Kick: return o.sine (freq.tick (t, dt), dt) * (float) g1.tick (t, dt) + f2.process (noise()) * (float) g2.tick (t, dt);
            case Inst::Snare: case Inst::Ghost: return f1.process (noise()) * (float) g1.tick (t, dt) + o.tri (freq.tick (t, dt), dt) * (float) g2.tick (t, dt);
            case Inst::Hat: case Inst::Shaker: return f1.process (noise()) * (float) g1.tick (t, dt);
            case Inst::DafDum: return o.sine (freq.tick (t, dt), dt) * (float) g1.tick (t, dt) + f2.process (noise()) * (float) g2.tick (t, dt);
            case Inst::DafTek: return f1.process (noise()) * (float) g1.tick (t, dt) + f2.process (noise()) * (float) g2.tick (t, dt);
            default: return o.sine (1180, dt) * (float) g1.tick (t, dt) + o2.sine (1180 * 2.71, dt) * (float) g2.tick (t, dt);
        }
    }
};

// Bordone: due seghe leggermente scordate, filtro che respira lentamente
struct Drone : Voice
{
    double f, sr, lfoRate; Osc a, b, l; Param amp; Biquad lp; int n = 0;
    Drone (double hz, double level, double t, double sampleRate, juce::Random& r) : f (hz), sr (sampleRate)
    {
        start = t; end = 1e12; laneId = "bor"; revSend = 0.35f; duck = true;
        lfoRate = 0.07 + r.nextDouble() * 0.05;
        amp.set (t, 0); amp.linear (t + 2, level);
    }
    void stop (double t) { amp.cancelFrom (t); amp.target (t, 0, 0.3); end = t + 2; }
    float tick (double t, double dt) override
    {
        double lf = l.sine (lfoRate, dt);
        if ((n++ & 31) == 0) lp.set (Biquad::LP, std::max (40.0, f * 3 + lf * f * 1.6), 0.6, sr);
        return lp.process (a.saw (f, dt) + b.saw (centsHz (f, 4), dt)) * (float) amp.tick (t, dt);
    }
};

// Voce melodica monofonica e continua: arco (kamancheh), ney, synth
struct Mel : Voice
{
    juce::String T; double sr; Osc o1, o2, o3, vib, slow; Param freq { 440 }, amp, vibDepth, noiseF { 2500 };
    Biquad hp, f1, f2, f3, air, lp, nb; int n = 0; double last = -1; bool hasLast = false; double peak, vibK;
    Mel (const juce::String& timbro, double sampleRate) : T (timbro), sr (sampleRate)
    {
        start = 0; end = 1e12; laneId = "mel"; revSend = 0.55f; dlSend = T == "synth" ? 0.8f : 0.6f;
        peak = T == "ney" ? 0.3 : 0.34; vibK = T == "ney" ? 0.7 : T == "synth" ? 0.6 : 1.0;
        hp.set (Biquad::HP, T == "ney" ? 180 : 260, 0.5, sr);
        f1.set (Biquad::BP, 620, 2.2, sr); f2.set (Biquad::BP, 1350, 3, sr); f3.set (Biquad::BP, 2800, 4, sr); air.set (Biquad::LP, 4200, 0.7, sr);
        lp.set (Biquad::LP, T == "ney" ? 2600 : 2000, T == "ney" ? 0.4 : 5, sr);
    }
    void note (double t, double dur, double hz, const juce::String& orn, bool legato, double upHz, double hzOfCents150)
    {
        freq.cancelFrom (t);
        if (orn == "slideUp") { freq.set (t, hz * std::pow (2.0, -190 / 1200.0)); freq.expo (t + 0.2, hz); }
        else if (orn == "slideDown") { freq.set (t, hzOfCents150); freq.expo (t + 0.28, hz); }
        else if (legato && hasLast) { freq.set (t, last); freq.expo (t + 0.075, hz); }
        else freq.set (t, hz);
        if (orn == "trill")
        {
            double e = t + std::min (dur * 0.5, 0.7); int k = 0;
            for (double x = t + 0.16; x < e; x += 0.075, ++k) freq.set (x, k % 2 ? hz : upHz);
            freq.set (e, hz);
        }
        noiseF.set (t, std::min (6000.0, hz * 2.2));
        if (! legato) { amp.target (t, peak * 0.7, 0.03); amp.target (t + 0.08, peak, 0.15); }
        else amp.target (t, peak, 0.05);
        double depth = (orn == "vib" || orn == "slideDown" ? 24 : 11) * vibK;
        vibDepth.set (t, 0); vibDepth.linear (t + std::min (0.45, dur * 0.6), depth);
        last = hz; hasLast = true;
    }
    void release (double t) { amp.target (t, 0, 0.09); hasLast = false; }
    float tick (double t, double dt) override
    {
        double v = vib.sine (T == "ney" ? 5.0 : 5.6, dt) * vibDepth.tick (t, dt);
        double f = freq.tick (t, dt) * std::pow (2.0, v / 1200.0);
        float s;
        if (T == "ney") s = o1.tri (f, dt) + o2.sine (centsHz (f, 7), dt) + 0.3f * o3.sine (f, dt);
        else s = o1.saw (f, dt) + o2.saw (centsHz (f, T == "synth" ? 12 : 7), dt) + 0.18f * o3.sq (f, dt);
        s = hp.process (s);
        float body;
        if (T == "arco") body = f1.process (s) + 0.8f * f2.process (s) + 0.45f * f3.process (s) + 0.25f * air.process (s);
        else if (T == "ney") body = 1.6f * lp.process (s);
        else
        {
            if ((n & 31) == 0) lp.set (Biquad::LP, 2000 + 700 * slow.sine (0.15, dt * 32), 5, sr);
            body = 0.55f * lp.process (s);
        }
        if ((n++ & 31) == 0) nb.set (Biquad::BP, noiseF.tick (t, dt), T == "ney" ? 0.6 : 0.9, sr); else noiseF.tick (t, dt);
        float ng = T == "ney" ? 0.22f : T == "synth" ? 0.0f : 0.05f;
        if (ng > 0) body += ng * nb.process (noise());
        return body * (float) amp.tick (t, dt);
    }
};

// ======================= Engine =======================
Engine::Engine()
{
    for (auto k : { "mel", "acc", "bas", "bor", "bat", "vin" }) { level[k] = 0.7f; target[k] = 0.7f; }
}
Engine::~Engine() = default;

void Engine::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 2 };
    reverb.prepare (spec);
    juce::dsp::Reverb::Parameters p; p.roomSize = 0.86f; p.damping = 0.45f; p.wetLevel = 1.0f; p.dryLevel = 0.0f; p.width = 1.0f;
    reverb.setParameters (p);
    comp.prepare (spec); comp.setThreshold (-16.0f); comp.setRatio (2.5f); comp.setAttack (10.0f); comp.setRelease (200.0f);
    dlL.assign ((size_t) (sr * 2.1), 0.0f); dlR = dlL; dlPos = 0;
    dlLpL.set (Biquad::LP, 2600, 0.7, sr); dlLpR = dlLpL;
    revLpL.set (Biquad::LP, 5200, 0.7, sr); revLpR = revLpL;
    lofiL.set (Biquad::LP, 18000, 0.5, sr); lofiR = lofiL;
    bus.setSize (6, maxBlock);
    mel = std::make_unique<Mel> (timbro, sr);
    mel->rnd = &noise;
}

void Engine::setMelTimbre (const juce::String& t)
{
    if (t == timbro && mel) return;
    timbro = t;
    mel = std::make_unique<Mel> (timbro, sr);
    mel->rnd = &noise;
}

void Engine::schedule (const Event& e, double when, double hz)
{
    std::unique_ptr<Voice> v;
    switch (e.inst)
    {
        case Inst::Rhodes: v = std::make_unique<Rhodes> (hz, when, e.dur, e.vel, sr); break;
        case Inst::Pad: v = std::make_unique<Pad> (hz, when, e.dur, e.vel, sr); break;
        case Inst::Bass: v = std::make_unique<Bass> (hz, when, e.dur, e.vel, sr); break;
        case Inst::Mel:
        {
            auto* m = static_cast<Mel*> (mel.get());
            double up = hz * std::pow (2.0, (e.up - e.cents) / 1200.0);
            m->note (when, e.dur, hz, e.orn, e.legato, up, hz * std::pow (2.0, 150 / 1200.0));
            return;
        }
        case Inst::MelRelease: static_cast<Mel*> (mel.get())->release (when); return;
        default:
            v = std::make_unique<Perc> (e.inst, when, e.vel, sr);
            if (e.inst == Inst::Kick && duckAmount > 0)
            {
                double d = 1.0 - duckAmount;
                for (auto* p : { &duckAcc, &duckBor }) { p->target (when, d, 0.008); p->target (when + 0.06, 1.0, 0.14); }
            }
    }
    v->rnd = &noise;
    if (voices.size() > 400) voices.erase (voices.begin());   // protezione CPU
    voices.push_back (std::move (v));
}

void Engine::setDrone (bool on, double hzTonic, double c0, double c1)
{
    for (auto& d : drone) static_cast<Drone*> (d.get())->stop (time);
    for (auto& d : drone) voices.push_back (std::move (d));
    drone.clear();
    if (! on) return;
    const double cs[3][2] { { c0 - 1200, 0.07 }, { c1 - 1200, 0.045 }, { c0, 0.035 } };
    for (auto& x : cs)
    {
        auto d = std::make_unique<Drone> (centsHz (hzTonic, x[0]), x[1], time, sr, noise);
        d->rnd = &noise;
        drone.push_back (std::move (d));
    }
}

void Engine::setVinyl (bool on) { vinylOn = on; }

void Engine::setMix (const Song& s, double bpm, double duck)
{
    for (auto& [k, v] : target) { auto it = s.mix.find (k); float lv = it != s.mix.end() ? it->second : 0.7f; auto mu = s.mute.find (k); target[k] = (mu != s.mute.end() && mu->second) ? 0.0f : lv; }
    auto r = s.mix.find ("rev"); revLevel = r != s.mix.end() ? r->second : 0.5f;
    if (lofi != s.lofi) { lofi = s.lofi; lofiL.set (Biquad::LP, lofi ? 3800 : 18000, 0.5, sr); lofiR = lofiL; }
    delaySec = std::min (1.9, 60.0 / bpm * 0.75);
    duckAmount = duck;
}

void Engine::allNotesOff()
{
    voices.clear();
    if (mel) static_cast<Mel*> (mel.get())->release (time);
}

void Engine::render (juce::AudioBuffer<float>& out, int start, int num)
{
    const double dt = 1.0 / sr;
    bus.clear();
    auto* dryL = bus.getWritePointer (0); auto* dryR = bus.getWritePointer (1);
    auto* rvL = bus.getWritePointer (2); auto* rvR = bus.getWritePointer (3);
    auto* dl = bus.getWritePointer (4);
    const char* lanes[] = { "mel", "acc", "bas", "bor", "bat", "vin" };
    float lv[6];
    for (int i = 0; i < num; ++i)
    {
        double t = time + i * dt;
        for (int k = 0; k < 6; ++k) { level[lanes[k]] += (target[lanes[k]] - level[lanes[k]]) * 0.0015f; lv[k] = level[lanes[k]]; }
        float da = (float) duckAcc.tick (t, dt), db = (float) duckBor.tick (t, dt);
        tremPhase += 4.3 * dt; if (tremPhase > 1) tremPhase -= 1;
        double pan = 0.45 * std::sin (TWO_PI * tremPhase);
        float pl = (float) std::cos ((pan + 1) * juce::MathConstants<double>::pi / 4) * 1.414f, pr = (float) std::sin ((pan + 1) * juce::MathConstants<double>::pi / 4) * 1.414f;
        auto mixIn = [&] (Voice& v, float s)
        {
            if (v.laneK < 0) { v.laneK = 1; for (int q = 0; q < 6; ++q) if (v.laneId == lanes[q]) v.laneK = q; }
            int k = v.laneK;
            float g = lv[k] * (v.duck ? (k == 3 ? db : da) : 1.0f);
            float x = s * g;
            if (v.tremolo) { dryL[i] += x * pl; dryR[i] += x * pr; } else { dryL[i] += x; dryR[i] += x; }
            rvL[i] += x * v.revSend; rvR[i] += x * v.revSend; dl[i] += x * v.dlSend;
        };
        for (auto& v : voices) if (t >= v->start && t < v->end) mixIn (*v, v->tick (t, dt));
        for (auto& v : drone) mixIn (*v, v->tick (t, dt));
        if (mel) mixIn (*mel, mel->tick (t, dt));
        if (vinylOn)
        {
            vinLp += 0.08f * (noise.nextFloat() * 2 - 1 - vinLp);
            float s = vinLp * 0.25f; if (noise.nextFloat() < 0.0006f) s += (noise.nextFloat() * 2 - 1) * 0.9f;
            dryL[i] += s * 0.22f * lv[5]; dryR[i] += s * 0.22f * lv[5];
        }
    }
    // riverbero
    {
        float* chans[] { rvL, rvR };
        juce::dsp::AudioBlock<float> b (chans, 2, (size_t) num);
        juce::dsp::ProcessContextReplacing<float> ctx (b);
        reverb.process (ctx);
    }
    auto* oL = out.getWritePointer (0, start);
    auto* oR = out.getNumChannels() > 1 ? out.getWritePointer (1, start) : nullptr;
    const int dlN = (int) dlL.size();
    const int dlSamp = juce::jlimit (1, dlN - 1, (int) (delaySec * sr));
    for (int i = 0; i < num; ++i)
    {
        float r1 = revLpL.process (rvL[i]) * revLevel * 0.35f, r2 = revLpR.process (rvR[i]) * revLevel * 0.35f;
        int rp = (dlPos - dlSamp + dlN) % dlN;
        float dOutL = dlLpL.process (dlL[(size_t) rp]), dOutR = dlLpR.process (dlR[(size_t) rp]);
        dlL[(size_t) dlPos] = dl[i] + dOutL * 0.32f; dlR[(size_t) dlPos] = dl[i] + dOutR * 0.32f;
        dlPos = (dlPos + 1) % dlN;
        float L = dryL[i] + r1 + dOutL * 0.28f, R = dryR[i] + r2 + dOutR * 0.28f;
        L = lofiL.process (L); R = lofiR.process (R);
        static const float norm = std::tanh (1.4f);
        oL[i] = std::tanh (1.4f * L) / norm;
        if (oR) oR[i] = std::tanh (1.4f * R) / norm;
    }
    {
        juce::dsp::AudioBlock<float> b (out); auto sub = b.getSubBlock ((size_t) start, (size_t) num);
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        comp.process (ctx);
        sub.multiplyBy (0.9f);
    }
    time += num * dt;
    voices.erase (std::remove_if (voices.begin(), voices.end(), [this] (auto& v) { return time > v->end; }), voices.end());
}
} // namespace bottega
