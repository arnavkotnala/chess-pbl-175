/*
 * ═══════════════════════════════════════════════════════════════════════════════
 *                         ♚ CHESSVERSE 2D ♚
 *                  Raylib Graphical Application
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Architecture:
 *   - DSA (Stack, Queue, LinkedList, Sort) → Pure C
 *   - Login/Auth/Store/Trivia (OOP)        → C++
 *   - Chess Engine & AI                    → C++
 *   - Graphical UI                         → C++ & Raylib
 */


#include "raylib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <thread>
#include <atomic>
#include <cmath>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
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
#include "auth/auth.h"
#include "trivia/trivia.h"

// ─── Constants ────────────────────────────────────────────────────────────────
int SCREEN_WIDTH  = 1024;
int SCREEN_HEIGHT = 768;
const int BOARD_SIZE    = 600;
const int CELL_SIZE     = BOARD_SIZE / 8;
const int NOTER_WIDTH   = 280;  // Move noter panel width
int BOARD_X       = 40;        // Board pinned to left area (noter panel on right)
int BOARD_Y       = (SCREEN_HEIGHT - BOARD_SIZE) / 2;

// ─── Chess Theme Colors ──────────────────────────────────────────────────────
Color COL_BG_DARK      = { 30,  30,  35,  255 };  // Deep dark background
Color COL_BG_DARKER    = { 22,  22,  26,  255 };  // Even darker panels
Color COL_BG_CARD      = { 42,  42,  50,  255 };  // Card backgrounds
Color COL_BG_CARD_HOV  = { 55,  55,  65,  255 };  // Card hover
Color COL_ACCENT       = { 200, 170, 110, 255 };  // Gold accent
Color COL_ACCENT_DIM   = { 150, 125, 80,  255 };  // Dimmer gold
Color COL_TEXT_LIGHT   = { 230, 225, 215, 255 };  // Light text
Color COL_TEXT_DIM     = { 150, 145, 135, 255 };  // Dimmed text
Color COL_TEXT_BRIGHT  = { 255, 250, 240, 255 };  // Bright white text
Color COL_BTN          = { 60,  58,  70,  255 };  // Button background
Color COL_BTN_HOV      = { 80,  75,  95,  255 };  // Button hover
Color COL_BTN_ACCENT   = { 180, 145, 80,  255 };  // Gold button
Color COL_BTN_ACC_HOV  = { 210, 175, 100, 255 };  // Gold button hover
Color COL_DANGER       = { 200, 70,  70,  255 };  // Red for errors/resign
Color COL_SUCCESS      = { 70,  180, 100, 255 };  // Green for success
Color COL_PANEL_BG     = { 35,  35,  42,  255 };  // Side panel background
Color COL_INPUT_BG     = { 50,  48,  58,  255 };  // Input field bg
Color COL_INPUT_ACTIVE = { 65,  63,  78,  255 };  // Active input field bg
Color COL_SEPARATOR    = { 70,  68,  80,  255 };  // Separator lines

// ─── Application State ────────────────────────────────────────────────────────
enum AppState { STATE_LOGIN, STATE_MAIN_MENU, STATE_GAME, STATE_LEADERBOARD, STATE_TRIVIA, STATE_P2_LOGIN, STATE_PROFILE_PIC, STATE_ONLINE_MENU };
AppState g_appState = STATE_LOGIN;

// ─── Global Objects ───────────────────────────────────────────────────────────
Board         g_board;
AI*           g_ai        = nullptr;
AuthManager   g_auth;
TriviaManager g_trivia;

// Login Buffers
char g_usernameInput[32] = "";
char g_passwordInput[32] = "";
bool g_usernameActive = true;
char g_loginMsg[128] = "";
bool g_showPassword = false;        // Password visibility toggle

// Trivia Buffer
int g_triviaSelected = -1;
char g_triviaMsg[128] = "";

// Player 2 Buffer & Ranked PVP
char g_p2UsernameInput[32] = "";
char g_p2PasswordInput[32] = "";
bool g_p2UsernameActive = true;
char g_p2LoginMsg[128] = "";
User* g_player2 = nullptr;
bool g_rankedPvp = false;
bool g_showP2Password = false;      // P2 password visibility toggle

Difficulty    g_difficulty = EASY;
bool          g_vsAI       = true;
PieceColor    g_playerColor = P_WHITE;

bool g_isOnline = false;
bool g_myTurnOnline = false;
char g_onlineRoomCode[16] = "";
char g_onlineJoinCode[16] = "";
bool g_onlineJoinActive = false;
char g_onlineMsg[128] = "";

enum OnlineMenuState {
    ONLINE_MENU_DEFAULT   = 0,
    ONLINE_MENU_WAITING   = 1,
    ONLINE_MENU_CONNECTING = 2,
    ONLINE_MENU_HINT_SETUP = 3   // host picks hint allowance before creating room
};
OnlineMenuState g_onlineMenuState = ONLINE_MENU_DEFAULT;
bool g_copiedRoomCode = false;
float g_copiedRoomTimer = 0.0f;
int  g_onlineHintCount = 3;  // -1=unlimited, 0=none, 3 or 5

bool          g_gameOver   = false;
char          g_statusMsg[128] = "";

int selectedRow = -1;
int selectedCol = -1;
int hintDestRow = -1;
int hintDestCol = -1;

// Move Noter / Prev-Next viewer state
int  g_viewingMoveIdx = -1;    // -1 = live board, 0..N = viewing snapshot index
int  g_noterScrollOffset = 0;  // Scroll offset for the move noter panel

// Copy feedback
bool g_showCopied = false;
float g_copiedTimer = 0.0f;
HintLimit     g_hintLimit;

std::atomic<bool> g_aiIsThinking(false);
std::atomic<bool> g_aiMoveReady(false);
Move g_aiCalculatedMove;

// ─── Profile Picture Names ───────────────────────────────────────────────────
const char* PFP_NAMES[] = {
    "Knight",     // 0
    "Bishop",     // 1
    "Rook",       // 2
    "Queen",      // 3
    "King",       // 4
    "Pawn",       // 5
    "Dragon",     // 6
    "Castle",     // 7
    "Sword",      // 8
    "Crown"       // 9
};

// Profile picture colors (two-tone for each avatar)
Color PFP_PRIMARY[] = {
    {180, 140, 90,  255},  // Knight - Gold
    {130, 100, 200, 255},  // Bishop - Purple
    {150, 80,  80,  255},  // Rook - Crimson
    {200, 170, 110, 255},  // Queen - Bright Gold
    {220, 190, 120, 255},  // King - Royal Gold
    {100, 140, 100, 255},  // Pawn - Green
    {180, 60,  60,  255},  // Dragon - Red
    {120, 130, 150, 255},  // Castle - Silver
    {170, 170, 180, 255},  // Sword - Steel
    {230, 200, 80,  255},  // Crown - Yellow Gold
};

Color PFP_SECONDARY[] = {
    {120, 90,  50,  255},  // Knight
    {80,  60,  140, 255},  // Bishop
    {100, 50,  50,  255},  // Rook
    {150, 120, 70,  255},  // Queen
    {170, 140, 80,  255},  // King
    {60,  100, 60,  255},  // Pawn
    {130, 30,  30,  255},  // Dragon
    {80,  90,  110, 255},  // Castle
    {120, 120, 130, 255},  // Sword
    {180, 150, 40,  255},  // Crown
};

// ─── Graphics Assets ──────────────────────────────────────────────────────────
Texture2D texPieces[3][7]; // [color][type]
Texture2D texShadows[7];   // [type]: 1=pawn, 2=knight, 3=bishop, 4=rook, 5=queen, 6=king
Texture2D texLines[7];     // [type]: glowing gold engraved linework


extern "C" {
    // hint_count: -1=unlimited, 0=none, 3 or 5
    void onRoomCreated(const char* roomId, int hint_count) {
        snprintf(g_onlineRoomCode, sizeof(g_onlineRoomCode), "%s", roomId);
#ifdef __EMSCRIPTEN__
        EM_ASM({
            window._currentRoomCode = UTF8ToString($0);
        }, roomId);
#endif
        snprintf(g_onlineMsg, sizeof(g_onlineMsg), "Room created! Hints: %s",
            hint_count < 0 ? "Unlimited" : (hint_count == 0 ? "None" :
            (hint_count == 5 ? "5 each" : "3 each")));
        g_hintLimit = HintLimit(hint_count);
        g_onlineMenuState = ONLINE_MENU_WAITING;
        g_copiedRoomCode = false;
    }
    void onRoomJoined(const char* colorStr, const char* roomId, int hint_count) {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            window._currentRoomCode = "";
        });
#endif
        if (roomId && strlen(roomId) > 0) {
            snprintf(g_onlineRoomCode, sizeof(g_onlineRoomCode), "%s", roomId);
        }
        selectedRow = -1;
        selectedCol = -1;
        g_hintLimit = HintLimit(hint_count);
        hintDestRow = -1; hintDestCol = -1;
        g_isOnline = true;
        g_vsAI = false;
        g_appState = STATE_GAME;
        g_onlineMenuState = ONLINE_MENU_DEFAULT;
        g_board.init();
        g_gameOver = false;
        
        if (strcmp(colorStr, "white") == 0) {
            g_playerColor = P_WHITE;
            g_myTurnOnline = true;
        } else {
            g_playerColor = P_BLACK;
            g_myTurnOnline = false;
        }
        snprintf(g_statusMsg, sizeof(g_statusMsg), "Game started! You play as %s.", (g_playerColor == P_WHITE) ? "White" : "Black");
    }
    void onOpponentJoined(int hint_count) {
        selectedRow = -1;
        selectedCol = -1;
        g_hintLimit = HintLimit(hint_count);
        hintDestRow = -1; hintDestCol = -1;
        g_isOnline = true;
        g_vsAI = false;
        g_appState = STATE_GAME;
        g_onlineMenuState = ONLINE_MENU_DEFAULT;
        g_board.init();
        g_gameOver = false;
        g_playerColor = P_WHITE;
        g_myTurnOnline = true;
        snprintf(g_statusMsg, sizeof(g_statusMsg), "Opponent connected! Your turn (White).");
    }
    void onOpponentDisconnected() {
        if (g_isOnline) {
            g_gameOver = true;
            snprintf(g_statusMsg, sizeof(g_statusMsg), "Opponent disconnected. You win!");
        }
    }
    void onOpponentResigned() {
        if (g_isOnline) {
            g_gameOver = true;
            snprintf(g_statusMsg, sizeof(g_statusMsg), "Opponent resigned! YOU WIN!");
        }
    }
    void applyOpponentMove(int fromRow, int fromCol, int toRow, int toCol, int promoteTo) {
        selectedRow = -1;
        selectedCol = -1;
        g_viewingMoveIdx = -1;
        MoveList legal = g_board.getLegalMoves(g_board.state.currentTurn);
        for(int i = 0; i < legal.count; i++) {
            if(legal.moves[i].fromRow == fromRow && legal.moves[i].fromCol == fromCol &&
               legal.moves[i].toRow == toRow && legal.moves[i].toCol == toCol) {
                if (legal.moves[i].isPromotion && promoteTo > 0 && legal.moves[i].promoteTo != (PieceType)promoteTo) {
                    continue;
                }
                Move m = legal.moves[i];
                if (m.isPromotion && promoteTo > 0) {
                    m.promoteTo = (PieceType)promoteTo;
                }
                g_board.applyMove(m);
                g_myTurnOnline = true;
                
                PieceColor nextTurn = g_board.state.currentTurn;
                if (g_board.isCheckmate(nextTurn)) { 
                    g_gameOver = true;
                    snprintf(g_statusMsg, sizeof(g_statusMsg), "CHECKMATE! You Lost!");
                }
                else if (g_board.isStalemate(nextTurn)) { g_gameOver = true; snprintf(g_statusMsg, sizeof(g_statusMsg), "STALEMATE! Draw."); }
                else if (g_board.isInCheck(nextTurn)) { snprintf(g_statusMsg, sizeof(g_statusMsg), "CHECK! Your turn."); }
                else { snprintf(g_statusMsg, sizeof(g_statusMsg), "Opponent moved. Your turn!"); }
                break;
            }
        }
    }
    void onWsError(const char* msg) {
        snprintf(g_onlineMsg, sizeof(g_onlineMsg), "%s", msg);
        g_onlineMenuState = ONLINE_MENU_DEFAULT;
    }
    void setOnlineJoinCode(const char* code) {
        snprintf(g_onlineJoinCode, sizeof(g_onlineJoinCode), "%s", code);
    }
}

void initWebSocketAndCreateRoom(int hintCount) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (window.socket) {
            try { window.socket.close(); } catch(e) {}
            window.socket = null;
        }
        var loc = window.location;
        var proto = (loc.protocol === 'https:') ? 'wss:' : 'ws:';
        var ws_url = window.CHESS_WS_URL || (proto + '//' + loc.host + '/ws');
        var hintCount = $0;

        try {
            window.socket = new WebSocket(ws_url);
            window.socket.onopen = function() {
                window.socket.send(JSON.stringify({action: "create_room", hint_count: hintCount}));
            };
            window.socket.onerror = function(err) {
                console.error("WebSocket error:", err);
                ccall('onWsError', 'void', ['string'], ["Could not connect to online server."]);
            };
            window.socket.onmessage = function(event) {
                var data = JSON.parse(event.data);
                var hc = (typeof data.hint_count === 'number') ? data.hint_count : 3;
                if (data.type === 'room_created') {
                    ccall('onRoomCreated', 'void', ['string', 'number'], [data.room_id, hc]);
                } else if (data.type === 'opponent_joined') {
                    ccall('onOpponentJoined', 'void', ['number'], [hc]);
                } else if (data.type === 'move') {
                    ccall('applyOpponentMove', 'void', ['number', 'number', 'number', 'number', 'number'],
                          [data.move.fromRow, data.move.fromCol, data.move.toRow, data.move.toCol, data.move.promoteTo]);
                } else if (data.type === 'opponent_resigned') {
                    ccall('onOpponentResigned', 'void', [], []);
                } else if (data.type === 'opponent_disconnected') {
                    ccall('onOpponentDisconnected', 'void', [], []);
                } else if (data.type === 'error') {
                    ccall('onWsError', 'void', ['string'], [data.message]);
                }
            };
        } catch(e) {
            console.error("WS init exception:", e);
            ccall('onWsError', 'void', ['string'], ["Failed to initialize connection."]);
        }
    }, hintCount);
