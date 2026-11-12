#include "Archive.h"
#include "BinaryData.h"

namespace bottega
{
namespace
{
    std::vector<int> ints (const juce::var& v)
    {
        std::vector<int> out;
        if (auto* a = v.getArray())
            for (auto& x : *a) out.push_back ((int) x);
        return out;
    }
    std::vector<double> doubles (const juce::var& v)
    {
        std::vector<double> out;
        if (auto* a = v.getArray())
            for (auto& x : *a) out.push_back ((double) x);
        return out;
    }
    juce::StringArray strings (const juce::var& v)
    {
        juce::StringArray out;
        if (auto* a = v.getArray())
            for (auto& x : *a) out.add (x.toString());
        return out;
    }
}

bool Archive::loadFromJson (const juce::String& json, juce::String& error)
{
    juce::var root;
    auto res = juce::JSON::parse (json, root);
    if (res.failed()) { error = res.getErrorMessage(); return false; }

    modi.clear(); ritmi.clear(); ricchi.clear(); mosse.clear();

    if (auto* a = root["modi"].getArray())
        for (auto& m : *a)
        {
            Mode md;
            md.id = m["id"].toString(); md.cultura = m["cultura"].toString(); md.nome = m["nome"].toString();
            md.testo = m["testo"].toString(); md.ornamento = m["ornamento"].toString();
            md.cents = doubles (m["cents"]);
            auto g = m["gerarchia"];
            md.tonica = (int) g["tonica"];
            md.riposo = ints (g["riposo"]); md.cardini = ints (g["cardini"]); md.caratteristica = ints (g["caratteristica"]);
            auto b = doubles (m["bordone"]);
            if (b.size() >= 2) { md.bordone[0] = b[0]; md.bordone[1] = b[1]; }
            if (auto* mob = m["mobile"].getDynamicObject())
                for (auto& p : mob->getProperties())
                {
                    auto r = doubles (p.value);
                    if (r.size() == 2) md.mobile[p.name.toString().getIntValue()] = { r[0], r[1] };
                }
            if (auto* gi = m["giri"].getArray())
                for (auto& x : *gi) md.giri.push_back ({ ints (x["gradi"]), x["nome"].toString() });
            md.fonti = strings (m["fonti"]); md.ritmi = strings (m["ritmi"]);
            if (md.cents.size() != 7) { error = "Il modo " + md.id + " non ha 7 gradi"; return false; }
            modi.push_back (std::move (md));
        }

    if (auto* a = root["ritmi"].getArray())
        for (auto& r : *a)
        {
            Rhythm rt;
            rt.id = r["id"].toString(); rt.cultura = r["cultura"].toString(); rt.nome = r["nome"].toString();
            rt.testo = r["testo"].toString();
            rt.comp = r.hasProperty ("comp") ? r["comp"].toString() : juce::String ("tenuto");
            rt.pulsazioni = (int) r["pulsazioni"]; rt.ppb = (int) r["ppb"]; rt.bpm = (int) r["bpm"];
            rt.gruppi = ints (r["gruppi"]); rt.accenti = ints (r["accenti"]);
            rt.segni = strings (r["segni"]); rt.colpi = strings (r["colpi"]);
            if (r.hasProperty ("swing")) rt.swing = (double) r["swing"];
            if (r.hasProperty ("duck")) rt.duck = (double) r["duck"];
            rt.vinile = (bool) r["vinile"];
            if (auto* tr = r["tracks"].getDynamicObject())
                for (auto& p : tr->getProperties())
                {
                    std::vector<Hit> hits;
                    if (auto* hs = p.value.getArray())
                        for (auto& h : *hs) hits.push_back ({ (int) h[0], (float) (double) h[1] });
                    rt.tracks[p.name.toString()] = hits;
                }
            if (auto* bs = r["bass"].getArray())
                for (auto& x : *bs) rt.bass.push_back ({ (int) x[0], (int) x[1], (double) x[2], (float) (double) x[3] });
            if (auto* st = r["stabs"].getArray())
                for (auto& x : *st) rt.stabs.push_back ({ (int) x[0], (double) x[1] });
            rt.fonti = strings (r["fonti"]);
            if ((int) rt.accenti.size() != rt.pulsazioni) rt.accenti.resize ((size_t) rt.pulsazioni, 0);
            ritmi.push_back (std::move (rt));
        }

    if (auto* a = root["ricchi"].getArray())
        for (auto& g : *a)
        {
            RichGiro rg { g["id"].toString(), g["nome"].toString(), g["testo"].toString(), {} };
            if (auto* sl = g["slots"].getArray())
                for (auto& s : *sl) rg.slots.push_back ({ (int) s["deg"], s["ext"].toString(), (double) s["dur"], s["bass"].toString() });
            ricchi.push_back (rg);
        }

    if (auto* a = root["mosse"].getArray())
        for (auto& m : *a)
            mosse.push_back ({ m["id"].toString(), m["nome"].toString(), m["stile"].toString(), m["testo"].toString() });

    if (modi.empty() || ritmi.empty()) { error = "Archivio vuoto"; return false; }
    return true;
}

const Archive& Archive::builtIn()
{
    static Archive a = []
    {
        Archive x;
        juce::String err;
        x.loadFromJson (juce::String::fromUTF8 (BinaryData::archivio_json, BinaryData::archivio_jsonSize), err);
        jassert (err.isEmpty());
        return x;
    }();
    return a;
}

const Mode* Archive::mode (const juce::String& id) const
{
    for (auto& m : modi) if (m.id == id) return &m;
    return nullptr;
}
const Rhythm* Archive::rhythm (const juce::String& id) const
{
    for (auto& r : ritmi) if (r.id == id) return &r;
    return nullptr;
}
} // namespace bottega
