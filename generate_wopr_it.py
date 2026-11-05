#!/usr/bin/env python3
import os
import shutil
import subprocess

TARGET_DIR = "/home/figaro/sorgenti/wargame/Wopr_it"
ORIGINAL_DIR = "/home/figaro/sorgenti/wargame/Wopr"
TMP_WAV = "/tmp/wopr_temp.wav"

os.makedirs(TARGET_DIR, exist_ok=True)

phrases = {
    "Warhello": "Saluti Professor Falken.",
    "Warsfm": "Come sta oggi?",
    "Warytd": "Sì, lo fanno.",
    "Warswpag": "Giochiamo a qualcosa?",
    "Wartwtg": "Per vincere la partita.",
    "Waribalt": "Eccellente, è passato molto tempo. Può spiegare l'eliminazione del suo account il ventitré giugno settantatré?",
    "Wargpf1": "Saluti Professor Falken.",
    "Wargpf2": "Saluti Professor Falken.",
    "Wargpf3": "Saluti Professor Falken.",
    "Warggoc": "Non preferisce una bella partita a scacchi?",
    "Warfine": "Bene.",
    "Warypgme": "Dovrebbe saperlo Professore. Mi ha programmato lei.",
    "Wardnpc": "Non preferisce una bella partita a scacchi?",
    "Warngoc": "Non preferisce una bella partita a scacchi?",
    "Warpkr": "Certo, dovrei raggiungere il DEFCON uno e lanciare i miei missili tra ventotto ore. Vuole vedere alcune previsioni sulle percentuali di vittime?",
    "Warwtd": "Che differenza fa? È un uomo difficile da raggiungere. Non sono riuscito a trovarla a Seattle.",
    "War5tcr": "I file pensionistici del D O D indicano come indirizzo attuale: Dottor Robert Hume, noto come Stephen W. Falken. Cinque Tall Cedar Road. Goose Island, Oregon.",
    "Warasgam": "Una strana partita. L'unica mossa vincente è non giocare.",
    "Warwsdyw": "Da che parte vuoi stare?",
    "Warplpt": "Elencare i bersagli primari per nome di città e o contea.",
    "Warcladd": "Nessun terminale è in funzione al suo indirizzo riservato.",
    "Warsidnpc": "Non preferisce una bella partita a scacchi?",
    "Warsin": "Giochiamo a qualcosa?"
}

# Copy music / non-speech files directly
for music_file in ["Warintro.dat", "Warend.dat"]:
    src = os.path.join(ORIGINAL_DIR, music_file)
    dst = os.path.join(TARGET_DIR, music_file)
    shutil.copyfile(src, dst)
    print(f"Copied soundtrack: {music_file}")

# Synthesize all spoken dialogue lines
for name, text in phrases.items():
    dst = os.path.join(TARGET_DIR, f"{name}.dat")
    # Generate WAV with espeak-ng using Klatt robotic model
    subprocess.run(["espeak-ng", "-v", "it+klatt", "-s", "115", "-p", "32", "-w", TMP_WAV, text], check=True)
    
    # Process with ffmpeg filter to achieve metallic vocoder resonance and vintage frequency response
    filter_chain = (
        "aresample=44100,pan=stereo|c0=c0|c1=c0,"
        "flanger=delay=2:depth=1:regen=40:width=80:speed=0.4,"
        "equalizer=f=350:width_type=h:width=250:g=4,"
        "equalizer=f=3000:width_type=h:width=600:g=2,"
        "highpass=f=120,lowpass=f=4200,volume=1.8"
    )
    subprocess.run([
        "ffmpeg", "-y", "-i", TMP_WAV,
        "-af", filter_chain,
        "-codec:a", "libmp3lame", "-b:a", "128k",
        "-f", "mp3", dst
    ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print(f"Generated Italian voice: {name}.dat -> '{text}'")

if os.path.exists(TMP_WAV):
    os.remove(TMP_WAV)

print("Done! All 25 Wopr_it assets generated successfully.")
