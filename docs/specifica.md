# Tool modale multiculturale – specifica di base

Documento di lavoro per il progetto Music Coach. Raccoglie le decisioni prese a voce l'11 novembre 2026 per lo strumento di sperimentazione di sequenze armoniche, melodiche e ritmiche di culture musicali diverse.

## Obiettivo

Uno strumento creativo che permetta di sperimentare strutture musicali di molte culture (occidentale, araba, turca, persiana, indiana, balcanica, greca, cinese, giapponese, africana, andina, nativi americani, tradizionale italiana ed europea, gamelan, ecc.), ascoltarle subito, incrociarle tra loro, salvare gli esperimenti e portarli in Studio One.

Principio chiave: in molte culture non esiste l'armonia per accordi. L'unità organizzativa è melodica (maqam, raga, modi pentatonici) o ritmica (tala, poliritmie africane). L'armonia è quindi un livello a parte, un'interpretazione che il tool propone sopra la melodia, non il punto di partenza.

## Architettura: tre motori e un archivio

1. **Motore melodico**: scale e percorsi melodici (gerarchia delle note, nota di riposo, frasi tipiche, differenze salita/discesa).
2. **Motore armonico**: tre modalità di armonizzazione selezionabili: accordi diatonici per terze; quinte e quarte aperte (reggono anche fuori dal temperamento); solo bordone più linea di basso.
3. **Motore ritmico**: cicli con accenti, suddivisioni, timeline e poliritmie (tala, clave, metri asimmetrici balcanici, samai arabo). Completo già nella prima versione.
4. **Archivio comune**: strutture di tutte le culture, in dati curati a mano (JSON), con fonti citate, arricchito nel tempo.

I tre motori sono corsie indipendenti che si possono combinare liberamente, anche tra culture diverse (es. raga indiano su ciclo africano a 12 pulsazioni con bordone).

## Sincronizzazione tra corsie

Due assi comuni:

- **Tempo**: un ciclo master (es. tala a 16) a cui le altre corsie si agganciano: cambio accordo sulle suddivisioni del ciclo, frasi melodiche che partono e chiudono sugli accenti forti.
- **Intonazione**: una tonica comune in hertz; ogni struttura è espressa in cents rispetto a quella, così maqam e accordi condividono la stessa fondamentale.

