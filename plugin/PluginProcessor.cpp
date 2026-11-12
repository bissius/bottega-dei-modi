#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace bottega;

BottegaProcessor::BottegaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    juce::ScopedLock sl (lock);
    theory.genArm ("r:pedale");
    theory.genRit();
    theory.refresh (true);
    voicings = theory.voiceAll();
}

bool BottegaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto o = layouts.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono();
}

void BottegaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock);
    lastTimbre = song.timbro;
    engine.setMelTimbre (song.timbro);
}

void BottegaProcessor::snapshot()
{
    juce::ScopedLock sl (lock);
    history.push_back (song.toJson());
    if (history.size() > 60) history.erase (history.begin());
}

bool BottegaProcessor::undo()
{
    juce::ScopedLock sl (lock);
    if (history.empty()) return false;
    auto s = Song::fromVar (juce::JSON::parse (history.back()));
    history.pop_back();
    song = s;
    theory.refresh (false);
    voicings = theory.voiceAll();
    needDrone = true;
    return true;
}

void BottegaProcessor::changed (bool regen)
{
    juce::ScopedLock sl (lock);
    theory.refresh (regen);
    voicings = theory.voiceAll();
    needDrone = true;
}

void BottegaProcessor::loadSong (const Song& s)
{
    snapshot();
    juce::ScopedLock sl (lock);
    song = s;
    theory.refresh (false);
    voicings = theory.voiceAll();
    needDrone = true;
}

void BottegaProcessor::preview (int slot) { previewSlot = slot; needPreview = true; }

void BottegaProcessor::startTransport (double now, const juce::AudioPlayHead::PositionInfo* pos)
{
    pending.clear();
    cycleStarts.clear();
    cycleCount = 0;
    nextCycleT = now + 0.05;
    if (pos != nullptr && pos->getPpqPosition())
    {
        // allinea il loop alla posizione della DAW: il ciclo dura (pulsazioni/ppb) quarti
        double bpc = theory.beatsPerCycle();
        double cyc = *pos->getPpqPosition() / bpc;
        cycleCount = (long) std::floor (cyc);
        nextCycleT = now - (cyc - std::floor (cyc)) * theory.cycleDur();
    }
    alloc.reset();
}

void BottegaProcessor::stopTransport (juce::MidiBuffer& midi)
{
    pending.clear();
    for (auto& o : offs) midi.addEvent (juce::MidiMessage::noteOff (o.channel, o.note), 0);
    offs.clear();
    if (melMidi[0] > 0) { midi.addEvent (juce::MidiMessage::noteOff (melMidi[0], melMidi[1]), 0); melMidi[0] = -1; }
    alloc.reset();
    engine.setDrone (false, song.hz, 0, 700);
    engine.allNotesOff();
    engine.setVinyl (false);
    playPos = -1;
}

void BottegaProcessor::dispatch (const Pending& p, double blockStart, int numSamples, juce::MidiBuffer& midi)
{
    if (song.suonoInterno) engine.schedule (p.e, p.when, p.hz);
    if (! song.midiOut) return;
    int off = juce::jlimit (0, numSamples - 1, (int) ((p.when - blockStart) * sr));
    auto vel = (juce::uint8) juce::jlimit (1, 127, (int) (p.e.vel * 100));
    if (p.e.inst >= Inst::Kick)
    {
        int n = ChannelAllocator::drumNote ((int) p.e.inst - (int) Inst::Kick);
        midi.addEvent (juce::MidiMessage::noteOn (10, n, vel), off);
        offs.push_back ({ p.when + 0.05, 10, n, -1 });
        return;
    }
    if (p.e.inst == Inst::MelRelease)
    {
        if (melMidi[0] > 0) { midi.addEvent (juce::MidiMessage::noteOff (melMidi[0], melMidi[1]), off); alloc.noteOff (melMidi[2]); melMidi[0] = -1; }
        return;
    }
    int stolen = -1;
    auto n = alloc.noteOn (p.hz, song.pitchBendRange, stolen);
    if (stolen >= 0)
        for (auto it = offs.begin(); it != offs.end(); ++it)
            if (it->id == stolen) { midi.addEvent (juce::MidiMessage::noteOff (it->channel, it->note), off); offs.erase (it); break; }
    midi.addEvent (juce::MidiMessage::pitchWheel (n.channel, n.bend), off);
    if (p.e.inst == Inst::Mel)
    {
        if (melMidi[0] > 0) { midi.addEvent (juce::MidiMessage::noteOff (melMidi[0], melMidi[1]), off); alloc.noteOff (melMidi[2]); }
        midi.addEvent (juce::MidiMessage::noteOn (n.channel, n.note, (juce::uint8) 96), off);
        melMidi[0] = n.channel; melMidi[1] = n.note; melMidi[2] = n.id;
        return;
    }
    midi.addEvent (juce::MidiMessage::noteOn (n.channel, n.note, vel), off);
    offs.push_back ({ p.when + std::max (0.03, p.e.dur), n.channel, n.note, n.id });
}

void BottegaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int num = buffer.getNumSamples();
    buffer.clear();
    midi.clear();
    const double now = engine.now(), blockDur = num / sr;

    // trasporto: la DAW comanda se suona, altrimenti il tasto Play
    juce::Optional<juce::AudioPlayHead::PositionInfo> pos;
    if (auto* ph = getPlayHead()) pos = ph->getPosition();
    bool hostPlay = pos && pos->getIsPlaying();
    hostSynced = hostPlay;
    bool play = uiPlay.load() || hostPlay;

    const juce::ScopedTryLock tl (lock);
    if (tl.isLocked())
    {
        internal = song.suonoInterno;
        if (hostPlay && pos->getBpm()) song.bpm = *pos->getBpm();
        engine.setMix (song, song.bpm, theory.R().duck);
        if (song.timbro != lastTimbre) { lastTimbre = song.timbro; engine.setMelTimbre (song.timbro); }

        if (needPreview.exchange (false))
        {
            int i = juce::jlimit (0, (int) voicings.size() - 1, previewSlot.load());
            double t = now + 0.02;
            if (i >= 0)
            {
                for (double c : voicings[(size_t) i]) { Event e; e.inst = Inst::Rhodes; e.dur = 1.8; e.vel = 0.8f; e.cents = c; engine.schedule (e, t, hzOf (c)); }
                Event b; b.inst = Inst::Bass; b.dur = 1.8; b.cents = -1200 + theory.bassRoot (i); engine.schedule (b, t, hzOf (b.cents));
            }
        }

        if (play && ! wasPlaying) { startTransport (now, hostPlay ? &*pos : nullptr); needDrone = true; }
        if (! play && wasPlaying) stopTransport (midi);
        wasPlaying = play;

        if (play)
        {
            if (needDrone.exchange (false))
            {
                auto& mo = theory.M();
                engine.setDrone (true, song.hz, mo.bordone[0], mo.bordone[1]);
            }
            bool vin = theory.R().vinile;
            engine.setVinyl (vin);
            const int L = theory.loopLen();
            while (nextCycleT < now + blockDur + 0.12)
            {
                int c = (int) (((cycleCount % L) + L) % L);
                for (auto& e : realizeCycle (theory, c, voicings))
                {
                    double when = nextCycleT + e.time;
                    if (when >= now - 1e-4)
                    {
                        double hz = e.inst == Inst::Bass || e.inst == Inst::Rhodes || e.inst == Inst::Pad || e.inst == Inst::Mel ? hzOf (e.cents) : 0;
                        pending.push_back ({ when, e, hz });
                    }
                }
                cycleStarts.push_back ({ nextCycleT, c });
                if (cycleStarts.size() > 8) cycleStarts.erase (cycleStarts.begin());
                nextCycleT += theory.cycleDur();
                ++cycleCount;
            }
            std::stable_sort (pending.begin(), pending.end(), [] (const Pending& a, const Pending& b) { return a.when < b.when; });
            // posizione per la testina dell'interfaccia
            double cd = theory.cycleDur();
            for (auto& [t0, c] : cycleStarts) if (t0 <= now) playPos = c + juce::jlimit (0.0, 0.999, (now - t0) / cd);
        }
    }

    // eventi di questo blocco
    size_t k = 0;
    for (; k < pending.size() && pending[k].when < now + blockDur; ++k) dispatch (pending[k], now, num, midi);
    pending.erase (pending.begin(), pending.begin() + (long) k);
    for (auto it = offs.begin(); it != offs.end();)
    {
        if (it->when < now + blockDur)
        {
            midi.addEvent (juce::MidiMessage::noteOff (it->channel, it->note), juce::jlimit (0, num - 1, (int) ((it->when - now) * sr)));
            if (it->id >= 0) alloc.noteOff (it->id);
            it = offs.erase (it);
        }
        else ++it;
    }
    playingNow = play;

    // solo MIDI: il motore interno resta muto (anche bordone, vinile e code del riverbero)
    // (il motore gira comunque: è anche l'orologio del sequencer)
    engine.render (buffer, 0, num);
    if (! internal) buffer.clear();
}

void BottegaProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ScopedLock sl (lock);
    auto json = song.toJson();
    dest.append (json.toRawUTF8(), json.getNumBytesAsUTF8());
}

void BottegaProcessor::setStateInformation (const void* data, int size)
{
    auto json = juce::String::fromUTF8 ((const char*) data, size);
    auto v = juce::JSON::parse (json);
    if (! v.isObject()) return;
    {
        juce::ScopedLock sl (lock);
        song = Song::fromVar (v);
        theory.refresh (false);
        voicings = theory.voiceAll();
        needDrone = true;
    }
    if (onExternalChange) juce::MessageManager::callAsync ([this] { if (onExternalChange) onExternalChange(); });
}

juce::AudioProcessorEditor* BottegaProcessor::createEditor() { return new BottegaEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BottegaProcessor(); }