#endif
}

void initWebSocketAndJoinRoom(const char* roomId) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (window.socket) {
            try { window.socket.close(); } catch(e) {}
            window.socket = null;
        }
        var loc = window.location;
        var proto = (loc.protocol === 'https:') ? 'wss:' : 'ws:';
        var ws_url = window.CHESS_WS_URL || (proto + '//' + loc.host + '/ws');
        var rId = UTF8ToString($0).trim().toUpperCase();

        try {
            window.socket = new WebSocket(ws_url);
            window.socket.onopen = function() {
                window.socket.send(JSON.stringify({action: "join_room", room_id: rId}));
            };
            window.socket.onerror = function(err) {
                console.error("WebSocket error:", err);
                ccall('onWsError', 'void', ['string'], ["Could not connect to online server."]);
            };
            window.socket.onmessage = function(event) {
                var data = JSON.parse(event.data);
                var hc = (typeof data.hint_count === 'number') ? data.hint_count : 3;
                if (data.type === 'room_joined') {
                    ccall('onRoomJoined', 'void', ['string', 'string', 'number'], [data.color, data.room_id || "", hc]);
                } else if (data.type === 'move') {
                    ccall('applyOpponentMove', 'void', ['number', 'number', 'number', 'number', 'number'],
                          [data.move.fromRow, data.move.fromCol, data.move.toRow, data.move.toCol, data.move.promoteTo]);
                } else if (data.type === 'opponent_resigned') {
                    ccall('onOpponentResigned', 'void', [], []);
                } else if (data.type === 'opponent_disconnected') {
                    ccall('onOpponentDisconnected', 'void', [], []);
                } else if (data.type === 'error') {
                    ccall('onWsError', 'void', ['string'], [data.message]);
                }
            };
        } catch(e) {
            console.error("WS join exception:", e);
            ccall('onWsError', 'void', ['string'], ["Failed to initialize connection."]);
        }
    }, roomId);
#endif
}

void sendMoveOnline(Move m) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (window.socket && window.socket.readyState === WebSocket.OPEN) {
            window.socket.send(JSON.stringify({
                action: "move",
                room_id: UTF8ToString($0),
                move: {
                    fromRow: $1, fromCol: $2, toRow: $3, toCol: $4, promoteTo: $5
                }
            }));
        }
    }, g_onlineRoomCode, m.fromRow, m.fromCol, m.toRow, m.toCol, m.promoteTo);
#endif
}

void sendResignOnline() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (window.socket && window.socket.readyState === WebSocket.OPEN) {
            window.socket.send(JSON.stringify({
                action: "resign",
                room_id: UTF8ToString($0)
            }));
        }
    }, g_onlineRoomCode);
#endif
}

void sendLeaveRoomOnline() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        window._currentRoomCode = "";
        if (window.socket) {
            try {
                if (window.socket.readyState === WebSocket.OPEN) {
                    window.socket.send(JSON.stringify({
                        action: "leave_room",
                        room_id: UTF8ToString($0)
                    }));
                }
                window.socket.close();
            } catch(e) {}
            window.socket = null;
        }
    }, g_onlineRoomCode);
#endif
    g_isOnline = false;
    g_onlineRoomCode[0] = '\0';
    g_onlineMenuState = ONLINE_MENU_DEFAULT;
}

void copyRoomCodeToClipboard(const char* code) {
    if (!code || strlen(code) == 0) return;
#ifdef __EMSCRIPTEN__
    EM_ASM({
        var text = UTF8ToString($0);
        window._currentRoomCode = text;
        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(text).catch(function(e){});
        } else {
            var ta = document.createElement("textarea");
            ta.value = text;
            document.body.appendChild(ta);
            ta.select();
            document.execCommand("copy");
            document.body.removeChild(ta);
        }
    }, code);
#else
    SetClipboardText(code);
#endif
    g_copiedRoomCode = true;
    g_copiedRoomTimer = 2.5f;
}

void pasteRoomCodeFromClipboard() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        function promptFallback() {
            var input = window.prompt("Enter 5-character Room Code:");
            if (input) {
                var clean = input.trim().toUpperCase().substring(0, 10);
                ccall('setOnlineJoinCode', 'void', ['string'], [clean]);
            }
        }
        if (navigator.clipboard && navigator.clipboard.readText) {
            navigator.clipboard.readText().then(function(text) {
                if (text && text.trim().length > 0) {
                    var clean = text.trim().toUpperCase().substring(0, 10);
                    ccall('setOnlineJoinCode', 'void', ['string'], [clean]);
                } else {
                    promptFallback();
                }
            }).catch(function(err) {
                promptFallback();
            });
        } else {
            promptFallback();
        }
    });
#else
    const char* clip = GetClipboardText();
    if (clip && strlen(clip) > 0) {
        setOnlineJoinCode(clip);
    }
#endif
}


void loadAssets() {
    const char* pNames[] = {"", "pawn", "knight", "bishop", "rook", "queen", "king"};
    for (int t = 1; t <= 6; t++) {
        char pathW[128], pathB[128];
        snprintf(pathW, sizeof(pathW), "assets/pieces/default/w_%s.png", pNames[t]);
        snprintf(pathB, sizeof(pathB), "assets/pieces/default/b_%s.png", pNames[t]);
        texPieces[P_WHITE][t] = LoadTexture(pathW);
        texPieces[P_BLACK][t] = LoadTexture(pathB);
        SetTextureFilter(texPieces[P_WHITE][t], TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(texPieces[P_BLACK][t], TEXTURE_FILTER_BILINEAR);

        char pathSil[128], pathLine[128];
        snprintf(pathSil, sizeof(pathSil), "assets/pieces/shadows/sil_%s.png", pNames[t]);
        snprintf(pathLine, sizeof(pathLine), "assets/pieces/shadows/line_%s.png", pNames[t]);
        texShadows[t] = LoadTexture(pathSil);
        texLines[t]   = LoadTexture(pathLine);
        if (texShadows[t].id != 0) SetTextureFilter(texShadows[t], TEXTURE_FILTER_BILINEAR);
        if (texLines[t].id != 0)   SetTextureFilter(texLines[t],   TEXTURE_FILTER_BILINEAR);
    }
}

void unloadAssets() {
    for (int t = 1; t <= 6; t++) {
        UnloadTexture(texPieces[P_WHITE][t]);
        UnloadTexture(texPieces[P_BLACK][t]);
        if (texShadows[t].id != 0) UnloadTexture(texShadows[t]);
        if (texLines[t].id != 0)   UnloadTexture(texLines[t]);
    }
}

// ─── Draw Profile Picture (Procedural) ───────────────────────────────────────
void drawProfilePic(int pfpIndex, float cx, float cy, float radius) {
    if (pfpIndex < 0 || pfpIndex > 9) pfpIndex = 0;
    Color primary = PFP_PRIMARY[pfpIndex];
    Color secondary = PFP_SECONDARY[pfpIndex];

    // Circle background
    DrawCircle((int)cx, (int)cy, radius, secondary);
    DrawCircle((int)cx, (int)cy, radius - 3, primary);

    // Draw letter-based icon
    float s = radius * 0.45f;
    const char* letters[] = {"N", "B", "R", "Q", "K", "P", "D", "C", "S", "W"};
    DrawText(letters[pfpIndex], (int)(cx - s*0.35f), (int)(cy - s*0.5f), (int)(s*1.8f), COL_TEXT_BRIGHT);

    // Ring border
    DrawCircleLines((int)cx, (int)cy, radius, COL_ACCENT);
    DrawCircleLines((int)cx, (int)cy, radius + 1, COL_ACCENT_DIM);
}

// ─── Draw Decorative Chess Pattern Background ────────────────────────────────
void drawChessBackground() {
    ClearBackground(COL_BG_DARK);
    
    // Subtle chess grid pattern across the background
    int gridSize = 60;
    for (int x = 0; x < SCREEN_WIDTH; x += gridSize) {
        for (int y = 0; y < SCREEN_HEIGHT; y += gridSize) {
            int gx = x / gridSize;
            int gy = y / gridSize;
            if ((gx + gy) % 2 == 0) {
                DrawRectangle(x, y, gridSize, gridSize, Fade(WHITE, 0.015f));
            }
        }
    }
    
    // Subtle vignette corners
    DrawRectangleGradientH(0, 0, 150, SCREEN_HEIGHT, Fade(BLACK, 0.3f), Fade(BLACK, 0.0f));
    DrawRectangleGradientH(SCREEN_WIDTH - 150, 0, 150, SCREEN_HEIGHT, Fade(BLACK, 0.0f), Fade(BLACK, 0.3f));
}

// ─── Draw Chess Piece Shadow Helper ──────────────────────────────────────────
void drawPieceShadow(int type, float cx, float cy, float size, float rotation, float alpha, bool flipH = false, bool withGoldLines = true) {
    if (type < 1 || type > 6 || texShadows[type].id == 0) return;
    
    float texW = (float)texShadows[type].width;
    float texH = (float)texShadows[type].height;
    Rectangle source = { 0, 0, flipH ? -texW : texW, texH };
    Vector2 origin = { size / 2.0f, size / 2.0f };
    Rectangle destBody = { cx, cy, size, size };

    // 1. Main silhouette body (deep atmospheric charcoal/black)
    DrawTexturePro(texShadows[type], source, destBody, origin, rotation, Fade((Color){ 12, 12, 16, 255 }, alpha * 0.88f));

    // 2. Subtle ambient gold rim glow
    DrawTexturePro(texShadows[type], source, destBody, origin, rotation, Fade(COL_ACCENT, alpha * 0.12f));

    // 3. Subtle gold engraved linework
    if (withGoldLines && texLines[type].id != 0) {
        DrawTexturePro(texLines[type], source, destBody, origin, rotation, Fade(COL_ACCENT, alpha * 0.28f));
    }
}

// ─── Draw Cinematic Background Chess Shadows for Login ───────────────────────
void drawLoginChessShadows(float cardX, float cardW) {
    float time = (float)GetTime();
    float bobLeft  = sinf(time * 0.6f) * 6.0f;
    float bobRight = cosf(time * 0.65f) * 6.0f;

    float leftMargin  = cardX;
    float rightMargin = (float)SCREEN_WIDTH - (cardX + cardW);
    
    // Scale pieces to fit comfortably in the side areas with clear margins
    float pieceSize = SCREEN_HEIGHT * 0.54f;
    if (pieceSize > leftMargin * 0.80f) pieceSize = leftMargin * 0.80f;

    float groundY = (float)SCREEN_HEIGHT * 0.60f;

    // ─── Left Side: Single grand King centered with no overlapping ───────────
    if (leftMargin > 100) {
        float kingX = leftMargin / 2.0f;
        drawPieceShadow(KING, kingX, groundY + bobLeft, pieceSize, 0.0f, 0.45f, false, true);
    }

    // ─── Right Side: Single grand Queen centered with no overlapping ─────────
    if (rightMargin > 100) {
        float queenX = (cardX + cardW) + rightMargin / 2.0f;
        drawPieceShadow(QUEEN, queenX, groundY + bobRight, pieceSize, 0.0f, 0.45f, false, true);
    }

    // Grounding floor fade gradient at the very bottom
    DrawRectangleGradientV(0, SCREEN_HEIGHT - 160, SCREEN_WIDTH, 160, Fade(BLACK, 0.0f), Fade(BLACK, 0.55f));
}

// ─── Themed UI Helpers ───────────────────────────────────────────────────────
bool drawThemedButton(Rectangle bounds, const char* text, bool accent = false, int fontSize = 20) {
    bool clicked = false;
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, bounds);

    if (hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) clicked = true;

    Color bgNorm = accent ? COL_BTN_ACCENT : COL_BTN;
    Color bgHov  = accent ? COL_BTN_ACC_HOV : COL_BTN_HOV;
    Color border = accent ? COL_ACCENT : COL_SEPARATOR;
    Color textCol = accent ? COL_BG_DARK : COL_TEXT_LIGHT;

    // Shadow
    DrawRectangleRounded({bounds.x + 2, bounds.y + 2, bounds.width, bounds.height}, 0.2f, 6, Fade(BLACK, 0.3f));
    // Button body
    DrawRectangleRounded(bounds, 0.2f, 6, hover ? bgHov : bgNorm);
    // Border
    DrawRectangleRoundedLinesEx(bounds, 0.2f, 6, 2, hover ? COL_ACCENT : border);
    
    int tw = MeasureText(text, fontSize);
    DrawText(text, (int)(bounds.x + (bounds.width - tw) / 2), (int)(bounds.y + (bounds.height - fontSize) / 2), fontSize, textCol);
    
    return clicked;
}

