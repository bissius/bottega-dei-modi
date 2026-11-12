#include "PluginEditor.h"

using namespace bottega;
using ui::col::bg;

namespace
{
juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

struct Tonica { const char* en; const char* it; double hz; };
const Tonica TON[] = { { "C", "Do", 130.81 }, { "D♭", "Re♭", 138.59 }, { "D", "Re", 146.83 }, { "E♭", "Mi♭", 155.56 },
                       { "E", "Mi", 164.81 }, { "F", "Fa", 174.61 }, { "F♯", "Fa♯", 185.00 }, { "G", "Sol", 196.00 },
                       { "A♭", "La♭", 207.65 }, { "A", "La", 110.00 }, { "B♭", "Si♭", 116.54 }, { "B", "Si", 123.47 } };
const char* ROM[] = { "I", "II", "III", "IV", "V", "VI", "VII" };

struct Timbro { const char* id; const char* nome; const char* testo; };
const Timbro TIMBRI[] = {
    { "arco", "Arco", "Voce ad arco ispirata al kamancheh: corde sfregate, formanti del corpo, rumore d'archetto." },
    { "ney", "Ney", "Flauto di canna con molto soffio: lo strumento classico di maqam e dastgah." },
    { "synth", "Synth", "Lead elettronico filtrato: stessi microtoni, colore moderno." } };

const std::map<juce::String, const char*> TXT_ARMO {
    { "terze", "Accordi per terze presi dal modo, con le estensioni scelte." },
    { "aperte", "Solo radice e quinta raddoppiate: reggono anche fuori dal temperamento e lasciano il colore alla melodia." },
    { "bordone", "Nessun accordo: bordone e basso, come nella pratica tradizionale. L'analisi resta visibile." } };
const std::map<juce::String, const char*> TXT_VOC {
    { "stretto", "Voicing stretto con condotta delle voci: ogni accordo si muove il meno possibile dal precedente." },
    { "aperto", "Drop 2: la seconda voce dall'alto scende di un'ottava, suono più largo da piano jazz." },
    { "shell", "Solo terza, settima e l'estensione in cima: il voicing essenziale del jazz." },
    { "quartale", "Voci per quarte prese dal modo: il suono modale alla McCoy Tyner." } };
const std::map<juce::String, const char*> TXT_BASSO {
    { "radice", "Basso sulla radice." },
    { "passaggi", "Basso sulla radice, con una nota di passaggio (grado di scala o semitono) prima di ogni cambio." },
    { "pedale", "Basso sempre sulla tonica." } };

const char* CH[][2] = { { "mel", "Melodia" }, { "acc", "Accordi" }, { "bas", "Basso" }, { "bor", "Bordone" }, { "bat", "Batteria" }, { "vin", "Vinile" }, { "rev", "Riverbero" } };

juce::String durTxt (double d) { return juce::String (d, std::floor (d) == d ? 0 : 1); }

// disposizione a flusso: i controlli si affiancano finché c'è spazio, poi vanno a capo
struct Item { juce::Component* c; int w; bool flex = false; int h = -1; };

int itemHeight (const Item& it, int w)
{
    if (it.h >= 0) return it.h;
    if (auto* s = dynamic_cast<ui::Seg*> (it.c)) return s->heightFor (w);
    if (auto* p = dynamic_cast<ui::Para*> (it.c)) return p->heightFor (w);
    return 28;
}

int flow (const std::vector<Item>& all, const std::map<juce::Component*, juce::Label*>& caps, int x, int y, int W, int gap = 12, int vgap = 10)
{
    std::vector<Item> items;   // i controlli nascosti non occupano spazio
    for (auto& it : all) if (it.c->isVisible()) items.push_back (it);
    size_t i = 0;
    while (i < items.size())
    {
        // riempi una riga
        size_t j = i; int used = 0; int nflex = 0;
        while (j < items.size())
        {
            int w = std::min (items[j].w, W);
            int need = used + (j > i ? gap : 0) + w;
            if (j > i && need > W) break;
            used = need; if (items[j].flex) ++nflex; ++j;
        }
        int extra = std::max (0, W - used);
        int cx = x, lineH = 0;
        for (size_t k = i; k < j; ++k)
        {
            auto& it = items[k];
            int w = std::min (it.w, W) + (it.flex && nflex > 0 ? extra / nflex : 0);
            auto cit = caps.find (it.c);
            int capH = (cit != caps.end() && cit->second->isVisible()) ? 16 : 0;
            int h = itemHeight (it, w);
            if (capH) cit->second->setBounds (cx, y, w, capH);
            it.c->setBounds (cx, y + capH, w, h);
            lineH = std::max (lineH, capH + h);
            cx += w + gap;
        }
        y += lineH + vgap;
        i = j;
    }
    return y;
}
} // namespace

