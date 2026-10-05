#include <iostream>
#include <cassert>
#include "chess/board.h"
#include "chess/ai.h"

using namespace std;

Move createMove(int fr, int fc, int tr, int tc) {
    Move m;
    m.fromRow = fr; m.fromCol = fc;
    m.toRow = tr; m.toCol = tc;
    return m;
}

void test_initial_state() {
    Board b;
    b.init();
    assert(b.state.currentTurn == P_WHITE);
    assert(b.at(0, 0).type == ROOK && b.at(0, 0).color == P_WHITE);
    assert(b.at(7, 4).type == KING && b.at(7, 4).color == P_BLACK);
    
    MoveList whiteMoves = b.getLegalMoves(P_WHITE);
    assert(whiteMoves.count == 20); // 16 pawn moves + 4 knight moves
    cout << "[PASS] Initial Board State & Legal Move Count" << endl;
}

void test_fools_mate() {
    Board b;
    b.init();
    // 1. f3
    assert(b.applyMove(createMove(1, 5, 2, 5)));
    // 1... e5
    assert(b.applyMove(createMove(6, 4, 4, 4)));
    // 2. g4
    assert(b.applyMove(createMove(1, 6, 3, 6)));
    // 2... Qh4#
    assert(b.applyMove(createMove(7, 3, 3, 7)));
    
    assert(b.isCheckmate(P_WHITE) == true);
    assert(b.isStalemate(P_WHITE) == false);
    cout << "[PASS] Fool's Mate (Checkmate Validation)" << endl;
}

void test_undo() {
    Board b;
    b.init();
    b.applyMove(createMove(1, 4, 3, 4)); // e4
    assert(b.at(3, 4).type == PAWN);
    assert(b.state.currentTurn == P_BLACK);
    
    b.undoMove();
    assert(b.at(1, 4).type == PAWN);
    assert(b.at(3, 4).isEmpty());
    assert(b.state.currentTurn == P_WHITE);
    cout << "[PASS] C-Stack History & Undo Mechanics" << endl;
}

void test_stalemate() {
    Board b;
    // Clear board
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            b.state.grid[r][c] = Cell();
            
    b.state.whiteKingsideCastle = false;
    b.state.whiteQueensideCastle = false;
    b.state.blackKingsideCastle = false;
    b.state.blackQueensideCastle = false;
            
    b.state.currentTurn = P_BLACK;
    
    // White King at c6 (5, 2), White Queen at b6 (5, 1)
    b.state.grid[5][2] = Cell(KING, P_WHITE);
    b.state.grid[5][1] = Cell(QUEEN, P_WHITE);
    
    // Black King at a8 (7, 0)
    b.state.grid[7][0] = Cell(KING, P_BLACK);
    
    MoveList mvs = b.getLegalMoves(P_BLACK);
    for (int i = 0; i < mvs.count; i++) {
        char alg[5];
        mvs.moves[i].toAlgebraic(alg);
        cout << "UNEXPECTED MOVE: " << alg << endl;
    }
    assert(b.isInCheck(P_BLACK) == false);
    assert(mvs.count == 0);
    assert(b.isStalemate(P_BLACK) == true);
    assert(b.isCheckmate(P_BLACK) == false);
    cout << "[PASS] Exact Stalemate Edge-Case Detection" << endl;
}

void test_en_passant() {
    Board b;
    b.init();
    b.applyMove(createMove(1, 4, 3, 4)); // e4
    b.applyMove(createMove(6, 0, 5, 0)); // a6
    b.applyMove(createMove(3, 4, 4, 4)); // e5
    b.applyMove(createMove(6, 3, 4, 3)); // d5 (Double jump)
    
    // e5 should be able to capture d5 en passant to d6
    MoveList moves = b.getLegalMoves(P_WHITE);
    bool foundEp = false;
    for(int i = 0; i < moves.count; i++) {
        if (moves.moves[i].isEnPassant && moves.moves[i].toRow == 5 && moves.moves[i].toCol == 3) {
            foundEp = true;
            break;
        }
    }
    assert(foundEp == true);
    cout << "[PASS] En Passant Target Generation" << endl;
}

int main() {
    cout << "--- CHESS ENGINE CORE QA ---" << endl;
    test_initial_state();
    test_fools_mate();
    test_undo();
    test_stalemate();
    test_en_passant();
    cout << "--- ALL 5 CRITICAL TEST SUITES PASSED ---" << endl;
    return 0;
}
