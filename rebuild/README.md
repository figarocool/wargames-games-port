# Wargames: Ricostruzione e Porting in C (8Byte Software)

Questa è la ricostruzione e porting nativo in linguaggio C (C11 + SDL2) del videogioco per PC *WarGames* (1983/1995/2008), originariamente programmato in FreeBASIC da **Franco Togliatto** e riscritto ed esteso da **Stefano Basile** per **8Byte Software** (2026).

Il codice C descrive ad alto livello le schermate, i comandi, le transizioni della storia, la gestione dei font TrueType e la riproduzione dei media; non contiene indirizzi fissi del programma originale né etichette opache del decompilatore. I file di reverse-engineering Ghidra sono conservati separatamente in `../re/` e non sono necessari per la compilazione del gioco.

## Compilazione e Avvio

### 1. Dipendenze
- **Debian / Ubuntu / Mint**:
  ```sh
  sudo apt install build-essential pkg-config libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev libx11-dev fonts-pc ffmpeg
  ```
- **Fedora / RHEL**:
  ```sh
  sudo dnf install gcc make SDL2-devel SDL2_ttf-devel SDL2_mixer-devel libX11-devel ffmpeg
  ```
- **Arch Linux / Manjaro**:
  ```sh
  sudo pacman -S base-devel sdl2 sdl2_ttf sdl2_mixer libx11 ffmpeg
  ```

### 2. Compilazione
Dalla cartella principale o da `rebuild/`:
```sh
make -C rebuild
```

### 3. Esecuzione
```sh
./rebuild/wargame-c
```

## Versioni e Modalità del Gioco

All'avvio, il menu permette di selezionare:
1. **ITALIANO (CLASSICO)**: Esperienza classica fedele all'originale.
2. **ITALIANO (VERSIONE ESTESA)**: Nuove funzionalità, terminali e contenuti da scoprire giocando.
3. **ENGLISH (CLASSIC)**: Classic original gameplay.
4. **ENGLISH (EXTENDED)**: Extended edition with new features to discover.

## Confronto automatico con l'originale

Con Wine e il C aperti sullo stesso display X11, `tools/replay_compare.py` manda
la stessa sequenza alle due finestre, porta in primo piano quella attiva, aspetta
i tempi indicati e salva schermate ai checkpoint richiesti. Il file JSON descrive
la sequenza:
per le azioni non sincronizzate si può aggiungere `"target": "original"` o
`"target": "candidate"`; senza `target`, l'azione raggiunge entrambi.

Per avviare tutto con un comando (Wine originale + C su due display Xvfb isolati),
usare `tools/run_comparison.py`. Avvia entrambi i programmi, attende le finestre,
esegue il replay e chiude processi e display anche se il confronto fallisce:

```sh
python3 tools/run_comparison.py evidence/carriers/sequence.json /tmp/carrier-check \
  --candidate-start carriers
```

Il launcher usa il prefix QA dedicato `~/.wine-wargame` e silenzia SDL e Wine
senza toccare il volume del desktop: instrada Wine a un sink PulseAudio temporaneo
muto e lo rimuove alla fine. `--wineprefix` sceglie un altro prefix. Per prove
audio esplicite passare `--audio`; `--candidate-start` permette di saltare al
ramo QA del C, mentre la timeline deve portare l'originale alla stessa scena.

La timeline `scenarios/first-logon-network.json` percorre carrier → primo LOGON
→ Joshua → fine della sequenza stato-rete → dialogo. Per testare il nuovo port
della `FUN_00410f00`:

```sh
python3 tools/run_comparison.py scenarios/first-logon-network.json \
  /tmp/first-logon-network --candidate-start carriers
```

Per fermare il C su un singolo fotogramma della sequenza, avviarlo con
`WARGAME_COMPARE_START=network-status` e `WARGAME_COMPARE_NETWORK_HOLD=0..7`;
Invio rilascia il fermo e riprende la timeline originale.

Per validare una pagina breve della routine contro un video Wine (senza dover
catturare i due display nello stesso millisecondo), per esempio la pagina 0:

