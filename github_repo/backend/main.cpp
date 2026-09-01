// ─────────────────────────────────────────────────────────────────────────────
// ChessVerse Backend — main.cpp
// Pure C/C++ HTTP Server supporting Single-Player AI, Local Versus,
// and Real-Time Online Multiplayer with Matchmaking Room Codes.
//
// Endpoints:
//   POST /login              → { username, password }
//   POST /logout
//   GET  /state              → board + user JSON
//   POST /move               → { fromRow, fromCol, toRow, toCol, promotion }
//   POST /hint               → get hint for current player
//   GET  /moves              → move log (queue serialized)
//   POST /analyze            → { sequence } → best counter move
//   GET  /store              → list store items
//   POST /purchase           → { itemId }
//   POST /equip              → { itemId }
//   GET  /trivia             → today's question
//   POST /trivia/answer      → { answer (0-3) }
//   GET  /leaderboard        → sorted user list
//   POST /admin/adduser      → { username, password, role }
//   POST /newgame            → { mode: "easy"|"medium"|"hard"|"versus", color }
//   POST /ai_move            → trigger AI move (vs AI mode)
//   POST /room/create        → { username, preferredColor } → { roomId, ... }
//   POST /room/join          → { roomId, username }
//   POST /room/state         → { roomId, username } (or GET with query params)
//   POST /room/move          → { roomId, username, fromRow, fromCol, toRow, toCol, promotion }
//   POST /room/legal_moves   → { roomId, row, col }
//   POST /room/resign        → { roomId, username }
//   POST /room/rematch       → { roomId, username }
// ─────────────────────────────────────────────────────────────────────────────

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
  #define CLOSE_SOCKET closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  typedef int SOCKET;
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR   (-1)
  #define CLOSE_SOCKET close
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>

#include "chess/Board.h"
#include "chess/AI.h"
#include "chess/HintEngine.h"
#include "chess/MoveQueue.h"
#include "chess/RoomManager.h"
#include "auth/AuthManager.h"
#include "store/Store.h"
#include "trivia/TriviaManager.h"

using namespace std;

// ─── Global State ─────────────────────────────────────────────────────────────
Board         g_board;
AI*           g_ai       = nullptr;
MoveQueue     g_moveQueue;
RoomManager   g_roomManager;
AuthManager   g_auth("data/users.dat");
Store         g_store("data/store.dat","data/inventory.dat");
TriviaManager g_trivia("data/trivia_records.dat");
Difficulty    g_difficulty = EASY;
bool          g_vsAI       = true;
Color         g_playerColor= WHITE;
HintLimit     g_hint1(-1);    // Player 1 hints (white)
HintLimit     g_hint2(-1);    // Player 2 hints (black / AI mode doesn't use)
bool          g_gameActive  = false;
int           g_whiteScore  = 0;
int           g_blackScore  = 0;
std::mutex    g_apiMutex;

// ─── Simple JSON helpers ──────────────────────────────────────────────────────
string jsonStr(const string& key, const string& val) {
    return "\"" + key + "\":\"" + val + "\"";
}
string jsonInt(const string& key, int val) {
    return "\"" + key + "\":" + to_string(val);
}
string jsonBool(const string& key, bool val) {
    return "\"" + key + "\":" + (val?"true":"false");
}
string ok(const string& body="{}") { return body; }
string err(const string& msg) { return "{\"error\":\""+msg+"\"}"; }

