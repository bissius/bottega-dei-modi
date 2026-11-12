#include "Sequencer.h"
#include <algorithm>
#include <cmath>

namespace bottega
{
const char* lane (Inst i)
{
    switch (i)
    {
        case Inst::Rhodes: case Inst::Pad: return "acc";
        case Inst::Bass: return "bas";
        case Inst::Mel: case Inst::MelRelease: return "mel";
        default: return "bat";
    }
}

namespace
{
    Inst percInst (const juce::String& k)
    {
        if (k == "kick") return Inst::Kick;
        if (k == "snare") return Inst::Snare;
        if (k == "ghost") return Inst::Ghost;
        if (k == "hat") return Inst::Hat;
        if (k == "shaker") return Inst::Shaker;
        if (k == "dafDum") return Inst::DafDum;
        if (k == "dafTek") return Inst::DafTek;
        return Inst::Bell;
    }
}

std::vector<Event> realizeCycle (const Theory& th, int c, const std::vector<std::vector<double>>& VO)
{
    auto& S = th.S;
    auto& r = th.R();
    const int P = r.pulsazioni, u = th.sub(), L = th.loopLen();
    const double pd = 60.0 / S.bpm / r.ppb;
    auto st = th.starts();
    std::vector<Event> ev;

    // percussioni
    for (auto& [k, hits] : th.tracksFor (c))
        for (auto& h : hits) { Event e; e.time = th.pulseT (h.step); e.inst = percInst (k); e.vel = h.vel; ev.push_back (e); }
    for (int j = 0; j < S.strato; ++j) { Event e; e.time = j * P / (double) S.strato * pd; e.inst = Inst::Bell; e.vel = 0.55f; ev.push_back (e); }

    // basso
    for (auto& b : th.bassNotes (c)) { Event e; e.time = th.pulseT (b.p); e.dur = b.len * pd * 0.92; e.inst = Inst::Bass; e.cents = b.cents; e.vel = b.vel; ev.push_back (e); }

    // accordi
    auto chord = [&] (int i, double t, double dur, float vel, bool shortN)
    {
        if (i < 0 || i >= (int) VO.size()) return;
        const bool rh = S.suono != "pad", pdOn = S.suono != "rhodes";
        int k = 0;
        for (double cc : VO[(size_t) i])
        {
            if (rh) { Event e; e.time = t + k * (shortN ? 0.006 : 0.016); e.dur = dur; e.inst = Inst::Rhodes; e.cents = cc; e.vel = vel; e.shortNote = shortN; ev.push_back (e); }
            if (pdOn && (! shortN || ! rh)) { Event e; e.time = t; e.dur = dur; e.inst = Inst::Pad; e.cents = cc; e.vel = rh ? 0.8f : 1.2f; ev.push_back (e); }
            ++k;
        }
    };
    if (S.armo != "bordone")
    {
        juce::String cm = S.comp == "auto" ? r.comp : S.comp;
        for (int p = 0; p < P; ++p)
        {
            double pos = c + p / (double) P;
            int i = th.slotAt (pos);
            bool isStart = false;
            for (double x : st) if (std::abs (x - pos) < 1e-6) isStart = true;
            double t = th.pulseT (p);
            int a = r.accenti[(size_t) p];
            if (cm == "tenuto")
            {
                if (isStart) { double end = std::min (st[(size_t) i] + S.slots[(size_t) i].dur, (double) L); chord (i, t, (end - pos) * P * pd * 0.98, 0.7f, false); }
                else if (p == 0 || a >= 2) chord (i, t, pd * 2, 0.42f, true);
            }
            else if (cm == "accenti")
            {
                bool on = false; double l = 1.5;
                if (! r.stabs.empty()) { for (auto& s : r.stabs) if (s.step == p) { on = true; l = s.len; } }
                else on = a >= 2 || (p < r.colpi.size() && r.colpi[p].containsChar ('T'));
                if (on || isStart) chord (i, t, l * pd * 0.9, 0.75f, true);
            }
            else if (cm == "arpeggio")
            {
                int every = r.ppb >= 4 ? 2 : 1;
                if (p % every == 0 && i < (int) VO.size() && ! VO[(size_t) i].empty())
                {
                    auto& V = VO[(size_t) i];
                    double n = V[(size_t) ((p / every) % (int) V.size())];
                    Event e; e.time = t; e.cents = n; e.shortNote = true;
                    if (S.suono != "pad") { e.inst = Inst::Rhodes; e.dur = pd * every * 1.6; e.vel = 0.6f; }
                    else { e.inst = Inst::Pad; e.dur = pd * every * 2; e.vel = 1.4f; }
                    ev.push_back (e);
                }
            }
        }
    }

    // melodia
    if (S.melOn && c < (int) S.frasi.size())
    {
        auto& f = S.frasi[(size_t) c];
        const int U = P * u;
        for (size_t j = 0; j < f.size(); ++j)
        {
            auto& n = f[j];
            double t = (n.u % u == 0) ? th.pulseT (n.u / u) : n.u / (double) u * pd;
            double dur = n.len / (double) u * pd;
            const MelNote* prev = nullptr;
            if (j > 0) prev = &f[j - 1];
            else { int pc = (c - 1 + L) % L; if (pc < (int) S.frasi.size() && ! S.frasi[(size_t) pc].empty()) prev = &S.frasi[(size_t) pc].back(); }
            bool leg = prev && (j > 0 ? prev->u + prev->len == n.u : (prev->u + prev->len == U && n.u == 0));
            auto& mo = th.modeOf (n.mo);
            Event e; e.time = t; e.dur = dur; e.inst = Inst::Mel; e.cents = th.melCents (n); e.orn = n.orn; e.legato = leg;
            e.up = n.t + th.degC (mo, n.deg + 1) + th.melOct();
            ev.push_back (e);
            const MelNote* nx = j + 1 < f.size() ? &f[j + 1] : nullptr;
            if (! nx || nx->u != n.u + n.len) { Event rl; rl.time = t + dur - 0.03; rl.inst = Inst::MelRelease; ev.push_back (rl); }
        }
    }
    std::stable_sort (ev.begin(), ev.end(), [] (const Event& a, const Event& b) { return a.time < b.time; });
    return ev;
}
} // namespace bottega