```sh
WARGAME_COMPARE_NETWORK_HOLD=0 python3 tools/run_comparison.py \
  scenarios/network-status-page.json /tmp/network-page-0 \
  --candidate-start carriers --record-original /tmp/network-original.mkv
python3 tools/compare_recording.py /tmp/network-original.mkv /tmp/network-page-0 \
  /tmp/network-page-0-match --pattern 'c-network-page.png'
```

Per isolare il menu dei carrier avviare il C con
`WARGAME_COMPARE_START=carriers ./wargame-c`; `phone-scan` e `intro` isolano il
ramo opzionale iniziale. Per la coda FBI usare
`WARGAME_COMPARE_START=fbi-tail ./wargame-c`, oppure per il capitolo NORAD
`WARGAME_COMPARE_START=norad ./wargame-c`; `modem` avvia il prompt di connessione;
`montage` avvia il montaggio finale della simulazione, `post-hack` isola le
didascalie successive, `post-sim` riparte dalle
didascalie precedenti alla mappa, `simulation-map` entra nella sequenza della
mappa, `simulation-overview` avvia la mappa testuale prima della scelta della
fazione, `simulation-battle` avvia le bitmap della mappa, `simulation-tactical`
apre la schermata strategica dopo Wartxt02, `simulation-warning`
isola la fascia lampeggiante `Warmitxt`, `tic-tac-toe` apre il ramo del tris,
`ending` isola la sequenza finale e `final-logon`/`game-list` isolano la scelta
dei giochi dopo NORAD. Senza questa variabile, la presenza del profilo `nfsctwd`
in `../Wargames.cfg` decide l'avvio: directory dei carrier con il profilo,
sequenza introduttiva e scansione senza profilo.

```json
[
  {"capture": "prima-della-scelta"},
  {"key": "Down", "repeat": 2, "interval_ms": 250},
  {"key": "Return", "settle_ms": 500},
  {"capture": "dopo-la-scelta"},
  {"wait_ms": 1000},
  {"text": "Joshua", "char_delay_ms": 30},
  {"key": "Return", "settle_ms": 5000},
  {"capture": "saluto"}
]
```

Avvio dalla cartella `rebuild/`:

```sh
python3 tools/replay_compare.py sequenza.json /tmp/confronto-wargame
```

Per sessioni lunghe, l'originale e il C possono stare su display X11 separati:
passare `--original-display :100 --candidate-display :101` (avviando ciascun
programma sul display corrispondente). Il replay manda input XTest e caratteri
con tasti tenuti premuti brevemente, poi confronta le catture offline.

Per isolare un fotogramma tattico si può usare
`WARGAME_COMPARE_START=simulation-tactical WARGAME_COMPARE_TACTICAL_HOLD=38:2 ./wargame-c`.
Il formato è `contatore:fase` (contatori 16–250, fasi 0–4).
`WARGAME_COMPARE_TACTICAL_FAST=1` raggiunge rapidamente il checkpoint;
`WARGAME_COMPARE_RNG_SKIP=0` riparte dalla sequenza casuale iniziale senza
consumi della scansione. Queste opzioni sono ingressi QA; per misurare le durate
usare l'esecuzione normale. `tools/compare_recording.py` cerca catture identiche
all'interno di una registrazione FFV1 e ne conserva il primo fotogramma e il tempo.

Il programma cerca le finestre `Wargames` e `Wargames - C reconstruction`; si
possono specificare altri titoli con `--original-window` e `--candidate-window`.
Per ogni checkpoint salva le due PNG, una mappa rossa dei pixel diversi e
`comparison.csv`, che riporta dimensioni, pixel discordanti e differenza massima
di colore. Il replay alza il desktop Wine che contiene la finestra del gioco,
così non perde l'originale durante le sequenze lunghe. I tasti sono tenuti premuti
per 140 ms di default (regolabili con `"hold_ms"`) affinché anche il polling
DirectInput dell'originale li rilevi; il testo usa pressioni separate da 120 ms
per carattere (regolabili con `"char_hold_ms"`). `tools/compare_frames.py`
permette anche di confrontare in blocco catture già esistenti. Servono
`xdotool`, `xwd`, `xwdtopnm`, `pnmtopng` e Pillow.

