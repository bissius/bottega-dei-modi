#pragma once
// Interfaccia: testata fissa (trasporto, tonica, tempo, caso, preset, uscita) e tre corsie scorrevoli.
#include "Lanes.h"

class BottegaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BottegaEditor (BottegaProcessor&);
    ~BottegaEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Content : juce::Component
    {
        std::function<void (juce::Graphics&)> painter;
        void paint (juce::Graphics& g) override { if (painter) painter (g); }
    };

    BottegaProcessor& proc;
    bottega::Theory& th;
    bottega::Song& S;
    ui::LookAndFeel laf;

    // testata
    juce::TextButton play { juce::String::fromUTF8 ("▶") }, undoB { "Annulla" }, tutto { "Sorprendimi" }, salva { "Salva preset" }, apri { "Apri preset" };
    juce::ComboBox ton;
    juce::TextEditor hz, bpm;
    juce::Slider caso;
    juce::Label casoTxt, status, title, sync;
    ui::Seg uscita, bend;

    // corsie nella vista scorrevole
    juce::Viewport view;
    Content content;

    struct Lane { juce::Label h; juce::TextButton var { "Variazione" }, lock { "Blocca" }; juce::Colour c; juce::Rectangle<int> box; };
    Lane arm, rit, mel, mix;

    // armonia
    juce::ComboBox modo, giro, mossaModo;
    juce::Slider koron;
    juce::Label koronLab;
    ui::Para modoTxt, selPos, selHead, selVoc, selSpec, selConf, optTxt, mosseCap;
    ui::Roll roll;
    ui::Seg edDeg, edExt, edDur, edBass, oArmo, oVoc, oComp, oSuono, oBasso;
    juce::TextButton edPrev { juce::String::fromUTF8 ("◀") }, edNext { juce::String::fromUTF8 ("▶") }, edAdd { "+ Aggiungi" }, edDel { juce::String::fromUTF8 ("− Togli") }, edHear { "Ascolta" };
    juce::OwnedArray<juce::TextButton> mosse;

    // ritmo
    juce::ComboBox ritmo;
    ui::Seg kit, strato;
    juce::Slider swing;
    juce::Label swingLab;
    ui::Para ritTxt;
    ui::RhythmGrid grid;

    // melodia
    ui::Seg melOn, timbro, dens;
    ui::Para melTxt;

    // mixer
    struct Ch { juce::String k; juce::Label l; juce::Slider s; juce::TextButton m { "Muto" }; };
    juce::OwnedArray<Ch> chans;
    juce::TextButton lofi { "Lo-fi" };

    // etichette dei controlli (testo sopra il controllo)
    juce::OwnedArray<juce::Label> caps;
    std::map<juce::Component*, juce::Label*> capOf;

    std::vector<juce::String> modeIds, ritIds, giroIds;
    std::unique_ptr<juce::FileChooser> chooser;

    void cap (juce::Component& c, const juce::String& text);
    void setupLane (Lane& l, const juce::String& name, juce::Colour c);
    void fillCombos();
    void fillGiri();
    void edit (bool regen, const std::function<void()>& fn, bool snap = true);
    void refreshAll();
    void renderTop();
    void renderEditor();
    void setStatus (const juce::String& t) { status.setText (t, juce::dontSendNotification); }
    int layoutContent (int width);
    void timerCallback() override;
    void savePreset();
    void openPreset();
};