// Small icon button (for eye toggle, etc.)
bool drawIconButton(Rectangle bounds, const char* icon) {
    bool clicked = false;
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, bounds);
    if (hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) clicked = true;

    DrawRectangleRounded(bounds, 0.3f, 4, hover ? COL_BTN_HOV : Fade(COL_BTN, 0.6f));
    int tw = MeasureText(icon, 16);
    DrawText(icon, (int)(bounds.x + (bounds.width - tw) / 2), (int)(bounds.y + (bounds.height - 16) / 2), 16, COL_TEXT_LIGHT);
    
    return clicked;
}

void drawThemedTextBox(Rectangle bounds, char* textBuf, int maxLen, bool active, bool isPassword = false, bool showPass = false) {
    Color bg = active ? COL_INPUT_ACTIVE : COL_INPUT_BG;
    Color border = active ? COL_ACCENT : COL_SEPARATOR;

    DrawRectangleRounded(bounds, 0.15f, 6, bg);
    DrawRectangleRoundedLinesEx(bounds, 0.15f, 6, 2, border);
    
    // Display text or stars
    if (isPassword && !showPass) {
        int len = (int)strlen(textBuf);
        char stars[64] = "";
        for (int i = 0; i < len && i < 63; i++) stars[i] = '*';
        stars[len < 63 ? len : 63] = '\0';
        DrawText(stars, (int)(bounds.x + 12), (int)(bounds.y + (bounds.height - 20)/2), 20, COL_TEXT_LIGHT);
    } else {
        DrawText(textBuf, (int)(bounds.x + 12), (int)(bounds.y + (bounds.height - 20)/2), 20, COL_TEXT_LIGHT);
    }
    
    // Blinking cursor
    if (active && ((int)(GetTime() * 2) % 2 == 0)) {
        const char* displayText = textBuf;
        char stars[64] = "";
        if (isPassword && !showPass) {
            int len = (int)strlen(textBuf);
            for (int i = 0; i < len && i < 63; i++) stars[i] = '*';
            stars[len < 63 ? len : 63] = '\0';
            displayText = stars;
        }
        int cursorX = (int)(bounds.x + 12 + MeasureText(displayText, 20));
        DrawRectangle(cursorX, (int)(bounds.y + 8), 2, (int)(bounds.height - 16), COL_ACCENT);
    }
    
    if (active) {
        bool isCtrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
                      IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
        if (isCtrl && IsKeyPressed(KEY_C)) {
            copyRoomCodeToClipboard(textBuf);
        }
        if (isCtrl && IsKeyPressed(KEY_V)) {
#ifdef __EMSCRIPTEN__
            pasteRoomCodeFromClipboard();
#else
            const char* clip = GetClipboardText();
            if (clip) {
                for (int i = 0; clip[i] && (int)strlen(textBuf) < maxLen - 1; i++) {
                    int len = (int)strlen(textBuf);
                    textBuf[len] = clip[i];
                    textBuf[len+1] = '\0';
                }
            }
#endif
        }
        int key = GetCharPressed();
        while (key > 0) {
            // Ignore character insertion if Ctrl or Cmd is held
            if ((key >= 32) && (key <= 125) && !isCtrl && ((int)strlen(textBuf) < maxLen - 1)) {
                int len = (int)strlen(textBuf);
                textBuf[len] = (char)key;
                textBuf[len+1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            int len = (int)strlen(textBuf);
            if (len > 0) textBuf[len-1] = '\0';
        }
    }
}

// Draw profile section at top-right corner
void drawProfileTopRight() {
    User* u = g_auth.getCurrentUser();
    if (!u) return;

    float panelW = 260;
    float panelH = 65;
    float panelX = SCREEN_WIDTH - panelW - 15;
    float panelY = 12;

    // Semi-transparent panel
    DrawRectangleRounded({panelX, panelY, panelW, panelH}, 0.2f, 6, Fade(COL_BG_CARD, 0.9f));
    DrawRectangleRoundedLinesEx({panelX, panelY, panelW, panelH}, 0.2f, 6, 1, COL_SEPARATOR);

    // Profile picture
    float pfpR = 22;
    float pfpCx = panelX + 32;
    float pfpCy = panelY + panelH / 2;
    drawProfilePic(u->profilePic, pfpCx, pfpCy, pfpR);

    // Username and rank
    RankInfo rank = getRank(u->exp);
    DrawText(u->username, (int)(pfpCx + pfpR + 12), (int)(panelY + 10), 18, COL_TEXT_BRIGHT);
    
    char rankStr[64];
    snprintf(rankStr, sizeof(rankStr), "%s | %dEXP", rank.title, u->exp);
    DrawText(rankStr, (int)(pfpCx + pfpR + 12), (int)(panelY + 32), 14, COL_ACCENT_DIM);

    // Buttons underneath account panel: Avatar, Logout, Quit
    float subBtnW = 80;
    float subBtnH = 28;
    float subGap = 6;
    float startBtnX = panelX + (panelW - (subBtnW * 3 + subGap * 2)) / 2;
    float btnY = panelY + panelH + 6;
    Vector2 mouse = GetMousePosition();

    // 1. Avatar
    Rectangle pfpBtn = {startBtnX, btnY, subBtnW, subBtnH};
    bool hoverPfp = CheckCollisionPointRec(mouse, pfpBtn);
    DrawRectangleRounded(pfpBtn, 0.25f, 4, hoverPfp ? Fade(COL_BTN_HOV, 0.95f) : Fade(COL_BG_CARD, 0.85f));
    DrawRectangleRoundedLinesEx(pfpBtn, 0.25f, 4, 1, hoverPfp ? COL_ACCENT : COL_SEPARATOR);
    int pfpTw = MeasureText("Avatar", 13);
    DrawText("Avatar", (int)(pfpBtn.x + (pfpBtn.width - pfpTw)/2), (int)(pfpBtn.y + 7), 13, hoverPfp ? COL_ACCENT : COL_TEXT_LIGHT);
    if (hoverPfp && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        g_appState = STATE_PROFILE_PIC;
    }

    // 2. Logout
    Rectangle logoutBtn = {startBtnX + subBtnW + subGap, btnY, subBtnW, subBtnH};
    bool hoverLogout = CheckCollisionPointRec(mouse, logoutBtn);
    Color colLogoutBorder = hoverLogout ? Color{230, 150, 60, 255} : COL_SEPARATOR;
    DrawRectangleRounded(logoutBtn, 0.25f, 4, hoverLogout ? Fade(Color{180, 100, 30, 255}, 0.85f) : Fade(COL_BG_CARD, 0.85f));
    DrawRectangleRoundedLinesEx(logoutBtn, 0.25f, 4, 1, colLogoutBorder);
    int ltw = MeasureText("Logout", 13);
    DrawText("Logout", (int)(logoutBtn.x + (logoutBtn.width - ltw)/2), (int)(logoutBtn.y + 7), 13, hoverLogout ? WHITE : COL_TEXT_LIGHT);
    if (hoverLogout && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        g_auth.logout();
        g_appState = STATE_LOGIN;
        g_usernameInput[0] = '\0';
        g_passwordInput[0] = '\0';
        g_loginMsg[0] = '\0';
    }

    // 3. Quit
    Rectangle quitBtn = {startBtnX + (subBtnW + subGap) * 2, btnY, subBtnW, subBtnH};
    bool hoverQuit = CheckCollisionPointRec(mouse, quitBtn);
    DrawRectangleRounded(quitBtn, 0.25f, 4, hoverQuit ? Fade(COL_DANGER, 0.9f) : Fade(COL_BG_CARD, 0.85f));
    DrawRectangleRoundedLinesEx(quitBtn, 0.25f, 4, 1, hoverQuit ? COL_DANGER : Fade(COL_DANGER, 0.45f));
    int qtw = MeasureText("Quit", 13);
    DrawText("Quit", (int)(quitBtn.x + (quitBtn.width - qtw)/2), (int)(quitBtn.y + 7), 13, hoverQuit ? WHITE : Color{230, 120, 120, 255});
    if (hoverQuit && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        CloseWindow();
        exit(0);
    }
}

// ─── Draw Decorative Title ───────────────────────────────────────────────────
void drawTitle(const char* text, int y, int fontSize, Color color) {
    int tw = MeasureText(text, fontSize);
    int x = SCREEN_WIDTH / 2 - tw / 2;
    
    // Shadow
    DrawText(text, x + 3, y + 3, fontSize, Fade(BLACK, 0.5f));
    // Main text
    DrawText(text, x, y, fontSize, color);
    
    // Decorative line under title
    int lineW = tw + 30;
    int lineX = SCREEN_WIDTH / 2 - lineW / 2;
    DrawRectangle(lineX, y + fontSize + 8, lineW, 2, Fade(COL_ACCENT, 0.5f));
    // Center diamond
    DrawRectanglePro({(float)(SCREEN_WIDTH/2), (float)(y + fontSize + 9), 8, 8}, {4, 4}, 45.0f, COL_ACCENT);
}

// ─── Clipboard Copy (Win32) ───────────────────────────────────────────────────
void copyToClipboard(const char* text) {
    SetClipboardText(text);
}

// ─── Build full move list string for clipboard ────────────────────────────────
void buildMoveListString(char* out, int maxLen) {
    out[0] = '\0';
    int pos = 0;
    const char* pieceLetters = ".PNBRQK";
    for (int i = 0; i < g_board.moveHistoryCount && pos < maxLen - 40; i++) {
        Move m = g_board.moveHistory[i];
        char pLetter = ' ';
        if (i < g_board.snapshotCount) {
            PieceType pt = g_board.stateSnapshots[i].grid[m.fromRow][m.fromCol].type;
            if (pt >= PAWN && pt <= KING) pLetter = pieceLetters[pt];
        }
        if (i % 2 == 0) {
            pos += snprintf(out + pos, maxLen - pos, "%d. ", (i / 2) + 1);
        }
        if (pLetter != ' ' && pLetter != 'P') {
            pos += snprintf(out + pos, maxLen - pos, "%c", pLetter);
        }
        if (m.isCapture) pos += snprintf(out + pos, maxLen - pos, "x");
        pos += snprintf(out + pos, maxLen - pos, "%c%d", 'a' + m.toCol, m.toRow + 1);
        if (m.isPromotion) {
            pos += snprintf(out + pos, maxLen - pos, "=%c", pieceLetters[m.promoteTo]);
        }
        if (i % 2 == 0) {
            pos += snprintf(out + pos, maxLen - pos, " ");
        } else {
            pos += snprintf(out + pos, maxLen - pos, "  ");
        }
    }
}

void tryExecuteMove(int fromR, int fromC, int toR, int toC) {
    MoveList legal = g_board.getLegalMoves(g_board.state.currentTurn);
    for (int i = 0; i < legal.count; i++) {
        Move m = legal.moves[i];
        if (m.fromRow == fromR && m.fromCol == fromC && m.toRow == toR && m.toCol == toC) {
            g_board.applyMove(m);
            if (g_isOnline) {
                sendMoveOnline(m);
                g_myTurnOnline = false;
            }
            selectedRow = -1;
            selectedCol = -1;
            
            PieceColor nextTurn = g_board.state.currentTurn;
            if (g_board.isCheckmate(nextTurn)) { 
                g_gameOver = true;
                const char* winner = (nextTurn == P_WHITE) ? "Black" : "White";
                snprintf(g_statusMsg, sizeof(g_statusMsg), "CHECKMATE! %s Won!", winner);
                User* u = g_auth.getCurrentUser();
                if (g_rankedPvp && g_player2 != nullptr && u != nullptr) {
                    if (nextTurn == P_BLACK) {
                        g_auth.addExp(*u, 50);
                        g_auth.addExp(*g_player2, -20);
                    } else {
                        g_auth.addExp(*g_player2, 50);
                        g_auth.addExp(*u, -20);
                    }
                } else if (u) {
                    g_auth.addPoints(*u, 150);
                }
            }
            else if (g_board.isStalemate(nextTurn)) { g_gameOver = true; snprintf(g_statusMsg, sizeof(g_statusMsg), "STALEMATE! Draw."); }
            else if (g_board.isInCheck(nextTurn)) { snprintf(g_statusMsg, sizeof(g_statusMsg), "CHECK!"); }
            else { snprintf(g_statusMsg, sizeof(g_statusMsg), "Move played."); }
            return;
        }
    }
}

void handleGameInput() {
    if (g_gameOver) return;
    if (g_viewingMoveIdx >= 0) return;
    
    // AI Turn
    if (g_vsAI && g_board.state.currentTurn != g_playerColor) {
        if (g_aiMoveReady) {
            if (g_aiCalculatedMove.fromRow >= 0) {
                g_board.applyMove(g_aiCalculatedMove);
                snprintf(g_statusMsg, sizeof(g_statusMsg), "CPU moved.");
            }
            g_aiMoveReady = false;
            
            if (g_board.isCheckmate(g_playerColor)) {
                g_gameOver = true;
                const char* winner = (g_playerColor == P_WHITE) ? "Black" : "White";
                snprintf(g_statusMsg, sizeof(g_statusMsg), "CHECKMATE! %s Won!", winner);
            }
            else if (g_board.isStalemate(g_playerColor)) { g_gameOver = true; snprintf(g_statusMsg, sizeof(g_statusMsg), "STALEMATE! Draw."); }
        }
        else if (!g_aiIsThinking) {
            snprintf(g_statusMsg, sizeof(g_statusMsg), "CPU is thinking...");
            g_aiIsThinking = true;
#ifdef __EMSCRIPTEN__
            // Heap-allocate the copy so the 409 KB Board doesn't land on
            // Emscripten's 64 KB WebAssembly stack and cause a stack overflow.
            Board* boardCopy = new Board(g_board);
            g_aiCalculatedMove = g_ai->getBestMove(*boardCopy, boardCopy->state.currentTurn);
            delete boardCopy;
            g_aiIsThinking = false;
            g_aiMoveReady = true;
#else
            std::thread([]() {
                Board* boardCopy = new Board(g_board);
                g_aiCalculatedMove = g_ai->getBestMove(*boardCopy, boardCopy->state.currentTurn);
                delete boardCopy;
                g_aiIsThinking = false;
                g_aiMoveReady = true;
            }).detach();
#endif
        }
        return;
    }

    if (g_isOnline && !g_myTurnOnline) {
        // Not our turn, ignore clicks
        return;
    }

    bool flipBoard = g_isOnline ? (g_playerColor == P_BLACK) : (!g_vsAI && (g_board.state.currentTurn == P_BLACK));

    Vector2 mouse = GetMousePosition();
    int hoverC = -1, hoverR = -1;
    if (mouse.x >= BOARD_X && mouse.x < BOARD_X + BOARD_SIZE &&
        mouse.y >= BOARD_Y && mouse.y < BOARD_Y + BOARD_SIZE) {
        int dispC = (int)(mouse.x - BOARD_X) / CELL_SIZE;
        int dispR = 7 - (int)(mouse.y - BOARD_Y) / CELL_SIZE;
        if (dispC >= 0 && dispC < 8 && dispR >= 0 && dispR < 8) {
            hoverC = flipBoard ? (7 - dispC) : dispC;
            hoverR = flipBoard ? (7 - dispR) : dispR;
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        hintDestRow = -1; hintDestCol = -1;
        if (hoverC >= 0 && hoverR >= 0) {
            PieceColor curTurn = g_board.state.currentTurn;
            PieceColor myColor = g_isOnline ? g_playerColor : curTurn;

            if (selectedRow == -1) {
                if (g_board.at(hoverR, hoverC).color == curTurn &&
                    (!g_isOnline || g_board.at(hoverR, hoverC).color == myColor)) {
                    selectedRow = hoverR;
                    selectedCol = hoverC;
                }
            } else {
                if (g_board.at(hoverR, hoverC).color == curTurn &&
                    (!g_isOnline || g_board.at(hoverR, hoverC).color == myColor)) {
                    selectedRow = hoverR;
                    selectedCol = hoverC;
                } else {
                    int prevR = selectedRow, prevC = selectedCol;
                    tryExecuteMove(prevR, prevC, hoverR, hoverC);
                    if (selectedRow == prevR && selectedCol == prevC) {
                        selectedRow = -1;
                        selectedCol = -1;
                    }
                }
            }
        } else {
            selectedRow = -1;
            selectedCol = -1;
        }
    }
    else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (selectedRow != -1 && hoverC >= 0 && hoverR >= 0 &&
            (hoverR != selectedRow || hoverC != selectedCol)) {
            tryExecuteMove(selectedRow, selectedCol, hoverR, hoverC);
        }
    }
}

void drawGame() {
    drawChessBackground();

    // Update Copied! timer
    if (g_showCopied) {
        g_copiedTimer -= GetFrameTime();
        if (g_copiedTimer <= 0.0f) g_showCopied = false;
    }
    
    bool viewingHistory = (g_viewingMoveIdx >= 0 && g_viewingMoveIdx < g_board.snapshotCount);
    const GameState& renderState = viewingHistory ? g_board.getSnapshot(g_viewingMoveIdx) : g_board.state;

    bool flipBoard = g_isOnline ? (g_playerColor == P_BLACK) : (!g_vsAI && (renderState.currentTurn == P_BLACK));

    // Draw Board
    Color colLight = { 240, 217, 181, 255 };
    Color colDark  = { 181, 136, 99, 255 };
    
    // Board shadow
    DrawRectangleRounded({(float)BOARD_X - 4, (float)BOARD_Y - 4, (float)BOARD_SIZE + 8, (float)BOARD_SIZE + 8}, 0.02f, 4, Fade(BLACK, 0.4f));
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            bool isDark = (r + c) % 2 == 0;
            int displayR = flipBoard ? (7 - r) : r;
            int displayC = flipBoard ? (7 - c) : c;
            Rectangle rect = { (float)BOARD_X + displayC * CELL_SIZE, (float)BOARD_Y + (7 - displayR) * CELL_SIZE, (float)CELL_SIZE, (float)CELL_SIZE };
            DrawRectangleRec(rect, isDark ? colDark : colLight);
            
            if (viewingHistory) {
                DrawRectangleRec(rect, Fade(BLUE, 0.08f));
            }
            
            if (!viewingHistory && r == selectedRow && c == selectedCol) {
                DrawRectangleRec(rect, Fade(YELLOW, 0.5f));
            }
            
            const Cell& cell = renderState.grid[r][c];
            if (!cell.isEmpty()) {
                Texture2D tex = texPieces[cell.color][cell.type];
                if (tex.id != 0) {
                    DrawTexturePro(tex, 
                        {0, 0, (float)tex.width, (float)tex.height}, 
                        rect, {0,0}, 0.0f, WHITE);
                } else {
                    const char* letter = "P";
                    if (cell.type == KNIGHT) letter = "N";
                    if (cell.type == BISHOP) letter = "B";
                    if (cell.type == ROOK) letter = "R";
                    if (cell.type == QUEEN) letter = "Q";
                    if (cell.type == KING) letter = "K";
                    DrawText(letter, (int)(rect.x + 20), (int)(rect.y + 10), 50, cell.color == P_WHITE ? WHITE : BLACK);
                }
            }
        }
    }
    
    // Rank/File labels
    for (int i = 0; i < 8; i++) {
        char file[2] = { (char)('a' + i), '\0' };
        char rankLabel[2] = { (char)('1' + i), '\0' };
        int fileI = flipBoard ? (7 - i) : i;
        int rankI = flipBoard ? (7 - i) : i;
        DrawText(file, BOARD_X + fileI * CELL_SIZE + CELL_SIZE/2 - 4, BOARD_Y + BOARD_SIZE + 5, 14, COL_TEXT_DIM);
        DrawText(rankLabel, BOARD_X - 16, BOARD_Y + (7 - rankI) * CELL_SIZE + CELL_SIZE/2 - 7, 14, COL_TEXT_DIM);
    }
    
    // Legal move highlights
    if (!viewingHistory && selectedRow != -1) {
        MoveList legal = g_board.getLegalMoves(g_board.state.currentTurn);
        for (int i = 0; i < legal.count; i++) {
            Move m = legal.moves[i];
            if (m.fromRow == selectedRow && m.fromCol == selectedCol) {
                int displayR = flipBoard ? (7 - m.toRow) : m.toRow;
                int displayC = flipBoard ? (7 - m.toCol) : m.toCol;
                float cx = BOARD_X + displayC * CELL_SIZE + CELL_SIZE / 2.0f;
                float cy = BOARD_Y + (7 - displayR) * CELL_SIZE + CELL_SIZE / 2.0f;
                DrawCircle((int)cx, (int)cy, 15, Fade(GREEN, 0.5f));
            }
        }
    }
    
    // Hint highlight
    if (!viewingHistory && hintDestRow != -1 && hintDestCol != -1) {
        int displayR = flipBoard ? (7 - hintDestRow) : hintDestRow;
        int displayC = flipBoard ? (7 - hintDestCol) : hintDestCol;
        float cx = BOARD_X + displayC * CELL_SIZE + CELL_SIZE / 2.0f;
        float cy = BOARD_Y + (7 - displayR) * CELL_SIZE + CELL_SIZE / 2.0f;
        DrawCircle((int)cx, (int)cy, 25, Fade(BLUE, 0.6f));
        DrawRectangleLinesEx({ (float)BOARD_X + displayC * CELL_SIZE, (float)BOARD_Y + (7 - displayR) * CELL_SIZE, (float)CELL_SIZE, (float)CELL_SIZE }, 4, BLUE);
    }

    if (viewingHistory) {
        char viewLabel[64];
        snprintf(viewLabel, sizeof(viewLabel), "Viewing move %d / %d", g_viewingMoveIdx, g_board.moveHistoryCount);
        int vw = MeasureText(viewLabel, 20);
        DrawText(viewLabel, BOARD_X + (BOARD_SIZE - vw) / 2, BOARD_Y - 30, 20, Fade(SKYBLUE, 0.9f));
    }
    
    // Status & Room Info
    if (g_isOnline) {
        char onlineTop[128];
        snprintf(onlineTop, sizeof(onlineTop), "ROOM: [%s]   |   YOU PLAY AS %s", 
                 g_onlineRoomCode, (g_playerColor == P_WHITE) ? "WHITE" : "BLACK");
        DrawText(onlineTop, 20, 15, 20, COL_ACCENT);

        if (g_gameOver) {
            DrawText(g_statusMsg, 20, 42, 20, COL_ACCENT);
        } else if (g_myTurnOnline) {
            DrawText("YOUR TURN - Click a piece to move", 20, 42, 20, COL_SUCCESS);
        } else {
            DrawText("Opponent is thinking...", 20, 42, 20, COL_TEXT_DIM);
        }
    } else {
        DrawText(g_statusMsg, 20, 20, 20, COL_TEXT_DIM);
    }
    
    // ─── Bottom Buttons ──────────────────────────────────────────────────────
    float btnY = (float)SCREEN_HEIGHT - 60;
    float btnX = 20;

    if (!g_isOnline) {
        if (drawThemedButton({btnX, btnY, 80, 40}, "Undo")) {
            g_viewingMoveIdx = -1;
            g_board.undoMove();
            if (g_vsAI && !stack_is_empty(g_board.history)) g_board.undoMove();
            g_gameOver = false;
            snprintf(g_statusMsg, sizeof(g_statusMsg), "Move undone.");
        }
        btnX += 90;
    }

    if (drawThemedButton({btnX, btnY, 80, 40}, g_isOnline ? "Leave" : "Menu")) {
        if (g_isOnline) {
            sendLeaveRoomOnline();
        }
        g_viewingMoveIdx = -1;
        g_appState = STATE_MAIN_MENU;
    }
    btnX += 90;

    if (drawThemedButton({btnX, btnY, 80, 40}, "Resign", false, 16)) {
        if (g_isOnline) {
            if (!g_gameOver) {
                sendResignOnline();
                g_gameOver = true;
                snprintf(g_statusMsg, sizeof(g_statusMsg), "You resigned. Opponent wins!");
            }
        } else {
            g_viewingMoveIdx = -1;
            g_gameOver = true;
            snprintf(g_statusMsg, sizeof(g_statusMsg), "You resigned. %s wins!", g_board.state.currentTurn == P_WHITE ? "BLACK" : "WHITE");
        }
    }
    btnX += 90;

    // Hint button — shown in all modes (offline AND online when hints are enabled)
    if (!g_isOnline || g_hintLimit.canUse() || g_hintLimit.unlimited) {
        // In online mode only show hint for the player whose turn it is
        bool canHintNow = !g_isOnline ||
            (g_myTurnOnline && !g_gameOver && g_board.state.currentTurn == g_playerColor);
        char hintBtnText[32];
        char hintRem[16];
        HintEngine::hintLabel(g_hintLimit, hintRem);
        snprintf(hintBtnText, sizeof(hintBtnText), "Hint(%s)", hintRem);
        bool hintDim = g_isOnline && !canHintNow;
        if (drawThemedButton({btnX, btnY, 100, 40}, hintBtnText, !hintDim)) {
            if (canHintNow && g_hintLimit.canUse()) {
                // Heap-allocate board copy so we don't put 409 KB on the WASM stack
                Board* bCopy = new Board(g_board);
                Move m = HintEngine::getHint(*bCopy, g_board.state.currentTurn, g_hintLimit);
                delete bCopy;
                if (m.fromRow != -1) {
                    g_viewingMoveIdx = -1;
                    selectedRow = m.fromRow;
                    selectedCol = m.fromCol;
                    hintDestRow = m.toRow;
                    hintDestCol = m.toCol;
                    snprintf(g_statusMsg, sizeof(g_statusMsg), "Hint: Move %c%d to %c%d (Blue)",
                             'a' + m.fromCol, m.fromRow + 1, 'a' + m.toCol, m.toRow + 1);
                }
            }
        }
        btnX += 110;
    }

    if (drawThemedButton({btnX, btnY, 70, 40}, "< Prev") || IsKeyPressed(KEY_LEFT)) {
        if (g_viewingMoveIdx < 0) {
            if (g_board.snapshotCount > 1)
                g_viewingMoveIdx = g_board.snapshotCount - 2;
            else
                g_viewingMoveIdx = 0;
        } else if (g_viewingMoveIdx > 0) {
            g_viewingMoveIdx--;
        }
    }
    btnX += 80;

    if (drawThemedButton({btnX, btnY, 70, 40}, "Next >") || IsKeyPressed(KEY_RIGHT)) {
        if (g_viewingMoveIdx >= 0) {
            g_viewingMoveIdx++;
            if (g_viewingMoveIdx >= g_board.snapshotCount) {
                g_viewingMoveIdx = -1;
            }
        }
    }

    // ─── Move Noter Panel ────────────────────────────────────────────────────
    int noterX = SCREEN_WIDTH - NOTER_WIDTH;
    DrawRectangle(noterX, 0, NOTER_WIDTH, SCREEN_HEIGHT, COL_PANEL_BG);
    DrawRectangleLinesEx({(float)noterX, 0, (float)NOTER_WIDTH, (float)SCREEN_HEIGHT}, 2, COL_SEPARATOR);
    DrawText("Move Noter", noterX + 15, 15, 22, COL_ACCENT);
    DrawLine(noterX + 10, 42, noterX + NOTER_WIDTH - 10, 42, COL_SEPARATOR);

    if (drawThemedButton({(float)noterX + NOTER_WIDTH - 125, 8, 115, 30}, "Copy Moves", false, 16)) {
        char moveStr[4096];
        buildMoveListString(moveStr, 4096);
        copyToClipboard(moveStr);
        g_showCopied = true;
        g_copiedTimer = 2.0f;
    }

    if (g_showCopied) {
        DrawText("Copied!", noterX + 15, 48, 18, COL_SUCCESS);
    }

    int moveListY = 70;
    
    if (g_rankedPvp && g_player2 != nullptr) {
        User* p1 = g_auth.getCurrentUser();
        if (p1) {
            char p1Text[64]; snprintf(p1Text, sizeof(p1Text), "White: %s", p1->username);
            char p2Text[64]; snprintf(p2Text, sizeof(p2Text), "Black: %s", g_player2->username);
            DrawText(p1Text, noterX + 15, 50, 18, COL_TEXT_LIGHT);
            DrawText(p2Text, noterX + 15, 70, 18, COL_TEXT_LIGHT);
            moveListY = 95;
        }
    }

    const char* pieceLetters = ".PNBRQK";
    int maxVisibleMoves = (SCREEN_HEIGHT - moveListY - 20) / 22;
    int totalPairs = (g_board.moveHistoryCount + 1) / 2;

    if (totalPairs > maxVisibleMoves) {
        Vector2 mouse = GetMousePosition();
        if (mouse.x >= noterX) {
            int wheel = (int)GetMouseWheelMove();
            g_noterScrollOffset -= wheel;
        }
        if (g_noterScrollOffset < 0) g_noterScrollOffset = 0;
        if (g_noterScrollOffset > totalPairs - maxVisibleMoves)
            g_noterScrollOffset = totalPairs - maxVisibleMoves;
    } else {
        g_noterScrollOffset = 0;
    }

    int startPair = g_noterScrollOffset;
    for (int p = startPair; p < totalPairs && (moveListY < SCREEN_HEIGHT - 60); p++) {
        int whiteIdx = p * 2;
        int blackIdx = p * 2 + 1;

        char whiteStr[24] = "";
        if (whiteIdx < g_board.moveHistoryCount) {
            Move m = g_board.moveHistory[whiteIdx];
            char pL = ' ';
            if (whiteIdx < g_board.snapshotCount) {
                PieceType pt = g_board.stateSnapshots[whiteIdx].grid[m.fromRow][m.fromCol].type;
                if (pt >= PAWN && pt <= KING) pL = pieceLetters[pt];
            }
            int wpos = 0;
            if (pL != ' ' && pL != 'P') wpos += snprintf(whiteStr + wpos, 24 - wpos, "%c", pL);
            if (m.isCapture) wpos += snprintf(whiteStr + wpos, 24 - wpos, "x");
            wpos += snprintf(whiteStr + wpos, 24 - wpos, "%c%d", 'a' + m.toCol, m.toRow + 1);
            if (m.isPromotion) wpos += snprintf(whiteStr + wpos, 24 - wpos, "=%c", pieceLetters[m.promoteTo]);
        }

        char blackStr[24] = "";
        if (blackIdx < g_board.moveHistoryCount) {
            Move m = g_board.moveHistory[blackIdx];
            char pL = ' ';
            if (blackIdx < g_board.snapshotCount) {
                PieceType pt = g_board.stateSnapshots[blackIdx].grid[m.fromRow][m.fromCol].type;
                if (pt >= PAWN && pt <= KING) pL = pieceLetters[pt];
            }
            int bpos = 0;
            if (pL != ' ' && pL != 'P') bpos += snprintf(blackStr + bpos, 24 - bpos, "%c", pL);
            if (m.isCapture) bpos += snprintf(blackStr + bpos, 24 - bpos, "x");
            bpos += snprintf(blackStr + bpos, 24 - bpos, "%c%d", 'a' + m.toCol, m.toRow + 1);
            if (m.isPromotion) bpos += snprintf(blackStr + bpos, 24 - bpos, "=%c", pieceLetters[m.promoteTo]);
        }

        bool highlightWhite = (viewingHistory && g_viewingMoveIdx == whiteIdx + 1);
        bool highlightBlack = (viewingHistory && g_viewingMoveIdx == blackIdx + 1);

        char line[64];
        snprintf(line, sizeof(line), "%3d.  %-10s  %-10s", p + 1, whiteStr, blackStr);

        Color lineColor = COL_TEXT_DIM;
        if (highlightWhite || highlightBlack) lineColor = SKYBLUE;

        DrawText(line, noterX + 10, moveListY, 18, lineColor);
        moveListY += 22;
    }

    // ─── Game Over Overlay ───────────────────────────────────────────────────
    if (g_gameOver) {
        DrawRectangle(0, 0, noterX, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
        int textW = MeasureText(g_statusMsg, 40);
        int boardCenterX = BOARD_X + BOARD_SIZE / 2;
        DrawText(g_statusMsg, boardCenterX - textW / 2, SCREEN_HEIGHT / 2 - 20, 40, COL_DANGER);
        if (drawThemedButton({(float)boardCenterX - 100, (float)SCREEN_HEIGHT/2 + 40, 200, 50}, "Return to Menu", true)) {
            g_viewingMoveIdx = -1;
            g_appState = STATE_MAIN_MENU;
        }
    }
}

// ─── Login Screen ────────────────────────────────────────────────────────────

void drawLogin() {
    drawChessBackground();
    
    // Layout: vertically center the entire block (title + subtitle + card + message)
    // Total block height: ~60 + 30 + 20 + 340 + 40 = ~490
    float totalBlockH = 490;
    float baseY = (SCREEN_HEIGHT - totalBlockH) / 2.0f;
    if (baseY < 30) baseY = 30;
    
    // Login card
    float cardW = 420;
    float cardH = 375;
    float cardX = SCREEN_WIDTH/2 - cardW/2;
    float cardY = baseY + 120;
    
    // Draw majestic chess piece shadows in background (behind title and card)
    drawLoginChessShadows(cardX, cardW);

    // Big title
    drawTitle("CHESSVERSE 2D", (int)baseY, 60, COL_ACCENT);
    
    // Subtitle
    const char* sub = "Enter the Arena";
    int subW = MeasureText(sub, 22);
    DrawText(sub, SCREEN_WIDTH/2 - subW/2, (int)(baseY + 85), 22, COL_TEXT_DIM);
    
    DrawRectangleRounded({cardX, cardY, cardW, cardH}, 0.05f, 8, Fade(COL_BG_CARD, 0.95f));
    DrawRectangleRoundedLinesEx({cardX, cardY, cardW, cardH}, 0.05f, 8, 2, COL_SEPARATOR);
    
    // Card title
    const char* loginTitle = "LOGIN";
    DrawText(loginTitle, (int)(cardX + cardW/2 - MeasureText(loginTitle, 24)/2), (int)(cardY + 20), 24, COL_ACCENT);
    DrawLine((int)(cardX + 30), (int)(cardY + 55), (int)(cardX + cardW - 30), (int)(cardY + 55), COL_SEPARATOR);

    // Username
    DrawText("Username", (int)(cardX + 30), (int)(cardY + 75), 18, COL_TEXT_DIM);
    drawThemedTextBox({cardX + 30, cardY + 100, cardW - 60, 42}, g_usernameInput, 32, g_usernameActive);

    // Password with eye toggle
    DrawText("Password", (int)(cardX + 30), (int)(cardY + 155), 18, COL_TEXT_DIM);
    drawThemedTextBox({cardX + 30, cardY + 180, cardW - 105, 42}, g_passwordInput, 32, !g_usernameActive, true, g_showPassword);
    
    // Eye toggle button
    if (drawIconButton({cardX + cardW - 65, cardY + 180, 42, 42}, g_showPassword ? "Hide" : "Show")) {
        g_showPassword = !g_showPassword;
    }

    // Toggle active box on click
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, {cardX + 30, cardY + 100, cardW - 60, 42})) g_usernameActive = true;
        else if (CheckCollisionPointRec(mouse, {cardX + 30, cardY + 180, cardW - 105, 42})) g_usernameActive = false;
    }
    
    // Tab to switch fields
    if (IsKeyPressed(KEY_TAB)) g_usernameActive = !g_usernameActive;

    // Buttons
    float btnW = (cardW - 80) / 2;
    bool triggerLogin = drawThemedButton({cardX + 30, cardY + 240, btnW, 42}, "Login", true);
    if (IsKeyPressed(KEY_ENTER)) triggerLogin = true;

    if (triggerLogin) {
        User* u = g_auth.login(g_usernameInput, g_passwordInput);
        if (u) {
            g_appState = STATE_MAIN_MENU;
            g_loginMsg[0] = '\0';
        } else {
            strcpy(g_loginMsg, "Invalid credentials!");
        }
    }

    if (drawThemedButton({cardX + 50 + btnW, cardY + 240, btnW, 42}, "Register")) {
        if (g_auth.registerUser(g_usernameInput, g_passwordInput)) {
            strcpy(g_loginMsg, "Registered! Now login.");
        } else {
            strcpy(g_loginMsg, "Registration failed!");
        }
    }

    // Guest Play Button
    if (drawThemedButton({cardX + 30, cardY + 295, cardW - 60, 38}, "Play as Guest (Quick Play)", false, 16)) {
        User* u = g_auth.login("Guest", "guest");
        if (!u) {
            g_auth.registerUser("Guest", "guest");
            u = g_auth.login("Guest", "guest");
        }
        if (u) {
            g_appState = STATE_MAIN_MENU;
            g_loginMsg[0] = '\0';
        }
    }

    // Feedback message inside the card
    if (g_loginMsg[0] != '\0') {
        Color msgCol = COL_DANGER;
        if (strstr(g_loginMsg, "Registered")) msgCol = COL_SUCCESS;
        int mw = MeasureText(g_loginMsg, 15);
        DrawText(g_loginMsg, (int)(cardX + cardW/2 - mw/2), (int)(cardY + 346), 15, msgCol);
    }

    // Quit — cleanly below the card
    if (drawThemedButton({cardX + cardW/2 - 60, cardY + cardH + 15, 120, 35}, "Quit", false, 16)) {
        CloseWindow();
        exit(0);
    }
}

