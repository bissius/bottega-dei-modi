// Prova senza finestra del plugin intero: audio interno, MIDI in uscita, interfaccia.
// Uso: BottegaSmoke [cartella per la schermata]
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    int fails = 0;
    auto check = [&] (bool ok, const juce::String& what) { std::cout << (ok ? "ok   " : "FAIL ") << what << std::endl; if (! ok) ++fails; };

    BottegaProcessor p;
    const double sr = 48000; const int bs = 512;
    p.setPlayConfigDetails (0, 2, sr, bs);
    p.prepareToPlay (sr, bs);

    auto run = [&] (double seconds, int& midiEvents, int& bends, double& rms, double& peak)
    {
        juce::AudioBuffer<float> buf (2, bs);
        juce::MidiBuffer midi;
        double sum = 0; long n = 0; midiEvents = 0; bends = 0; peak = 0;
        for (int b = 0; b < (int) (seconds * sr / bs); ++b)
        {
            p.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < bs; ++i) { float x = buf.getSample (ch, i); sum += x * x; ++n; peak = std::max (peak, (double) std::abs (x)); }
            for (auto m : midi) { ++midiEvents; if (m.getMessage().isPitchWheel()) ++bends; }
        }
        rms = std::sqrt (sum / std::max (1L, n));
    };

    int ev, bends; double rms, peak;
    p.setPlaying (true);
    run (12.0, ev, bends, rms, peak);
    std::cout << "suono interno: rms " << rms << " picco " << peak << std::endl;
    check (rms > 0.005 && peak <= 1.0 && std::isfinite (rms), "suono interno udibile e senza clip");
    check (ev == 0, "nessun MIDI se l'uscita MIDI è spenta");

    { juce::ScopedLock sl (p.lock); p.song.midiOut = true; p.song.suonoInterno = false; }
    run (8.0, ev, bends, rms, peak);
    std::cout << "MIDI: " << ev << " eventi, " << bends << " pitch bend" << std::endl;
    check (ev > 50 && bends > 10, "MIDI in uscita con pitch bend");
    check (rms < 1e-3, "suono interno spento quando l'uscita è solo MIDI");
    p.setPlaying (false);
    run (0.5, ev, bends, rms, peak);

    // tutti i modi con tutti i ritmi, qualche secondo ciascuno, suono interno
    { juce::ScopedLock sl (p.lock); p.song.midiOut = false; p.song.suonoInterno = true; }
    bool allFinite = true;
    for (auto& m : p.theory.A.modi)
    {
        { juce::ScopedLock sl (p.lock); p.song.modo = m.id; p.theory.genArm(); }
        p.changed (true);
        p.setPlaying (true);
        run (1.5, ev, bends, rms, peak);
        if (! std::isfinite (rms) || peak > 1.0) { allFinite = false; std::cout << "  problema in " << m.id << std::endl; }
        p.setPlaying (false);
        run (0.2, ev, bends, rms, peak);
    }
    check (allFinite, "tutti i modi suonano senza NaN né clip");

    // interfaccia: schermate a due larghezze
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        juce::File dir = argc > 1 ? juce::File (juce::String (argv[1])) : juce::File::getCurrentWorkingDirectory();
        for (int w : { 1060, 480 })
        {
            ed->setSize (w, 860);
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::PNGImageFormat png; auto f = dir.getChildFile ("bottega-" + juce::String (w) + ".png");
            f.deleteFile();
            juce::FileOutputStream os (f);
            check (os.openedOk() && png.writeImageToStream (img, os), "schermata " + f.getFileName());
        }
        // anche il contenuto scorrevole per intero
        ed->setSize (1060, 860);
        for (auto* c : ed->getChildren())
            if (auto* v = dynamic_cast<juce::Viewport*> (c))
                if (auto* content = v->getViewedComponent())
                {
                    auto img = content->createComponentSnapshot (content->getLocalBounds(), true, 1.0f);
                    juce::PNGImageFormat png; auto f = dir.getChildFile ("bottega-corsie.png");
                    f.deleteFile();
                    juce::FileOutputStream os (f);
                    png.writeImageToStream (img, os);
                }
    }

    std::cout << (fails ? "ALCUNE PROVE FALLITE" : "TUTTO OK") << std::endl;
    return fails ? 1 : 0;
}
