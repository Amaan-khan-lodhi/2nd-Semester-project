#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <fstream>
#include <algorithm>

using namespace std;

// ================= GAME CONSTANTS =================
namespace GameConstants {
    constexpr int MAX_HEALTH = 100;
    constexpr int MAX_FOOD = 100;
    constexpr int MAX_DISTANCE = 100;
    constexpr int MAX_INVENTORY = 10;
    constexpr int MAX_STICKS = 3;
    
    // Travel costs
    constexpr int TRAVEL_HEALTH_COST = 10;
    constexpr int TRAVEL_FOOD_COST = 5;
    constexpr int TRAVEL_DISTANCE_GAIN = 15;
    
    // Hunt gains
    constexpr int HUNT_HEALTH_COST = 20;
    constexpr int HUNT_FOOD_GAIN = 20;
    
    // Rest gains
    constexpr int REST_HEALTH_GAIN = 20;
    
    // Food/Money
    constexpr int FOOD_EAT_COST = 10;
    constexpr int HEALTH_EAT_GAIN = 15;
    constexpr int FOOD_SELL_AMOUNT = 10;
    constexpr int FOOD_SELL_PRICE = 15;
    
    // Tent
    constexpr int TENT_COST = 30;
    
    // Storm damage
    constexpr int STORM_DAMAGE_DAY_WITH_TENT = 5;
    constexpr int STORM_DAMAGE_DAY_NO_TENT = 30;
    constexpr int STORM_DAMAGE_NIGHT_NO_TENT = 40;
    constexpr int STORM_FOOD_LOSS = 10;
    constexpr int STORM_DISTANCE_LOSS = 5;
    
    // Wolf attack damage
    constexpr int WOLF_DAMAGE_DAY = 25;
    constexpr int WOLF_DAMAGE_NIGHT = 40;
    constexpr int WOLF_ESCAPE_DAMAGE_DAY = 10;
    constexpr int WOLF_ESCAPE_DAMAGE_NIGHT = 20;
    constexpr int WOLF_ESCAPE_FOOD_LOSS = 10;
    constexpr int WOLF_STICK_DAMAGE = 5;
    
    // Probabilities (0-100)
    constexpr int STICK_FIND_CHANCE = 20;
    constexpr int STORM_BASE_CHANCE = 15;
    constexpr int WOLF_BASE_CHANCE = 20;
    constexpr int WOLF_DAY_CHANCE_OFFSET = 20;
    constexpr int WOLF_NIGHT_CHANCE_OFFSET = 0;
    constexpr int STORM_DAY_CHANCE_OFFSET = 15;
}

// ================= RANDOM NUMBER GENERATOR =================
class RandomGenerator {
private:
    static mt19937 generator;
    static uniform_int_distribution<int> distribution;
    
public:
    static int getRandomInt(int min, int max) {
        uniform_int_distribution<int> dist(min, max);
        return dist(generator);
    }
    
    static bool checkProbability(int percentage) {
        return getRandomInt(0, 99) < percentage;
    }
};

mt19937 RandomGenerator::generator(random_device{}());
uniform_int_distribution<int> RandomGenerator::distribution(0, 99);

// ================= PLAYER =================
class Player {
private:
    int health;
    int food;
    int distance;
    int money;
    bool hasTent;
    vector<string> inventory;
    int sticksFound;
    static int totalPlayers;
    
    void clampValues() {
        health = max(0, min(health, GameConstants::MAX_HEALTH));
        food = max(0, min(food, GameConstants::MAX_FOOD));
        distance = max(0, distance);
    }
    
public:
    Player() : health(GameConstants::MAX_HEALTH), 
               food(GameConstants::MAX_FOOD), 
               distance(0), 
               money(0), 
               hasTent(false), 
               sticksFound(0) {
        totalPlayers++;
    }
    
    ~Player() { totalPlayers--; }
    
    // ================= STAT MANAGEMENT =================
    void updateStats(int healthDelta, int foodDelta, int distanceDelta) {
        health += healthDelta;
        food += foodDelta;
        distance += distanceDelta;
        clampValues();
    }
    
    void addMoney(int amount) {
        money = max(0, money + amount);
    }
    
    void buildTent() {
        hasTent = true;
    }
    
