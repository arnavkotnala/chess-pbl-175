#pragma once
#include "Board.h"
#include <climits>
#include <algorithm>
#include <random>
#include <chrono>

// ─── AI Engine (Swastik) ──────────────────────────────────────────────────────
// Implements Minimax with Alpha-Beta Pruning + Iterative Deepening +
// Killer Move Heuristic for Easy/Medium/Hard difficulty.
//
// Optimizations applied:
//   1. Iterative Deepening: search depth 1→N for better move ordering
//   2. Killer Moves: store refutation moves per depth for better pruning
//   3. Move Ordering: captures first (MVV-LVA style) + killer moves
//   4. Fixed randomness: uses C++ <random> instead of srand/rand

enum Difficulty { EASY, MEDIUM, HARD };

// Maximum search depth supported
static const int MAX_SEARCH_DEPTH = 10;

class AI {
public:
    Difficulty difficulty;
    int        nodesEvaluated;   // for performance reporting

    explicit AI(Difficulty d = EASY) : difficulty(d), nodesEvaluated(0) {
        clearKillers();
    }

    // Returns best move for the given color
    // Easy   → random valid move
    // Medium → iterative deepening to depth 3
    // Hard   → iterative deepening to depth 5 + killer heuristic
    Move getBestMove(Board& board, Color color) {
        nodesEvaluated = 0;
        vector<Move> legal = board.getLegalMoves(color);
        if(legal.empty()) { return Move{}; }

        if(difficulty == EASY) {
            return randomMove(legal);
        }

        int maxDepth = (difficulty == MEDIUM) ? 3 : 5;
        bool maximizing = (color == WHITE);
        clearKillers();

        Move bestMove = legal[0];

        // ── Iterative Deepening ──────────────────────────────────────────
        // Search depth 1, then 2, ... up to maxDepth.
        // Each shallow search improves move ordering for deeper searches,
        // making alpha-beta pruning dramatically more effective.
        for(int depth = 1; depth <= maxDepth; depth++) {
            int bestScore = maximizing ? INT_MIN : INT_MAX;
            Move iterBest = legal[0];

            for(auto& m : legal) {
                board.applyMove(m);
                int score = minimax(board, depth-1, INT_MIN, INT_MAX, !maximizing, 0);
                board.undoMove();

                if(maximizing && score > bestScore)  { bestScore=score; iterBest=m; }
                if(!maximizing && score < bestScore)  { bestScore=score; iterBest=m; }
            }
            bestMove = iterBest;

            // Reorder legal moves: put best move from this iteration first
            // This gives the next deeper iteration a huge head start
            for(int i=0;i<(int)legal.size();i++) {
                if(legal[i].fromRow==bestMove.fromRow && legal[i].fromCol==bestMove.fromCol &&
                   legal[i].toRow==bestMove.toRow && legal[i].toCol==bestMove.toCol) {
                    swap(legal[0], legal[i]);
                    break;
                }
            }
        }
        return bestMove;
    }

    // Point multiplier for scoring
    float multiplier() const {
        switch(difficulty) {
            case EASY:   return 1.5f;
            case MEDIUM: return 2.0f;
            case HARD:   return 3.0f;
        }
        return 1.0f;
    }

private:
    // ── Killer Move Table ─────────────────────────────────────────────────
    // Stores 2 "killer" moves per depth level — moves that caused a beta
    // cutoff previously. These are quiet (non-capture) moves that are
    // likely to be strong in sibling positions at the same depth.
    struct KillerEntry {
        int fromRow, fromCol, toRow, toCol;
        bool valid;
    };
    KillerEntry killers[2][MAX_SEARCH_DEPTH]; // 2 slots per depth

    void clearKillers() {
        for(int i=0;i<2;i++) for(int d=0;d<MAX_SEARCH_DEPTH;d++)
            killers[i][d].valid = false;
    }

    bool isKiller(const Move& m, int depth) const {
        if(depth >= MAX_SEARCH_DEPTH) return false;
        for(int i=0;i<2;i++) {
            auto& k = killers[i][depth];
            if(k.valid && k.fromRow==m.fromRow && k.fromCol==m.fromCol &&
               k.toRow==m.toRow && k.toCol==m.toCol) return true;
        }
        return false;
    }

    void storeKiller(const Move& m, int depth) {
        if(depth >= MAX_SEARCH_DEPTH || m.isCapture) return; // only quiet moves
        // Shift slot 1 → slot 0, store new in slot 1
        killers[0][depth] = killers[1][depth];
        killers[1][depth] = {m.fromRow, m.fromCol, m.toRow, m.toCol, true};
    }

    // ── Random move for Easy (fixed: proper C++ random) ───────────────────
    Move randomMove(const vector<Move>& moves) {
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<int> dist(0, (int)moves.size()-1);
        return moves[dist(rng)];
    }

    // ── Move Ordering ─────────────────────────────────────────────────────
    // Priority: 1. Captures (MVV-LVA: high-value victim first)
    //           2. Killer moves at this depth
    //           3. All other quiet moves
    void orderMoves(vector<Move>& moves, const Board& board, int depth) {
        sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            int scoreA = 0, scoreB = 0;

            // Captures get high priority (MVV-LVA: Most Valuable Victim - Least Valuable Attacker)
            if(a.isCapture) {
                const Cell& victim = board.at(a.toRow, a.toCol);
                const Cell& attacker = board.at(a.fromRow, a.fromCol);
                scoreA = 10000 + pieceValue(victim.type) - pieceValue(attacker.type)/10;
            }
            if(b.isCapture) {
                const Cell& victim = board.at(b.toRow, b.toCol);
                const Cell& attacker = board.at(b.fromRow, b.fromCol);
                scoreB = 10000 + pieceValue(victim.type) - pieceValue(attacker.type)/10;
            }

            // Killer moves get medium priority (above quiet, below captures)
            if(!a.isCapture && isKiller(a, depth)) scoreA = 5000;
            if(!b.isCapture && isKiller(b, depth)) scoreB = 5000;

            return scoreA > scoreB;
        });
    }

    // ── Minimax with Alpha-Beta Pruning + Killer Heuristic ────────────────
    int minimax(Board& board, int depth, int alpha, int beta, bool maximizing, int ply) {
        nodesEvaluated++;

        // Terminal nodes
        Color turn = board.state.currentTurn;
        if(depth == 0) return board.evaluate();

        vector<Move> moves = board.getLegalMoves(turn);
        if(moves.empty()) {
            if(board.isInCheck(turn)) return maximizing ? -50000+ply : 50000-ply;
            return 0; // stalemate
        }

        // Order moves for better pruning
        orderMoves(moves, board, ply);

        if(maximizing) {
            int maxEval = INT_MIN;
            for(auto& m : moves) {
                board.applyMove(m);
                int eval = minimax(board, depth-1, alpha, beta, false, ply+1);
                board.undoMove();
                maxEval = max(maxEval, eval);
                alpha   = max(alpha, eval);
                if(beta <= alpha) {
                    storeKiller(m, ply); // ✂ Beta cutoff — store killer
                    break;
                }
            }
            return maxEval;
        } else {
            int minEval = INT_MAX;
            for(auto& m : moves) {
                board.applyMove(m);
                int eval = minimax(board, depth-1, alpha, beta, true, ply+1);
                board.undoMove();
                minEval = min(minEval, eval);
                beta    = min(beta, eval);
                if(beta <= alpha) {
                    storeKiller(m, ply); // ✂ Alpha cutoff — store killer
                    break;
                }
            }
            return minEval;
        }
    }
};
