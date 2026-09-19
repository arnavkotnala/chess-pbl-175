#pragma once

/*
 * ─── Auth Manager (C++ OOP — public/private access control) ──────────────────
 * File-based authentication using C++ struct serialization to users.dat.
 * Demonstrates OOP concepts: encapsulation, public/private access modifiers.
 *
 * Features:
 *   - Login / Logout
 *   - Admin: add users
 *   - Points & EXP system
 *   - Rank progression (Beginner → Legend)
 *   - Daily login streak
 *   - Leaderboard (uses C merge sort from dsa/sort.h)
 */

#include <cstring>
#include <cstdio>
#include <ctime>

extern "C" {
    #include "../dsa/sort.h"
}

enum Role { PLAYER, ADMIN };

// EXP rank thresholds
struct RankInfo {
    char title[32];
    char tier[16];    // "pawn","knight","bishop","rook","queen","king"
    int  minExp;
};

static const RankInfo RANKS[] = {
    {"Beginner",    "pawn",   0},
    {"Apprentice",  "knight", 500},
    {"Tactician",   "rook",   1500},
    {"Strategist",  "bishop", 3000},
    {"Grandmaster", "queen",  6000},
    {"Legend",      "king",   10000}
};
static const int RANK_COUNT = 6;

inline RankInfo getRank(int exp) {
    RankInfo best = RANKS[0];
    for (int i = 0; i < RANK_COUNT; i++)
        if (exp >= RANKS[i].minExp) best = RANKS[i];
    return best;
}

struct User {
    char   username[32];
    char   passwordHash[64];
    Role   role;
    int    points;
    int    exp;
    int    streak;
    time_t lastLoginDate;
    int    hintsRemaining;
    char   equippedSkin[32];
    char   equippedPieces[32];
    bool   isActive;

    User() {
        memset(this, 0, sizeof(User));
        role = PLAYER; points = 0; exp = 0; streak = 0;
        hintsRemaining = 3; isActive = true;
        strcpy(equippedSkin, "classic");
        strcpy(equippedPieces, "default");
    }
};

// Simple hash (demo quality)
inline void simpleHash(const char* input, char* output, int maxLen) {
    unsigned long h = 5381;
    for (int i = 0; input[i]; i++)
        h = ((h << 5) + h) + input[i];
    snprintf(output, maxLen, "%lx", h);
}

// Comparison function for sorting by EXP (descending) — used with C merge sort
static int compareByExpDesc(const void* a, const void* b) {
    const User* ua = (const User*)a;
    const User* ub = (const User*)b;
    return ub->exp - ua->exp;  // descending
}

#define MAX_USERS 100

class AuthManager {
// ─── PUBLIC interface ─────────────────────────────────────────────────────────
public:
    explicit AuthManager(const char* file = "data/users.dat") {
        strncpy(dataFile, file, 255);
        userCount = 0;
        currentUser = nullptr;
        loadAll();
        if (userCount == 0) {
            createDefaultUsers();
            saveAll();
        }
    }

    // ── Login ────────────────────────────────────────────────────────────────
    User* login(const char* username, const char* password) {
        char hash[64];
        simpleHash(password, hash, 64);
        for (int i = 0; i < userCount; i++) {
            if (strcmp(users[i].username, username) == 0 &&
                strcmp(users[i].passwordHash, hash) == 0 &&
                users[i].isActive) {
                updateStreak(users[i]);
                currentUser = &users[i];
                saveAll();
                return currentUser;
            }
        }
        return nullptr;
    }

    void logout() { currentUser = nullptr; }

    User* getCurrentUser() { return currentUser; }

    // ── Register (public — any user can register) ────────────────────────────
    bool registerUser(const char* username, const char* password) {
        // Check if username already exists
        for (int i = 0; i < userCount; i++)
            if (strcmp(users[i].username, username) == 0) return false;
        if (userCount >= MAX_USERS) return false;

        User nu;
        strncpy(nu.username, username, 31);
        char hash[64];
        simpleHash(password, hash, 64);
        strncpy(nu.passwordHash, hash, 63);
        nu.role = PLAYER;
        users[userCount++] = nu;
        saveAll();
        return true;
    }

