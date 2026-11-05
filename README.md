# WarGames (1983 / 1995 / 2026) - Porting Nativo in C

Porting e ricostruzione nativa in linguaggio C del videogioco per PC ispirato al celebre film cult **WarGames** (1983).

Originariamente programmato in FreeBASIC da **Franco Togliatto** (1995/2008), il progetto è stato completamente reingegnerizzato, riscritto in C nativo (C11 + SDL2) ed esteso da **Stefano Basile** per **8Byte Software** (2026).

---

## ⚡ Come Giocare Subito (Senza compilare)

Tutti gli asset di gioco e i binari precompilati sono inclusi nel repository:

- **Su Linux (64-bit):**
  Basta avviare direttamente l'eseguibile:
  ```bash
  ./wargame-linux
  ```
- **Su Windows (64-bit):**
  Basta fare doppio clic su:
  ```text
  wargame.exe
  ```
  *(Tutte le librerie SDL2 e le dipendenze DLL sono già fornite nella cartella).*
- **Su macOS:**
  I binari universali (Apple Silicon M1/M2/M3/M4 + Intel) vengono compilati e rilasciati automaticamente tramite **GitHub Actions** nella scheda *Releases* del repository.

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

## 🎮 Modalità di Gioco

All'avvio, il menu principale consente di selezionare la lingua e la modalità desiderata:

```text
1. ITALIANO (CLASSICO)
2. ITALIANO (VERSIONE ESTESA)
3. ENGLISH  (CLASSIC)
4. ENGLISH  (EXTENDED)
```

- **Modalità Classica**: Ricostruzione fedele dell'esperienza originale del videogioco e della narrazione cult del film.
- **Versione Estesa**: Nuove possibilità di interazione, sistemi sbloccati ed elementi inediti tutti da scoprire esplorando e giocando.

---

## 📜 Licenza e Crediti

- Progetto originale (1995/2008): **Franco Togliatto** (FreeBASIC).
- Porting in C nativo ed estensioni (2026): **Stefano Basile** per **8Byte Software**.
- Tutti i marchi, loghi e riferimenti a *WarGames* (1983) appartengono ai rispettivi titolari (MGM / United Artists).
