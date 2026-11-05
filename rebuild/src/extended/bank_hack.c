#include "extended.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_BANK_LINES 22
#define LINE_CAP 96

typedef enum {
    BANK_LOGIN,
    BANK_MAIN,
    BANK_DISCONNECTED
} BankState;

typedef struct {
    char id[16];
    char name[48];
    double balance;
} BankAccount;

static BankState b_state;
static int b_logged_in;
static uint32_t b_start_time;
static uint32_t b_last_trace_tick;
static int b_trace_percent;
static int b_bypass_count;
static char b_history[MAX_BANK_LINES][LINE_CAP];
static int b_history_count;

static BankAccount b_accounts[] = {
    {"UP-8841", "UNION PACIFIC CORP - PAYROLL", 4850200.00},
    {"ND-0012", "NORAD DEFENSE LOGISTICS", 12400000.00},
    {"PA-3921", "PAN AMERICAN WORLD AIRWAYS", 1290450.00},
    {"DL-1983", "DAVID LIGHTMAN - SAVINGS", 14.25}
};

static void bank_push(const char *fmt, ...) {
    if (b_history_count >= MAX_BANK_LINES) {
        for (int i = 0; i < MAX_BANK_LINES - 1; ++i)
            memcpy(b_history[i], b_history[i + 1], sizeof(b_history[i]));
        b_history_count = MAX_BANK_LINES - 1;
    }
    char buf[LINE_CAP];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    snprintf(b_history[b_history_count++], sizeof(b_history[0]), "%s", buf);
}

void ext_bank_init(App *a) {
    (void)a;
    b_state = BANK_LOGIN;
    b_logged_in = 0;
    b_start_time = SDL_GetTicks();
    b_last_trace_tick = b_start_time;
    b_trace_percent = 5;
    b_bypass_count = 2;
    b_history_count = 0;

    int is_it = app_get_lang(a);
    bank_push("TEG SYSTEM A-15 24:34:33                    Y- 1293.323");
    bank_push("Union Pacific Bank - SouthWest Regional Data Control");
    bank_push("------------------------------------------------------------------");
    if (is_it) {
        bank_push("TERMINALE REMOTO ATTIVO. INSERIRE CREDENZIALI DI ACCESSO.");
        bank_push("DIGITA 'AIUTO' PER LA GUIDA ALLE BACKDOOR DI SISTEMA.");
        bank_push("LOGON: ");
    } else {
        bank_push("REMOTE TERMINAL ACTIVE. ENTER SECURITY CREDENTIALS.");
        bank_push("TYPE 'HELP' FOR SYSTEM BACKDOOR HINTS.");
        bank_push("LOGON: ");
    }
}

void ext_bank_update(App *a) {
    if (b_state != BANK_MAIN) return;
    uint32_t now = SDL_GetTicks();
    if (now - b_last_trace_tick >= 1400u) {
        b_last_trace_tick = now;
        b_trace_percent += 2;
        if (b_trace_percent >= 80 && b_trace_percent < 84) {
            app_play_sound(a, "Waracdnd");
        }
        if (b_trace_percent >= 100) {
            b_trace_percent = 100;
            b_state = BANK_DISCONNECTED;
            app_play_sound(a, "Waracdnd");
            int is_it = app_get_lang(a);
            bank_push("");
            bank_push(is_it ? "*** ALLARME! TRACCIAMENTO TELEFONICO COMPLETATO! ***" :
                              "*** ALARM! CARRIER TRACE COMPLETED! ***");
            bank_push(is_it ? "LOCALIZZAZIONE: SEATTLE, WASHINGTON (206) 555-1983" :
                              "LOCATION: SEATTLE, WASHINGTON (206) 555-1983");
            bank_push(is_it ? "DISCONNESSIONE FORZATA DI EMERGENZA..." :
                              "EMERGENCY DISCONNECT BYPASS...");
        }
    }
}

static void lower_str(char *dst, size_t cap, const char *src) {
    size_t i = 0;
    for (; src[i] && i + 1 < cap; ++i) dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = 0;
}

