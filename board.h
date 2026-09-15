#pragma once

/*
 * ─── Board Class ──────────────────────────────────────────────────────────────
 * Stores the 8x8 grid of cells.
 * Uses C Stack (from dsa/stack.h) for move undo history.
 *
 * Row 0 = rank 1 (P_WHITE back rank), Row 7 = rank 8 (P_BLACK back rank)
 * Col 0 = a-file, Col 7 = h-file
 */

#include "piece.h"

extern "C" {
    #include "../dsa/stack.h"
}

#define MAX_MOVES 256  /* max pseudo-legal moves in a position */

struct Cell {
    PieceType type;
    PieceColor    color;
    bool      hasMoved;

    Cell() : type(EMPTY), color(P_NONE), hasMoved(false) {}
    Cell(PieceType t, PieceColor c) : type(t), color(c), hasMoved(false) {}
    bool isEmpty() const { return type == EMPTY; }
};

struct GameState {
    Cell  grid[8][8];
    PieceColor currentTurn;
    int   enPassantCol;   // -1 if P_NONE
    int   enPassantRow;
    bool  whiteKingsideCastle,  whiteQueensideCastle;
    bool  blackKingsideCastle,  blackQueensideCastle;
    int   halfMoveClock;
    int   fullMoveNumber;
};

/* Move array — dynamic array of Moves (avoids std::vector) */
struct MoveList {
    Move  moves[MAX_MOVES];
    int   count;

    MoveList() : count(0) {}
    void add(const Move& m) { if (count < MAX_MOVES) moves[count++] = m; }
    bool empty() const { return count == 0; }
};

class Board {
public:
    GameState state;
    Stack*    history;  // C Stack DSA for undo (stores GameState snapshots)
    Move      moveHistory[500];
    int       moveHistoryCount;

    // Snapshots for move-by-move replay (Prev/Next viewer)
    GameState stateSnapshots[501];  // snapshot[0] = initial position, snapshot[i] = state after move i
    int       snapshotCount;

    const GameState& getSnapshot(int idx) const { return stateSnapshots[idx]; }

    Board();
    ~Board();

    // Copy constructor (for AI board copies)
    Board(const Board& other);
    Board& operator=(const Board& other);

    // Setup standard starting position
    void init();

    // Access
    Cell&       at(int r, int c)       { return state.grid[r][c]; }
    const Cell& at(int r, int c) const { return state.grid[r][c]; }
    bool inBounds(int r, int c) const  { return r >= 0 && r < 8 && c >= 0 && c < 8; }

    // Move application
    bool applyMove(const Move& m);   // returns true if move is legal
    void undoMove();

    // Legal move generation for a PieceColor
    MoveList getLegalMoves(PieceColor c) const;

    // Check detection
    bool isInCheck(PieceColor c) const;
    bool isCheckmate(PieceColor c) const;
    bool isStalemate(PieceColor c) const;

    // Find king position
    void kingPos(PieceColor c, int& outRow, int& outCol) const;

    // Is square attacked by given PieceColor?
    bool isAttacked(int r, int c, PieceColor byColor) const;

    // Board evaluation score (positive = P_WHITE advantage)
    int evaluate() const;

    // Pretty print to console with Unicode pieces
    void print() const;

    // Piece code string at position e.g. "wK", "bP"
    void pieceCode(int r, int c, char* out) const;

private:
    // Generate pseudo-legal moves for all pieces of a PieceColor
    MoveList getPseudoMoves(PieceColor c, bool checkCastle = true) const;

    // Generate moves for a specific piece at position
    void getPieceMoves(int r, int c, MoveList& moves, bool checkCastle = true) const;

    // Apply move without legality check (internal)
    void applyMoveRaw(GameState& s, const Move& m) const;
};
