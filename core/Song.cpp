#include "Song.h"

namespace bottega
{
namespace
{
    using juce::var;
    var arr (const std::vector<double>& v) { juce::Array<var> a; for (auto x : v) a.add (x); return a; }
    std::vector<double> dbl (const var& v) { std::vector<double> o; if (auto* a = v.getArray()) for (auto& x : *a) o.push_back ((double) x); return o; }
    var tracks (const TrackMap& t)
    {
        auto* o = new juce::DynamicObject();
        for (auto& [k, hits] : t)
        {
            juce::Array<var> a;
            for (auto& h : hits) a.add (juce::Array<var> { h.step, (double) h.vel });
            o->setProperty (k, a);
        }
        return var (o);
    }
    TrackMap tracksFrom (const var& v)
    {
        TrackMap t;
        if (auto* o = v.getDynamicObject())
            for (auto& p : o->getProperties())
                if (auto* a = p.value.getArray())
                    for (auto& h : *a) t[p.name.toString()].push_back ({ (int) h[0], (float) (double) h[1] });
        return t;
    }
    template <typename T> void get (const var& v, const char* k, T& out) { if (v.hasProperty (k)) out = (T) v[k]; }
    void getS (const var& v, const char* k, juce::String& out) { if (v.hasProperty (k)) out = v[k].toString(); }
}

juce::var Song::toVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("formato", "bottega-preset");
    o->setProperty ("versione", 1);
    o->setProperty ("nome", nome); o->setProperty ("note", note); o->setProperty ("tags", tags);
    o->setProperty ("ton", ton); o->setProperty ("hz", hz); o->setProperty ("bpm", bpm);
    o->setProperty ("modo", modo); o->setProperty ("caso", caso);
    auto* k = new juce::DynamicObject(); for (auto& [id, c] : koron) k->setProperty (id, c); o->setProperty ("koron", var (k));
    o->setProperty ("rit", rit); o->setProperty ("kit", kit); o->setProperty ("strato", strato); o->setProperty ("swing", swing);
    o->setProperty ("ritLock", ritLock);
    juce::Array<var> rv; for (auto& x : ritVar) rv.add (x ? tracks (*x) : var()); o->setProperty ("ritVar", rv);

    juce::Array<var> sl;
    for (auto& s : slots)
    {
        auto* x = new juce::DynamicObject();
        x->setProperty ("deg", s.deg); x->setProperty ("ext", s.ext); x->setProperty ("dur", s.dur); x->setProperty ("bass", s.bass);
        if (s.modo.isNotEmpty()) x->setProperty ("modo", s.modo);
        if (s.t != 0.0) x->setProperty ("t", s.t);
        if (s.explicitIv) { x->setProperty ("r", s.r); x->setProperty ("iv", arr (s.iv)); x->setProperty ("q", s.q); }
        for (auto [key, val] : { std::pair<const char*, juce::String> { "rom", s.rom }, { "fun", s.fun }, { "tag", s.tag }, { "nota", s.nota } })
            if (val.isNotEmpty()) x->setProperty (key, val);
        sl.add (var (x));
    }
    o->setProperty ("slots", sl);
    o->setProperty ("sel", sel); o->setProperty ("giro", giro); o->setProperty ("voc", voc); o->setProperty ("armo", armo);
    o->setProperty ("comp", comp); o->setProperty ("suono", suono); o->setProperty ("basso", basso); o->setProperty ("armLock", armLock);

    o->setProperty ("melOn", melOn); o->setProperty ("melLock", melLock); o->setProperty ("timbro", timbro); o->setProperty ("dens", dens);
    juce::Array<var> fr;
    for (auto& f : frasi)
    {
        juce::Array<var> a;
        for (auto& n : f) a.add (juce::Array<var> { n.c, n.u, n.len, n.deg, n.mo, n.t, n.orn });
        fr.add (a);
    }
    o->setProperty ("frasi", fr);

    auto* m = new juce::DynamicObject(); for (auto& [id, v] : mix) m->setProperty (id, (double) v); o->setProperty ("mix", var (m));
    auto* mu = new juce::DynamicObject(); for (auto& [id, v] : mute) mu->setProperty (id, v); o->setProperty ("mute", var (mu));
    o->setProperty ("lofi", lofi);
    o->setProperty ("suonoInterno", suonoInterno); o->setProperty ("midiOut", midiOut); o->setProperty ("pitchBendRange", pitchBendRange);
    return var (o);
}

Song Song::fromVar (const juce::var& v)
{
    Song s;
    if (! v.isObject()) return s;
    getS (v, "nome", s.nome); getS (v, "note", s.note); getS (v, "tags", s.tags);
    get (v, "ton", s.ton); get (v, "hz", s.hz); get (v, "bpm", s.bpm); getS (v, "modo", s.modo); get (v, "caso", s.caso);
    if (auto* k = v["koron"].getDynamicObject()) for (auto& p : k->getProperties()) s.koron[p.name.toString()] = (double) p.value;
    getS (v, "rit", s.rit); getS (v, "kit", s.kit); get (v, "strato", s.strato); get (v, "swing", s.swing); get (v, "ritLock", s.ritLock);
    if (auto* a = v["ritVar"].getArray())
        for (auto& x : *a) s.ritVar.push_back (x.isObject() ? std::optional<TrackMap> (tracksFrom (x)) : std::nullopt);
    if (auto* a = v["slots"].getArray())
    {
        s.slots.clear();
        for (auto& x : *a)
        {
            Slot sl;
            get (x, "deg", sl.deg); getS (x, "ext", sl.ext); get (x, "dur", sl.dur); getS (x, "bass", sl.bass); getS (x, "modo", sl.modo); get (x, "t", sl.t);
            if (x.hasProperty ("iv")) { sl.explicitIv = true; sl.r = (double) x["r"]; sl.iv = dbl (x["iv"]); getS (x, "q", sl.q); }
            getS (x, "rom", sl.rom); getS (x, "fun", sl.fun); getS (x, "tag", sl.tag); getS (x, "nota", sl.nota);
            s.slots.push_back (sl);
        }
    }
    get (v, "sel", s.sel); getS (v, "giro", s.giro); getS (v, "voc", s.voc); getS (v, "armo", s.armo);
    getS (v, "comp", s.comp); getS (v, "suono", s.suono); getS (v, "basso", s.basso); get (v, "armLock", s.armLock);
    get (v, "melOn", s.melOn); get (v, "melLock", s.melLock); getS (v, "timbro", s.timbro); getS (v, "dens", s.dens);
    if (auto* a = v["frasi"].getArray())
        for (auto& f : *a)
        {
            std::vector<MelNote> ph;
            if (auto* b = f.getArray())
                for (auto& n : *b) ph.push_back ({ (int) n[0], (int) n[1], (int) n[2], (int) n[3], n[4].toString(), (double) n[5], n[6].toString() });
            s.frasi.push_back (ph);
        }
    if (auto* m = v["mix"].getDynamicObject()) for (auto& p : m->getProperties()) s.mix[p.name.toString()] = (float) (double) p.value;
    if (auto* m = v["mute"].getDynamicObject()) for (auto& p : m->getProperties()) s.mute[p.name.toString()] = (bool) p.value;
    get (v, "lofi", s.lofi); get (v, "suonoInterno", s.suonoInterno); get (v, "midiOut", s.midiOut); get (v, "pitchBendRange", s.pitchBendRange);
    return s;
}
} // namespace bottega