L'eseguibile cerca i media originali nelle cartelle sorelle `Gfx/`, `Snd/`, `Vid/` e `Wopr/`; avviarlo da `rebuild/`. Invio invia il testo o salta il filmato/la didascalia corrente, Spazio avanza le didascalie, Esc chiude il gioco.

## Stato attuale

- Schermate SDL2 a 800×600 con bitmap e media originali.
- L'avvio ora legge il profilo `nfsctwd` da `../Wargames.cfg`: con il profilo entra direttamente nella directory carrier; senza profilo riproduce l'introduzione e la schermata di scansione iniziale. `intro` e `phone-scan` permettono di isolare i due stati.
- La schermata di logon vuota e il cursore sono stati riallineati al frame Wine. Il dialogo iniziale ora usa le righe DOS originali (senza prefisso `>`), con righe di risposta ai punti `(8,64)`, `(8,96)`, `(8,144)` e `(8,176)`; il confronto completo della conversazione, incluse le attese vocali, è ancora in corso.
- Al completamento della scansione, il C registra la data `nfsctwd` in `../Wargames.cfg`, come fa l'originale, così la scelta di intro al successivo avvio resta coerente.
- Il percorso C è stato eseguito fino al ramo iniziale della storia: prefissi, scansione, lista, banca, prenotazione Pan Am, accesso Joshua, dialogo iniziale, mappa della strategia e schermata del modem.
- La lista dei numeri è stata confrontata direttamente con Wine: coordinate, spaziatura e cursore a blocco sono allineati; la freccia `===>` lampeggia sul numero selezionato. La verifica sull'handle corretto della finestra Wine mostra che la freccia Giù sposta la selezione; il decompilato usa i codici di scansione `0x50`/`0x48`, corrispondenti a Giù/Su. La precedente indicazione P/H era errata.
- La schermata della banca è stata confrontata sotto Wine e con la routine originale: intestazione, prompt `Logon:`, attesa di 800 ms e scrittura a blocchi ogni 30 ms sono riprodotti. Dopo la scrittura, qualunque tasto riporta alla selezione; il C conserva anche i tasti premuti durante l'attesa di connessione e la scrittura iniziale.
- Il ramo Pan Am è stato seguito nell'originale sotto Wine fino al biglietto, inserendo `Paris`, `Chicago` e `John Smith`. Per le tre schermate d'inserimento e il biglietto, le catture 800×600 del C e dell'originale hanno dato zero pixel diversi; il menu è stato catturato e confrontato visivamente. Il C conserva i campi precedenti, accetta qualunque tasto per avanzare dal menu e dal biglietto e torna alla lista dopo destinazione o partenza errate. Per ottenere glifi e colore corretti usa il font `Px IBM VGA8` (`fonts-pc`) e il visual X11 TrueColor predefinito.
- Le otto catture e i dettagli del confronto Pan Am sono raccolti in [`evidence/panam/`](evidence/panam/).
- L'inserimento e la scansione sono stati confrontati direttamente sotto Wine: l'originale accetta i 24 caratteri senza Invio, mostra 0001–0003 mentre riproduce Warhello e Warsfm, poi scorre i numeri 9977–0000 per i prefissi 399, 437 e 767. Il prefisso 936 parte dopo la serie casuale di attese da 3 a 10 secondi. Il C riproduce i primi risultati, le quattro colonne, il suono Wargtr durante la ricerca, la didascalia trasparente Wartxt1 e la progressione temporizzata per 767/936. Il controllo di flusso originale chiama la selezione del numero dopo la scansione; il C passa automaticamente alla stessa schermata dopo l'ultimo tono.
- Il dialogo iniziale è stato confrontato sotto Wine: sette checkpoint stabili — logon vuoto, saluto, risposte `Hello`, `I'm fine. how are you?`, `People sometimes make mistakes`, offerta degli scacchi e prompt della fazione — sono identici pixel per pixel. Il frame transitorio `simulation-fine` differisce di 367 pixel e richiede un confronto tramite registrazione per separare il disallineamento di cattura dal timing effettivo. Evidenze e replay: [`evidence/dialogue-opening/`](evidence/dialogue-opening/). Il prompt del modem richiede `M` (confermato dalla stringa nel binario); `M` avvia il saluto successivo.
- È stato completato il blocco lasciato incompleto della mappa iniziale: quattordici righe scritte con pause da 20 ms sui caratteri non-spazio, etichette e menu a 30 ms per carattere, prompt parlati a 40 ms e attesa della fine della voce. Una risposta non vuota non intercettata dal dialogo, per esempio `Global thermonuclear war`, avvia `FINE.`/`Warfine`, la pausa da 1500 ms e poi la mappa. La scelta `2` è accettata immediatamente, senza Invio. La fazione lampeggia tre volte prima della richiesta dei bersagli. Le città errate e duplicate vengono cancellate senza i precedenti messaggi aggiunti. Sette checkpoint 800×600 sono identici pixel per pixel a Wine: [`evidence/simulation-overview/`](evidence/simulation-overview/). Il replay del C con ASan/UBSan ha verificato anche il rifiuto di `1` e il passaggio dai due bersagli alla battaglia e alla schermata tattica. Questo confronto riguarda le schermate stabili; il sincronismo completo di animazione e audio resta da misurare.
- Dopo la simulazione il C riproduce il montaggio originale `Warswoff` → `Warswof2` → … → `Warswof7` con le attese 500/500/100/100/100/100/500 ms; l'avvio di `Warswoff` e `Warswof2` è collocato sui rispettivi cambi immagine, come nel binario. Dopo il montaggio si passa alle didascalie `Wartxt03`–`Wartxt5` e quindi al modem.
- La battaglia mostra ora tutte le sette pagine originali, incluso `Wararro1`, e i cartelli `Wartxt01/02` con la palette opaca nativa. Il lampeggio di `Warmitxt` alterna la bitmap al rettangolo nero originale; `Warmiswn` resta in loop durante le sei ripetizioni. Le attese del timer intero aspettano il successivo secondo, anziché durare sempre 1000 ms. Dieci immagini complete compaiono identiche sia nella registrazione Wine sia in quella del C: [`evidence/simulation-battle/`](evidence/simulation-battle/). Il confronto dell'intera animazione e dell'audio resta da completare.
- Dopo `Wartxt02`, mappa tattica e tabelle compaiono immediatamente: entrambe le mappe contengono quattordici righe, ma solo quella iniziale viene digitata. La griglia tattica conserva anche le cancellazioni e le sovrascritture delle celle. Il Mersenne Twister viene inizializzato con seed zero dal runtime; coordinate e attese usano `floor`, come confermato dal disassemblato e dalla sequenza osservata. Tredici checkpoint completi, inclusi gli archi ai contatori 16/20/25/30/35 e i cartelli ai contatori 36/38/40, sono identici pixel per pixel a Wine. Nell'esecuzione normale i loro tempi relativi coincidono entro un fotogramma a 30 fps, dopo l'allineamento dell'ingresso tattico: [`evidence/simulation-tactical/`](evidence/simulation-tactical/). `S` porta al montaggio, e il ciclo termina anche al contatore 250; il sincronismo dell'audio e il resto della partita richiedono ancora verifica.
- Il tris accetta la sequenza originale `5`, `3`, `8`, `6`, `1`; a ogni turno traccia l’X e la risposta automatica O nelle caselle codificate dal binario, con pause da 750 ms e i campioni `Warxttt`/`Warottt`. Le coordinate dei tratti X e i centri/raggi O derivano dalle formule in `FUN_0040b590`; il replay arriva a `STALEMATE.` e `WANT TO PLAY AGAIN?`. `1`/`One` prosegue, `2`/`Two` ripete il prompt. Una cattura del candidato è in [`evidence/tic-tac-toe/`](evidence/tic-tac-toe/); circonferenza/rasterizzazione, coordinate dei testi e sincronismo audio richiedono ancora confronto diretto con Wine.
- Il messaggio finale conserva ora le tre righe indicate dai separatori `^` della stringa originale (`A STRANGE GAME.` / `THE ONLY WINNING MOVE IS` / `NOT TO PLAY.`); `WARGAME_COMPARE_START=ending` permette di isolare la schermata per ulteriori confronti.
- Il montaggio `Warswoff`–`Warswof7` è stato eseguito sotto Xvfb. È stato eliminato il prompt generico che si sovrapponeva alle bitmap; una cattura di `Warswof2` dopo `S` coincide esattamente con l'asset a dimensione nativa. Il controllo ASan/UBSan ha percorso il ciclo tattico fino al checkpoint 250 e il passaggio al montaggio, senza errori rilevati.
- Il prompt di logon ora gestisce `Help`, `Games`, `List Games` e `Franco` oltre a `Joshua`; le risposte C sono state eseguite e catturate in [`evidence/logon-commands/`](evidence/logon-commands/). I messaggi derivano dalle stringhe e dai rami di `FUN_00403ce0`, ma non sono ancora confrontati pixel per pixel con Wine.
- Dopo l’ultima sequenza NORAD, il flusso torna al prompt `LOGON:` di `FUN_0040a8c0`; `List Games` apre il catalogo dopo il suono e la pausa da 500 ms, mentre `Tic Tac Toe` può essere inserito direttamente. Il catalogo riproduce le 15 voci effettivamente stampate dal binario (senza elencare Tic Tac Toe) e corregge `AIR-TO-GROUND ACTIONS`. Il percorso C `LOGON → List Games → Tic Tac Toe` è stato eseguito sotto Xvfb; schermate e tempi di questo ramo attendono ancora il confronto diretto con Wine.
- Il primo cartello `Wartxt03` dopo il montaggio post-simulazione ora è stato confrontato contro una registrazione Wine lossless: il frame originale 128 e la cattura C sono identici pixel per pixel. Le altre didascalie successive e l'ingresso al modem richiedono ancora confronto temporale: [`evidence/dialogue-opening/post-hack-recording/`](evidence/dialogue-opening/post-hack-recording/).
- La coda di tre didascalie FBI attende Invio, come l'originale. Sei checkpoint, inclusi due attese di sette secondi e il passaggio alla prima didascalia NORAD, sono identici pixel per pixel; catture e report sono in [`evidence/fbi-tail/`](evidence/fbi-tail/).
- Le clip video, le voci MP3 e gli effetti PCM sono collegati, ma non è stata verificata l'intera riproduzione né il sincronismo con l'originale.