    // ================= GETTERS (const) =================
    int getHealth() const { return health; }
    int getFood() const { return food; }
    int getDistance() const { return distance; }
    int getMoney() const { return money; }
    int getStickCount() const { return sticksFound; }
    bool hasTentBuilt() const { return hasTent; }
    size_t getInventorySize() const { return inventory.size(); }
    
    // ================= INVENTORY MANAGEMENT =================
    bool addItem(const string& item) {
        if (inventory.size() >= GameConstants::MAX_INVENTORY) {
            cout << "❌ Inventory full!\n";
            return false;
        }
        inventory.push_back(item);
        cout << "✓ " << item << " added to inventory.\n";
        return true;
    }
    
    void showInventory() const {
        cout << "\n========= INVENTORY =========\n";
        if (inventory.empty()) {
            cout << "Empty\n";
            return;
        }
        for (size_t i = 0; i < inventory.size(); ++i) {
            cout << i + 1 << ". " << inventory[i] << endl;
        }
        cout << "=============================\n";
    }
    
    bool useItem(int index) {
        if (index < 0 || (size_t)index >= inventory.size()) {
            cout << "❌ Invalid item!\n";
            return false;
        }
        inventory.erase(inventory.begin() + index);
        return true;
    }
    
    // ================= WEAPON SYSTEM =================
    bool hasWeapon() const {
        return find(inventory.begin(), inventory.end(), "Stick") != inventory.end();
    }
    
    bool giveStick() {
        if (sticksFound >= GameConstants::MAX_STICKS) {
            return false;
        }
        addItem("Stick");
        sticksFound++;
        return true;
    }
    
    bool useStick() {
        auto it = find(inventory.begin(), inventory.end(), "Stick");
        if (it != inventory.end()) {
            cout << "⚔️ You used a Stick to defend yourself!\n";
            inventory.erase(it);
            return true;
        }
        return false;
    }
    
    // ================= FOOD CONSUMPTION =================
    bool eatFood() {
        if (food < GameConstants::FOOD_EAT_COST) {
            cout << "❌ Not enough food!\n";
            return false;
        }
        food -= GameConstants::FOOD_EAT_COST;
        health += GameConstants::HEALTH_EAT_GAIN;
        if (health > GameConstants::MAX_HEALTH) {
            health = GameConstants::MAX_HEALTH;
        }
        cout << "🍖 You ate food and gained health!\n";
        return true;
    }
    
    // ================= DISPLAY =================
    void displayStats() const {
        cout << "\n╔════════════════════════════╗\n";
        cout << "║    PLAYER STATUS           ║\n";
        cout << "╠════════════════════════════╣\n";
        cout << "║ ❤️  Health:  " << health << "/100" << string(max(0, (int)(15 - to_string(health).length())), ' ') << "║\n";
        cout << "║ 🍖 Food:    " << food << "/100" << string(max(0, (int)(15 - to_string(food).length())), ' ') << "║\n";
        cout << "║ 📍 Distance: " << distance << "/100" << string(max(0, (int)(14 - to_string(distance).length())), ' ') << "║\n";
        cout << "║ 💰 Money:   " << money << string(max(0, (int)(20 - to_string(money).length())), ' ') << "║\n";
        cout << "║ ⛺ Tent:    " << (hasTent ? "Yes" : "No") << string(22, ' ') << "║\n";
        cout << "╚════════════════════════════╝\n";
    }
    
    // ================= FILE I/O =================
    void saveToFile(const string& filename) const {
        ofstream fout(filename);
        if (!fout) {
            cout << "❌ Cannot open save file!\n";
            return;
        }
        fout << health << " " << food << " " << distance << " " 
             << money << " " << hasTent << " " << sticksFound << " " 
             << inventory.size() << "\n";
        for (const auto& item : inventory) {
            fout << item << "\n";
        }
        fout.close();
        cout << "✓ Game saved successfully!\n";
    }
    
    void loadFromFile(const string& filename) {
        ifstream fin(filename);
        if (!fin) {
            cout << "⚠️  No save file found. Starting new game.\n";
            return;
        }
        
        size_t itemCount;
        fin >> health >> food >> distance >> money >> hasTent >> sticksFound >> itemCount;
        fin.ignore();
        
        inventory.clear();
        for (size_t i = 0; i < itemCount; ++i) {
            string item;
            getline(fin, item);
            inventory.push_back(item);
        }
        fin.close();
        cout << "✓ Game loaded successfully!\n";
        clampValues();
    }
    
