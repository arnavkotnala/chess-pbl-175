/*
 * ═══════════════════════════════════════════════════════════════════════════════
 *                         ♚ CHESSVERSE ♚
 *                  Pure C/C++ Console Application
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Architecture:
 *   - DSA (Stack, Queue, LinkedList, Sort) → Pure C
 *   - Login/Auth/Store (public/private OOP)  → C++
 *   - Chess Engine & AI                      → C++
 *   - Console UI & Game Loop                 → C++
 *
 * Build: g++ -o ChessVerse.exe main.cpp chess/board.cpp dsa/stack.c dsa/queue.c
 *        dsa/linkedlist.c dsa/sort.c -std=c++17
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define CLEAR_CMD "cls"
#else
#include <sys/stat.h>
#define CLEAR_CMD "clear"
#endif

// C DSA modules
extern "C" {
    #include "dsa/stack.h"
    #include "dsa/queue.h"
    #include "dsa/linkedlist.h"
    #include "dsa/sort.h"
}

// C++ modules
#include "chess/board.h"
#include "chess/ai.h"
#include "chess/hint.h"

// ─── Console Colors (Windows) ─────────────────────────────────────────────────
#ifdef _WIN32
static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void setColor(int color) { SetConsoleTextAttribute(hConsole, color); }
void resetColor()        { SetConsoleTextAttribute(hConsole, 7); }

// Color codes
#define CLR_TITLE    11  // Bright Cyan
#define CLR_MENU     14  // Yellow
#define CLR_SUCCESS  10  // Green
#define CLR_ERROR    12  // Red
#define CLR_INFO      9  // Blue
#define CLR_GOLD     14  // Yellow
#define CLR_PROMPT   15  // White
#define CLR_DIM       8  // Dark Gray
#define CLR_WHITE_PC 15  // White pieces
#define CLR_BLACK_PC 13  // Magenta for black pieces
#define CLR_BOARD_L   6  // Dark Cyan (light squares)
#define CLR_BOARD_D   2  // Dark Green (dark squares)
#define CLR_HIGHLIGHT 14 // Yellow (highlighted moves)
#else
void setColor(int color) {}
void resetColor()        {}
#define CLR_TITLE    0
#define CLR_MENU     0
#define CLR_SUCCESS  0
#define CLR_ERROR    0
#define CLR_INFO     0
#define CLR_GOLD     0
#define CLR_PROMPT   0
#define CLR_DIM      0
#define CLR_WHITE_PC 0
#define CLR_BLACK_PC 0
#define CLR_BOARD_L  0
#define CLR_BOARD_D  0
#define CLR_HIGHLIGHT 0
#endif

// ─── Global State ─────────────────────────────────────────────────────────────
Board         g_board;
AI*           g_ai        = nullptr;
Queue*        g_moveQueue = nullptr;   // C Queue DSA for move log
Difficulty    g_difficulty = EASY;
bool          g_vsAI       = true;
Color         g_playerColor = WHITE;
HintLimit     g_hint1(-1);
HintLimit     g_hint2(-1);
bool          g_gameActive  = false;
int           g_whiteScore  = 0;
int           g_blackScore  = 0;

// ─── Utility Functions ────────────────────────────────────────────────────────
void clearScreen() { system(CLEAR_CMD); }

void pressEnter() {
    setColor(CLR_DIM);
    printf("\n  Press Enter to continue...");
    resetColor();
    char dummy[8];
    if (fgets(dummy, 8, stdin)) {}
}

void readLine(char* buf, int maxLen) {
    if (fgets(buf, maxLen, stdin)) {
        int len = (int)strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    }
}

void printHeader() {
    setColor(CLR_TITLE);
    printf("\n");
    printf("  ══════════════════════════════════════════\n");
    printf("             ♚  C H E S S V E R S E  ♚     \n");
    printf("          Pure C/C++ Console Edition        \n");
    printf("  ══════════════════════════════════════════\n");
    resetColor();
}

void printDivider() {
    setColor(CLR_DIM);
    printf("  ──────────────────────────────────────────\n");
    resetColor();
}

// ─── Enhanced Board Print ─────────────────────────────────────────────────────
void printBoardColored(const Board& board, const MoveList* highlights = nullptr) {
    static const char* pieceChars[] = {".", "P", "N", "B", "R", "Q", "K"};

    printf("\n");
    setColor(CLR_DIM);
    printf("       a    b    c    d    e    f    g    h\n");
    printf("     +----+----+----+----+----+----+----+----+\n");
    resetColor();

    for (int r = 7; r >= 0; r--) {
        setColor(CLR_DIM);
        printf("  %d  |", r + 1);
        resetColor();

        for (int c = 0; c < 8; c++) {
            const Cell& cell = board.at(r, c);

            // Check if this square is a highlighted move target
            bool isHighlighted = false;
            if (highlights) {
                for (int h = 0; h < highlights->count; h++) {
                    if (highlights->moves[h].toRow == r && highlights->moves[h].toCol == c) {
                        isHighlighted = true;
                        break;
                    }
                }
            }

            if (cell.isEmpty()) {
                if (isHighlighted) {
                    setColor(CLR_HIGHLIGHT);
                    printf(" ** ");
                } else {
                    setColor(CLR_DIM);
                    printf("    ");
                }
            } else {
                if (cell.color == WHITE) setColor(CLR_WHITE_PC);
                else                     setColor(CLR_BLACK_PC);

                char prefix = (cell.color == WHITE) ? 'w' : 'b';
                printf(" %c%s ", prefix, pieceChars[cell.type]);
            }
            setColor(CLR_DIM);
            printf("|");
        }
        resetColor();
        printf("  %d\n", r + 1);

        setColor(CLR_DIM);
        printf("     +----+----+----+----+----+----+----+----+\n");
        resetColor();
    }

    setColor(CLR_DIM);
    printf("       a    b    c    d    e    f    g    h\n");
    resetColor();
}

// ─── Move Queue Helpers ───────────────────────────────────────────────────────
void initMoveQueue() {
    if (g_moveQueue) queue_destroy(g_moveQueue);
    g_moveQueue = queue_create(256);
}

void recordMove(const Move& m, bool isWhite) {
    static int moveNum = 1;
    char notation[QUEUE_STR_LEN];
    char alg[8];
    m.toAlgebraic(alg);

    if (isWhite) {
        snprintf(notation, QUEUE_STR_LEN, "%d. %s", moveNum, alg);
    } else {
        snprintf(notation, QUEUE_STR_LEN, "%s", alg);
        moveNum++;
    }
    queue_enqueue(g_moveQueue, notation);
}

void resetMoveNum() {
    // Called when new game starts — reset move numbering
}

void printMoveLog() {
    if (!g_moveQueue || queue_is_empty(g_moveQueue)) {
        setColor(CLR_DIM);
        printf("  No moves recorded.\n");
        resetColor();
        return;
    }

    setColor(CLR_INFO);
    printf("  Move Log:\n");
    resetColor();

    int count = queue_size(g_moveQueue);
    for (int i = 0; i < count; i++) {
        const char* entry = queue_get_at(g_moveQueue, i);
        if (entry) {
            if (i % 2 == 0) {
                setColor(CLR_WHITE_PC);
            } else {
                setColor(CLR_BLACK_PC);
            }
            printf("  %s ", entry);
        }
    }
    resetColor();
    printf("\n");
}

// ─── Parse Move Input ─────────────────────────────────────────────────────────
bool parseAlgebraic(const char* input, int& fromRow, int& fromCol, int& toRow, int& toCol) {
    // Expected format: "e2e4" or "e2 e4"
    char clean[16];
    int ci = 0;
    for (int i = 0; input[i] && ci < 15; i++) {
        if (input[i] != ' ') clean[ci++] = (char)tolower(input[i]);
    }
    clean[ci] = '\0';

    if (ci < 4) return false;

    fromCol = clean[0] - 'a';
    fromRow = clean[1] - '1';
    toCol   = clean[2] - 'a';
    toRow   = clean[3] - '1';

    return (fromCol >= 0 && fromCol < 8 && fromRow >= 0 && fromRow < 8 &&
            toCol >= 0 && toCol < 8 && toRow >= 0 && toRow < 8);
}

// ═══════════════════════════════════════════════════════════════════════════════
//                           MENU FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

// ─── Game Loop (the core chess game) ──────────────────────────────────────────────────────────────────────────────────
void gameLoop() {
    g_gameActive = true;

    while (g_gameActive) {
        clearScreen();

        // Header
        setColor(CLR_TITLE);
        printf("  ♚ CHESSVERSE — ");
        if (g_vsAI) {
            const char* diffNames[] = {"Easy", "Medium", "Hard"};
            printf("vs AI (%s)", diffNames[g_difficulty]);
        } else {
            printf("Player vs Player");
        }
        printf(" ♚\n");
        resetColor();
        printDivider();

        // Scores
        setColor(CLR_WHITE_PC);
        printf("  White: %d pts", g_whiteScore);
        resetColor();
        printf("  |  ");
        setColor(CLR_BLACK_PC);
        printf("Black: %d pts\n", g_blackScore);
        resetColor();

        // Turn indicator
        Color turn = g_board.state.currentTurn;
        if (turn == WHITE) setColor(CLR_WHITE_PC);
        else               setColor(CLR_BLACK_PC);
        printf("  Turn: %s\n", turn == WHITE ? "WHITE" : "BLACK");
        resetColor();

        // Check/Checkmate/Stalemate status
        if (g_board.isCheckmate(turn)) {
            printBoardColored(g_board);
            setColor(CLR_ERROR);
            printf("\n  ═══ CHECKMATE! %s wins! ═══\n", turn == WHITE ? "BLACK" : "WHITE");
            resetColor();
            g_gameActive = false;
            pressEnter();
            return;
        }
        if (g_board.isStalemate(turn)) {
            printBoardColored(g_board);
            setColor(CLR_INFO);
            printf("\n  ═══ STALEMATE! Draw! ═══\n");
            resetColor();
            g_gameActive = false;
            pressEnter();
            return;
        }
        if (g_board.isInCheck(turn)) {
            setColor(CLR_ERROR);
            printf("  ⚠ %s is in CHECK!\n", turn == WHITE ? "WHITE" : "BLACK");
            resetColor();
        }

        // Print board
        printBoardColored(g_board);
        printMoveLog();

        // If AI's turn, compute automatically
        if (g_vsAI && turn != g_playerColor) {
            setColor(CLR_INFO);
            printf("\n  AI is thinking...\n");
            resetColor();

            Move aiMove = g_ai->getBestMove(g_board, turn);
            if (aiMove.fromRow < 0) {
                setColor(CLR_ERROR);
                printf("  AI has no moves!\n");
                resetColor();
                g_gameActive = false;
                pressEnter();
                return;
            }

            // Score AI capture
            if (aiMove.isCapture && !g_board.at(aiMove.toRow, aiMove.toCol).isEmpty()) {
                int pts = piecePoints(g_board.at(aiMove.toRow, aiMove.toCol).type);
                pts = (int)(pts * g_ai->multiplier());
                if (turn == WHITE) g_whiteScore += pts;
                else               g_blackScore += pts;
            }

            g_board.applyMove(aiMove);
            recordMove(aiMove, turn == WHITE);

            char alg[8];
            aiMove.toAlgebraic(alg);
            setColor(CLR_BLACK_PC);
            printf("  AI plays: %s\n", alg);
            resetColor();

#ifdef _WIN32
            Sleep(500);
#endif
            continue;
        }

        // Player's turn — show commands
        printDivider();
        setColor(CLR_MENU);
        printf("  Commands: [move e2e4] [hint] [undo] [resign] [quit]\n");
        resetColor();

        setColor(CLR_PROMPT);
        printf("  > ");
        resetColor();

        char input[64];
        readLine(input, 64);

        // Parse command
        if (strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {
            g_gameActive = false;
            return;
        }
        else if (strcmp(input, "resign") == 0) {
            setColor(CLR_ERROR);
            printf("\n  You resigned. %s wins!\n", turn == WHITE ? "BLACK" : "WHITE");
            resetColor();
            g_gameActive = false;
            pressEnter();
            return;
        }
        else if (strcmp(input, "hint") == 0) {
            HintLimit& limit = (turn == WHITE) ? g_hint1 : g_hint2;
            Move hint = HintEngine::getHint(g_board, turn, limit);
            if (hint.fromRow < 0) {
                setColor(CLR_ERROR);
                printf("  No hints remaining!\n");
            } else {
                char alg[8];
                hint.toAlgebraic(alg);
                setColor(CLR_GOLD);
                printf("  Hint: %s", alg);
                char label[16];
                HintEngine::hintLabel(limit, label);
                printf("  (remaining: %s)\n", label);
            }
            resetColor();
            pressEnter();
            continue;
        }
        else if (strcmp(input, "undo") == 0) {
            if (stack_is_empty(g_board.history)) {
                setColor(CLR_ERROR);
                printf("  Nothing to undo!\n");
                resetColor();
            } else {
                g_board.undoMove();
                queue_remove_last(g_moveQueue);
                if (g_vsAI && !stack_is_empty(g_board.history)) {
                    g_board.undoMove();
                    queue_remove_last(g_moveQueue);
                }
                setColor(CLR_SUCCESS);
                printf("  Move undone!\n");
                resetColor();
            }
            pressEnter();
            continue;
        }
        else {
            // Try to parse as move
            int fr, fc, tr, tc;
            if (parseAlgebraic(input, fr, fc, tr, tc)) {
                MoveList legal = g_board.getLegalMoves(turn);
                Move chosen; chosen.fromRow = -1;

                for (int i = 0; i < legal.count; i++) {
                    Move& m = legal.moves[i];
                    if (m.fromRow == fr && m.fromCol == fc && m.toRow == tr && m.toCol == tc) {
                        if (m.isPromotion) {
                            // Ask for promotion piece
                            setColor(CLR_MENU);
                            printf("  Promote to? [q]ueen [r]ook [b]ishop [n]knight: ");
                            resetColor();
                            char promInput[8];
                            readLine(promInput, 8);
                            PieceType promType = QUEEN;
                            if (promInput[0] == 'r') promType = ROOK;
                            else if (promInput[0] == 'b') promType = BISHOP;
                            else if (promInput[0] == 'n') promType = KNIGHT;

                            // Find matching promotion move
                            for (int j = 0; j < legal.count; j++) {
                                if (legal.moves[j].fromRow == fr && legal.moves[j].fromCol == fc &&
                                    legal.moves[j].toRow == tr && legal.moves[j].toCol == tc &&
                                    legal.moves[j].isPromotion && legal.moves[j].promoteTo == promType) {
                                    chosen = legal.moves[j];
                                    break;
                                }
                            }
                            break;
                        } else {
                            chosen = m;
                            break;
                        }
                    }
                }

                if (chosen.fromRow < 0) {
                    setColor(CLR_ERROR);
                    printf("  Illegal move!\n");
                    resetColor();
                    pressEnter();
                    continue;
                }

                // Score capture
                if (chosen.isCapture && !g_board.at(tr, tc).isEmpty()) {
                    int pts = piecePoints(g_board.at(tr, tc).type);
                    if (g_ai) pts = (int)(pts * g_ai->multiplier());
                    if (turn == WHITE) g_whiteScore += pts;
                    else               g_blackScore += pts;
                }

                g_board.applyMove(chosen);
                recordMove(chosen, turn == WHITE);
            } else {
                setColor(CLR_ERROR);
                printf("  Invalid input! Enter move as e2e4 or type a command.\n");
                resetColor();
                pressEnter();
            }
        }
    }
}

// ─── New Game Menu ────────────────────────────────────────────────────────────
void menuNewGame() {
    clearScreen();
    printHeader();
    printDivider();

    setColor(CLR_INFO);
    printf("  NEW GAME\n");
    resetColor();
    printDivider();

    setColor(CLR_MENU);
    printf("  1. Easy   (Random AI)\n");
    printf("  2. Medium (AI Depth 3)\n");
    printf("  3. Hard   (AI Depth 5)\n");
    printf("  4. Player vs Player (local)\n");
    printf("  0. Back\n");
    resetColor();
    printDivider();

    setColor(CLR_PROMPT);
    printf("  > ");
    resetColor();

    char choice[8];
    readLine(choice, 8);

    switch (choice[0]) {
        case '1': g_difficulty = EASY;   g_hint1 = HintLimit(-1); g_hint2 = HintLimit(-1); g_vsAI = true; break;
        case '2': g_difficulty = MEDIUM; g_hint1 = HintLimit(3);  g_hint2 = HintLimit(3);  g_vsAI = true; break;
        case '3': g_difficulty = HARD;   g_hint1 = HintLimit(0);  g_hint2 = HintLimit(0);  g_vsAI = true; break;
        case '4': g_difficulty = MEDIUM; g_vsAI = false;
                  g_hint1 = HintLimit(3); g_hint2 = HintLimit(3); break;
        default: return;
    }

    if (g_vsAI) {
        setColor(CLR_MENU);
        printf("\n  Play as: 1. White  2. Black\n");
        printf("  > ");
        resetColor();
        char colorChoice[8];
        readLine(colorChoice, 8);
        g_playerColor = (colorChoice[0] == '2') ? BLACK : WHITE;
    }

    // Initialize game
    if (g_ai) delete g_ai;
    g_ai = new AI(g_difficulty);
    g_board.init();
    initMoveQueue();
    g_whiteScore = g_blackScore = 0;

    gameLoop();
}

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    srand((unsigned)time(NULL));
    g_board.init();
    initMoveQueue();

    while (true) {
        clearScreen();
        printHeader();
        printDivider();

        setColor(CLR_MENU);
        printf("  1. Play Chess (vs AI)
");
        printf("  2. Play Chess (vs Friend)
");
        printf("  0. Exit
");
        resetColor();
        printDivider();

        setColor(CLR_PROMPT);
        printf("  > ");
        resetColor();

        char choice[8];
        readLine(choice, 8);

        switch (choice[0]) {
            case '1': {
                menuNewGame();
                break;
            }
            case '2': {
                g_vsAI = false;
                g_difficulty = MEDIUM;
                g_hint1 = HintLimit(3); g_hint2 = HintLimit(3);
                if (g_ai) delete g_ai;
                g_ai = new AI(g_difficulty);
                g_board.init();
                initMoveQueue();
                g_whiteScore = g_blackScore = 0;
                gameLoop();
                break;
            }
            case '0':
                setColor(CLR_DIM);
                printf("  Thanks for playing ChessVerse!
");
                resetColor();
                if (g_moveQueue) queue_destroy(g_moveQueue);
                if (g_ai) delete g_ai;
                return 0;
        }
    }
    return 0;
}
