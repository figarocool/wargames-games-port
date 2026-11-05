# WarGames (1983 / 1995 / 2026) - Porting Nativo in C

Porting e ricostruzione nativa in linguaggio C del videogioco per PC ispirato al celebre film cult **WarGames** (1983).

Originariamente programmato in FreeBASIC da **Franco Togliatto** (1995/2008), il progetto è stato completamente reingegnerizzato, riscritto in C nativo (C11 + SDL2) ed esteso da **Stefano Basile** ([stefanobasile@usa.com](mailto:stefanobasile@usa.com)) per **8Byte Software** (2026).

---

## 🚀 Requisiti di Sistema e Installazione

Il gioco è compatibile con qualsiasi distribuzione Linux moderna (e compilabile su qualsiasi sistema POSIX provvisto di supporto SDL2 e X11).

### Dipendenze richieste

#### Debian / Ubuntu / Linux Mint / Pop!_OS:
```bash
sudo apt update
sudo apt install build-essential pkg-config libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev libx11-dev fonts-pc ffmpeg
```

#### Fedora / RHEL:
```bash
sudo dnf install gcc make SDL2-devel SDL2_ttf-devel SDL2_mixer-devel libX11-devel ffmpeg
```

#### Arch Linux / Manjaro:
```bash
sudo pacman -S base-devel sdl2 sdl2_ttf sdl2_mixer libx11 ffmpeg
```

---

## 🛠️ Come Compilare e Avviare

1. **Clona il repository:**
   ```bash
   git clone https://github.com/figarocool/wargames-games-port.git
   cd wargames-games-port
   ```

2. **Compila il gioco:**
   ```bash
   make -C rebuild
   ```
   *(oppure entra nella cartella `cd rebuild` e digita `make`)*.

3. **Avvia il gioco:**
   ```bash
   ./rebuild/wargame-c
   ```

All'avvio comparirà il selettore iniziale per scegliere la lingua e la modalità desiderata.

---

## 🎮 Differenze tra le Versioni

All'avvio, il menu principale consente di selezionare 4 modalità:

```text
1. ITALIANO (CLASSICO)
2. ITALIANO (VERSIONE ESTESA)
3. ENGLISH  (CLASSIC)
4. ENGLISH  (EXTENDED)
```

### 1. Modalità Classica (Italiano / English)
È la ricostruzione fedele 1:1 dell'esperienza originale di Franco Togliatto e degli eventi narrativi del film:
- **Wardialing IMSAI (311):** ricerca sequenziale dei toni vettore componendo i prefissi `399`, `437`, `767`, `936`.
- **Pan American Airlines:** prenotazione dei voli per Parigi per eludere l'FBI.
- **Logon W.O.P.R.:** accesso al supercomputer militare NORAD tramite la celebre backdoor `Joshua`.
- **Interrogatorio e Falken:** conversazione intelligente su Stephen Falken, la morte del figlio e l'eliminazione dell'account.
- **Simulazione e Mappa Strategica:** tracciamento in tempo reale delle traiettorie dei missili ICBM e calcolo delle perdite stimate.
- **Confronto Finale a Tris:** partita a Tic-Tac-Toe con la rivelazione che l'unica mossa vincente è non giocare.
- **Localizzazione completa in Italiano:** didascalie grafiche trasparenti (`Wartxt1.dat`, `Wartxt6.dat`), dialoghi, terminale e tracce audio tradotte.

### 2. Versione Estesa (Italiano / English)
Oltre a tutta la storia originale, la **Versione Estesa** sblocca e implementa tutti i sistemi e giochi che nell'originale erano inaccessibili o simulati come semplice testo:
- **Hacking Bancario Union Pacific (311-399-2364):** terminale interattivo di sicurezza bancaria con comandi per consultare saldi, forzare password e trasferire fondi.
- **Pan Am GDS Esteso:** sistema completo di prenotazione con consultazione libera degli orari di volo, ricerca passeggeri e cambio itinerari.
- **Tutti i Giochi della Lista di Joshua Giocabili:**
  - ♟️ **Scacchi (Chess):** scacchiera ASCII interattiva completa con motore di gioco e IA avversaria.
  - 🌀 **Falken's Maze (Labirinto):** labirinto procedurale bidimensionale navigabile in tempo reale con frecce direzionali / WASD.
  - ♠️ **Blackjack (Ventuno):** tavolo da casinò giocabile con gestione delle fiches, puntate e banco.
  - 🃏 **Poker:** partita completa a 5-card draw con cambio carte, scommesse e rilanci contro W.O.P.R.
  - ❌⭕ **Tris (Tic-Tac-Toe)** e **Guerra Termonucleare Globale**.

---

## 🕵️ Easter Eggs & Crediti Nascosti

Al prompt di accesso (`LOGON:`), oltre alla password di trama `Joshua`:
- Digitando **`stefano`** (o **`stefano basile`**): visualizza i crediti del porting in C e l'indirizzo email dell'autore:
  ```text
  **STEFANO BASILE**
  PORTING E REWRITING DAL FREEBASIC DI FRANCO TOGLIATTO IN C NATIVO
  EMAIL: STEFANOBASILE@USA.COM
  ```
- Digitando **`franco`**: visualizza l'omaggio all'autore storico originale Franco Togliatto.
- Il banner scorrevole inferiore riporta la dicitura:
  `COPYRIGHT (C) 2026 8BYTE SOFTWARE...`

---

## 📜 Licenza e Crediti

- Progetto originale (1995/2008): **Franco Togliatto** (FreeBASIC).
- Porting in C nativo, estensioni e localizzazione (2026): **Stefano Basile** per **8Byte Software**.
- Tutti i marchi, loghi e riferimenti a *WarGames* (1983) appartengono ai rispettivi titolari (MGM / United Artists).
