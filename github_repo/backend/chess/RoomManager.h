#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <ctime>
#include <chrono>
#include <cstdint>
#include "Board.h"
#include "MoveQueue.h"

using namespace std;

// ─── Online Room Struct ───────────────────────────────────────────────────────
struct GameRoom {
    string roomId;
    string whitePlayer;
    string blackPlayer;
    Board  board;
    MoveQueue moveQueue;
    string status;       // "waiting", "active", "finished"
    string winner;       // "white", "black", "draw", ""
    string winReason;    // "checkmate", "stalemate", "resignation", ""
    int whiteScore;
    int blackScore;
    int lastFromRow;
    int lastFromCol;
    int lastToRow;
    int lastToCol;
    int moveCount;
    int64_t lastActivityTime; // Milliseconds timestamp

    GameRoom()
        : roomId(""), whitePlayer(""), blackPlayer(""),
          status("waiting"), winner(""), winReason(""),
          whiteScore(0), blackScore(0),
          lastFromRow(-1), lastFromCol(-1), lastToRow(-1), lastToCol(-1),
          moveCount(0), lastActivityTime(0) {
        board.init();
    }
};

// ─── RoomManager Class (Pure C++ / STL / Mutex Safe) ──────────────────────────
class RoomManager {
private:
    map<string, GameRoom> m_rooms;
    mutable mutex m_mutex;