Quando due corsie confliggono (es. nota melodica a un quarto di tono dall'accordo) il tool lo segnala e propone l'adattamento, senza mascherarlo.

## Logica musicale

- Dal modo si ricavano gli accordi su ogni grado, marcando quelli che contengono la nota caratteristica del modo (es. sesto grado del dorico, secondo del frigio). Le sequenze suggerite alternano tonica e accordi caratteristici, evitando le cadenze che tirano verso il maggiore.
- Ogni accordo è classificato per funzione e per presenza della nota caratteristica.
- **Bordone**: solo tonica e quinta, mai la terza. Gli accordi con un semitono o quarto di tono adiacente al bordone sono marcati come tensione, da usare di passaggio.
- **Basso**: fondamentale sul battere, passaggi diatonici verso l'accordo successivo.

## Modello dati dell'archivio

Per ogni struttura:

- cultura di origine e nome
- intervalli in cents rispetto alla tonica
- gerarchia delle note (tonica, nota di riposo, note cardine, nota caratteristica)
- regole melodiche (salita/discesa, frasi tipiche, note di passaggio)
- bordone tipico
- cicli ritmici associati
- spiegazione in testo (perché suona così, come si usa)
- fonti

## Prima lista di strutture per cultura (esempi)

- **Occidentale**: i sette modi diatonici, minore armonica e melodica, modi della minore melodica.
- **Araba**: maqam Rast, Bayati, Hijaz, Saba, Nahawand, Kurd; ritmi maqsum, samai thaqil (10/8), wahda.
- **Turca**: makam Hicaz, Uşşak, Nihavend, Segah con le kome; usul aksak (9/8).
- **Persiana**: dastgah Shur, Mahur, Segah, Homayun.
- **Indiana**: raga Bhairav, Yaman, Bhimpalasi, Kafi, Darbari; tala Teental (16), Jhaptal (10), Rupak (7), Dadra (6).
- **Balcanica**: modi frigio dominante e doppia armonica; metri 7/8, 9/8, 11/8 (kopanitsa), 15/8.
- **Greca**: dromoi del rebetiko (Hitzaz, Ousak, Rast, Niavent); ritmi zeibekiko (9/4), tsamiko (3/4), kalamatianos (7/8).
- **Cinese**: cinque modi pentatonici (Gong, Shang, Jue, Zhi, Yu).
- **Giapponese**: scale In e Yo, Hirajoshi, Kumoi, Ryo e Ritsu del gagaku.
- **Indonesiana**: slendro e pelog del gamelan di Giava e Bali, cicli colotomici.
- **Africana**: pentatoniche e eptatoniche dell'Africa occidentale, qenet etiopi (Tizita, Bati, Ambassel, Anchihoye); timeline a 12 pulsazioni, clave 3-2 e 2-3, poliritmie 3 contro 2 e 4 contro 3.
- **Andina**: pentatonica minore, huayno, ritmi in 6/8 e 2/4 sovrapposti.
- **Nativi americani**: pentatoniche e tetratoniche, canto con pulsazione di tamburo regolare, discese melodiche terrazzate.
- **Italiana tradizionale**: modi lidio e misolidio del canto popolare, tarantella e pizzica in 6/8 e 12/8, canto a tenore sardo con bordone.
- **Europea popolare**: celtica (dorico, misolidio), nordica (polska in 3/4 asimmetrico), flamenco (modo frigio andaluso e cadenza andalusa, compás a 12).

## Interfaccia e UX

Tre corsie orizzontali, melodia, armonia e ritmo, come un mini sequencer. Ogni corsia ha un selettore di cultura e struttura; sotto, un pannello che spiega in due righe cosa si sta ascoltando e perché. L'audio gira sempre in loop: si cambia una cosa e si sente subito. Per ogni corsia un tasto "variazione" che propone un'alternativa coerente e un tasto per bloccare ciò che piace.

Stile sulla falsariga di Teenage Engineering: estetica minimalista scandinava, linee pulite e colori decisi, dove l'estetica riflette la funzione; i vincoli trasformati in tratti distintivi, con carattere monospazio come sull'OP-1 e pensiero modulare. Nella pratica: tema monocromo con un colore per corsia, carattere monospazio, controlli grandi e pochi, un'informazione per volta, niente menu annidati, ogni corsia che si comporta come un modulo hardware. Riferimenti: [analisi del design Teenage Engineering](https://medium.com/@ihorkostiuk.design/the-product-design-of-teenage-engineering-why-it-works-71071f359a97), [Constraints as Aesthetic](https://blakecrosley.com/guides/design/teenage-engineering), [app ufficiali Teenage Engineering](https://teenage.engineering/apps), [OP-1 field](https://teenage.engineering/products/op-1), [OP-Z](https://teenage.engineering/products/op-z).

## Funzioni creative aggiuntive

- **Ingresso dell'utente**: suonare una linea di basso o pochi accordi (MIDI o audio); il tool analizza ciò che è stato suonato e propone in quali strutture di altre culture si incastra.
- **Traduci**: prendere una sequenza occidentale e farla reinterpretare in un maqam, un raga o un altro sistema, mostrando cosa si perde e cosa si trasforma.
- **Ornamenti**: il motore melodico applica gli ornamenti tipici di ogni cultura (gamaka indiani, tremoli e quarti di tono arabi, melismi balcanici), perché sono metà del carattere.
- **Manopola del caso**: da prudente a selvaggia, regola quanto le variazioni proposte si allontanano dalla struttura di partenza; la serendipità è parte del gioco.

## Scene e struttura del pezzo

L'arrangiamento completo (durate, dinamiche, automazioni) resta nella DAW: il tool non rifà Studio One. Il tool gestisce però un livello leggero di **scene**, come i clip di Ableton: ogni scena è un preset (intro, strofa, ritornello, special, finale) e le scene si concatenano in una catena.

Il valore sta nelle **transizioni**: passare da una strofa in raga a un ritornello in maqam è un problema di modulazione tra culture. Il tool suggerisce quali note in comune usare come ponte, quali accordi o cicli ritmici fanno da cerniera, e segnala i salti troppo bruschi. Esportazione della catena intera o delle singole scene verso la DAW.

## Riferimenti sonori e priorità

Il primo prototipo nel progetto Music Coach ha molte funzioni ma non suona ancora come voluto. Prima di sviluppare altro, il tool deve riprodurre un suono di riferimento, e solo quello.

Riferimenti indicati: la sigla della serie Tehran di Apple TV (Mark Eliyahu, kamanche iraniano ed elettronica); smooth jazz e funk jazz con accordi complessi, estesi e sospesi; trip hop (Massive Attack, forse Portishead).

Filo comune: un centro tonale fermo con bordone o basso ipnotico, e il colore che arriva da sopra: scala persiana e timbro dello strumento in Eliyahu, campione e ritmo lento nel trip hop, estensioni degli accordi nel jazz. Priorità del tool: un groove fermo su una tonica, accordi ricchi (none, undicesime, sospensioni) e una melodia che prende le note fuori dal temperamento. Il resto dell'archivio viene dopo.

## Analisi funzionale e catalogo delle mosse

Ogni accordo in corsia mostra grado romano, funzione (tonica, sottodominante, dominante) e la sua nota speciale (es. "qui c'è la sensibile che tira verso la tonica"). Un catalogo di mosse prese da stili diversi, applicabili con un tasto e spiegate in due righe: dominante secondaria, accordo pivot per modulare, scambio modale dal minore, sostituzione di tritono, salita cromatica del basso, salita di mezzo tono, cambio di scala sulla stessa tonica.

## Archivio di analisi di pezzi esistenti

Un archivio di studi su pezzi già esistenti, che col tempo alimenta i suggerimenti ("questa mossa l'hai vista in quel pezzo"). Le fonti si fondono in ordine di affidabilità e il tool indica quanto è sicuro di ogni accordo:

1. **Da Studio One**: accordi estratti dalla Chord Track e stem (il basso isolato dà la fondamentale).
2. **Inserimento manuale** degli accordi o importazione MIDI.
3. **Cataloghi online**: siti di accordi e tab; metadati da DJ (tonalità, BPM, energia, mood, ruota di Camelot) da servizi tipo Tunebat o Mixed In Key.
4. **Analisi dall'audio grezzo** come fallback: risultati mediocri, specie con il microtonale.

## Archivio delle culture collegato alle analisi

L'archivio delle culture non è storico ma grammaticale: per ogni cultura raccoglie teoria, tecniche, frasi standard ed esempi, e si alimenta con i pezzi analizzati. Tre livelli collegati:

1. **Cultura**: nome, area, sistemi di intonazione e ritmici, strumenti tipici, spiegazione in testo.
2. **Stilema**: l'unità centrale. Una cella ritmica, una frase melodica tipica, una cadenza, un ornamento, un modo di muovere il basso. Ogni stilema ha la sua regola grammaticale (quando e come si usa), i parametri tecnici (intervalli in cents, ciclo ritmico, accenti) e gli esempi dai pezzi in cui è stato trovato.
3. **Pezzo analizzato**: evidenza. Ogni analisi viene taggata per cultura e stile; il tool propone di promuovere a stilema i pattern che ricorrono in più pezzi.

In fase creativa gli stilemi di una cultura si richiamano insieme (es. pizzica pugliese: ritmo in 6/8 o 12/8 più la frase melodica tipica), e si possono incrociare con quelli di un'altra cultura.

## Preset ed esperimenti

Ogni esperimento è un preset JSON con le tre corsie, tonica, tempo e note dell'utente. Preferiti, tag per cultura e umore, cronologia delle variazioni per tornare indietro e ripartire da un ramo diverso. Esportazione in MIDI (con pitch bend per nota o MPE) e in audio per Studio One.

## Audio e suoni

Motore audio interno con accordatura flessibile (supporto Scala, MTS-ESP) per suonare davvero i quarti di tono. Set di suoni volutamente piccolo e neutro: pad, piano elettrico, basso, voce tipo flauto o corda, percussioni base, bordone. Estensioni possibili: caricamento di SoundFont o campioni SFZ; ospitare un altro plugin dentro l'app (JUCE lo permette).

## Tecnologia

- **JUCE in C++**: stesso codice per app standalone e plugin VST3/AU. La logica musicale (archivio, motori, preset) vive in un nucleo indipendente dall'interfaccia.
- Nel plugin cambiano solo due cose: il tempo si prende dalla DAW, e i preset si salvano anche nello stato del progetto.
- MIDI in uscita con pitch bend per canale o MPE per gli intervalli non temperati.

## Progetti GitHub da cui partire

- [Surge tuning library e tuning-workbench-synth](https://github.com/surge-synthesizer/tuning-workbench-synth): lettura di file Scala, synth microtonale di test.
- [Tune-Up-MIDI (vsicurella)](https://github.com/vsicurella/Tune-Up-MIDI): converte MIDI mono-canale in multicanale con pitch bend.
- [ChordGeneratorandArpeggiator](https://github.com/kaveenaram/ChordGeneratorandArpeggiator): piccolo plugin JUCE di generazione accordi, utile per la struttura.
- [awesome-juce](https://github.com/sudara/awesome-juce): lista curata di moduli e plugin JUCE.

Non esiste un progetto che faccia già tutto: si parte dallo scheletro JUCE innestando la libreria Surge e il modulo di pitch bend, e si scrive da zero la logica musicale, che è il cuore del tool.

## Piano di sviluppo

**Versione zero: il suono di riferimento (aggiornamento del 13 novembre 2026)**

Prima di tutto il resto, il tool deve produrre un suono che convince: un groove fermo su una tonica con basso ipnotico e bordone, accordi ricchi (none, undicesime, sospensioni) con analisi funzionale visibile, e una melodia in una scala persiana o araba con i quarti di tono. Criterio di successo: un loop di 8 battute che ricorda i riferimenti (Eliyahu, trip hop, funk jazz). Finché questo non suona bene, nessun'altra funzione.

**Prima versione (standalone)**

1. Motore audio microtonale con bordone, basso e pad.
2. Motore ritmico completo (cicli, accenti, suddivisioni, poliritmie, ciclo master).
3. Motore armonico con i modi occidentali più un maqam e un raga.
4. Motore melodico di base.
5. Salvataggio e richiamo dei preset.
6. Interfaccia a tre corsie in stile Teenage Engineering.

**Versioni successive**

- Plugin VST3/AU per Studio One.
- Ampliamento dell'archivio culture.
- Esportazione MIDI/MPE e audio.
- SoundFont/SFZ e hosting di plugin esterni.

## Nota sulla memoria condivisa tra progetti

Idea parcheggiata: un server MCP in PHP su hosting IONOS con MySQL, esposto come connettore, per avere una memoria unica leggibile da tutti i progetti e dalle chat, anche vocali. Da verificare OAuth e disponibilità del connettore in voce.
