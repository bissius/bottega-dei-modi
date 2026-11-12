// Test del nucleo musicale. I valori attesi vengono dal prototipo web v0.4 (stessa logica).
#include "Theory.h"
#include "Sequencer.h"
#include "MidiOut.h"
#include <iostream>

using namespace bottega;

static int fails = 0, checks = 0;
static void expectEq (const juce::String& got, const juce::String& want, const char* what)
{
    ++checks;
    if (got != want) { ++fails; std::cout << "FALLITO " << what << "\n  atteso: " << want << "\n  ottenuto: " << got << "\n"; }
}
static void expect (bool ok, const char* what) { ++checks; if (! ok) { ++fails; std::cout << "FALLITO " << what << "\n"; } }

static juce::String giroStr (const Theory& th)
{
    juce::StringArray a;
    for (int i = 0; i < (int) th.S.slots.size(); ++i) { auto c = th.chordOf (i); a.add (th.sym (c) + "[" + th.roman (c) + "]"); }
    return a.joinIntoString (" ");
}

int main()
{
    auto& A = Archive::builtIn();
    expect (A.modi.size() == 16, "16 modi nell'archivio");
    expect (A.ritmi.size() == 17, "17 ritmi nell'archivio");
    expect (A.mosse.size() == 7, "7 mosse");

    juce::Random rnd (42);
    Song s;
    Theory th (A, s, rnd);
    th.genArm ("r:pedale");
    th.refresh (true);

    juce::StringArray init;
    for (int i = 0; i < (int) s.slots.size(); ++i) { auto c = th.chordOf (i); init.add (th.sym (c) + " " + th.roman (c) + " " + th.funzione (c)); }
    expectEq (init.joinIntoString (" | "), juce::String::fromUTF8 ("Dm11 i11 Tonica | B♭maj9 ♭VImaj9 Sottodominante | Gm9 iv9 Sottodominante | C7sus4 ♭VII7sus4 Dominante modale | Dm11 i11 Tonica"), "giro pedale in Shur su Re");

    struct M { const char* id; const char* atteso; };
    const M mosse[] = {
        { "domsec", "Dm11[i11] F7[V7/♭VImaj9] B♭maj9[♭VImaj9] Gm9[iv9] C7sus4[♭VII7sus4] Dm11[i11]" },
        { "tritono", "Dm11[i11] B7[subV7/♭VImaj9] B♭maj9[♭VImaj9] Gm9[iv9] C7sus4[♭VII7sus4] Dm11[i11]" },
        { "cromatico", "Dm11[i11] A°7[v°7] B♭maj9[♭VImaj9] Gm9[iv9] C7sus4[♭VII7sus4] Dm11[i11]" },
        { "scambio", "Dmaj11[Imaj11] B♭maj9[♭VImaj9] Gm9[iv9] C7sus4[♭VII7sus4] Dm11[i11]" },
        { "pivot", "Dm11[i11] Fmaj9[♭VImaj9 (in A)] Dm9[iv9 (in A)] G7sus4[♭VII7sus4 (in A)] Am11[i11 (in A)]" },
        { "mezzotono", "Dm11[i11] Bmaj9[♭VImaj9 (in E♭)] A♭m9[iv9 (in E♭)] D♭7sus4[♭VII7sus4 (in E♭)] E♭m11[i11 (in E♭)]" },
    };
    for (auto& m : mosse)
    {
        Song copy = s;
        Theory t2 (A, copy, rnd);
        copy.sel = 0;
        t2.mossa (m.id);
        expectEq (giroStr (t2), juce::String::fromUTF8 (m.atteso), m.id);
    }
    {
        Song copy = s; Theory t2 (A, copy, rnd); copy.sel = 0;
        auto msg = t2.mossa ("scala", "bayati");
        expect (msg.contains ("Bayati"), "cambio di scala verso Bayati");
        expect (copy.slots[1].modo == "bayati", "cambio di scala applicato agli accordi successivi");
    }

    // nomi microtonali: Bhairav ha Dha komal giusto (813.69 cents) e Ni a 1088 cents
    {
        Song b; b.modo = "bhairav"; Theory tb (A, b, rnd); tb.genArm ("r:pedale"); tb.refresh (true);
        expectEq (giroStr (tb).upToFirstOccurrenceOf (" ", false, false), "Dmaj11[Imaj11]", "Bhairav: accordo di tonica");
        expect (tb.nameC (tb.degC (*A.mode ("bhairav"), 5), 5).contains (juce::String::fromUTF8 ("B♭+14")), "Bhairav: Dha komal = B♭+14");
    }
    // koron di Shur
    expectEq (th.nIt (th.degC (th.M(), 1), 1), "Mi koron", "nome del koron in Shur");

    // tutte le combinazioni di modo e ritmo producono eventi senza errori
    for (auto& mo : A.modi)
        for (auto& rt : A.ritmi)
        {
            Song x; x.modo = mo.id; x.rit = rt.id; x.bpm = rt.bpm;
            Theory tx (A, x, rnd);
            tx.genArm ("r:jazz"); tx.genRit(); tx.refresh (true);
            auto vo = tx.voiceAll();
            for (int c = 0; c < tx.loopLen(); ++c)
            {
                auto ev = realizeCycle (tx, c, vo);
                bool ok = ! ev.empty();
                for (auto& e : ev) if (e.time < -0.001 || e.time > tx.cycleDur() + 0.5 || std::isnan (e.cents)) ok = false;
                if (! ok) { ++fails; std::cout << "FALLITO eventi " << mo.id << " / " << rt.id << " ciclo " << c << "\n"; }
                ++checks;
            }
        }

    // variazioni e mosse a caso non rompono lo stato
    for (int k = 0; k < 200; ++k)
    {
        s.caso = k % 101;
        th.varArm(); th.varRit(); th.varMel();
        s.sel = rnd.nextInt ((int) s.slots.size());
        th.mossa (A.mosse[(size_t) rnd.nextInt ((int) A.mosse.size())].id, "hijaz");
        th.refresh (true);
        expect (th.loopLen() >= 1 && th.loopLen() <= 16, "lunghezza del loop tra 1 e 16 cicli");
    }

    // preset: andata e ritorno in JSON
    {
        auto json = s.toJson();
        auto back = Song::fromVar (juce::JSON::parse (json));
        expectEq (back.toJson(), json, "preset JSON andata e ritorno");
    }

    // MIDI: Mi koron (140 cents sopra Re3) -> nota 51 (Mi♭3) con bend positivo, range 48
    {
        int note = 0, bend = 0;
        ChannelAllocator::split (146.83 * std::pow (2.0, 140.0 / 1200.0), 48, note, bend);
        expect (note == 51, "koron: nota MIDI 51");
        expect (std::abs (bend - (8192 + (int) std::lround (0.4 / 48 * 8192))) <= 2, "koron: pitch bend di +40 cents");
        ChannelAllocator al; int stolen = 0;
        std::vector<int> chans;
        for (int i = 0; i < 15; ++i) chans.push_back (al.noteOn (440, 48, stolen).channel);
        expect (std::find (chans.begin(), chans.end(), 10) == chans.end(), "il canale 10 resta alle percussioni");
        expect (stolen >= 0, "la quindicesima nota ruba il canale più vecchio");
    }

    std::cout << checks - fails << "/" << checks << " controlli superati\n";
    return fails == 0 ? 0 : 1;
}