    // Helper: current timestamp in milliseconds
    int64_t nowMs() const {
        return chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    // Helper: generate 6-character room code (e.g. "K9X2P8")
    string generateCode() {
        const char charset[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // Removed ambiguous chars (0/O, 1/I)
        const size_t max_index = sizeof(charset) - 2;
        random_device rd;
        mt19937 gen(rd() ^ (unsigned int)time(nullptr));
        uniform_int_distribution<> dis(0, (int)max_index);

        string code;
        do {
            code = "";
            for (int i = 0; i < 6; ++i) {
                code += charset[dis(gen)];
            }
        } while (m_rooms.find(code) != m_rooms.end()); // Ensure uniqueness

        return code;
    }

public:
    RoomManager() {}

    // Create a new match room
    string createRoom(const string& hostUsername, const string& preferredColor = "white") {
        lock_guard<mutex> lock(m_mutex);
        string code = generateCode();

        GameRoom room;
        room.roomId = code;
        room.status = "waiting";
        room.lastActivityTime = nowMs();
        room.board.init();
        room.moveQueue.clear();

        if (preferredColor == "black") {
            room.blackPlayer = hostUsername;
            room.whitePlayer = "";
        } else {
            room.whitePlayer = hostUsername;
            room.blackPlayer = "";
        }

        m_rooms[code] = room;
        return code;
    }

    // Join an existing match room
    bool joinRoom(const string& roomId, const string& username, string& assignedColor, string& errorMsg) {
        lock_guard<mutex> lock(m_mutex);
        auto it = m_rooms.find(roomId);
        if (it == m_rooms.end()) {
            errorMsg = "Room not found. Check the code and try again.";
            return false;
        }

        GameRoom& room = it->second;
        room.lastActivityTime = nowMs();

        // If player is already in the room, re-connect them
        if (room.whitePlayer == username) {
            assignedColor = "white";
            return true;
        }
        if (room.blackPlayer == username) {
            assignedColor = "black";
            return true;
        }

        // Check if room is waiting for opponent
        if (room.status == "waiting") {
            if (room.whitePlayer.empty()) {
                room.whitePlayer = username;
                assignedColor = "white";
            } else if (room.blackPlayer.empty()) {
                room.blackPlayer = username;
                assignedColor = "black";
            }

            // Both players present -> start game
            if (!room.whitePlayer.empty() && !room.blackPlayer.empty()) {
                room.status = "active";
            }
            return true;
        }

        if (room.status == "active") {
            errorMsg = "Room is already full with 2 active players.";
            return false;
        }

        errorMsg = "This match has already concluded.";
        return false;
    }

    // Check if room exists
    bool hasRoom(const string& roomId) const {
        lock_guard<mutex> lock(m_mutex);
        return m_rooms.find(roomId) != m_rooms.end();
    }

    // Get legal moves for a piece in a room
    string getLegalMovesJSON(const string& roomId, int row, int col) {
        lock_guard<mutex> lock(m_mutex);
        auto it = m_rooms.find(roomId);
        if (it == m_rooms.end()) return "[]";

        const GameRoom& room = it->second;
        Color turn = room.board.state.currentTurn;
        auto legal = room.board.getLegalMoves(turn);

        ostringstream oss;
        oss << "[";
        bool first = true;
        for (const auto& m : legal) {
            if (m.fromRow == row && m.fromCol == col) {
                if (!first) oss << ",";
                oss << "{\"toRow\":" << m.toRow << ",\"toCol\":" << m.toCol << "}";
                first = false;
            }
        }
        oss << "]";
        return oss.str();
    }

    // Make move in an online room
    bool makeMove(const string& roomId, const string& username,
                  int fr, int fc, int tr, int tc, PieceType desiredPromotion,
                  int& ptsEarned, string& errorMsg) {
        lock_guard<mutex> lock(m_mutex);
        ptsEarned = 0;

        auto it = m_rooms.find(roomId);
        if (it == m_rooms.end()) {
            errorMsg = "Room not found.";
            return false;
        }

        GameRoom& room = it->second;
        room.lastActivityTime = nowMs();

        if (room.status == "waiting") {
            errorMsg = "Waiting for an opponent to join the match.";
            return false;
        }
        if (room.status != "active") {
            errorMsg = "Game is already finished.";
            return false;
        }

        Color turn = room.board.state.currentTurn;

        // Turn enforcement: White player can only move when turn is White
        if (turn == WHITE && room.whitePlayer != username) {
            errorMsg = "It is White's turn (" + room.whitePlayer + ").";
            return false;
        }
        if (turn == BLACK && room.blackPlayer != username) {
            errorMsg = "It is Black's turn (" + room.blackPlayer + ").";
            return false;
        }

        auto legal = room.board.getLegalMoves(turn);
        Move chosen; chosen.fromRow = -1;

        for (const auto& m : legal) {
            if (m.fromRow == fr && m.fromCol == fc && m.toRow == tr && m.toCol == tc) {
                if (m.isPromotion) {
                    if (m.promoteTo == desiredPromotion) {
                        chosen = m;
                        break;
                    }
                } else {
                    chosen = m;
                    break;
                }
            }
        }

        if (chosen.fromRow < 0) {
            errorMsg = "Illegal move.";
            return false;
        }

        // Calculate score for capture
        Cell& target = room.board.at(tr, tc);
        if (chosen.isCapture && !target.isEmpty()) {
            ptsEarned = piecePoints(target.type);
            if (turn == WHITE) room.whiteScore += ptsEarned;
            else               room.blackScore += ptsEarned;
        }

        // Apply move to room board
        room.board.applyMove(chosen);
        room.moveQueue.push(chosen.toAlgebraic(), (turn == WHITE));
        room.lastFromRow = fr;
        room.lastFromCol = fc;
        room.lastToRow = tr;
        room.lastToCol = tc;
        room.moveCount++;

        // Check if move resulted in checkmate or stalemate
        Color nextTurn = room.board.state.currentTurn;
        if (room.board.isCheckmate(nextTurn)) {
            room.status = "finished";
            room.winner = (nextTurn == WHITE) ? "black" : "white";
            room.winReason = "checkmate";
        } else if (room.board.isStalemate(nextTurn)) {
            room.status = "finished";
            room.winner = "draw";
            room.winReason = "stalemate";
        }

        return true;
    }

    // Resign from online room
    bool resign(const string& roomId, const string& username, string& errorMsg) {
        lock_guard<mutex> lock(m_mutex);
        auto it = m_rooms.find(roomId);
        if (it == m_rooms.end()) {
            errorMsg = "Room not found.";
            return false;
        }

        GameRoom& room = it->second;
        if (room.status == "finished") return true;

        room.lastActivityTime = nowMs();
        room.status = "finished";
        room.winReason = "resignation";

        if (room.whitePlayer == username) {
            room.winner = "black";
        } else if (room.blackPlayer == username) {
            room.winner = "white";
        } else {
            room.winner = "draw";
        }
        return true;
    }

    // Rematch in same room
    bool rematch(const string& roomId, const string& username, string& errorMsg) {
        lock_guard<mutex> lock(m_mutex);
        auto it = m_rooms.find(roomId);
        if (it == m_rooms.end()) {
            errorMsg = "Room not found.";
            return false;
        }

        GameRoom& room = it->second;
        // Swap colors for rematch
        string temp = room.whitePlayer;
        room.whitePlayer = room.blackPlayer;
        room.blackPlayer = temp;

        room.board.init();
        room.moveQueue.clear();
        room.whiteScore = 0;
        room.blackScore = 0;
        room.status = "active";
        room.winner = "";
        room.winReason = "";
        room.lastFromRow = -1;
        room.lastFromCol = -1;
        room.lastToRow = -1;
        room.lastToCol = -1;
        room.moveCount = 0;
        room.lastActivityTime = nowMs();
        return true;
    }

    // Serialize full room state to JSON
    string getRoomStateJSON(const string& roomId, const string& requestingUser = "") const {
        lock_guard<mutex> lock(m_mutex);
        auto it = m_rooms.find(roomId);
        if (it == m_rooms.end()) {
            return "{\"error\":\"Room not found\"}";
        }

        const GameRoom& room = it->second;
        Color turn = room.board.state.currentTurn;
        bool inCheck = room.board.isInCheck(turn);
        bool checkmate = room.board.isCheckmate(turn);
        bool stalemate = room.board.isStalemate(turn);

        string yourColor = "spectator";
        if (requestingUser == room.whitePlayer) yourColor = "white";
        else if (requestingUser == room.blackPlayer) yourColor = "black";

        ostringstream oss;
        oss << "{";
        oss << "\"status\":\"" << room.status << "\",";
        oss << "\"roomId\":\"" << room.roomId << "\",";
        oss << "\"whitePlayer\":\"" << room.whitePlayer << "\",";
        oss << "\"blackPlayer\":\"" << room.blackPlayer << "\",";
        oss << "\"yourColor\":\"" << yourColor << "\",";
        oss << "\"turn\":\"" << (turn == WHITE ? "white" : "black") << "\",";
        oss << "\"inCheck\":" << (inCheck ? "true" : "false") << ",";
        oss << "\"checkmate\":" << (checkmate ? "true" : "false") << ",";
        oss << "\"stalemate\":" << (stalemate ? "true" : "false") << ",";
        oss << "\"winner\":\"" << room.winner << "\",";
        oss << "\"winReason\":\"" << room.winReason << "\",";
        oss << "\"whiteScore\":" << room.whiteScore << ",";
        oss << "\"blackScore\":" << room.blackScore << ",";
        oss << "\"moveCount\":" << room.moveCount << ",";
        oss << "\"lastMove\":{";
        oss << "\"fromRow\":" << room.lastFromRow << ",";
        oss << "\"fromCol\":" << room.lastFromCol << ",";
        oss << "\"toRow\":" << room.lastToRow << ",";
        oss << "\"toCol\":" << room.lastToCol;
        oss << "},";
        oss << "\"moves\":\"" << room.moveQueue.serialize() << "\",";
        oss << "\"board\":" << room.board.toJSON();
        oss << "}";

        return oss.str();
    }

    // Periodically remove stale rooms inactive for > 2 hours
    void cleanupStaleRooms() {
        lock_guard<mutex> lock(m_mutex);
        int64_t current = nowMs();
        const int64_t TWO_HOURS_MS = 2LL * 60 * 60 * 1000;

        for (auto it = m_rooms.begin(); it != m_rooms.end(); ) {
            if (current - it->second.lastActivityTime > TWO_HOURS_MS) {
                it = m_rooms.erase(it);
            } else {
                ++it;
            }
        }
    }
};
