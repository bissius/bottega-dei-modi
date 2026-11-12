# Bottega dei Modi

Strumento per sperimentare con modi di culture diverse su tre corsie: **armonia**, **ritmo** e **melodia**. Funziona come app autonoma (standalone) e come plugin **VST3** (e **AU** su Mac) dentro Studio One / Fender Studio Pro.

- 16 modi con microtoni (persiani, arabi, indiani, occidentali…) e 17 cicli ritmici, ognuno con le sue fonti (`data/archivio.json`).
- 12 toniche più la tonica libera in Hz.
- Suono interno (Rhodes, pad, basso, bordone, percussioni, voce ad arco / ney / synth, riverbero) **e/o** uscita MIDI con un canale e un pitch bend per ogni nota, così i quarti di tono arrivano intatti a qualsiasi synth.
- Preset in JSON, Annulla, Sorprendimi, manopola del caso, blocco di ogni corsia.

La specifica completa è in [`docs/specifica.md`](docs/specifica.md); il prototipo web da cui nasce è in `docs/prototipo-web-v04.html`.

## Scaricare la versione pronta

Ogni modifica al repository fa partire una build automatica (GitHub → scheda **Actions** → ultima esecuzione verde → sezione **Artifacts**):

| Pacchetto | Contiene |
|---|---|
| `Bottega-dei-Modi-Windows` | `Bottega dei Modi.vst3`, `Bottega dei Modi.exe` |
| `Bottega-dei-Modi-Mac` | `.vst3`, `.component` (AU), `.app` (Apple Silicon e Intel) |
| `Bottega-dei-Modi-Linux` | `.vst3` e standalone |

### Installazione

**Windows**: copia `Bottega dei Modi.vst3` in `C:\Program Files\Common Files\VST3\`. La versione autonoma si apre col doppio clic sul `.exe` (Windows può mostrare l'avviso SmartScreen perché il file non è firmato: *Ulteriori informazioni → Esegui comunque*).

**Mac**: copia il `.vst3` in `~/Library/Audio/Plug-Ins/VST3/` e il `.component` in `~/Library/Audio/Plug-Ins/Components/`. I file non sono firmati da Apple, quindi la prima volta dal Terminale:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Bottega dei Modi.vst3"
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Bottega dei Modi.component"
```

Poi in Studio One: *Opzioni → Posizioni → Plug-in VST* → **Ripeti scansione**, se non compare da solo.

## Usarla in Studio One

1. Trascina **Bottega dei Modi** (categoria Strumenti) nella song: crea una traccia strumento.
2. Premi Play in Studio One: il loop parte allineato alla griglia e segue il **tempo della song** (in alto a destra compare *SINCRONIZZATO ALLA DAW*). Da sola, con il tasto ▶ del plugin, usa i BPM scritti nel plugin.
3. Lo stato (modo, giro, ritmo, mixer…) viene salvato dentro la song. Con **Salva preset / Apri preset** lo porti tra song diverse.

### Suonare un altro synth con il MIDI della Bottega

1. In alto, **Uscita → MIDI** (solo MIDI) oppure **Entrambi**.
2. Scegli il **Pitch bend del synth**: deve essere **uguale** all'intervallo di pitch bend impostato sul synth che riceve. ±48 è lo standard MPE; ±2 va bene solo per synth che non si possono regolare, ma i microtoni grandi verranno tagliati.
3. Il synth che riceve deve suonare **ogni canale separatamente** (modalità MPE o multi-canale): ogni nota arriva su un canale suo, da 2 a 16, con il suo pitch bend. La batteria arriva sul **canale 10** con le note General MIDI (cassa 36, rullante 38, charleston 42…).
4. In Studio One crea una seconda traccia strumento con il synth che vuoi e, come ingresso, scegli l'uscita della Bottega. *Da verificare sulla tua versione di Studio One / Fender Studio Pro*: se l'uscita del plugin non compare tra gli ingressi, usa la versione autonoma con una porta MIDI virtuale (loopMIDI su Windows, Driver IAC su Mac) e scegli quella porta come ingresso della traccia.

Limite noto della v0.5: gli scivolamenti della melodia (glissando) via MIDI arrivano come salto alla nota d'arrivo, non come bend graduale. Il suono interno li fa per intero.

## Compilare dai sorgenti

Serve CMake ≥ 3.22 e un compilatore C++17 (Visual Studio 2022, Xcode, oppure gcc/clang). JUCE 8.0.10 viene scaricato da solo.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

I risultati sono in `build/Bottega_artefacts/Release/`. Le prove: `BottegaTests` (nucleo musicale, confrontato con il prototipo web) e `BottegaSmoke` (suona il plugin senza finestra, controlla audio e MIDI, salva le schermate dell'interfaccia).

## Struttura

| Cartella | Cosa c'è |
|---|---|
| `core/` | Nucleo musicale senza interfaccia: archivio, teoria (accordi, analisi, mosse, ritmo, basso, voicing, melodia), stato e preset, sequencer, allocazione dei canali MIDI |
| `synth/` | Motore audio interno |
| `plugin/` | Processore del plugin e interfaccia |
| `data/` | `archivio.json`: modi, ritmi, giri, mosse, con fonti |
| `tests/` | Prove automatiche |
