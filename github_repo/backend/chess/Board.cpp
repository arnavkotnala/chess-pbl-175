#include "Board.h"
#include <iostream>
#include <sstream>
#include <algorithm>
using namespace std;

// ─── Piece-square tables for evaluation (White's perspective) ─────────────────
static const int pawnTable[8][8] = {
    { 0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    { 5,  5, 10, 25, 25, 10,  5,  5},
    { 0,  0,  0, 20, 20,  0,  0,  0},
    { 5, -5,-10,  0,  0,-10, -5,  5},
    { 5, 10, 10,-20,-20, 10, 10,  5},
    { 0,  0,  0,  0,  0,  0,  0,  0}
};
static const int knightTable[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};
static const int bishopTable[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};
static const int rookTable[8][8] = {
    { 0,  0,  0,  0,  0,  0,  0,  0},
    { 5, 10, 10, 10, 10, 10, 10,  5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    { 0,  0,  0,  5,  5,  0,  0,  0}
};
static const int queenTable[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    { -5,  0,  5,  5,  5,  5,  0, -5},
    {  0,  0,  5,  5,  5,  5,  0, -5},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};
static const int kingMiddleTable[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    { 20, 20,  0,  0,  0,  0, 20, 20},
    { 20, 30, 10,  0,  0, 10, 30, 20}
};

// ─── Board Constructor ────────────────────────────────────────────────────────
Board::Board() { init(); }

void Board::init() {
    // Clear
    for(int r=0;r<8;r++) for(int c=0;c<8;c++) state.grid[r][c] = Cell();

    // White pieces (row 0 = rank 1)
    auto place = [&](int r, int c, PieceType t, Color col) {
        state.grid[r][c] = Cell(t, col);
    };

    place(0,0,ROOK,WHITE);  place(0,7,ROOK,WHITE);
    place(0,1,KNIGHT,WHITE);place(0,6,KNIGHT,WHITE);
    place(0,2,BISHOP,WHITE);place(0,5,BISHOP,WHITE);
    place(0,3,QUEEN,WHITE); place(0,4,KING,WHITE);
    for(int c=0;c<8;c++) place(1,c,PAWN,WHITE);

    // Black pieces (row 7 = rank 8)
    place(7,0,ROOK,BLACK);  place(7,7,ROOK,BLACK);
    place(7,1,KNIGHT,BLACK);place(7,6,KNIGHT,BLACK);
    place(7,2,BISHOP,BLACK);place(7,5,BISHOP,BLACK);
    place(7,3,QUEEN,BLACK); place(7,4,KING,BLACK);
    for(int c=0;c<8;c++) place(6,c,PAWN,BLACK);

    state.currentTurn        = WHITE;
    state.enPassantCol       = -1;
    state.enPassantRow       = -1;
    state.whiteKingsideCastle  = true;
    state.whiteQueensideCastle = true;
    state.blackKingsideCastle  = true;
    state.blackQueensideCastle = true;
    state.halfMoveClock      = 0;
    state.fullMoveNumber     = 1;
}

// ─── Move Generation ──────────────────────────────────────────────────────────
vector<Move> Board::getPieceMoves(int r, int c, bool checkCastle) const {
    vector<Move> moves;
    const Cell& cell = state.grid[r][c];
    if(cell.isEmpty()) return moves;

    Color me   = cell.color;
    Color them = (me == WHITE) ? BLACK : WHITE;

    auto addMove = [&](int fr, int fc, int tr, int tc, bool cap=false) {
        Move m;
        m.fromRow=fr; m.fromCol=fc; m.toRow=tr; m.toCol=tc;
        m.isCapture=cap; m.isCastle=false; m.isEnPassant=false;
        m.isPromotion=false; m.promoteTo=QUEEN;
        moves.push_back(m);
    };

    auto slide = [&](int dr, int dc) {
        int nr=r+dr, nc=c+dc;
        while(inBounds(nr,nc)) {
            const Cell& t = state.grid[nr][nc];
            if(t.isEmpty())     { addMove(r,c,nr,nc); }
            else if(t.color==them) { addMove(r,c,nr,nc,true); break; }
            else break;
            nr+=dr; nc+=dc;
        }
    };

    switch(cell.type) {
    // ── PAWN ─────────────────────────────────────────────────────────────────
    case PAWN: {
        int dir = (me==WHITE) ? 1 : -1;
        int startRow = (me==WHITE) ? 1 : 6;
        int promRow  = (me==WHITE) ? 7 : 0;

        // Forward 1
        if(inBounds(r+dir,c) && state.grid[r+dir][c].isEmpty()) {
            if(r+dir==promRow) {
                for(PieceType pt : {QUEEN,ROOK,BISHOP,KNIGHT}) {
                    Move m; m.fromRow=r;m.fromCol=c;m.toRow=r+dir;m.toCol=c;
                    m.isCapture=false;m.isCastle=false;m.isEnPassant=false;
                    m.isPromotion=true;m.promoteTo=pt;
                    moves.push_back(m);
                }
            } else {
                addMove(r,c,r+dir,c);
                // Forward 2 from start
                if(r==startRow && state.grid[r+2*dir][c].isEmpty())
                    addMove(r,c,r+2*dir,c);
            }
        }
        // Captures
        for(int dc : {-1,1}) {
            int nc=c+dc, nr=r+dir;
            if(!inBounds(nr,nc)) continue;
            if(!state.grid[nr][nc].isEmpty() && state.grid[nr][nc].color==them) {
                if(nr==promRow) {
                    for(PieceType pt : {QUEEN,ROOK,BISHOP,KNIGHT}) {
                        Move m; m.fromRow=r;m.fromCol=c;m.toRow=nr;m.toCol=nc;
                        m.isCapture=true;m.isCastle=false;m.isEnPassant=false;
                        m.isPromotion=true;m.promoteTo=pt;
                        moves.push_back(m);
                    }
                } else { addMove(r,c,nr,nc,true); }
            }
            // En passant
            if(nr==state.enPassantRow && nc==state.enPassantCol) {
                Move m; m.fromRow=r;m.fromCol=c;m.toRow=nr;m.toCol=nc;
                m.isCapture=true;m.isCastle=false;m.isEnPassant=true;
                m.isPromotion=false;m.promoteTo=QUEEN;
                moves.push_back(m);
            }
        }
        break;
    }
    // ── KNIGHT ───────────────────────────────────────────────────────────────
    case KNIGHT: {
        int jumps[8][2]={{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
        for(auto& j:jumps) {
            int nr=r+j[0],nc=c+j[1];
            if(!inBounds(nr,nc)) continue;
            if(state.grid[nr][nc].isEmpty()) addMove(r,c,nr,nc);
            else if(state.grid[nr][nc].color==them) addMove(r,c,nr,nc,true);
        }
        break;
    }
    // ── BISHOP ───────────────────────────────────────────────────────────────
    case BISHOP:
        slide(1,1);slide(1,-1);slide(-1,1);slide(-1,-1);
        break;
    // ── ROOK ─────────────────────────────────────────────────────────────────
    case ROOK:
        slide(1,0);slide(-1,0);slide(0,1);slide(0,-1);
        break;
    // ── QUEEN ────────────────────────────────────────────────────────────────
    case QUEEN:
        slide(1,0);slide(-1,0);slide(0,1);slide(0,-1);
        slide(1,1);slide(1,-1);slide(-1,1);slide(-1,-1);
        break;
    // ── KING ─────────────────────────────────────────────────────────────────
    case KING: {
        int dirs[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
        for(auto& d:dirs) {
            int nr=r+d[0],nc=c+d[1];
            if(!inBounds(nr,nc)) continue;
            if(state.grid[nr][nc].isEmpty()) addMove(r,c,nr,nc);
            else if(state.grid[nr][nc].color==them) addMove(r,c,nr,nc,true);
        }
        // Castling
        bool kside = (me==WHITE) ? state.whiteKingsideCastle  : state.blackKingsideCastle;
        bool qside = (me==WHITE) ? state.whiteQueensideCastle : state.blackQueensideCastle;
        if(checkCastle && !cell.hasMoved && !isInCheck(me)) {
            if(kside && state.grid[r][5].isEmpty() && state.grid[r][6].isEmpty()
               && !isAttacked(r,5,them) && !isAttacked(r,6,them)) {
                Move m; m.fromRow=r;m.fromCol=4;m.toRow=r;m.toCol=6;
                m.isCapture=false;m.isCastle=true;m.isEnPassant=false;
                m.isPromotion=false;m.promoteTo=QUEEN;
                moves.push_back(m);
            }
            if(qside && state.grid[r][3].isEmpty() && state.grid[r][2].isEmpty()
               && state.grid[r][1].isEmpty()
               && !isAttacked(r,3,them) && !isAttacked(r,2,them)) {
                Move m; m.fromRow=r;m.fromCol=4;m.toRow=r;m.toCol=2;
                m.isCapture=false;m.isCastle=true;m.isEnPassant=false;
                m.isPromotion=false;m.promoteTo=QUEEN;
                moves.push_back(m);
            }
        }
        break;
    }
    default: break;
    }
    return moves;
}

vector<Move> Board::getPseudoMoves(Color c, bool checkCastle) const {
    vector<Move> all;
    all.reserve(50); // avg ~30 pseudo-legal moves per position
    for(int r=0;r<8;r++) for(int col=0;col<8;col++)
        if(state.grid[r][col].color==c) {
            auto mv = getPieceMoves(r,col,checkCastle);
            all.insert(all.end(),mv.begin(),mv.end());
        }
    return all;
}

// ─── isAttacked (Optimized: direct geometric checks, zero-copy) ──────────────
bool Board::isAttacked(int r, int c, Color byColor) const {
    // 1. Knight attacks
    static const int knightJumps[8][2]={{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
    for(auto& j : knightJumps) {
        int nr=r+j[0], nc=c+j[1];
        if(inBounds(nr,nc) && state.grid[nr][nc].type==KNIGHT && state.grid[nr][nc].color==byColor)
            return true;
    }

    // 2. Pawn attacks
    int pawnDir = (byColor==WHITE) ? -1 : 1; // direction FROM which a pawn would attack
    for(int dc : {-1, 1}) {
        int pr=r+pawnDir, pc=c+dc;
        if(inBounds(pr,pc) && state.grid[pr][pc].type==PAWN && state.grid[pr][pc].color==byColor)
            return true;
    }

    // 3. King attacks (adjacent squares)
    static const int kingDirs[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    for(auto& d : kingDirs) {
        int nr=r+d[0], nc=c+d[1];
        if(inBounds(nr,nc) && state.grid[nr][nc].type==KING && state.grid[nr][nc].color==byColor)
            return true;
    }

    // 4. Rook/Queen attacks (straight lines)
    static const int straightDirs[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
    for(auto& d : straightDirs) {
        int nr=r+d[0], nc=c+d[1];
        while(inBounds(nr,nc)) {
            const Cell& cell = state.grid[nr][nc];
            if(!cell.isEmpty()) {
                if(cell.color==byColor && (cell.type==ROOK || cell.type==QUEEN))
                    return true;
                break; // blocked by any piece
            }
            nr+=d[0]; nc+=d[1];
        }
    }

    // 5. Bishop/Queen attacks (diagonals)
    static const int diagDirs[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}};
    for(auto& d : diagDirs) {
        int nr=r+d[0], nc=c+d[1];
        while(inBounds(nr,nc)) {
            const Cell& cell = state.grid[nr][nc];
            if(!cell.isEmpty()) {
                if(cell.color==byColor && (cell.type==BISHOP || cell.type==QUEEN))
                    return true;
                break; // blocked by any piece
            }
            nr+=d[0]; nc+=d[1];
        }
    }

    return false;
}

// ─── King Position ────────────────────────────────────────────────────────────
pair<int,int> Board::kingPos(Color c) const {
    for(int r=0;r<8;r++) for(int col=0;col<8;col++)
        if(state.grid[r][col].type==KING && state.grid[r][col].color==c)
            return {r,col};
    return {-1,-1};
}

bool Board::isInCheck(Color c) const {
    auto [kr,kc] = kingPos(c);
    if(kr<0) return false;
    Color opp = (c==WHITE)?BLACK:WHITE;
    return isAttacked(kr,kc,opp);
}

// ─── Apply Move (internal raw) ────────────────────────────────────────────────
void Board::applyMoveRaw(GameState& s, const Move& m) const {
    Cell& from = s.grid[m.fromRow][m.fromCol];
    Cell& to   = s.grid[m.toRow][m.toCol];

    s.enPassantCol = -1;
    s.enPassantRow = -1;

    // En passant capture
    if(m.isEnPassant) {
        int dir = (from.color==WHITE)?-1:1;
        s.grid[m.toRow+dir][m.toCol] = Cell(); // remove captured pawn
    }

    // Update castle rights
    if(from.type==KING) {
        if(from.color==WHITE) { s.whiteKingsideCastle=false; s.whiteQueensideCastle=false; }
        else                  { s.blackKingsideCastle=false; s.blackQueensideCastle=false; }
    }
    if(from.type==ROOK) {
        if(m.fromCol==0) { if(from.color==WHITE) s.whiteQueensideCastle=false; else s.blackQueensideCastle=false; }
        if(m.fromCol==7) { if(from.color==WHITE) s.whiteKingsideCastle=false;  else s.blackKingsideCastle=false;  }
    }

    // Castling: move rook
    if(m.isCastle) {
        int row = m.fromRow;
        if(m.toCol==6) { // kingside
            s.grid[row][5] = s.grid[row][7];
            s.grid[row][7] = Cell();
            s.grid[row][5].hasMoved = true;
        } else { // queenside
            s.grid[row][3] = s.grid[row][0];
            s.grid[row][0] = Cell();
            s.grid[row][3].hasMoved = true;
        }
    }

    // En passant target square setup
    if(from.type==PAWN && abs(m.toRow-m.fromRow)==2) {
        s.enPassantRow = (m.fromRow+m.toRow)/2;
        s.enPassantCol = m.fromCol;
    }

    // Move piece
    to = from;
    to.hasMoved = true;
    from = Cell();

    // Promotion
    if(m.isPromotion) to.type = m.promoteTo;

    // Switch turn
    s.currentTurn = (s.currentTurn==WHITE)?BLACK:WHITE;
    if(s.currentTurn==WHITE) s.fullMoveNumber++;
}

bool Board::applyMove(const Move& m) {
    // Save state for undo
    GameState saved = state;

    applyMoveRaw(state, m);

    // Check: did moving player leave their own king in check?
    Color moved = (state.currentTurn==WHITE)?BLACK:WHITE; // turn switched already
    if(isInCheck(moved)) {
        state = saved; // illegal move, revert
        return false;
    }
    history.push({saved, m});
    return true;
}

void Board::undoMove() {
    if(history.empty()) return;
    state = history.top().first;
    history.pop();
}

// ─── Legal Moves ─────────────────────────────────────────────────────────────
vector<Move> Board::getLegalMoves(Color c) const {
    vector<Move> pseudo = getPseudoMoves(c);
    vector<Move> legal;
    legal.reserve(pseudo.size());
    for(auto& m : pseudo) {
        Board tmp = *this;
        tmp.applyMoveRaw(tmp.state, m);
        Color moved = (tmp.state.currentTurn==WHITE)?BLACK:WHITE;
        if(!tmp.isInCheck(moved)) legal.push_back(m);
    }
    return legal;
}

bool Board::isCheckmate(Color c) const {
    return isInCheck(c) && getLegalMoves(c).empty();
}

bool Board::isStalemate(Color c) const {
    return !isInCheck(c) && getLegalMoves(c).empty();
}

// ─── Board Evaluation ─────────────────────────────────────────────────────────
int Board::evaluate() const {
    int score = 0;
    for(int r=0;r<8;r++) for(int c=0;c<8;c++) {
        const Cell& cell = state.grid[r][c];
        if(cell.isEmpty()) continue;
        int val = pieceValue(cell.type);
        int pos = 0;
        int tr = (cell.color==WHITE) ? r : (7-r); // flip table for black
        switch(cell.type) {
            case PAWN:   pos=pawnTable[7-tr][c];   break;
            case KNIGHT: pos=knightTable[7-tr][c]; break;
            case BISHOP: pos=bishopTable[7-tr][c]; break;
            case ROOK:   pos=rookTable[7-tr][c];   break;
            case QUEEN:  pos=queenTable[7-tr][c];  break;
            case KING:   pos=kingMiddleTable[7-tr][c]; break;
            default: break;
        }
        if(cell.color==WHITE) score += val+pos;
        else                  score -= val+pos;
    }
    return score;
}

// ─── Console Print (debug) ────────────────────────────────────────────────────
void Board::print() const {
    static const char* symbols[2][7] = {
        {".","\u2659","\u2658","\u2657","\u2656","\u2655","\u2654"},  // WHITE
        {".","\u265f","\u265e","\u265d","\u265c","\u265b","\u265a"}   // BLACK
    };
    cout << "  a b c d e f g h\n";
    for(int r=7;r>=0;r--) {
        cout << (r+1) << " ";
        for(int c=0;c<8;c++) {
            const Cell& cell=state.grid[r][c];
            if(cell.isEmpty()) cout << ". ";
            else cout << symbols[cell.color-1][cell.type] << " ";
        }
        cout << (r+1) << "\n";
    }
    cout << "  a b c d e f g h\n";
}

// ─── Piece Code for JS ────────────────────────────────────────────────────────
string Board::pieceCode(int r, int c) const {
    const Cell& cell = state.grid[r][c];
    if(cell.isEmpty()) return "0";
    string col = (cell.color==WHITE)?"w":"b";
    string type;
    switch(cell.type) {
        case PAWN:   type="P"; break;
        case KNIGHT: type="N"; break;
        case BISHOP: type="B"; break;
        case ROOK:   type="R"; break;
        case QUEEN:  type="Q"; break;
        case KING:   type="K"; break;
        default:     type="?"; break;
    }
    return col+type;
}

// ─── JSON Serialization (Optimized: compute legal moves once) ─────────────────
string Board::toJSON() const {
    // Compute once — avoids 3 redundant calls to getLegalMoves/isInCheck
    bool inCheck = isInCheck(state.currentTurn);
    auto legalMoves = getLegalMoves(state.currentTurn);
    bool checkmate = inCheck && legalMoves.empty();
    bool stalemate = !inCheck && legalMoves.empty();

    ostringstream oss;
    oss << "{";
    oss << "\"turn\":\"" << (state.currentTurn==WHITE?"white":"black") << "\",";
    oss << "\"board\":[";
    for(int r=7;r>=0;r--) {
        oss << "[";
        for(int c=0;c<8;c++) {
            oss << "\"" << pieceCode(r,c) << "\"";
            if(c<7) oss << ",";
        }
        oss << "]";
        if(r>0) oss << ",";
    }
    oss << "],";
    oss << "\"inCheck\":" << (inCheck?"true":"false") << ",";
    oss << "\"checkmate\":" << (checkmate?"true":"false") << ",";
    oss << "\"stalemate\":" << (stalemate?"true":"false");
    oss << "}";
    return oss.str();
}