// ─── Leaderboard ─────────────────────────────────────────────────────────────

void drawLeaderboard() {
    drawChessBackground();
    
    drawProfileTopRight();
    
    User sortedUsers[MAX_USERS];
    int count = g_auth.getLeaderboard(sortedUsers);
    int displayCount = count < 15 ? count : 15;
    
    // Calculate total content height: title(40) + gap(15) + subtitle(18) + gap(25) + table + gap(25) + button(45)
    float tableH = (float)(displayCount * 35 + 55);
    if (tableH < 100) tableH = 100;
    float totalH = 40 + 15 + 18 + 25 + tableH + 25 + 45;
    float baseY = (SCREEN_HEIGHT - totalH) / 2.0f;
    if (baseY < 30) baseY = 30;
    
    drawTitle("LEADERBOARD", (int)baseY, 40, COL_ACCENT);
    DrawText("Sorted by EXP (Merge Sort)", SCREEN_WIDTH/2 - MeasureText("Sorted by EXP (Merge Sort)", 18)/2, (int)(baseY + 65), 18, COL_TEXT_DIM);
    
    // Table card
    float tableW = 600;
    float tableX = SCREEN_WIDTH/2 - tableW/2;
    float tableY = baseY + 100;
    
    DrawRectangleRounded({tableX, tableY, tableW, tableH}, 0.02f, 6, Fade(COL_BG_CARD, 0.9f));
    DrawRectangleRoundedLinesEx({tableX, tableY, tableW, tableH}, 0.02f, 6, 1, COL_SEPARATOR);
    
    int y = (int)tableY + 15;
    DrawText("Rank   Username             EXP   Points", (int)(tableX + 20), y, 18, COL_ACCENT);
    DrawLine((int)(tableX + 15), y + 25, (int)(tableX + tableW - 15), y + 25, COL_SEPARATOR);
    y += 40;

    for (int i = 0; i < displayCount; i++) {
        char entry[128];
        snprintf(entry, sizeof(entry), "#%-4d  %-15s      %-5d %-5d", i+1, sortedUsers[i].username, sortedUsers[i].exp, sortedUsers[i].points);
        Color entryCol = COL_TEXT_DIM;
        if (i == 0) entryCol = GOLD;
        else if (i == 1) entryCol = (Color){192, 192, 192, 255};
        else if (i == 2) entryCol = (Color){205, 127, 50, 255};
        DrawText(entry, (int)(tableX + 20), y, 18, entryCol);
        y += 35;
    }

    if (drawThemedButton({(float)SCREEN_WIDTH/2 - 100, tableY + tableH + 25, 200, 45}, "Back to Menu", true)) {
        g_appState = STATE_MAIN_MENU;
    }
}

