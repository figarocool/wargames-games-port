#!/usr/bin/env python3
"""Build a function-level port audit from the local Ghidra export.

This does not decompile machine code. Ghidra has already done that in
``re/decomp.c``; this script inventories all exported functions, call edges,
string cross-references, and assets, then adds the manually reviewed game-flow
assessment used by ``PORTING_AUDIT.md``.
"""

from __future__ import annotations

import argparse
import csv
import re
from collections import deque
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HEADER = re.compile(r"^/\* (?P<name>.*?) @ (?P<address>[0-9a-fA-F]{8}) \*/\s*$", re.M)
CALL = re.compile(r"\b(FUN_[0-9a-fA-F]{8})\s*\(")
STRING = re.compile(r"\bs_([A-Za-z0-9_]+)_004[0-9a-fA-F]+\b")


@dataclass
class Function:
    name: str
    address: str
    body: str
    calls: list[str]
    strings: list[str]


# These are the game-specific routines, rather than the hundreds of compiler,
# BASIC runtime, graphics, and Win32 support functions in the executable.
# Statuses are manual review judgments, not inferred from matching names.
GAME_REVIEW = {
    "004012d0": ("Scansione carrier e intro telefonica", "Parziale", "Schermata e scan implementati; coda casuale completa e timing non confrontati.", "`start_intro`; stage `PHONE`/`SCANNING`; input in `main`; scan in `render` e loop principale."),
    "00402490": ("Lista carrier e chiamata", "Parziale", "Navigazione/rami presenti; timing della banca e tutti i numeri non verificati.", "Stage `DIAL_NUMBERS`, eventi Su/Giù/Invio in `main`, transizioni verso banca e carrier."),
    "00403230": ("Prenotazione Pan Am", "Verificato in parte", "Quattro schermate statiche identiche per il percorso Paris/Chicago/nome provato.", "Stage `DIAL_MENU`, `DIAL_DESTINATION`, `DIAL_DEPARTURE`, `DIAL_PASSENGER`, `DIAL_TICKET`; `submit` e `render`."),
    "00403b20": ("Schermata banca", "Parziale", "Rendering/typewriter presenti; confronto temporale completo non documentato.", "Stage `BANK_PROMPT`; `update_bank_typewriter_audio`; ramo banca nel loop principale."),
    "00403ce0": ("Prompt LOGON iniziale e comandi", "Parziale", "Joshua/Help/Games/List Games/Franco implementati; layout dei comandi non confrontato con Wine.", "Stage `LOGON`/`LOGON_REJECTED`; `stage_prompt`, `submit`, `render`."),
    "004047f0": ("Dialogo iniziale con Joshua", "Parziale", "Sette checkpoint stabili identici; alternative e sincronismo audio incompleti.", "Stage `GREETING`, `HOW_ARE_YOU`, `MISTAKES`, `PLAY`, `CHESS_REFUSAL`, `LATER`; `submit`."),
    "00405190": ("Mappa iniziale e sequenza di simulazione", "Avanzato, non completo", "Checkpoint di mappa, battaglia e tattica confrontati; intero audio/percorso non validato.", "Stage `SIMULATION_OVERVIEW`, `SIDE`, `CITIES`, `SIMULATION_BATTLE`, `SIMULATION_TACTICAL`, `SIMULATION_MONTAGE`."),
    "00407e60": ("Post-simulazione, cartelli e modem", "Parziale", "Cartelli e ingresso modem presenti; confronto integrale di sequenze e audio assente.", "`CaptionProgram` post-simulazione/post-hack; `STORY_CAPTION`, `MODEM_CONNECT`, `GREETING3`."),
    "004087b0": ("Capitolo Falken/rapporti NORAD", "Incompleto", "Il port salta rapporti di distruzione, domanda sulla differenza, indirizzo di Falken e le relative voci.", "Solo `IDENTIFICATION`, `PRIMARY_GOAL`, `WIN_STATEMENT`, `LOGON_AGAIN`, `STILL_PLAYING`, `REAL_OR_GAME`; mancano scene e voci intermedie."),
    "004098f0": ("Sequenza cartelli NORAD 12–32", "Avanzato, non completo", "Asset/sequenza presenti; non risultano verificati tutti i tempi, input e audio.", "`CaptionProgram` `CAPTIONS_NORAD_*`; `start_caption_sequence` e `advance_story_caption`."),
    "0040a8c0": ("Secondo LOGON e catalogo giochi", "Parziale", "Catalogo e selezioni principali presenti; ogni risposta/ramo e gli effetti non sono confrontati.", "Stage `LOGON_FINAL`, `LIST_GAMES`, `GAME_SHELL`; selezione delle clip in `submit`."),
    "0040b590": ("Tic Tac Toe", "Incompleto", "È riprodotta la partita prescritta; non è dimostrata la logica generale a uno/due giocatori.", "Stage `TIC_TAC_TOE`, `TIC_MARK`, `TIC_MOVE`, `TIC_ANIMATION`, `TIC_AGAIN`; `advance_tic_animation`."),
    "0040dd10": ("Saluto e finale", "Parziale", "Testi/tempi implementati; confronto pixel e temporale con l'originale non documentato.", "Stage `FINAL_GREETING`, `ENDING_PRELUDE`, `ENDING`, `ENDING_CHESS`, `ENDING_MUSIC`."),
    "0040e170": ("Intro", "Parziale", "Intro implementata; sequenza completa e riproduzione media non confrontate.", "Stage `INTRO`; `start_intro`, `render`, timer nel loop principale."),
    "0040e470": ("Helper originale per cartelli/animazioni", "Copertura parziale", "Meccanismi ricreati tramite le sequenze di caption C; equivalenza dei vari mode non provata.", "`load_story_caption`, `start_caption_sequence`, `advance_story_caption`, `render`."),
    "0040ee60": ("Risposte di accesso negato / gioco bloccato", "Incompleto", "Il binario ha schermate e rami distinti; il port non ne dimostra la parità, soprattutto dal LOGON finale.", "Nessun equivalente dedicato; alcuni ingressi LOGON sono simulati da clip e transizioni."),
    "0040fb30": ("Pannello GAME TIME / ESTIMATED TIME", "Mancante", "Il pannello con i tempi della guerra, chiamato dal capitolo Falken, non è implementato come schermata dedicata.", "Nessun stage o helper C corrispondente."),
    "00410f00": ("Montaggio di schermate stato-rete dopo il primo LOGON", "Implementato; confronto puntuale da completare", "Le otto fasi e gli intervalli sono ricostruiti; i fotogrammi brevi richiedono confronto via registrazione Wine.", "Stage `NETWORK_STATUS`; `start_network_status`, `start_network_page`, `advance_network_status`, `render_network_status`; `WARGAME_COMPARE_NETWORK_HOLD` isola ogni pagina."),
}