// ─── Parse simple JSON body ────────────────────────────────────────────────────
map<string,string> parseBody(const string& body) {
    map<string,string> m;
    string s = body;
    size_t pos = 0;
    while((pos = s.find('"', pos)) != string::npos) {
        size_t k1 = pos + 1;
        size_t k2 = s.find('"', k1);
        if(k2 == string::npos) break;
        string key = s.substr(k1, k2 - k1);
        size_t colon = s.find(':', k2);
        if(colon == string::npos) break;
        size_t v1 = colon + 1;
        while(v1 < s.size() && (s[v1] == ' ' || s[v1] == '\t' || s[v1] == '\r' || s[v1] == '\n')) v1++;
        string val;
        if(v1 < s.size() && s[v1] == '"') {
            v1++;
            size_t v2 = s.find('"', v1);
            if (v2 != string::npos) {
                val = s.substr(v1, v2 - v1);
                pos = v2 + 1;
            } else {
                pos = string::npos;
            }
        } else {
            size_t v2 = s.find_first_of(",}\r\n", v1);
            if (v2 != string::npos) {
                val = s.substr(v1, v2 - v1);
                pos = v2;
            } else {
                val = s.substr(v1);
                pos = s.size();
            }
            // Trim trailing spaces
            while(!val.empty() && (val.back()==' '||val.back()=='}'||val.back()==',')) val.pop_back();
        }
        m[key] = val;
    }
    return m;
}

// ─── Parse URL Query Parameters ────────────────────────────────────────────────
map<string,string> parseQuery(const string& query) {
    map<string,string> params;
    istringstream iss(query);
    string pair;
    while(getline(iss, pair, '&')) {
        size_t eq = pair.find('=');
        if(eq != string::npos) {
            params[pair.substr(0, eq)] = pair.substr(eq + 1);
        } else if(!pair.empty()) {
            params[pair] = "";
        }
    }
    return params;
}

// ─── Route Handlers ───────────────────────────────────────────────────────────
string handleLogin(const string& body) {
    auto p = parseBody(body);
    User* u = g_auth.login(p["username"], p["password"]);
    if(!u) return err("Invalid credentials");
    return "{" + jsonStr("status","ok") + "," + g_auth.currentUserJSON().substr(1);
}

string handleLogout() {
    g_auth.logout();
    return ok("{\"status\":\"ok\"}");
}

string handleNewGame(const string& body) {
    auto p = parseBody(body);
    string mode = p.count("mode") ? p["mode"] : "easy";

    if(mode=="easy")   { g_difficulty=EASY;   g_hint1=HintLimit(-1); g_hint2=HintLimit(-1); }
    if(mode=="medium") { g_difficulty=MEDIUM;  g_hint1=HintLimit(3);  g_hint2=HintLimit(3);  }
    if(mode=="hard")   { g_difficulty=HARD;    g_hint1=HintLimit(0);  g_hint2=HintLimit(0);  }
    if(mode=="versus") {
        g_difficulty=MEDIUM; g_vsAI=false;
        User* u = g_auth.getCurrentUser();
        int exp = u ? u->exp : 0;
        int hints = (exp >= 6000) ? -1 : (exp >= 1500) ? 5 : 3;
        g_hint1 = HintLimit(hints);
        g_hint2 = HintLimit(3);
    } else {
        g_vsAI = true;
        g_playerColor = (p.count("color") && p["color"]=="black") ? BLACK : WHITE;
    }

    g_ai = new AI(g_difficulty);
    g_board.init();
    g_moveQueue.clear();
    g_whiteScore = g_blackScore = 0;
    g_gameActive = true;

    return "{\"status\":\"ok\",\"mode\":\"" + mode + "\"}";
}

