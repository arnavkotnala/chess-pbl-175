#pragma once
#include <vector>
#include <string>
using namespace std;

// Piece type identifiers
enum PieceType { EMPTY=0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum Color     { NONE=0, WHITE, BLACK };

struct Move {
    int fromRow, fromCol;
    int toRow,   toCol;
    bool isCapture;
    bool isCastle;
    bool isEnPassant;
    bool isPromotion;
    PieceType promoteTo;  // for promotion

    // Algebraic notation string e.g. "e2e4"
    string toAlgebraic() const {
        string s = "";
        s += (char)('a' + fromCol);
        s += (char)('1' + fromRow);
        s += (char)('a' + toCol);
        s += (char)('1' + toRow);
        return s;
    }
};

// Point values per piece (for scoring)
inline int piecePoints(PieceType t) {
    switch(t) {
        case PAWN:   return 10;
        case KNIGHT: return 30;
        case BISHOP: return 30;
        case ROOK:   return 50;
        case QUEEN:  return 90;
        case KING:   return 0;
        default:     return 0;
    }
}

// Material value for AI evaluation
inline int pieceValue(PieceType t) {
    switch(t) {
        case PAWN:   return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK:   return 500;
        case QUEEN:  return 900;
        case KING:   return 20000;
        default:     return 0;
    }
}

// ─── Base Piece Class ─────────────────────────────────────────────────────────
class Piece {
public:
    PieceType type;
    Color     color;
    bool      hasMoved;

    Piece() : type(EMPTY), color(NONE), hasMoved(false) {}
    Piece(PieceType t, Color c) : type(t), color(c), hasMoved(false) {}

    virtual ~Piece() {}

    // Returns all pseudo-legal moves (does not check for leaving king in check)
    virtual vector<Move> getMoves(int row, int col, const class Board& board) const = 0;

    // Unicode symbol for display
    virtual string symbol() const = 0;

    // Clone (for AI board copies)
    virtual Piece* clone() const = 0;
};
