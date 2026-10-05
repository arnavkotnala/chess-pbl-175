#include "board.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

// ─── Piece-square tables for evaluation (P_WHITE's perspective) ─────────────────
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

// ─── Board Constructor / Destructor ───────────────────────────────────────────
Board::Board() {
    history = stack_create(sizeof(GameState), 64);
    init();
}

Board::~Board() {
    if (history) stack_destroy(history);
}

Board::Board(const Board& other) {
    state = other.state;
    history = stack_create(sizeof(GameState), 64);
    // Note: we don't copy history for AI copies (not needed)
}

Board& Board::operator=(const Board& other) {
    if (this != &other) {
        state = other.state;
        // Don't copy history for assignment (used in AI copies)
    }
    return *this;
}

void Board::init() {
    moveHistoryCount = 0;
    snapshotCount = 0;
    // Clear
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            state.grid[r][c] = Cell();

    // Place pieces
    auto place = [&](int r, int c, PieceType t, PieceColor col) {
        state.grid[r][c] = Cell(t, col);
    };

    place(0, 0, ROOK, P_WHITE);   place(0, 7, ROOK, P_WHITE);
    place(0, 1, KNIGHT, P_WHITE); place(0, 6, KNIGHT, P_WHITE);
    place(0, 2, BISHOP, P_WHITE); place(0, 5, BISHOP, P_WHITE);
    place(0, 3, QUEEN, P_WHITE);  place(0, 4, KING, P_WHITE);
    for (int c = 0; c < 8; c++) place(1, c, PAWN, P_WHITE);

    place(7, 0, ROOK, P_BLACK);   place(7, 7, ROOK, P_BLACK);
    place(7, 1, KNIGHT, P_BLACK); place(7, 6, KNIGHT, P_BLACK);
    place(7, 2, BISHOP, P_BLACK); place(7, 5, BISHOP, P_BLACK);
    place(7, 3, QUEEN, P_BLACK);  place(7, 4, KING, P_BLACK);
    for (int c = 0; c < 8; c++) place(6, c, PAWN, P_BLACK);

    state.currentTurn           = P_WHITE;
    state.enPassantCol          = -1;
    state.enPassantRow          = -1;
    state.whiteKingsideCastle   = true;
    state.whiteQueensideCastle  = true;
    state.blackKingsideCastle   = true;
    state.blackQueensideCastle  = true;
    state.halfMoveClock         = 0;
    state.fullMoveNumber        = 1;

    if (history) stack_clear(history);

    // Save initial position as snapshot[0]
    stateSnapshots[0] = state;
    snapshotCount = 1;
}