string handleMove(const string& body) {
    if(!g_gameActive) return err("No active game");
    auto p = parseBody(body);

    int fr=p["fromRow"][0]-'0', fc=p["fromCol"][0]-'0';
    int tr=p["toRow"][0]-'0',   tc=p["toCol"][0]-'0';

    Color turn = g_board.state.currentTurn;
    auto legal = g_board.getLegalMoves(turn);

    Move chosen; chosen.fromRow=-1;
    PieceType desiredPromotion = QUEEN;
    if (p.count("promotion")) {
        string prom = p["promotion"];
        if (prom == "q") desiredPromotion = QUEEN;
        else if (prom == "r") desiredPromotion = ROOK;
        else if (prom == "b") desiredPromotion = BISHOP;
        else if (prom == "n") desiredPromotion = KNIGHT;
    }

    for(auto& m : legal) {
        if(m.fromRow==fr&&m.fromCol==fc&&m.toRow==tr&&m.toCol==tc) {
            if (m.isPromotion) {
                if (m.promoteTo == desiredPromotion) {
                    chosen = m; break;
                }
            } else {
                chosen=m; break;
            }
        }
    }
    if(chosen.fromRow<0) return err("Illegal move");

    int pts = 0;
    Cell& target = g_board.at(tr,tc);
    if(chosen.isCapture && !target.isEmpty()) {
        pts = piecePoints(target.type);
        if (g_ai) pts = (int)(pts * g_ai->multiplier());
        if(turn==WHITE) g_whiteScore+=pts;
        else            g_blackScore+=pts;
        User* u = g_auth.getCurrentUser();
        if(u) g_auth.addPoints(*u, pts);
    }

    g_board.applyMove(chosen);

    bool isWhite = (turn==WHITE);
    g_moveQueue.push(chosen.toAlgebraic(), isWhite);

    ostringstream oss;
    oss << "{";
    oss << "\"board\":" << g_board.toJSON() << ",";
    oss << "\"pointsEarned\":" << pts << ",";
    oss << "\"whiteScore\":" << g_whiteScore << ",";
    oss << "\"blackScore\":" << g_blackScore << ",";
    oss << "\"moves\":\"" << g_moveQueue.serialize() << "\"";
    oss << "}";
    return oss.str();
}

string handleAIMove() {
    Board boardCopy;
    Difficulty diff;
    Color aiColor;
    float multiplier = 1.0f;
    {
        std::lock_guard<std::mutex> lock(g_apiMutex);
        if(!g_gameActive || !g_vsAI || !g_ai) return err("Not in AI mode");
        aiColor = (g_playerColor==WHITE)?BLACK:WHITE;
        if(g_board.state.currentTurn != aiColor) return err("Not AI turn");
        
        boardCopy = g_board;
        diff = g_difficulty;
        multiplier = g_ai->multiplier();
    }

    AI localAI(diff);
    Move aiMove = localAI.getBestMove(boardCopy, aiColor);

    std::lock_guard<std::mutex> lock(g_apiMutex);
    if(!g_gameActive || !g_vsAI || !g_ai) return err("Game was terminated during calculation");
    if(g_board.state.currentTurn != aiColor) return err("Game state changed during calculation");
    if(g_board.history.size() != boardCopy.history.size()) return err("Game state changed during calculation");
    if(aiMove.fromRow<0) return err("AI has no moves");

    Cell& target = g_board.at(aiMove.toRow, aiMove.toCol);
    int pts = 0;
    if(aiMove.isCapture && !target.isEmpty()) {
        pts = piecePoints(target.type);
        pts = (int)(pts * multiplier);
        if(aiColor==WHITE) g_whiteScore+=pts;
        else               g_blackScore+=pts;
    }
    g_board.applyMove(aiMove);
    g_moveQueue.push(aiMove.toAlgebraic(), aiColor==WHITE);

    ostringstream oss;
    oss << "{";
    oss << "\"move\":\"" << aiMove.toAlgebraic() << "\",";
    oss << "\"board\":" << g_board.toJSON() << ",";
    oss << "\"whiteScore\":" << g_whiteScore << ",";
    oss << "\"blackScore\":" << g_blackScore << ",";
    oss << "\"moves\":\"" << g_moveQueue.serialize() << "\"";
    oss << "}";
    return oss.str();
}

