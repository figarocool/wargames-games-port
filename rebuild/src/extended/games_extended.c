#include "extended.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ========================================================================= */
/*                          EXTENDED CHESS ENGINE                            */
/* ========================================================================= */

#define CHESS_EMPTY 0
#define CHESS_PAWN   1
#define CHESS_KNIGHT 2
#define CHESS_BISHOP 3
#define CHESS_ROOK   4
#define CHESS_QUEEN  5
#define CHESS_KING   6

#define COLOR_WHITE 1
#define COLOR_BLACK 2

typedef struct {
    int piece;
    int color;
} ChessSquare;

static ChessSquare c_board[8][8];
static int c_player_turn; /* 1 = White (Human), 2 = Black (Joshua) */
static char c_last_move[64];
static char c_message[128];
static int c_game_over;

static void chess_reset_board(void) {
    memset(c_board, 0, sizeof(c_board));
    static const int back_row[] = {
        CHESS_ROOK, CHESS_KNIGHT, CHESS_BISHOP, CHESS_QUEEN,
        CHESS_KING, CHESS_BISHOP, CHESS_KNIGHT, CHESS_ROOK
    };
    for (int col = 0; col < 8; ++col) {
        c_board[7][col].piece = back_row[col];
        c_board[7][col].color = COLOR_WHITE;
        c_board[6][col].piece = CHESS_PAWN;
        c_board[6][col].color = COLOR_WHITE;

        c_board[0][col].piece = back_row[col];
        c_board[0][col].color = COLOR_BLACK;
        c_board[1][col].piece = CHESS_PAWN;
        c_board[1][col].color = COLOR_BLACK;
    }
    c_player_turn = COLOR_WHITE;
    c_last_move[0] = '\0';
    c_message[0] = '\0';
    c_game_over = 0;
}

void ext_chess_init(App *a) {
    (void)a;
    chess_reset_board();
    int is_it = app_get_lang(a);
    snprintf(c_message, sizeof(c_message), "%s",
             is_it ? "JOSHUA: SCEGLI LA TUA MOSSA (ESEMPIO: E2E4, G1F3, RESIGN)." :
                     "JOSHUA: ENTER YOUR MOVE (EXAMPLE: E2E4, G1F3, RESIGN).");
}

static int is_valid_coord(int r, int c) {
    return r >= 0 && r < 8 && c >= 0 && c < 8;
}