// =====================================================================================
BottegaEditor::BottegaEditor (BottegaProcessor& p)
    : AudioProcessorEditor (p), proc (p), th (p.theory), S (p.song), roll (p), grid (p)
{
    setLookAndFeel (&laf);
    auto label = [this] (juce::Label& l, const juce::String& t, float size, bool bold, juce::Colour c)
    {
        l.setText (t, juce::dontSendNotification); l.setFont (ui::mono (size, bold)); l.setColour (juce::Label::textColourId, c);
        l.setBorderSize ({ 0, 0, 0, 0 });
    };

    // ---------------- testata ----------------
    label (title, u8 ("BOTTEGA DEI MODI  ·  accordi · ritmo · melodia"), 13.0f, true, ui::col::ink);
    label (status, {}, 11.5f, false, ui::col::muted);
    label (sync, {}, 10.5f, true, ui::col::rit);
    label (casoTxt, {}, 11.0f, true, ui::col::ink);
    for (auto* c : std::initializer_list<juce::Component*> { &title, &status, &sync, &play, &ton, &hz, &bpm, &caso, &undoB, &tutto, &salva, &apri, &uscita, &bend })
        addAndMakeVisible (c);
    addAndMakeVisible (view);
    view.setViewedComponent (&content, false);
    view.setScrollBarsShown (true, false);
    view.setScrollBarThickness (8);

    play.getProperties().set ("size", 16.0);
    play.onClick = [this] { proc.setPlaying (! proc.uiPlaying()); };

    cap (ton, "Tonica");
    for (int i = 0; i < 12; ++i) ton.addItem (u8 (TON[i].it) + " (" + u8 (TON[i].en) + ")", i + 1);
    ton.onChange = [this]
    {
        int t = ton.getSelectedId() - 1;
        if (t < 0 || t == S.ton) return;
        edit (false, [this, t] { S.ton = t; S.hz = TON[t].hz; });
    };

    for (auto* te : { &hz, &bpm })
    {
        te->setFont (ui::mono (12.5f)); te->setJustification (juce::Justification::centredLeft);
        te->setInputRestrictions (7, "0123456789.");
        te->setIndents (6, 6);
    }
    cap (hz, "Hz tonica"); cap (bpm, "BPM");
    auto commitHz = [this]
    {
        double v = hz.getText().getDoubleValue();
        if (v >= 80 && v <= 300 && std::abs (v - S.hz) > 1e-3) edit (false, [this, v] { S.hz = v; });
        else renderTop();
    };
    hz.onReturnKey = commitHz; hz.onFocusLost = commitHz;
    auto commitBpm = [this]
    {
        double v = juce::jlimit (40.0, 200.0, bpm.getText().getDoubleValue());
        if (v > 0 && std::abs (v - S.bpm) > 1e-3) edit (false, [this, v] { S.bpm = v; }, false);
        else renderTop();
    };
    bpm.onReturnKey = commitBpm; bpm.onFocusLost = commitBpm;

    caso.setSliderStyle (juce::Slider::LinearHorizontal);
    caso.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    caso.setRange (0, 100, 1);
    cap (caso, "Manopola del caso");
    caso.onValueChange = [this]
    {
        { juce::ScopedLock sl (proc.lock); S.caso = (int) caso.getValue(); }
        casoTxt.setText (S.caso < 20 ? "prudente" : S.caso < 45 ? "moderato" : S.caso < 75 ? "curioso" : "selvaggio", juce::dontSendNotification);
    };
    addAndMakeVisible (casoTxt);

    undoB.onClick = [this] { if (proc.undo()) { refreshAll(); setStatus ("Annullato."); } };
    tutto.onClick = [this]
    {
        juce::String msg;
        edit (true, [this, &msg] { msg = th.sorprendimi(); });
        setStatus (msg);
    };
    salva.onClick = [this] { savePreset(); };
    apri.onClick = [this] { openPreset(); };

    cap (uscita, "Uscita");
    uscita.setOptions ({ { "int", "Suono interno" }, { "midi", "MIDI" }, { "both", "Entrambi" } });
    uscita.onChange = [this] (const juce::String& v)
    {
        { juce::ScopedLock sl (proc.lock); S.suonoInterno = v != "midi"; S.midiOut = v != "int"; }
        renderTop();
        setStatus (v == "int" ? "Solo suono interno." : u8 ("MIDI attivo: ogni nota ha il suo canale (2–16, batteria sul 10) e il suo pitch bend per i microtoni."));
    };
    cap (bend, "Pitch bend del synth");
    bend.setOptions ({ { "2", u8 ("±2") }, { "12", u8 ("±12") }, { "24", u8 ("±24") }, { "48", u8 ("±48 MPE") } });
    bend.onChange = [this] (const juce::String& v)
    {
        { juce::ScopedLock sl (proc.lock); S.pitchBendRange = v.getIntValue(); }
        setStatus ("Imposta lo stesso intervallo di pitch bend (" + v + " semitoni) sul synth che riceve il MIDI.");
    };

    // ---------------- corsie ----------------
    setupLane (arm, "ARMONIA", ui::col::arm);
    setupLane (rit, "RITMO", ui::col::rit);
    setupLane (mel, "MELODIA", ui::col::mel);
    setupLane (mix, "MIXER", ui::col::muted);
    mix.var.setVisible (false); mix.lock.setVisible (false);

    content.painter = [this] (juce::Graphics& g)
    {
        for (auto* l : { &arm, &rit, &mel, &mix })
        {
            auto r = l->box.toFloat();
            g.setColour (ui::col::panel); g.fillRoundedRectangle (r, 4.0f);
            g.setColour (ui::col::line); g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
            g.setColour (l->c); g.fillRect (r.removeFromTop (4.0f).reduced (2.0f, 0));
        }
    };

    // armonia
    for (auto* c : std::initializer_list<juce::Component*> { &modo, &giro, &koron, &modoTxt, &roll, &selPos, &selHead, &selVoc, &selSpec, &selConf,
                                                              &edDeg, &edExt, &edDur, &edBass, &edPrev, &edNext, &edAdd, &edDel, &edHear,
                                                              &mosseCap, &mossaModo, &oArmo, &oVoc, &oComp, &oSuono, &oBasso, &optTxt })
        content.addAndMakeVisible (c);
    cap (modo, "Modo"); cap (giro, "Giro di partenza"); cap (koron, "Koron"); cap (mossaModo, "Cambio di scala verso");
    cap (edDeg, "Grado"); cap (edExt, "Estensione"); cap (edDur, "Durata (cicli)"); cap (edBass, "Basso");
    cap (oArmo, "Armonizzazione"); cap (oVoc, "Voicing"); cap (oComp, "Accompagnamento"); cap (oSuono, "Suono"); cap (oBasso, "Linea di basso");
    mosseCap.setText ("MOSSE SULL'ACCORDO SELEZIONATO", ui::col::muted, 10.5f, true);

    arm.var.onClick = [this]
    {
        if (S.armLock) { setStatus ("Armonia bloccata."); return; }
        juce::String m; edit (true, [this, &m] { m = th.varArm(); S.giro = "modificato"; }); setStatus (m);
    };
    arm.lock.onClick = [this] { { juce::ScopedLock sl (proc.lock); S.armLock = ! S.armLock; } renderTop(); renderEditor(); };

    modo.onChange = [this]
    {
        int k = modo.getSelectedId() - 1;
        if (k < 0 || k >= (int) modeIds.size() || modeIds[(size_t) k] == S.modo) return;
        auto id = modeIds[(size_t) k];
        edit (true, [this, id]
        {
            S.modo = id;
            if (! S.armLock) { if (! S.giro.startsWith ("r:")) S.giro = "r:pedale"; th.genArm(); }
            for (auto& s : S.slots) if (! s.explicitIv) s.modo.clear();
        });
    };
    giro.onChange = [this]
    {
        int k = giro.getSelectedId() - 1;
        if (k < 0 || k >= (int) giroIds.size() || giroIds[(size_t) k] == S.giro) return;
        if (S.armLock) { setStatus ("Armonia bloccata: sbloccala per cambiare giro."); renderTop(); return; }
        auto id = giroIds[(size_t) k];
        edit (true, [this, id] { th.genArm (id); });
    };

    koron.setSliderStyle (juce::Slider::LinearHorizontal);
    koron.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
    koron.setTextValueSuffix (" c");
    koron.onDragStart = [this] { proc.snapshot(); };
    koron.onValueChange = [this]
    {
        { juce::ScopedLock sl (proc.lock); if (std::abs (S.koron[S.modo] - koron.getValue()) < 0.5) return; S.koron[S.modo] = koron.getValue(); }
        proc.changed (false);
        roll.repaint(); renderEditor();
    };

    roll.onSelect = [this] (int i) { { juce::ScopedLock sl (proc.lock); S.sel = i; } roll.repaint(); renderEditor(); proc.preview (i); };

    edDeg.onChange = [this] (const juce::String& v) { edit (false, [this, v] { S.slots[(size_t) S.sel].deg = v.getIntValue(); S.giro = "modificato"; }); };
    edExt.onChange = [this] (const juce::String& v) { edit (false, [this, v] { S.slots[(size_t) S.sel].ext = v; S.giro = "modificato"; }); };
    edDur.onChange = [this] (const juce::String& v) { edit (true, [this, v] { S.slots[(size_t) S.sel].dur = v.getDoubleValue(); S.giro = "modificato"; th.fixLen(); }); };
    edBass.onChange = [this] (const juce::String& v) { edit (false, [this, v] { S.slots[(size_t) S.sel].bass = v; }); };
    edDur.setOptions ({ { "0.5", u8 ("½") }, { "1", "1" }, { "2", "2" }, { "3", "3" }, { "4", "4" } });
    edBass.setOptions ({ { "radice", "Radice" }, { "rivolto", "Rivolto" }, { "pedale", "Pedale" } });
    {
        std::vector<std::pair<juce::String, juce::String>> e;
        for (auto& x : extensions()) e.push_back ({ x.id, u8 (x.nome) });
        edExt.setOptions (e);
    }
    auto step = [this] (int d)
    {
        { juce::ScopedLock sl (proc.lock); S.sel = imod (S.sel + d, (int) S.slots.size()); }
        roll.repaint(); renderEditor();
    };
    edPrev.onClick = [step] { step (-1); };
    edNext.onClick = [step] { step (1); };
    edAdd.onClick = [this]
    {
        edit (true, [this]
        {
            auto& sl = S.slots[(size_t) S.sel];
            Slot ns;
            ns.deg = sl.explicitIv ? 0 : imod (sl.deg + 3, 7);
            ns.ext = sl.explicitIv ? juce::String ("7") : sl.ext;
            ns.t = sl.t;
            if (sl.modo.isNotEmpty() && ! sl.explicitIv) ns.modo = sl.modo;
            S.slots.insert (S.slots.begin() + S.sel + 1, ns);
            ++S.sel; S.giro = "modificato";
        });
    };
    edDel.onClick = [this]
    {
        if (S.slots.size() <= 1) return;
        edit (true, [this]
        {
            S.slots.erase (S.slots.begin() + S.sel);
            S.sel = std::min (S.sel, (int) S.slots.size() - 1); S.giro = "modificato";
        });
    };
    edHear.onClick = [this] { proc.preview (S.sel); };

    for (auto& m : th.A.mosse)
    {
        auto* b = mosse.add (new juce::TextButton (m.nome + "\n" + m.stile));
        b->setTooltip (m.testo);
        b->getProperties().set ("size", 10.5);
        auto id = m.id;
        b->onClick = [this, id]
        {
            if (S.armLock) { setStatus ("Armonia bloccata."); return; }
            juce::String verso, msg;
            int k = mossaModo.getSelectedId() - 1;
            if (k >= 0 && k < (int) modeIds.size()) verso = modeIds[(size_t) k];
            edit (true, [this, id, verso, &msg] { msg = th.mossa (id, verso); S.giro = "modificato"; });
            setStatus (msg);
        };
        content.addAndMakeVisible (b);
    }

    auto optSeg = [this] (ui::Seg& s, std::vector<std::pair<juce::String, juce::String>> o, juce::String Song::* field)
    {
        s.setOptions (o);
        s.onChange = [this, field] (const juce::String& v) { edit (false, [this, field, v] { S.*field = v; }); };
    };
    optSeg (oArmo, { { "terze", "Terze" }, { "aperte", "Quinte aperte" }, { "bordone", "Solo bordone" } }, &Song::armo);
    optSeg (oVoc, { { "stretto", "Stretto" }, { "aperto", "Aperto" }, { "shell", "Shell" }, { "quartale", "Quartale" } }, &Song::voc);
    optSeg (oComp, { { "auto", "Dal ritmo" }, { "tenuto", "Tenuto" }, { "accenti", "Sugli accenti" }, { "arpeggio", "Arpeggio" } }, &Song::comp);
    optSeg (oSuono, { { "rp", "Rhodes + pad" }, { "rhodes", "Rhodes" }, { "pad", "Pad" } }, &Song::suono);
    optSeg (oBasso, { { "radice", "Radice" }, { "passaggi", "Con passaggi" }, { "pedale", "Pedale" } }, &Song::basso);
    optSeg (kit, { { "auto", "Auto" }, { "elettronico", "Elettronico" }, { "daf", "Daf" } }, &Song::kit);

    // ritmo
    for (auto* c : std::initializer_list<juce::Component*> { &ritmo, &kit, &strato, &swing, &ritTxt, &grid })
        content.addAndMakeVisible (c);
    cap (ritmo, "Ciclo"); cap (kit, "Kit"); cap (strato, "Poliritmia (colpi sul ciclo)"); cap (swing, "Swing");
    rit.var.onClick = [this]
    {
        if (S.ritLock) { setStatus ("Ritmo bloccato."); return; }
        juce::String m; edit (false, [this, &m] { m = th.varRit(); }); setStatus (m);
    };
    rit.lock.onClick = [this] { { juce::ScopedLock sl (proc.lock); S.ritLock = ! S.ritLock; } renderTop(); };
    ritmo.onChange = [this]
    {
        int k = ritmo.getSelectedId() - 1;
        if (k < 0 || k >= (int) ritIds.size() || ritIds[(size_t) k] == S.rit) return;
        if (S.ritLock) { setStatus ("Ritmo bloccato: sbloccalo per cambiare ciclo."); renderTop(); return; }
        auto id = ritIds[(size_t) k];
        edit (true, [this, id]
        {
            S.rit = id; S.bpm = th.R().bpm; S.swing = -1;
            if (! S.armLock && S.giro != "modificato") th.genArm();
            else for (auto& s : S.slots) s.dur = std::max (0.5, (th.R().pulsazioni % 2 != 0 && std::fmod (s.dur, 1.0) != 0) ? std::ceil (s.dur) : s.dur);
            th.genRit();
            if (! S.melLock) th.genMel();
        });
    };
    strato.setOptions ({ { "0", "No" }, { "2", "2" }, { "3", "3" }, { "4", "4" }, { "5", "5" } });
    strato.onChange = [this] (const juce::String& v) { edit (false, [this, v] { S.strato = v.getIntValue(); }); };
    swing.setSliderStyle (juce::Slider::LinearHorizontal);
    swing.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 22);
    swing.setRange (50, 68, 1);
    swing.setTextValueSuffix ("%");
    swing.onValueChange = [this]
    {
        juce::ScopedLock sl (proc.lock);
        double v = swing.getValue() / 100.0;
        if (std::abs (v - th.swingV()) > 1e-4) S.swing = v;
    };

    // melodia
    for (auto* c : std::initializer_list<juce::Component*> { &melOn, &timbro, &dens, &melTxt })
        content.addAndMakeVisible (c);
    cap (melOn, "Melodia"); cap (timbro, "Timbro"); cap (dens, u8 ("Densità"));
    mel.var.onClick = [this]
    {
        if (S.melLock) { setStatus ("Melodia bloccata."); return; }
        juce::String m; edit (false, [this, &m] { m = th.varMel(); }); setStatus (m);
    };
    mel.lock.onClick = [this] { { juce::ScopedLock sl (proc.lock); S.melLock = ! S.melLock; } renderTop(); };
    melOn.setOptions ({ { "on", "Accesa" }, { "off", "Spenta" } });
    melOn.onChange = [this] (const juce::String& v) { edit (false, [this, v] { S.melOn = v == "on"; }); };
    {
        std::vector<std::pair<juce::String, juce::String>> o;
        for (auto& t : TIMBRI) o.push_back ({ t.id, t.nome });
        timbro.setOptions (o);
    }
    timbro.onChange = [this] (const juce::String& v) { edit (false, [this, v] { S.timbro = v; }); };
    dens.setOptions ({ { "sparsa", "Sparsa" }, { "media", "Media" }, { "fitta", "Fitta" } });
    dens.onChange = [this] (const juce::String& v) { edit (false, [this, v] { S.dens = v; if (! S.melLock) th.genMel(); }); };

    // mixer
    for (auto& c : CH)
    {
        auto* ch = chans.add (new Ch());
        ch->k = c[0];
        label (ch->l, juce::String (c[1]).toUpperCase(), 10.5f, true, ui::col::muted);
        ch->s.setSliderStyle (juce::Slider::LinearHorizontal);
        ch->s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        ch->s.setRange (0, 1, 0.01);
        ch->s.onValueChange = [this, ch] { juce::ScopedLock sl (proc.lock); S.mix[ch->k] = (float) ch->s.getValue(); };
        ch->m.onClick = [this, ch] { { juce::ScopedLock sl (proc.lock); S.mute[ch->k] = ! S.mute[ch->k]; } renderTop(); };
        content.addAndMakeVisible (ch->l); content.addAndMakeVisible (ch->s);
        if (ch->k != "rev") content.addAndMakeVisible (ch->m);
    }
    lofi.onClick = [this] { { juce::ScopedLock sl (proc.lock); S.lofi = ! S.lofi; } renderTop(); };
    content.addAndMakeVisible (lofi);

    fillCombos();
    proc.onExternalChange = [this] { refreshAll(); };
    refreshAll();
    setStatus ("Premi Play, oppure avvia il trasporto di Studio One: il loop segue il tempo della DAW.");

    setResizable (true, true);
    setResizeLimits (420, 480, 2400, 2400);
    setSize (1060, 860);
    startTimerHz (30);
}

