#pragma once
#include "Board.h"
#include "AI.h"

// ─── Hint Engine (Priyani) ────────────────────────────────────────────────────
// Suggests the best move for the current player.
// Uses the same minimax logic as Hard AI but always looks ahead depth 4.
//
// Hint limits per game mode:
//   Easy   → unlimited
//   Medium → 3 per game
//   Hard   → 0 (disabled)
//
// In Versus mode each player has an independent hint counter.

struct HintLimit {
    int remaining;    // -1 = unlimited
    bool unlimited;

    HintLimit(int n=-1) {
        if(n < 0) { unlimited=true; remaining=0; }
        else       { unlimited=false; remaining=n; }
    }
    bool canUse() const { return unlimited || remaining > 0; }
    void use()          { if(!unlimited && remaining>0) remaining--; }
};

class HintEngine {
public:
    // Returns best move suggestion, or empty Move (fromRow=-1) if no hints left
    static Move getHint(Board& board, Color color, HintLimit& limit) {
        if(!limit.canUse()) {
            Move none; none.fromRow=-1; return none;
        }
        limit.use();

        // Use AI at depth 3 for hint quality and performance
        AI hintAI(MEDIUM);
        return hintAI.getBestMove(board, color);
    }

    // Get hint remaining count as string (for display)
    static string hintLabel(const HintLimit& limit) {
        if(limit.unlimited) return "∞";
        return to_string(limit.remaining);
    }
};