static int can_move_piece(int sr, int sc, int dr, int dc, int color) {
    if (!is_valid_coord(sr, sc) || !is_valid_coord(dr, dc)) return 0;
    if (sr == dr && sc == dc) return 0;
    ChessSquare *src = &c_board[sr][sc];
    ChessSquare *dst = &c_board[dr][dc];
    if (src->color != color) return 0;
    if (dst->color == color) return 0;

    int r_diff = dr - sr;
    int c_diff = dc - sc;
    int abs_r = abs(r_diff);
    int abs_c = abs(c_diff);

    switch (src->piece) {
    case CHESS_PAWN:
        if (color == COLOR_WHITE) {
            /* Move forward 1 */
            if (c_diff == 0 && r_diff == -1 && dst->piece == CHESS_EMPTY) return 1;
            /* Move forward 2 from starting rank */
            if (c_diff == 0 && sr == 6 && r_diff == -2 &&
                c_board[5][sc].piece == CHESS_EMPTY && dst->piece == CHESS_EMPTY) return 1;
            /* Capture diagonally */
            if (abs_c == 1 && r_diff == -1 && dst->piece != CHESS_EMPTY && dst->color == COLOR_BLACK) return 1;
        } else {
            /* Move forward 1 */
            if (c_diff == 0 && r_diff == 1 && dst->piece == CHESS_EMPTY) return 1;
            /* Move forward 2 from starting rank */
            if (c_diff == 0 && sr == 1 && r_diff == 2 &&
                c_board[2][sc].piece == CHESS_EMPTY && dst->piece == CHESS_EMPTY) return 1;
            /* Capture diagonally */
            if (abs_c == 1 && r_diff == 1 && dst->piece != CHESS_EMPTY && dst->color == COLOR_WHITE) return 1;
        }
        return 0;

    case CHESS_KNIGHT:
        return (abs_r == 1 && abs_c == 2) || (abs_r == 2 && abs_c == 1);

    case CHESS_BISHOP:
        if (abs_r != abs_c) return 0;
        int step_r = (dr > sr) ? 1 : -1;
        int step_c = (dc > sc) ? 1 : -1;
        for (int i = 1; i < abs_r; ++i) {
            if (c_board[sr + i * step_r][sc + i * step_c].piece != CHESS_EMPTY) return 0;
        }
        return 1;

    case CHESS_ROOK:
        if (sr != dr && sc != dc) return 0;
        if (sr == dr) {
            int step = (dc > sc) ? 1 : -1;
            for (int c = sc + step; c != dc; c += step) {
                if (c_board[sr][c].piece != CHESS_EMPTY) return 0;
            }
        } else {
            int step = (dr > sr) ? 1 : -1;
            for (int r = sr + step; r != dr; r += step) {
                if (c_board[r][sc].piece != CHESS_EMPTY) return 0;
            }
        }
        return 1;

    case CHESS_QUEEN:
        if (abs_r == abs_c) {
            int step_r = (dr > sr) ? 1 : -1;
            int step_c = (dc > sc) ? 1 : -1;
            for (int i = 1; i < abs_r; ++i) {
                if (c_board[sr + i * step_r][sc + i * step_c].piece != CHESS_EMPTY) return 0;
            }
            return 1;
        } else if (sr == dr || sc == dc) {
            if (sr == dr) {
                int step = (dc > sc) ? 1 : -1;
                for (int c = sc + step; c != dc; c += step) {
                    if (c_board[sr][c].piece != CHESS_EMPTY) return 0;
                }
            } else {
                int step = (dr > sr) ? 1 : -1;
                for (int r = sr + step; r != dr; r += step) {
                    if (c_board[r][sc].piece != CHESS_EMPTY) return 0;
                }
            }
            return 1;
        }
        return 0;

    case CHESS_KING:
        return abs_r <= 1 && abs_c <= 1;
    }
    return 0;
}

static void joshua_make_move(App *a) {
    int is_it = app_get_lang(a);
    typedef struct {
        int sr, sc, dr, dc, score;
    } MoveCandidate;
    MoveCandidate candidates[256];
    int count = 0;

    for (int sr = 0; sr < 8; ++sr) {
        for (int sc = 0; sc < 8; ++sc) {
            if (c_board[sr][sc].color == COLOR_BLACK) {
                for (int dr = 0; dr < 8; ++dr) {
                    for (int dc = 0; dc < 8; ++dc) {
                        if (can_move_piece(sr, sc, dr, dc, COLOR_BLACK)) {
                            int score = 0;
                            /* Capture evaluation */
                            if (c_board[dr][dc].piece != CHESS_EMPTY) {
                                static const int values[] = {0, 10, 30, 30, 50, 90, 900};
                                score = values[c_board[dr][dc].piece] * 10;
                            }
                            /* Center control preference */
                            score += (3 - abs(dr - 3)) + (3 - abs(dc - 3));
                            if (count < 256) {
                                candidates[count++] = (MoveCandidate){sr, sc, dr, dc, score};
                            }
                        }
                    }
                }
            }
        }
    }

    if (count == 0) {
        c_game_over = 1;
        snprintf(c_message, sizeof(c_message), "%s",
                 is_it ? "SCACCO MATTO! HAI VINTO CONTRO W.O.P.R.!" :
                         "CHECKMATE! YOU HAVE WON AGAINST W.O.P.R.!");
        app_play_sound(a, "Warxor0");
        return;
    }

    /* Pick best move with slight random jitter */
    int best_idx = 0;
    int best_score = -9999;
    for (int i = 0; i < count; ++i) {
        int s = candidates[i].score + (rand() % 4);
        if (s > best_score) {
            best_score = s;
            best_idx = i;
        }
    }

    MoveCandidate m = candidates[best_idx];
    int target_piece = c_board[m.dr][m.dc].piece;
    c_board[m.dr][m.dc] = c_board[m.sr][m.sc];
    c_board[m.sr][m.sc] = (ChessSquare){CHESS_EMPTY, 0};

    /* Pawn promotion for black */
    if (m.dr == 7 && c_board[m.dr][m.dc].piece == CHESS_PAWN) {
        c_board[m.dr][m.dc].piece = CHESS_QUEEN;
    }

    snprintf(c_last_move, sizeof(c_last_move), "JOSHUA: %c%d-%c%d",
             'a' + m.sc, 8 - m.sr, 'a' + m.dc, 8 - m.dr);
    app_play_sound(a, "Wargtr");

    if (target_piece == CHESS_KING) {
        c_game_over = 1;
        snprintf(c_message, sizeof(c_message), "%s",
                 is_it ? "JOSHUA HA CATTURATO IL TUO RE. PARTITA TERMINATA." :
                         "JOSHUA CAPTURED YOUR KING. GAME OVER.");
        return;
    }

    snprintf(c_message, sizeof(c_message), "%s",
             is_it ? "A TE LA MOSSA. (DIGITA LE COORDINATE, ES: E7E5, O 'ESCI')" :
                     "YOUR TURN. (ENTER COORDINATES, EX: E7E5, OR 'QUIT')");
    c_player_turn = COLOR_WHITE;
}