// ─── Trivia ──────────────────────────────────────────────────────────────────

void drawTrivia() {
    drawChessBackground();
    User* u = g_auth.getCurrentUser();
    
    if (!u) {
        DrawText("You must be logged in to play Trivia!", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2, 20, COL_DANGER);
        if (drawThemedButton({(float)SCREEN_WIDTH/2 - 100, (float)SCREEN_HEIGHT/2 + 40, 200, 45}, "Back to Menu", true)) {
            g_appState = STATE_MAIN_MENU;
        }
        return;
    }

    drawProfileTopRight();

    Question* q = g_trivia.getDailyQuestion(u->username);
    
    if (!q) {
        drawTitle("DAILY TRIVIA", SCREEN_HEIGHT/2 - 80, 40, COL_ACCENT);
        const char* doneMsg = "You have already answered today's trivia!";
        DrawText(doneMsg, SCREEN_WIDTH/2 - MeasureText(doneMsg, 22)/2, SCREEN_HEIGHT/2 + 10, 22, COL_SUCCESS);
        if (drawThemedButton({(float)SCREEN_WIDTH/2 - 100, (float)SCREEN_HEIGHT/2 + 60, 200, 45}, "Back to Menu", true)) {
            g_appState = STATE_MAIN_MENU;
        }
        return;
    }

    // Layout: title(40) + gap(30) + question(20) + gap(30) + 4 options(4*70) + gap(20) + msg(20) + gap(20) + btn(45)
    // Total: ~505
    float totalH = 505;
    float baseY = (SCREEN_HEIGHT - totalH) / 2.0f;
    if (baseY < 30) baseY = 30;
    
    drawTitle("DAILY TRIVIA", (int)baseY, 40, COL_ACCENT);
    
    int qWidth = MeasureText(q->question, 20);
    DrawText(q->question, SCREEN_WIDTH/2 - qWidth/2, (int)(baseY + 80), 20, COL_TEXT_BRIGHT);

    float startY = baseY + 120;
    for (int i = 0; i < 5; i++) {
        Rectangle optRect = {(float)SCREEN_WIDTH/2 - 280, startY + i * 70, 560, 55};
        
        Vector2 mouse = GetMousePosition();
        bool hover = CheckCollisionPointRec(mouse, optRect);
        DrawRectangleRounded(optRect, 0.15f, 6, hover ? COL_BG_CARD_HOV : COL_BG_CARD);
        DrawRectangleRoundedLinesEx(optRect, 0.15f, 6, 2, hover ? COL_ACCENT : COL_SEPARATOR);
        
        char letter[4];
        snprintf(letter, sizeof(letter), "%c.", 'A' + i);
        DrawText(letter, (int)(optRect.x + 15), (int)(optRect.y + 17), 20, COL_ACCENT);
        DrawText(q->options[i], (int)(optRect.x + 45), (int)(optRect.y + 17), 20, COL_TEXT_LIGHT);
        
        if (hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            int earned = g_trivia.submitAnswer(u->username, i);
            if (earned > 0) {
                g_auth.addPoints(*u, earned);
                snprintf(g_triviaMsg, sizeof(g_triviaMsg), "Correct! You earned %d points/EXP.", earned);
            } else {
                snprintf(g_triviaMsg, sizeof(g_triviaMsg), "Incorrect! Better luck tomorrow.");
            }
        }
    }

    if (g_triviaMsg[0] != '\0') {
        Color triviaCol = strstr(g_triviaMsg, "Correct") ? COL_SUCCESS : COL_DANGER;
        DrawText(g_triviaMsg, SCREEN_WIDTH/2 - MeasureText(g_triviaMsg, 20)/2, (int)(startY + 300), 20, triviaCol);
    }

    if (drawThemedButton({(float)SCREEN_WIDTH/2 - 100, startY + 340, 200, 45}, "Back to Menu", true)) {
        g_appState = STATE_MAIN_MENU;
        g_triviaMsg[0] = '\0';
    }
}

