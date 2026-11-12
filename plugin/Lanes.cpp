#include "Lanes.h"

using namespace bottega;

namespace ui
{
juce::Font mono (float size, bool bold)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, bold ? juce::Font::bold : juce::Font::plain));
}

// ---------------- LookAndFeel ----------------
LookAndFeel::LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::bg);
    setColour (juce::TextButton::buttonColourId, col::panel);
    setColour (juce::TextButton::buttonOnColourId, col::ink);
    setColour (juce::TextButton::textColourOffId, col::ink);
    setColour (juce::TextButton::textColourOnId, col::panel);
    setColour (juce::ComboBox::backgroundColourId, col::panel);
    setColour (juce::ComboBox::textColourId, col::ink);
    setColour (juce::ComboBox::outlineColourId, col::line);
    setColour (juce::ComboBox::arrowColourId, col::ink);
    setColour (juce::PopupMenu::backgroundColourId, col::panel);
    setColour (juce::PopupMenu::textColourId, col::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::ink);
    setColour (juce::PopupMenu::highlightedTextColourId, col::panel);
    setColour (juce::PopupMenu::headerTextColourId, col::muted);
    setColour (juce::Label::textColourId, col::ink);
    setColour (juce::TextEditor::backgroundColourId, col::panel);
    setColour (juce::TextEditor::textColourId, col::ink);
    setColour (juce::TextEditor::outlineColourId, col::line);
    setColour (juce::TextEditor::focusedOutlineColourId, col::mel);
    setColour (juce::Slider::thumbColourId, col::ink);
    setColour (juce::Slider::trackColourId, col::ink);
    setColour (juce::Slider::backgroundColourId, col::line);
    setColour (juce::Slider::textBoxTextColourId, col::ink);
    setColour (juce::Slider::textBoxOutlineColourId, col::line);
    setColour (juce::ScrollBar::thumbColourId, col::line);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.75f);
    bool on = b.getToggleState();
    g.setColour (on ? col::ink : (over || down ? col::soft : col::panel));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (b.isEnabled() ? col::ink : col::line);
    g.drawRoundedRectangle (r, 3.0f, 1.5f);
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (mono (b.getProperties().getWithDefault ("size", 11.0), true));
    g.setColour (b.getToggleState() ? col::panel : (b.isEnabled() ? col::ink : col::line));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 2), juce::Justification::centred, 3, 0.85f);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& cb)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.75f);
    g.setColour (col::panel); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (cb.hasKeyboardFocus (true) ? col::mel : col::line); g.drawRoundedRectangle (r, 3.0f, 1.5f);
    juce::Path p; float cx = w - 14.0f, cy = h * 0.5f;
    p.addTriangle (cx - 4, cy - 2, cx + 4, cy - 2, cx, cy + 3);
    g.setColour (col::ink); g.fillPath (p);
}

void LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    float cy = y + h * 0.5f;
    g.setColour (col::line); g.fillRoundedRectangle ((float) x, cy - 2, (float) w, 4, 2);
    g.setColour (s.isEnabled() ? col::ink : col::line); g.fillRoundedRectangle ((float) x, cy - 2, pos - x, 4, 2);
    g.fillEllipse (pos - 7, cy - 7, 14, 14);
}

// ---------------- Seg ----------------
void Seg::setOptions (const std::vector<std::pair<juce::String, juce::String>>& opts)
{
    bool same = opts.size() == ids.size();
    for (size_t i = 0; same && i < opts.size(); ++i) same = ids[i] == opts[i].first && btns[(int) i]->getButtonText() == opts[i].second;
    if (same) return;
    btns.clear(); ids.clear();
    for (auto& [id, txt] : opts)
    {
        auto* b = btns.add (new juce::TextButton (txt));
        ids.push_back (id);
        b->onClick = [this, id = id] { if (onChange) onChange (id); };
        addAndMakeVisible (b);
    }
    setValue (value);
    resized();
}
void Seg::setValue (const juce::String& v)
{
    value = v;
    for (int i = 0; i < btns.size(); ++i) btns[i]->setToggleState (ids[(size_t) i] == v, juce::dontSendNotification);
}
int Seg::layout (int width, bool apply) const
{
    int x = 0, y = 0, h = 28;
    for (auto* b : btns)
    {
        int w = (int) juce::GlyphArrangement::getStringWidthInt (mono (11.0f, true), b->getButtonText()) + 20;
        if (x > 0 && x + w > width) { x = 0; y += h + 4; }
        if (apply) b->setBounds (x, y, w, h);
        x += w - 1;
    }
    return y + h;
}
int Seg::heightFor (int width) const { return layout (width, false); }
void Seg::resized() { layout (getWidth(), true); }