BottegaEditor::~BottegaEditor()
{
    stopTimer();
    proc.onExternalChange = nullptr;
    setLookAndFeel (nullptr);
}

void BottegaEditor::cap (juce::Component& c, const juce::String& text)
{
    auto* l = caps.add (new juce::Label());
    l->setText (text.toUpperCase(), juce::dontSendNotification);
    l->setFont (ui::mono (10.0f, true));
    l->setColour (juce::Label::textColourId, ui::col::muted);
    l->setBorderSize ({ 0, 0, 0, 0 });
    l->setInterceptsMouseClicks (false, false);
    capOf[&c] = l;
    // le etichette della testata stanno nell'editor, le altre nella vista scorrevole
    bool top = &c == &ton || &c == &hz || &c == &bpm || &c == &caso || &c == &uscita || &c == &bend;
    if (top) addAndMakeVisible (l); else content.addAndMakeVisible (l);
}

void BottegaEditor::setupLane (Lane& l, const juce::String& name, juce::Colour c)
{
    l.c = c;
    l.h.setText (name, juce::dontSendNotification);
    l.h.setFont (ui::mono (15.0f, true));
    l.h.setColour (juce::Label::textColourId, c == ui::col::muted ? ui::col::muted : c);
    l.h.setBorderSize ({ 0, 0, 0, 0 });
    content.addAndMakeVisible (l.h);
    content.addAndMakeVisible (l.var);
    content.addAndMakeVisible (l.lock);
}