void ext_chess_submit(App *a, const char *input) {
    int is_it = app_get_lang(a);
    char cmd[64];
    size_t len = 0;
    for (size_t i = 0; input[i] && len + 1 < sizeof(cmd); ++i) {
        if (!isspace((unsigned char)input[i]) && input[i] != '-') {
            cmd[len++] = (char)tolower((unsigned char)input[i]);
        }
    }
    cmd[len] = '\0';

    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "esci") == 0 ||
        strcmp(cmd, "quit") == 0 || strcmp(cmd, "resign") == 0 ||
        strcmp(cmd, "arrenditi") == 0) {
        app_start_stage(a, STAGE_LIST_GAMES, "");
        return;
    }

    if (c_game_over) {
        chess_reset_board();
        return;
    }

    if (len == 4) {
        int sc = cmd[0] - 'a';
        int sr = 8 - (cmd[1] - '0');
        int dc = cmd[2] - 'a';
        int dr = 8 - (cmd[3] - '0');

        if (can_move_piece(sr, sc, dr, dc, COLOR_WHITE)) {
            int captured = c_board[dr][dc].piece;
            c_board[dr][dc] = c_board[sr][sc];
            c_board[sr][sc] = (ChessSquare){CHESS_EMPTY, 0};

            /* Pawn promotion for white */
            if (dr == 0 && c_board[dr][dc].piece == CHESS_PAWN) {
                c_board[dr][dc].piece = CHESS_QUEEN;
            }

            app_play_sound(a, "Warxor0");

            if (captured == CHESS_KING) {
                c_game_over = 1;
                snprintf(c_message, sizeof(c_message), "%s",
                         is_it ? "HAI CATTURATO IL RE DI JOSHUA! VITTORIA!" :
                                 "YOU CAPTURED JOSHUA'S KING! VICTORY!");
                return;
            }

            c_player_turn = COLOR_BLACK;
            joshua_make_move(a);
            return;
        }
    }

    app_play_sound(a, "Waracdnd");
    snprintf(c_message, sizeof(c_message), "%s",
             is_it ? "MOSSA NON VALIDA. FORMATO: SORGENTE+DESTINAZIONE (ES: E2E4)" :
                     "INVALID MOVE. FORMAT: SOURCE+DESTINATION (EX: E2E4)");
}

