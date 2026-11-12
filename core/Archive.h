#pragma once
// Archivio delle culture: modi, cicli ritmici, giri ricchi e catalogo delle mosse.
// Dati curati a mano in data/archivio.json (lo stesso file del prototipo web).
#include <juce_core/juce_core.h>
#include <map>
#include <vector>

namespace bottega
{
struct Giro
{
    std::vector<int> gradi;
    juce::String nome;
};

struct Mode
{
    juce::String id, cultura, nome, testo, ornamento;
    std::vector<double> cents;                 // 7 gradi, cents dalla tonica
    int tonica = 0;
    std::vector<int> riposo, cardini, caratteristica;
    double bordone[2] { 0.0, 700.0 };          // tonica e quinta (o quarta nel locrio)
    std::map<int, std::pair<double, double>> mobile; // gradi regolabili a orecchio (koron)
    std::vector<Giro> giri;
    juce::StringArray fonti, ritmi;
};

struct Hit { int step; float vel; };
struct BassStep { int step; int len; double cents; float vel; };
struct Stab { int step; double len; };

struct Rhythm
{
    juce::String id, cultura, nome, testo, comp;
    int pulsazioni = 16, ppb = 4, bpm = 90;
    std::vector<int> gruppi, accenti;
    juce::StringArray segni, colpi;             // colpi: D=dum/cassa, T=tek/rullante, k=leggero, C=campana
    double swing = 0.5, duck = 0.2;
    bool vinile = false;
    std::map<juce::String, std::vector<Hit>> tracks; // solo i groove hanno tracce esplicite
    std::vector<BassStep> bass;
    std::vector<Stab> stabs;
    juce::StringArray fonti;
    bool hasTracks() const { return ! tracks.empty(); }
};

struct SlotDef { int deg; juce::String ext; double dur; juce::String bass; };
struct RichGiro { juce::String id, nome, testo; std::vector<SlotDef> slots; };
struct Mossa { juce::String id, nome, stile, testo; };

class Archive
{
public:
    bool loadFromJson (const juce::String& json, juce::String& error);
    static const Archive& builtIn();            // archivio incorporato nel binario

    std::vector<Mode> modi;
    std::vector<Rhythm> ritmi;
    std::vector<RichGiro> ricchi;
    std::vector<Mossa> mosse;

    const Mode* mode (const juce::String& id) const;
    const Rhythm* rhythm (const juce::String& id) const;
};
} // namespace bottega