void BottegaEditor::fillCombos()
{
    auto& A = th.A;
    modo.clear (juce::dontSendNotification); mossaModo.clear (juce::dontSendNotification); modeIds.clear();
    std::vector<juce::String> culture;
    for (auto& m : A.modi) if (std::find (culture.begin(), culture.end(), m.cultura) == culture.end()) culture.push_back (m.cultura);
    for (auto& cu : culture)
    {
        modo.addSectionHeading (cu); mossaModo.addSectionHeading (cu);
        for (auto& m : A.modi)
            if (m.cultura == cu)
            {
                modeIds.push_back (m.id);
                modo.addItem (m.nome, (int) modeIds.size());
                mossaModo.addItem (m.nome, (int) modeIds.size());
            }
    }
    for (size_t i = 0; i < modeIds.size(); ++i) if (modeIds[i] == "bayati") mossaModo.setSelectedId ((int) i + 1, juce::dontSendNotification);

    ritmo.clear (juce::dontSendNotification); ritIds.clear();
    std::vector<juce::String> rc;
    for (auto& r : A.ritmi) if (std::find (rc.begin(), rc.end(), r.cultura) == rc.end()) rc.push_back (r.cultura);
    for (auto& cu : rc)
    {
        ritmo.addSectionHeading (cu);
        for (auto& r : A.ritmi)
            if (r.cultura == cu) { ritIds.push_back (r.id); ritmo.addItem (r.nome + u8 (" · ") + juce::String (r.pulsazioni), (int) ritIds.size()); }
    }
}

