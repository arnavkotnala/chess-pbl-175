#pragma once

/*
 * ─── AI Engine ────────────────────────────────────────────────────────────────
 * Implements Minimax with Alpha-Beta Pruning + Iterative Deepening +
 * Killer Move Heuristic for Easy/Medium/Hard/Expert difficulty.
 *
 * Optimizations applied:
 *   1. Iterative Deepening: search depth 1→N for better move ordering
 *   2. Killer Moves: store refutation moves per depth for better pruning
 *   3. Move Ordering: captures first (MVV-LVA style) + killer moves
 *
 * Expert is adaptive: after every search it compares its evaluation with the
 * one from its previous turn. If the player's reply made the AI's position
 * worse, the player is playing well, so Expert searches one ply deeper (up to
 * 7). If the player gave material away, Expert eases off by one ply (down to 4).
 */

#include "board.h"
#include <climits>
#include <cstdlib>
#include <ctime>
#include <chrono>

enum Difficulty { EASY, MEDIUM, HARD, EXPERT };

static const int MAX_SEARCH_DEPTH = 10;

// Expert adaptive search settings
static const int EXPERT_MIN_DEPTH   = 4;
static const int EXPERT_MAX_DEPTH   = 7;
static const int EXPERT_START_DEPTH = 5;
static const int PLAYER_GAIN_MARGIN = 60;    // player gained >= 0.6 pawn  -> search deeper
static const int PLAYER_LOSS_MARGIN = 150;   // player lost   >= 1.5 pawns -> search shallower
static const int ADAPT_SCORE_CLAMP  = 2000;  // keeps checkmate scores from swamping the comparison

class AI {
public:
    Difficulty difficulty;
    int        nodesEvaluated;
    bool       timeout;
    int        expertDepth;    // Expert's current target depth (4-7)
    int        depthReached;   // deepest fully completed search depth of the last move

    explicit AI(Difficulty d = EASY) : difficulty(d), nodesEvaluated(0), timeout(false),
                                       expertDepth(EXPERT_START_DEPTH), depthReached(0),
                                       timeLimitMs(500.0), lastScore(0), hasLastScore(false) {
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
        if (difficulty == EXPERT) maxDepth = expertDepth;
        timeLimitMs = searchTimeLimit();
        bool maximizing = (PieceColor == P_WHITE);
        clearKillers();

        auto startTime = std::chrono::high_resolution_clock::now();
        Move bestMove = legal.moves[0];
        int  bestMoveScore = 0;
        depthReached = 0;
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
            bestMoveScore = bestScore;
            depthReached = depth;

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

        // Expert adapts its depth for the next move (score from the AI's point of view)
        if (difficulty == EXPERT && depthReached > 0) {
            adaptToPlayer(maximizing ? bestMoveScore : -bestMoveScore);
        }
        return bestMove;
    }

    // Point multiplier for scoring
    float multiplier() const {
        switch (difficulty) {
            case EASY:   return 1.5f;
            case MEDIUM: return 2.0f;
            case HARD:   return 3.0f;
            case EXPERT: return 4.0f;
        }
        return 1.0f;
    }

private:
    double timeLimitMs;   // search time budget for the current move
    int    lastScore;     // Expert's evaluation on its previous turn (AI's point of view)
    bool   hasLastScore;

    // ── Search time budget ────────────────────────────────────────────────
    double searchTimeLimit() const {
        if (difficulty != EXPERT) return 500.0;
#ifdef __EMSCRIPTEN__
        return 1500.0;   // browser runs the search on the main thread, so keep the freeze short
#else
        return 1000.0 * (expertDepth - 3);   // depth 4 -> 1 s ... depth 7 -> 4 s
#endif
    }

    // ── Expert: adapt search depth to the player's strength ──────────────
    void adaptToPlayer(int score) {
        if (score >  ADAPT_SCORE_CLAMP) score =  ADAPT_SCORE_CLAMP;
        if (score < -ADAPT_SCORE_CLAMP) score = -ADAPT_SCORE_CLAMP;

        if (hasLastScore) {
            int change = score - lastScore;
            if (change <= -PLAYER_GAIN_MARGIN && expertDepth < EXPERT_MAX_DEPTH) expertDepth++;
            else if (change >= PLAYER_LOSS_MARGIN && expertDepth > EXPERT_MIN_DEPTH) expertDepth--;
        }
        lastScore = score;
        hasLastScore = true;
    }

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
            if (std::chrono::duration<double, std::milli>(now - startTime).count() > timeLimitMs) {
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
