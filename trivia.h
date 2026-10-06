#pragma once

/*
 * ─── Trivia Manager (C++ class using C Linked List DSA) ──────────────────────
 * Daily chess trivia system with questions stored in a C linked list.
 * Demonstrates integration of C DSA with C++ OOP.
 *
 * Features:
 *   - Questions stored as linked list nodes (C DSA)
 *   - Daily question system (one attempt per day per user)
 *   - Bonus points for correct answers
 *   - Persistent trivia records
 */

#include <cstring>
#include <cstdio>
#include <ctime>

extern "C" {
    #include "../dsa/linkedlist.h"
}

#include "trivia_questions.h"

// Trivia question (stored in linked list)
struct Question {
    char question[256];
    char options[4][64];   // 4 multiple choice options
    int  correctOption;    // 0-3 index
    int  bonusPoints;

    Question() {
        memset(this, 0, sizeof(Question));
        correctOption = 0; bonusPoints = 50;
    }
};

// Daily trivia record
struct TriviaRecord {
    char   username[32];
    time_t date;
    bool   answered;
    bool   correct;

    TriviaRecord() { memset(this, 0, sizeof(TriviaRecord)); }
};

#define MAX_TRIVIA_RECORDS 100

class TriviaManager {
// ─── PUBLIC interface ─────────────────────────────────────────────────────────
public:
    TriviaManager(const char* rf = "data/trivia_records.dat") {
        strncpy(recordsFile, rf, 255);
        recordCount = 0;
        questions = ll_create(sizeof(Question));  // C Linked List DSA
        loadRecords();
        initDefaultQuestions();
    }

    ~TriviaManager() {
        if (questions) ll_destroy(questions);  // Free C linked list
    }

    // ── Add question to linked list ──────────────────────────────────────────
    void addQuestion(const char* q, const char* const opts[4], int correct, int bonus = 50) {
        Question node;
        strncpy(node.question, q, 255);
        for (int i = 0; i < 4; i++) strncpy(node.options[i], opts[i], 63);
        node.correctOption = correct;
        node.bonusPoints = bonus;

        ll_append(questions, &node);  // C Linked List append
    }

    Question* getDailyQuestion(const char* username) {
        int count = ll_size(questions);
        if (!canPlayToday(username) || count == 0) return nullptr;

        time_t now = time(nullptr);
        struct tm t;
        
#ifdef _WIN32
        localtime_s(&t, &now);
#else
        localtime_r(&now, &t);
#endif

        int seed = t.tm_yday + t.tm_year;
        int idx = seed % count;

        return (Question*)ll_get_at(questions, idx);  // C Linked List access
    }

    // ── Submit answer ────────────────────────────────────────────────────────
    int submitAnswer(const char* username, int answer) {
        if (!canPlayToday(username)) return 0;
        Question* q = getDailyQuestion(username);
        if (!q) return 0;

        bool correct = (answer == q->correctOption);
        markPlayed(username, correct);
        return correct ? q->bonusPoints : 0;
    }

    bool canPlayToday(const char* username) {
        time_t now = time(nullptr);
        struct tm today;
        
#ifdef _WIN32
        localtime_s(&today, &now);
#else
        localtime_r(&now, &today);
#endif

        for (int i = 0; i < recordCount; i++) {
            if (strcmp(records[i].username, username) == 0 && records[i].answered) {
                struct tm last;
                
#ifdef _WIN32
        localtime_s(&last, &records[i].date);
#else
        localtime_r(&records[i].date, &last);
#endif

                if (last.tm_yday == today.tm_yday && last.tm_year == today.tm_year)
                    return false;
            }
        }
        return true;
    }

    int questionCount() const { return ll_size(questions); }

// ─── PRIVATE implementation ───────────────────────────────────────────────────
private:
    LinkedList*  questions;    // C Linked List DSA for question storage
    TriviaRecord records[MAX_TRIVIA_RECORDS];
    int          recordCount;
    char         recordsFile[256];

    void markPlayed(const char* username, bool correct) {
        for (int i = 0; i < recordCount; i++) {
            if (strcmp(records[i].username, username) == 0) {
                records[i].date = time(nullptr);
                records[i].answered = true;
                records[i].correct = correct;
                saveRecords();
                return;
            }
        }
        if (recordCount < MAX_TRIVIA_RECORDS) {
            TriviaRecord rec;
            strncpy(rec.username, username, 31);
            rec.date = time(nullptr);
            rec.answered = true;
            rec.correct = correct;
            records[recordCount++] = rec;
            saveRecords();
        }
    }

    void initDefaultQuestions() {
        for (int i = 0; i < 100; i++) {
            const TriviaQuestionData& td = g_triviaBank[i];
            addQuestion(td.question, td.options, td.correct, 50);
        }
    }

    void loadRecords() {
        FILE* f = fopen(recordsFile, "rb");
        if (!f) return;
        TriviaRecord r;
        while (fread(&r, sizeof(TriviaRecord), 1, f) == 1 && recordCount < MAX_TRIVIA_RECORDS)
            records[recordCount++] = r;
        fclose(f);
    }
    void saveRecords() {
        FILE* f = fopen(recordsFile, "wb");
        if (!f) return;
        for (int i = 0; i < recordCount; i++)
            fwrite(&records[i], sizeof(TriviaRecord), 1, f);
        fclose(f);
    }
};
