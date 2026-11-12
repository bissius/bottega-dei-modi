#pragma once
// Stato completo di un esperimento: le tre corsie, tonica, tempo, mixer e note dell'utente.
// È anche il formato dei preset (JSON) e dello stato salvato nel progetto di Studio One.
#include "Archive.h"
#include <optional>

namespace bottega
{
using TrackMap = std::map<juce::String, std::vector<Hit>>;

struct Slot
{
    int deg = 0;                      // grado del modo (0..6)
    juce::String ext { "7" };         // estensione: triade, 5, 6, 7, 9, 11, 13, add9, 69, sus2, sus4, 7sus4, 9sus4
    double dur = 1.0;                 // durata in cicli del ritmo (multipli di 0.5)
    juce::String bass { "radice" };   // radice | rivolto | pedale
    juce::String modo;                // vuoto = modo globale (serve per scambio modale e cambio di scala)
    double t = 0.0;                   // centro tonale relativo in cents (pivot, mezzo tono)
    // accordi creati dalle mosse: radice e intervalli espliciti
    bool explicitIv = false;
    double r = 0.0;
    std::vector<double> iv;
    juce::String q, rom, fun, tag, nota;
};

struct MelNote
{
    int c = 0;        // ciclo
    int u = 0;        // posizione in unità (pulsazione * suddivisione)
    int len = 1;      // durata in unità
    int deg = 0;
    juce::String mo;  // modo usato
    double t = 0.0;   // centro tonale
    juce::String orn; // vib, trill, slideUp, slideDown
};

struct Song
{
    // globali
    juce::String nome { "Esperimento" }, note, tags;
    int ton = 2;                      // 0 = Do ... 11 = Si
    double hz = 146.83;
    double bpm = 82.0;
    juce::String modo { "shur" };
    std::map<juce::String, double> koron { { "shur", 140.0 }, { "bayati", 150.0 } };
    int caso = 30;                    // manopola del caso 0..100

    // ritmo
    juce::String rit { "triphop" }, kit { "auto" };
    int strato = 0;                   // poliritmia: N colpi sul ciclo (0 = spenta)
    double swing = -1.0;              // < 0 = quello del ciclo
    bool ritLock = false;
    std::vector<std::optional<TrackMap>> ritVar;

    // armonia
    std::vector<Slot> slots;
    int sel = 0;
    juce::String giro { "r:pedale" }, voc { "stretto" }, armo { "terze" }, comp { "auto" }, suono { "rp" }, basso { "radice" };
    bool armLock = false;

    // melodia
    bool melOn = true, melLock = false;
    juce::String timbro { "arco" }, dens { "media" };
    std::vector<std::vector<MelNote>> frasi;

    // mixer
    std::map<juce::String, float> mix { { "mel", 0.7f }, { "acc", 0.7f }, { "bas", 0.8f }, { "bor", 0.45f }, { "bat", 0.75f }, { "vin", 0.35f }, { "rev", 0.5f } };
    std::map<juce::String, bool> mute;
    bool lofi = false;

    // uscita: suono interno e/o MIDI (con pitch bend per nota)
    bool suonoInterno = true, midiOut = false;
    int pitchBendRange = 48;          // semitoni (48 = standard MPE)

    juce::var toVar() const;
    static Song fromVar (const juce::var& v);
    juce::String toJson() const { return juce::JSON::toString (toVar(), false); }
};
} // namespace bottega