void ext_chess_render(App *a) {
    SDL_Color cyan = {131, 237, 240, 255};
    SDL_Color amber = {255, 191, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {105, 255, 184, 255};
    int is_it = app_get_lang(a);

    app_draw_text(a, "================================================================================", 8, 12, cyan);
    app_draw_text(a, "W.O.P.R. STRATEGIC SIMULATION TERMINAL - CHESS / SCACCHI", 16, 28, amber);
    app_draw_text(a, "================================================================================", 8, 44, cyan);

    /* Render ASCII Chess Board */
    static const char piece_symbols_w[] = " PNBRQK";
    static const char piece_symbols_b[] = " pnbrqk";

    int start_y = 68;
    int start_x = 100;

    app_draw_text(a, "    a   b   c   d   e   f   g   h", start_x, start_y, green);
    app_draw_text(a, "  +---+---+---+---+---+---+---+---+", start_x, start_y + 16, cyan);

    for (int r = 0; r < 8; ++r) {
        char row_str[64];
        int pos = snprintf(row_str, sizeof(row_str), "%d |", 8 - r);
        for (int c = 0; c < 8; ++c) {
            char p = ' ';
            if (c_board[r][c].color == COLOR_WHITE) p = piece_symbols_w[c_board[r][c].piece];
            else if (c_board[r][c].color == COLOR_BLACK) p = piece_symbols_b[c_board[r][c].piece];
            pos += snprintf(row_str + pos, sizeof(row_str) - pos, " %c |", p);
        }
        snprintf(row_str + pos, sizeof(row_str) - pos, " %d", 8 - r);
        app_draw_text(a, row_str, start_x, start_y + 32 + r * 22, white);
        app_draw_text(a, "  +---+---+---+---+---+---+---+---+", start_x, start_y + 44 + r * 22, cyan);
    }
    app_draw_text(a, "    a   b   c   d   e   f   g   h", start_x, start_y + 220, green);

    /* Side info */
    int side_x = 420;
    app_draw_text(a, is_it ? "AVVERSARIO: JOSHUA (NERO [p,n,b,r,q,k])" :
                             "OPPONENT:   JOSHUA (BLACK [p,n,b,r,q,k])", side_x, 80, amber);
    app_draw_text(a, is_it ? "GIOCATORE:  DAVID (BIANCO [P,N,B,R,Q,K])" :
                             "PLAYER:     DAVID (WHITE [P,N,B,R,Q,K])", side_x, 100, white);
    if (c_last_move[0]) {
        app_draw_text(a, c_last_move, side_x, 130, green);
    }

    app_draw_text(a, is_it ? "COMANDI:" : "COMMANDS:", side_x, 160, amber);
    app_draw_text(a, is_it ? "- DIGITA MOSSA: E2E4, D7D5, G1F3" : "- ENTER MOVE: E2E4, D7D5, G1F3", side_x, 180, cyan);
    app_draw_text(a, is_it ? "- ESCI: TORNA ALL'ELENCO DEI GIOCHI" : "- QUIT: RETURN TO GAME LIST", side_x, 200, cyan);

    /* Status message */
    app_draw_text(a, c_message, 16, 320, amber);

    /* Input line */
    const char *inp = app_get_input(a);
    char prompt[128];
    snprintf(prompt, sizeof(prompt), is_it ? "LA TUA MOSSA [ES: E2E4 / ESCI]: %s" :
                                             "YOUR MOVE [EX: E2E4 / QUIT]: %s", inp);
    app_draw_text(a, prompt, 16, 350, white);
    app_draw_block_cursor(a, 16 + (int)strlen(prompt) * 8, 350, cyan);
}

/* ========================================================================= */
/*                          FALKEN'S MAZE GAME                               */
/* ========================================================================= */

#define MAZE_W 27
#define MAZE_H 13

static char m_grid[MAZE_H][MAZE_W + 1] = {
    "###########################",
    "#@  #       #       #     #",
    "# # ### # # ### ### # ### #",
    "# #   # # #     # #   #   #",
    "# ### # ### ### # ##### ###",
    "#   # #   #   # #     #   #",
    "### # ### # # # ##### # # #",
    "#   #     # # #     #   # #",
    "# ##### ### # ##### ### # #",
    "#     #   # #   #     # # #",
    "##### ### # ### # ### # # #",
    "#         #     #   #    E#",
    "###########################"
};

static int m_player_x = 1;
static int m_player_y = 1;
static int m_moves_count = 0;
static int m_won = 0;

void ext_maze_init(App *a) {
    (void)a;
    m_player_x = 1;
    m_player_y = 1;
    m_moves_count = 0;
    m_won = 0;
}

void ext_maze_key(App *a, SDL_Keycode key) {
    if (m_won) {
        if (key == SDLK_RETURN || key == SDLK_SPACE || key == SDLK_ESCAPE) {
            app_start_stage(a, STAGE_LIST_GAMES, "");
        }
        return;
    }

    int dx = 0, dy = 0;
    if (key == SDLK_UP || key == SDLK_w) dy = -1;
    else if (key == SDLK_DOWN || key == SDLK_s) dy = 1;
    else if (key == SDLK_LEFT || key == SDLK_a) dx = -1;
    else if (key == SDLK_RIGHT || key == SDLK_d) dx = 1;
    else if (key == SDLK_ESCAPE) {
        app_start_stage(a, STAGE_LIST_GAMES, "");
        return;
    }

    if (dx != 0 || dy != 0) {
        int nx = m_player_x + dx;
        int ny = m_player_y + dy;
        if (nx >= 0 && nx < MAZE_W && ny >= 0 && ny < MAZE_H && m_grid[ny][nx] != '#') {
            m_player_x = nx;
            m_player_y = ny;
            m_moves_count++;
            app_play_sound(a, "Warcursor");
            if (m_grid[ny][nx] == 'E') {
                m_won = 1;
                app_play_sound(a, "Warxor0");
            }
        }
    }
}

void ext_maze_submit(App *a, const char *input) {
    char cmd[32];
    size_t len = 0;
    for (size_t i = 0; input[i] && len + 1 < sizeof(cmd); ++i) {
        cmd[len++] = (char)tolower((unsigned char)input[i]);
    }
    cmd[len] = '\0';

    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "esci") == 0 || strcmp(cmd, "quit") == 0) {
        app_start_stage(a, STAGE_LIST_GAMES, "");
        return;
    }
    if (m_won) {
        app_start_stage(a, STAGE_LIST_GAMES, "");
        return;
    }

    for (size_t i = 0; cmd[i]; ++i) {
        if (cmd[i] == 'w' || cmd[i] == 'u') ext_maze_key(a, SDLK_UP);
        else if (cmd[i] == 's' || cmd[i] == 'd') ext_maze_key(a, SDLK_DOWN);
        else if (cmd[i] == 'a' || cmd[i] == 'l') ext_maze_key(a, SDLK_LEFT);
        else if (cmd[i] == 'r') ext_maze_key(a, SDLK_RIGHT);
    }
}