    static int getTotalPlayers() { return totalPlayers; }
};

int Player::totalPlayers = 0;

// ================= ACTION SYSTEM (STRATEGY PATTERN) =================
class Action {
public:
    virtual void execute(Player& player) = 0;
    virtual ~Action() = default;
};

class TravelAction : public Action {
public:
    void execute(Player& player) override {
        player.updateStats(-GameConstants::TRAVEL_HEALTH_COST, 
                          -GameConstants::TRAVEL_FOOD_COST, 
                          GameConstants::TRAVEL_DISTANCE_GAIN);
        cout << "🚶 You traveled 15 km.\n";
    }
};

class HuntAction : public Action {
public:
    void execute(Player& player) override {
        player.updateStats(-GameConstants::HUNT_HEALTH_COST, 
                          GameConstants::HUNT_FOOD_GAIN, 0);
        cout << "🏹 You hunted and found food!\n";
    }
};

class RestAction : public Action {
public:
    void execute(Player& player) override {
        player.updateStats(GameConstants::REST_HEALTH_GAIN, 0, 0);
        cout << "😴 You rested and recovered health.\n";
    }
};

class SellFoodAction : public Action {
public:
    void execute(Player& player) override {
        if (player.getFood() >= GameConstants::FOOD_SELL_AMOUNT) {
            player.updateStats(0, -GameConstants::FOOD_SELL_AMOUNT, 0);
            player.addMoney(GameConstants::FOOD_SELL_PRICE);
            cout << "💵 You sold food for $15.\n";
        } else {
            cout << "❌ Not enough food to sell!\n";
        }
    }
};

class BuildTentAction : public Action {
public:
    void execute(Player& player) override {
        if (player.getMoney() >= GameConstants::TENT_COST) {
            player.addMoney(-GameConstants::TENT_COST);
            player.buildTent();
            cout << "⛺ Tent built! You're protected from storms.\n";
        } else {
            cout << "❌ Not enough money! (Need $30)\n";
        }
    }
};

class EatFoodAction : public Action {
public:
    void execute(Player& player) override {
        player.eatFood();
    }
};

// ================= ACTION FACTORY PATTERN =================
class ActionFactory {
public:
    static unique_ptr<Action> createAction(int choice) {
        switch (choice) {
            case 1: return make_unique<TravelAction>();
            case 2: return make_unique<RestAction>();
            case 3: return make_unique<HuntAction>();
            case 4: return make_unique<SellFoodAction>();
            case 5: return make_unique<BuildTentAction>();
            case 6: return make_unique<EatFoodAction>();
            default: return nullptr;
        }
    }
};

// ================= INPUT VALIDATION =================
class InputValidator {
public:
    static int getValidInput(int minChoice, int maxChoice) {
        int choice;
        while (true) {
            cout << "Enter choice [" << minChoice << "-" << maxChoice << "]: ";
            if (cin >> choice && choice >= minChoice && choice <= maxChoice) {
                cin.ignore(10000, '\n');
                return choice;
            }
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "❌ Invalid input! Please try again.\n";
        }
    }
};

// ================= GAME =================
class Game {
private:
    Player player;
    bool isRunning;
    bool isDay;
    int turnCount;
    
    // Event handling
    void handleFindStick() {
        if (RandomGenerator::checkProbability(GameConstants::STICK_FIND_CHANCE)) {
            if (player.getStickCount() < GameConstants::MAX_STICKS) {
                player.giveStick();
                cout << "🎉 You found a Stick!\n";
            }
        }
    }
    
    void handleStorm() {
        int chance = isDay ? 
            GameConstants::STORM_BASE_CHANCE + GameConstants::STORM_DAY_CHANCE_OFFSET :
            GameConstants::STORM_BASE_CHANCE;
            
        if (RandomGenerator::checkProbability(chance)) {
            cout << "\n⛈️  A STORM APPROACHES!\n";
            
            if (!player.hasTentBuilt()) {
                int damage = isDay ? GameConstants::STORM_DAMAGE_DAY_NO_TENT : 
                                    GameConstants::STORM_DAMAGE_NIGHT_NO_TENT;
                player.updateStats(-damage, -GameConstants::STORM_FOOD_LOSS, 
                                 -GameConstants::STORM_DISTANCE_LOSS);
                cout << "❌ No tent! You took " << damage << " damage!\n";
            } else {
                player.updateStats(-GameConstants::STORM_DAMAGE_DAY_WITH_TENT, 0, 0);
                cout << "✓ Your tent protected you!\n";
            }
        }
    }
    
