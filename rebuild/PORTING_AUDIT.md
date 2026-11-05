# Audit del porting Wargames C

> Generato da `tools/audit_port.py` il 2026-10-31 usando l’export fresco Ghidra 12.2. 
> Non è un decompilatore: la traduzione delle istruzioni è stata fatta da Ghidra; 
> ruoli e stato del port sono revisione manuale e vanno verificati a runtime.

## Metodo e completezza dell’export

- Funzioni esportate: **590**; righe export: **25874**.
- Un export preesistente di Ghidra 12.1.4 riportava 596 funzioni; la nuova analisi 12.2 ne identifica 590. Le sei funzioni di differenza dipendono dall’analisi/versione e gli export precedenti sono stati conservati.
- Funzioni raggiungibili dal main Ghidra `FUN_004122f0` tramite chiamate dirette nell’export: **217**.
- Funzioni con stringhe in `string_xrefs.csv`: **36**; record CSV: **768**.
- Funzioni game-specific selezionate per revisione manuale: **18**.
- L’export Ghidra è best-effort, non sorgente originale e non è compilabile come porting. La raggiungibilità è approssimata: puntatori a funzione e chiamate indirette non sono risolti da questo script.
- RetDec è un’alternativa PE/x86 open source, ma il progetto dichiara manutenzione limitata e richiede un’installazione di diversi GB. Qui non aggiunge valore rispetto all’export Ghidra esistente, già completo a livello di funzione.
- Rigenerazione dal repository con gli export locali presenti: `python3 rebuild/tools/audit_port.py`. I file `re/` sono esclusi da Git, quindi servono il progetto Ghidra e gli export nella macchina di analisi.

## Flusso originale: funzioni di gioco

| Indirizzo Ghidra | Routine originale | Stato port | Evidenza / gap | Mapping semantico nel C |
|---|---|---|---|---|
| `0x004012d0` `FUN_004012d0` | Scansione carrier e intro telefonica | **Parziale** | Schermata e scan implementati; coda casuale completa e timing non confrontati. | `start_intro`; stage `PHONE`/`SCANNING`; input in `main`; scan in `render` e loop principale. |
| `0x00402490` `FUN_00402490` | Lista carrier e chiamata | **Parziale** | Navigazione/rami presenti; timing della banca e tutti i numeri non verificati. | Stage `DIAL_NUMBERS`, eventi Su/Giù/Invio in `main`, transizioni verso banca e carrier. |
| `0x00403230` `FUN_00403230` | Prenotazione Pan Am | **Verificato in parte** | Quattro schermate statiche identiche per il percorso Paris/Chicago/nome provato. | Stage `DIAL_MENU`, `DIAL_DESTINATION`, `DIAL_DEPARTURE`, `DIAL_PASSENGER`, `DIAL_TICKET`; `submit` e `render`. |
| `0x00403b20` `FUN_00403b20` | Schermata banca | **Parziale** | Rendering/typewriter presenti; confronto temporale completo non documentato. | Stage `BANK_PROMPT`; `update_bank_typewriter_audio`; ramo banca nel loop principale. |
| `0x00403ce0` `FUN_00403ce0` | Prompt LOGON iniziale e comandi | **Parziale** | Joshua/Help/Games/List Games/Franco implementati; layout dei comandi non confrontato con Wine. | Stage `LOGON`/`LOGON_REJECTED`; `stage_prompt`, `submit`, `render`. |
| `0x004047f0` `FUN_004047f0` | Dialogo iniziale con Joshua | **Parziale** | Sette checkpoint stabili identici; alternative e sincronismo audio incompleti. | Stage `GREETING`, `HOW_ARE_YOU`, `MISTAKES`, `PLAY`, `CHESS_REFUSAL`, `LATER`; `submit`. |
| `0x00405190` `FUN_00405190` | Mappa iniziale e sequenza di simulazione | **Avanzato, non completo** | Checkpoint di mappa, battaglia e tattica confrontati; intero audio/percorso non validato. | Stage `SIMULATION_OVERVIEW`, `SIDE`, `CITIES`, `SIMULATION_BATTLE`, `SIMULATION_TACTICAL`, `SIMULATION_MONTAGE`. |
| `0x00407e60` `FUN_00407e60` | Post-simulazione, cartelli e modem | **Parziale** | Cartelli e ingresso modem presenti; confronto integrale di sequenze e audio assente. | `CaptionProgram` post-simulazione/post-hack; `STORY_CAPTION`, `MODEM_CONNECT`, `GREETING3`. |
| `0x004087b0` `FUN_004087b0` | Capitolo Falken/rapporti NORAD | **Incompleto** | Il port salta rapporti di distruzione, domanda sulla differenza, indirizzo di Falken e le relative voci. | Solo `IDENTIFICATION`, `PRIMARY_GOAL`, `WIN_STATEMENT`, `LOGON_AGAIN`, `STILL_PLAYING`, `REAL_OR_GAME`; mancano scene e voci intermedie. |
| `0x004098f0` `FUN_004098f0` | Sequenza cartelli NORAD 12–32 | **Avanzato, non completo** | Asset/sequenza presenti; non risultano verificati tutti i tempi, input e audio. | `CaptionProgram` `CAPTIONS_NORAD_*`; `start_caption_sequence` e `advance_story_caption`. |
| `0x0040a8c0` `FUN_0040a8c0` | Secondo LOGON e catalogo giochi | **Parziale** | Catalogo e selezioni principali presenti; ogni risposta/ramo e gli effetti non sono confrontati. | Stage `LOGON_FINAL`, `LIST_GAMES`, `GAME_SHELL`; selezione delle clip in `submit`. |
| `0x0040b590` `FUN_0040b590` | Tic Tac Toe | **Incompleto** | È riprodotta la partita prescritta; non è dimostrata la logica generale a uno/due giocatori. | Stage `TIC_TAC_TOE`, `TIC_MARK`, `TIC_MOVE`, `TIC_ANIMATION`, `TIC_AGAIN`; `advance_tic_animation`. |
| `0x0040dd10` `FUN_0040dd10` | Saluto e finale | **Parziale** | Testi/tempi implementati; confronto pixel e temporale con l'originale non documentato. | Stage `FINAL_GREETING`, `ENDING_PRELUDE`, `ENDING`, `ENDING_CHESS`, `ENDING_MUSIC`. |
| `0x0040e170` `FUN_0040e170` | Intro | **Parziale** | Intro implementata; sequenza completa e riproduzione media non confrontate. | Stage `INTRO`; `start_intro`, `render`, timer nel loop principale. |
| `0x0040e470` `FUN_0040e470` | Helper originale per cartelli/animazioni | **Copertura parziale** | Meccanismi ricreati tramite le sequenze di caption C; equivalenza dei vari mode non provata. | `load_story_caption`, `start_caption_sequence`, `advance_story_caption`, `render`. |
| `0x0040ee60` `FUN_0040ee60` | Risposte di accesso negato / gioco bloccato | **Incompleto** | Il binario ha schermate e rami distinti; il port non ne dimostra la parità, soprattutto dal LOGON finale. | Nessun equivalente dedicato; alcuni ingressi LOGON sono simulati da clip e transizioni. |
| `0x0040fb30` `FUN_0040fb30` | Pannello GAME TIME / ESTIMATED TIME | **Mancante** | Il pannello con i tempi della guerra, chiamato dal capitolo Falken, non è implementato come schermata dedicata. | Nessun stage o helper C corrispondente. |
| `0x00410f00` `FUN_00410f00` | Montaggio di schermate stato-rete dopo il primo LOGON | **Implementato; confronto puntuale da completare** | Le otto fasi e gli intervalli sono ricostruiti; i fotogrammi brevi richiedono confronto via registrazione Wine. | Stage `NETWORK_STATUS`; `start_network_status`, `start_network_page`, `advance_network_status`, `render_network_status`; `WARGAME_COMPARE_NETWORK_HOLD` isola ogni pagina. |

## Gap confermati dal confronto statico

Il mapping sopra è intenzionalmente molti-a-molti: i confini delle funzioni FreeBASIC originali non coincidono con gli stage/helper della riscrittura SDL. Per questo la verifica automatica misura presenza, raggiungibilità e stringhe, mentre l’equivalenza si valuta con gli scenari comportamentali.

1. **Schermate stato-rete (`FUN_00410f00`).** La sequenza è ora implementata dopo il primo `Joshua`: otto pagine temporizzate e voce typewriter. Restano da confrontare ogni pagina e i relativi intervalli con una registrazione Wine; la timeline `scenarios/first-logon-network.json` verifica il percorso stabile fino al dialogo.
2. **Capitolo Falken/NORAD (`FUN_004087b0`).** Il binario include la proiezione dei kill ratio, `WHAT'S THE DIFFERENCE?`, l’indirizzo di Falken e voci `Warsin`, `Warwtd`, `Warcladd`, `War5tcr`. Il ramo C `REAL_OR_GAME` salta direttamente alla coda di cartelli FBI (`main.c`, circa righe 1453–1458).
3. **Pannello tempi guerra (`FUN_0040fb30`).** Il binario visualizza `GAME TIME ELAPSED / ESTIMATED TIME REMAINING`; il port non ha una schermata equivalente nel flusso sopra menzionato.
4. **Risposte bloccate (`FUN_0040ee60`).** L’originale ha varianti con `CHANGES LOCKED OUT`, `ACCESS DENIED`, `IDENTIFICATION NOT RECOGNIZED`, `GAME ROUTINE RUNNING` e attese/audio dedicati. La semplice riproduzione di clip nel LOGON finale non prova equivalenza di questi rami.
5. **Tic Tac Toe.** Il C riproduce la sequenza dimostrativa `5,3,8,6,1`; nel sorgente lo stato è sostanzialmente scriptato e non è dimostrata la partita generale a uno/due giocatori, con input alternativi e relativi esiti.
6. **Media e copertura temporale.** Mancano il confronto di una partita completa, tutte le diramazioni, tutte le animazioni e il sincronismo audio/video. Il player C scandisce i frame a 25 fps e accoda l’audio in modo indipendente; la parità globale non è dimostrata.

## Evidenze già disponibili

- Pan Am: quattro frame statici identici pixel per pixel per la sequenza provata.
- Dialogo iniziale: sette checkpoint stabili identici; il checkpoint in animazione resta da riallineare.
- Mappa iniziale: sette checkpoint identici, inclusi input errati e duplicati.
- Battaglia: dieci immagini identiche; tattica: tredici checkpoint identici e scarto temporale massimo salvato di un frame a 30 fps.
- Coda FBI: sei checkpoint identici.
- Queste prove sono report e catture preesistenti. Questo ambiente non ha `make`, compilatore C, Wine o Ghidra eseguibile, quindi non sono state ricompilate/rieseguite qui.

## Inventario asset non referenziati dal C (8)

La verifica è per nome base nel sorgente e può sovrastimare i mancanti quando l’originale costruisce nomi dinamicamente. I file audio con riferimenti certi nel decompilato sono evidenziati.

- `Gfx/War10sim.dat`
- `Gfx/Warquit.dat`
- `Vid/War1.dat`
- `Wopr/War5tcr.dat` — voce referenziata dal decompilato
- `Wopr/Warcladd.dat` — voce referenziata dal decompilato
- `Wopr/Warsidnpc.dat`
- `Wopr/Warsin.dat` — voce referenziata dal decompilato
- `Wopr/Warwtd.dat` — voce referenziata dal decompilato

## Inventario di tutte le funzioni esportate

Il prospetto seguente copre ogni header Ghidra. `Reachable` usa il grafo delle chiamate dirette ricavato dall’output C; `xref` è il numero di riferimenti stringa noti; `dimensione` è il numero di righe nel blocco export.