void BottegaEditor::fillGiri()
{
    giro.clear (juce::dontSendNotification); giroIds.clear();
    bool headI = false, headR = false, found = false;
    for (auto& [id, nome] : th.giriList())
    {
        if (id.startsWith ("i:") && ! headI) { giro.addSectionHeading ("Giri del modo"); headI = true; }
        if (id.startsWith ("r:") && ! headR) { giro.addSectionHeading ("Accordi ricchi"); headR = true; }
        giroIds.push_back (id); giro.addItem (nome, (int) giroIds.size());
        if (id == S.giro) { giro.setSelectedId ((int) giroIds.size(), juce::dontSendNotification); found = true; }
    }
    if (! found)
    {
        giroIds.push_back (S.giro);
        giro.addItem ("(giro modificato)", (int) giroIds.size());
        giro.setSelectedId ((int) giroIds.size(), juce::dontSendNotification);
    }
}

// ogni modifica: Annulla, blocco del suono, ricalcolo, ridisegno
void BottegaEditor::edit (bool regen, const std::function<void()>& fn, bool snap)
{
    if (snap) proc.snapshot();
    {
        juce::ScopedLock sl (proc.lock);
        fn();
    }
    proc.changed (regen);
    refreshAll();
}

void BottegaEditor::refreshAll()
{
    renderTop();
    renderEditor();
    roll.repaint(); grid.repaint();
    resized();
}

