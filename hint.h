#pragma once

/*
 * ─── Hint Engine ──────────────────────────────────────────────────────────────
 * Suggests the best move for the current player.
 * Uses the same minimax logic as Medium AI (depth 3).
 *
 * Hint limits per game mode:
 *   Easy   → unlimited
 *   Medium → 3 per game
 *   Hard   → 0 (disabled)
 */

#include "board.h"
#include "ai.h"
#include <cstdio>

struct HintLimit {
    int  remaining;    // -1 = unlimited
    bool unlimited;

    HintLimit(int n = -1) {
        if (n < 0) { unlimited = true; remaining = 0; }
        else       { unlimited = false; remaining = n; }
    }
    bool canUse() const { return unlimited || remaining > 0; }
    void use()          { if (!unlimited && remaining > 0) remaining--; }
};

class HintEngine {
public:
    // Returns best move suggestion, or empty Move (fromRow=-1) if no hints left
    static Move getHint(Board& board, PieceColor color, HintLimit& limit) {
        if (!limit.canUse()) {
            Move none; none.fromRow = -1; return none;
        }
        limit.use();

        AI hintAI(MEDIUM);
        return hintAI.getBestMove(board, color);
    }

    // Get hint remaining count as string (for display)
    static void hintLabel(const HintLimit& limit, char* out) {
        if (limit.unlimited) {
            out[0] = 'U'; out[1] = 'N'; out[2] = 'L'; out[3] = '\0';
        } else {
            sprintf(out, "%d", limit.remaining);
        }
    }
};