| Indirizzo | Nome export | Reachable | Righe | Xref | Stringhe note | Chiamate interne (prime 8) |
|---|---|---:|---:|---:|---|---|
| `0x00401000` | `FUN_00401000` | no | 14 | 0 | — | FUN_00401000 |
| `0x00401040` | `FUN_00401040` | no | 29 | 0 | — | FUN_00401040 |
| `0x004011e0` | `FUN_004011e0` | no | 20 | 0 | — | FUN_004011e0, FUN_0042f670, FUN_00401000, FUN_00401040, FUN_0042f640, FUN_004122f0 |
| `0x00401260` | `entry` | no | 11 | 0 | — | FUN_004011e0 |
| `0x00401280` | `FUN_00401280` | sì | 10 | 0 | — | FUN_00401280 |
| `0x004012c0` | `FUN_004012c0` | no | 143 | 1 | _LIBGCCW32_EH_3_SJLJ_GTHR_MINGW3 | FUN_004012c0, FUN_0042ef10 |
| `0x004012d0` | `FUN_004012d0` | sì | 535 | 16 | nfsctwd; cnrbs-ct; TO SCAN FOR CARRIER TONES, PLEASE LIST; DESIRED AREA CODES AND PREFIXES; AREA                       AREA                       AREA                       AREA | FUN_004012d0, FUN_00423de0, FUN_00424010, FUN_004240d0, FUN_004242b0, FUN_00424300, FUN_00424390, FUN_00424660 |
| `0x00402490` | `FUN_00402490` | sì | 370 | 11 | cnrbs-ct; (311) 399 2364; (311) 399 3582; (311) 437 8739; (311) 437 1883 | FUN_00402490, FUN_00424010, FUN_00423de0, FUN_004240d0, FUN_004242b0, FUN_00424300, FUN_00425350, FUN_00424390 |
| `0x00403230` | `FUN_00403230` | sì | 273 | 26 | Pan Am Reservation System; DTRA:2342:435:936; Technical Status:  Active; 1. Flight Schedule; 2. Credit Check | FUN_00403230, FUN_00425350, FUN_00424390, FUN_00424660, FUN_00410a10, FUN_004246e0, FUN_00424780, FUN_004247d0 |
| `0x00403b20` | `FUN_00403b20` | sì | 70 | 4 | Y- 1293. 323; Union Pacific Bank - SouthWest Regional; Data Control; Logon: | FUN_00403b20, FUN_00425350, FUN_00424390, FUN_00424660, FUN_00410a10, FUN_004246e0, FUN_004247d0, FUN_00424920 |
| `0x00403ce0` | `FUN_00403ce0` | sì | 363 | 32 | cnrbs-ct; LOGON:; Franco; **FRANCO TOGLIATTO**; SYSTEM INDICATES USER (AKA FALCON) ORIGINATOR OF THE PROGRAM | FUN_00403ce0, FUN_00424390, FUN_004247d0, FUN_00424920, FUN_00424660, FUN_00410a10, FUN_004246e0, FUN_0040fd20 |
| `0x004047f0` | `FUN_004047f0` | sì | 365 | 15 | GREETINGS PROFESSOR FALKEN.; HOW ARE YOU TODAY?; EXCELLENT,  IT'S BEEN A LONG TIME.^CAN YOU EXPLAIN THE REMOVAL OF YOUR USER ACCOUNT ON 6/23/73?; Waribalt; YES THEY DO. | FUN_004047f0, FUN_00424660, FUN_00410a10, FUN_004246e0, FUN_0040fd20, FUN_004247d0, FUN_00424920, FUN_00424010 |
| `0x00405190` | `FUN_00405190` | sì | 1392 | 61 | _; __________________                                           __/ \; /                  \___        _                           __/     /  /\   _; {                       \_   __/ \|                       __/        `-'  \_/ \; /                         \_/   /                _    __/                      `~-~~._ | FUN_00405190, FUN_00424390, FUN_00424660, FUN_00410bf0, FUN_004246e0, FUN_00424780, FUN_00410a10, FUN_00410730 |
| `0x00407e60` | `FUN_00407e60` | sì | 369 | 14 | GREETINGS PROFESSOR FALKEN.; Wartxt03; Wartxt4; Wartxt04; Wartxt5 | FUN_00407e60, FUN_00424660, FUN_00412220, FUN_0040e470, FUN_004246e0, FUN_004251d0, FUN_0040fc90, FUN_00424cc0 |
| `0x004087b0` | `FUN_004087b0` | sì | 622 | 39 | Joshua; GREETINGS PROFESSOR FALKEN.; Wartxt7; Wartxt8; D:.  34::36:73        5:5  PROC  2435.45.8456      ACC+P    STATUS: PC-455 | FUN_004087b0, FUN_00424660, FUN_0040e470, FUN_004246e0, FUN_00425a20, FUN_0040fc90, FUN_00425640, FUN_00424390 |
| `0x004098f0` | `FUN_004098f0` | sì | 691 | 31 | LOGON:; Wartxt12; Wargisld; Wartxt13; Warpswde | FUN_004098f0, FUN_00425a20, FUN_00412220, FUN_00424660, FUN_0040e470, FUN_004246e0, FUN_00411800, FUN_00424390 |
| `0x0040a8c0` | `FUN_0040a8c0` | sì | 348 | 35 | LOGON:; List Games; FALKEN'S MAZE; BLACKJACK; GIN RUMMY | FUN_0040a8c0, FUN_00424390, FUN_004247d0, FUN_00424920, FUN_00424780, FUN_00425ac0, FUN_0040fd20, FUN_00425350 |
| `0x0040b590` | `FUN_0040b590` | sì | 560 | 10 | Snd/Wartttsc.dat; Snd/Warxttt.dat; Snd/Warottt.dat; Snd/Warxor0.dat; ONE OR TWO PLAYERS? | FUN_0040b590, FUN_00425350, FUN_004251d0, FUN_00413750, FUN_00424390, FUN_004247d0, FUN_00424920, FUN_00424780 |
| `0x0040dd10` | `FUN_0040dd10` | sì | 171 | 10 | LOGON:; Tic Tac Toe; Tic-tac-toe; GREETINGS PROFESSOR FALKEN; Wargpf3 | FUN_0040dd10, FUN_00423de0, FUN_0040ee60, FUN_00424390, FUN_004247d0, FUN_00424920, FUN_0040fd20, FUN_0040b590 |
| `0x0040e170` | `FUN_0040e170` | sì | 137 | 3 | Gfx/Warints2.dat; Warintro; Warintsc | FUN_0040e170, FUN_00423de0, FUN_00412800, FUN_004247d0, FUN_004163b0, FUN_00424660, FUN_00410730, FUN_004246e0 |
| `0x0040e470` | `FUN_0040e470` | sì | 221 | 2 | Gfx/Warmitxt.dat; Gfx/Wartxt03.dat | FUN_0040e470, FUN_00424b20, FUN_00424660, FUN_00412800, FUN_004247d0, FUN_004163b0, FUN_00412a30, FUN_004251d0 |
| `0x0040ee60` | `FUN_0040ee60` | sì | 267 | 8 | Snd/Waracdnd.dat; CHANGES LOCKED OUT; ** IMPROPER REQUEST **; ** ACCESS DENIED **; >>> CHANGES LOCKED OUT <<< | FUN_0040ee60, FUN_00425640, FUN_00424cc0, FUN_00412020, FUN_00424390, FUN_004247d0, FUN_00424920, FUN_00424010 |
| `0x0040fb30` | `FUN_0040fb30` | sì | 40 | 3 | GAME TIME ELAPSED                                   ESTIMATED TIME REMAINING; 55 HRS  30 MIN  42 SEC                              27 HRS  59 MIN  18 SEC; 31 HRS  12 MIN  17 SEC                              52 HRS  17 MIN  43 SEC | FUN_0040fb30, FUN_00424660, FUN_00425640, FUN_00424390, FUN_00424920, FUN_004247d0 |
| `0x0040fc90` | `FUN_0040fc90` | sì | 22 | 0 | — | FUN_0040fc90, FUN_00425230, FUN_00423de0 |
| `0x0040fcb0` | `FUN_0040fcb0` | sì | 26 | 0 | — | FUN_0040fcb0, FUN_00425230, FUN_00424660, FUN_00423de0, FUN_00412020, FUN_004242b0 |
| `0x0040fd20` | `FUN_0040fd20` | sì | 248 | 3 | .327846317871; .823975485775; .476138914618 | FUN_0040fd20, FUN_0040fc90, FUN_00424660, FUN_004247d0, FUN_00424920, FUN_00425ac0, FUN_00424390, FUN_00424300 |
| `0x004105e0` | `FUN_004105e0` | sì | 28 | 0 | — | FUN_004105e0, FUN_00424390, FUN_004247d0, FUN_00424920, FUN_00425ac0 |
| `0x00410690` | `FUN_00410690` | sì | 27 | 1 | Wargames.cfg | FUN_00410690, FUN_004247d0, FUN_00425f80, FUN_00426000, FUN_004260f0, FUN_00426210 |
| `0x004106e0` | `FUN_004106e0` | sì | 29 | 1 | Wargames.cfg | FUN_004106e0, FUN_004247d0, FUN_00425f80, FUN_00426330, FUN_004264d0, FUN_00426210 |
| `0x00410730` | `FUN_00410730` | sì | 91 | 1 | Wopr/ | FUN_00410730, FUN_00424b20, FUN_00424660, FUN_00425d10, FUN_004240d0, FUN_00423de0, FUN_004247d0, FUN_00424920 |
| `0x004109b0` | `FUN_004109b0` | sì | 13 | 4 | Snd/Warnocmp.dat; Snd/Warcords.dat; Snd/Wartxtsd.dat; Snd/Warhtent.dat | FUN_004109b0 |
| `0x00410a10` | `FUN_00410a10` | sì | 72 | 0 | — | FUN_00410a10, FUN_00425d10, FUN_004247d0, FUN_00424920, FUN_00424390, FUN_004240d0, FUN_00425ac0, FUN_004251d0 |
| `0x00410bf0` | `FUN_00410bf0` | sì | 38 | 0 | — | FUN_00410bf0, FUN_00425d10, FUN_00424cc0, FUN_00412020, FUN_004240d0, FUN_00424920, FUN_00423de0, FUN_004251d0 |
| `0x00410c90` | `FUN_00410c90` | sì | 67 | 1 | Wopr/ | FUN_00410c90, FUN_00424b20, FUN_00424660, FUN_00424cc0, FUN_00412020, FUN_00424390, FUN_00424920, FUN_004247d0 |
| `0x00410e70` | `FUN_00410e70` | sì | 21 | 0 | — | FUN_00410e70, FUN_00417580, FUN_00417880, FUN_00426580, FUN_00425a20, FUN_004266f0 |
| `0x00410f00` | `FUN_00410f00` | sì | 237 | 32 | 145      11455         11283       11333       #-11972       11315; PRT CON. 3.4.5.  SECTRAN 9.4.3.                PORT  STAT:  58-345; (311):835-7385; (311)935-3532; (311) 767-8735 | FUN_00410f00, FUN_004253c0, FUN_00425640, FUN_00424390, FUN_004247d0, FUN_00424920, FUN_004251d0, FUN_00425350 |
| `0x00411730` | `FUN_00411730` | sì | 21 | 0 | — | FUN_00411730, FUN_00412280, FUN_004251d0 |
| `0x00411800` | `FUN_00411800` | sì | 91 | 0 | — | FUN_00411800, FUN_00424b20, FUN_00424660, FUN_004267a0, FUN_00426820, FUN_00425350, FUN_00425530, FUN_004251d0 |
| `0x00412020` | `FUN_00412020` | sì | 50 | 0 | — | FUN_00412020, FUN_00412800, FUN_00418070, FUN_00412a30, FUN_00424cc0, FUN_0040fc90, FUN_00425290, FUN_00425cd0 |
| `0x004121d0` | `FUN_004121d0` | sì | 18 | 0 | — | FUN_004121d0, FUN_00425290, FUN_00412020 |
| `0x00412220` | `FUN_00412220` | sì | 27 | 0 | — | FUN_00412220, FUN_00424300, FUN_004251d0, FUN_00424cc0, FUN_00412020 |
| `0x00412280` | `FUN_00412280` | sì | 15 | 0 | — | FUN_00412280, FUN_00412a30, FUN_00412e50 |
| `0x004122f0` | `FUN_004122f0` | sì | 75 | 2 | Gfx/Warquit.dat; Wargames.cfg | FUN_004122f0, FUN_0042f710, FUN_00426930, FUN_00410e70, FUN_00412800, FUN_004247d0, FUN_004163b0, FUN_00410690 |
| `0x004124c0` | `_FSOUND_PlaySound@8` | no | 12 | 0 | — | — |
| `0x004124d0` | `_FSOUND_Sample_Free@4` | no | 12 | 0 | — | — |
| `0x004124e0` | `_FSOUND_StopSound@4` | no | 12 | 0 | — | — |
| `0x004124f0` | `_FSOUND_Close@0` | no | 12 | 0 | — | — |
| `0x00412500` | `_FSOUND_Stream_Close@4` | no | 12 | 0 | — | — |
| `0x00412510` | `_FSOUND_Sample_Load@20` | no | 12 | 0 | — | — |
| `0x00412520` | `_FSOUND_Stream_Open@16` | no | 12 | 0 | — | — |
| `0x00412530` | `_FSOUND_Stream_Play@8` | no | 12 | 0 | — | — |
| `0x00412540` | `_FSOUND_Stream_GetPosition@4` | no | 12 | 0 | — | — |
| `0x00412550` | `_FSOUND_Stream_GetLength@4` | no | 12 | 0 | — | — |
| `0x00412560` | `_FSOUND_SetPaused@8` | no | 12 | 0 | — | — |
| `0x00412570` | `_FSOUND_Init@12` | no | 12 | 0 | — | — |
| `0x00412580` | `_FSOUND_Stream_SetBufferSize@4` | no | 12 | 0 | — | — |
| `0x00412590` | `SetUnhandledExceptionFilter` | no | 15 | 0 | — | — |
| `0x004125a0` | `ExitProcess` | no | 13 | 0 | — | — |
| `0x004125b0` | `Sleep` | no | 12 | 0 | — | — |
| `0x004125c0` | `CoInitialize` | no | 14 | 0 | — | — |
| `0x004125d0` | `CoCreateInstance` | no | 14 | 0 | — | — |
| `0x004125e0` | `CoUninitialize` | no | 12 | 0 | — | — |
| `0x004125f0` | `FUN_004125f0` | sì | 78 | 0 | — | FUN_004125f0, FUN_00418c80, FUN_00418aa0, FUN_004183b0 |
| `0x00412800` | `FUN_00412800` | sì | 10 | 0 | — | FUN_00412800, FUN_004125f0 |
| `0x00412870` | `FUN_00412870` | sì | 10 | 0 | — | FUN_00412870 |
| `0x00412890` | `FUN_00412890` | no | 18 | 0 | — | FUN_00412890 |
| `0x00412990` | `FUN_00412990` | no | 30 | 0 | — | FUN_00412990, FUN_004191d8 |
| `0x00412a30` | `FUN_00412a30` | sì | 149 | 0 | — | FUN_00412a30, FUN_00418c80, FUN_00418aa0, FUN_004183b0, FUN_00418510, FUN_00418410, FUN_004185c0, FUN_00418e00 |
| `0x00412e50` | `FUN_00412e50` | sì | 239 | 0 | — | FUN_00412e50, FUN_00418c80, FUN_00426a00, FUN_00418aa0, FUN_00419370, FUN_004183b0, FUN_00418510, FUN_00418410 |
| `0x00413680` | `FUN_00413680` | sì | 32 | 0 | — | FUN_00413680 |
| `0x004136f0` | `FUN_004136f0` | sì | 15 | 0 | — | FUN_004136f0 |
| `0x00413750` | `FUN_00413750` | sì | 263 | 0 | — | FUN_00413750, FUN_00418c80, FUN_00418aa0, FUN_00419370, FUN_004183b0, FUN_00418510, FUN_00418410, FUN_004185c0 |
| `0x00413e20` | `FUN_00413e20` | sì | 39 | 0 | — | FUN_00413e20 |
| `0x00413f60` | `FUN_00413f60` | sì | 33 | 0 | — | FUN_00413f60 |
| `0x00414010` | `FUN_00414010` | sì | 229 | 0 | — | FUN_00414010, FUN_00418c80, FUN_00418aa0, FUN_00419370, FUN_004183b0, FUN_00418510, FUN_00418410, FUN_00413f60 |
| `0x00414820` | `FUN_00414820` | no | 10 | 0 | — | FUN_00414820, FUN_00419c10 |
| `0x00415190` | `FUN_00415190` | sì | 543 | 0 | — | FUN_00415190, FUN_00418c80, FUN_00426a00, FUN_00426fc0, FUN_00426e70, FUN_00418aa0, FUN_004183b0, FUN_00418e00 |
| `0x004163b0` | `FUN_004163b0` | sì | 10 | 0 | — | FUN_004163b0, FUN_00415190 |
| `0x00416410` | `FUN_00416410` | no | 44 | 0 | — | FUN_00416410, FUN_00418c80 |
| `0x00416550` | `FUN_00416550` | no | 56 | 0 | — | FUN_00416550 |
| `0x00416880` | `FUN_00416880` | no | 9 | 0 | — | FUN_00416880 |
| `0x00416890` | `FUN_00416890` | sì | 31 | 0 | — | FUN_00416890 |
| `0x00416930` | `FUN_00416930` | sì | 51 | 0 | — | FUN_00416930, FUN_00416890 |
| `0x00416a60` | `FUN_00416a60` | sì | 343 | 1 | FBGFX | FUN_00416a60, FUN_00416930, FUN_00427050, FUN_00426580, FUN_00425350, FUN_00427240, FUN_00427000, FUN_00418880 |
| `0x00417430` | `FUN_00417430` | sì | 15 | 0 | — | FUN_00417430, FUN_00416a60 |
| `0x00417580` | `FUN_00417580` | sì | 24 | 0 | — | FUN_00417580, FUN_00426a00, FUN_00416a60 |
| `0x00417650` | `FUN_00417650` | no | 24 | 0 | — | FUN_00417650, FUN_00426e70 |
| `0x004176e0` | `FUN_004176e0` | no | 83 | 0 | — | FUN_004176e0, FUN_00417650, FUN_00427280, FUN_00424660 |
| `0x00417880` | `FUN_00417880` | sì | 264 | 0 | — | FUN_00417880, FUN_00418c80, FUN_0041b3c0, FUN_0041b320 |
| `0x00417e40` | `FUN_00417e40` | sì | 75 | 0 | — | FUN_00417e40, FUN_00418c80, FUN_00418aa0, FUN_004183b0, FUN_00418510, FUN_00418410, FUN_004185c0, FUN_00418e00 |
| `0x00418070` | `FUN_00418070` | sì | 11 | 0 | — | FUN_00418070, FUN_00417e40 |
| `0x00418110` | `FUN_00418110` | no | 24 | 0 | — | FUN_00418110 |
| `0x00418160` | `FUN_00418160` | no | 28 | 0 | — | FUN_00418160 |
| `0x004182f0` | `FUN_004182f0` | no | 9 | 0 | — | FUN_004182f0 |
| `0x00418300` | `FUN_00418300` | no | 25 | 0 | — | FUN_00418300, FUN_00427080, FUN_004270b0 |
| `0x004183a0` | `FUN_004183a0` | no | 9 | 0 | — | FUN_004183a0 |
| `0x004183b0` | `FUN_004183b0` | sì | 19 | 0 | — | FUN_004183b0 |
| `0x00418410` | `FUN_00418410` | sì | 45 | 0 | — | FUN_00418410 |
| `0x00418510` | `FUN_00418510` | sì | 40 | 0 | — | FUN_00418510 |
| `0x004185c0` | `FUN_004185c0` | sì | 25 | 0 | — | FUN_004185c0 |
| `0x004187a0` | `FUN_004187a0` | no | 16 | 0 | — | FUN_004187a0 |
| `0x004187c0` | `FUN_004187c0` | no | 22 | 0 | — | FUN_004187c0 |
| `0x00418870` | `FUN_00418870` | no | 12 | 0 | — | FUN_00418870 |
| `0x00418880` | `FUN_00418880` | sì | 76 | 0 | — | FUN_00418880, FUN_004273f0 |
| `0x00418aa0` | `FUN_00418aa0` | sì | 91 | 0 | — | FUN_00418aa0, FUN_00418880 |
| `0x00418c80` | `FUN_00418c80` | sì | 40 | 0 | — | FUN_00418c80, FUN_00427300, FUN_00418aa0, FUN_004183b0 |
| `0x00418d70` | `FUN_00418d70` | no | 72 | 0 | — | FUN_00418d70, FUN_00418c80 |
| `0x00418e00` | `FUN_00418e00` | sì | 24 | 0 | — | FUN_00418e00 |
| `0x00418e50` | `FUN_00418e50` | sì | 33 | 0 | — | FUN_00418e50, FUN_00418c80 |
| `0x00418f10` | `FUN_00418f10` | no | 19 | 0 | — | FUN_00418f10, FUN_00418c80, FUN_00418aa0, FUN_004183b0 |
| `0x00418f60` | `FUN_00418f60` | no | 42 | 0 | — | FUN_00418f60 |
| `0x004191d8` | `FUN_004191d8` | no | 99 | 0 | — | FUN_004191d8 |
| `0x00419330` | `FUN_00419330` | sì | 12 | 0 | — | FUN_00419330 |
| `0x00419370` | `FUN_00419370` | sì | 17 | 0 | — | FUN_00419370, FUN_00419330 |
| `0x004193e0` | `FUN_004193e0` | sì | 24 | 0 | — | FUN_004193e0 |
| `0x00419450` | `FUN_00419450` | sì | 40 | 0 | — | FUN_00419450 |
| `0x00419580` | `FUN_00419580` | sì | 89 | 0 | — | FUN_00419580, FUN_00418e00, FUN_00419450, FUN_00418e50 |
| `0x00419860` | `FUN_00419860` | sì | 178 | 0 | — | FUN_00419860, FUN_00418c80, FUN_00418e00, FUN_00418e50 |
| `0x00419c10` | `FUN_00419c10` | no | 17 | 0 | — | FUN_00419c10 |
| `0x0041a080` | `FUN_0041a080` | no | 68 | 0 | — | FUN_0041a080, FUN_00418c80 |
| `0x0041a130` | `FUN_0041a130` | no | 149 | 0 | — | FUN_0041a130 |
| `0x0041a4b0` | `FUN_0041a4b0` | no | 113 | 0 | — | FUN_0041a4b0 |
| `0x0041a760` | `FUN_0041a760` | sì | 17 | 0 | — | FUN_0041a760, FUN_0041ea40 |
| `0x0041a7b0` | `FUN_0041a7b0` | no | 9 | 0 | — | FUN_0041a7b0 |
| `0x0041a7c0` | `FUN_0041a7c0` | no | 24 | 0 | — | FUN_0041a7c0 |
| `0x0041a980` | `FUN_0041a980` | no | 18 | 0 | — | FUN_0041a980 |
| `0x0041a9e0` | `FUN_0041a9e0` | no | 22 | 0 | — | FUN_0041a9e0 |
| `0x0041aab0` | `FUN_0041aab0` | no | 21 | 0 | — | FUN_0041aab0 |
| `0x0041aaf0` | `FUN_0041aaf0` | no | 93 | 5 | SetLayeredWindowAttributes; MonitorFromWindow; USER32; FB WM_MOUSEENTER emulation; FB_PROGRAM_ICON | FUN_0041aaf0, FUN_0041aab0 |
| `0x0041afa0` | `FUN_0041afa0` | no | 10 | 0 | — | FUN_0041afa0 |
| `0x0041afc0` | `FUN_0041afc0` | no | 10 | 0 | — | FUN_0041afc0 |
| `0x0041b020` | `FUN_0041b020` | no | 16 | 0 | — | FUN_0041b020 |
| `0x0041b320` | `FUN_0041b320` | sì | 22 | 0 | — | FUN_0041b320 |
| `0x0041b3c0` | `FUN_0041b3c0` | sì | 9 | 0 | — | FUN_0041b3c0 |
| `0x0041b3d0` | `FUN_0041b3d0` | no | 27 | 0 | — | FUN_0041b3d0 |
| `0x0041c2c0` | `FUN_0041c2c0` | no | 90 | 0 | — | FUN_0041c2c0, FUN_00418c80, FUN_00418aa0, FUN_004183b0, FUN_00418e00, FUN_00418e50, FUN_00426a00 |
| `0x0041c6b0` | `FUN_0041c6b0` | no | 15 | 0 | — | FUN_0041c6b0, FUN_00418e00, FUN_00418e50 |
| `0x0041c6f0` | `FUN_0041c6f0` | no | 17 | 0 | — | FUN_0041c6f0 |
| `0x0041c740` | `FUN_0041c740` | no | 19 | 0 | — | FUN_0041c740, FUN_00418e00, FUN_0041c6b0, FUN_00418e50 |
| `0x0041c910` | `FUN_0041c910` | no | 29 | 0 | — | FUN_0041c910, FUN_0041c6b0, FUN_00427f80 |
| `0x0041c990` | `FUN_0041c990` | no | 52 | 0 | — | FUN_0041c990, FUN_00418c80 |
| `0x0041cc21` | `FUN_0041cc21` | no | 23 | 0 | — | FUN_0041cc21, FUN_00428c20, FUN_00424780, FUN_00426a00, FUN_00424660 |
| `0x0041cccf` | `FUN_0041cccf` | no | 10 | 0 | — | FUN_0041cccf, FUN_0041cc21 |
| `0x0041cfa0` | `FUN_0041cfa0` | no | 18 | 0 | — | FUN_0041cfa0, FUN_00426a00, FUN_00418e00, FUN_00418e50 |
| `0x0041d030` | `FUN_0041d030` | no | 44 | 0 | — | FUN_0041d030, FUN_00418e00, FUN_00418e50, FUN_00426a00 |
| `0x0041d140` | `FUN_0041d140` | no | 20 | 0 | — | FUN_0041d140, FUN_00418e00, FUN_00418e50 |
| `0x0041d1c0` | `FUN_0041d1c0` | no | 20 | 0 | — | FUN_0041d1c0 |
| `0x0041d320` | `FUN_0041d320` | no | 41 | 0 | — | FUN_0041d320, FUN_00426f90, FUN_0041d990 |
| `0x0041d3e0` | `FUN_0041d3e0` | no | 13 | 0 | — | FUN_0041d3e0, FUN_0041d320 |
| `0x0041d410` | `FUN_0041d410` | no | 120 | 0 | — | FUN_0041d410, FUN_00416890 |
| `0x0041d950` | `FUN_0041d950` | no | 18 | 0 | — | FUN_0041d950 |
| `0x0041d990` | `FUN_0041d990` | no | 70 | 0 | — | FUN_0041d990, FUN_00418c80, FUN_00418aa0, FUN_004183b0, FUN_00418e00, FUN_00429010, FUN_00427260, FUN_0041d410 |
| `0x0041dbe0` | `FUN_0041dbe0` | no | 13 | 0 | — | FUN_0041dbe0, FUN_0041d990 |
| `0x0041dc10` | `FUN_0041dc10` | no | 15 | 0 | — | FUN_0041dc10, FUN_0041d950, FUN_00429090 |
| `0x0041dc80` | `FUN_0041dc80` | no | 9 | 0 | — | FUN_0041dc80 |
| `0x0041dc90` | `FUN_0041dc90` | no | 9 | 0 | — | FUN_0041dc90 |
| `0x0041dd10` | `FUN_0041dd10` | no | 61 | 0 | — | FUN_0041dd10, FUN_00418c80, FUN_00416930, FUN_00421520, FUN_00425bf0, FUN_0041e0b0 |
| `0x0041de50` | `FUN_0041de50` | no | 85 | 0 | — | FUN_0041de50, FUN_00418c80, FUN_00419580, FUN_00419330 |
| `0x0041e0b0` | `FUN_0041e0b0` | no | 101 | 0 | — | FUN_0041e0b0, FUN_00418c80, FUN_00418aa0, FUN_004183b0, FUN_00418e00, FUN_004271d0, FUN_004271f0, FUN_00416890 |
| `0x0041e420` | `FUN_0041e420` | no | 21 | 0 | — | FUN_0041e420 |
| `0x0041e5d0` | `FUN_0041e5d0` | no | 89 | 0 | — | FUN_0041e5d0 |
| `0x0041e7d0` | `FUN_0041e7d0` | no | 51 | 3 | glEnable; glDisable; GL_EXT_bgra | FUN_0041e7d0, FUN_00429480, FUN_0041e420 |
| `0x0041e9f0` | `FUN_0041e9f0` | sì | 25 | 0 | — | FUN_0041e9f0 |
| `0x0041ea40` | `FUN_0041ea40` | sì | 89 | 0 | — | FUN_0041ea40, FUN_00426f90, FUN_0041e9f0 |
| `0x0041ec40` | `FUN_0041ec40` | no | 239 | 5 | ddraw.dll; dinput.dll; DirectDrawCreate; DirectDrawEnumerateExA; DirectInputCreateA | FUN_0041ec40, FUN_0041a9e0, FUN_00422d30 |
| `0x0041f500` | `FUN_0041f500` | no | 67 | 0 | — | FUN_0041f500 |
| `0x0041f730` | `FUN_0041f730` | no | 38 | 0 | — | FUN_0041f730, FUN_0041f500, FUN_0041ec40 |
| `0x0041f810` | `FUN_0041f810` | no | 37 | 0 | — | FUN_0041f810, FUN_0041f730 |
| `0x0041fb60` | `FUN_0041fb60` | no | 30 | 0 | — | FUN_0041fb60, FUN_0041aaf0 |
| `0x0041fd60` | `FUN_0041fd60` | no | 47 | 2 | ddraw.dll; DirectDrawCreate | FUN_0041fd60 |
| `0x0041fea0` | `FUN_0041fea0` | no | 39 | 0 | — | FUN_0041fea0 |
| `0x00420010` | `FUN_00420010` | no | 130 | 0 | — | FUN_00420010, FUN_0041a9e0, FUN_00422d30 |
| `0x00420410` | `FUN_00420410` | no | 28 | 0 | — | FUN_00420410 |
| `0x004208a0` | `FUN_004208a0` | no | 45 | 0 | — | FUN_004208a0 |
| `0x004209b0` | `FUN_004209b0` | no | 10 | 0 | — | FUN_004209b0 |
| `0x004209e0` | `FUN_004209e0` | no | 71 | 0 | — | FUN_004209e0 |
| `0x00421480` | `FUN_00421480` | no | 32 | 0 | — | FUN_00421480 |
| `0x00421520` | `FUN_00421520` | no | 84 | 0 | — | FUN_00421520, FUN_00418c80, FUN_00418aa0, FUN_004183b0, FUN_004185c0, FUN_00419370, FUN_00419860, FUN_0041e0b0 |
| `0x00421740` | `FUN_00421740` | no | 9 | 0 | — | FUN_00421740 |
| `0x00422d30` | `FUN_00422d30` | no | 96 | 0 | — | FUN_00422d30 |
| `0x00422f10` | `FUN_00422f10` | no | 65 | 0 | — | FUN_00422f10 |
| `0x004231b8` | `FUN_004231b8` | no | 66 | 0 | — | FUN_004231b8 |
| `0x004232a0` | `FUN_004232a0` | no | 50 | 0 | — | FUN_004232a0 |
| `0x00423de0` | `FUN_00423de0` | sì | 105 | 0 | — | FUN_00423de0, FUN_00426e30 |
| `0x00424010` | `FUN_00424010` | sì | 31 | 0 | — | FUN_00424010, FUN_00426d70, FUN_00426eb0, FUN_00426e30 |
| `0x004240d0` | `FUN_004240d0` | sì | 49 | 0 | — | FUN_004240d0, FUN_00426d70, FUN_00426e30 |
| `0x004241c0` | `FUN_004241c0` | sì | 54 | 0 | — | FUN_004241c0, FUN_00426f00, FUN_00429570 |
| `0x004242b0` | `FUN_004242b0` | sì | 22 | 0 | — | FUN_004242b0, FUN_004241c0, FUN_00426e70 |
| `0x00424300` | `FUN_00424300` | sì | 30 | 0 | — | FUN_00424300 |
| `0x00424390` | `FUN_00424390` | sì | 17 | 0 | — | FUN_00424390, FUN_00429640 |
| `0x00424400` | `FUN_00424400` | sì | 105 | 0 | — | FUN_00424400, FUN_00426e30, FUN_004246e0, FUN_00426bd0, FUN_00426c80, FUN_00426c10, FUN_00426eb0 |
| `0x00424660` | `FUN_00424660` | sì | 10 | 0 | — | FUN_00424660, FUN_00424400 |
| `0x004246e0` | `FUN_004246e0` | sì | 19 | 0 | — | FUN_004246e0 |
| `0x00424730` | `FUN_00424730` | sì | 17 | 0 | — | FUN_00424730, FUN_00429880, FUN_00429740 |
| `0x00424780` | `FUN_00424780` | sì | 17 | 0 | — | FUN_00424780, FUN_00424730 |
| `0x004247d0` | `FUN_004247d0` | sì | 22 | 0 | — | FUN_004247d0, FUN_00426b20 |
| `0x00424860` | `FUN_00424860` | sì | 25 | 0 | — | FUN_00424860, FUN_00424730, FUN_00426e70, FUN_00429880 |
| `0x00424920` | `FUN_00424920` | sì | 17 | 0 | — | FUN_00424920, FUN_00424860 |
| `0x00424970` | `FUN_00424970` | sì | 19 | 0 | — | FUN_00424970, FUN_00426de0 |
| `0x00424a50` | `FUN_00424a50` | sì | 33 | 0 | — | FUN_00424a50, FUN_00426d70, FUN_00426eb0, FUN_00426e30 |
| `0x00424b20` | `FUN_00424b20` | sì | 79 | 0 | — | FUN_00424b20, FUN_004246e0, FUN_00426de0, FUN_00426e30 |
| `0x00424cc0` | `FUN_00424cc0` | sì | 19 | 0 | — | FUN_00424cc0, FUN_004274b0 |
| `0x00424d30` | `FUN_00424d30` | sì | 15 | 0 | — | FUN_00424d30 |
| `0x00424e40` | `FUN_00424e40` | no | 53 | 0 | — | FUN_00424e40, FUN_00424300 |
| `0x00424fc0` | `FUN_00424fc0` | no | 24 | 0 | — | FUN_00424fc0, FUN_00424e40, FUN_00424d30 |
| `0x004251d0` | `FUN_004251d0` | sì | 19 | 0 | — | FUN_004251d0, FUN_00429950 |
| `0x00425230` | `FUN_00425230` | sì | 19 | 0 | — | FUN_00425230, FUN_004299d0 |
| `0x00425290` | `FUN_00425290` | sì | 19 | 0 | — | FUN_00425290, FUN_00429a80 |
| `0x004252f0` | `FUN_004252f0` | no | 20 | 0 | — | FUN_004252f0, FUN_00429af0 |
| `0x00425350` | `FUN_00425350` | sì | 22 | 0 | — | FUN_00425350, FUN_00429c60, FUN_00429b10 |
| `0x004253c0` | `FUN_004253c0` | sì | 20 | 0 | — | FUN_004253c0, FUN_004251d0, FUN_00426a00, FUN_00427f80 |
| `0x004254a0` | `FUN_004254a0` | sì | 25 | 1 | %.16g | FUN_004254a0, FUN_00426de0 |
| `0x00425530` | `FUN_00425530` | sì | 10 | 0 | — | FUN_00425530 |
| `0x00425550` | `FUN_00425550` | no | 28 | 0 | — | FUN_00425550, FUN_00429d00, FUN_00429da0, FUN_00427370 |
| `0x004255e0` | `FUN_004255e0` | sì | 19 | 0 | — | FUN_004255e0, FUN_00429e30 |
| `0x00425640` | `FUN_00425640` | sì | 54 | 0 | — | FUN_00425640, FUN_00426e70, FUN_00426c80, FUN_00426eb0 |
| `0x004257c0` | `FUN_004257c0` | no | 43 | 1 | % d | FUN_004257c0, FUN_00429880, FUN_00429740 |
| `0x004258f0` | `FUN_004258f0` | sì | 43 | 1 | %u | FUN_004258f0, FUN_00429880, FUN_00429740 |
| `0x00425a20` | `FUN_00425a20` | sì | 19 | 0 | — | FUN_00425a20, FUN_00429f00 |
| `0x00425ac0` | `FUN_00425ac0` | sì | 10 | 0 | — | FUN_00425ac0, FUN_00429fa0 |
| `0x00425ad0` | `FUN_00425ad0` | sì | 56 | 0 | — | FUN_00425ad0, FUN_00429010, FUN_004271d0, FUN_004271f0, FUN_00427240, FUN_0042a000, FUN_00424390 |
| `0x00425bf0` | `FUN_00425bf0` | sì | 10 | 0 | — | FUN_00425bf0, FUN_00425ad0 |
| `0x00425c20` | `FUN_00425c20` | no | 11 | 0 | — | FUN_00425c20, FUN_004273c0 |
| `0x00425c50` | `FUN_00425c50` | sì | 27 | 0 | — | FUN_00425c50 |
| `0x00425cd0` | `FUN_00425cd0` | sì | 14 | 0 | — | FUN_00425cd0 |
| `0x00425d10` | `FUN_00425d10` | sì | 23 | 0 | — | FUN_00425d10, FUN_00426e70 |
| `0x00425d60` | `FUN_00425d60` | sì | 22 | 0 | — | FUN_00425d60, FUN_00426e70 |
| `0x00425dd0` | `FUN_00425dd0` | sì | 30 | 0 | — | FUN_00425dd0, FUN_00426de0 |
| `0x00425e40` | `FUN_00425e40` | sì | 42 | 0 | — | FUN_00425e40, FUN_00424660, FUN_00426a00, FUN_0042a250, FUN_004246e0 |
| `0x00425f80` | `FUN_00425f80` | sì | 20 | 0 | — | FUN_00425f80, FUN_00426a00, FUN_00425e40 |
| `0x00426000` | `FUN_00426000` | sì | 35 | 0 | — | FUN_00426000, FUN_00427300, FUN_004246e0, FUN_00426a00 |
| `0x004260f0` | `FUN_004260f0` | sì | 19 | 0 | — | FUN_004260f0, FUN_00426f90, FUN_0042a6d0, FUN_00424660, FUN_00426a00 |
| `0x00426170` | `FUN_00426170` | sì | 31 | 0 | — | FUN_00426170, FUN_00426a00 |
| `0x00426210` | `FUN_00426210` | sì | 21 | 0 | — | FUN_00426210, FUN_00429d00, FUN_00426a00, FUN_00426170 |
| `0x00426270` | `FUN_00426270` | sì | 31 | 0 | — | FUN_00426270, FUN_00429880 |
| `0x00426330` | `FUN_00426330` | sì | 33 | 0 | — | FUN_00426330, FUN_00426270, FUN_00426e70, FUN_00429880 |
| `0x004264d0` | `FUN_004264d0` | sì | 24 | 1 | %02d-%02d-%04d | FUN_004264d0, FUN_00426de0 |
| `0x00426580` | `FUN_00426580` | sì | 31 | 0 | — | FUN_00426580, FUN_00429c60, FUN_0042a8d0, FUN_00425bf0 |
| `0x00426640` | `FUN_00426640` | no | 20 | 0 | — | FUN_00426640, FUN_0042ab70 |
| `0x004266f0` | `FUN_004266f0` | sì | 19 | 0 | — | FUN_004266f0, FUN_0042ad20 |
| `0x004267a0` | `FUN_004267a0` | sì | 33 | 0 | — | FUN_004267a0 |
| `0x00426820` | `FUN_00426820` | sì | 12 | 0 | — | FUN_00426820 |
| `0x00426840` | `FUN_00426840` | no | 20 | 0 | — | FUN_00426840, FUN_00426de0 |
| `0x004268c0` | `FUN_004268c0` | no | 25 | 0 | — | FUN_004268c0, FUN_00429510 |
| `0x00426930` | `FUN_00426930` | sì | 14 | 0 | — | FUN_00426930 |
| `0x00426950` | `FUN_00426950` | sì | 38 | 0 | — | FUN_00426950, FUN_00426e70, FUN_00426a00 |
| `0x00426a00` | `FUN_00426a00` | sì | 13 | 0 | — | FUN_00426a00, FUN_00427300 |
| `0x00426b20` | `FUN_00426b20` | sì | 23 | 0 | — | FUN_00426b20, FUN_0042ad40, FUN_0042ada0 |
| `0x00426b90` | `FUN_00426b90` | sì | 13 | 0 | — | FUN_00426b90, FUN_0042add0 |
| `0x00426bd0` | `FUN_00426bd0` | sì | 16 | 0 | — | FUN_00426bd0, FUN_00426b90 |
| `0x00426c10` | `FUN_00426c10` | sì | 27 | 0 | — | FUN_00426c10 |
| `0x00426c80` | `FUN_00426c80` | sì | 51 | 0 | — | FUN_00426c80, FUN_004246e0 |
| `0x00426d70` | `FUN_00426d70` | sì | 29 | 0 | — | FUN_00426d70, FUN_00426b20, FUN_00426c80, FUN_00426bd0 |
| `0x00426de0` | `FUN_00426de0` | sì | 14 | 0 | — | FUN_00426de0, FUN_00426d70 |
| `0x00426e30` | `FUN_00426e30` | sì | 18 | 0 | — | FUN_00426e30, FUN_004246e0, FUN_00426bd0 |
| `0x00426e70` | `FUN_00426e70` | sì | 14 | 0 | — | FUN_00426e70, FUN_00426e30 |
| `0x00426eb0` | `FUN_00426eb0` | sì | 24 | 0 | — | FUN_00426eb0 |
| `0x00426f00` | `FUN_00426f00` | sì | 13 | 0 | — | FUN_00426f00 |
| `0x00426f90` | `FUN_00426f90` | sì | 20 | 0 | — | FUN_00426f90 |
| `0x00426fc0` | `FUN_00426fc0` | sì | 29 | 0 | — | FUN_00426fc0 |
| `0x00427000` | `FUN_00427000` | sì | 20 | 0 | — | FUN_00427000 |
| `0x00427050` | `FUN_00427050` | sì | 13 | 0 | — | FUN_00427050 |
| `0x00427080` | `FUN_00427080` | no | 12 | 0 | — | FUN_00427080 |
| `0x004270b0` | `FUN_004270b0` | no | 12 | 0 | — | FUN_004270b0 |
| `0x004270e0` | `FUN_004270e0` | no | 24 | 0 | — | FUN_004270e0, FUN_00429c60, FUN_0042aef0 |
| `0x00427170` | `FUN_00427170` | sì | 18 | 0 | — | FUN_00427170 |
| `0x004271d0` | `FUN_004271d0` | sì | 13 | 0 | — | FUN_004271d0 |
| `0x004271f0` | `FUN_004271f0` | sì | 18 | 0 | — | FUN_004271f0, FUN_00429010 |
| `0x00427240` | `FUN_00427240` | sì | 11 | 0 | — | FUN_00427240 |
| `0x00427260` | `FUN_00427260` | sì | 15 | 0 | — | FUN_00427260, FUN_004271d0, FUN_004271f0 |
| `0x00427280` | `FUN_00427280` | no | 30 | 0 | — | FUN_00427280, FUN_00426b20 |
| `0x00427300` | `FUN_00427300` | sì | 16 | 0 | — | FUN_00427300 |
| `0x00427370` | `FUN_00427370` | no | 16 | 0 | — | FUN_00427370 |
| `0x004273c0` | `FUN_004273c0` | no | 18 | 0 | — | FUN_004273c0, FUN_00427370 |
| `0x004273f0` | `FUN_004273f0` | sì | 67 | 0 | — | FUN_004273f0 |
| `0x00427460` | `FUN_00427460` | no | 22 | 0 | — | FUN_00427460 |
| `0x004274b0` | `FUN_004274b0` | sì | 54 | 1 | _fb_console_title %f | FUN_004274b0, FUN_00424300 |
| `0x004275e0` | `FUN_004275e0` | sì | 28 | 0 | — | FUN_004275e0 |
| `0x00427760` | `FUN_00427760` | sì | 111 | 0 | — | FUN_00427760 |
| `0x00427990` | `FUN_00427990` | sì | 148 | 0 | — | FUN_00427990, FUN_00401280, FUN_0042afd0, FUN_004275e0, FUN_00427760 |
| `0x00427e40` | `FUN_00427e40` | sì | 16 | 0 | — | FUN_00427e40, FUN_00427990 |
| `0x00427e80` | `FUN_00427e80` | sì | 29 | 0 | — | FUN_00427e80, FUN_00427990, FUN_00427e40 |
| `0x00427f40` | `FUN_00427f40` | no | 10 | 0 | — | FUN_00427f40, FUN_00427e80 |
| `0x00427f60` | `FUN_00427f60` | sì | 10 | 0 | — | FUN_00427f60, FUN_00427e80 |
| `0x00427f80` | `FUN_00427f80` | sì | 10 | 0 | — | FUN_00427f80 |
| `0x00427f90` | `FUN_00427f90` | no | 17 | 0 | — | FUN_00427f90, FUN_0042b350 |
| `0x00428050` | `FUN_00428050` | no | 22 | 0 | — | FUN_00428050, FUN_0042b730, FUN_0042b7d0 |
| `0x004280f0` | `FUN_004280f0` | no | 17 | 0 | — | FUN_004280f0, FUN_00428050 |
| `0x00428140` | `FUN_00428140` | no | 26 | 0 | — | FUN_00428140 |
| `0x004281b0` | `FUN_004281b0` | no | 26 | 0 | — | FUN_004281b0, FUN_00428140, FUN_00424390, FUN_00427f90 |
| `0x00428270` | `FUN_00428270` | no | 216 | 0 | — | FUN_00428270, FUN_00429010, FUN_00424390, FUN_00427f90, FUN_0042b8b0, FUN_00428fc0, FUN_004252f0, FUN_00427f80 |
| `0x00428c20` | `FUN_00428c20` | no | 15 | 0 | — | FUN_00428c20, FUN_00426b20, FUN_00428270 |
| `0x00428c50` | `FUN_00428c50` | no | 45 | 0 | — | FUN_00428c50, FUN_00426e70 |
| `0x00428f50` | `FUN_00428f50` | no | 20 | 0 | — | FUN_00428f50, FUN_00424730, FUN_00429880 |
| `0x00428fc0` | `FUN_00428fc0` | no | 17 | 0 | — | FUN_00428fc0, FUN_00428f50 |
| `0x00429010` | `FUN_00429010` | sì | 17 | 0 | — | FUN_00429010, FUN_0042baa0 |
| `0x00429090` | `FUN_00429090` | no | 13 | 0 | — | FUN_00429090, FUN_00426a00 |
| `0x004290d0` | `FUN_004290d0` | no | 113 | 0 | — | FUN_004290d0, FUN_0042ba80, FUN_0042bb30 |
| `0x00429410` | `FUN_00429410` | no | 32 | 0 | — | FUN_00429410 |
| `0x00429480` | `FUN_00429480` | no | 23 | 0 | — | FUN_00429480 |
| `0x00429510` | `FUN_00429510` | no | 21 | 0 | — | FUN_00429510 |
| `0x00429570` | `FUN_00429570` | sì | 70 | 0 | — | FUN_00429570 |
| `0x00429640` | `FUN_00429640` | sì | 42 | 0 | — | FUN_00429640, FUN_00426a00, FUN_00427260, FUN_00429010, FUN_00429c60, FUN_0042bc00 |
| `0x00429740` | `FUN_00429740` | sì | 41 | 0 | — | FUN_00429740, FUN_0042bd10, FUN_00429880 |
| `0x00429830` | `FUN_00429830` | no | 17 | 0 | — | FUN_00429830, FUN_00429740 |
| `0x00429880` | `FUN_00429880` | sì | 15 | 0 | — | FUN_00429880, FUN_0042bd10, FUN_00426a00, FUN_0042bd30 |
| `0x004298f0` | `FUN_004298f0` | no | 19 | 0 | — | FUN_004298f0, FUN_00429880 |
| `0x00429950` | `FUN_00429950` | sì | 29 | 0 | — | FUN_00429950, FUN_00427e40, FUN_00427f80 |
| `0x004299d0` | `FUN_004299d0` | sì | 33 | 0 | — | FUN_004299d0, FUN_00427f60, FUN_00426de0 |
| `0x00429a80` | `FUN_00429a80` | sì | 22 | 0 | — | FUN_00429a80, FUN_00427f60, FUN_004251d0 |
| `0x00429af0` | `FUN_00429af0` | no | 12 | 0 | — | FUN_00429af0, FUN_00427f40 |
| `0x00429b10` | `FUN_00429b10` | sì | 32 | 0 | — | FUN_00429b10, FUN_0042c480, FUN_00427260, FUN_0042c150, FUN_0042afd0, FUN_0042c2b0 |
| `0x00429c10` | `FUN_00429c10` | sì | 22 | 0 | — | FUN_00429c10, FUN_00429010, FUN_00429fa0 |
| `0x00429c60` | `FUN_00429c60` | sì | 29 | 0 | — | FUN_00429c60, FUN_00429c10 |
| `0x00429d00` | `FUN_00429d00` | sì | 33 | 0 | — | FUN_00429d00 |
| `0x00429da0` | `FUN_00429da0` | no | 12 | 0 | — | FUN_00429da0 |
| `0x00429de0` | `FUN_00429de0` | sì | 18 | 0 | — | FUN_00429de0 |
| `0x00429e10` | `FUN_00429e10` | sì | 13 | 0 | — | FUN_00429e10, FUN_0042afd0, FUN_00429de0 |
| `0x00429e30` | `FUN_00429e30` | sì | 13 | 0 | — | FUN_00429e30, FUN_00429e10, FUN_0042c200 |
| `0x00429e70` | `FUN_00429e70` | no | 24 | 0 | — | FUN_00429e70, FUN_0042c580 |
| `0x00429f00` | `FUN_00429f00` | sì | 29 | 0 | — | FUN_00429f00, FUN_0042afd0 |
| `0x00429fa0` | `FUN_00429fa0` | sì | 19 | 0 | — | FUN_00429fa0, FUN_0042c620 |
| `0x0042a000` | `FUN_0042a000` | sì | 17 | 0 | — | FUN_0042a000, FUN_0042c660 |
| `0x0042a050` | `FUN_0042a050` | sì | 95 | 0 | — | FUN_0042a050, FUN_00426a00 |
| `0x0042a250` | `FUN_0042a250` | sì | 23 | 0 | — | FUN_0042a250, FUN_00426f90, FUN_00426eb0, FUN_00426e70, FUN_0042a050 |
| `0x0042a2f0` | `FUN_0042a2f0` | no | 116 | 0 | — | FUN_0042a2f0, FUN_00426f90, FUN_00426fc0, FUN_00426a00, FUN_0042c700, FUN_0042c670 |
| `0x0042a5f0` | `FUN_0042a5f0` | sì | 35 | 0 | — | FUN_0042a5f0, FUN_0042d600 |
| `0x0042a680` | `FUN_0042a680` | sì | 24 | 0 | — | FUN_0042a680, FUN_0042d490 |
| `0x0042a6d0` | `FUN_0042a6d0` | sì | 103 | 0 | — | FUN_0042a6d0, FUN_00427300, FUN_0042a5f0, FUN_0042a680 |
| `0x0042a8d0` | `FUN_0042a8d0` | sì | 90 | 0 | — | FUN_0042a8d0, FUN_0042c480, FUN_0042afd0, FUN_0042dbd0, FUN_0042c3b0 |
| `0x0042ab70` | `FUN_0042ab70` | no | 52 | 0 | — | FUN_0042ab70, FUN_0042afd0, FUN_0042c150, FUN_00427990, FUN_0042c200, FUN_00426a00 |
| `0x0042ad20` | `FUN_0042ad20` | sì | 10 | 0 | — | FUN_0042ad20, FUN_00426a00 |
| `0x0042ad40` | `FUN_0042ad40` | sì | 27 | 0 | — | FUN_0042ad40, FUN_0042dcb0 |
| `0x0042ada0` | `FUN_0042ada0` | sì | 19 | 0 | — | FUN_0042ada0, FUN_0042dcd0 |
| `0x0042add0` | `FUN_0042add0` | sì | 12 | 0 | — | FUN_0042add0, FUN_0042dd10 |
| `0x0042ae10` | `FUN_0042ae10` | no | 20 | 0 | — | FUN_0042ae10 |
| `0x0042ae80` | `FUN_0042ae80` | no | 18 | 0 | — | FUN_0042ae80 |
| `0x0042aef0` | `FUN_0042aef0` | no | 49 | 0 | — | FUN_0042aef0, FUN_0042afd0, FUN_0042ae10, FUN_0042ae80 |
| `0x0042afd0` | `FUN_0042afd0` | sì | 28 | 0 | — | FUN_0042afd0 |
| `0x0042b0a0` | `FUN_0042b0a0` | no | 91 | 0 | — | FUN_0042b0a0 |
| `0x0042b350` | `FUN_0042b350` | no | 117 | 0 | — | FUN_0042b350, FUN_00427260, FUN_0042dbd0, FUN_0042afd0, FUN_004290d0, FUN_0042c3b0 |
| `0x0042b730` | `FUN_0042b730` | no | 17 | 0 | — | FUN_0042b730, FUN_0042b7d0, FUN_0042dd70 |
| `0x0042b7d0` | `FUN_0042b7d0` | no | 15 | 0 | — | FUN_0042b7d0, FUN_0042def0, FUN_00426a00, FUN_0042bd30 |
| `0x0042b8b0` | `FUN_0042b8b0` | no | 17 | 0 | — | FUN_0042b8b0, FUN_0042dfb0 |
| `0x0042b930` | `FUN_0042b930` | no | 22 | 0 | — | FUN_0042b930, FUN_00426b20 |
| `0x0042b990` | `FUN_0042b990` | no | 20 | 0 | — | FUN_0042b990, FUN_00426de0 |
| `0x0042ba80` | `FUN_0042ba80` | no | 10 | 0 | — | FUN_0042ba80 |
| `0x0042baa0` | `FUN_0042baa0` | sì | 26 | 0 | — | FUN_0042baa0, FUN_0042c480, FUN_0042dbd0 |
| `0x0042bb30` | `FUN_0042bb30` | no | 46 | 0 | — | FUN_0042bb30 |
| `0x0042bc00` | `FUN_0042bc00` | sì | 28 | 0 | — | FUN_0042bc00, FUN_0042c620, FUN_00429e30, FUN_0042afd0, FUN_0042c150, FUN_0042e040 |
| `0x0042bd10` | `FUN_0042bd10` | sì | 13 | 0 | — | FUN_0042bd10, FUN_00429c60 |
| `0x0042bd30` | `FUN_0042bd30` | sì | 101 | 0 | — | FUN_0042bd30, FUN_00426a00, FUN_0042e150 |
| `0x0042c0f0` | `FUN_0042c0f0` | no | 29 | 0 | — | FUN_0042c0f0, FUN_0042afd0 |
| `0x0042c150` | `FUN_0042c150` | sì | 28 | 0 | — | FUN_0042c150, FUN_0042c480, FUN_0042dbd0 |
| `0x0042c200` | `FUN_0042c200` | sì | 28 | 0 | — | FUN_0042c200, FUN_0042c480, FUN_0042dbd0 |
| `0x0042c2b0` | `FUN_0042c2b0` | sì | 31 | 0 | — | FUN_0042c2b0, FUN_0042c580, FUN_0042e040 |
| `0x0042c3b0` | `FUN_0042c3b0` | sì | 41 | 0 | — | FUN_0042c3b0, FUN_0042afd0 |
| `0x0042c480` | `FUN_0042c480` | sì | 14 | 0 | — | FUN_0042c480, FUN_0042c3b0 |
| `0x0042c580` | `FUN_0042c580` | sì | 18 | 0 | — | FUN_0042c580 |
| `0x0042c5d0` | `FUN_0042c5d0` | sì | 18 | 0 | — | FUN_0042c5d0 |
| `0x0042c600` | `FUN_0042c600` | sì | 13 | 0 | — | FUN_0042c600, FUN_0042afd0, FUN_0042c5d0 |
| `0x0042c620` | `FUN_0042c620` | sì | 13 | 0 | — | FUN_0042c620, FUN_0042c600, FUN_0042c200 |
| `0x0042c660` | `FUN_0042c660` | sì | 10 | 0 | — | FUN_0042c660, FUN_0042c3b0 |
| `0x0042c670` | `FUN_0042c670` | no | 29 | 0 | — | FUN_0042c670, FUN_0042f730 |
| `0x0042c700` | `FUN_0042c700` | no | 47 | 0 | — | FUN_0042c700, FUN_0042f730, FUN_0042c670 |
| `0x0042c800` | `FUN_0042c800` | no | 18 | 0 | — | FUN_0042c800, FUN_00426a00 |
| `0x0042c960` | `FUN_0042c960` | no | 10 | 0 | — | FUN_0042c960 |
| `0x0042c970` | `FUN_0042c970` | no | 90 | 0 | — | FUN_0042c970, FUN_00426a00, FUN_004246e0, FUN_00425640, FUN_00424660 |
| `0x0042cc70` | `FUN_0042cc70` | no | 22 | 0 | — | FUN_0042cc70, FUN_0042e340, FUN_00426a00 |
| `0x0042cd10` | `FUN_0042cd10` | no | 22 | 0 | — | FUN_0042cd10, FUN_0042e2f0, FUN_00426a00 |
| `0x0042cdb0` | `FUN_0042cdb0` | no | 60 | 0 | — | FUN_0042cdb0, FUN_00426f90, FUN_00426a00 |
| `0x0042ced0` | `FUN_0042ced0` | no | 25 | 0 | — | FUN_0042ced0, FUN_00426a00 |
| `0x0042cf90` | `FUN_0042cf90` | no | 71 | 0 | — | FUN_0042cf90, FUN_00426f90, FUN_00426a00 |
| `0x0042d130` | `FUN_0042d130` | no | 39 | 0 | — | FUN_0042d130, FUN_00426a00 |
| `0x0042d210` | `FUN_0042d210` | no | 32 | 0 | — | FUN_0042d210, FUN_00426a00 |
| `0x0042d2a0` | `FUN_0042d2a0` | no | 21 | 0 | — | FUN_0042d2a0, FUN_00426a00, FUN_0042f730 |
| `0x0042d340` | `FUN_0042d340` | no | 16 | 0 | — | FUN_0042d340, FUN_00426a00 |
| `0x0042d3a0` | `FUN_0042d3a0` | no | 38 | 0 | — | FUN_0042d3a0 |
| `0x0042d490` | `FUN_0042d490` | sì | 52 | 0 | — | FUN_0042d490, FUN_00426a00 |
| `0x0042d600` | `FUN_0042d600` | sì | 144 | 0 | — | FUN_0042d600, FUN_00426a00, FUN_0042e150 |
| `0x0042dbd0` | `FUN_0042dbd0` | sì | 48 | 0 | — | FUN_0042dbd0, FUN_0042c480 |
| `0x0042dcb0` | `FUN_0042dcb0` | sì | 15 | 0 | — | FUN_0042dcb0 |
| `0x0042dcd0` | `FUN_0042dcd0` | sì | 25 | 0 | — | FUN_0042dcd0 |
| `0x0042dd10` | `FUN_0042dd10` | sì | 33 | 0 | — | FUN_0042dd10 |
| `0x0042dd70` | `FUN_0042dd70` | no | 43 | 0 | — | FUN_0042dd70, FUN_0042def0, FUN_0042b7d0 |
| `0x0042de60` | `FUN_0042de60` | no | 17 | 0 | — | FUN_0042de60, FUN_0042dd70 |
| `0x0042def0` | `FUN_0042def0` | no | 13 | 0 | — | FUN_0042def0, FUN_00429c60 |
| `0x0042df10` | `FUN_0042df10` | no | 30 | 0 | — | FUN_0042df10 |
| `0x0042dfb0` | `FUN_0042dfb0` | no | 33 | 0 | — | FUN_0042dfb0, FUN_0042afd0, FUN_0042c200 |
| `0x0042e040` | `FUN_0042e040` | sì | 26 | 0 | — | FUN_0042e040, FUN_0042c5d0, FUN_00429de0, FUN_0042afd0 |
| `0x0042e150` | `FUN_0042e150` | sì | 38 | 0 | — | FUN_0042e150, FUN_00426a00 |
| `0x0042e2a0` | `FUN_0042e2a0` | no | 17 | 0 | — | FUN_0042e2a0, FUN_0042e150 |
| `0x0042e2f0` | `FUN_0042e2f0` | no | 15 | 0 | — | FUN_0042e2f0, FUN_00426a00 |
| `0x0042e340` | `FUN_0042e340` | no | 15 | 0 | — | FUN_0042e340, FUN_00426a00 |
| `0x0042e390` | `FUN_0042e390` | no | 17 | 0 | — | FUN_0042e390, FUN_0042e700 |
| `0x0042e450` | `FUN_0042e450` | no | 91 | 0 | — | FUN_0042e450 |
| `0x0042e700` | `FUN_0042e700` | no | 122 | 0 | — | FUN_0042e700, FUN_00427260, FUN_0042dbd0, FUN_0042afd0, FUN_0042eaf0, FUN_0042c3b0 |
| `0x0042eaf0` | `FUN_0042eaf0` | no | 113 | 0 | — | FUN_0042eaf0, FUN_0042ba80, FUN_0042ee30 |
| `0x0042ee30` | `FUN_0042ee30` | no | 46 | 0 | — | FUN_0042ee30 |
| `0x0042ef10` | `FUN_0042ef10` | no | 195 | 4 | -LIBGCCW32-EH-3-SJLJ-GTHR-MINGW32; w32_sharedptr->size == sizeof(W32_EH_SHARED); ../../gcc-3.4.5/gcc/config/i386/w32-shared-ptr.c; GetAtomNameA (atom, s, sizeof(s)) != 0 | FUN_0042ef10 |
| `0x0042f230` | `_cexit` | no | 12 | 0 | — | — |
| `0x0042f240` | `__p__environ` | no | 12 | 0 | — | — |
| `0x0042f250` | `signal` | no | 14 | 0 | — | — |
| `0x0042f260` | `__p__fmode` | no | 12 | 0 | — | — |
| `0x0042f270` | `_fileno` | no | 14 | 0 | — | — |
| `0x0042f280` | `_setmode` | no | 14 | 0 | — | — |
| `0x0042f290` | `__getmainargs` | no | 15 | 0 | — | — |
| `0x0042f2a0` | `free` | no | 12 | 0 | — | — |
| `0x0042f2b0` | `malloc` | no | 14 | 0 | — | — |
| `0x0042f2c0` | `sqrt` | no | 14 | 0 | — | — |
| `0x0042f2d0` | `sin` | no | 14 | 0 | — | — |
| `0x0042f2e0` | `cos` | no | 14 | 0 | — | — |
| `0x0042f2f0` | `fclose` | no | 14 | 0 | — | — |
| `0x0042f300` | `rewind` | no | 12 | 0 | — | — |
| `0x0042f310` | `_snprintf` | no | 14 | 0 | — | — |
| `0x0042f320` | `fgetc` | no | 14 | 0 | — | — |
| `0x0042f330` | `fseek` | no | 14 | 0 | — | — |
| `0x0042f340` | `fread` | no | 14 | 0 | — | — |
| `0x0042f350` | `fopen` | no | 14 | 0 | — | — |
| `0x0042f360` | `memset` | no | 14 | 0 | — | — |
| `0x0042f370` | `getenv` | no | 14 | 0 | — | — |
| `0x0042f380` | `strrchr` | no | 14 | 0 | — | — |
| `0x0042f390` | `calloc` | no | 14 | 0 | — | — |
| `0x0042f3a0` | `_stricmp` | no | 14 | 0 | — | — |
| `0x0042f3b0` | `strlen` | no | 14 | 0 | — | — |
| `0x0042f3c0` | `memcpy` | no | 14 | 0 | — | — |
| `0x0042f3d0` | `realloc` | no | 14 | 0 | — | — |
| `0x0042f3e0` | `_beginthread` | no | 14 | 0 | — | — |
| `0x0042f3f0` | `strncat` | no | 14 | 0 | — | — |
| `0x0042f400` | `wcslen` | no | 14 | 0 | — | — |
| `0x0042f410` | `strncpy` | no | 14 | 0 | — | — |
| `0x0042f420` | `strstr` | no | 14 | 0 | — | — |
| `0x0042f430` | `strtod` | no | 14 | 0 | — | — |
| `0x0042f450` | `_itoa` | no | 14 | 0 | — | — |
| `0x0042f460` | `srand` | no | 12 | 0 | — | — |
| `0x0042f470` | `rand` | no | 14 | 0 | — | — |
| `0x0042f480` | `sprintf` | no | 14 | 0 | — | — |
| `0x0042f490` | `fprintf` | no | 14 | 0 | — | — |
| `0x0042f4a0` | `exit` | no | 13 | 0 | — | — |
| `0x0042f4b0` | `_beginthreadex` | no | 16 | 0 | — | — |
| `0x0042f4c0` | `localtime` | no | 14 | 0 | — | — |
| `0x0042f4d0` | `time` | no | 14 | 0 | — | — |
| `0x0042f4e0` | `mbstowcs` | no | 14 | 0 | — | — |
| `0x0042f4f0` | `_errno` | no | 14 | 0 | — | — |
| `0x0042f500` | `remove` | no | 14 | 0 | — | — |
| `0x0042f510` | `toupper` | no | 14 | 0 | — | — |
| `0x0042f520` | `tolower` | no | 14 | 0 | — | — |
| `0x0042f530` | `_isctype` | no | 14 | 0 | — | — |
| `0x0042f540` | `_controlfp` | no | 14 | 0 | — | — |
| `0x0042f550` | `setvbuf` | no | 14 | 0 | — | — |
| `0x0042f560` | `freopen` | no | 14 | 0 | — | — |
| `0x0042f570` | `fgetpos` | no | 14 | 0 | — | — |
| `0x0042f580` | `fflush` | no | 14 | 0 | — | — |
| `0x0042f590` | `fgets` | no | 14 | 0 | — | — |
| `0x0042f5a0` | `fwrite` | no | 14 | 0 | — | — |
| `0x0042f5b0` | `ftell` | no | 14 | 0 | — | — |
| `0x0042f5c0` | `memmove` | no | 14 | 0 | — | — |
| `0x0042f5d0` | `_get_osfhandle` | no | 14 | 0 | — | — |
| `0x0042f5e0` | `assert` | no | 12 | 0 | — | — |
| `0x0042f5f0` | `abort` | no | 13 | 0 | — | — |
| `0x0042f600` | `FUN_0042f600` | no | 12 | 0 | — | FUN_0042f600 |
| `0x0042f640` | `FUN_0042f640` | no | 10 | 0 | — | FUN_0042f640, FUN_0042f600 |
| `0x0042f670` | `FUN_0042f670` | no | 9 | 0 | — | FUN_0042f670 |
| `0x0042f680` | `FUN_0042f680` | no | 17 | 0 | — | FUN_0042f680 |
| `0x0042f6b0` | `FUN_0042f6b0` | sì | 25 | 0 | — | FUN_0042f6b0, FUN_00401280 |
| `0x0042f710` | `FUN_0042f710` | sì | 14 | 0 | — | FUN_0042f710, FUN_0042f6b0 |
| `0x0042f730` | `FUN_0042f730` | no | 38 | 0 | — | FUN_0042f730 |
| `0x0042f7e0` | `_strdup` | no | 14 | 0 | — | — |
| `0x0042f7f0` | `FUN_0042f7f0` | no | 10 | 0 | — | FUN_0042f7f0, FUN_004268c0 |
| `0x0042f800` | `FUN_0042f800` | no | 10 | 0 | — | FUN_0042f800, FUN_00425550 |
| `0x0042f810` | `GetVersionExA` | no | 14 | 0 | — | — |
| `0x0042f820` | `GetModuleHandleA` | no | 14 | 0 | — | — |
| `0x0042f830` | `GetProcAddress` | no | 14 | 0 | — | — |
| `0x0042f840` | `InitializeCriticalSection` | no | 12 | 0 | — | — |
| `0x0042f850` | `CreateEventA` | no | 15 | 0 | — | — |
| `0x0042f860` | `WaitForMultipleObjects` | no | 14 | 0 | — | — |
| `0x0042f870` | `CloseHandle` | no | 14 | 0 | — | — |
| `0x0042f880` | `SetThreadPriority` | no | 14 | 0 | — | — |
| `0x0042f890` | `WaitForSingleObject` | no | 14 | 0 | — | — |
| `0x0042f8a0` | `DeleteCriticalSection` | no | 12 | 0 | — | — |
| `0x0042f8b0` | `EnterCriticalSection` | no | 12 | 0 | — | — |
| `0x0042f8c0` | `LeaveCriticalSection` | no | 12 | 0 | — | — |
| `0x0042f8d0` | `MultiByteToWideChar` | no | 16 | 0 | — | — |
| `0x0042f8e0` | `WideCharToMultiByte` | no | 17 | 0 | — | — |
| `0x0042f8f0` | `LoadLibraryA` | no | 14 | 0 | — | — |
| `0x0042f900` | `FreeLibrary` | no | 14 | 0 | — | — |
| `0x0042f910` | `SetEvent` | no | 14 | 0 | — | — |
| `0x0042f920` | `ResetEvent` | no | 14 | 0 | — | — |
| `0x0042f930` | `QueryPerformanceCounter` | no | 14 | 0 | — | — |
| `0x0042f940` | `QueryPerformanceFrequency` | no | 14 | 0 | — | — |
| `0x0042f950` | `GetTickCount` | no | 14 | 0 | — | — |
| `0x0042f960` | `TlsFree` | no | 14 | 0 | — | — |
| `0x0042f970` | `TlsAlloc` | no | 14 | 0 | — | — |
| `0x0042f980` | `CreateSemaphoreA` | no | 15 | 0 | — | — |
| `0x0042f990` | `ReleaseSemaphore` | no | 14 | 0 | — | — |
| `0x0042f9a0` | `GetModuleFileNameA` | no | 14 | 0 | — | — |
| `0x0042f9b0` | `TlsGetValue` | no | 14 | 0 | — | — |
| `0x0042f9c0` | `TlsSetValue` | no | 14 | 0 | — | — |
| `0x0042f9d0` | `GetConsoleTitleA` | no | 14 | 0 | — | — |
| `0x0042f9e0` | `SetConsoleTitleA` | no | 14 | 0 | — | — |
| `0x0042f9f0` | `SetConsoleCtrlHandler` | no | 14 | 0 | — | — |
| `0x0042fa00` | `PeekConsoleInputA` | no | 15 | 0 | — | — |
| `0x0042fa10` | `ReadConsoleInputA` | no | 15 | 0 | — | — |
| `0x0042fa30` | `GetConsoleScreenBufferInfo` | no | 15 | 0 | — | — |
| `0x0042fa40` | `SetConsoleTextAttribute` | no | 14 | 0 | — | — |
| `0x0042fa50` | `GetLargestConsoleWindowSize` | no | 14 | 0 | — | — |
| `0x0042fa60` | `SetConsoleScreenBufferSize` | no | 14 | 0 | — | — |
| `0x0042fa70` | `SetConsoleWindowInfo` | no | 14 | 0 | — | — |
| `0x0042fa80` | `GetConsoleMode` | no | 14 | 0 | — | — |
| `0x0042fa90` | `SetConsoleMode` | no | 14 | 0 | — | — |
| `0x0042faa0` | `CreateConsoleScreenBuffer` | no | 17 | 0 | — | — |
| `0x0042fab0` | `GetConsoleCursorInfo` | no | 14 | 0 | — | — |
| `0x0042fac0` | `SetConsoleCursorInfo` | no | 14 | 0 | — | — |
| `0x0042fad0` | `SetConsoleActiveScreenBuffer` | no | 14 | 0 | — | — |
| `0x0042fae0` | `GetStdHandle` | no | 14 | 0 | — | — |
| `0x0042faf0` | `ScrollConsoleScreenBufferA` | no | 17 | 0 | — | — |
| `0x0042fb00` | `FillConsoleOutputAttribute` | no | 17 | 0 | — | — |
| `0x0042fb10` | `FillConsoleOutputCharacterA` | no | 17 | 0 | — | — |
| `0x0042fb20` | `WriteConsoleOutputA` | no | 15 | 0 | — | — |
| `0x0042fb30` | `WriteFile` | no | 15 | 0 | — | — |
| `0x0042fb40` | `SetConsoleCursorPosition` | no | 14 | 0 | — | — |
| `0x0042fb50` | `Beep` | no | 14 | 0 | — | — |
| `0x0042fb60` | `LockFile` | no | 16 | 0 | — | — |
| `0x0042fb70` | `UnlockFile` | no | 16 | 0 | — | — |
| `0x0042fb80` | `ScrollConsoleScreenBufferW` | no | 17 | 0 | — | — |
| `0x0042fb90` | `FillConsoleOutputCharacterW` | no | 17 | 0 | — | — |
| `0x0042fba0` | `WriteConsoleOutputW` | no | 15 | 0 | — | — |
| `0x0042fbe0` | `GetDeviceCaps` | no | 14 | 0 | — | — |
| `0x0042fbf0` | `SelectPalette` | no | 14 | 0 | — | — |
| `0x0042fc00` | `RealizePalette` | no | 14 | 0 | — | — |
| `0x0042fc10` | `SetDIBitsToDevice` | no | 15 | 0 | — | — |
| `0x0042fc20` | `CreatePalette` | no | 14 | 0 | — | — |
| `0x0042fc30` | `DeleteObject` | no | 14 | 0 | — | — |
| `0x0042fc40` | `SetPaletteEntries` | no | 14 | 0 | — | — |
| `0x0042fc50` | `ChoosePixelFormat` | no | 14 | 0 | — | — |
| `0x0042fc60` | `SetPixelFormat` | no | 14 | 0 | — | — |
| `0x0042fc70` | `SwapBuffers` | no | 14 | 0 | — | — |
| `0x0042fc80` | `GetClientRect` | no | 14 | 0 | — | — |
| `0x0042fc90` | `ClientToScreen` | no | 14 | 0 | — | — |
| `0x0042fca0` | `ClipCursor` | no | 14 | 0 | — | — |
| `0x0042fcb0` | `MapWindowPoints` | no | 14 | 0 | — | — |
| `0x0042fcc0` | `GetCursorPos` | no | 14 | 0 | — | — |
| `0x0042fcd0` | `PtInRect` | no | 14 | 0 | — | — |
| `0x0042fce0` | `KillTimer` | no | 14 | 0 | — | — |
| `0x0042fcf0` | `PostMessageA` | no | 14 | 0 | — | — |
| `0x0042fd00` | `WindowFromPoint` | no | 14 | 0 | — | — |
| `0x0042fd10` | `SetTimer` | no | 14 | 0 | — | — |
| `0x0042fd20` | `TranslateMessage` | no | 14 | 0 | — | — |
| `0x0042fd30` | `DispatchMessageA` | no | 14 | 0 | — | — |
| `0x0042fd40` | `PeekMessageA` | no | 14 | 0 | — | — |
| `0x0042fd50` | `CreateWindowExA` | no | 17 | 0 | — | — |
| `0x0042fd60` | `SetForegroundWindow` | no | 14 | 0 | — | — |
| `0x0042fd70` | `SetWindowPos` | no | 14 | 0 | — | — |
| `0x0042fd80` | `RegisterWindowMessageA` | no | 14 | 0 | — | — |
| `0x0042fd90` | `SystemParametersInfoA` | no | 14 | 0 | — | — |
| `0x0042fda0` | `LoadCursorA` | no | 14 | 0 | — | — |
| `0x0042fdb0` | `LoadIconA` | no | 14 | 0 | — | — |
| `0x0042fdc0` | `RegisterClassA` | no | 14 | 0 | — | — |
| `0x0042fdd0` | `UnregisterClassA` | no | 14 | 0 | — | — |
| `0x0042fde0` | `SetCursorPos` | no | 14 | 0 | — | — |
| `0x0042fdf0` | `SetWindowTextA` | no | 14 | 0 | — | — |
| `0x0042fe00` | `GetWindowRect` | no | 14 | 0 | — | — |
| `0x0042fe10` | `GetDC` | no | 14 | 0 | — | — |
| `0x0042fe20` | `ReleaseDC` | no | 14 | 0 | — | — |
| `0x0042fe30` | `GetKeyboardState` | no | 14 | 0 | — | — |
| `0x0042fe40` | `MapVirtualKeyA` | no | 14 | 0 | — | — |
| `0x0042fe50` | `SetCapture` | no | 14 | 0 | — | — |
| `0x0042fe60` | `DefWindowProcA` | no | 14 | 0 | — | — |
| `0x0042fe70` | `ToAscii` | no | 14 | 0 | — | — |
| `0x0042fe80` | `BeginPaint` | no | 14 | 0 | — | — |
| `0x0042fe90` | `EndPaint` | no | 14 | 0 | — | — |
| `0x0042fea0` | `SetCursor` | no | 14 | 0 | — | — |
| `0x0042feb0` | `GetCapture` | no | 14 | 0 | — | — |
| `0x0042fec0` | `ReleaseCapture` | no | 14 | 0 | — | — |
| `0x0042fed0` | `AdjustWindowRect` | no | 14 | 0 | — | — |
| `0x0042fee0` | `GetSystemMetrics` | no | 14 | 0 | — | — |
| `0x0042fef0` | `DestroyWindow` | no | 14 | 0 | — | — |
| `0x0042ff00` | `InvalidateRect` | no | 14 | 0 | — | — |
| `0x0042ff10` | `ChangeDisplaySettingsA` | no | 14 | 0 | — | — |
| `0x0042ff20` | `EnumDisplaySettingsA` | no | 14 | 0 | — | — |
| `0x0042ff30` | `GetWindowLongA` | no | 14 | 0 | — | — |
| `0x0042ff40` | `SetWindowLongA` | no | 14 | 0 | — | — |
| `0x0042ff50` | `ShowWindow` | no | 14 | 0 | — | — |
| `0x0042ff60` | `GetForegroundWindow` | no | 14 | 0 | — | — |
| `0x0042ff70` | `FindWindowA` | no | 14 | 0 | — | — |
| `0x0042ff80` | `GetAsyncKeyState` | no | 14 | 0 | — | — |
| `0x0042ff90` | `VkKeyScanA` | no | 14 | 0 | — | — |
| `0x0042ffa0` | `fsetpos` | no | 14 | 0 | — | — |
| `0x0042ffb0` | `_filelengthi64` | no | 14 | 0 | — | — |
| `0x0042ffc0` | `FUN_0042ffc0` | no | 9 | 0 | — | FUN_0042ffc0, FUN_004012c0 |

## Stima aggiornata per chiudere un port fedele

Una partita solo giocabile non equivale a una conversione fedele: i due blocchi di storia mancanti sono invocati nel flusso principale. Stima di lavoro per un singolo sviluppatore C esperto, assunto accesso a una macchina con build SDL/Wine/Ghidra e media originali:

| Fase | Stima |
|---|---:|
| Ricostruire le scene mandatory `00410f00`, `004087b0` e relative voci | 4–7 giorni |
| Allineare helper/access-denied, TTT e risposte alternative | 2–4 giorni |
| Build, replay completo, confronti frame/audio e correzioni | 4–7 giorni |
| **Totale indicativo per buona parità** | **10–18 giorni lavorativi** |

Per un percorso giocabile senza fedeltà 1:1, il codice attuale è vicino, ma le scene obbligatorie mancanti richiedono comunque circa **4–7 giorni**. La stima ha confidenza media-bassa finché non si compila e si percorre l’intera storia contro Wine.

## Riferimenti

- Ghidra: <https://github.com/NationalSecurityAgency/ghidra> — decompilatore/disassemblatore già usato per generare l’export locale.
- RetDec: <https://github.com/avast/retdec> — supporta PE e x86, ma è in manutenzione limitata secondo il repository ufficiale.
- Export usati: `../re/decomp_ghidra_12.2.c`, `../re/string_xrefs_ghidra_12.2.csv`; export Ghidra 12.1.4 conservati come `../re/decomp.c` e `../re/string_xrefs.csv`.
- Ricostruzione: `src/main.c`, `src/media.c`; prove: `evidence/`.
