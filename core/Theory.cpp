#include "Theory.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace bottega
{
double mod (double a, double n) { double r = std::fmod (a, n); return r < 0 ? r + n : r; }
int imod (int a, int n) { int r = a % n; return r < 0 ? r + n : r; }
double cdist (double a, double b) { return std::abs (mod (a - b + 600.0, 1200.0) - 600.0); }

namespace
{
    const char* LET_EN[] = { "C", "D", "E", "F", "G", "A", "B" };
    const char* LET_IT[] = { "Do", "Re", "Mi", "Fa", "Sol", "La", "Si" };
    const double NAT[] = { 0, 200, 400, 500, 700, 900, 1100 };
    const char* FL_EN[] = { "C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B" };
    const int TON_LETTER[] = { 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6 };
    const char* ROM[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
    juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    int nearestK (double rel)
    {
        int best = 0; double bd = 1e9;
        for (int k = 0; k < 7; ++k)
        {
            double sd = mod (rel - NAT[k] + 600, 1200) - 600, d = std::abs (sd);
            if (d < bd - 1 || (std::abs (d - bd) <= 1 && sd < 0)) { bd = d; best = k; }
        }
        return best;
    }
    juce::String altTxt (double x, double nat)
    {
        int d = (int) std::lround ((x - nat) / 50.0) * 50;
        switch (d) { case -100: return u8 ("♭"); case -50: return u8 ("½♭"); case 50: return u8 ("½♯"); case 100: return u8 ("♯"); default: return {}; }
    }
}

const std::vector<Ext>& extensions()
{
    // passi di scala sopra la radice: 0 radice, 2 terza, 4 quinta, 6 settima, 8 nona, 10 undicesima, 12 tredicesima
    static const std::vector<Ext> e {
        { "triade", "Triade", { 0, 2, 4 } }, { "5", "5", { 0, 4 } }, { "6", "6", { 0, 2, 4, 5 } }, { "7", "7", { 0, 2, 4, 6 } },
        { "9", "9", { 0, 2, 4, 6, 8 } }, { "11", "11", { 0, 2, 4, 6, 10 } }, { "13", "13", { 0, 2, 4, 6, 8, 12 } },
        { "add9", "add9", { 0, 2, 4, 8 } }, { "69", "6/9", { 0, 2, 4, 5, 8 } }, { "sus2", "sus2", { 0, 1, 4 } }, { "sus4", "sus4", { 0, 3, 4 } },
        { "7sus4", "7sus4", { 0, 3, 4, 6 } }, { "9sus4", "9sus4", { 0, 3, 4, 6, 8 } } };
    return e;
}
const Ext* findExt (const juce::String& id)
{
    for (auto& e : extensions()) if (id == e.id) return &e;
    return &extensions()[3];
}

const Mode& Theory::M() const { return modeOf (S.modo); }
const Mode& Theory::modeOf (const juce::String& id) const { auto* m = A.mode (id); return m ? *m : A.modi.front(); }
const Rhythm& Theory::R() const { auto* r = A.rhythm (S.rit); return r ? *r : A.ritmi.front(); }

double Theory::degC (const Mode& mo, int i) const
{
    int k = imod (i, 7), o = (int) std::floor (i / 7.0);
    double c = mo.cents[(size_t) k];
    if (mo.mobile.count (k) && S.koron.count (mo.id)) c = S.koron.at (mo.id);
    return c + 1200.0 * o;
}

juce::String Theory::nameC (double c, int deg, bool it, bool nodev) const
{
    double pc = mod (tonPc() + c, 1200);
    int li;
    if (deg != -999) li = imod (TON_LETTER[S.ton] + imod (deg, 7), 7);
    else { int s = (int) std::lround (pc / 100.0) % 12; juce::String fl = u8 (FL_EN[s]); li = juce::String ("CDEFGAB").indexOfChar (fl[0]); }
    double diff = pc - NAT[li];
    if (diff > 600) diff -= 1200;
    if (diff < -600) diff += 1200;
    int st = (int) std::lround (diff / 50.0) * 50;
    int dev = (int) std::lround (diff - st);
    bool persiana = M().cultura == "Persiana";
    juce::String a;
    switch (st)
    {
        case -200: a = u8 ("𝄫"); break; case -150: a = u8 ("♭½♭"); break; case -100: a = u8 ("♭"); break;
        case -50: a = persiana ? (it ? " koron" : "k") : u8 ("½♭"); break; case 0: break;
        case 50: a = u8 ("½♯"); break; case 100: a = u8 ("♯"); break; case 150: a = u8 ("♯½♯"); break; case 200: a = u8 ("𝄪"); break;
        default: a = "?";
    }
    juce::String out = juce::String (it ? LET_IT[li] : LET_EN[li]) + a;
    if (! nodev && st % 100 == 0 && std::abs (dev) >= 8) out += (dev > 0 ? "+" : u8 ("−")) + juce::String (std::abs (dev));
    return out;
}

int Theory::degOfPc (const Mode& mo, double t, double pc) const
{
    for (int k = 0; k < 7; ++k) if (cdist (mod (t + degC (mo, k), 1200), pc) < 15) return k;
    return -999;
}

Chord Theory::chordOf (int i) const
{
    Chord c;
    i = juce::jlimit (0, (int) S.slots.size() - 1, i);
    c.i = i; c.sl = &S.slots[(size_t) i];
    c.mo = &modeOf (c.sl->modo.isNotEmpty() ? c.sl->modo : S.modo);
    c.t = c.sl->t;
    if (c.sl->explicitIv) { c.r = mod (c.sl->r, 1200); c.iv = c.sl->iv; }
    else
    {
        double b = degC (*c.mo, c.sl->deg);
        c.r = mod (c.t + b, 1200);
        for (int k : findExt (c.sl->ext)->k) c.iv.push_back (degC (*c.mo, c.sl->deg + k) - b);
    }
    for (double x : c.iv) c.pcs.push_back (mod (c.r + x, 1200));
    return c;
}

juce::String Theory::rootName (const Chord& c) const
{
    if (! c.sl->explicitIv && c.t == 0) return nameC (c.r, c.sl->deg, false, true);
    int d = degOfPc (*c.mo, c.t, c.r);
    return nameC (c.r, (d != -999 && c.t == 0) ? d : -999, false, true);
}

double Theory::ivAt (const Chord& c, int k) const
{
    if (c.sl->explicitIv) return 0;
    auto& ks = findExt (c.sl->ext)->k;
    auto it = std::find (ks.begin(), ks.end(), k);
    return it == ks.end() ? 0 : mod (c.iv[(size_t) (it - ks.begin())], 1200);
}

Theory::Q Theory::qual (const Chord& c) const
{
    Q q;
    std::vector<double> m; for (double x : c.iv) m.push_back (mod (x, 1200));
    for (double x : m) if (x >= 250 && x <= 450) { q.t3 = x <= 325 ? 'm' : x >= 375 ? 'M' : 'n'; break; }
    for (double x : m) if (x >= 660 && x <= 740) q.p5 = true;
    if (! q.p5) for (double x : m) if (x >= 560 && x <= 640) q.d5 = true;
    for (double x : m) if (x >= 960 && x <= 1150) { q.s7 = x; break; }
    q.maj7 = q.s7 > 1075;
    return q;
}

juce::String Theory::sym (const Chord& c) const
{
    auto root = rootName (c);
    if (c.sl->explicitIv) return root + c.sl->q;
    auto q = qual (c);
    juce::String e = c.sl->ext;
    bool aug = q.t3 == 'M' && ! q.p5 && e != "6" && e != "69" && e != "13";
    if (aug) { aug = false; for (double x : c.iv) { double m = mod (x, 1200); if (m >= 760 && m <= 840) aug = true; } }
    juce::String base = q.t3 == 'm' ? (q.d5 ? "dim" : "m") : q.t3 == 'n' ? "n" : (aug ? "+" : "");
    auto sev = [&] (const juce::String& x) -> juce::String
    {
        if (base == "dim") return x == "7" ? (q.s7 >= 0 && q.s7 < 960 ? u8 ("°7") : u8 ("m7♭5")) : "m" + x + u8 ("♭5");
        if (base == "m" && q.maj7) return "m(maj" + x + ")";
        return base + (q.maj7 ? "maj" : "") + x;
    };
    juce::String mb = base == "dim" ? "m" : base;
    if (e == "triade") return root + (base == "dim" ? u8 ("°") : base);
    if (e == "5") return root + "5";
    if (e == "7") return root + sev ("7");
    if (e == "9") { auto a = altTxt (ivAt (c, 8), 200); return root + (a.isNotEmpty() ? sev ("7") + a + "9" : sev ("9")); }
    if (e == "11") { auto a = altTxt (ivAt (c, 10), 500); return root + (a.isNotEmpty() ? sev ("7") + a + "11" : sev ("11")); }
    if (e == "13") { auto a = altTxt (ivAt (c, 12), 900); return root + (a.isNotEmpty() ? sev ("9") + a + "13" : sev ("13")); }
    if (e == "6") return root + mb + altTxt (ivAt (c, 5), 900) + "6";
    if (e == "69") return root + mb + "6/9";
    if (e == "add9") return root + mb + "add" + altTxt (ivAt (c, 8), 200) + "9";
    if (e == "sus2") return root + "sus" + altTxt (ivAt (c, 1), 200) + "2";
    if (e == "sus4") return root + "sus" + altTxt (ivAt (c, 3), 500) + "4";
    if (e == "7sus4") return root + (q.maj7 ? "maj7" : "7") + "sus" + altTxt (ivAt (c, 3), 500) + "4";
    if (e == "9sus4") return root + (q.maj7 ? "maj9" : "9") + "sus" + altTxt (ivAt (c, 3), 500) + "4";
    return root;
}

juce::String Theory::romanOf (const Chord& c, double center) const
{
    double rel = mod (c.r - center, 1200);
    int k = (! c.sl->explicitIv && center == c.t) ? imod (c.sl->deg, 7) : nearestK (rel);
    double d = mod (rel - NAT[k] + 600, 1200) - 600;
    juce::String acc = d <= -75 ? u8 ("♭") : d <= -25 ? u8 ("½♭") : d >= 75 ? u8 ("♯") : d >= 25 ? u8 ("½♯") : juce::String();
    auto q = qual (c);
    juce::String n = ROM[k];
    if (q.t3 == 'm' || q.d5) n = n.toLowerCase();
    juce::String suf, e = c.sl->ext;
    if (c.sl->explicitIv) suf = c.sl->q;
    else if (q.d5 && q.t3 == 'm') suf = e == "triade" ? u8 ("°") : (q.s7 >= 0 && q.s7 < 960 ? u8 ("°7") : u8 ("ø") + (e == "7" ? juce::String ("7") : findExt (e)->nome));
    else suf = e == "triade" ? juce::String() : e == "69" ? juce::String ("6/9") : ((q.maj7 && (e == "7" || e == "9" || e == "11" || e == "13")) ? "maj" + e : e);
    return acc + n + suf;
}

juce::String Theory::roman (const Chord& c) const
{
    if (c.sl->rom.isNotEmpty()) return c.sl->rom;
    return romanOf (c, c.t) + (c.t != 0 ? " (in " + nameC (c.t) + ")" : juce::String());
}

juce::String Theory::funzione (const Chord& c) const
{
    if (c.sl->fun.isNotEmpty()) return c.sl->fun;
    double rel = mod (c.r - c.t, 1200);
    int s = (int) std::lround (rel / 100.0) % 12;
    bool lt = false;
    for (double p : c.pcs) if (cdist (p, mod (c.t + 1100, 1200)) < 30) lt = true;
    const char* f[] = { "Tonica", "Dominante frigia", "Sottodominante", "Tonica relativa", "Tonica relativa", "Sottodominante",
                        "Dominante (tritono)", lt ? "Dominante" : "Dominante modale", "Sottodominante", "Tonica relativa",
                        lt ? "Dominante" : "Dominante modale", "Dominante" };
    return f[s];
}

juce::StringArray Theory::speciali (const Chord& c) const
{
    juce::StringArray out;
    if (c.sl->nota.isNotEmpty()) out.add (c.sl->nota);
    for (int k : c.mo->caratteristica)
    {
        double p = mod (c.t + degC (*c.mo, k), 1200);
        for (double x : c.pcs)
            if (cdist (x, p) < 12) { out.add ("Contiene " + nIt (p, k) + ", la nota caratteristica di " + c.mo->nome + u8 (": è questo accordo che fa sentire il modo.")); break; }
    }
    double lt = mod (c.t + 1100, 1200);
    if ((int) std::lround (mod (c.r - c.t, 1200) / 100.0) % 12 != 0)
        for (double x : c.pcs)
            if (cdist (x, lt) < 20) { out.add ("Contiene " + nIt (lt) + ", la sensibile: tira verso la tonica e chiede di risolvere."); break; }
    if (! c.sl->explicitIv)
    {
        static const std::map<int, const char*> nomi { { 1, "la seconda" }, { 3, "la quarta" }, { 5, "la sesta" }, { 8, "la nona" }, { 10, "l'undicesima" }, { 12, "la tredicesima" } };
        auto& ks = findExt (c.sl->ext)->k;
        for (int j = (int) ks.size() - 1; j >= 0; --j)
            if (nomi.count (ks[(size_t) j]))
            {
                double p = mod (c.r + c.iv[(size_t) j], 1200);
                out.add (juce::String ("Il colore viene da ") + nomi.at (ks[(size_t) j]) + ": " + nIt (p, degOfPc (*c.mo, c.t, p)) + ".");
                break;
            }
    }
    if (S.armo != "bordone")
    {
        bool done = false;
        for (double p : c.pcs)
        {
            for (double b : M().bordone)
            {
                int d = (int) std::lround (cdist (p, b));
                if (d >= 15 && d <= 130)
                {
                    out.add ("Tensione con il bordone: " + nIt (p, degOfPc (*c.mo, c.t, p)) + " sta a " + juce::String (d) + " cents da " + nIt (b)
                             + (d < 85 ? " (quarto di tono)" : " (semitono)") + ". Da usare di passaggio.");
                    done = true; break;
                }
            }
            if (done) break;
        }
    }
    if (out.isEmpty()) out.add ("Accordo tutto dentro il modo, senza tensioni con il bordone: stabile.");
    return out;
}

juce::String Theory::attrito (int i) const
{
    if (! S.melOn) return {};
    auto c = chordOf (i);
    auto st = starts();
    double a = st[(size_t) i], b = a + c.sl->dur;
    int P = R().pulsazioni, u = sub();
    double worst = 1e9; juce::String txt;
    for (auto& f : S.frasi)
        for (auto& n : f)
        {
            double pos = n.c + n.u / (double) (P * u);
            if (pos < a || pos >= b) continue;
            double pc = mod (melCents (n), 1200);
            for (double q : c.pcs)
            {
                double d = cdist (pc, q);
                if (d > 15 && d < 85 && d < worst)
                { worst = d; txt = nIt (pc, n.deg) + " contro " + nIt (q) + " (" + juce::String ((int) std::lround (d)) + " cents)"; }
            }
        }
    return txt;
}

// ---------------- sequenza ----------------
double Theory::beatsPerCycle() const { return R().pulsazioni / (double) R().ppb; }

double Theory::scaleDur (double d) const
{
    double B = beatsPerCycle(), f = B > 10 ? 4 : B > 5 ? 2 : 1;
    double x = std::max (0.5, std::round (d / f * 2) / 2);
    if (R().pulsazioni % 2 && std::fmod (x, 1.0) != 0) x = std::ceil (x);
    return x;
}

std::vector<std::pair<juce::String, juce::String>> Theory::giriList() const
{
    std::vector<std::pair<juce::String, juce::String>> g;
    for (size_t i = 0; i < M().giri.size(); ++i) g.push_back ({ "i:" + juce::String ((int) i), M().giri[i].nome });
    for (auto& r : A.ricchi) g.push_back ({ "r:" + r.id, r.nome + " (accordi ricchi)" });
    return g;
}

void Theory::genArm (juce::String id)
{
    if (id.isEmpty()) id = S.giro;
    S.giro = id;
    S.slots.clear();
    if (id.startsWith ("r:"))
    {
        for (auto& r : A.ricchi)
            if ("r:" + r.id == id)
                for (auto& d : r.slots) { Slot s; s.deg = d.deg; s.ext = d.ext; s.dur = scaleDur (d.dur); s.bass = d.bass; S.slots.push_back (s); }
    }
    else
    {
        int gi = id.substring (2).getIntValue();
        std::vector<int> gradi { 0 };
        if (gi >= 0 && gi < (int) M().giri.size()) gradi = M().giri[(size_t) gi].gradi;
        juce::String ext = M().cultura == "Occidentale" ? "7" : "triade";
        for (int d : gradi) { Slot s; s.deg = d; s.ext = ext; s.dur = scaleDur (gradi.size() <= 4 ? 2 : 1); S.slots.push_back (s); }
    }
    if (S.slots.empty()) { Slot s; s.deg = 0; s.ext = "7"; s.dur = 1; S.slots.push_back (s); }
    S.sel = 0;
    fixLen();
}

juce::String Theory::giroTxt() const
{
    if (S.giro.startsWith ("r:")) { for (auto& r : A.ricchi) if ("r:" + r.id == S.giro) return r.testo; }
    else if (S.giro.startsWith ("i:")) { int gi = S.giro.substring (2).getIntValue(); if (gi < (int) M().giri.size()) return M().giri[(size_t) gi].nome; }
    return "Giro modificato a mano.";
}

void Theory::fixLen()
{
    auto total = [&] { double a = 0; for (auto& s : S.slots) a += s.dur; return a; };
    double L = total();
    if (std::fmod (L, 1.0) > 1e-9) S.slots.back().dur += std::ceil (L) - L;
    while (total() > 16 && S.slots.size() > 1) S.slots.pop_back();
    S.sel = juce::jlimit (0, (int) S.slots.size() - 1, S.sel);
}

int Theory::loopLen() const { double a = 0; for (auto& s : S.slots) a += s.dur; return std::max (1, (int) std::ceil (a - 1e-9)); }

std::vector<double> Theory::starts() const { std::vector<double> s; double a = 0; for (auto& x : S.slots) { s.push_back (a); a += x.dur; } return s; }

int Theory::slotAt (double pos) const
{
    double a = 0;
    for (int i = 0; i < (int) S.slots.size(); ++i) { if (pos < a + S.slots[(size_t) i].dur - 1e-9) return i; a += S.slots[(size_t) i].dur; }
    return (int) S.slots.size() - 1;
}

int Theory::splitInsert (int i, Slot ns)
{
    auto& sl = S.slots[(size_t) i];
    if (sl.dur >= 1) { double h = sl.dur >= 2 ? std::floor (sl.dur / 2) : 0.5; ns.dur = h; sl.dur -= h; }
    else ns.dur = 0.5;
    S.slots.insert (S.slots.begin() + i + 1, ns);
    fixLen();
    return i + 1;
}

juce::String Theory::mossa (const juce::String& id, const juce::String& versoModo)
{
    int n = (int) S.slots.size(), i = S.sel;
    auto c = chordOf (i), nx = chordOf ((i + 1) % n);
    auto symNx = sym (nx), romNx = romanOf (nx, nx.t);
    auto dom7 = [&] (double r, const juce::String& rom, const juce::String& fun, const juce::String& tag, const juce::String& nota)
    {
        Slot s; s.explicitIv = true; s.iv = { 0, 400, 700, 1000 }; s.r = r; s.q = "7"; s.t = c.t; s.rom = rom; s.fun = fun; s.tag = tag; s.nota = nota;
        return s;
    };
    if (id == "domsec")
    {
        double r = mod (nx.r + 700, 1200);
        S.sel = splitInsert (i, dom7 (r, "V7/" + romNx, "Dominante secondaria", "Dominante secondaria",
            "Prepara " + symNx + ": la sua terza (" + nIt (r + 400) + ") sta un semitono sotto " + nIt (nx.r) + " e la fa sentire come una tonica provvisoria."));
        return "Dominante secondaria inserita prima di " + symNx + ".";
    }
    if (id == "tritono")
    {
        double r = mod (nx.r + 100, 1200);
        auto s = dom7 (r, "subV7/" + romNx, "Dominante (sostituto di tritono)", "Sostituzione di tritono",
            "Sostituisce " + nameC (mod (nx.r + 700, 1200)) + "7: stesso tritono (" + nIt (r + 400) + " e " + nIt (r + 1000) + "), ma il basso scende di semitono su " + nIt (nx.r) + ".");
        if (c.sl->tag == "Dominante secondaria" || c.sl->tag == "Sostituzione di tritono") { s.dur = c.sl->dur; S.slots[(size_t) i] = s; }
        else S.sel = splitInsert (i, s);
        return "Sostituzione di tritono verso " + symNx + ".";
    }
    if (id == "scambio")
    {
        if (c.sl->explicitIv) return u8 ("Lo scambio modale funziona sugli accordi del modo: scegli un accordo che non sia una mossa.");
        juce::String par = degC (*c.mo, 2) < 350 ? "ionico" : "eolio";
        auto& sl = S.slots[(size_t) i];
        sl.modo = par; sl.tag = "Scambio modale";
        auto c2 = chordOf (i);
        juce::StringArray nuove, vecchie;
        for (double p : c2.pcs) if (std::find (c.pcs.begin(), c.pcs.end(), p) == c.pcs.end()) nuove.add (nIt (p));
        for (double p : c.pcs) if (std::find (c2.pcs.begin(), c2.pcs.end(), p) == c2.pcs.end()) vecchie.add (nIt (p));
        sl.nota = nuove.isEmpty() ? "Qui " + modeOf (par).nome + " e " + c.mo->nome + " hanno lo stesso accordo: prova un altro grado."
                                  : "Preso da " + modeOf (par).nome + " sulla stessa tonica: stesso grado, colore opposto (" + nuove.joinIntoString (", ") + " al posto di " + vecchie.joinIntoString (", ") + ").";
        return "Scambio modale da " + modeOf (par).nome + ".";
    }
    if (id == "cromatico")
    {
        double r = mod (nx.r - 100, 1200);
        Slot s; s.explicitIv = true; s.iv = { 0, 300, 600, 900 }; s.r = r; s.q = u8 ("°7"); s.t = c.t; s.fun = "Passaggio cromatico"; s.tag = "Salita cromatica del basso";
        s.nota = "Diminuito di passaggio: il basso sale di semitono (" + nIt (r) + u8 (" → ") + nIt (nx.r) + ") e ogni voce si muove al massimo di un semitono.";
        S.sel = splitInsert (i, s);
        return "Diminuito di passaggio inserito prima di " + symNx + ".";
    }
    if (id == "mezzotono")
    {
        if (i >= n - 1) return u8 ("Seleziona un accordo che non sia l'ultimo: la salita parte dall'accordo successivo.");
        for (int k = i + 1; k < n; ++k) { auto& s = S.slots[(size_t) k]; s.t += 100; if (s.explicitIv) s.r = mod (s.r + 100, 1200); }
        auto& s = S.slots[(size_t) i + 1];
        s.tag = "Salita di mezzo tono";
        s.nota = u8 ("Da qui tutto è un semitono più in alto. Il bordone resta sulla tonica di partenza: l'attrito che senti è voluto, e al giro successivo si torna giù.");
        return "Dalla battuta successiva il giro sale di mezzo tono.";
    }
    if (id == "pivot")
    {
        if (i >= n - 1) return u8 ("Seleziona un accordo che non sia l'ultimo: il pivot apre la strada a quelli dopo.");
        double base = c.t; int found = -1;
        for (int tt : { 700, 500, 300, 900, 800, 1000, 200, 400 })
        {
            bool all = true;
            for (double p : c.pcs)
            {
                bool any = false;
                for (int k = 0; k < 7; ++k) if (cdist (p, mod (base + tt + degC (*c.mo, k), 1200)) < 15) any = true;
                if (! any) { all = false; break; }
            }
            if (all) { found = tt; break; }
        }
        if (found < 0) return u8 ("Questo accordo non appartiene a nessuna tonalità vicina nello stesso modo: prova un accordo più semplice (triade o 7).");
        double nt = base + found;
        Chord c2 = c; c2.t = nt;
        for (int k = i + 1; k < n; ++k) { auto& s = S.slots[(size_t) k]; double d = nt - s.t; s.t = nt; if (s.explicitIv) s.r = mod (s.r + d, 1200); }
        auto& sl = S.slots[(size_t) i];
        sl.tag = "Accordo pivot";
        sl.nota = u8 ("Accordo pivot: è ") + romanOf (c, base) + u8 (" nella tonalità di partenza e ") + romanOf (c2, nt) + " in " + nameC (nt) + " " + c.mo->nome + u8 (". Da qui il giro continua nella nuova tonalità.");
        return "Modulazione con pivot verso " + nameC (nt) + ".";
    }
    if (id == "scala")
    {
        auto* to = A.mode (versoModo);
        if (to == nullptr || to->id == c.mo->id) return "Scegli una scala diversa da quella attuale.";
        for (int k = i; k < n; ++k) if (! S.slots[(size_t) k].explicitIv) S.slots[(size_t) k].modo = to->id;
        juce::StringArray diff;
        for (int k = 0; k < 7; ++k) if (std::abs (degC (*c.mo, k) - degC (*to, k)) > 10) diff.add (nIt (degC (*c.mo, k), k) + u8 (" → ") + nIt (degC (*to, k), k));
        auto& sl = S.slots[(size_t) i];
        sl.tag = "Cambio di scala";
        sl.nota = "Stessa tonica, da qui " + to->nome + ". Cambiano " + (diff.isEmpty() ? juce::String ("nessuna nota") : diff.joinIntoString (", ")) + ": cambia il colore, non il centro.";
        return "Cambio di scala verso " + to->nome + ".";
    }
    return {};
}

juce::String Theory::varArm()
{
    double p = S.caso / 100.0;
    std::vector<int> cand;
    for (int i = 0; i < (int) S.slots.size(); ++i) if (! S.slots[(size_t) i].explicitIv) cand.push_back (i);
    if (cand.empty()) return "Nessun accordo da variare.";
    int i = pick (cand);
    auto& sl = S.slots[(size_t) i];
    S.sel = i;
    double x = rnd.nextDouble();
    int bar = (int) starts()[(size_t) i] + 1;
    if (x < 0.55 || p < 0.2)
    {
        std::vector<juce::String> opts = p < 0.35 ? std::vector<juce::String> { "7", "9", "11", "sus4", "add9", "69" }
                                                   : std::vector<juce::String> { "7", "9", "11", "13", "sus2", "sus4", "7sus4", "9sus4", "6", "69", "add9" };
        opts.erase (std::remove (opts.begin(), opts.end(), sl.ext), opts.end());
        sl.ext = pick (opts);
        return "Battuta " + juce::String (bar) + ": nuova estensione (" + findExt (sl.ext)->nome + ").";
    }
    if (x < 0.55 + 0.45 * (1 - p) || p < 0.6)
    {
        int d = sl.deg;
        sl.deg = imod (d + pick (std::vector<int> { 2, -2, 5, -5 }), 7);
        return "Battuta " + juce::String (bar) + ": grado sostituito (" + ROM[imod (d, 7)] + u8 (" → ") + ROM[sl.deg] + "), condivide note con il precedente.";
    }
    return mossa (pick (std::vector<juce::String> { "domsec", "tritono", "cromatico", "scambio" }));
}

// ---------------- ritmo ----------------
juce::String Theory::kitOf() const
{
    if (S.kit != "auto") return S.kit;
    auto c = R().cultura;
    return (c == "Occidentale" || c == "Afro-cubana" || c == "Groove") ? "elettronico" : "daf";
}

TrackMap Theory::baseTracks() const
{
    auto& r = R();
    if (r.hasTracks() && S.kit == "auto") return r.tracks;
    TrackMap t;
    auto add = [&] (const char* k, int p, float v) { t[k].push_back ({ p, v }); };
    auto kit = kitOf();
    juce::StringArray colpi = r.colpi;
    if (colpi.isEmpty())
        for (int p = 0; p < r.pulsazioni; ++p)
        {
            auto has = [&] (const char* k) { auto it = r.tracks.find (k); if (it == r.tracks.end()) return false; for (auto& h : it->second) if (h.step == p) return true; return false; };
            juce::String s;
            if (has ("kick")) s += "D";
            if (has ("snare") || has ("dafTek")) s += "T";
            if (s.isEmpty() && (has ("hat") || has ("shaker"))) s += "k";
            colpi.add (s);
        }
    for (int p = 0; p < colpi.size(); ++p)
    {
        int a = p < (int) r.accenti.size() ? r.accenti[(size_t) p] : 0;
        float v = 0.55f + 0.15f * a;
        for (auto ch : colpi[p])
        {
            if (ch == 'D') add (kit == "daf" ? "dafDum" : "kick", p, v);
            else if (ch == 'T') add (kit == "daf" ? "dafTek" : (a >= 1 ? "snare" : "ghost"), p, kit == "daf" ? v : (a >= 1 ? v : 0.45f));
            else if (ch == 'k') add (kit == "daf" ? "shaker" : "hat", p, 0.3f + 0.1f * a);
            else if (ch == 'C') add ("bell", p, v);
        }
    }
    if (kit == "elettronico" && ! t.count ("hat") && r.ppb <= 2)
        for (int p = 0; p < r.pulsazioni; ++p) add ("hat", p, 0.22f + 0.08f * r.accenti[(size_t) p]);
    return t;
}

TrackMap Theory::varianteRit()
{
    auto& r = R();
    double p = S.caso / 100.0;
    TrackMap v = baseTracks();
    juce::String light = v.count ("dafTek") ? "dafTek" : "ghost";
    for (int s = 0; s < r.pulsazioni; ++s)
    {
        if (r.accenti[(size_t) s] >= 1) continue;
        bool hit = false;
        for (auto& [k, hs] : v) for (auto& h : hs) if (h.step == s) hit = true;
        if (! hit && rnd.nextDouble() < 0.15 + 0.4 * p) v[light].push_back ({ s, (float) (0.2 + 0.25 * rnd.nextDouble()) });
        else if (hit && rnd.nextDouble() < 0.25 * p)
            for (auto& [k, hs] : v)
                if (k != "hat" && k != "shaker")
                    hs.erase (std::remove_if (hs.begin(), hs.end(), [s] (const Hit& h) { return h.step == s; }), hs.end());
    }
    if (p > 0.6 && v.count ("kick"))
    {
        std::vector<int> free;
        for (int s = 0; s < r.pulsazioni; ++s) if (r.accenti[(size_t) s] == 0) free.push_back (s);
        if (! free.empty()) v["kick"].push_back ({ pick (free), 0.6f });
    }
    return v;
}

void Theory::genRit()
{
    int L = loopLen();
    double p = S.caso / 100.0;
    S.ritVar.assign ((size_t) L, std::nullopt);
    for (int c = 1; c < L; ++c) if (rnd.nextDouble() < 0.25 + 0.6 * p) S.ritVar[(size_t) c] = varianteRit();
}

juce::String Theory::varRit()
{
    int L = loopLen();
    S.ritVar.resize ((size_t) L);
    int c = rnd.nextInt (L);
    S.ritVar[(size_t) c] = varianteRit();
    return "Ritmo: variato il ciclo " + juce::String (c + 1) + " (gli accenti forti restano).";
}

TrackMap Theory::tracksFor (int c) const
{
    if (c >= 0 && c < (int) S.ritVar.size() && S.ritVar[(size_t) c]) return *S.ritVar[(size_t) c];
    return baseTracks();
}

double Theory::swingV() const { return S.swing >= 0 ? S.swing : R().swing; }
double Theory::cycleDur() const { return R().pulsazioni * 60.0 / S.bpm / R().ppb; }
double Theory::pulseT (double p) const
{
    double pd = 60.0 / S.bpm / R().ppb;
    bool odd = R().ppb >= 2 && std::abs (p - std::round (p)) < 1e-6 && ((long) std::lround (p)) % 2 == 1;
    return p * pd + (odd ? (swingV() - 0.5) * 2 * pd : 0.0);
}

// ---------------- basso ----------------
double Theory::bassRoot (int i) const
{
    auto c = chordOf (i);
    if (S.basso == "pedale" || c.sl->bass == "pedale") return 0;
    double r = c.sl->bass == "rivolto" && c.iv.size() > 1 ? mod (c.r + c.iv[1], 1200) : c.r;
    if (r > 500) r -= 1200;
    return r;
}

double Theory::approach (double target, const Mode& mo, double t) const
{
    double best = -1e9; bool found = false;
    for (int k = -14; k < 14; ++k)
    {
        double x = t + degC (mo, k);
        double p = x - 1200 * std::ceil ((x - target) / 1200 + 0.0001);
        if (target - p > 40 && target - p <= 250 && (! found || p > best)) { best = p; found = true; }
    }
    return found ? best : target - 100;
}

std::vector<BassNote> Theory::bassNotes (int c) const
{
    auto& r = R();
    int P = r.pulsazioni;
    std::vector<BassNote> out;
    auto st = starts();
    if (! r.bass.empty())
    {
        for (auto& b : r.bass) { int i = slotAt (c + b.step / (double) P); out.push_back ({ (double) b.step, (double) b.len, b.cents + bassRoot (i), b.vel, i }); }
    }
    else
    {
        std::vector<int> hits;
        for (int p = 0; p < P; ++p) if (p == 0 || r.accenti[(size_t) p] >= 2) hits.push_back (p);
        for (size_t j = 0; j < hits.size(); ++j)
        {
            int p = hits[j], nxt = j + 1 < hits.size() ? hits[j + 1] : P;
            int i = slotAt (c + p / (double) P), a = r.accenti[(size_t) p];
            auto ch = chordOf (i);
            double fifth = -1;
            for (double x : ch.iv) { double m = mod (x, 1200); if (m >= 660 && m <= 740) { fifth = m; break; } }
            bool pos = std::abs (st[(size_t) i] - (c + p / (double) P)) < 1e-6 || p == 0;
            double add = (pos || a >= 3 || fifth < 0 || S.basso == "pedale") ? 0 : (j % 2 ? fifth : 1200);
            out.push_back ({ (double) p, (nxt - p) * 0.9, -1200 + bassRoot (i) + add, a >= 3 ? 1.0f : 0.8f, i });
        }
    }
    if (S.basso == "passaggi" && out.size() > 1)
    {
        int L = loopLen();
        for (size_t j = 0; j < out.size(); ++j)
        {
            double nextStart = j + 1 < out.size() ? out[j + 1].p : P;
            int ni = slotAt (mod (c + nextStart / P, L));
            if (ni != out[j].slot && out[j].p != 0)
            {
                auto ch = chordOf (ni);
                out[j].cents = approach (-1200 + bassRoot (ni), *ch.mo, ch.t);
            }
        }
    }
    return out;
}

// ---------------- voicing ----------------
namespace
{
    std::vector<double> fitRange (std::vector<double> v)
    {
        if (v.empty()) return v;
        while (v.back() > 2500) for (auto& x : v) x -= 1200;
        while (v.front() < -300) for (auto& x : v) x += 1200;
        return v;
    }
    const std::vector<double>& closeTo (const std::vector<std::vector<double>>& cands, const std::vector<double>* prev, double center)
    {
        size_t best = 0; double bs = 1e18;
        for (size_t j = 0; j < cands.size(); ++j)
        {
            auto& v = cands[j];
            double s = 0;
            if (prev && ! prev->empty())
            {
                for (double x : v) { double m = 1e9; for (double y : *prev) m = std::min (m, std::abs (x - y)); s += m; }
                s = s / v.size() + std::abs (v[0] - (*prev)[0]) * 0.15;
            }
            else s = std::abs ((v.front() + v.back()) / 2 - center);
            if (s < bs) { bs = s; best = j; }
        }
        return cands[best];
    }
}

std::vector<double> Theory::voiceOne (const Chord& c, const std::vector<double>* prev) const
{
    if (S.armo == "aperte")
    {
        double r = c.r > 600 ? c.r - 1200 : c.r, fv = 700;
        for (double x : c.iv) { double m = mod (x, 1200); if (m >= 560 && m <= 840) { fv = m; break; } }
        return { r, r + fv, r + 1200, r + 1200 + fv };
    }
    std::vector<double> pcs;
    for (double p : c.pcs) if (std::find (pcs.begin(), pcs.end(), p) == pcs.end()) pcs.push_back (p);
    if (pcs.size() >= 5) pcs.erase (std::remove (pcs.begin(), pcs.end(), c.r), pcs.end());   // senza radice: la suona il basso
    if (S.voc == "shell")
    {
        std::vector<double> q; for (double x : c.iv) q.push_back (mod (x, 1200));
        auto find = [&] (double lo, double hi) { for (double x : q) if (x >= lo && x <= hi) return x; return -1.0; };
        std::vector<double> g;
        double th = find (250, 450); if (th < 0) th = find (450, 640); if (th < 0) th = find (61, 249);
        double s7 = find (860, 1150);
        if (th >= 0) g.push_back (th);
        if (s7 >= 0) g.push_back (s7);
        if (c.iv.size() > 4) { double top = mod (c.iv.back(), 1200); if (std::find (g.begin(), g.end(), top) == g.end()) g.push_back (top); }
        if (g.size() < 2) g.push_back (mod (c.iv.size() > 2 ? c.iv[2] : 700, 1200));
        pcs.clear(); for (double x : g) pcs.push_back (mod (c.r + x, 1200));
    }
    if (S.voc == "quartale")
    {
        std::vector<double> scale; for (int k = -7; k < 28; ++k) scale.push_back (c.t + degC (*c.mo, k));
        double st = 500; for (double x : c.iv) { double m = mod (x, 1200); if (m >= 250 && m <= 640) { st = m; break; } }
        auto mk = [&] (double b)
        {
            std::vector<double> v { b };
            for (int n = 1; n < 4; ++n)
            {
                double want = v.back() + 500, q = scale[0];
                for (double x : scale) if (std::abs (x - want) < std::abs (q - want)) q = x;
                if (std::abs (q - want) > 60) q = want;
                v.push_back (q);
            }
            return v;
        };
        double b = mod (c.r + st, 1200);
        return fitRange (closeTo ({ mk (b - 1200), mk (b), mk (b + 1200) }, prev, 900));
    }
    std::sort (pcs.begin(), pcs.end());
    std::vector<std::vector<double>> cands;
    for (size_t rot = 0; rot < pcs.size(); ++rot)
    {
        std::vector<double> v; double last = -1e9;
        for (size_t j = 0; j < pcs.size(); ++j) { double x = pcs[(rot + j) % pcs.size()]; while (x <= last) x += 1200; v.push_back (x); last = x; }
        for (double o : { -1200.0, 0.0, 1200.0 })
        {
            std::vector<double> w; for (double x : v) w.push_back (x + o);
            if (w.front() >= 100 && w.back() <= 2400) cands.push_back (w);
        }
    }
    if (cands.empty()) { std::vector<double> v; for (double x : pcs) v.push_back (x < 100 ? x + 1200 : x); std::sort (v.begin(), v.end()); cands.push_back (v); }
    auto v = closeTo (cands, prev, 1000);
    if (S.voc == "aperto" && v.size() >= 4) { v[v.size() - 2] -= 1200; std::sort (v.begin(), v.end()); }
    return fitRange (v);
}

std::vector<std::vector<double>> Theory::voiceAll() const
{
    std::vector<std::vector<double>> out;
    const std::vector<double>* prev = nullptr;
    for (int i = 0; i < (int) S.slots.size(); ++i) { out.push_back (voiceOne (chordOf (i), prev)); prev = &out.back(); }
    return out;
}

// ---------------- melodia ----------------
int Theory::sub() const { int ppb = R().ppb; return ppb >= 3 ? 1 : ppb == 2 ? 2 : 4; }

double Theory::weight (int p) const
{
    auto& r = R();
    std::vector<int> gs; int a = 0; for (int g : r.gruppi) { gs.push_back (a); a += g; }
    bool groupStart = std::find (gs.begin(), gs.end(), p) != gs.end();
    return r.accenti[(size_t) p] * 2 + (groupStart ? 1 : 0) + (r.ppb >= 2 && p % 2 == 0 ? 0.5 : 0) + (r.ppb == 4 && p % 4 == 0 ? 0.5 : 0);
}

std::vector<MelNote> Theory::genMelCycle (int c, int prev, bool last)
{
    auto& r = R();
    int P = r.pulsazioni, u = sub();
    double p = S.caso / 100.0;
    std::vector<int> on;
    for (int s = 0; s < P; ++s)
    {
        double w = weight (s), th = S.dens == "sparsa" ? 4 : S.dens == "media" ? 2 : 0.5;
        if (w >= th || (S.dens != "sparsa" && w >= 0.5 && rnd.nextDouble() < 0.3) || (S.dens == "fitta" && rnd.nextDouble() < 0.4)) on.push_back (s * u);
        if (S.dens == "fitta" && u > 1 && rnd.nextDouble() < 0.2 + 0.2 * p) on.push_back (s * u + u / 2);
    }
    std::sort (on.begin(), on.end());
    on.erase (std::unique (on.begin(), on.end()), on.end());
    if (on.empty() || on[0] != 0) on.insert (on.begin(), 0);
    if (c % 2 == 1 && ! last && on.size() > 3) on.pop_back();   // respiro a fine frase

    std::vector<MelNote> notes;
    int d = prev;
    for (size_t j = 0; j < on.size(); ++j)
    {
        int x = on[j];
        double pos = c + x / (double) (P * u);
        int i = slotAt (pos);
        auto ch = chordOf (i);
        auto& mo = *ch.mo;
        double w = weight (x / u);
        int nd;
        if (last && j == on.size() - 1) nd = 0;
        else if (w >= 3 || j == 0)
        {
            std::vector<int> opts;
            for (int k = -2; k <= 9; ++k)
            {
                double pc = mod (ch.t + degC (mo, k), 1200);
                for (double q : ch.pcs) if (cdist (q, pc) < 30) { opts.push_back (k); break; }
            }
            // contorno: nella prima metà della frase tende a salire, nella seconda a scendere
            int dir = ((j / (double) on.size() < 0.5) != (c % 2 == 1)) ? 1 : -1;
            int target = d + dir * 2;
            auto cost = [&] (int k) { return std::abs (k - target) + (k == d ? 1.5 : 0) + rnd.nextDouble() * 1.2; };
            if (opts.empty()) nd = d + dir;
            else { nd = opts[0]; double bc = cost (nd); for (int k : opts) { double cc = cost (k); if (cc < bc) { bc = cc; nd = k; } } }
        }
        else nd = d + pick (p > 0.6 ? std::vector<int> { -2, -1, 1, 2 } : std::vector<int> { -1, 1, 1, -1, 2, -2 });
        nd = juce::jlimit (-2, 9, nd);
        int end = j + 1 < on.size() ? on[j + 1] : P * u;
        int len = end - x;
        if (len > u * 2 && rnd.nextDouble() < 0.4) len -= std::max (1, u / 2);
        double frac = std::abs (mod (degC (mo, nd), 100));
        bool micro = frac > 20 && frac < 80;
        juce::String orn;
        auto cult = mo.cultura;
        if (cult == "Persiana" || cult == "Araba" || cult == "Turca")
        {
            if (micro && len >= u * 2 && rnd.nextDouble() < 0.4) orn = "trill";
            else if (len >= u * 2) orn = "vib";
            else if (nd - d >= 2 && rnd.nextDouble() < 0.5) orn = "slideUp";
        }
        else if (cult == "Indiana") { if (len >= u * 2 && rnd.nextDouble() < 0.5) orn = "slideUp"; else if (len >= u * 3) orn = "vib"; }
        else if (len >= u * 3) orn = "vib";
        if (last && j == on.size() - 1 && d > 0) orn = "slideDown";
        notes.push_back ({ c, x, std::max (1, len), nd, mo.id, ch.t, orn });
        d = nd;
    }
    return notes;
}

void Theory::genMel()
{
    int L = loopLen();
    S.frasi.clear();
    int prev = 2;
    for (int c = 0; c < L; ++c)
    {
        auto f = genMelCycle (c, prev, c == L - 1);
        if (! f.empty()) prev = f.back().deg;
        S.frasi.push_back (f);
    }
}

juce::String Theory::varMel()
{
    int L = loopLen();
    if ((int) S.frasi.size() != L) genMel();
    int c = rnd.nextInt (L);
    int prev = (c > 0 && ! S.frasi[(size_t) c - 1].empty()) ? S.frasi[(size_t) c - 1].back().deg : 2;
    S.frasi[(size_t) c] = genMelCycle (c, prev, c == L - 1);
    return "Melodia: nuova frase nel ciclo " + juce::String (c + 1) + ".";
}

double Theory::melCents (const MelNote& n) const { return n.t + degC (modeOf (n.mo), n.deg) + melOct(); }

juce::String Theory::sorprendimi()
{
    auto& mo = M();
    if (! S.armLock)
    {
        auto g = giriList();
        genArm (pick (g).first);
        for (int k = 0; k < (int) std::lround (S.caso / 25.0); ++k) varArm();
    }
    if (! S.ritLock)
    {
        std::vector<juce::String> pool;
        for (auto& x : mo.ritmi) if (A.rhythm (x)) pool.push_back (x);
        for (auto x : { "triphop", "persiana", "funk" }) if (A.rhythm (x)) pool.push_back (x);
        S.rit = pick (pool); S.bpm = R().bpm; S.swing = -1;
        if (! S.armLock) for (auto& s : S.slots) s.dur = scaleDur (s.dur);
        fixLen();
        genRit();
    }
    if (! S.melLock)
    {
        S.timbro = pick (std::vector<juce::String> { "arco", "ney", "synth" });
        S.dens = pick (std::vector<juce::String> { "sparsa", "media", "media", "fitta" });
    }
    refresh (true);
    return "Nuova combinazione: " + giroTxt().upToFirstOccurrenceOf (":", false, false) + u8 (" · ") + R().nome + u8 (" · ") + S.timbro + ".";
}

void Theory::refresh (bool regen)
{
    if (S.slots.empty()) genArm();
    fixLen();
    int L = loopLen();
    if ((int) S.ritVar.size() != L) S.ritVar.resize ((size_t) L);
    if (regen && (! S.melLock || (int) S.frasi.size() != L)) genMel();
    if ((int) S.frasi.size() != L) genMel();
}
} // namespace bottega
