#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <ctime>
using namespace std;

// ─── User & Auth System ───────────────────────────────────────────────────────
// File-based authentication using C++ struct serialization to users.dat

enum Role { PLAYER, ADMIN };

// EXP rank thresholds
struct RankInfo {
    string title;
    string tier;   // "pawn","knight","bishop","rook","queen","king"
    int    minExp;
};

static const RankInfo RANKS[] = {
    {"Beginner",    "pawn",   0},
    {"Apprentice",  "knight", 500},
    {"Tactician",   "rook",   1500},
    {"Strategist",  "bishop", 3000},
    {"Grandmaster", "queen",  6000},
    {"Legend",      "king",   10000}
};

inline RankInfo getRank(int exp) {
    RankInfo best = RANKS[0];
    for(auto& r : RANKS) if(exp >= r.minExp) best = r;
    return best;
}

struct User {
    char     username[32];
    char     passwordHash[64];  // simple hash (for demo, use SHA-256 in prod)
    Role     role;
    int      points;
    int      exp;
    int      streak;            // daily consecutive game/login streak
    time_t   lastLoginDate;     // for streak tracking
    int      hintsRemaining;    // for current session
    char     equippedSkin[32];  // board skin name
    char     equippedPieces[32];// piece skin name
    bool     isActive;

    User() {
        memset(this, 0, sizeof(User));
        role=PLAYER; points=0; exp=0; streak=0;
        hintsRemaining=3; isActive=true;
        strcpy(equippedSkin,"classic");
        strcpy(equippedPieces,"default");
    }
};

// ─── Simple hash (demo quality) ───────────────────────────────────────────────
inline string simpleHash(const string& s) {
    unsigned long h = 5381;
    for(char c : s) h = ((h<<5)+h) + c;
    ostringstream oss; oss << hex << h;
    return oss.str();
}

// ─── Auth Manager ─────────────────────────────────────────────────────────────
class AuthManager {
private:
    vector<User> users;
    string       dataFile;
    User*        currentUser = nullptr;

public:
    explicit AuthManager(const string& file = "data/users.dat") : dataFile(file) {
        loadAll();
        // Create default users if empty
        if(users.empty()) {
            createDefaultUsers();
            saveAll();
        }
    }

    // ── Login ────────────────────────────────────────────────────────────────
    User* login(const string& username, const string& password) {
        string hash = simpleHash(password);
        for(auto& u : users) {
            if(string(u.username)==username && string(u.passwordHash)==hash && u.isActive) {
                updateStreak(u);
                currentUser = &u;
                saveAll();
                return currentUser;
            }
        }
        return nullptr;  // invalid credentials
    }

    void logout() { currentUser = nullptr; }

    User* getCurrentUser() { return currentUser; }

    // ── Admin: Add User ──────────────────────────────────────────────────────
    bool addUser(const string& username, const string& password, Role role=PLAYER) {
        if(!currentUser || currentUser->role != ADMIN) return false;
        for(auto& u : users) if(string(u.username)==username) return false; // exists
        User nu;
        strncpy(nu.username, username.c_str(), 31);
        string hash = simpleHash(password);
        strncpy(nu.passwordHash, hash.c_str(), 63);
        nu.role = role;
        users.push_back(nu);
        saveAll();
        return true;
    }

    // ── Add Points & EXP ─────────────────────────────────────────────────────
    void addPoints(User& u, int pts) {
        u.points += pts;
        u.exp    += pts / 2;   // EXP = half of points earned
        saveAll();
    }

    // ── Deduct Points (store purchase) ───────────────────────────────────────
    bool spendPoints(User& u, int cost) {
        if(u.points < cost) return false;
        u.points -= cost;
        saveAll();
        return true;
    }

    // ── Get all users (admin only) ───────────────────────────────────────────
    vector<User> getAllUsers() {
        if(currentUser && currentUser->role==ADMIN) return users;
        return {};
    }

    // ── Get user by name ─────────────────────────────────────────────────────
    User* findUser(const string& username) {
        for(auto& u : users) if(string(u.username)==username) return &u;
        return nullptr;
    }

    // ── Leaderboard (sorted by exp) ──────────────────────────────────────────
    vector<User> getLeaderboard() {
        vector<User> copy = users;
        sort(copy.begin(), copy.end(), [](const User& a, const User& b){
            return a.exp > b.exp;
        });
        return copy;
    }

    // ── Serialize current user to JSON string (for JS) ───────────────────────
    string currentUserJSON() {
        if(!currentUser) return "null";
        User& u = *currentUser;
        RankInfo rank = getRank(u.exp);
        ostringstream oss;
        oss << "{";
        oss << "\"username\":\"" << u.username << "\",";
        oss << "\"role\":\"" << (u.role==ADMIN?"admin":"player") << "\",";
        oss << "\"points\":" << u.points << ",";
        oss << "\"exp\":" << u.exp << ",";
        oss << "\"streak\":" << u.streak << ",";
        oss << "\"rank\":\"" << rank.title << "\",";
        oss << "\"tier\":\"" << rank.tier << "\",";
        oss << "\"skin\":\"" << u.equippedSkin << "\",";
        oss << "\"pieces\":\"" << u.equippedPieces << "\",";
        oss << "\"hints\":" << u.hintsRemaining;
        oss << "}";
        return oss.str();
    }

private:
    // ── Streak update (Fixed: localtime_s with separate buffers) ──────────
    void updateStreak(User& u) {
        time_t now = time(nullptr);
        struct tm today_buf, last_buf;
#ifdef _WIN32
        localtime_s(&today_buf, &now);
#else
        localtime_r(&now, &today_buf);
#endif

        int daysDiff = 0;
        if(u.lastLoginDate != 0) {
#ifdef _WIN32
            localtime_s(&last_buf, &u.lastLoginDate);
#else
            localtime_r(&u.lastLoginDate, &last_buf);
#endif
            // Calculate calendar day difference
            time_t midnight_now  = now  - (today_buf.tm_hour*3600+today_buf.tm_min*60+today_buf.tm_sec);
            time_t midnight_last = u.lastLoginDate - (last_buf.tm_hour*3600+last_buf.tm_min*60+last_buf.tm_sec);
            daysDiff = (int)((midnight_now - midnight_last) / 86400);
        }

        if(daysDiff == 1)      u.streak++;          // consecutive day
        else if(daysDiff > 1)  u.streak = 1;        // broken streak
        else if(daysDiff == 0) {}                    // same day, no change

        u.lastLoginDate = now;
    }

    // ── File I/O ─────────────────────────────────────────────────────────────
    void loadAll() {
        ifstream f(dataFile, ios::binary);
        if(!f.is_open()) return;
        User u;
        while(f.read(reinterpret_cast<char*>(&u), sizeof(User)))
            users.push_back(u);
        f.close();
    }

    void saveAll() {
        ofstream f(dataFile, ios::binary | ios::trunc);
        for(auto& u : users)
            f.write(reinterpret_cast<const char*>(&u), sizeof(User));
        f.close();
    }

    void createDefaultUsers() {
        // Admin user
        User admin;
        strncpy(admin.username, "admin", 31);
        string h = simpleHash("10on10");
        strncpy(admin.passwordHash, h.c_str(), 63);
        admin.role = ADMIN;
        users.push_back(admin);
    }
};
