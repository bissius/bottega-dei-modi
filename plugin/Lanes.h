#pragma once
// Widget dell'interfaccia in stile Teenage Engineering: monocromo, un colore per corsia, carattere monospazio.
#include "PluginProcessor.h"

namespace ui
{
namespace col
{
    const juce::Colour bg { 0xffE3E4E6 }, panel { 0xffF3F4F5 }, ink { 0xff111214 }, muted { 0xff666A70 }, line { 0xffC6C9CE }, soft { 0xffE9EAEC };
    const juce::Colour mel { 0xff1D54F0 }, arm { 0xffF0501A }, rit { 0xff0B9A5E }, warn { 0xffB98A00 };
}
juce::Font mono (float size, bool bold = false);

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return mono (11.5f, true); }
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return mono (12.5f); }
    juce::Font getPopupMenuFont() override { return mono (13.0f); }
    juce::Font getLabelFont (juce::Label& l) override { return l.getFont(); }
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
};

// gruppo di tasti a scelta singola (segmentato), va a capo se lo spazio non basta
class Seg : public juce::Component
{
public:
    std::function<void (const juce::String&)> onChange;
    void setOptions (const std::vector<std::pair<juce::String, juce::String>>& opts);
    void setValue (const juce::String& v);
    void setEnabledAll (bool e) { for (auto* b : btns) b->setEnabled (e); }
    int heightFor (int width) const;
    void resized() override;
private:
    juce::OwnedArray<juce::TextButton> btns;
    std::vector<juce::String> ids;
    juce::String value;
    int layout (int width, bool apply) const;
};

// paragrafo di testo che va a capo
class Para : public juce::Component
{
public:
    void setText (const juce::String& t, juce::Colour c = col::muted, float size = 12.0f, bool bold = false);
    int heightFor (int width) const;
    void paint (juce::Graphics&) override;
private:
    juce::AttributedString as;
};

// rullo del loop: accordi (cliccabili), melodia, basso, bordone, accenti del ritmo, testina
class Roll : public juce::Component
{
public:
    explicit Roll (BottegaProcessor& p) : proc (p) {}
    std::function<void (int)> onSelect;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    double head = -1;
private:
    BottegaProcessor& proc;
    std::vector<juce::Rectangle<float>> blocks;
};

// un ciclo ritmico: gruppi, segni, accenti, colpi per strumento, poliritmia
class RhythmGrid : public juce::Component
{
public:
    explicit RhythmGrid (BottegaProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    int preferredHeight() const;
    double head = -1;
private:
    BottegaProcessor& proc;
};
} // namespace ui
