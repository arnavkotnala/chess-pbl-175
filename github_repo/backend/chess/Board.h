#pragma once
#include "Piece.h"
#include <array>
#include <stack>
#include <memory>

// ─── Board Class ──────────────────────────────────────────────────────────────
// Stores the 8x8 grid of piece pointers.
// Row 0 = rank 1 (white back rank), Row 7 = rank 8 (black back rank)
// Col 0 = a-file, Col 7 = h-file

struct Cell {
    PieceType type;
    Color     color;
    bool      hasMoved;

    Cell() : type(EMPTY), color(NONE), hasMoved(false) {}
    Cell(PieceType t, Color c) : type(t), color(c), hasMoved(false) {}
    bool isEmpty() const { return type == EMPTY; }
};

struct GameState {
    Cell      grid[8][8];
    Color     currentTurn;
    int       enPassantCol;   // -1 if none; en passant target col after 2-square pawn advance
    int       enPassantRow;
    bool      whiteKingsideCastle,  whiteQueensideCastle;
    bool      blackKingsideCastle,  blackQueensideCastle;
    int       halfMoveClock;  // for 50-move rule
    int       fullMoveNumber;
};

class Board {
public:
    GameState state;
    stack<pair<GameState, Move>> history;  // for undo (Stack DSA)

    Board();

    // Setup standard starting position
    void init();

    // Access
    Cell&       at(int r, int c)       { return state.grid[r][c]; }
    const Cell& at(int r, int c) const { return state.grid[r][c]; }
    bool inBounds(int r, int c) const  { return r>=0&&r<8&&c>=0&&c<8; }

    // Move application
    bool applyMove(const Move& m);   // returns true if move is legal
    void undoMove();

    // Legal move generation for a color
    vector<Move> getLegalMoves(Color c) const;

    // Check detection
    bool isInCheck(Color c) const;
    bool isCheckmate(Color c) const;
    bool isStalemate(Color c) const;

    // Find king position
    pair<int,int> kingPos(Color c) const;

    // Is square attacked by given color?
    bool isAttacked(int r, int c, Color byColor) const;

    // Board evaluation score (positive = white advantage)
    int evaluate() const;

    // Pretty print (console debug)
    void print() const;

    // Serialize board state to JSON-like string for JS frontend
    string toJSON() const;

    // Get piece at position as string code for JS e.g. "wK", "bP"
    string pieceCode(int r, int c) const;

private:
    // Generate pseudo-legal moves for all pieces of a color
    vector<Move> getPseudoMoves(Color c, bool checkCastle=true) const;

    // Generate moves for a specific piece type at position
    vector<Move> getPieceMoves(int r, int c, bool checkCastle=true) const;

    // Apply move without legality check (internal)
    void applyMoveRaw(GameState& s, const Move& m) const;
};