void BottegaEditor::renderTop()
{
    juce::ScopedLock sl (proc.lock);
    auto& mo = th.M();
    auto& r = th.R();
    ton.setSelectedId (S.ton + 1, juce::dontSendNotification);
    if (! hz.hasKeyboardFocus (true)) hz.setText (juce::String (S.hz, 2), false);
    if (! bpm.hasKeyboardFocus (true)) bpm.setText (juce::String (S.bpm, S.bpm == std::floor (S.bpm) ? 0 : 1), false);
    caso.setValue (S.caso, juce::dontSendNotification);
    casoTxt.setText (S.caso < 20 ? "prudente" : S.caso < 45 ? "moderato" : S.caso < 75 ? "curioso" : "selvaggio", juce::dontSendNotification);
    uscita.setValue (S.suonoInterno && S.midiOut ? "both" : S.midiOut ? "midi" : "int");
    bend.setValue (juce::String (S.pitchBendRange));
    capOf[&bend]->setVisible (S.midiOut); bend.setVisible (S.midiOut);

    for (size_t i = 0; i < modeIds.size(); ++i) if (modeIds[i] == S.modo) modo.setSelectedId ((int) i + 1, juce::dontSendNotification);
    for (size_t i = 0; i < ritIds.size(); ++i) if (ritIds[i] == S.rit) ritmo.setSelectedId ((int) i + 1, juce::dontSendNotification);
    fillGiri();

    // grado mobile (koron, seconda neutra)
    bool mob = ! mo.mobile.empty();
    koron.setVisible (mob); capOf[&koron]->setVisible (mob);
    if (mob)
    {
        auto [lo, hi] = mo.mobile.begin()->second;
        koron.setRange (lo, hi, 1);
        auto it = S.koron.find (mo.id);
        koron.setValue (it != S.koron.end() ? it->second : (lo + hi) / 2, juce::dontSendNotification);
        capOf[&koron]->setText (u8 (mo.cultura == "Persiana" ? "KORON (2° GRADO)" : "SECONDA NEUTRA (2° GRADO)"), juce::dontSendNotification);
    }

    auto nm = [&] (int k) { return th.nIt (th.degC (mo, k), k); };
    auto list = [&] (const std::vector<int>& v) { juce::StringArray a; for (int k : v) a.add (nm (k)); return a.joinIntoString (", "); };
    juce::StringArray scala; for (int k = 0; k < 7; ++k) scala.add (nm (k));
    juce::String t = mo.nome + " (" + mo.cultura + "): " + scala.joinIntoString (" ") + ". Riposo: " + list (mo.riposo) + "; caratteristica: " + list (mo.caratteristica) + ". "
                     + mo.testo + " Giro: " + th.giroTxt();
    if (! mo.fonti.isEmpty()) t << " Fonti: " << mo.fonti.joinIntoString ("; ") << ".";
    modoTxt.setText (t);

    arm.lock.setToggleState (S.armLock, juce::dontSendNotification);
    rit.lock.setToggleState (S.ritLock, juce::dontSendNotification);
    mel.lock.setToggleState (S.melLock, juce::dontSendNotification);
    for (auto* b : mosse) b->setEnabled (! S.armLock);

    oArmo.setValue (S.armo); oVoc.setValue (S.voc); oComp.setValue (S.comp); oSuono.setValue (S.suono); oBasso.setValue (S.basso);
    auto get = [] (const std::map<juce::String, const char*>& m, const juce::String& k) { auto it = m.find (k); return it != m.end() ? u8 (it->second) : juce::String(); };
    optTxt.setText (get (TXT_ARMO, S.armo) + " " + get (TXT_VOC, S.voc) + " " + get (TXT_BASSO, S.basso));

    // ritmo
    kit.setValue (S.kit);
    strato.setValue (juce::String (S.strato));
    swing.setEnabled (r.ppb >= 2);
    swing.setValue (std::round (th.swingV() * 100), juce::dontSendNotification);
    juce::StringArray gr; for (int g : r.gruppi) gr.add (juce::String (g));
    juce::String rt = r.nome + " (" + r.cultura + "): " + juce::String (r.pulsazioni) + " pulsazioni divise " + gr.joinIntoString ("+") + ", " + juce::String (r.bpm) + " bpm. " + r.testo;
    if (S.strato) rt << " Sopra: " << S.strato << " colpi equidistanti contro le " << r.pulsazioni << " pulsazioni (poliritmia " << S.strato << ":" << (int) r.gruppi.size() << ").";
    if (! r.fonti.isEmpty()) rt << " Fonti: " << r.fonti.joinIntoString ("; ") << ".";
    ritTxt.setText (rt);

    // melodia
    melOn.setValue (S.melOn ? "on" : "off");
    timbro.setValue (S.timbro);
    dens.setValue (S.dens);
    juce::String tt; for (auto& x : TIMBRI) if (S.timbro == x.id) tt = u8 (x.testo);
    melTxt.setText (u8 ("La melodia segue gli accordi: nei tempi forti prende una nota dell'accordo, in mezzo si muove per grado nel modo; finisce sulla tonica. Ornamenti secondo la cultura del modo. ") + tt);

    // mixer
    for (auto* ch : chans)
    {
        ch->s.setValue (S.mix[ch->k], juce::dontSendNotification);
        ch->m.setToggleState (S.mute[ch->k], juce::dontSendNotification);
    }
    lofi.setToggleState (S.lofi, juce::dontSendNotification);
}