void ext_bank_submit(App *a, const char *input) {
    int is_it = app_get_lang(a);
    char cmd[LINE_CAP];
    lower_str(cmd, sizeof(cmd), input);

    if (b_state == BANK_DISCONNECTED) {
        /* Any key exits back to carrier select */
        app_start_stage(a, 6 /* DIAL_NUMBERS */, "");
        return;
    }

    if (b_state == BANK_LOGIN) {
        bank_push("LOGON: %s", input);
        if (strstr(cmd, "help") || strstr(cmd, "aiuto")) {
            if (is_it) {
                bank_push(">> HINT: LE CREDENZIALI DI MANUTENZIONE PREDEFINITE");
                bank_push(">> SONO SPESSO 'TEG', 'ADMIN', 'OVERRIDE' O 'DAVID'.");
            } else {
                bank_push(">> HINT: DEFAULT MAINTENANCE CREDENTIALS ARE");
                bank_push(">> COMMONLY 'TEG', 'ADMIN', 'OVERRIDE', OR 'DAVID'.");
            }
            return;
        }
        if (strstr(cmd, "teg") || strstr(cmd, "admin") || strstr(cmd, "override") ||
            strstr(cmd, "david") || strstr(cmd, "falken") || strstr(cmd, "joshua")) {
            b_logged_in = 1;
            b_state = BANK_MAIN;
            app_play_sound(a, "Wargtr");
            bank_push("");
            if (is_it) {
                bank_push(">> ACCESSO AUTORIZZATO! LIVELLO DI SICUREZZA 4 (SUPERVISORE)");
                bank_push(">> ATTENZIONE: SISTEMA DI TRACCIAMENTO TELEFONICO ATTIVATO!");
                bank_push("COMANDI: CONTI | SALDO <ID> | BONIFICO <DA> <SOMMA> <A> | BYPASS | ESCI");
            } else {
                bank_push(">> ACCESS GRANTED! SECURITY LEVEL 4 (SUPERVISOR)");
                bank_push(">> WARNING: TELECOM TRACER ACTIVE! MONITOR TRACE LEVEL!");
                bank_push("COMMANDS: ACCOUNTS | BALANCE <ID> | TRANSFER <FROM> <AMT> <TO> | BYPASS | LOGOUT");
            }
        } else {
            app_play_sound(a, "Waracdnd");
            bank_push(is_it ? "ACCESSO NEGATO: CREDENZIALI NON VALIDE." : "ACCESS DENIED: INVALID CREDENTIALS.");
        }
        return;
    }

    /* BANK_MAIN state */
    bank_push("> %s", input);

    if (strstr(cmd, "logout") || strstr(cmd, "esci") || strstr(cmd, "exit") || strstr(cmd, "quit")) {
        app_play_sound(a, "Warnocmp");
        bank_push(is_it ? "DISCONNESSIONE ESEGUITA. TRACCIAMENTO EVITATO CON SUCCESSO!" :
                          "DISCONNECTED. TRACER EVADED SUCCESSFULLY!");
        b_state = BANK_DISCONNECTED;
        return;
    }

    if (strstr(cmd, "help") || strstr(cmd, "aiuto")) {
        if (is_it) {
            bank_push("COMANDI DISPONIBILI:");
            bank_push(" - CONTI: Mostra l'elenco dei conti correnti attivi.");
            bank_push(" - BONIFICO <DA> <IMPORTO> <A>: Trasferisce fondi su altro conto.");
            bank_push("   (Es: BONIFICO UP-8841 250000 DL-1983)");
            bank_push(" - BYPASS: Dirotta la linea telefonica per ridurre il tracciamento.");
            bank_push(" - ESCI: Termina la chiamata e salva il denaro trasferito.");
        } else {
            bank_push("AVAILABLE COMMANDS:");
            bank_push(" - ACCOUNTS: Displays list of active regional bank accounts.");
            bank_push(" - TRANSFER <FROM> <AMOUNT> <TO>: Wires funds between accounts.");
            bank_push("   (Ex: TRANSFER UP-8841 250000 DL-1983)");
            bank_push(" - BYPASS: Reroutes switch to drop the telecom tracer.");
            bank_push(" - LOGOUT: Safely hangs up modem to secure stolen funds.");
        }
        return;
    }

    if (strstr(cmd, "conti") || strstr(cmd, "accounts") || strstr(cmd, "list")) {
        bank_push("------------------------------------------------------------------");
        bank_push(is_it ? "ID CONTO   INTESTATARIO                     SALDO ATTUALE" :
                          "ACCT ID    ACCOUNT HOLDER                   CURRENT BALANCE");
        bank_push("------------------------------------------------------------------");
        for (int i = 0; i < 4; ++i) {
            bank_push("%-10s %-32s $ %11.2f", b_accounts[i].id, b_accounts[i].name, b_accounts[i].balance);
        }
        bank_push("------------------------------------------------------------------");
        return;
    }

    if (strstr(cmd, "bypass") || strstr(cmd, "devia")) {
        if (b_bypass_count > 0) {
            b_bypass_count--;
            b_trace_percent = b_trace_percent > 30 ? b_trace_percent - 30 : 5;
            app_play_sound(a, "Wargtr");
            bank_push(is_it ? ">> DEVIAZIONE SWITCH PACIFIC BELL ESEGUITA! TRACCIAMENTO RIDOTTO." :
                              ">> PACIFIC BELL SWITCH REROUTED! TRACE LEVEL DROPPED.");
        } else {
            bank_push(is_it ? ">> ERRORE: NESSUN NODO TELEFONICO DISPONIBILE PER IL BYPASS!" :
                              ">> ERROR: NO MORE TELEPHONE HUBS AVAILABLE FOR BYPASS!");
        }
        return;
    }

    if (strstr(cmd, "bonifico") || strstr(cmd, "transfer") || strstr(cmd, "wire") || strstr(cmd, "send")) {
        char src[32] = {0}, dst[32] = {0};
        double amt = 0.0;
        char word[32];
        if (sscanf(cmd, "%s %s %lf %s", word, src, &amt, dst) == 4 ||
            sscanf(cmd, "%s %s %s %lf", word, src, dst, &amt) == 4) {
            int src_idx = -1, dst_idx = -1;
            for (int i = 0; i < 4; ++i) {
                char tmp[16]; lower_str(tmp, sizeof(tmp), b_accounts[i].id);
                if (strcmp(tmp, src) == 0) src_idx = i;
                if (strcmp(tmp, dst) == 0) dst_idx = i;
            }
            if (src_idx >= 0 && dst_idx >= 0 && amt > 0.0) {
                if (b_accounts[src_idx].balance >= amt) {
                    b_accounts[src_idx].balance -= amt;
                    b_accounts[dst_idx].balance += amt;
                    app_play_sound(a, "Warxor0");
                    bank_push(is_it ? ">> TRANSAZIONE COMPLETATA: $ %.2f TRASFERITI CON SUCCESSO!" :
                                      ">> WIRE CONFIRMED: $ %.2f TRANSFERRED SUCCESSFULLY!", amt);
                    bank_push(">> %s: $ %.2f | %s: $ %.2f",
                              b_accounts[src_idx].id, b_accounts[src_idx].balance,
                              b_accounts[dst_idx].id, b_accounts[dst_idx].balance);
                    return;
                } else {
                    bank_push(is_it ? ">> ERRORE: FONDI INSUFFICIENTI SUL CONTO SORGENTE." :
                                      ">> ERROR: INSUFFICIENT FUNDS ON SOURCE ACCOUNT.");
                    return;
                }
            }
        }
        bank_push(is_it ? "SINTASSI: BONIFICO <CONTO_SORGENTE> <IMPORTO> <CONTO_DESTINAZIONE>" :
                          "SYNTAX: TRANSFER <SOURCE_ACCT> <AMOUNT> <DEST_ACCT>");
        bank_push(is_it ? "ESEMPIO: BONIFICO UP-8841 500000 DL-1983" :
                          "EXAMPLE: TRANSFER UP-8841 500000 DL-1983");
        return;
    }

    bank_push(is_it ? "COMANDO NON RICONOSCIUTO. DIGITA 'AIUTO' PER LA GUIDA." :
                      "COMMAND NOT RECOGNIZED. TYPE 'HELP' FOR ASSISTANCE.");
}