// ─── Move Generation ──────────────────────────────────────────────────────────
void Board::getPieceMoves(int r, int c, MoveList& moves, bool checkCastle) const {
    const Cell& cell = state.grid[r][c];
    if (cell.isEmpty()) return;

    PieceColor me   = cell.color;
    PieceColor them = (me == P_WHITE) ? P_BLACK : P_WHITE;

    auto addMove = [&](int fr, int fc, int tr, int tc, bool cap = false) {
        Move m;
        m.fromRow = fr; m.fromCol = fc; m.toRow = tr; m.toCol = tc;
        m.isCapture = cap; m.isCastle = false; m.isEnPassant = false;
        m.isPromotion = false; m.promoteTo = QUEEN;
        moves.add(m);
    };

    auto slide = [&](int dr, int dc) {
        int nr = r + dr, nc = c + dc;
        while (inBounds(nr, nc)) {
            const Cell& t = state.grid[nr][nc];
            if (t.isEmpty())          { addMove(r, c, nr, nc); }
            else if (t.color == them) { addMove(r, c, nr, nc, true); break; }
            else break;
            nr += dr; nc += dc;
        }
    };

    switch (cell.type) {
    case PAWN: {
        int dir      = (me == P_WHITE) ? 1 : -1;
        int startRow = (me == P_WHITE) ? 1 : 6;
        int promRow  = (me == P_WHITE) ? 7 : 0;

        // Forward 1
        if (inBounds(r + dir, c) && state.grid[r + dir][c].isEmpty()) {
            if (r + dir == promRow) {
                PieceType promos[] = {QUEEN, ROOK, BISHOP, KNIGHT};
                for (int i = 0; i < 4; i++) {
                    Move m;
                    m.fromRow = r; m.fromCol = c; m.toRow = r + dir; m.toCol = c;
                    m.isCapture = false; m.isCastle = false; m.isEnPassant = false;
                    m.isPromotion = true; m.promoteTo = promos[i];
                    moves.add(m);
                }
            } else {
                addMove(r, c, r + dir, c);
                if (r == startRow && state.grid[r + 2 * dir][c].isEmpty())
                    addMove(r, c, r + 2 * dir, c);
            }
        }
        // Captures
        int capDirs[] = {-1, 1};
        for (int d = 0; d < 2; d++) {
            int nc2 = c + capDirs[d], nr2 = r + dir;
            if (!inBounds(nr2, nc2)) continue;
            if (!state.grid[nr2][nc2].isEmpty() && state.grid[nr2][nc2].color == them) {
                if (nr2 == promRow) {
                    PieceType promos[] = {QUEEN, ROOK, BISHOP, KNIGHT};
                    for (int i = 0; i < 4; i++) {
                        Move m;
                        m.fromRow = r; m.fromCol = c; m.toRow = nr2; m.toCol = nc2;
                        m.isCapture = true; m.isCastle = false; m.isEnPassant = false;
                        m.isPromotion = true; m.promoteTo = promos[i];
                        moves.add(m);
                    }
                } else {
                    addMove(r, c, nr2, nc2, true);
                }
            }
            // En passant
            if (nr2 == state.enPassantRow && nc2 == state.enPassantCol) {
                Move m;
                m.fromRow = r; m.fromCol = c; m.toRow = nr2; m.toCol = nc2;
                m.isCapture = true; m.isCastle = false; m.isEnPassant = true;
                m.isPromotion = false; m.promoteTo = QUEEN;
                moves.add(m);
            }
        }
        break;
    }
    case KNIGHT: {
        int jumps[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
        for (int j = 0; j < 8; j++) {
            int nr = r + jumps[j][0], nc = c + jumps[j][1];
            if (!inBounds(nr, nc)) continue;
            if (state.grid[nr][nc].isEmpty()) addMove(r, c, nr, nc);
            else if (state.grid[nr][nc].color == them) addMove(r, c, nr, nc, true);
        }
        break;
    }
    case BISHOP:
        slide(1, 1); slide(1, -1); slide(-1, 1); slide(-1, -1);
        break;
    case ROOK:
        slide(1, 0); slide(-1, 0); slide(0, 1); slide(0, -1);
        break;
    case QUEEN:
        slide(1, 0); slide(-1, 0); slide(0, 1); slide(0, -1);
        slide(1, 1); slide(1, -1); slide(-1, 1); slide(-1, -1);
        break;
    case KING: {
        int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
        for (int d = 0; d < 8; d++) {
            int nr = r + dirs[d][0], nc = c + dirs[d][1];
            if (!inBounds(nr, nc)) continue;
            if (state.grid[nr][nc].isEmpty()) addMove(r, c, nr, nc);
            else if (state.grid[nr][nc].color == them) addMove(r, c, nr, nc, true);
        }
        // Castling
        bool kside = (me == P_WHITE) ? state.whiteKingsideCastle  : state.blackKingsideCastle;
        bool qside = (me == P_WHITE) ? state.whiteQueensideCastle : state.blackQueensideCastle;
        if (checkCastle && !cell.hasMoved && !isInCheck(me)) {
            if (kside && state.grid[r][5].isEmpty() && state.grid[r][6].isEmpty()
                && !isAttacked(r, 5, them) && !isAttacked(r, 6, them)) {
                Move m;
                m.fromRow = r; m.fromCol = 4; m.toRow = r; m.toCol = 6;
                m.isCapture = false; m.isCastle = true; m.isEnPassant = false;
                m.isPromotion = false; m.promoteTo = QUEEN;
                moves.add(m);
            }
            if (qside && state.grid[r][3].isEmpty() && state.grid[r][2].isEmpty()
                && state.grid[r][1].isEmpty()
                && !isAttacked(r, 3, them) && !isAttacked(r, 2, them)) {
                Move m;
                m.fromRow = r; m.fromCol = 4; m.toRow = r; m.toCol = 2;
                m.isCapture = false; m.isCastle = true; m.isEnPassant = false;
                m.isPromotion = false; m.promoteTo = QUEEN;
                moves.add(m);
            }
        }
        break;
    }
    default: break;
    }
}

MoveList Board::getPseudoMoves(PieceColor c, bool checkCastle) const {
    MoveList all;
    for (int r = 0; r < 8; r++)
        for (int col = 0; col < 8; col++)
            if (state.grid[r][col].color == c)
                getPieceMoves(r, col, all, checkCastle);
    return all;
}

// ─── isAttacked ───────────────────────────────────────────────────────────────
bool Board::isAttacked(int r, int c, PieceColor byColor) const {
    // Knight attacks
    static const int knightJumps[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
    for (int j = 0; j < 8; j++) {
        int nr = r + knightJumps[j][0], nc = c + knightJumps[j][1];
        if (inBounds(nr, nc) && state.grid[nr][nc].type == KNIGHT && state.grid[nr][nc].color == byColor)
            return true;
    }
    // Pawn attacks
    int pawnDir = (byColor == P_WHITE) ? -1 : 1;
    int pawnCapDirs[] = {-1, 1};
    for (int d = 0; d < 2; d++) {
        int pr = r + pawnDir, pc = c + pawnCapDirs[d];
        if (inBounds(pr, pc) && state.grid[pr][pc].type == PAWN && state.grid[pr][pc].color == byColor)
            return true;
    }
    // King attacks
    static const int kingDirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int d = 0; d < 8; d++) {
        int nr = r + kingDirs[d][0], nc = c + kingDirs[d][1];
        if (inBounds(nr, nc) && state.grid[nr][nc].type == KING && state.grid[nr][nc].color == byColor)
            return true;
    }
    // Rook/Queen attacks (straight lines)
    static const int straightDirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (int d = 0; d < 4; d++) {
        int nr = r + straightDirs[d][0], nc = c + straightDirs[d][1];
        while (inBounds(nr, nc)) {
            const Cell& cell = state.grid[nr][nc];
            if (!cell.isEmpty()) {
                if (cell.color == byColor && (cell.type == ROOK || cell.type == QUEEN))
                    return true;
                break;
            }
            nr += straightDirs[d][0]; nc += straightDirs[d][1];
        }
    }
    // Bishop/Queen attacks (diagonals)
    static const int diagDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int d = 0; d < 4; d++) {
        int nr = r + diagDirs[d][0], nc = c + diagDirs[d][1];
        while (inBounds(nr, nc)) {
            const Cell& cell = state.grid[nr][nc];
            if (!cell.isEmpty()) {
                if (cell.color == byColor && (cell.type == BISHOP || cell.type == QUEEN))
                    return true;
                break;
            }
            nr += diagDirs[d][0]; nc += diagDirs[d][1];
        }
    }
    return false;
}