// ---------------- Para ----------------
void Para::setText (const juce::String& t, juce::Colour c, float size, bool bold)
{
    as = {};
    as.setWordWrap (juce::AttributedString::byWord);
    as.append (t, mono (size, bold), c);
    repaint();
}
int Para::heightFor (int width) const
{
    juce::TextLayout tl; tl.createLayout (as, (float) std::max (50, width));
    return (int) std::ceil (tl.getHeight()) + 2;
}
void Para::paint (juce::Graphics& g) { as.draw (g, getLocalBounds().toFloat()); }

// ---------------- Roll ----------------
void Roll::paint (juce::Graphics& g)
{
    juce::ScopedLock sl (proc.lock);
    auto& th = proc.theory; auto& S = proc.song;
    const float W = (float) getWidth(), X0 = 44, XW = W - X0 - 6;
    const int L = th.loopLen(); const float cw = XW / L;
    auto& r = th.R(); const int P = r.pulsazioni, u = th.sub();
    g.fillAll (col::soft);
    g.setFont (mono (9.5f));
    for (int c = 0; c <= L; ++c) { g.setColour (col::line); g.fillRect (X0 + c * cw, 0.0f, c % 4 ? 1.0f : 2.0f, (float) getHeight()); }
    g.setColour (col::muted);
    for (int c = 0; c < L; ++c) g.drawText (juce::String (c + 1), (int) (X0 + c * cw + 3), 0, 30, 14, juce::Justification::left);
    auto label = [&] (const char* t, float y) { g.setColour (col::muted); g.drawText (t, 4, (int) y - 6, 38, 12, juce::Justification::left); };

    // accordi
    label ("ACC", 46);
    blocks.clear();
    auto st = th.starts();
    for (int i = 0; i < (int) S.slots.size(); ++i)
    {
        auto c = th.chordOf (i);
        juce::Rectangle<float> b (X0 + (float) st[(size_t) i] * cw + 2, 16, (float) S.slots[(size_t) i].dur * cw - 4, 58);
        blocks.push_back (b);
        bool on = i == S.sel;
        g.setColour (col::arm.withAlpha (S.armo == "bordone" ? 0.35f : on ? 1.0f : 0.8f));
        g.fillRoundedRectangle (b, 3);
        if (on) { g.setColour (col::ink); g.drawRoundedRectangle (b, 3, 3); }
        g.setColour (juce::Colours::white);
        bool big = b.getWidth() > 110;
        g.setFont (mono (big ? 15.0f : b.getWidth() > 60 ? 11.5f : 9.0f, true));
        g.drawText (th.sym (c), b.reduced (6, 2).withHeight (22), juce::Justification::left, true);
        if (b.getWidth() > 40) { g.setFont (mono (big ? 10.5f : 9.0f)); g.drawText (th.roman (c), b.reduced (6, 0).withY (b.getY() + 24).withHeight (14), juce::Justification::left, true); }
        if (b.getWidth() > 90) { g.setFont (mono (9.5f)); g.drawText (th.funzione (c), b.reduced (6, 0).withY (b.getY() + 39).withHeight (14), juce::Justification::left, true); }
        if (S.slots[(size_t) i].tag.isNotEmpty() && b.getWidth() > 24) g.fillEllipse (b.getRight() - 11, b.getY() + 4, 7, 7);
    }

    // melodia
    std::vector<const MelNote*> notes;
    for (auto& f : S.frasi) for (auto& n : f) notes.push_back (&n);
    if (! notes.empty() && S.melOn)
    {
        double lo = 1e9, hi = -1e9;
        for (auto* n : notes) { double c = th.melCents (*n); lo = std::min (lo, c); hi = std::max (hi, c); }
        lo -= 100; hi += 100;
        label ("MEL", 134);
        for (auto* n : notes)
        {
            double c = th.melCents (*n);
            float x = X0 + (float) (n->c + n->u / (double) (P * u)) * cw + 1, w = std::max (2.0f, (float) (n->len / (double) (P * u)) * cw - 2);
            float y = (float) (182 - (c - lo) / (hi - lo) * 96);
            g.setColour (col::mel); g.fillRoundedRectangle (x, y - 4, w, 8, 2);
            double fr = std::abs (bottega::mod (th.degC (th.modeOf (n->mo), n->deg), 100));
            if (fr > 20 && fr < 80) { g.setColour (col::ink); g.drawRoundedRectangle (x, y - 4, w, 8, 2, 1.3f); }
        }
    }

    // basso e bordone
    label ("BAS", 222);
    g.setColour (col::ink);
    for (int c = 0; c < L; ++c)
        for (auto& b : th.bassNotes (c))
        {
            float x = X0 + (float) (c + b.p / P) * cw + 1, w = std::max (2.0f, (float) (b.len / P) * cw - 2);
            float y = juce::jlimit (196.0f, 244.0f, (float) (236 - (b.cents + 2100) / 1800 * 40));
            g.fillRect (x, y - 2.5f, w, 5.0f);
        }
    label ("BOR", 254);
    g.setColour (col::arm.withAlpha (0.35f)); g.fillRoundedRectangle (X0, 252, XW, 4, 2);

    // accenti
    label ("RIT", 290);
    for (int c = 0; c < L; ++c)
    {
        for (int p = 0; p < P; ++p)
        {
            int a = r.accenti[(size_t) p];
            float x = X0 + (float) (c + (p + 0.5) / P) * cw;
            g.setColour (col::rit.withAlpha (a ? 1.0f : 0.35f));
            g.fillRect (x - 1, 300.0f - 6 - a * 7, 2.0f, 6.0f + a * 7);
        }
        if (c < (int) S.ritVar.size() && S.ritVar[(size_t) c]) { g.setColour (col::rit); g.setFont (mono (9.0f)); g.drawText ("var", (int) (X0 + c * cw + 3), 308, 30, 12, juce::Justification::left); }
    }
    if (head >= 0) { g.setColour (col::ink); g.fillRect (X0 + (float) head * cw - 1, 0.0f, 2.0f, (float) getHeight()); }
}