void ext_bank_render(App *a) {
    SDL_Color cyan = {131, 237, 240, 255};
    SDL_Color amber = {255, 191, 0, 255};
    SDL_Color red = {255, 64, 64, 255};
    int is_it = app_get_lang(a);

    /* Render history rows */
    int y = 20;
    for (int i = 0; i < b_history_count; ++i) {
        SDL_Color c = cyan;
        if (strstr(b_history[i], ">>") || strstr(b_history[i], "ACCESSO") || strstr(b_history[i], "GRANTED"))
            c = amber;
        if (strstr(b_history[i], "***") || strstr(b_history[i], "NEGATO") || strstr(b_history[i], "DENIED"))
            c = red;
        app_draw_text(a, b_history[i], 16, y, c);
        y += 18;
    }

    /* Trace status indicator in top-right */
    if (b_state == BANK_MAIN) {
        char trace_str[64];
        snprintf(trace_str, sizeof(trace_str), is_it ? "TRACCIAMENTO LINEA: %3d%% [BYPASS DISPONIBILI: %d]" :
                                                       "CARRIER TRACE: %3d%% [BYPASS REMAINING: %d]",
                 b_trace_percent, b_bypass_count);
        SDL_Color tc = b_trace_percent > 70 ? red : amber;
        app_draw_text(a, trace_str, 320, 16, tc);
    }

    /* Current input line with blinking cursor */
    if (b_state != BANK_DISCONNECTED) {
        const char *inp = app_get_input(a);
        char prompt_buf[128];
        snprintf(prompt_buf, sizeof(prompt_buf), "> %s", inp);
        app_draw_text(a, prompt_buf, 16, y + 6, cyan);
        app_draw_block_cursor(a, 16 + (int)strlen(prompt_buf) * 8, y + 6, cyan);
    } else {
        app_draw_text(a, is_it ? "[PREMI INVIO PER USCIRE AL SELETTORE LINEE TELEFONICHE]" :
                                 "[PRESS ENTER TO RETURN TO PHONE CARRIERS]", 16, y + 16, amber);
    }
}