    void handleWolfAttack() {
        int chance = isDay ? 
            GameConstants::WOLF_BASE_CHANCE + GameConstants::WOLF_DAY_CHANCE_OFFSET :
            GameConstants::WOLF_BASE_CHANCE + GameConstants::WOLF_NIGHT_CHANCE_OFFSET;
            
        if (RandomGenerator::checkProbability(chance)) {
            cout << "\n🐺 A WOLF APPEARED!\n";
            
            if (player.hasWeapon()) {
                player.updateStats(-GameConstants::WOLF_STICK_DAMAGE, 0, 0);
                player.useStick();
                return;
            }
            
            cout << "1. Fight\n2. Run\n";
            int choice = InputValidator::getValidInput(1, 2);
            
            if (choice == 1) {
                int damage = isDay ? GameConstants::WOLF_DAMAGE_DAY : 
                                    GameConstants::WOLF_DAMAGE_NIGHT;
                player.updateStats(-damage, 0, 0);
                cout << "⚔️  You fought the wolf and took " << damage << " damage!\n";
            } else {
                int damage = isDay ? GameConstants::WOLF_ESCAPE_DAMAGE_DAY : 
                                   GameConstants::WOLF_ESCAPE_DAMAGE_NIGHT;
                player.updateStats(-damage, -GameConstants::WOLF_ESCAPE_FOOD_LOSS, 0);
                cout << "🏃 You ran away, losing health and food!\n";
            }
        }
    }
    
    void checkGameStatus() {
        if (player.getHealth() <= 0) {
            cout << "\n💀 GAME OVER! You died!\n";
            isRunning = false;
        } else if (player.getDistance() >= GameConstants::MAX_DISTANCE) {
            cout << "\n🎉 YOU WIN! You reached the end of the trail!\n";
            isRunning = false;
        }
    }
    
public:
    Game() : isRunning(true), isDay(true), turnCount(0) {}
    
    void updateTime() {
        turnCount++;
        if (turnCount % 2 == 0) {
            isDay = !isDay;
        }
        cout << (isDay ? "\n☀️  DAY TIME\n" : "\n🌙 NIGHT TIME\n");
    }
    
    void displayMenu() const {
        cout << "\n╔═══════════════════════════════════╗\n";
        cout << "║          CHOOSE ACTION            ║\n";
        cout << "╠═══════════════════════════════════╣\n";
        cout << "║ 1. Travel      2. Rest            ║\n";
        cout << "║ 3. Hunt        4. Sell Food       ║\n";
        cout << "║ 5. Build Tent  6. Eat Food        ║\n";
        cout << "║ 7. Inventory   8. Save Game       ║\n";
        cout << "║ 9. Load Game   10. Exit           ║\n";
        cout << "╚═══════════════════════════════════╝\n";
    }
    
    void startGame() {
        cout << "\n╔════════════════════════════════════╗\n";
        cout << "║   SURVIVAL TRAIL SIMULATOR START   ║\n";
        cout << "║   Reach 10.0 km   to escape!       ║\n";
        cout << "╚════════════════════════════════════╝\n";
        
        while (isRunning) {
            player.displayStats();
            displayMenu();
            
            int choice = InputValidator::getValidInput(1, 10);
            
            // Handle menu actions
            if (choice == 7) {
                player.showInventory();
                continue;
            } else if (choice == 8) {
                player.saveToFile("save.txt");
                continue;
            } else if (choice == 9) {
                player.loadFromFile("save.txt");
                continue;
            } else if (choice == 10) {
                cout << "👋 Thanks for playing!\n";
                break;
            }
            
            // Execute game action
            auto action = ActionFactory::createAction(choice);
            if (action) {
                action->execute(player);
            }
            
            // Update game state
            updateTime();
            handleFindStick();
            handleStorm();
            handleWolfAttack();
            checkGameStatus();
        }
    }
};

int main() {
    Game game;
    game.startGame();
    return 0;
}
