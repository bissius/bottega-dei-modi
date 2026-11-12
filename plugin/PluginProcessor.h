#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"
#include "MidiOut.h"

// Processore audio: possiede lo stato (Song), la logica (Theory), il motore interno e l'uscita MIDI.
// Il loop gira da solo (standalone) o segue il trasporto e il tempo della DAW (plugin).
class BottegaProcessor : public juce::AudioProcessor
{
public:
    BottegaProcessor();
    ~BottegaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Bottega dei Modi"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- usato dall'interfaccia (thread dei messaggi) ----
    juce::CriticalSection lock;          // protegge song: l'interfaccia lo tiene durante le modifiche
    bottega::Song song;
    juce::Random rnd;
    bottega::Theory theory { bottega::Archive::builtIn(), song, rnd };

    void snapshot();                      // per Annulla
    bool undo();
    bool canUndo() const { return ! history.empty(); }
    void changed (bool regen);            // dopo ogni modifica: ricalcola e aggiorna il suono
    void setPlaying (bool p) { uiPlay = p; }
    bool uiPlaying() const { return uiPlay.load(); }
    bool isPlaying() const { return playingNow.load(); }
    double playPosition() const { return playPos.load(); }       // posizione nel loop in cicli
    bool hostSync() const { return hostSynced.load(); }
    void preview (int slot);              // ascolta l'accordo selezionato
    void loadSong (const bottega::Song& s);

    std::function<void()> onExternalChange;

private:
    struct Pending { double when; bottega::Event e; double hz; };
    struct Off { double when; int channel; int note; int id; };
    std::vector<Pending> pending;
    std::vector<Off> offs;
    std::vector<juce::String> history;
    std::vector<std::vector<double>> voicings;
    bottega::Engine engine;
    bottega::ChannelAllocator alloc;
    std::atomic<bool> uiPlay { false }, playingNow { false }, hostSynced { false }, needDrone { true }, needPreview { false };
    std::atomic<double> playPos { -1 };
    std::atomic<int> previewSlot { 0 };
    bool wasPlaying = false, wasVinyl = false, internal = true;
    double nextCycleT = 0, sr = 44100;
    long cycleCount = 0;
    std::vector<std::pair<double, int>> cycleStarts;  // (tempo, ciclo) per la testina
    int melMidi[3] { -1, -1, -1 };                    // canale, nota, id della nota di melodia aperta
    juce::String lastTimbre;

    double hzOf (double cents) const { return song.hz * std::pow (2.0, cents / 1200.0); }
    void dispatch (const Pending& p, double blockStart, int numSamples, juce::MidiBuffer& midi);
    void startTransport (double now, const juce::AudioPlayHead::PositionInfo* pos);
    void stopTransport (juce::MidiBuffer& midi);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BottegaProcessor)
};