// ─── Player 2 Login ──────────────────────────────────────────────────────────

void drawP2Login() {
    drawChessBackground();
    
    drawProfileTopRight();

    // Layout: title(40) + gap(30) + card(310) + gap(20) + msg(20) = ~420
    float totalH = 420;
    float baseY = (SCREEN_HEIGHT - totalH) / 2.0f;
    if (baseY < 30) baseY = 30;
    
    drawTitle("PLAYER 2 LOGIN", (int)baseY, 40, COL_ACCENT);

    float cardW = 420;
    float cardH = 310;
    float cardX = SCREEN_WIDTH/2 - cardW/2;
    float cardY = baseY + 80;
    
    DrawRectangleRounded({cardX, cardY, cardW, cardH}, 0.05f, 8, Fade(COL_BG_CARD, 0.95f));
    DrawRectangleRoundedLinesEx({cardX, cardY, cardW, cardH}, 0.05f, 8, 2, COL_SEPARATOR);
    
    const char* p2Title = "Authenticate Player 2";
    DrawText(p2Title, (int)(cardX + cardW/2 - MeasureText(p2Title, 20)/2), (int)(cardY + 20), 20, COL_TEXT_DIM);
    DrawLine((int)(cardX + 30), (int)(cardY + 50), (int)(cardX + cardW - 30), (int)(cardY + 50), COL_SEPARATOR);

    DrawText("Username", (int)(cardX + 30), (int)(cardY + 70), 18, COL_TEXT_DIM);
    drawThemedTextBox({cardX + 30, cardY + 95, cardW - 60, 42}, g_p2UsernameInput, 32, g_p2UsernameActive);

    DrawText("Password", (int)(cardX + 30), (int)(cardY + 155), 18, COL_TEXT_DIM);
    drawThemedTextBox({cardX + 30, cardY + 180, cardW - 105, 42}, g_p2PasswordInput, 32, !g_p2UsernameActive, true, g_showP2Password);
    
    // Eye toggle for P2
    if (drawIconButton({cardX + cardW - 65, cardY + 180, 42, 42}, g_showP2Password ? "Hide" : "Show")) {
        g_showP2Password = !g_showP2Password;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, {cardX + 30, cardY + 95, cardW - 60, 42})) g_p2UsernameActive = true;
        else if (CheckCollisionPointRec(mouse, {cardX + 30, cardY + 180, cardW - 105, 42})) g_p2UsernameActive = false;
    }
    
    if (IsKeyPressed(KEY_TAB)) g_p2UsernameActive = !g_p2UsernameActive;

    float btnW = (cardW - 80) / 2;
    if (drawThemedButton({cardX + 30, cardY + 245, btnW, 45}, "Login & Play", true)) {
        User* p1 = g_auth.getCurrentUser();
        if (p1 && strcmp(p1->username, g_p2UsernameInput) == 0) {
            strcpy(g_p2LoginMsg, "Cannot play against yourself!");
        } else {
            User* p2 = g_auth.authenticateOnly(g_p2UsernameInput, g_p2PasswordInput);
            if (p2) {
                g_player2 = p2;
                g_rankedPvp = true;
                g_vsAI = false;
                g_playerColor = P_WHITE;
                g_board.init(); g_gameOver = false; strcpy(g_statusMsg, "Game started.");
                g_hintLimit = HintLimit(-1);
                g_appState = STATE_GAME;
            } else {
                strcpy(g_p2LoginMsg, "Invalid credentials!");
            }
        }
    }

    if (drawThemedButton({cardX + 50 + btnW, cardY + 245, btnW, 45}, "Back")) {
        g_appState = STATE_MAIN_MENU;
    }

    DrawText(g_p2LoginMsg, SCREEN_WIDTH/2 - MeasureText(g_p2LoginMsg, 20)/2, (int)(cardY + cardH + 15), 20, COL_DANGER);
}

// ─── Profile Picture Selection Screen ────────────────────────────────────────

void drawProfilePicSelection() {
    drawChessBackground();

    User* u = g_auth.getCurrentUser();
    if (!u) {
        g_appState = STATE_MAIN_MENU;
        return;
    }

    // Layout: title(36) + gap(20) + subtitle(18) + gap(25) + grid(2*180) + gap(25) + btn(45)
    // Total: ~529
    float gridW = 750;
    float cellW = gridW / 5;
    float cellH = 180;
    float totalH = 36 + 20 + 18 + 25 + cellH * 2 + 25 + 45;
    float baseY = (SCREEN_HEIGHT - totalH) / 2.0f;
    if (baseY < 20) baseY = 20;
    
    drawTitle("CHOOSE YOUR AVATAR", (int)baseY, 36, COL_ACCENT);
    
    const char* pickMsg = "Select a profile picture for your account";
    DrawText(pickMsg, SCREEN_WIDTH/2 - MeasureText(pickMsg, 18)/2, (int)(baseY + 65), 18, COL_TEXT_DIM);

    // Grid of 10 profile pics (2 rows of 5)
    float startX = SCREEN_WIDTH/2 - gridW/2;
    float startY = baseY + 100;

    for (int i = 0; i < 10; i++) {
        int row = i / 5;
        int col = i % 5;
        float cx = startX + col * cellW + cellW/2;
        float cy = startY + row * cellH + 60;
        
        Rectangle card = {startX + col * cellW + 8, startY + row * cellH + 5, cellW - 16, cellH - 10};
        
        Vector2 mouse = GetMousePosition();
        bool hover = CheckCollisionPointRec(mouse, card);
        bool selected = (u->profilePic == i);
        
        Color cardBg = selected ? Fade(COL_ACCENT, 0.2f) : (hover ? COL_BG_CARD_HOV : COL_BG_CARD);
        Color cardBorder = selected ? COL_ACCENT : (hover ? COL_ACCENT_DIM : COL_SEPARATOR);
        
        DrawRectangleRounded(card, 0.1f, 6, cardBg);
        DrawRectangleRoundedLinesEx(card, 0.1f, 6, selected ? 3.0f : 1.0f, cardBorder);
        
        drawProfilePic(i, cx, cy, 40);
        
        int nameW = MeasureText(PFP_NAMES[i], 16);
        DrawText(PFP_NAMES[i], (int)(cx - nameW/2), (int)(cy + 50), 16, selected ? COL_ACCENT : COL_TEXT_DIM);
        
        if (selected) {
            const char* eqText = "EQUIPPED";
            DrawText(eqText, (int)(cx - MeasureText(eqText, 12)/2), (int)(cy + 68), 12, COL_SUCCESS);
        }
        
        if (hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            u->profilePic = i;
            g_auth.addExp(*u, 0); // Trigger save
        }
    }

    if (drawThemedButton({(float)SCREEN_WIDTH/2 - 100, startY + cellH * 2 + 20, 200, 45}, "Back to Menu", true)) {
        g_appState = STATE_MAIN_MENU;
    }
}

// ─── Main Menu (Spread Layout) ──────────────────────────────────────────────