def load_functions(path: Path) -> list[Function]:
    text = path.read_text(encoding="utf-8", errors="replace")
    headers = list(HEADER.finditer(text))
    result = []
    for index, match in enumerate(headers):
        end = headers[index + 1].start() if index + 1 < len(headers) else len(text)
        body = text[match.end():end]
        result.append(Function(
            name=match.group("name").strip(),
            address=match.group("address").lower(),
            body=body,
            calls=list(dict.fromkeys(CALL.findall(body))),
            strings=list(dict.fromkeys(STRING.findall(body))),
        ))
    return result


def load_xrefs(path: Path) -> tuple[dict[str, list[str]], int]:
    result: dict[str, list[str]] = {}
    with path.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    for row in rows:
        function = row.get("function", "").strip()
        value = row.get("string", "").strip()
        if function and value:
            values = result.setdefault(function, [])
            if value not in values:
                values.append(value)
    return result, len(rows)


def reachable_functions(functions: list[Function], root_name: str) -> set[str]:
    by_name = {function.name: function for function in functions}
    visited: set[str] = set()
    pending = deque([root_name])
    while pending:
        name = pending.popleft()
        if name in visited or name not in by_name:
            continue
        visited.add(name)
        pending.extend(by_name[name].calls)
    return visited


def cell(value: str) -> str:
    return value.replace("|", "\\|").replace("\n", " ").replace("\r", " ")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--decomp", type=Path, default=ROOT / "re" / "decomp_ghidra_12.2.c")
    parser.add_argument("--xrefs", type=Path, default=ROOT / "re" / "string_xrefs_ghidra_12.2.csv")
    parser.add_argument("--assets", type=Path, default=ROOT / "re" / "asset_inventory.csv")
    parser.add_argument("--port-source", type=Path, default=ROOT / "rebuild" / "src")
    parser.add_argument("--output", type=Path, default=ROOT / "rebuild" / "PORTING_AUDIT.md")
    args = parser.parse_args()

    for path in (args.decomp, args.xrefs, args.assets):
        if not path.is_file():
            parser.error(f"artefatto Ghidra richiesto non trovato: {path}")

    functions = load_functions(args.decomp)
    xrefs, xref_rows = load_xrefs(args.xrefs)
    source_text = "\n".join(path.read_text(encoding="utf-8", errors="replace")
                             for path in sorted(args.port_source.glob("*.c")))
    reachable = reachable_functions(functions, "FUN_004122f0")
    function_by_address = {function.address: function for function in functions}
    known_function_names = {function.name for function in functions}

    with args.assets.open(newline="", encoding="utf-8") as asset_file:
        asset_rows = list(csv.DictReader(asset_file))
    unreferenced = [row["path"] for row in asset_rows
                    if Path(row["path"]).stem.lower() not in source_text.lower()]

    lines = [
        "# Audit del porting Wargames C",
        "",
        "> Generato da `tools/audit_port.py` il 2026-10-31 usando l’export fresco Ghidra 12.2. ",
        "> Non è un decompilatore: la traduzione delle istruzioni è stata fatta da Ghidra; ",
        "> ruoli e stato del port sono revisione manuale e vanno verificati a runtime.",
        "",
        "## Metodo e completezza dell’export",
        "",
        f"- Funzioni esportate: **{len(functions)}**; righe export: **{sum(1 for _ in args.decomp.open(encoding='utf-8', errors='replace'))}**.",
        "- Un export preesistente di Ghidra 12.1.4 riportava 596 funzioni; la nuova analisi 12.2 ne identifica 590. Le sei funzioni di differenza dipendono dall’analisi/versione e gli export precedenti sono stati conservati.",
        f"- Funzioni raggiungibili dal main Ghidra `FUN_004122f0` tramite chiamate dirette nell’export: **{len(reachable)}**.",
        f"- Funzioni con stringhe in `string_xrefs.csv`: **{len(xrefs)}**; record CSV: **{xref_rows}**.",
        f"- Funzioni game-specific selezionate per revisione manuale: **{len(GAME_REVIEW)}**.",
        "- L’export Ghidra è best-effort, non sorgente originale e non è compilabile come porting. La raggiungibilità è approssimata: puntatori a funzione e chiamate indirette non sono risolti da questo script.",
        "- RetDec è un’alternativa PE/x86 open source, ma il progetto dichiara manutenzione limitata e richiede un’installazione di diversi GB. Qui non aggiunge valore rispetto all’export Ghidra esistente, già completo a livello di funzione.",
        "- Rigenerazione dal repository con gli export locali presenti: `python3 rebuild/tools/audit_port.py`. I file `re/` sono esclusi da Git, quindi servono il progetto Ghidra e gli export nella macchina di analisi.",
        "",
        "## Flusso originale: funzioni di gioco",
        "",
        "| Indirizzo Ghidra | Routine originale | Stato port | Evidenza / gap | Mapping semantico nel C |",
        "|---|---|---|---|---|",
    ]
    for address, (role, status, note, c_mapping) in GAME_REVIEW.items():
        function = function_by_address.get(address)
        name = function.name if function else "export assente"
        lines.append(f"| `0x{address}` `{cell(name)}` | {cell(role)} | **{status}** | {cell(note)} | {cell(c_mapping)} |")

    lines.extend([
        "",
        "## Gap confermati dal confronto statico",
        "",
        "Il mapping sopra è intenzionalmente molti-a-molti: i confini delle funzioni FreeBASIC originali non coincidono con gli stage/helper della riscrittura SDL. Per questo la verifica automatica misura presenza, raggiungibilità e stringhe, mentre l’equivalenza si valuta con gli scenari comportamentali.",
        "",
        "1. **Schermate stato-rete (`FUN_00410f00`).** La sequenza è ora implementata dopo il primo `Joshua`: otto pagine temporizzate e voce typewriter. Restano da confrontare ogni pagina e i relativi intervalli con una registrazione Wine; la timeline `scenarios/first-logon-network.json` verifica il percorso stabile fino al dialogo.",
        "2. **Capitolo Falken/NORAD (`FUN_004087b0`).** Il binario include la proiezione dei kill ratio, `WHAT'S THE DIFFERENCE?`, l’indirizzo di Falken e voci `Warsin`, `Warwtd`, `Warcladd`, `War5tcr`. Il ramo C `REAL_OR_GAME` salta direttamente alla coda di cartelli FBI (`main.c`, circa righe 1453–1458).",
        "3. **Pannello tempi guerra (`FUN_0040fb30`).** Il binario visualizza `GAME TIME ELAPSED / ESTIMATED TIME REMAINING`; il port non ha una schermata equivalente nel flusso sopra menzionato.",
        "4. **Risposte bloccate (`FUN_0040ee60`).** L’originale ha varianti con `CHANGES LOCKED OUT`, `ACCESS DENIED`, `IDENTIFICATION NOT RECOGNIZED`, `GAME ROUTINE RUNNING` e attese/audio dedicati. La semplice riproduzione di clip nel LOGON finale non prova equivalenza di questi rami.",
        "5. **Tic Tac Toe.** Il C riproduce la sequenza dimostrativa `5,3,8,6,1`; nel sorgente lo stato è sostanzialmente scriptato e non è dimostrata la partita generale a uno/due giocatori, con input alternativi e relativi esiti.",
        "6. **Media e copertura temporale.** Mancano il confronto di una partita completa, tutte le diramazioni, tutte le animazioni e il sincronismo audio/video. Il player C scandisce i frame a 25 fps e accoda l’audio in modo indipendente; la parità globale non è dimostrata.",
        "",
        "## Evidenze già disponibili",
        "",
        "- Pan Am: quattro frame statici identici pixel per pixel per la sequenza provata.",
        "- Dialogo iniziale: sette checkpoint stabili identici; il checkpoint in animazione resta da riallineare.",
        "- Mappa iniziale: sette checkpoint identici, inclusi input errati e duplicati.",
        "- Battaglia: dieci immagini identiche; tattica: tredici checkpoint identici e scarto temporale massimo salvato di un frame a 30 fps.",
        "- Coda FBI: sei checkpoint identici.",
        "- Queste prove sono report e catture preesistenti. Questo ambiente non ha `make`, compilatore C, Wine o Ghidra eseguibile, quindi non sono state ricompilate/rieseguite qui.",
        "",
        f"## Inventario asset non referenziati dal C ({len(unreferenced)})",
        "",
        "La verifica è per nome base nel sorgente e può sovrastimare i mancanti quando l’originale costruisce nomi dinamicamente. I file audio con riferimenti certi nel decompilato sono evidenziati.",
        "",
    ])
    for asset in unreferenced:
        marker = " — voce referenziata dal decompilato" if Path(asset).stem in {
            "Warsin", "Warwtd", "Warcladd", "War5tcr"
        } else ""
        lines.append(f"- `{asset}`{marker}")

    lines.extend([
        "",
        "## Inventario di tutte le funzioni esportate",
        "",
        "Il prospetto seguente copre ogni header Ghidra. `Reachable` usa il grafo delle chiamate dirette ricavato dall’output C; `xref` è il numero di riferimenti stringa noti; `dimensione` è il numero di righe nel blocco export.",
        "",
        "| Indirizzo | Nome export | Reachable | Righe | Xref | Stringhe note | Chiamate interne (prime 8) |",
        "|---|---|---:|---:|---:|---|---|",
    ])
    for function in functions:
        name = function.name
        strings = xrefs.get(name, function.strings)
        call_names = [call for call in function.calls if call in known_function_names]
        body_lines = len(function.body.splitlines())
        lines.append(
            f"| `0x{function.address}` | `{cell(name)}` | "
            f"{'sì' if name in reachable else 'no'} | {body_lines} | {len(strings)} | "
            f"{cell('; '.join(strings[:5]) if strings else '—')} | "
            f"{cell(', '.join(call_names[:8]) if call_names else '—')} |"
        )

    lines.extend([
        "",
        "## Stima aggiornata per chiudere un port fedele",
        "",
        "Una partita solo giocabile non equivale a una conversione fedele: i due blocchi di storia mancanti sono invocati nel flusso principale. Stima di lavoro per un singolo sviluppatore C esperto, assunto accesso a una macchina con build SDL/Wine/Ghidra e media originali:",
        "",
        "| Fase | Stima |",
        "|---|---:|",
        "| Ricostruire le scene mandatory `00410f00`, `004087b0` e relative voci | 4–7 giorni |",
        "| Allineare helper/access-denied, TTT e risposte alternative | 2–4 giorni |",
        "| Build, replay completo, confronti frame/audio e correzioni | 4–7 giorni |",
        "| **Totale indicativo per buona parità** | **10–18 giorni lavorativi** |",
        "",
        "Per un percorso giocabile senza fedeltà 1:1, il codice attuale è vicino, ma le scene obbligatorie mancanti richiedono comunque circa **4–7 giorni**. La stima ha confidenza media-bassa finché non si compila e si percorre l’intera storia contro Wine.",
        "",
        "## Riferimenti",
        "",
        "- Ghidra: <https://github.com/NationalSecurityAgency/ghidra> — decompilatore/disassemblatore già usato per generare l’export locale.",
        "- RetDec: <https://github.com/avast/retdec> — supporta PE e x86, ma è in manutenzione limitata secondo il repository ufficiale.",
        "- Export usati: `../re/decomp_ghidra_12.2.c`, `../re/string_xrefs_ghidra_12.2.csv`; export Ghidra 12.1.4 conservati come `../re/decomp.c` e `../re/string_xrefs.csv`.",
        "- Ricostruzione: `src/main.c`, `src/media.c`; prove: `evidence/`.",
        "",
    ])

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="utf-8")
    print(f"Analizzate {len(functions)} funzioni; {len(reachable)} raggiungibili dalla main; report: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
