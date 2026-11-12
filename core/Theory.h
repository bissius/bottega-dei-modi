#pragma once
// Logica musicale: accordi dal modo, nomi e analisi funzionale, mosse, ritmo, basso, voicing, melodia.
// Porting fedele del prototipo web v0.4 (docs/prototipo-web-v04.html).
#include "Song.h"

namespace bottega
{
struct Chord
{
    int i = 0;
    const Slot* sl = nullptr;
    const Mode* mo = nullptr;
    double t = 0, r = 0;              // centro tonale e radice (0..1199)
    std::vector<double> iv, pcs;      // intervalli sopra la radice, classi d'altezza
};

struct BassNote { double p, len, cents; float vel; int slot; };

struct Ext { const char* id; const char* nome; std::vector<int> k; };
const std::vector<Ext>& extensions();
const Ext* findExt (const juce::String& id);

double mod (double a, double n);
int imod (int a, int n);
double cdist (double a, double b);

class Theory
{
public:
    Theory (const Archive& a, Song& s, juce::Random& r) : A (a), S (s), rnd (r) {}

    const Archive& A;
    Song& S;
    juce::Random& rnd;

    const Mode& M() const;
    const Mode& modeOf (const juce::String& id) const;
    const Rhythm& R() const;
    double tonPc() const { return S.ton * 100.0; }
    double degC (const Mode& mo, int i) const;

    // nomi
    juce::String nameC (double c, int deg = -999, bool italiano = false, bool senzaDeviazione = false) const;
    juce::String nIt (double c, int deg = -999) const { return nameC (c, deg, true); }
    int degOfPc (const Mode& mo, double t, double pc) const;

    // accordi e analisi
    Chord chordOf (int i) const;
    juce::String sym (const Chord& c) const;
    juce::String romanOf (const Chord& c, double center) const;
    juce::String roman (const Chord& c) const;
    juce::String funzione (const Chord& c) const;
    juce::StringArray speciali (const Chord& c) const;
    juce::String attrito (int i) const;        // melodia contro accordo, vuoto se nessun attrito

    // sequenza
    double beatsPerCycle() const;
    double scaleDur (double d) const;
    std::vector<std::pair<juce::String, juce::String>> giriList() const;
    void genArm (juce::String id = {});
    juce::String giroTxt() const;
    void fixLen();
    int loopLen() const;
    std::vector<double> starts() const;
    int slotAt (double pos) const;
    juce::String mossa (const juce::String& id, const juce::String& versoModo = {});
    juce::String varArm();

    // ritmo
    juce::String kitOf() const;
    TrackMap baseTracks() const;
    TrackMap varianteRit();
    void genRit();
    juce::String varRit();
    TrackMap tracksFor (int c) const;
    double swingV() const;
    double pulseT (double p) const;       // tempo di una pulsazione in secondi dall'inizio del ciclo (con swing)
    double cycleDur() const;

    // basso e voicing
    double bassRoot (int i) const;
    double approach (double target, const Mode& mo, double t) const;
    std::vector<BassNote> bassNotes (int c) const;
    std::vector<std::vector<double>> voiceAll() const;

    // melodia
    int sub() const;
    double weight (int p) const;
    std::vector<MelNote> genMelCycle (int c, int prev, bool last);
    void genMel();
    juce::String varMel();
    double melOct() const { return S.ton >= 9 ? 3600.0 : 2400.0; }
    double melCents (const MelNote& n) const;

    // tutto insieme
    juce::String sorprendimi();
    void refresh (bool regen);            // riallinea lunghezze, variazioni e melodia dopo una modifica

private:
    struct Q { char t3 = 0; bool p5 = false, d5 = false; double s7 = -1; bool maj7 = false; };
    Q qual (const Chord& c) const;
    double ivAt (const Chord& c, int k) const;
    juce::String rootName (const Chord& c) const;
    std::vector<double> voiceOne (const Chord& c, const std::vector<double>* prev) const;
    int splitInsert (int i, Slot ns);
    template <typename T> const T& pick (const std::vector<T>& v) { return v[(size_t) rnd.nextInt ((int) v.size())]; }
};
} // namespace bottega