## Struttura del codice

- `src/main.c` contiene gli stati nominati della storia, il trattamento delle risposte, le schermate e le sequenze di didascalie e filmati. Ogni funzione ha un commento che ne spiega lo scopo.
- `src/media.c` contiene caricamento e riproduzione degli effetti e delle voci, oltre al collegamento FFmpeg/SDL per video e audio.
- `src/media.h` dichiara le interfacce e le risorse condivise dai moduli.
- `Makefile` compila i due moduli con avvisi del compilatore attivi.

## Verifica e limiti

Il decompilato Ghidra e l'eseguibile originale sotto Wine sono usati come riferimenti. Pan Am, la coda FBI e i checkpoint documentati di mappa iniziale, battaglia e mappa tattica sono validati pixel per pixel. Non è stata completata una partita comparativa né verificata ogni diramazione, immagine, voce, effetto, fotogramma e temporizzazione. La coda casuale della scansione non è confrontata su una durata completa e i tempi della banca non sono confrontati fotogramma per fotogramma. Il tris richiede ancora il confronto con Wine di rasterizzazione, testi e audio. Perciò questo programma **non è ancora un clone 1:1**.

Il codice è stato ricompilato con `make` senza avvisi. Tutti i sette filmati hanno video 800×592 a 25 fps e audio stereo MP2 a 48 kHz, coerenti con il decoder SDL/FFmpeg. L'originale e la ricostruzione C sono stati avviati; il ramo Pan Am è stato percorso su entrambi e le schermate del C sono state confrontate direttamente con le catture Wine. Il ramo mappa dispone ora di un ingresso QA dedicato per il confronto diretto di fotogrammi e audio.