    // ── Admin: Add User ──────────────────────────────────────────────────────
    bool addUser(const char* username, const char* password, Role role = PLAYER) {
        if (!currentUser || currentUser->role != ADMIN) return false;
        for (int i = 0; i < userCount; i++)
            if (strcmp(users[i].username, username) == 0) return false;
        if (userCount >= MAX_USERS) return false;

        User nu;
        strncpy(nu.username, username, 31);
        char hash[64];
        simpleHash(password, hash, 64);
        strncpy(nu.passwordHash, hash, 63);
        nu.role = role;
        users[userCount++] = nu;
        saveAll();
        return true;
    }

    // ── Add Points & EXP ─────────────────────────────────────────────────────
    void addPoints(User& u, int pts) {
        u.points += pts;
        u.exp    += pts / 2;
        saveAll();
    }

    // ── Deduct Points (store purchase) ───────────────────────────────────────
    bool spendPoints(User& u, int cost) {
        if (u.points < cost) return false;
        u.points -= cost;
        saveAll();
        return true;
    }

    // ── Get leaderboard (sorted by EXP using C merge sort) ───────────────────
    int getLeaderboard(User* outUsers) {
        // Copy all users to output array
        for (int i = 0; i < userCount; i++)
            outUsers[i] = users[i];
        // Sort using C merge sort DSA
        merge_sort(outUsers, userCount, sizeof(User), compareByExpDesc);
        return userCount;
    }

    // ── Get user by name ─────────────────────────────────────────────────────
    User* findUser(const char* username) {
        for (int i = 0; i < userCount; i++)
            if (strcmp(users[i].username, username) == 0) return &users[i];
        return nullptr;
    }

    int getUserCount() const { return userCount; }

// ─── PRIVATE implementation ───────────────────────────────────────────────────
private:
    User  users[MAX_USERS];
    int   userCount;
    char  dataFile[256];
    User* currentUser;

    // ── Streak update ────────────────────────────────────────────────────────
    void updateStreak(User& u) {
        time_t now = time(nullptr);
        struct tm today_buf, last_buf;
#ifdef _WIN32
        localtime_s(&today_buf, &now);
#else
        localtime_r(&now, &today_buf);
#endif

        int daysDiff = 0;
        if (u.lastLoginDate != 0) {
#ifdef _WIN32
            localtime_s(&last_buf, &u.lastLoginDate);
#else
            localtime_r(&u.lastLoginDate, &last_buf);
#endif
            time_t midnight_now  = now - (today_buf.tm_hour * 3600 + today_buf.tm_min * 60 + today_buf.tm_sec);
            time_t midnight_last = u.lastLoginDate - (last_buf.tm_hour * 3600 + last_buf.tm_min * 60 + last_buf.tm_sec);
            daysDiff = (int)((midnight_now - midnight_last) / 86400);
        }

        if (daysDiff == 1)     u.streak++;
        else if (daysDiff > 1) u.streak = 1;
        // daysDiff == 0: same day, no change

        u.lastLoginDate = now;
    }

    // ── File I/O (binary struct serialization) ───────────────────────────────
    void loadAll() {
        FILE* f = fopen(dataFile, "rb");
        if (!f) return;
        User u;
        while (fread(&u, sizeof(User), 1, f) == 1 && userCount < MAX_USERS)
            users[userCount++] = u;
        fclose(f);
    }

    void saveAll() {
        FILE* f = fopen(dataFile, "wb");
        if (!f) return;
        for (int i = 0; i < userCount; i++)
            fwrite(&users[i], sizeof(User), 1, f);
        fclose(f);
    }

    void createDefaultUsers() {
        User admin;
        strncpy(admin.username, "admin", 31);
        char hash[64];
        simpleHash("10on10", hash, 64);
        strncpy(admin.passwordHash, hash, 63);
        admin.role = ADMIN;
        users[userCount++] = admin;
    }
};