// ─── King Position ────────────────────────────────────────────────────────────
void Board::kingPos(PieceColor c, int& outRow, int& outCol) const {
    for (int r = 0; r < 8; r++)
        for (int col = 0; col < 8; col++)
            if (state.grid[r][col].type == KING && state.grid[r][col].color == c) {
                outRow = r; outCol = col; return;
            }
    outRow = -1; outCol = -1;
}

bool Board::isInCheck(PieceColor c) const {
    int kr, kc;
    kingPos(c, kr, kc);
    if (kr < 0) return false;
    PieceColor opp = (c == P_WHITE) ? P_BLACK : P_WHITE;
    return isAttacked(kr, kc, opp);
}

// ─── Apply Move (internal raw) ────────────────────────────────────────────────
void Board::applyMoveRaw(GameState& s, const Move& m) const {
    Cell& from = s.grid[m.fromRow][m.fromCol];
    Cell& to   = s.grid[m.toRow][m.toCol];

    s.enPassantCol = -1;
    s.enPassantRow = -1;

    // En passant capture
    if (m.isEnPassant) {
        int dir = (from.color == P_WHITE) ? -1 : 1;
        s.grid[m.toRow + dir][m.toCol] = Cell();
    }

    // Update castle rights
    if (from.type == KING) {
        if (from.color == P_WHITE) { s.whiteKingsideCastle = false; s.whiteQueensideCastle = false; }
        else                     { s.blackKingsideCastle = false; s.blackQueensideCastle = false; }
    }
    if (from.type == ROOK) {
        if (m.fromCol == 0) {
            if (from.color == P_WHITE) s.whiteQueensideCastle = false;
            else s.blackQueensideCastle = false;
        }
        if (m.fromCol == 7) {
            if (from.color == P_WHITE) s.whiteKingsideCastle = false;
            else s.blackKingsideCastle = false;
        }
    }

    // Castling: move rook
    if (m.isCastle) {
        int row = m.fromRow;
        if (m.toCol == 6) {  // kingside
            s.grid[row][5] = s.grid[row][7];
            s.grid[row][7] = Cell();
            s.grid[row][5].hasMoved = true;
        } else {  // queenside
            s.grid[row][3] = s.grid[row][0];
            s.grid[row][0] = Cell();
            s.grid[row][3].hasMoved = true;
        }
    }

    // En passant target square setup
    if (from.type == PAWN && abs(m.toRow - m.fromRow) == 2) {
        s.enPassantRow = (m.fromRow + m.toRow) / 2;
        s.enPassantCol = m.fromCol;
    }

    // Move piece
    to = from;
    to.hasMoved = true;
    from = Cell();

    // Promotion
    if (m.isPromotion) to.type = m.promoteTo;

    // Switch turn
    s.currentTurn = (s.currentTurn == P_WHITE) ? P_BLACK : P_WHITE;
    if (s.currentTurn == P_WHITE) s.fullMoveNumber++;
}

