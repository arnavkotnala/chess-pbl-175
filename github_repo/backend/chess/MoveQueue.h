#pragma once
#include <queue>
#include <string>
#include <sstream>
#include <vector>
using namespace std;

// ─── Move Notation (DSA: Queue) ───────────────────────────────────────────────
// Records every move as algebraic notation in a FIFO queue.
// Supports:
//   - push(move)           → enqueue a move notation
//   - pop()                → dequeue oldest move
//   - serialize()          → export full move sequence as string
//   - loadSequence(str)    → import opponent's move sequence
//   - toVector()           → copy queue to vector for iteration

class MoveQueue {
private:
    queue<string> moveLog;     // Primary DSA: Queue
    int           moveNumber;

public:
    MoveQueue() : moveNumber(1) {}

    // Push a move notation onto the queue
    // moveStr can be algebraic: "e2e4", "Nf3", "O-O"
    void push(const string& moveStr, bool isWhite) {
        string entry;
        if(isWhite) {
            entry = to_string(moveNumber) + ". " + moveStr;
        } else {
            entry = moveStr;
            moveNumber++;
        }
        moveLog.push(entry);
    }

    // Pop the front move (oldest)
    string pop() {
        if(moveLog.empty()) return "";
        string m = moveLog.front();
        moveLog.pop();
        return m;
    }

    bool empty() const { return moveLog.empty(); }
    int  size()  const { return (int)moveLog.size(); }

    // Export all moves as a single space-separated string
    // e.g. "1. e2e4 e7e5 2. g1f3 b8c6"
    string serialize() const {
        queue<string> tmp = moveLog;
        string result;
        while(!tmp.empty()) {
            result += tmp.front();
            tmp.pop();
            if(!tmp.empty()) result += " ";
        }
        return result;
    }

    // Reset queue
    void clear() {
        while(!moveLog.empty()) moveLog.pop();
        moveNumber = 1;
    }

    // Remove the most recent move (for Undo)
    void popLast() {
        vector<string> v = toVector();
        if(v.empty()) return;
        v.pop_back();
        clear();
        // Restore items with original formatting
        for(auto& s : v) moveLog.push(s);
        // moveNumber logic: each white move adds "1. move", black just adds "move"
        // v.size() is the total number of moves. 
        // 0 moves -> moveNumber 1
        // 1 move -> moveNumber 1 (black's turn)
        // 2 moves -> moveNumber 2 (white's turn)
        moveNumber = 1 + v.size() / 2;
    }

    // Load a sequence string back into queue (e.g. pasted from opponent)
    void loadSequence(const string& seq) {
        clear();
        istringstream iss(seq);
        string token;
        while(iss >> token) {
            // Skip move numbers like "1.", "2."
            if(token.back()=='.') continue;
            moveLog.push(token);
        }
    }

    // Convert queue to vector for display (non-destructive)
    vector<string> toVector() const {
        vector<string> v;
        queue<string> tmp = moveLog;
        while(!tmp.empty()) { v.push_back(tmp.front()); tmp.pop(); }
        return v;
    }
};

// ─── Move Analyzer ────────────────────────────────────────────────────────────
// Takes a loaded sequence and suggests counter-moves or winning lines.
// Used with the "paste opponent moves → find best response" feature.

#include "Board.h"
#include "AI.h"

class MoveAnalyzer {
public:
    // Replay a move sequence string on a fresh board, return the board state
    // Returns false if any move in the sequence is illegal
    static bool replaySequence(Board& board, const string& sequence) {
        board.init();
        istringstream iss(sequence);
        string token;
        bool whiteTurn = true;
        while(iss >> token) {
            if(token.back()=='.') continue;  // skip "1.", "2." etc.
            if(token.size() < 4) continue;

            // Parse algebraic "e2e4" format
            int fc = token[0]-'a', fr = token[1]-'1';
            int tc = token[2]-'a', tr = token[3]-'1';

            // Find matching legal move
            Color c = whiteTurn ? WHITE : BLACK;
            auto legal = board.getLegalMoves(c);
            bool found = false;
            for(auto& m : legal) {
                if(m.fromRow==fr && m.fromCol==fc && m.toRow==tr && m.toCol==tc) {
                    board.applyMove(m);
                    found = true;
                    break;
                }
            }
            if(!found) return false;
            whiteTurn = !whiteTurn;
        }
        return true;
    }

    // After replaying opponent's sequence, get best counter move
    static string getBestCounter(const string& sequence, Color respondingColor, Difficulty diff) {
        Board board;
        if(!replaySequence(board, sequence)) return "Invalid sequence";

        AI ai(diff);
        Move best = ai.getBestMove(board, respondingColor);
        if(best.fromRow < 0) return "No moves available";
        return best.toAlgebraic();
    }
};
