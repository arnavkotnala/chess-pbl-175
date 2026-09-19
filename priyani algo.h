#pragma once

/*
 * ─── Priyani Algo — Medium Difficulty (Depth 3) ────────────────────────────
 * Implements Minimax with Alpha-Beta Pruning + Iterative Deepening +
 * Killer Move Heuristic, fixed at search depth 3.
 */

#include "board.h"
#include <climits>
#include <chrono>

static const int PRIYANI_MAX_SEARCH_DEPTH = 10;
static const int PRIYANI_DEPTH = 3;

class PriyaniAlgo {
public:
    int  nodesEvaluated;
    bool timeout;

    PriyaniAlgo() : nodesEvaluated(0), timeout(false) {
        clearKillers();
    }

    // Returns best move for the given PieceColor at depth 3
    Move getBestMove(Board& board, PieceColor PieceColor) {
        nodesEvaluated = 0;
        MoveList legal = board.getLegalMoves(PieceColor);
        if (legal.empty()) { return Move(); }

        int maxDepth = PRIYANI_DEPTH;
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

    // Point multiplier for scoring (Medium difficulty)
    float multiplier() const {
        return 2.0f;
    }

private:
    // ── Killer Move Table ─────────────────────────────────────────────────
    struct KillerEntry {
        int fromRow, fromCol, toRow, toCol;
        bool valid;
    };
    KillerEntry killers[2][PRIYANI_MAX_SEARCH_DEPTH];

    void clearKillers() {
        for (int i = 0; i < 2; i++)
            for (int d = 0; d < PRIYANI_MAX_SEARCH_DEPTH; d++)
                killers[i][d].valid = false;
    }

    bool isKiller(const Move& m, int depth) const {
        if (depth >= PRIYANI_MAX_SEARCH_DEPTH) return false;
        for (int i = 0; i < 2; i++) {
            const KillerEntry& k = killers[i][depth];
            if (k.valid && k.fromRow == m.fromRow && k.fromCol == m.fromCol &&
                k.toRow == m.toRow && k.toCol == m.toCol) return true;
        }
        return false;
    }

    void storeKiller(const Move& m, int depth) {
        if (depth >= PRIYANI_MAX_SEARCH_DEPTH || m.isCapture) return;
        killers[0][depth] = killers[1][depth];
        killers[1][depth] = {m.fromRow, m.fromCol, m.toRow, m.toCol, true};
    }

    // ── Move Ordering ─────────────────────────────────────────────────────
    void orderMoves(MoveList& moves, const Board& board, int depth) {
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