bool Board::applyMove(const Move& m) {
    GameState saved = state;
    applyMoveRaw(state, m);

    // Check: did moving player leave their own king in check?
    PieceColor moved = (state.currentTurn == P_WHITE) ? P_BLACK : P_WHITE;
    if (isInCheck(moved)) {
        state = saved;
        return false;
    }
    // Push saved state to C Stack (DSA)
    stack_push(history, &saved);
    if (moveHistoryCount < 500) moveHistory[moveHistoryCount++] = m;

    // Save snapshot of the new state for Prev/Next replay
    if (snapshotCount < 501) stateSnapshots[snapshotCount++] = state;

    return true;
}

void Board::undoMove() {
    GameState saved;
    if (stack_pop(history, &saved)) {
        state = saved;
        if (moveHistoryCount > 0) moveHistoryCount--;
        if (snapshotCount > 1) snapshotCount--;
    }
}

// ─── Legal Moves ──────────────────────────────────────────────────────────────
MoveList Board::getLegalMoves(PieceColor c) const {
    MoveList pseudo = getPseudoMoves(c);
    MoveList legal;
    for (int i = 0; i < pseudo.count; i++) {
        Board tmp = *this;
        tmp.applyMoveRaw(tmp.state, pseudo.moves[i]);
        PieceColor moved = (tmp.state.currentTurn == P_WHITE) ? P_BLACK : P_WHITE;
        if (!tmp.isInCheck(moved)) legal.add(pseudo.moves[i]);
    }
    return legal;
}

bool Board::isCheckmate(PieceColor c) const {
    return isInCheck(c) && getLegalMoves(c).empty();
}

bool Board::isStalemate(PieceColor c) const {
    return !isInCheck(c) && getLegalMoves(c).empty();
}

// ─── Board Evaluation ─────────────────────────────────────────────────────────
int Board::evaluate() const {
    int score = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            const Cell& cell = state.grid[r][c];
            if (cell.isEmpty()) continue;
            int val = pieceValue(cell.type);
            int pos = 0;
            int tr = (cell.color == P_WHITE) ? r : (7 - r);
            switch (cell.type) {
                case PAWN:   pos = pawnTable[7 - tr][c];   break;
                case KNIGHT: pos = knightTable[7 - tr][c]; break;
                case BISHOP: pos = bishopTable[7 - tr][c]; break;
                case ROOK:   pos = rookTable[7 - tr][c];   break;
                case QUEEN:  pos = queenTable[7 - tr][c];  break;
                case KING:   pos = kingMiddleTable[7 - tr][c]; break;
                default: break;
            }
            if (cell.color == P_WHITE) score += val + pos;
            else                     score -= val + pos;
        }
    return score;
}

// ─── Console Print ────────────────────────────────────────────────────────────
void Board::print() const {
    static const char* letterSymbols[] = {".", "P", "N", "B", "R", "Q", "K"};

    printf("\n     a   b   c   d   e   f   g   h\n");
    printf("   +---+---+---+---+---+---+---+---+\n");
    for (int r = 7; r >= 0; r--) {
        printf(" %d |", r + 1);
        for (int c = 0; c < 8; c++) {
            const Cell& cell = state.grid[r][c];
            if (cell.isEmpty()) {
                printf(" . |");
            } else {
                char prefix = (cell.color == P_WHITE) ? 'w' : 'b';
                printf("%c%s |", prefix, letterSymbols[cell.type]);
            }
        }
        printf(" %d\n", r + 1);
        printf("   +---+---+---+---+---+---+---+---+\n");
    }
    printf("     a   b   c   d   e   f   g   h\n\n");
}

// ─── Piece Code ───────────────────────────────────────────────────────────────
void Board::pieceCode(int r, int c, char* out) const {
    const Cell& cell = state.grid[r][c];
    if (cell.isEmpty()) { out[0] = '0'; out[1] = '\0'; return; }
    out[0] = (cell.color == P_WHITE) ? 'w' : 'b';
    switch (cell.type) {
        case PAWN:   out[1] = 'P'; break;
        case KNIGHT: out[1] = 'N'; break;
        case BISHOP: out[1] = 'B'; break;
        case ROOK:   out[1] = 'R'; break;
        case QUEEN:  out[1] = 'Q'; break;
        case KING:   out[1] = 'K'; break;
        default:     out[1] = '?'; break;
    }
    out[2] = '\0';
}