void drawOnlineMenu() {
    drawChessBackground();

    // Timer for copy feedback
    if (g_copiedRoomCode) {
        g_copiedRoomTimer -= GetFrameTime();
        if (g_copiedRoomTimer <= 0.0f) g_copiedRoomCode = false;
    }

    float cx = SCREEN_WIDTH / 2.0f;
    float cy = SCREEN_HEIGHT / 2.0f;

    drawTitle("ONLINE MULTIPLAYER", 36, 44, COL_ACCENT);

    const char* sub = "Play live chess across PC and Mobile via Room Codes";
    DrawText(sub, (int)(cx - MeasureText(sub, 18) / 2), 92, 18, COL_TEXT_DIM);

    // Alert / Status message bar if any
    if (strlen(g_onlineMsg) > 0) {
        Color msgCol = (strncmp(g_onlineMsg, "Error", 5) == 0 || strstr(g_onlineMsg, "not found") || strstr(g_onlineMsg, "full") || strstr(g_onlineMsg, "Could not")) ? COL_DANGER : COL_SUCCESS;
        int mw = MeasureText(g_onlineMsg, 18);
        DrawRectangleRounded({cx - mw/2.0f - 16, 122, (float)mw + 32, 34}, 0.2f, 4, Fade(msgCol, 0.15f));
        DrawRectangleRoundedLinesEx({cx - mw/2.0f - 16, 122, (float)mw + 32, 34}, 0.2f, 4, 1.5f, msgCol);
        DrawText(g_onlineMsg, (int)(cx - mw / 2), 130, 18, msgCol);
    }

#ifdef __EMSCRIPTEN__
    if (g_onlineMenuState == ONLINE_MENU_HINT_SETUP) {
        // ─── HOST: HINT COUNT SELECTION CARD ──────────────────────────────────
        float cardW = 480.0f;
        float cardH = 380.0f;
        float cardX = cx - cardW / 2.0f;
        float cardY = cy - cardH / 2.0f + 20.0f;

        DrawRectangleRounded({cardX, cardY, cardW, cardH}, 0.08f, 6, COL_BG_CARD);
        DrawRectangleRoundedLinesEx({cardX, cardY, cardW, cardH}, 0.08f, 6, 2, COL_ACCENT);

        DrawText("HINT SETTINGS", (int)(cx - MeasureText("HINT SETTINGS", 26)/2), (int)(cardY + 22), 26, COL_ACCENT);

        const char* sub1 = "How many hints should each player get?";
        DrawText(sub1, (int)(cx - MeasureText(sub1, 16)/2), (int)(cardY + 62), 16, COL_TEXT_DIM);
        const char* sub2 = "This applies to both you and your opponent.";
        DrawText(sub2, (int)(cx - MeasureText(sub2, 15)/2), (int)(cardY + 84), 15, COL_TEXT_DIM);

        // 4 option buttons: 0, 3, 5, Unlimited
        struct HintOption { int val; const char* label; const char* desc; };
        static const HintOption opts[] = {
            { 0,  "No Hints",  "Neither player can use hints" },
            { 3,  "3 Hints",   "3 suggestions each per game" },
            { 5,  "5 Hints",   "5 suggestions each per game" },
            {-1,  "Unlimited", "Use as many hints as you like" }
        };
        float btnW = cardW - 60.0f;
        float btnH = 46.0f;
        float btnGap = 12.0f;
        float firstBtnY = cardY + 118.0f;

        for (int i = 0; i < 4; i++) {
            float by = firstBtnY + i * (btnH + btnGap);
            bool selected = (g_onlineHintCount == opts[i].val);

            Color borderCol = selected ? COL_ACCENT : COL_SEPARATOR;
            Color bgCol     = selected ? Fade(COL_ACCENT, 0.18f) : COL_BG_CARD_HOV;
            Vector2 mouse = GetMousePosition();
            Rectangle btnRect = {cardX + 30, by, btnW, btnH};
            bool hover = CheckCollisionPointRec(mouse, btnRect);
            if (hover) bgCol = selected ? Fade(COL_ACCENT, 0.28f) : COL_BG_CARD_HOV;

            DrawRectangleRounded(btnRect, 0.12f, 6, bgCol);
            DrawRectangleRoundedLinesEx(btnRect, 0.12f, 6, selected ? 2.5f : 1.0f, borderCol);

            // Radio circle
            float ry = by + btnH/2.0f;
            float rx = cardX + 55.0f;
            DrawCircleLines((int)rx, (int)ry, 9, borderCol);
            if (selected) DrawCircle((int)rx, (int)ry, 5, COL_ACCENT);

            // Label and description
            DrawText(opts[i].label, (int)(cardX + 76), (int)(by + 8), 18, selected ? COL_ACCENT : COL_TEXT_LIGHT);
            DrawText(opts[i].desc,  (int)(cardX + 76), (int)(by + 28), 13, COL_TEXT_DIM);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
                g_onlineHintCount = opts[i].val;
            }
        }

        // Confirm button
        float confirmY = cardY + cardH - 58.0f;
        if (drawThemedButton({cx - 120, confirmY, 240, 44}, "Confirm & Create Room", true, 17)) {
            g_onlineMsg[0] = '\0';
            g_onlineMenuState = ONLINE_MENU_CONNECTING;
            snprintf(g_onlineRoomCode, sizeof(g_onlineRoomCode), "...");
            initWebSocketAndCreateRoom(g_onlineHintCount);
        }
        // Back link
        if (drawThemedButton({cx - 55, confirmY + 52, 110, 32}, "< Back", false, 15)) {
            g_onlineMenuState = ONLINE_MENU_DEFAULT;
        }
    }
    else if (g_onlineMenuState == ONLINE_MENU_WAITING) {
        // ─── WAITING ROOM CARD ─────────────────────────────────────────────
        float cardW = 460.0f;
        float cardH = 350.0f;
        float cardX = cx - cardW / 2.0f;
        float cardY = cy - cardH / 2.0f + 30.0f;

        DrawRectangleRounded({cardX, cardY, cardW, cardH}, 0.08f, 6, COL_BG_CARD);
        DrawRectangleRoundedLinesEx({cardX, cardY, cardW, cardH}, 0.08f, 6, 2, COL_ACCENT);

        DrawText("ROOM CREATED!", (int)(cx - MeasureText("ROOM CREATED!", 24) / 2), (int)(cardY + 25), 24, COL_ACCENT);
        
        const char* shareMsg = "Share this 5-character code with your opponent:";
        DrawText(shareMsg, (int)(cx - MeasureText(shareMsg, 16) / 2), (int)(cardY + 62), 16, COL_TEXT_LIGHT);

        // Big Code Badge
        float badgeW = 260.0f;
        float badgeH = 65.0f;
        float badgeX = cx - badgeW / 2.0f;
        float badgeY = cardY + 95.0f;
        DrawRectangleRounded({badgeX, badgeY, badgeW, badgeH}, 0.15f, 6, COL_BG_DARKER);
        DrawRectangleRoundedLinesEx({badgeX, badgeY, badgeW, badgeH}, 0.15f, 6, 2, COL_ACCENT);
        
        int codeW = MeasureText(g_onlineRoomCode, 38);
        DrawText(g_onlineRoomCode, (int)(cx - codeW / 2), (int)(badgeY + 14), 38, COL_ACCENT);

        // Copy Code Button & Shortcut
        bool isCtrlWaiting = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
                             IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
        if (isCtrlWaiting && IsKeyPressed(KEY_C)) {
            copyRoomCodeToClipboard(g_onlineRoomCode);
        }

        const char* copyBtnText = g_copiedRoomCode ? "COPIED TO CLIPBOARD!" : "COPY CODE (Ctrl+C)";
        if (drawThemedButton({cx - 120, cardY + 180, 240, 42}, copyBtnText, true, 16)) {
            copyRoomCodeToClipboard(g_onlineRoomCode);
        }

        // Animated Waiting Text
        int dotCount = ((int)(GetTime() * 2)) % 4;
        char waitText[64];
        snprintf(waitText, sizeof(waitText), "Waiting for opponent to connect%.*s", dotCount, "...");
        DrawText(waitText, (int)(cx - MeasureText(waitText, 17) / 2), (int)(cardY + 242), 17, COL_TEXT_DIM);

        // Cancel Button
        if (drawThemedButton({cx - 80, cardY + 285, 160, 38}, "Cancel Room", false, 16)) {
            sendLeaveRoomOnline();
            g_onlineMsg[0] = '\0';
        }
    }
    else if (g_onlineMenuState == ONLINE_MENU_CONNECTING) {
        // ─── CONNECTING CARD ───────────────────────────────────────────────
        float cardW = 400.0f;
        float cardH = 220.0f;
        float cardX = cx - cardW / 2.0f;
        float cardY = cy - cardH / 2.0f;

        DrawRectangleRounded({cardX, cardY, cardW, cardH}, 0.08f, 6, COL_BG_CARD);
        DrawRectangleRoundedLinesEx({cardX, cardY, cardW, cardH}, 0.08f, 6, 2, COL_ACCENT);

        DrawText("CONNECTING...", (int)(cx - MeasureText("CONNECTING...", 24) / 2), (int)(cardY + 35), 24, COL_ACCENT);
        
        char connStr[64];
        if (strcmp(g_onlineRoomCode, "...") == 0 || strlen(g_onlineRoomCode) == 0) {
            snprintf(connStr, sizeof(connStr), "Creating private room on server...");
        } else {
            snprintf(connStr, sizeof(connStr), "Joining Room: %s", g_onlineRoomCode);
        }
        DrawText(connStr, (int)(cx - MeasureText(connStr, 18) / 2), (int)(cardY + 80), 18, COL_TEXT_LIGHT);

        if (drawThemedButton({cx - 70, cardY + 140, 140, 38}, "Cancel", false, 16)) {
            sendLeaveRoomOnline();
            g_onlineMsg[0] = '\0';
        }
    }
    else {
        // ─── DEFAULT TWO-CARD LAYOUT ───────────────────────────────────────
        float cardW = 340.0f;
        float cardH = 340.0f;
        float gap = 40.0f;
        float card1X = cx - cardW - gap / 2.0f;
        float card2X = cx + gap / 2.0f;
        float cardY = cy - cardH / 2.0f + 25.0f;

        // CARD 1: HOST MATCH
        DrawRectangleRounded({card1X, cardY, cardW, cardH}, 0.08f, 6, COL_BG_CARD);
        DrawRectangleRoundedLinesEx({card1X, cardY, cardW, cardH}, 0.08f, 6, 2, COL_SEPARATOR);

        DrawText("CREATE A ROOM", (int)(card1X + cardW/2 - MeasureText("CREATE A ROOM", 22)/2), (int)(cardY + 30), 22, COL_ACCENT);
        
        const char* hostDesc1 = "Generate a private 5-digit code";
        const char* hostDesc2 = "and invite your friend on";
        const char* hostDesc3 = "Mobile or PC browser.";
        DrawText(hostDesc1, (int)(card1X + cardW/2 - MeasureText(hostDesc1, 15)/2), (int)(cardY + 80), 15, COL_TEXT_DIM);
        DrawText(hostDesc2, (int)(card1X + cardW/2 - MeasureText(hostDesc2, 15)/2), (int)(cardY + 105), 15, COL_TEXT_DIM);
        DrawText(hostDesc3, (int)(card1X + cardW/2 - MeasureText(hostDesc3, 15)/2), (int)(cardY + 130), 15, COL_TEXT_DIM);

        DrawText("You will play as WHITE", (int)(card1X + cardW/2 - MeasureText("You will play as WHITE", 16)/2), (int)(cardY + 180), 16, COL_TEXT_LIGHT);

        if (drawThemedButton({card1X + 45, cardY + 240, cardW - 90, 48}, "Create Room", true, 19)) {
            g_onlineMsg[0] = '\0';
            // Go to hint-selection first; actual WebSocket connect happens after
            g_onlineMenuState = ONLINE_MENU_HINT_SETUP;
        }

        // CARD 2: JOIN MATCH
        DrawRectangleRounded({card2X, cardY, cardW, cardH}, 0.08f, 6, COL_BG_CARD);
        DrawRectangleRoundedLinesEx({card2X, cardY, cardW, cardH}, 0.08f, 6, 2, COL_SEPARATOR);

        DrawText("JOIN A ROOM", (int)(card2X + cardW/2 - MeasureText("JOIN A ROOM", 22)/2), (int)(cardY + 30), 22, COL_ACCENT);
        
        const char* joinDesc = "Enter the 5-digit code from your friend:";
        DrawText(joinDesc, (int)(card2X + cardW/2 - MeasureText(joinDesc, 14)/2), (int)(cardY + 75), 14, COL_TEXT_DIM);

        // Input Box for Join Code
        float inputW = 170.0f;
        float inputX = card2X + 35.0f;
        float inputY = cardY + 115.0f;
        drawThemedTextBox({inputX, inputY, inputW, 45}, g_onlineJoinCode, 10, g_onlineJoinActive, false, false);
        
        // Auto uppercase input
        for (int i = 0; g_onlineJoinCode[i]; i++) {
            g_onlineJoinCode[i] = (char)toupper(g_onlineJoinCode[i]);
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            g_onlineJoinActive = CheckCollisionPointRec(GetMousePosition(), {inputX, inputY, inputW, 45});
        }

        // Global card shortcut: Ctrl+V anywhere on join card pastes code, Ctrl+C copies
        bool isCtrlJoin = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
                          IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
        if (isCtrlJoin && IsKeyPressed(KEY_V)) {
            pasteRoomCodeFromClipboard();
            g_onlineJoinActive = true;
        }
        if (isCtrlJoin && IsKeyPressed(KEY_C) && strlen(g_onlineJoinCode) > 0) {
            copyRoomCodeToClipboard(g_onlineJoinCode);
        }

        // Paste Button
        if (drawThemedButton({inputX + inputW + 10, inputY, 80, 45}, "Paste", false, 15)) {
            pasteRoomCodeFromClipboard();
            g_onlineJoinActive = true;
        }
        DrawText("(Ctrl+V to paste)", (int)(inputX + 2), (int)(inputY + 48), 12, COL_TEXT_DIM);

        DrawText("You will play as BLACK", (int)(card2X + cardW/2 - MeasureText("You will play as BLACK", 16)/2), (int)(cardY + 185), 16, COL_TEXT_LIGHT);

        // Join Room Action (Triggered by button or Enter key)
        bool triggerJoin = drawThemedButton({card2X + 45, cardY + 240, cardW - 90, 48}, "Join Room", true, 19);
        if (g_onlineJoinActive && IsKeyPressed(KEY_ENTER)) {
            triggerJoin = true;
        }

        if (triggerJoin) {
            int len = (int)strlen(g_onlineJoinCode);
            while (len > 0 && g_onlineJoinCode[len-1] == ' ') g_onlineJoinCode[--len] = '\0';
            
            if (len == 0) {
                snprintf(g_onlineMsg, sizeof(g_onlineMsg), "Please enter a 5-letter Room Code.");
            } else {
                g_onlineMsg[0] = '\0';
                snprintf(g_onlineRoomCode, sizeof(g_onlineRoomCode), "%s", g_onlineJoinCode);
                g_onlineMenuState = ONLINE_MENU_CONNECTING;
                initWebSocketAndJoinRoom(g_onlineJoinCode);
            }
        }

        // Back Button
        if (drawThemedButton({cx - 90, cardY + cardH + 25, 180, 40}, "Back to Menu", false, 16)) {
            g_onlineMsg[0] = '\0';
            g_appState = STATE_MAIN_MENU;
        }
    }
#else
    DrawText("Online play is supported in the Web version.", (int)(cx - 180), (int)cy, 18, COL_TEXT_DIM);
    if (drawThemedButton({cx - 80, cy + 60, 160, 40}, "Back to Menu")) {
        g_appState = STATE_MAIN_MENU;
    }
#endif
}