void ext_maze_render(App *a) {
    SDL_Color cyan = {131, 237, 240, 255};
    SDL_Color amber = {255, 191, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {105, 255, 184, 255};
    int is_it = app_get_lang(a);

    app_draw_text(a, "================================================================================", 8, 12, cyan);
    app_draw_text(a, "W.O.P.R. PROTO PROTOCOL - FALKEN'S MAZE / IL LABIRINTO DI FALKEN", 16, 28, amber);
    app_draw_text(a, "================================================================================", 8, 44, cyan);

    int start_y = 70;
    for (int y = 0; y < MAZE_H; ++y) {
        char line[MAZE_W + 1];
        memcpy(line, m_grid[y], MAZE_W);
        line[MAZE_W] = '\0';
        if (y == m_player_y) line[m_player_x] = '@';
        app_draw_text(a, line, 80, start_y + y * 18, cyan);
    }

    int side_x = 360;
    app_draw_text(a, is_it ? "CONTROLLI:" : "CONTROLS:", side_x, 80, amber);
    app_draw_text(a, is_it ? "- FRECCE DIREZIONALI o W, A, S, D" : "- ARROW KEYS or W, A, S, D", side_x, 100, white);
    app_draw_text(a, is_it ? "- DIGITA 'ESCI' PER TORNARE AL MENU" : "- TYPE 'QUIT' TO RETURN TO MENU", side_x, 120, white);

    char stats[64];
    snprintf(stats, sizeof(stats), is_it ? "PASSI EFFETTUATI: %d" : "STEPS TAKEN: %d", m_moves_count);
    app_draw_text(a, stats, side_x, 150, green);

    app_draw_text(a, is_it ? "OBIETTIVO: RAGGIUNGI L'USCITA 'E'" : "OBJECTIVE: REACH EXIT 'E'", side_x, 180, amber);

    if (m_won) {
        app_draw_text(a, "****************************************************************", 16, 330, green);
        app_draw_text(a, is_it ? "COMPLIMENTI! HAI RISOLTO IL LABIRINTO DI FALKEN!" :
                                 "CONGRATULATIONS! YOU SOLVED FALKEN'S MAZE!", 16, 350, amber);
        app_draw_text(a, is_it ? "JOSHUA: 'UN GIOCO INTERESSANTE, PROFESSORE.' [PREMI INVIO PER USCIRE]" :
                                 "JOSHUA: 'AN INTERESTING GAME, PROFESSOR.' [PRESS ENTER TO EXIT]", 16, 370, white);
        app_draw_text(a, "****************************************************************", 16, 390, green);
    } else {
        const char *inp = app_get_input(a);
        char prompt[128];
        snprintf(prompt, sizeof(prompt), is_it ? "COMANDO [ESCI]: %s" : "COMMAND [QUIT]: %s", inp);
        app_draw_text(a, prompt, 16, 350, white);
        app_draw_block_cursor(a, 16 + (int)strlen(prompt) * 8, 350, cyan);
    }
}

/* ========================================================================= */
/*                   BLACKJACK & 5-CARD DRAW POKER                           */
/* ========================================================================= */

typedef struct {
    int rank; /* 2-14 (14=Ace) */
    int suit; /* 0=S, 1=H, 2=D, 3=C */
} Card;

static int cd_is_poker = 0;
static int cd_chips = 1000;
static Card cd_player_hand[5];
static Card cd_dealer_hand[5];
static int cd_player_count = 0;
static int cd_dealer_count = 0;
static int cd_state = 0; /* 0 = dealing/betting, 1 = player turn, 2 = resolved */
static char cd_msg[128];

static Card get_random_card(void) {
    Card c;
    c.rank = 2 + (rand() % 13);
    c.suit = rand() % 4;
    return c;
}

static int blackjack_eval(const Card *hand, int count) {
    int sum = 0, aces = 0;
    for (int i = 0; i < count; ++i) {
        if (hand[i].rank >= 10 && hand[i].rank <= 13) sum += 10;
        else if (hand[i].rank == 14) { sum += 11; aces++; }
        else sum += hand[i].rank;
    }
    while (sum > 21 && aces > 0) {
        sum -= 10;
        aces--;
    }
    return sum;
}

void ext_cards_init(App *a, int is_poker) {
    (void)a;
    cd_is_poker = is_poker;
    cd_chips = 1000;
    cd_state = 0;
    cd_player_count = 0;
    cd_dealer_count = 0;
    int is_it = app_get_lang(a);

    if (!is_poker) {
        /* Start a round of blackjack */
        cd_player_hand[0] = get_random_card();
        cd_player_hand[1] = get_random_card();
        cd_player_count = 2;

        cd_dealer_hand[0] = get_random_card();
        cd_dealer_hand[1] = get_random_card();
        cd_dealer_count = 2;

        cd_state = 1;
        snprintf(cd_msg, sizeof(cd_msg), "%s",
                 is_it ? "BLACKJACK: DIGITA 'CARTA' (C), 'STAI' (S) O 'ESCI'." :
                         "BLACKJACK: TYPE 'HIT' (H), 'STAND' (S), OR 'QUIT'.");
    } else {
        /* Start 5-card draw poker */
        for (int i = 0; i < 5; ++i) {
            cd_player_hand[i] = get_random_card();
            cd_dealer_hand[i] = get_random_card();
        }
        cd_player_count = 5;
        cd_dealer_count = 5;
        cd_state = 1;
        snprintf(cd_msg, sizeof(cd_msg), "%s",
                 is_it ? "POKER: INDICA QUALI CARTE CAMBIARE (ES: 1 3 5), 'STAI' O 'ESCI'." :
                         "POKER: ENTER CARDS TO DISCARD (EX: 1 3 5), 'HOLD', OR 'QUIT'.");
    }
}

void ext_cards_submit(App *a, const char *input) {
    int is_it = app_get_lang(a);
    char cmd[64];
    size_t len = 0;
    for (size_t i = 0; input[i] && len + 1 < sizeof(cmd); ++i) {
        cmd[len++] = (char)tolower((unsigned char)input[i]);
    }
    cmd[len] = '\0';

    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "esci") == 0 || strcmp(cmd, "quit") == 0) {
        app_start_stage(a, STAGE_LIST_GAMES, "");
        return;
    }

    if (!cd_is_poker) {
        /* Blackjack Logic */
        if (cd_state == 2) {
            /* Start new hand */
            ext_cards_init(a, 0);
            return;
        }
        if (strcmp(cmd, "h") == 0 || strcmp(cmd, "hit") == 0 || strcmp(cmd, "carta") == 0 || strcmp(cmd, "c") == 0) {
            if (cd_player_count < 5) {
                cd_player_hand[cd_player_count++] = get_random_card();
                app_play_sound(a, "Warcursor");
                int val = blackjack_eval(cd_player_hand, cd_player_count);
                if (val > 21) {
                    cd_chips -= 50;
                    cd_state = 2;
                    app_play_sound(a, "Waracdnd");
                    snprintf(cd_msg, sizeof(cd_msg), "%s",
                             is_it ? "HAI SBALLATO! W.O.P.R. VINCE LA MANO. [INVIO PER NUOVA MANO]" :
                                     "YOU BUSTED! W.O.P.R. WINS THE HAND. [ENTER FOR NEW HAND]");
                    return;
                }
            }
        } else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "stand") == 0 || strcmp(cmd, "stai") == 0) {
            /* Dealer resolves */
            while (blackjack_eval(cd_dealer_hand, cd_dealer_count) < 17 && cd_dealer_count < 5) {
                cd_dealer_hand[cd_dealer_count++] = get_random_card();
            }
            int p_val = blackjack_eval(cd_player_hand, cd_player_count);
            int d_val = blackjack_eval(cd_dealer_hand, cd_dealer_count);
            cd_state = 2;

            if (d_val > 21 || p_val > d_val) {
                cd_chips += 50;
                app_play_sound(a, "Warxor0");
                snprintf(cd_msg, sizeof(cd_msg), "%s",
                         is_it ? "HAI BATTUTO IL BANCO DI W.O.P.R.! [INVIO PER NUOVA MANO]" :
                                 "YOU BEAT W.O.P.R.'S DEALER! [ENTER FOR NEW HAND]");
            } else if (d_val == p_val) {
                snprintf(cd_msg, sizeof(cd_msg), "%s",
                         is_it ? "PAREGGIO (PUSH)! CHIPS INVARIATE. [INVIO PER NUOVA MANO]" :
                                 "PUSH! CHIPS UNCHANGED. [ENTER FOR NEW HAND]");
            } else {
                cd_chips -= 50;
                app_play_sound(a, "Waracdnd");
                snprintf(cd_msg, sizeof(cd_msg), "%s",
                         is_it ? "W.O.P.R. VINCE IL BANCO. [INVIO PER NUOVA MANO]" :
                                 "W.O.P.R. WINS THE HAND. [ENTER FOR NEW HAND]");
            }
        }
    } else {
        /* Poker Logic */
        if (cd_state == 2) {
            ext_cards_init(a, 1);
            return;
        }
        /* Discard card indices (1-5) or 'stai' / 'hold' */
        if (strcmp(cmd, "stai") != 0 && strcmp(cmd, "hold") != 0 && strcmp(cmd, "none") != 0 && strcmp(cmd, "nessuna") != 0) {
            for (size_t i = 0; cmd[i]; ++i) {
                if (cmd[i] >= '1' && cmd[i] <= '5') {
                    int idx = cmd[i] - '1';
                    cd_player_hand[idx] = get_random_card();
                }
            }
        }
        /* Dealer swaps up to 2 cards randomly */
        for (int i = 0; i < 2; ++i) {
            cd_dealer_hand[rand() % 5] = get_random_card();
        }

        cd_state = 2;
        int p_high = 0, d_high = 0;
        for (int i = 0; i < 5; ++i) {
            if (cd_player_hand[i].rank > p_high) p_high = cd_player_hand[i].rank;
            if (cd_dealer_hand[i].rank > d_high) d_high = cd_dealer_hand[i].rank;
        }

        if (p_high >= d_high) {
            cd_chips += 100;
            app_play_sound(a, "Warxor0");
            snprintf(cd_msg, sizeof(cd_msg), "%s",
                     is_it ? "SHOWDOWN: HAI VINTO IL PIATTO CONTRO JOSHUA! [INVIO PER NUOVA MANO]" :
                             "SHOWDOWN: YOU WON THE POT AGAINST JOSHUA! [ENTER FOR NEW HAND]");
        } else {
            cd_chips -= 100;
            app_play_sound(a, "Waracdnd");
            snprintf(cd_msg, sizeof(cd_msg), "%s",
                     is_it ? "SHOWDOWN: JOSHUA HA LA CARTA PIU' ALTA! [INVIO PER NUOVA MANO]" :
                             "SHOWDOWN: JOSHUA WINS THE POT! [ENTER FOR NEW HAND]");
        }
    }
}

