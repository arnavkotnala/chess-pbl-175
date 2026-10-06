#pragma once

/*
 * ─── Piece Definitions ───────────────────────────────────────────────────────
 * Piece types, colors, Move struct, and value functions.
 * Used by all chess modules.
 */

#include <cstring>

// Piece type identifiers
enum PieceType { EMPTY = 0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum PieceColor { P_NONE = 0, P_WHITE, P_BLACK };

struct Move {
    int fromRow, fromCol;
    int toRow,   toCol;
    bool isCapture;
    bool isCastle;
    bool isEnPassant;
    bool isPromotion;
    PieceType promoteTo;  // for promotion

    Move() : fromRow(-1), fromCol(-1), toRow(-1), toCol(-1),
             isCapture(false), isCastle(false), isEnPassant(false),
             isPromotion(false), promoteTo(QUEEN) {}

    // Algebraic notation string e.g. "e2e4"
    void toAlgebraic(char* out) const {
        out[0] = (char)('a' + fromCol);
        out[1] = (char)('1' + fromRow);
        out[2] = (char)('a' + toCol);
        out[3] = (char)('1' + toRow);
        out[4] = '\0';
    }
};

// Point values per piece (for game scoring)
inline int piecePoints(PieceType t) {
    switch (t) {
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
    switch (t) {
        case PAWN:   return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK:   return 500;
        case QUEEN:  return 900;
        case KING:   return 20000;
        default:     return 0;
    }
}
