#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
using namespace std;

// ─── Trivia Question (stored in linked list) ──────────────────────────────────
struct Question {
    char      question[256];
    char      options[4][64];   // 4 multiple choice options
    int       correctOption;    // 0-3 index
    int       bonusPoints;
    Question* next;             // linked list node

    Question() {
        memset(this, 0, sizeof(Question) - sizeof(Question*));
        correctOption=0; bonusPoints=50; next=nullptr;
    }
};

// ─── Daily Trivia Record ──────────────────────────────────────────────────────
struct TriviaRecord {
    char   username[32];
    time_t date;       // date of last trivia attempt
    bool   answered;   // did they answer today?
    bool   correct;

    TriviaRecord() { memset(this, 0, sizeof(TriviaRecord)); }
};

// ─── Trivia Manager (DSA: Linked List) ───────────────────────────────────────
class TriviaManager {
private:
    Question*          head;      // head of linked list
    int                count;
    vector<TriviaRecord> records;
    string             recordsFile;

public:
    TriviaManager(const string& rf="data/trivia_records.dat")
        : head(nullptr), count(0), recordsFile(rf) {
        loadRecords();
        initDefaultQuestions();
    }

    ~TriviaManager() {
        // Free linked list
        Question* curr = head;
        while(curr) { Question* tmp=curr; curr=curr->next; delete tmp; }
    }

    // ── Admin: Add Question ───────────────────────────────────────────────────
    void addQuestion(const string& q, const string opts[4], int correct, int bonus=50) {
        Question* node = new Question();
        strncpy(node->question, q.c_str(), 255);
        for(int i=0;i<4;i++) strncpy(node->options[i], opts[i].c_str(), 63);
        node->correctOption = correct;
        node->bonusPoints   = bonus;
        node->next = nullptr;

        // Append to tail of linked list
        if(!head) { head=node; }
        else {
            Question* curr=head;
            while(curr->next) curr=curr->next;
            curr->next = node;
        }
        count++;
    }

    // ── Get daily question (random) ───────────────────────────────────────────
    Question* getDailyQuestion(const string& username) {
        if(!canPlayToday(username) || count==0) return nullptr;

        // Pick a pseudo-random question based on date + username hash
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        int seed = t->tm_yday + t->tm_year + (int)username[0];
        int idx  = seed % count;

        Question* curr = head;
        for(int i=0;i<idx && curr->next;i++) curr=curr->next;
        return curr;
    }

    // ── Submit answer ─────────────────────────────────────────────────────────
    // Returns bonus points earned (0 if wrong or already played)
    int submitAnswer(const string& username, int answer) {
        if(!canPlayToday(username)) return 0;
        Question* q = getDailyQuestion(username);
        if(!q) return 0;

        bool correct = (answer == q->correctOption);
        markPlayed(username, correct);
        return correct ? q->bonusPoints : 0;
    }

    bool canPlayToday(const string& username) {
        time_t now = time(nullptr);
        struct tm* today = localtime(&now);
        for(auto& r : records) {
            if(string(r.username)==username && r.answered) {
                struct tm* last = localtime(&r.date);
                if(last->tm_yday==today->tm_yday && last->tm_year==today->tm_year)
                    return false; // already played today
            }
        }
        return true;
    }

    // JSON for JS (question without revealing answer)
    string questionToJSON(Question* q) {
        if(!q) return "null";
        ostringstream oss;
        oss << "{";
        oss << "\"question\":\"" << q->question << "\",";
        oss << "\"options\":[";
        for(int i=0;i<4;i++) {
            oss << "\"" << q->options[i] << "\"";
            if(i<3) oss << ",";
        }
        oss << "],";
        oss << "\"bonus\":" << q->bonusPoints;
        oss << "}";
        return oss.str();
    }

    // All questions count
    int size() const { return count; }

private:
    void markPlayed(const string& username, bool correct) {
        for(auto& r : records) {
            if(string(r.username)==username) {
                r.date=time(nullptr); r.answered=true; r.correct=correct;
                saveRecords();
                return;
            }
        }
        TriviaRecord rec;
        strncpy(rec.username, username.c_str(), 31);
        rec.date=time(nullptr); rec.answered=true; rec.correct=correct;
        records.push_back(rec);
        saveRecords();
    }

    void initDefaultQuestions() {
        string opts[4];
        // Chess trivia questions
        opts[0]="16"; opts[1]="20"; opts[2]="32"; opts[3]="8";
        addQuestion("How many squares are on a standard chessboard?",opts,2,50);

        opts[0]="Checkmate"; opts[1]="En passant"; opts[2]="Castling"; opts[3]="Fork";
        addQuestion("What is it called when a pawn moves two squares and can be captured as if it only moved one?",opts,1,60);

        opts[0]="Horse"; opts[1]="Knight"; opts[2]="Cavalry"; opts[3]="Joker";
        addQuestion("Which piece moves in an L-shape?",opts,1,40);

        opts[0]="Queen"; opts[1]="King"; opts[2]="Rook"; opts[3]="Bishop";
        addQuestion("Which piece can move any number of squares in any direction?",opts,0,50);

        opts[0]="Ruy Lopez"; opts[1]="Sicilian Defense"; opts[2]="Kings Indian"; opts[3]="Dutch Defense";
        addQuestion("What is the most popular chess opening at grandmaster level?",opts,1,70);

        opts[0]="Garry Kasparov"; opts[1]="Magnus Carlsen"; opts[2]="Bobby Fischer"; opts[3]="Anatoly Karpov";
        addQuestion("Who was the first undisputed World Chess Champion from the USA?",opts,2,80);

        opts[0]="En passant"; opts[1]="Promotion"; opts[2]="Castling"; opts[3]="Check";
        addQuestion("What is it called when the king and rook swap positions?",opts,2,50);

        opts[0]="3"; opts[1]="2"; opts[2]="1"; opts[3]="4";
        addQuestion("How many points is a bishop worth in standard chess valuation?",opts,0,60);
    }

    void loadRecords() {
        ifstream f(recordsFile, ios::binary);
        if(!f.is_open()) return;
        TriviaRecord r;
        while(f.read(reinterpret_cast<char*>(&r), sizeof(TriviaRecord)))
            records.push_back(r);
    }
    void saveRecords() {
        ofstream f(recordsFile, ios::binary|ios::trunc);
        for(auto& r : records) f.write(reinterpret_cast<const char*>(&r), sizeof(TriviaRecord));
    }
};