static void format_card(char *dst, size_t cap, Card c) {
    static const char *ranks[] = {"?", "?", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
    static const char suits[] = "SHDC";
    snprintf(dst, cap, "[%2s%c]", ranks[c.rank], suits[c.suit]);
}

void ext_cards_render(App *a) {
    SDL_Color cyan = {131, 237, 240, 255};
    SDL_Color amber = {255, 191, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {105, 255, 184, 255};
    int is_it = app_get_lang(a);

    app_draw_text(a, "================================================================================", 8, 12, cyan);
    app_draw_text(a, cd_is_poker ? "W.O.P.R. STRATEGIC CASINO - 5-CARD DRAW POKER" :
                                   "W.O.P.R. STRATEGIC CASINO - LAS VEGAS BLACKJACK", 16, 28, amber);
    app_draw_text(a, "================================================================================", 8, 44, cyan);

    char chip_str[64];
    snprintf(chip_str, sizeof(chip_str), is_it ? "CHIPS DISPONIBILI: $ %d" : "AVAILABLE CHIPS: $ %d", cd_chips);
    app_draw_text(a, chip_str, 500, 28, green);

    int y = 80;
    /* Dealer hand */
    app_draw_text(a, "--- W.O.P.R. (DEALER) ---", 16, y, amber); y += 22;
    char d_line[128] = {0};
    for (int i = 0; i < cd_dealer_count; ++i) {
        char c_buf[16];
        if (cd_state == 1 && i > 0 && !cd_is_poker) {
            snprintf(c_buf, sizeof(c_buf), "[ ?? ] ");
        } else if (cd_state == 1 && cd_is_poker) {
            snprintf(c_buf, sizeof(c_buf), "[ ?? ] ");
        } else {
            format_card(c_buf, sizeof(c_buf), cd_dealer_hand[i]);
            strcat(c_buf, " ");
        }
        strcat(d_line, c_buf);
    }
    app_draw_text(a, d_line, 32, y, cyan); y += 35;

    /* Player hand */
    app_draw_text(a, is_it ? "--- LA TUA MANO (DAVID LIGHTMAN) ---" : "--- YOUR HAND (DAVID LIGHTMAN) ---", 16, y, green); y += 22;
    char p_line[128] = {0};
    for (int i = 0; i < cd_player_count; ++i) {
        char c_buf[16];
        format_card(c_buf, sizeof(c_buf), cd_player_hand[i]);
        strcat(c_buf, " ");
        strcat(p_line, c_buf);
    }
    app_draw_text(a, p_line, 32, y, white); y += 25;

    if (!cd_is_poker) {
        char val_str[64];
        snprintf(val_str, sizeof(val_str), is_it ? "VALORE TOTALE MANO: %d" : "TOTAL HAND VALUE: %d",
                 blackjack_eval(cd_player_hand, cd_player_count));
        app_draw_text(a, val_str, 32, y, cyan); y += 30;
    } else {
        y += 20;
    }

    /* Message */
    app_draw_text(a, cd_msg, 16, y, amber); y += 35;

    /* Prompt */
    const char *inp = app_get_input(a);
    char prompt[128];
    snprintf(prompt, sizeof(prompt), is_it ? "COMANDO [ESCI]: %s" : "COMMAND [QUIT]: %s", inp);
    app_draw_text(a, prompt, 16, y, white);
    app_draw_block_cursor(a, 16 + (int)strlen(prompt) * 8, y, cyan);
}
