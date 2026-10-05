#pragma once

/*
 * ─── AI Engine ────────────────────────────────────────────────────────────────
 * Implements Minimax with Alpha-Beta Pruning + Iterative Deepening +
 * Killer Move Heuristic for Easy/Medium/Hard difficulty.
 *
 * Optimizations applied:
 *   1. Iterative Deepening: search depth 1→N for better move ordering
 *   2. Killer Moves: store refutation moves per depth for better pruning
 *   3. Move Ordering: captures first (MVV-LVA style) + killer moves
 */

#include "board.h"
#include <climits>
#include <cstdlib>
#include <ctime>
#include <chrono>

enum Difficulty { EASY, MEDIUM, HARD };

static const int MAX_SEARCH_DEPTH = 10;

class AI {
public:
    Difficulty difficulty;
    int        nodesEvaluated;
    bool       timeout;

    explicit AI(Difficulty d = EASY) : difficulty(d), nodesEvaluated(0), timeout(false) {
        clearKillers();
    }

    // Returns best move for the given PieceColor
    Move getBestMove(Board& board, PieceColor PieceColor) {
        nodesEvaluated = 0;
        MoveList legal = board.getLegalMoves(PieceColor);
        if (legal.empty()) { return Move(); }

        if (difficulty == EASY) {
            return randomMove(legal);
        }

        int maxDepth = (difficulty == MEDIUM) ? 3 : 5;
        bool maximizing = (PieceColor == P_WHITE);
        clearKillers();

        auto startTime = std::chrono::high_resolution_clock::now();
        Move bestMove = legal.moves[0];
        timeout = false;

        // ── Iterative Deepening ──────────────────────────────────────────
        for (int depth = 1; depth <= maxDepth; depth++) {
            int bestScore = maximizing ? INT_MIN : INT_MAX;
            Move iterBest = legal.moves[0];
            bool iterationComplete = true;

            for (int i = 0; i < legal.count; i++) {
                Move& m = legal.moves[i];
                board.applyMove(m);
                int score = minimax(board, depth - 1, INT_MIN, INT_MAX, !maximizing, 0, startTime);
                board.undoMove();

                if (timeout) { iterationComplete = false; break; }

                if (maximizing  && score > bestScore) { bestScore = score; iterBest = m; }
                if (!maximizing && score < bestScore) { bestScore = score; iterBest = m; }
            }
            
            if (!iterationComplete) break; // discard aborted depth
            bestMove = iterBest;

            // Reorder: put best move first for next iteration
            for (int i = 0; i < legal.count; i++) {
                if (legal.moves[i].fromRow == bestMove.fromRow &&
                    legal.moves[i].fromCol == bestMove.fromCol &&
                    legal.moves[i].toRow   == bestMove.toRow   &&
                    legal.moves[i].toCol   == bestMove.toCol) {
                    Move tmp = legal.moves[0];
                    legal.moves[0] = legal.moves[i];
                    legal.moves[i] = tmp;
                    break;
                }
            }
        }
        return bestMove;
    }

    // Point multiplier for scoring
    float multiplier() const {
        switch (difficulty) {
            case EASY:   return 1.5f;
            case MEDIUM: return 2.0f;
            case HARD:   return 3.0f;
        }
        return 1.0f;
    }

private:
    // ── Killer Move Table ─────────────────────────────────────────────────
    struct KillerEntry {
        int fromRow, fromCol, toRow, toCol;
        bool valid;
    };
    KillerEntry killers[2][MAX_SEARCH_DEPTH];

    void clearKillers() {
        for (int i = 0; i < 2; i++)
            for (int d = 0; d < MAX_SEARCH_DEPTH; d++)
                killers[i][d].valid = false;
    }

    bool isKiller(const Move& m, int depth) const {
        if (depth >= MAX_SEARCH_DEPTH) return false;
        for (int i = 0; i < 2; i++) {
            const KillerEntry& k = killers[i][depth];
            if (k.valid && k.fromRow == m.fromRow && k.fromCol == m.fromCol &&
                k.toRow == m.toRow && k.toCol == m.toCol) return true;
        }
        return false;
    }

    void storeKiller(const Move& m, int depth) {
        if (depth >= MAX_SEARCH_DEPTH || m.isCapture) return;
        killers[0][depth] = killers[1][depth];
        killers[1][depth] = {m.fromRow, m.fromCol, m.toRow, m.toCol, true};
    }

    // ── Random move for Easy ──────────────────────────────────────────────
    Move randomMove(const MoveList& moves) {
        static bool seeded = false;
        if (!seeded) { srand((unsigned)time(NULL)); seeded = true; }
        return moves.moves[rand() % moves.count];
    }

    // ── Move Ordering ─────────────────────────────────────────────────────
    void orderMoves(MoveList& moves, const Board& board, int depth) {
        // Simple bubble sort by score (small lists, acceptable)
        for (int i = 0; i < moves.count - 1; i++) {
            for (int j = 0; j < moves.count - i - 1; j++) {
                int scoreA = moveScore(moves.moves[j], board, depth);
                int scoreB = moveScore(moves.moves[j + 1], board, depth);
                if (scoreA < scoreB) {
                    Move tmp = moves.moves[j];
                    moves.moves[j] = moves.moves[j + 1];
                    moves.moves[j + 1] = tmp;
                }
            }
        }
    }

    int moveScore(const Move& m, const Board& board, int depth) const {
        int score = 0;
        if (m.isCapture) {
            const Cell& victim   = board.at(m.toRow, m.toCol);
            const Cell& attacker = board.at(m.fromRow, m.fromCol);
            score = 10000 + pieceValue(victim.type) - pieceValue(attacker.type) / 10;
        }
        if (!m.isCapture && isKiller(m, depth)) score = 5000;
        return score;
    }

    // ── Minimax with Alpha-Beta Pruning ───────────────────────────────────
    int minimax(Board& board, int depth, int alpha, int beta, bool maximizing, int ply, std::chrono::time_point<std::chrono::high_resolution_clock> startTime) {
        nodesEvaluated++;

        if ((nodesEvaluated & 2047) == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<double, std::milli>(now - startTime).count() > 500.0) {
                timeout = true;
                return 0;
            }
        }
        if (timeout) return 0;

        if (depth == 0) return board.evaluate();

        PieceColor turn = board.state.currentTurn;
        MoveList moves = board.getLegalMoves(turn);
        if (moves.empty()) {
            if (board.isInCheck(turn)) return maximizing ? -50000 + ply : 50000 - ply;
            return 0;  // stalemate
        }

        orderMoves(moves, board, ply);

        if (maximizing) {
            int maxEval = INT_MIN;
            for (int i = 0; i < moves.count; i++) {
                board.applyMove(moves.moves[i]);
                int eval = minimax(board, depth - 1, alpha, beta, false, ply + 1, startTime);
                board.undoMove();
                if (eval > maxEval) maxEval = eval;
                if (eval > alpha)   alpha = eval;
                if (beta <= alpha) {
                    storeKiller(moves.moves[i], ply);
                    break;
                }
            }
            return maxEval;
        } else {
            int minEval = INT_MAX;
            for (int i = 0; i < moves.count; i++) {
                board.applyMove(moves.moves[i]);
                int eval = minimax(board, depth - 1, alpha, beta, true, ply + 1, startTime);
                board.undoMove();
                if (eval < minEval) minEval = eval;
                if (eval < beta)    beta = eval;
                if (beta <= alpha) {
                    storeKiller(moves.moves[i], ply);
                    break;
                }
            }
            return minEval;
        }
    }
};
