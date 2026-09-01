#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
using namespace std;

// ─── Store Item ───────────────────────────────────────────────────────────────
enum ItemType { BOARD_SKIN, PIECE_SKIN };

struct StoreItem {
    char     id[32];
    char     name[64];
    char     description[128];
    ItemType type;
    int      cost;           // in game points
    int      minRankExp;     // minimum EXP required to purchase (0 = any rank)
    bool     isAvailable;

    StoreItem() { memset(this, 0, sizeof(StoreItem)); isAvailable=true; }
};

// ─── User Inventory ───────────────────────────────────────────────────────────
struct InventoryEntry {
    char username[32];
    char itemId[32];
    bool isEquipped;

    InventoryEntry() { memset(this, 0, sizeof(InventoryEntry)); }
};

// ─── Store Manager ────────────────────────────────────────────────────────────
class Store {
private:
    vector<StoreItem>      items;
    vector<InventoryEntry> inventory;
    string                 itemsFile;
    string                 inventoryFile;

public:
    Store(const string& iFile="data/store.dat", const string& invFile="data/inventory.dat")
        : itemsFile(iFile), inventoryFile(invFile) {
        loadItems();
        loadInventory();
        if(items.empty()) initDefaultItems();
    }

    // ── Browse store ─────────────────────────────────────────────────────────
    vector<StoreItem> getAvailableItems(int userExp) {
        vector<StoreItem> result;
        for(auto& item : items)
            if(item.isAvailable && item.minRankExp <= userExp)
                result.push_back(item);
        return result;
    }

    // ── Purchase item ────────────────────────────────────────────────────────
    // Returns: 0=success, 1=insufficient points, 2=rank too low, 3=already owned, 4=not found
    int purchase(const string& username, const string& itemId, int& userPoints, int userExp) {
        StoreItem* item = findItem(itemId);
        if(!item) return 4;
        if(item->minRankExp > userExp) return 2;
        if(owns(username, itemId)) return 3;
        if(userPoints < item->cost) return 1;

        userPoints -= item->cost;

        InventoryEntry entry;
        strncpy(entry.username, username.c_str(), 31);
        strncpy(entry.itemId, itemId.c_str(), 31);
        entry.isEquipped = false;
        inventory.push_back(entry);
        saveInventory();
        return 0;
    }

    // ── Equip item ───────────────────────────────────────────────────────────
    bool equip(const string& username, const string& itemId) {
        StoreItem* item = findItem(itemId);
        if(!item || !owns(username, itemId)) return false;

        // Unequip any other item of same type
        for(auto& e : inventory) {
            if(string(e.username)==username) {
                StoreItem* si = findItem(string(e.itemId));
                if(si && si->type == item->type) e.isEquipped = false;
            }
        }
        for(auto& e : inventory) {
            if(string(e.username)==username && string(e.itemId)==itemId)
                e.isEquipped = true;
        }
        saveInventory();
        return true;
    }

    bool owns(const string& username, const string& itemId) {
        for(auto& e : inventory)
            if(string(e.username)==username && string(e.itemId)==itemId) return true;
        return false;
    }

    // Get equipped item of a type
    string getEquipped(const string& username, ItemType type) {
        for(auto& e : inventory) {
            if(string(e.username)==username && e.isEquipped) {
                StoreItem* si = findItem(string(e.itemId));
                if(si && si->type==type) return string(si->id);
            }
        }
        return "default";
    }

    // Admin: add store item
    bool addItem(const StoreItem& item) {
        items.push_back(item);
        saveItems();
        return true;
    }

    // JSON for JS frontend
    string itemsToJSON(int userExp, const string& username) {
        auto available = getAvailableItems(userExp);
        ostringstream oss;
        oss << "[";
        for(int i=0;i<(int)available.size();i++) {
            auto& it = available[i];
            oss << "{";
            oss << "\"id\":\"" << it.id << "\",";
            oss << "\"name\":\"" << it.name << "\",";
            oss << "\"desc\":\"" << it.description << "\",";
            oss << "\"type\":\"" << (it.type==BOARD_SKIN?"board":"piece") << "\",";
            oss << "\"cost\":" << it.cost << ",";
            oss << "\"owned\":" << (owns(username,string(it.id))?"true":"false");
            oss << "}";
            if(i<(int)available.size()-1) oss << ",";
        }
        oss << "]";
        return oss.str();
    }

private:
    StoreItem* findItem(const string& id) {
        for(auto& item : items) if(string(item.id)==id) return &item;
        return nullptr;
    }

    void initDefaultItems() {
        auto makeItem = [&](const char* id, const char* name, const char* desc,
                            ItemType t, int cost, int minExp) {
            StoreItem si;
            strncpy(si.id,id,31); strncpy(si.name,name,63);
            strncpy(si.description,desc,127);
            si.type=t; si.cost=cost; si.minRankExp=minExp; si.isAvailable=true;
            items.push_back(si);
        };
        // Board skins
        makeItem("classic","Classic Wood","Traditional wooden board",BOARD_SKIN,0,0);
        makeItem("neon","Neon Glow","Futuristic neon board",BOARD_SKIN,200,500);
        makeItem("marble","Marble Luxury","Elegant marble board",BOARD_SKIN,500,1500);
        makeItem("galaxy","Galaxy Board","Space-themed board",BOARD_SKIN,900,3000);
        makeItem("dragon","Dragon Board","Legendary dragon board",BOARD_SKIN,1500,6000);
        // Piece skins
        makeItem("default_pieces","Classic Pieces","Traditional chess pieces",PIECE_SKIN,0,0);
        makeItem("fire_pieces","Fire Pieces","Pieces wreathed in flame",PIECE_SKIN,300,500);
        makeItem("ice_pieces","Ice Crystal Pieces","Frozen crystal pieces",PIECE_SKIN,600,1500);
        makeItem("gold_pieces","Gold Pieces","Gleaming gold pieces",PIECE_SKIN,1000,3000);
        makeItem("shadow_pieces","Shadow Pieces","Dark shadowy pieces",PIECE_SKIN,1800,6000);
        saveItems();
    }

    void loadItems() {
        ifstream f(itemsFile, ios::binary);
        if(!f.is_open()) return;
        StoreItem si;
        while(f.read(reinterpret_cast<char*>(&si), sizeof(StoreItem)))
            items.push_back(si);
    }
    void saveItems() {
        ofstream f(itemsFile, ios::binary|ios::trunc);
        for(auto& si : items) f.write(reinterpret_cast<const char*>(&si), sizeof(StoreItem));
    }
    void loadInventory() {
        ifstream f(inventoryFile, ios::binary);
        if(!f.is_open()) return;
        InventoryEntry e;
        while(f.read(reinterpret_cast<char*>(&e), sizeof(InventoryEntry)))
            inventory.push_back(e);
    }
    void saveInventory() {
        ofstream f(inventoryFile, ios::binary|ios::trunc);
        for(auto& e : inventory) f.write(reinterpret_cast<const char*>(&e), sizeof(InventoryEntry));
    }
};