void BottegaEditor::renderEditor()
{
    juce::ScopedLock sl (proc.lock);
    if (S.slots.empty()) return;
    S.sel = juce::jlimit (0, (int) S.slots.size() - 1, S.sel);
    const int i = S.sel;
    auto c = th.chordOf (i);
    auto& slot = S.slots[(size_t) i];
    auto st = th.starts();
    juce::String pos = "ACCORDO " + juce::String (i + 1) + " DI " + juce::String ((int) S.slots.size()) + u8 (" · CICLO ") + durTxt (st[(size_t) i] + 1);
    if (slot.dur > 1) pos << u8 ("–") << durTxt (st[(size_t) i] + slot.dur);
    selPos.setText (pos, ui::col::muted, 10.5f, true);

    juce::String head = th.sym (c) + "   " + th.roman (c) + u8 (" · ") + th.funzione (c);
    if (slot.tag.isNotEmpty()) head << "   [" << slot.tag << "]";
    selHead.setText (head, ui::col::ink, 17.0f, true);

    if (S.armo == "bordone") selVoc.setText ("Accordo non suonato: solo bordone e basso.");
    else
    {
        auto V = th.voiceAll();
        juce::StringArray n;
        if (i < (int) V.size()) for (double x : V[(size_t) i]) { double pc = mod (x, 1200); n.add (th.nIt (pc, th.degOfPc (*c.mo, c.t, pc))); }
        selVoc.setText ("Voicing: " + n.joinIntoString (u8 (" · ")) + u8 (" · basso ") + th.nIt (mod (-1200 + th.bassRoot (i), 1200)), ui::col::ink);
    }
    auto sp = th.speciali (c);
    juce::String spt = "NOTA SPECIALE\n";
    for (auto& s : sp) spt << u8 ("• ") << s << "\n";
    if (slot.nota.isNotEmpty()) spt << slot.nota;
    selSpec.setText (spt.trimEnd());

    auto at = th.attrito (i);
    if (! S.melOn) selConf.setText ("Melodia spenta.");
    else if (at.isNotEmpty()) selConf.setText ("Attrito di quarto di tono: " + at + ". Lo segnalo, non lo nascondo: se stride, cambia estensione o usa le quinte aperte.", ui::col::warn);
    else selConf.setText ("Nessun attrito di quarto di tono tra melodia e accordo.", ui::col::rit);

    // editor dell'accordo
    auto& mo = *c.mo;
    std::vector<std::pair<juce::String, juce::String>> d;
    for (int k = 0; k < 7; ++k) d.push_back ({ juce::String (k), juce::String (ROM[k]) + " " + th.nameC (th.degC (mo, k) + slot.t, slot.t != 0 ? -999 : k) });
    edDeg.setOptions (d);
    edDeg.setValue (slot.explicitIv ? juce::String() : juce::String (slot.deg));
    edExt.setValue (slot.explicitIv ? juce::String() : slot.ext);
    edDeg.setEnabledAll (! slot.explicitIv);
    edExt.setEnabledAll (! slot.explicitIv);
    edDur.setValue (durTxt (slot.dur));
    edBass.setValue (slot.bass);
    edDel.setEnabled (S.slots.size() > 1);
    edAdd.setEnabled (th.loopLen() < 16);
}

int BottegaEditor::layoutContent (int W)
{
    const int pad = 16, inner = W - 2 * pad - 24;
    int y = 12;
    auto& C = capOf;
    auto laneHead = [&] (Lane& l, int y0)
    {
        int x = pad + 12;
        l.h.setBounds (x, y0 + 14, 160, 22);
        l.lock.setBounds (pad + 12 + inner - 84, y0 + 12, 84, 26);
        l.var.setBounds (l.lock.getX() - 112, y0 + 12, 104, 26);
        return y0 + 50;
    };
    auto x0 = pad + 12;

    // ARMONIA
    int top = y;
    y = laneHead (arm, y);
    y = flow ({ { &modo, 220, true }, { &giro, 260, true }, { &koron, 300, true } }, C, x0, y, inner);
    y = flow ({ { &modoTxt, inner } }, C, x0, y, inner);
    roll.setBounds (x0, y, inner, 156); y += 166;
    {
        bool two = inner >= 760;
        int lw = two ? (int) (inner * 0.38) : inner, rx = two ? x0 + lw + 20 : x0, rw = two ? inner - lw - 20 : inner;
        int ly = flow ({ { &selPos, lw }, { &selHead, lw }, { &selVoc, lw }, { &selSpec, lw }, { &selConf, lw } }, C, x0, y, lw, 12, 6);
        int ry = two ? y : ly + 6;
        ry = flow ({ { &edDeg, rw } }, C, rx, ry, rw);
        ry = flow ({ { &edExt, rw } }, C, rx, ry, rw);
        ry = flow ({ { &edDur, 200, true }, { &edBass, 240, true } }, C, rx, ry, rw);
        ry = flow ({ { &edPrev, 44 }, { &edNext, 44 }, { &edAdd, 110 }, { &edDel, 92 }, { &edHear, 92 } }, C, rx, ry, rw, 8);
        y = std::max (two ? ly : 0, ry) + 6;
    }
    {
        int my = flow ({ { &mosseCap, 320, true }, { &mossaModo, 240 } }, C, x0, y, inner);
        std::vector<Item> mi;
        int bw = inner >= 760 ? (inner - 3 * 8) / 4 : inner >= 500 ? (inner - 8) / 2 : inner;
        for (auto* b : mosse) mi.push_back ({ b, bw, false, 46 });
        y = flow (mi, C, x0, my, inner, 8, 8);
    }
    y = flow ({ { &oArmo, 340, true }, { &oVoc, 380, true }, { &oComp, 440, true }, { &oSuono, 300, true }, { &oBasso, 340, true } }, C, x0, y, inner);
    y = flow ({ { &optTxt, inner } }, C, x0, y, inner);
    arm.box = { pad, top, W - 2 * pad, y - top + 4 };
    y += 18;

    // RITMO
    top = y;
    y = laneHead (rit, y);
    y = flow ({ { &ritmo, 260, true }, { &kit, 280, true }, { &strato, 260, true }, { &swing, 220, true } }, C, x0, y, inner);
    y = flow ({ { &ritTxt, inner } }, C, x0, y, inner);
    { juce::ScopedLock sl (proc.lock); grid.setBounds (x0, y, inner, grid.preferredHeight()); }
    y += grid.getHeight() + 10;
    rit.box = { pad, top, W - 2 * pad, y - top + 4 };
    y += 18;

    // MELODIA
    top = y;
    y = laneHead (mel, y);
    y = flow ({ { &melOn, 200, true }, { &timbro, 260, true }, { &dens, 260, true } }, C, x0, y, inner);
    y = flow ({ { &melTxt, inner } }, C, x0, y, inner);
    mel.box = { pad, top, W - 2 * pad, y - top + 4 };
    y += 18;

    // MIXER
    top = y;
    y = laneHead (mix, y);
    lofi.setBounds (pad + 12 + inner - 84, top + 12, 84, 26);
    {
        int n = chans.size(), cols = inner >= 900 ? n : inner >= 520 ? 4 : 2;
        int cw = (inner - (cols - 1) * 12) / cols;
        for (int k = 0; k < n; ++k)
        {
            auto* ch = chans[k];
            int cx = x0 + (k % cols) * (cw + 12), cy = y + (k / cols) * 62;
            ch->l.setBounds (cx, cy, cw - 60, 18);
            ch->m.setBounds (cx + cw - 56, cy - 3, 56, 22);
            ch->s.setBounds (cx, cy + 24, cw, 22);
        }
        y += ((n + cols - 1) / cols) * 62;
    }
    mix.box = { pad, top, W - 2 * pad, y - top + 4 };
    return y + 24;
}

void BottegaEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (ui::col::line);
    g.drawHorizontalLine (view.getY() - 1, 0.0f, (float) getWidth());
}

void BottegaEditor::resized()
{
    const int W = getWidth(), pad = 16;
    title.setBounds (pad, 10, W - 2 * pad - 180, 20);
    sync.setBounds (W - pad - 180, 10, 180, 20);
    sync.setJustificationType (juce::Justification::centredRight);
    int y = 38;
    caso.setSize (180, 24);
    y = flow ({ { &play, 56, false, 44 }, { &ton, 150 }, { &hz, 84 }, { &bpm, 64 }, { &caso, 170 }, { &casoTxt, 84, false, 44 },
                { &undoB, 92, false, 30 }, { &tutto, 120, false, 30 } }, capOf, pad, y, W - 2 * pad);
    std::vector<Item> row2 { { &salva, 124, false, 28 }, { &apri, 116, false, 28 }, { &uscita, 330 } };
    if (bend.isVisible()) row2.push_back ({ &bend, 300 });
    y = flow (row2, capOf, pad, y, W - 2 * pad);
    status.setBounds (pad, y - 4, W - 2 * pad, 18);
    y += 18;
    view.setBounds (0, y, W, getHeight() - y);
    int cw = view.getMaximumVisibleWidth();
    if (cw <= 0) cw = W;
    content.setSize (cw, layoutContent (cw));
    content.repaint();
}

void BottegaEditor::timerCallback()
{
    bool pl = proc.isPlaying();
    play.setButtonText (pl ? u8 ("■") : u8 ("▶"));
    play.setToggleState (pl, juce::dontSendNotification);
    undoB.setEnabled (proc.canUndo());
    bool hs = proc.hostSync();
    sync.setText (hs ? "SINCRONIZZATO ALLA DAW" : (S.midiOut ? "MIDI OUT ATTIVO" : ""), juce::dontSendNotification);
    bpm.setEnabled (! hs);
    if (hs && ! bpm.hasKeyboardFocus (true)) bpm.setText (juce::String (S.bpm, 1), false);

    double p = pl ? proc.playPosition() : -1;
    if (p != roll.head)
    {
        roll.head = p;
        grid.head = p >= 0 ? p - std::floor (p) : -1;
        roll.repaint(); grid.repaint();
    }
}

void BottegaEditor::savePreset()
{
    chooser = std::make_unique<juce::FileChooser> ("Salva preset", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (S.nome + ".json"), "*.json");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            f = f.withFileExtension ("json");
            juce::String json;
            { juce::ScopedLock sl (proc.lock); S.nome = f.getFileNameWithoutExtension(); json = S.toJson(); }
            setStatus (f.replaceWithText (json) ? "Preset salvato: " + f.getFullPathName() : "Non riesco a scrivere " + f.getFullPathName());
        });
}

void BottegaEditor::openPreset()
{
    chooser = std::make_unique<juce::FileChooser> ("Apri preset", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.json");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (! f.existsAsFile()) return;
            auto v = juce::JSON::parse (f.loadFileAsString());
            if (! v.isObject()) { setStatus (u8 ("Il file non è un preset della Bottega.")); return; }
            proc.loadSong (Song::fromVar (v));
            refreshAll();
            setStatus ("Preset aperto: " + f.getFileNameWithoutExtension());
        });
}