void Roll::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < (int) blocks.size(); ++i)
        if (blocks[(size_t) i].contains (e.position)) { if (onSelect) onSelect (i); return; }
}

// ---------------- RhythmGrid ----------------
static const std::pair<const char*, const char*> ROWS[] = { { "kick", "Cassa" }, { "snare", "Rull." }, { "ghost", "Ghost" }, { "hat", "Charl." }, { "dafDum", "Dum" }, { "dafTek", "Tek" }, { "shaker", "Sonag." }, { "bell", "Camp." } };

int RhythmGrid::preferredHeight() const
{
    juce::ScopedLock sl (proc.lock);
    auto tr = proc.theory.baseTracks(); int rows = 0;
    for (auto& [k, l] : ROWS) if (tr.count (k)) ++rows;
    return 62 + rows * 16 + (proc.song.strato ? 16 : 0) + 6;
}

void RhythmGrid::paint (juce::Graphics& g)
{
    juce::ScopedLock sl (proc.lock);
    auto& th = proc.theory; auto& r = th.R();
    const int P = r.pulsazioni; const float X0 = 52, cw = (getWidth() - X0 - 6) / (float) P, H = (float) getHeight();
    g.fillAll (col::soft);
    float a = 0; int j = 0;
    for (int gsz : r.gruppi) { if (j++ % 2) { g.setColour (col::panel.withAlpha (0.6f)); g.fillRect (X0 + a * cw, 0.0f, gsz * cw, H); } a += gsz; }
    g.setFont (mono (9.5f));
    for (int p = 0; p < P; ++p)
    {
        float x = X0 + p * cw; int ac = r.accenti[(size_t) p];
        g.setColour (col::line); g.fillRect (x, 0.0f, 1.0f, H);
        if (p < r.segni.size()) { g.setColour (col::muted); g.drawText (r.segni[p], (int) x + 3, 1, (int) cw + 30, 12, juce::Justification::left); }
        g.setColour (col::rit.withAlpha (ac ? 1.0f : 0.3f));
        g.fillRoundedRectangle (x + cw * 0.2f, 44.0f - ac * 8, cw * 0.6f, 4.0f + ac * 8, 1);
    }
    g.setColour (col::muted); g.drawText ("ACC", 4, 34, 40, 12, juce::Justification::left);
    auto tr = th.baseTracks(); int row = 0;
    for (auto& [k, l] : ROWS)
    {
        auto it = tr.find (k); if (it == tr.end()) continue;
        float y = 62.0f + row++ * 16;
        g.setColour (col::muted); g.drawText (l, 4, (int) y - 6, 46, 12, juce::Justification::left);
        g.setColour (col::rit);
        for (auto& h : it->second) { float rr = 2 + h.vel * 4; g.fillEllipse (X0 + (h.step + 0.5f) * cw - rr, y - rr, rr * 2, rr * 2); }
    }
    if (proc.song.strato)
    {
        float y = 62.0f + row * 16;
        g.setColour (col::muted); g.drawText (juce::String (proc.song.strato) + ":" + juce::String (P), 4, (int) y - 6, 46, 12, juce::Justification::left);
        g.setColour (col::ink);
        for (int k = 0; k < proc.song.strato; ++k)
        {
            float x = X0 + (float) k * P / proc.song.strato * cw;
            juce::Path d; d.addRectangle (-3.5f, -3.5f, 7.0f, 7.0f);
            d.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::pi / 4).translated (x, y));
            g.fillPath (d);
        }
    }
    if (head >= 0) { g.setColour (col::ink); g.fillRect (X0 + (float) head * P * cw - 1, 0.0f, 2.0f, H); }
}
} // namespace ui