void drawMainMenu() {
    drawChessBackground();
    
    // Profile at top right with logout (drawn first, always top-right)
    drawProfileTopRight();
    
    // ─── Header: Moved to the Top and Made Bigger ────────────────────────────
    int titleSize = SCREEN_WIDTH > 1400 ? 68 : (SCREEN_WIDTH > 1150 ? 54 : 46);
    float titleY = 22.0f;
    drawTitle("CHESSVERSE 2D", (int)titleY, titleSize, COL_ACCENT);

    // Subtitle ("Master the Board" - larger)
    int subSize = SCREEN_WIDTH > 1400 ? 24 : (SCREEN_WIDTH > 1150 ? 20 : 17);
    float subY = titleY + (float)titleSize + 14.0f;
    const char* subtitle = "Master the Board";
    DrawText(subtitle, SCREEN_WIDTH/2 - MeasureText(subtitle, subSize)/2, (int)subY, subSize, COL_TEXT_DIM);

    // User status / EXP badge (larger and styled)
    int badgeSize = SCREEN_WIDTH > 1400 ? 18 : 14;
    float badgeY = subY + (float)subSize + 12.0f;
    User* u = g_auth.getCurrentUser();
    if (u) {
        char welcomeMsg[128];
        RankInfo rank = getRank(u->exp);
        snprintf(welcomeMsg, sizeof(welcomeMsg), "%s   |   %s   |   %d EXP   |   %d Pts   |   Streak: %d", 
                 u->username, rank.title, u->exp, u->points, u->streak);
        int wmW = MeasureText(welcomeMsg, badgeSize);
        float pillW = (float)wmW + 36;
        float pillH = (float)badgeSize + 14;
        float pillX = (float)SCREEN_WIDTH/2 - pillW/2;
        DrawRectangleRounded({pillX, badgeY - 5, pillW, pillH}, 0.5f, 6, Fade(COL_BG_CARD, 0.85f));
        DrawRectangleRoundedLinesEx({pillX, badgeY - 5, pillW, pillH}, 0.5f, 6, 1, Fade(COL_ACCENT_DIM, 0.6f));
        DrawText(welcomeMsg, SCREEN_WIDTH/2 - wmW/2, (int)badgeY, badgeSize, COL_ACCENT);
    }

    // ─── Body Area Layout (Lowered for clean breathing room) ─────────────────
    float bodyY = badgeY + (float)badgeSize + 32.0f;
    if (bodyY < 185.0f) bodyY = 185.0f;
    Vector2 mouse = GetMousePosition();

    // Determine total card height to fit screen comfortably
    float bottomMargin = SCREEN_HEIGHT > 800 ? 38.0f : 28.0f;
    float cardH = (float)SCREEN_HEIGHT - bodyY - bottomMargin;
    if (cardH > 520.0f) cardH = 520.0f;
    if (cardH < 400.0f) cardH = 400.0f;

    // ─── Left Side: 4 Linear Mode Tabs (Equally Spread Vertically) ───────────
    float leftX = SCREEN_WIDTH > 1400 ? 45.0f : 22.0f;
    float leftW = SCREEN_WIDTH > 1400 ? 270.0f : 225.0f;
    float itemH = cardH > 480.0f ? 76.0f : 68.0f;
    float gapY  = (cardH - 5.0f * itemH) / 4.0f;
    if (gapY < 8.0f) gapY = 8.0f;

    const char* linearTitles[] = {"Play vs Friend", "Ranked Match", "Leaderboard", "Daily Trivia", "Play Online"};
    const char* linearSubs[]   = {"Local 2-Player Match", "Competitive Authenticated", "Global EXP Rankings", "Daily Chess Puzzle", "Multiplayer over Web"};
    const char* linearIcons[]  = {"PP", "VS", "LB", "TR", "WWW"};
    Color linearAccents[] = {
        {100, 180, 220, 255},
        {220, 140, 60,  255},
        {255, 215, 0,   255},
        {160, 120, 220, 255},
        {50, 200, 150, 255}
    };

    for (int i = 0; i < 5; i++) {
        float iy = bodyY + i * (itemH + gapY);
        Rectangle card = {leftX, iy, leftW, itemH};
        bool hover = CheckCollisionPointRec(mouse, card);

        // Shadow & Body
        DrawRectangleRounded({leftX + 2, iy + 2, leftW, itemH}, 0.12f, 6, Fade(BLACK, 0.25f));
        DrawRectangleRounded(card, 0.12f, 6, hover ? COL_BG_CARD_HOV : COL_BG_CARD);
        DrawRectangleRoundedLinesEx(card, 0.12f, 6, 2, hover ? linearAccents[i] : COL_SEPARATOR);

        // Left accent indicator on hover
        if (hover) {
            DrawRectangle((int)leftX + 2, (int)iy + 10, 4, (int)itemH - 20, linearAccents[i]);
        }

        // Icon badge
        float iconBox = itemH - 24.0f;
        Rectangle badge = {leftX + 10, iy + 12, iconBox, iconBox};
        DrawRectangleRounded(badge, 0.2f, 4, Fade(linearAccents[i], hover ? 0.22f : 0.12f));
        DrawRectangleRoundedLinesEx(badge, 0.2f, 4, 1, Fade(linearAccents[i], 0.7f));
        int iconFont = iconBox > 46.0f ? 20 : 18;
        int iw = MeasureText(linearIcons[i], iconFont);
        DrawText(linearIcons[i], (int)(badge.x + (badge.width - iw)/2), (int)(badge.y + (badge.height - iconFont)/2), iconFont, linearAccents[i]);

        // Title & Subtitle
        int titleFontSize = leftW > 250.0f ? 17 : 15;
        int subFontSize = leftW > 250.0f ? 12 : 11;
        float textOffsetX = leftX + 10 + iconBox + 10;
        DrawText(linearTitles[i], (int)textOffsetX, (int)(iy + itemH/2 - 17), titleFontSize, hover ? COL_TEXT_BRIGHT : COL_TEXT_LIGHT);
        DrawText(linearSubs[i], (int)textOffsetX, (int)(iy + itemH/2 + 2), subFontSize, COL_TEXT_DIM);

        // Hover arrow
        if (hover) {
            DrawText(">", (int)(leftX + leftW - 18), (int)(iy + itemH/2 - 9), 18, linearAccents[i]);
        }

        if (hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            switch (i) {
                case 0:
                    g_vsAI = false; g_playerColor = P_WHITE;
                    g_rankedPvp = false; g_player2 = nullptr;
                    g_board.init(); g_gameOver = false; strcpy(g_statusMsg, "Game started.");
                    g_hintLimit = HintLimit(-1);
                    g_appState = STATE_GAME;
                    break;
                case 1:
                    g_appState = STATE_P2_LOGIN;
                    g_p2UsernameInput[0] = '\0';
                    g_p2PasswordInput[0] = '\0';
                    g_p2LoginMsg[0] = '\0';
                    break;
                case 2:
                    g_appState = STATE_LEADERBOARD;
                    break;
                case 3:
                    g_appState = STATE_TRIVIA;
                    break;
                case 4:
#ifdef __EMSCRIPTEN__
                    g_appState = STATE_ONLINE_MENU;
                    g_onlineJoinCode[0] = '\0';
                    g_onlineMsg[0] = '\0';
#else
                    snprintf(g_onlineMsg, sizeof(g_onlineMsg), "Online Play is only available in the Web Version.");
                    g_appState = STATE_ONLINE_MENU;
#endif
                    break;
            }
        }
    }

    // ─── Center: EASY / MEDIUM / HARD Mode Cards (Lowered and Centered) ───────
    float leftBound = leftX + leftW + (SCREEN_WIDTH > 1400 ? 28.0f : 18.0f);
    float rightBound = (float)SCREEN_WIDTH - (SCREEN_WIDTH > 1400 ? 45.0f : 22.0f);
    float availW = rightBound - leftBound;
    float gap = SCREEN_WIDTH > 1400 ? 24.0f : 14.0f;
    float cardW = (availW - gap * 2.0f) / 3.0f;
    if (cardW > 340.0f) cardW = 340.0f;
    if (cardW < 210.0f) cardW = 210.0f;

    float totalCardsW = cardW * 3.0f + gap * 2.0f;
    float cardsStartX = leftBound + (availW - totalCardsW) / 2.0f;

    const char* diffTitles[] = {"EASY", "MEDIUM", "HARD"};
    const char* diffIcons[]  = {"P", "N", "Q"};
    const char* diffTagline[]= {"Beginner Friendly", "Balanced Challenge", "No Mercy Mode"};
    const char* diffDesc1[]  = {"Unlimited hints allowed", "3 hints available", "No hints allowed"};
    const char* diffDesc2[]  = {"Casual pacing", "Standard chess clock", "Maximum depth AI"};
    const char* diffExp[]    = {"+50 EXP on Victory", "+100 EXP on Victory", "+200 EXP on Victory"};
    Color diffAccents[]      = {COL_SUCCESS, {220, 180, 60, 255}, COL_DANGER};

    for (int i = 0; i < 3; i++) {
        float cx = cardsStartX + i * (cardW + gap);
        Rectangle card = {cx, bodyY, cardW, cardH};
        bool hover = CheckCollisionPointRec(mouse, card);

        // Card shadow
        DrawRectangleRounded({cx + 3, bodyY + 3, cardW, cardH}, 0.08f, 6, Fade(BLACK, 0.35f));
        // Card body
        DrawRectangleRounded(card, 0.08f, 6, hover ? COL_BG_CARD_HOV : COL_BG_CARD);
        DrawRectangleRoundedLinesEx(card, 0.08f, 6, 2, hover ? diffAccents[i] : COL_SEPARATOR);

        // Top accent bar
        DrawRectangle((int)(cx + 2), (int)bodyY + 2, (int)(cardW - 4), 6, diffAccents[i]);

        // Big Icon
        int iconSize = cardH > 480.0f ? 58 : 46;
        float iconY = bodyY + (cardH > 480.0f ? 20.0f : 14.0f);
        int iconW = MeasureText(diffIcons[i], iconSize);
        DrawText(diffIcons[i], (int)(cx + cardW/2 - iconW/2), (int)iconY, iconSize, diffAccents[i]);

        // Title
        int titleW = MeasureText(diffTitles[i], 28);
        float titleCardY = iconY + iconSize + 10.0f;
        DrawText(diffTitles[i], (int)(cx + cardW/2 - titleW/2), (int)titleCardY, 28, COL_TEXT_BRIGHT);

        // Separator
        float sepY = titleCardY + 38.0f;
        DrawLine((int)(cx + 24), (int)sepY, (int)(cx + cardW - 24), (int)sepY, COL_SEPARATOR);

        // Tagline
        int tagW = MeasureText(diffTagline[i], 17);
        float tagY = sepY + 12.0f;
        DrawText(diffTagline[i], (int)(cx + cardW/2 - tagW/2), (int)tagY, 17, COL_TEXT_LIGHT);

        // Descriptions
        int l1w = MeasureText(diffDesc1[i], 14);
        int l2w = MeasureText(diffDesc2[i], 14);
        int expW = MeasureText(diffExp[i], 14);
        float desc1Y = tagY + 32.0f;
        float desc2Y = desc1Y + 24.0f;
        float expY   = desc2Y + 28.0f;
        DrawText(diffDesc1[i], (int)(cx + cardW/2 - l1w/2), (int)desc1Y, 14, COL_TEXT_DIM);
        DrawText(diffDesc2[i], (int)(cx + cardW/2 - l2w/2), (int)desc2Y, 14, COL_TEXT_DIM);
        DrawText(diffExp[i],   (int)(cx + cardW/2 - expW/2), (int)expY,   14, COL_ACCENT_DIM);

        // Big glowing PLAY button at bottom of card
        float btnH = cardH > 480.0f ? 46.0f : 40.0f;
        Rectangle playBtn = {cx + 24, bodyY + cardH - btnH - 18, cardW - 48, btnH};
        if (drawThemedButton(playBtn, "PLAY", hover, 19)) {
            Difficulty diffs[] = {EASY, MEDIUM, HARD};
            int hintCounts[] = {-1, 3, 0};
            g_difficulty = diffs[i]; g_vsAI = true; g_playerColor = P_WHITE;
            g_rankedPvp = false; g_player2 = nullptr;
            if (g_ai) delete g_ai; 
            g_ai = new AI(diffs[i]);
            g_board.init(); g_gameOver = false; strcpy(g_statusMsg, "Game started.");
            g_hintLimit = HintLimit(hintCounts[i]);
            g_appState = STATE_GAME;
        }
    }
}

// ─── Main Game Loop Frame ───────────────────────────────────────────────────

void UpdateDrawFrame() {
    SCREEN_WIDTH = GetScreenWidth();
    SCREEN_HEIGHT = GetScreenHeight();
    BOARD_X = (SCREEN_WIDTH - NOTER_WIDTH - BOARD_SIZE) / 2;
    if (BOARD_X < 20) BOARD_X = 20;
    BOARD_Y = (SCREEN_HEIGHT - BOARD_SIZE) / 2;

    BeginDrawing();
    
    switch (g_appState) {
        case STATE_LOGIN:       drawLogin(); break;
        case STATE_MAIN_MENU:   drawMainMenu(); break;
        case STATE_ONLINE_MENU: drawOnlineMenu(); break;
        case STATE_GAME:        handleGameInput(); drawGame(); break;
        case STATE_LEADERBOARD: drawLeaderboard(); break;
        case STATE_TRIVIA:      drawTrivia(); break;
        case STATE_P2_LOGIN:    drawP2Login(); break;
        case STATE_PROFILE_PIC: drawProfilePicSelection(); break;
        default: break;
    }
    
    EndDrawing();
}

// ─── Main Entry Point ───────────────────────────────────────────────────────

int main() {
#ifdef __EMSCRIPTEN__
    InitWindow(1024, 768, "ChessVerse 2D");
    SetTargetFPS(60);
    SCREEN_WIDTH = 1024;
    SCREEN_HEIGHT = 768;
    BOARD_X = (SCREEN_WIDTH - NOTER_WIDTH - BOARD_SIZE) / 2;
    if (BOARD_X < 20) BOARD_X = 20;
    BOARD_Y = (SCREEN_HEIGHT - BOARD_SIZE) / 2;
    loadAssets();
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "ChessVerse 2D");
    SetTargetFPS(60);
    
    // Dynamic dimensions for Fullscreen
    SCREEN_WIDTH = GetMonitorWidth(0);
    SCREEN_HEIGHT = GetMonitorHeight(0);
    // Board on the left, noter panel on the right
    BOARD_X = (SCREEN_WIDTH - NOTER_WIDTH - BOARD_SIZE) / 2;
    if (BOARD_X < 20) BOARD_X = 20;
    BOARD_Y = (SCREEN_HEIGHT - BOARD_SIZE) / 2;
    
    loadAssets();
    
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
    
    unloadAssets();
    CloseWindow();
#endif
    return 0;
}