string handleHint(const string& body) {
    if(!g_gameActive) return err("No active game");
    Color turn = g_board.state.currentTurn;
    HintLimit& limit = (turn==WHITE) ? g_hint1 : g_hint2;

    Move hint = HintEngine::getHint(g_board, turn, limit);
    if(hint.fromRow<0) return err("No hints remaining");

    ostringstream oss;
    oss << "{";
    oss << "\"from\":\"" << (char)('a'+hint.fromCol) << (hint.fromRow+1) << "\",";
    oss << "\"to\":\"" << (char)('a'+hint.toCol) << (hint.toRow+1) << "\",";
    oss << "\"fromRow\":" << hint.fromRow << ",\"fromCol\":" << hint.fromCol << ",";
    oss << "\"toRow\":" << hint.toRow << ",\"toCol\":" << hint.toCol << ",";
    oss << "\"hintsLeft\":\"" << HintEngine::hintLabel(limit) << "\"";
    oss << "}";
    return oss.str();
}

string handleAnalyze(const string& body) {
    auto p = parseBody(body);
    string seq = p["sequence"];
    Color c = (p.count("color")&&p["color"]=="black") ? BLACK : WHITE;
    string counter = MoveAnalyzer::getBestCounter(seq, c, g_difficulty);
    return "{\"counterMove\":\"" + counter + "\"}";
}

string handleStore() {
    User* u = g_auth.getCurrentUser();
    if(!u) return err("Not logged in");
    return g_store.itemsToJSON(u->exp, string(u->username));
}

string handlePurchase(const string& body) {
    User* u = g_auth.getCurrentUser();
    if(!u) return err("Not logged in");
    auto p = parseBody(body);
    int result = g_store.purchase(string(u->username), p["itemId"], u->points, u->exp);
    string msgs[]={"Purchase successful","Insufficient points","Rank too low","Already owned","Item not found"};
    return "{\"result\":" + to_string(result) + ",\"message\":\"" + msgs[result] + "\"}";
}

string handleEquip(const string& body) {
    User* u = g_auth.getCurrentUser();
    if(!u) return err("Not logged in");
    auto p = parseBody(body);
    bool ok2 = g_store.equip(string(u->username), p["itemId"]);
    return ok2 ? ok("{\"status\":\"ok\"}") : err("Cannot equip");
}

string handleTrivia() {
    User* u = g_auth.getCurrentUser();
    if(!u) return err("Not logged in");
    Question* q = g_trivia.getDailyQuestion(string(u->username));
    bool canPlay = g_trivia.canPlayToday(string(u->username));
    if(!canPlay) return "{\"canPlay\":false,\"message\":\"Already played today\"}";
    if(!q) return err("No questions available");
    return "{\"canPlay\":true,\"question\":" + g_trivia.questionToJSON(q) + "}";
}

string handleTriviaAnswer(const string& body) {
    User* u = g_auth.getCurrentUser();
    if(!u) return err("Not logged in");
    auto p = parseBody(body);
    int answer = stoi(p["answer"]);
    int bonus  = g_trivia.submitAnswer(string(u->username), answer);
    if(bonus > 0) g_auth.addPoints(*u, bonus);
    bool correct = (bonus > 0);
    return "{\"correct\":" + string(correct?"true":"false") +
           ",\"bonus\":" + to_string(bonus) +
           ",\"newPoints\":" + to_string(u->points) + "}";
}

string handleUndo() {
    if(g_board.history.empty()) return "{\"status\":\"error\",\"message\":\"Nothing to undo\"}";
    if(g_vsAI) {
        g_board.undoMove();
        g_moveQueue.popLast();
        if(!g_board.history.empty()) {
            g_board.undoMove();
            g_moveQueue.popLast();
        }
    } else {
        g_board.undoMove();
        g_moveQueue.popLast();
    }
    return "{\"status\":\"ok\"}";
}

string handleResign() {
    if(!g_gameActive) return err("No active game");
    g_gameActive = false;
    return "{\"status\":\"ok\"}";
}

