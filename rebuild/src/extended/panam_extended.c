#include "extended.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef enum {
    PANAM_DEPARTURE,
    PANAM_DESTINATION,
    PANAM_DATE,
    PANAM_PASSENGER,
    PANAM_CLASS,
    PANAM_TICKET_VIEW
} PanamStep;

static PanamStep p_step;
static char p_dep[64];
static char p_dest[64];
static char p_date[64];
static char p_pass[64];
static char p_class[32];
static char p_seat[16];
static int p_flight_num;
static double p_fare;

static void trim_copy(char *dst, size_t cap, const char *src) {
    while (*src == ' ' || *src == '\t') src++;
    size_t len = strlen(src);
    while (len > 0 && (src[len - 1] == ' ' || src[len - 1] == '\t' || src[len - 1] == '\r' || src[len - 1] == '\n'))
        len--;
    if (len >= cap) len = cap - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

void ext_panam_init(App *a) {
    (void)a;
    p_step = PANAM_DEPARTURE;
    p_dep[0] = '\0';
    p_dest[0] = '\0';
    p_date[0] = '\0';
    p_pass[0] = '\0';
    p_class[0] = '\0';
    p_seat[0] = '\0';
    p_flight_num = 114;
    p_fare = 1235.00;
}

void ext_panam_submit(App *a, const char *input) {
    char clean[64];
    trim_copy(clean, sizeof(clean), input);
    int is_it = app_get_lang(a);

    switch (p_step) {
    case PANAM_DEPARTURE:
        if (!clean[0]) {
            snprintf(p_dep, sizeof(p_dep), "%s", is_it ? "CHICAGO / O'HARE" : "CHICAGO / O'HARE");
        } else if (strcmp(clean, "1") == 0) {
            snprintf(p_dep, sizeof(p_dep), "SEATTLE / TACOMA INTL");
        } else if (strcmp(clean, "2") == 0) {
            snprintf(p_dep, sizeof(p_dep), "NEW YORK / JFK");
        } else if (strcmp(clean, "3") == 0) {
            snprintf(p_dep, sizeof(p_dep), "CHICAGO / O'HARE");
        } else if (strcmp(clean, "4") == 0) {
            snprintf(p_dep, sizeof(p_dep), "SAN FRANCISCO / SFO");
        } else if (strcmp(clean, "5") == 0) {
            snprintf(p_dep, sizeof(p_dep), "ROMA / FIUMICINO");
        } else {
            snprintf(p_dep, sizeof(p_dep), "%s", clean);
        }
        app_play_sound(a, "Wargtr");
        p_step = PANAM_DESTINATION;
        break;

    case PANAM_DESTINATION:
        if (!clean[0]) {
            snprintf(p_dest, sizeof(p_dest), "%s", "PARIS / CHARLES DE GAULLE");
        } else if (strcmp(clean, "1") == 0) {
            snprintf(p_dest, sizeof(p_dest), "PARIS / CHARLES DE GAULLE");
        } else if (strcmp(clean, "2") == 0) {
            snprintf(p_dest, sizeof(p_dest), "LONDON / HEATHROW");
        } else if (strcmp(clean, "3") == 0) {
            snprintf(p_dest, sizeof(p_dest), "TOKYO / HANEDA");
        } else if (strcmp(clean, "4") == 0) {
            snprintf(p_dest, sizeof(p_dest), "HONOLULU / INTL");
        } else if (strcmp(clean, "5") == 0) {
            snprintf(p_dest, sizeof(p_dest), "ROMA / FIUMICINO");
        } else {
            snprintf(p_dest, sizeof(p_dest), "%s", clean);
        }
        app_play_sound(a, "Wargtr");
        p_step = PANAM_DATE;
        break;

    case PANAM_DATE:
        if (!clean[0]) {
            snprintf(p_date, sizeof(p_date), "08/18/1984");
        } else {
            snprintf(p_date, sizeof(p_date), "%s", clean);
        }
        app_play_sound(a, "Wargtr");
        p_step = PANAM_PASSENGER;
        break;

    case PANAM_PASSENGER:
        if (!clean[0]) {
            snprintf(p_pass, sizeof(p_pass), "DAVID LIGHTMAN & JENNIFER MACK");
        } else {
            snprintf(p_pass, sizeof(p_pass), "%s", clean);
        }
        app_play_sound(a, "Wargtr");
        p_step = PANAM_CLASS;
        break;

    case PANAM_CLASS:
        if (clean[0] == '1' || clean[0] == 'f' || clean[0] == 'F') {
            snprintf(p_class, sizeof(p_class), "FIRST CLASS (F)");
            snprintf(p_seat, sizeof(p_seat), "02A / 02B");
            p_fare = 2840.00;
        } else if (clean[0] == '2' || clean[0] == 'c' || clean[0] == 'C') {
            snprintf(p_class, sizeof(p_class), "CLIPPER CLASS (C)");
            snprintf(p_seat, sizeof(p_seat), "08C / 08D");
            p_fare = 1890.00;
        } else {
            snprintf(p_class, sizeof(p_class), "ECONOMY (Y)");
            snprintf(p_seat, sizeof(p_seat), "17A / 17B");
            p_fare = 1164.00;
        }
        p_flight_num = 100 + (rand() % 899);
        app_play_sound(a, "Warxor0");
        p_step = PANAM_TICKET_VIEW;
        break;

    case PANAM_TICKET_VIEW:
        if (clean[0] == 'n' || clean[0] == 'N' || clean[0] == 'b' || clean[0] == 'B') {
            ext_panam_init(a);
        } else {
            /* Return to dial numbers */
            app_start_stage(a, 6 /* DIAL_NUMBERS */, "");
        }
        break;
    }
}

void ext_panam_render(App *a) {
    SDL_Color cyan = {131, 237, 240, 255};
    SDL_Color amber = {255, 191, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {105, 255, 184, 255};
    int is_it = app_get_lang(a);

    /* Header */
    app_draw_text(a, "================================================================================", 8, 12, cyan);
    app_draw_text(a, "PAN AMERICAN WORLD AIRWAYS - SYSTEM ONE / RESERVATION MAINFRAME", 16, 28, amber);
    app_draw_text(a, "================================================================================", 8, 44, cyan);

    int y = 70;
    if (p_step == PANAM_DEPARTURE) {
        app_draw_text(a, is_it ? "--- PRENOTAZIONE VOLO PAN AM (EDIZIONE ESTESA) ---" :
                                 "--- PAN AM FLIGHT RESERVATION SYSTEM (EXTENDED) ---", 16, y, green);
        y += 24;
        app_draw_text(a, is_it ? "SCELTA RAPIDA CITTA' DI PARTENZA:" :
                                 "QUICK AIRPORT SELECTION (DEPARTURE):", 16, y, amber);
        y += 20;
        app_draw_text(a, "1. SEATTLE / TACOMA INTL (SEA)       4. SAN FRANCISCO INTL (SFO)", 32, y, cyan); y += 18;
        app_draw_text(a, "2. NEW YORK / JFK INTL (JFK)         5. ROMA / FIUMICINO (FCO)", 32, y, cyan); y += 18;
        app_draw_text(a, "3. CHICAGO / O'HARE (ORD)            [O DIGITA QUALSIASI ALTRA CITTA']", 32, y, cyan); y += 30;

        app_draw_text(a, is_it ? "LUOGO DI PARTENZA [Invio = Chicago]: " :
                                 "PLACE OF DEPARTURE [Enter = Chicago]: ", 16, y, white);
        const char *inp = app_get_input(a);
        int ox = 16 + (is_it ? 37 : 38) * 8;
        app_draw_text(a, inp, ox, y, cyan);
        app_draw_block_cursor(a, ox + (int)strlen(inp) * 8, y, cyan);
    }
    else if (p_step == PANAM_DESTINATION) {
        char buf[128];
        snprintf(buf, sizeof(buf), is_it ? "PARTENZA CONFERMATA: %s" : "DEPARTURE SELECTED: %s", p_dep);
        app_draw_text(a, buf, 16, y, green);
        y += 24;
        app_draw_text(a, is_it ? "SCELTA RAPIDA DESTINAZIONE:" :
                                 "QUICK AIRPORT SELECTION (DESTINATION):", 16, y, amber);
        y += 20;
        app_draw_text(a, "1. PARIS / CHARLES DE GAULLE (CDG)   4. HONOLULU INTL (HNL)", 32, y, cyan); y += 18;
        app_draw_text(a, "2. LONDON / HEATHROW (LHR)           5. ROMA / FIUMICINO (FCO)", 32, y, cyan); y += 18;
        app_draw_text(a, "3. TOKYO / HANEDA (HND)              [O DIGITA QUALSIASI ALTRA CITTA']", 32, y, cyan); y += 30;

        app_draw_text(a, is_it ? "DESTINAZIONE [Invio = Parigi]: " :
                                 "DESTINATION [Enter = Paris]: ", 16, y, white);
        const char *inp = app_get_input(a);
        int ox = 16 + (is_it ? 31 : 29) * 8;
        app_draw_text(a, inp, ox, y, cyan);
        app_draw_block_cursor(a, ox + (int)strlen(inp) * 8, y, cyan);
    }
    else if (p_step == PANAM_DATE) {
        char buf1[128], buf2[128];
        snprintf(buf1, sizeof(buf1), is_it ? "PARTENZA:     %s" : "DEPARTURE:    %s", p_dep);
        snprintf(buf2, sizeof(buf2), is_it ? "DESTINAZIONE: %s" : "DESTINATION:  %s", p_dest);
        app_draw_text(a, buf1, 16, y, cyan); y += 20;
        app_draw_text(a, buf2, 16, y, cyan); y += 28;

        app_draw_text(a, is_it ? "DATA DEL VOLO (MM/GG/AA) [Invio = 08/18/1984]: " :
                                 "DEPARTURE DATE (MM/DD/YY) [Enter = 08/18/1984]: ", 16, y, white);
        const char *inp = app_get_input(a);
        int ox = 16 + (is_it ? 47 : 48) * 8;
        app_draw_text(a, inp, ox, y, cyan);
        app_draw_block_cursor(a, ox + (int)strlen(inp) * 8, y, cyan);
    }
    else if (p_step == PANAM_PASSENGER) {
        char buf1[256], buf2[256];
        snprintf(buf1, sizeof(buf1), is_it ? "ITINERARIO: %s ===> %s" : "ROUTING:    %s ===> %s", p_dep, p_dest);
        snprintf(buf2, sizeof(buf2), is_it ? "DATA:       %s" : "DATE:       %s", p_date);
        app_draw_text(a, buf1, 16, y, cyan); y += 20;
        app_draw_text(a, buf2, 16, y, cyan); y += 28;

        app_draw_text(a, is_it ? "NOME PASSEGGERO [Invio = David Lightman & Jennifer Mack]: " :
                                 "NAME OF PASSENGER [Enter = David Lightman & Jennifer Mack]: ", 16, y, white);
        const char *inp = app_get_input(a);
        int ox = 16 + (is_it ? 58 : 60) * 8;
        app_draw_text(a, inp, ox, y, cyan);
        app_draw_block_cursor(a, ox + (int)strlen(inp) * 8, y, cyan);
    }
    else if (p_step == PANAM_CLASS) {
        char buf[128];
        snprintf(buf, sizeof(buf), is_it ? "PASSEGGERO: %s" : "PASSENGER:  %s", p_pass);
        app_draw_text(a, buf, 16, y, cyan); y += 26;

        app_draw_text(a, is_it ? "SELEZIONA CLASSE DI VIAGGIO:" : "SELECT TRAVEL CLASS:", 16, y, amber); y += 20;
        app_draw_text(a, "1. FIRST CLASS (F)         - $ 2,840.00", 32, y, cyan); y += 18;
        app_draw_text(a, "2. CLIPPER BUSINESS (C)    - $ 1,890.00", 32, y, cyan); y += 18;
        app_draw_text(a, "3. ECONOMY CABIN (Y)       - $ 1,164.00", 32, y, cyan); y += 28;

        app_draw_text(a, is_it ? "CLASSE SCELTA [1-3, Invio = First Class]: " :
                                 "CHOSEN CLASS [1-3, Enter = First Class]: ", 16, y, white);
        const char *inp = app_get_input(a);
        int ox = 16 + (is_it ? 42 : 41) * 8;
        app_draw_text(a, inp, ox, y, cyan);
        app_draw_block_cursor(a, ox + (int)strlen(inp) * 8, y, cyan);
    }
    else if (p_step == PANAM_TICKET_VIEW) {
        /* ASCII Boarding Pass & Itinerary */
        app_draw_text(a, "+----------------------------------------------------------------------------+", 8, y, amber); y += 16;
        app_draw_text(a, "| PAN AM ELECTRONIC PASSENGER TICKET & BOARDING PASS                         |", 8, y, amber); y += 16;
        app_draw_text(a, "+----------------------------------------------------------------------------+", 8, y, amber); y += 20;

        char line[256];
        snprintf(line, sizeof(line), "PASSENGER:  %-48s STATUS: OK / CONFIRMED", p_pass);
        app_draw_text(a, line, 16, y, white); y += 18;

        snprintf(line, sizeof(line), "DATE OF ISSUE: 08/08/84      FLIGHT: PA-%-4d     CLASS: %-16s", p_flight_num, p_class);
        app_draw_text(a, line, 16, y, cyan); y += 18;

        snprintf(line, sizeof(line), "DEPARTURE:     %-32s DATE: %s", p_dep, p_date);
        app_draw_text(a, line, 16, y, cyan); y += 18;

        snprintf(line, sizeof(line), "DESTINATION:   %-32s SEAT: %s", p_dest, p_seat);
        app_draw_text(a, line, 16, y, cyan); y += 18;

        snprintf(line, sizeof(line), "BASE FARE:     $ %-10.2f     TAX: $ 70.90      TOTAL: $ %-10.2f", p_fare, p_fare + 70.90);
        app_draw_text(a, line, 16, y, cyan); y += 18;

        app_draw_text(a, "PAYMENT:       DIRECT BILLING TO CORPORATE ACCOUNT ND-0012 [APPROVED]", 16, y, green); y += 24;

        app_draw_text(a, "+----------------------------------------------------------------------------+", 8, y, amber); y += 16;
        app_draw_text(a, "| BOARDING PASS VALIDATED - NO PHYSICAL TICKET REQUIRED AT CHECK-IN          |", 8, y, green); y += 16;
        app_draw_text(a, "+----------------------------------------------------------------------------+", 8, y, amber); y += 30;

        app_draw_text(a, is_it ? "[N] PRENOTA UN ALTRO VOLO     |     [INVIO] TORNA AL SELETTORE LINEE" :
                                 "[N] BOOK ANOTHER FLIGHT       |     [ENTER] RETURN TO CARRIER SELECTION", 16, y, amber);
        app_draw_block_cursor(a, 16 + (is_it ? 72 : 71) * 8, y, amber);
    }
}