string handleLeaderboard() {
    auto users = g_auth.getLeaderboard();
    ostringstream oss;
    oss << "[";
    for(int i=0;i<(int)users.size();i++) {
        auto& u = users[i];
        RankInfo rank = getRank(u.exp);
        oss << "{";
        oss << "\"rank\":" << (i+1) << ",";
        oss << "\"username\":\"" << u.username << "\",";
        oss << "\"exp\":" << u.exp << ",";
        oss << "\"points\":" << u.points << ",";
        oss << "\"streak\":" << u.streak << ",";
        oss << "\"title\":\"" << rank.title << "\",";
        oss << "\"tier\":\"" << rank.tier << "\"";
        oss << "}";
        if(i<(int)users.size()-1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

string handleAdminAddUser(const string& body) {
    auto p = parseBody(body);
    Role role = (p["role"]=="admin") ? ADMIN : PLAYER;
    bool success = g_auth.addUser(p["username"], p["password"], role);
    return success ? ok("{\"status\":\"ok\"}") : err("Could not add user");
}

string handleLegalMoves(const string& body) {
    auto p = parseBody(body);
    int r = stoi(p["row"]), c = stoi(p["col"]);
    Color turn = g_board.state.currentTurn;
    auto legal = g_board.getLegalMoves(turn);
    ostringstream oss;
    oss << "[";
    bool first=true;
    for(auto& m : legal) {
        if(m.fromRow==r && m.fromCol==c) {
            if(!first) oss<<",";
            oss<<"{\"toRow\":"<<m.toRow<<",\"toCol\":"<<m.toCol<<"}";
            first=false;
        }
    }
    oss << "]";
    return oss.str();
}

// ─── Online Room Route Handlers (Multi-Room Matchmaking) ──────────────────────
string handleRoomCreate(const string& body) {
    auto p = parseBody(body);
    string username = p.count("username") ? p["username"] : "";
    if (username.empty()) {
        User* u = g_auth.getCurrentUser();
        if (u) username = u->username;
    }
    if (username.empty()) username = "Player1";

    cout << "[API /room/create] body=[" << body << "] parsed_user=[" << username << "]" << endl;

    string prefColor = p.count("preferredColor") ? p["preferredColor"] : "white";
    string roomId = g_roomManager.createRoom(username, prefColor);

    ostringstream oss;
    oss << "{\"status\":\"ok\",\"roomId\":\"" << roomId << "\",\"yourColor\":\"" << (prefColor=="black"?"black":"white") << "\"}";
    return oss.str();
}

string handleRoomJoin(const string& body) {
    auto p = parseBody(body);
    string roomId = p.count("roomId") ? p["roomId"] : "";
    string username = p.count("username") ? p["username"] : "";
    if (username.empty()) {
        User* u = g_auth.getCurrentUser();
        if (u) username = u->username;
    }
    if (username.empty()) username = "Player2";

    // Auto capitalize room code
    for (char& c : roomId) c = toupper(c);

    string assignedColor = "";
    string errorMsg = "";
    bool success = g_roomManager.joinRoom(roomId, username, assignedColor, errorMsg);
    if (!success) {
        return err(errorMsg);
    }

    return g_roomManager.getRoomStateJSON(roomId, username);
}

string handleRoomState(const string& body, const map<string,string>& queryParams) {
    auto p = parseBody(body);
    string roomId = p.count("roomId") ? p["roomId"] : (queryParams.count("roomId") ? queryParams.at("roomId") : "");
    string username = p.count("username") ? p["username"] : (queryParams.count("username") ? queryParams.at("username") : "");
    if (username.empty()) {
        User* u = g_auth.getCurrentUser();
        if (u) username = u->username;
    }

    for (char& c : roomId) c = toupper(c);
    return g_roomManager.getRoomStateJSON(roomId, username);
}

string handleRoomMove(const string& body) {
    auto p = parseBody(body);
    string roomId = p.count("roomId") ? p["roomId"] : "";
    string username = p.count("username") ? p["username"] : "";
    if (username.empty()) {
        User* u = g_auth.getCurrentUser();
        if (u) username = u->username;
    }

    for (char& c : roomId) c = toupper(c);

    if (roomId.empty() || username.empty()) return err("Missing roomId or username");
    if (!p.count("fromRow") || !p.count("fromCol") || !p.count("toRow") || !p.count("toCol")) {
        return err("Missing coordinates");
    }

    int fr = stoi(p["fromRow"]);
    int fc = stoi(p["fromCol"]);
    int tr = stoi(p["toRow"]);
    int tc = stoi(p["toCol"]);

    PieceType desiredPromotion = QUEEN;
    if (p.count("promotion")) {
        string prom = p["promotion"];
        if (prom == "q") desiredPromotion = QUEEN;
        else if (prom == "r") desiredPromotion = ROOK;
        else if (prom == "b") desiredPromotion = BISHOP;
        else if (prom == "n") desiredPromotion = KNIGHT;
    }

    int ptsEarned = 0;
    string errorMsg = "";
    bool success = g_roomManager.makeMove(roomId, username, fr, fc, tr, tc, desiredPromotion, ptsEarned, errorMsg);
    if (!success) {
        return err(errorMsg);
    }

    // Award points if logged in
    if (ptsEarned > 0) {
        User* u = g_auth.getCurrentUser();
        if (u && string(u->username) == username) {
            g_auth.addPoints(*u, ptsEarned);
        }
    }

    return g_roomManager.getRoomStateJSON(roomId, username);
}

string handleRoomLegalMoves(const string& body) {
    auto p = parseBody(body);
    string roomId = p.count("roomId") ? p["roomId"] : "";
    for (char& c : roomId) c = toupper(c);
    int r = stoi(p["row"]);
    int c = stoi(p["col"]);
    return g_roomManager.getLegalMovesJSON(roomId, r, c);
}

string handleRoomResign(const string& body) {
    auto p = parseBody(body);
    string roomId = p.count("roomId") ? p["roomId"] : "";
    string username = p.count("username") ? p["username"] : "";
    for (char& c : roomId) c = toupper(c);
    string errorMsg;
    g_roomManager.resign(roomId, username, errorMsg);
    return g_roomManager.getRoomStateJSON(roomId, username);
}

string handleRoomRematch(const string& body) {
    auto p = parseBody(body);
    string roomId = p.count("roomId") ? p["roomId"] : "";
    string username = p.count("username") ? p["username"] : "";
    for (char& c : roomId) c = toupper(c);
    string errorMsg;
    g_roomManager.rematch(roomId, username, errorMsg);
    return g_roomManager.getRoomStateJSON(roomId, username);
}

// ─── HTTP Server Response Builder ─────────────────────────────────────────────
string buildResponse(const string& body, int status=200, const string& contentType="application/json") {
    string statusText = (status==200)?"OK":(status==404?"Not Found":"Bad Request");
    string resp = "HTTP/1.1 " + to_string(status) + " " + statusText + "\r\n";
    resp += "Content-Type: " + contentType + "\r\n";
    resp += "Access-Control-Allow-Origin: *\r\n";
    resp += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    resp += "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    resp += "Content-Length: " + to_string(body.size()) + "\r\n";
    resp += "\r\n" + body;
    return resp;
}

// ─── Static File Server ──────────────────────────────────────────────────────
string readFile(const string& filepath) {
    ifstream f(filepath, ios::binary);
    if(!f.is_open()) return "";
    ostringstream oss;
    oss << f.rdbuf();
    return oss.str();
}

string getMimeType(const string& path) {
    if(path.find(".html")!=string::npos) return "text/html; charset=utf-8";
    if(path.find(".css")!=string::npos)  return "text/css; charset=utf-8";
    if(path.find(".js")!=string::npos)   return "application/javascript; charset=utf-8";
    if(path.find(".json")!=string::npos) return "application/json";
    if(path.find(".png")!=string::npos)  return "image/png";
    if(path.find(".jpg")!=string::npos || path.find(".jpeg")!=string::npos) return "image/jpeg";
    if(path.find(".svg")!=string::npos)  return "image/svg+xml";
    if(path.find(".wav")!=string::npos)  return "audio/wav";
    if(path.find(".mp3")!=string::npos)  return "audio/mpeg";
    return "text/plain";
}

string serveStatic(const string& urlPath) {
    string filePath = "frontend";
    if(urlPath == "/" || urlPath.empty()) filePath += "/index.html";
    else filePath += urlPath;

    // Security: prevent directory traversal
    if(filePath.find("..")!=string::npos) return buildResponse("Forbidden", 403, "text/plain");

    string content = readFile(filePath);
    if(content.empty()) return buildResponse("File not found: " + filePath, 404, "text/plain");
    return buildResponse(content, 200, getMimeType(filePath));
}

void handleClient(SOCKET client) {
    vector<char> buf(8192, 0);
    int received = recv(client, buf.data(), (int)buf.size()-1, 0);
    if(received<=0) { CLOSE_SOCKET(client); return; }

    string request(buf.data(), received);
    istringstream iss(request);
    string method, fullPath, version;
    iss >> method >> fullPath >> version;

    // Split path and query parameters
    string path = fullPath;
    string queryString = "";
    size_t qmark = fullPath.find('?');
    if (qmark != string::npos) {
        path = fullPath.substr(0, qmark);
        queryString = fullPath.substr(qmark + 1);
    }
    auto queryParams = parseQuery(queryString);

    // Find body (supports both \r\n\r\n and \n\n header terminators)
    string body;
    size_t bodyStart = request.find("\r\n\r\n");
    if(bodyStart != string::npos) {
        body = request.substr(bodyStart + 4);
    } else {
        bodyStart = request.find("\n\n");
        if(bodyStart != string::npos) {
            body = request.substr(bodyStart + 2);
        }
    }

    // Parse Content-Length header to ensure complete body is received
    int contentLength = 0;
    size_t clPos = request.find("Content-Length:");
    if (clPos == string::npos) clPos = request.find("content-length:");
    if (clPos != string::npos) {
        size_t lineEnd = request.find("\r\n", clPos);
        if (lineEnd == string::npos) lineEnd = request.find("\n", clPos);
        string clStr = request.substr(clPos + 15, lineEnd - (clPos + 15));
        while(!clStr.empty() && (clStr.front()==' '||clStr.front()=='\t')) clStr.erase(clStr.begin());
        while(!clStr.empty() && (clStr.back()==' '||clStr.back()=='\r'||clStr.back()=='\n'||clStr.back()=='\t')) clStr.pop_back();
        if (!clStr.empty()) {
            try { contentLength = stoi(clStr); } catch(...) { contentLength = 0; }
        }
    }

    // Receive remaining body bytes if needed
    while((int)body.length() < contentLength) {
        int more = recv(client, buf.data(), (int)buf.size()-1, 0);
        if(more <= 0) break;
        body.append(buf.data(), more);
    }

    string response;
    // Handle preflight
    if(method=="OPTIONS") {
        response = buildResponse("{}");
    }
    // API Route dispatch
    else if (path != "/" && path.find('.') == string::npos) {
        if(method=="POST" && path=="/login") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleLogin(body));
        }
        else if(method=="POST" && path=="/logout") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleLogout());
        }
        else if(method=="POST" && path=="/newgame") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleNewGame(body));
        }
        else if(method=="POST" && path=="/move") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleMove(body));
        }
        else if(method=="POST" && path=="/ai_move") {
            response = buildResponse(handleAIMove());
        }
        else if(method=="POST" && path=="/undo") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleUndo());
        }
        else if(method=="POST" && path=="/resign") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleResign());
        }
        else if(method=="POST" && path=="/hint") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleHint(body));
        }
        else if(method=="POST" && path=="/analyze") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleAnalyze(body));
        }
        else if(method=="GET"  && path=="/store") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleStore());
        }
        else if(method=="POST" && path=="/purchase") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handlePurchase(body));
        }
        else if(method=="POST" && path=="/equip") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleEquip(body));
        }
        else if(method=="GET"  && path=="/trivia") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleTrivia());
        }
        else if(method=="POST" && path=="/trivia/answer") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleTriviaAnswer(body));
        }
        else if(method=="GET"  && path=="/leaderboard") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleLeaderboard());
        }
        else if(method=="GET"  && path=="/state") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse("{\"board\":" + g_board.toJSON() + ",\"user\":" + g_auth.currentUserJSON() + "}");
        }
        else if(method=="GET"  && path=="/moves") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse("{\"moves\":\"" + g_moveQueue.serialize() + "\",\"log\":" + [&](){
                ostringstream o; o<<"["; auto v=g_moveQueue.toVector(); for(int i=0;i<(int)v.size();i++){o<<"\""<<v[i]<<"\""; if(i<(int)v.size()-1)o<<",";}o<<"]"; return o.str(); }() + "}");
        }
        else if(method=="POST" && path=="/legal_moves") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleLegalMoves(body));
        }
        else if(method=="POST" && path=="/admin/adduser") {
            std::lock_guard<std::mutex> lock(g_apiMutex);
            response = buildResponse(handleAdminAddUser(body));
        }
        // ── Room Endpoints ──
        else if(method=="POST" && path=="/room/create") {
            response = buildResponse(handleRoomCreate(body));
        }
        else if(method=="POST" && path=="/room/join") {
            response = buildResponse(handleRoomJoin(body));
        }
        else if((method=="POST" || method=="GET") && path=="/room/state") {
            response = buildResponse(handleRoomState(body, queryParams));
        }
        else if(method=="POST" && path=="/room/move") {
            response = buildResponse(handleRoomMove(body));
        }
        else if(method=="POST" && path=="/room/legal_moves") {
            response = buildResponse(handleRoomLegalMoves(body));
        }
        else if(method=="POST" && path=="/room/resign") {
            response = buildResponse(handleRoomResign(body));
        }
        else if(method=="POST" && path=="/room/rematch") {
            response = buildResponse(handleRoomRematch(body));
        }
        else {
            response = buildResponse(err("Not found"), 404);
        }
    }
    // Static file serving (frontend HTML/CSS/JS)
    else if(method=="GET" || method=="HEAD") response = serveStatic(path);
    else                   response = buildResponse(err("Not found"), 404);

    send(client, response.c_str(), (int)response.size(), 0);
    CLOSE_SOCKET(client);
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main() {
    #ifdef _WIN32
      CreateDirectoryA("data", nullptr);
      WSADATA wsa;
      WSAStartup(MAKEWORD(2,2), &wsa);
    #else
      mkdir("data", 0777);
    #endif

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    #ifdef _WIN32
      setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    #else
      setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif

    int port = 8080;
    const char* portEnv = getenv("PORT");
    if (portEnv && strlen(portEnv) > 0) {
        try { port = stoi(portEnv); } catch(...) { port = 8080; }
    }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cout << "[ERROR] Could not bind to port " << port << ". Check if another instance is running.\n";
        return 1;
    }

    listen(server, 20);

    cout << "╔═══════════════════════════════════════════════════╗\n";
    cout << "║     ChessVerse Multiplayer Backend (C/C++)        ║\n";
    cout << "║  Running on port " << port << "                            ║\n";
    cout << "║  Running at http://localhost:8080                 ║\n";
    cout << "║  Multiplayer Room Matchmaking: ENABLED            ║\n";
    cout << "║  Open frontend/index.html in phone or PC browser  ║\n";
    cout << "╚═══════════════════════════════════════════════════╝\n";

    while(true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        SOCKET client = accept(server, (sockaddr*)&clientAddr, &clientLen);
        if(client == INVALID_SOCKET) continue;
        std::thread(handleClient, client).detach();
    }

    CLOSE_SOCKET(server);
    #ifdef _WIN32
      WSACleanup();
    #endif
    return 0;
}
