
#include <raylib.h>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <fstream>
#include <algorithm>
 
using namespace std;
 
// ================= GAME CONSTANTS (FROM CONSOLE VERSION - 100% UNCHANGED) =================
namespace GameConstants {
    constexpr int MAX_HEALTH = 100;
    constexpr int MAX_FOOD = 100;
    constexpr int MAX_DISTANCE = 100;
    constexpr int MAX_INVENTORY = 10;
    constexpr int MAX_STICKS = 3;
    
    // Travel costs
    constexpr int TRAVEL_HEALTH_COST = 10;
    constexpr int TRAVEL_FOOD_COST = 10;
    constexpr int TRAVEL_DISTANCE_GAIN = 10;
    
    // Hunt gains
    constexpr int HUNT_HEALTH_COST = 15;
    constexpr int HUNT_FOOD_GAIN = 20;
    
    // Rest gains
    constexpr int REST_HEALTH_GAIN = 15;
    
    // Food/Money
    constexpr int FOOD_EAT_COST = 10;
    constexpr int HEALTH_EAT_GAIN = 15;
    constexpr int FOOD_SELL_AMOUNT = 10;
    constexpr int FOOD_SELL_PRICE = 12;
    
    // Tent
    constexpr int TENT_COST = 35;
    
    // Storm damage
    constexpr int STORM_DAMAGE_DAY_WITH_TENT = 5;
    constexpr int STORM_DAMAGE_DAY_NO_TENT = 20;
    constexpr int STORM_DAMAGE_NIGHT_NO_TENT = 35;
    constexpr int STORM_FOOD_LOSS = 10;
    constexpr int STORM_DISTANCE_LOSS = 5;
    
    // Wolf attack damage
    constexpr int WOLF_DAMAGE_DAY = 20;
    constexpr int WOLF_DAMAGE_NIGHT = 35;
    constexpr int WOLF_ESCAPE_DAMAGE_DAY = 10;
    constexpr int WOLF_ESCAPE_DAMAGE_NIGHT = 20;
    constexpr int WOLF_ESCAPE_FOOD_LOSS = 10;
    constexpr int WOLF_STICK_DAMAGE = 5;
    
    // Probabilities (0-100)
    constexpr int STICK_FIND_CHANCE = 25;
    constexpr int STORM_BASE_CHANCE = 15;
    constexpr int WOLF_BASE_CHANCE = 15;
    constexpr int WOLF_DAY_CHANCE_OFFSET = 15;
    constexpr int WOLF_NIGHT_CHANCE_OFFSET = 5;
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
    
    int getHealth() const { return health; }
    int getFood() const { return food; }
    int getDistance() const { return distance; }
    int getMoney() const { return money; }
    int getStickCount() const { return sticksFound; }
    bool hasTentBuilt() const { return hasTent; }
    size_t getInventorySize() const { return inventory.size(); }
    vector<string> getInventory() const { return inventory; }
    
    bool addItem(const string& item) {
        if (inventory.size() >= GameConstants::MAX_INVENTORY) {
            return false;
        }
        inventory.push_back(item);
        return true;
    }
    
    bool useItem(int index) {
        if (index < 0 || (size_t)index >= inventory.size()) {
            return false;
        }
        inventory.erase(inventory.begin() + index);
        return true;
    }
    
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
            inventory.erase(it);
            return true;
        }
        return false;
    }
    
    bool eatFood() {
        if (food < GameConstants::FOOD_EAT_COST) {
            return false;
        }
        food -= GameConstants::FOOD_EAT_COST;
        health += GameConstants::HEALTH_EAT_GAIN;
        if (health > GameConstants::MAX_HEALTH) {
            health = GameConstants::MAX_HEALTH;
        }
        return true;
    }
    
    void saveToFile(const string& filename) {
        ofstream fout(filename);
        if (!fout) {
            return;
        }
        fout << health << " " << food << " " << distance << " " 
             << money << " " << hasTent << " " << sticksFound << " " 
             << inventory.size() << "\n";
        for (const auto& item : inventory) {
            fout << item << "\n";
        }
        fout.close();
    }
    
    bool loadFromFile(const string& filename) {
        ifstream fin(filename);
        if (!fin) {
            return false;
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
        clampValues();
        return true;
    }
    
    static int getTotalPlayers() { return totalPlayers; }
};
 
int Player::totalPlayers = 0;
 
// ================= GUI BUTTON CLASS =================
struct Button {
    Rectangle rect;
    string text;
    Color color;
    Color hoverColor;
    bool isHovered;
    
    Button(float x, float y, float w, float h, const string& txt, Color col, Color hcol)
        : rect{x, y, w, h}, text(txt), color(col), hoverColor(hcol), isHovered(false) {}
    
    void update() {
        Vector2 mouse = GetMousePosition();
        isHovered = CheckCollisionPointRec(mouse, rect);
    }
    
    bool isClicked() {
        return isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }
    
    void draw(Font& font) {
        Color currentColor = isHovered ? hoverColor : color;
        
        DrawRectangleRec(rect, currentColor);
        Color borderColor = isHovered ? Color{255, 255, 255, 255} : Color{100, 100, 100, 255};
        DrawRectangleLinesEx(rect, 2, borderColor);
        
        Vector2 textSize = MeasureTextEx(font, text.c_str(), 14, 1);
        float textX = rect.x + (rect.width - textSize.x) / 2;
        float textY = rect.y + (rect.height - textSize.y) / 2;
        
        DrawTextEx(font, text.c_str(), {textX + 1, textY + 1}, 14, 2, Color{0, 0, 0, 100});
        DrawTextEx(font, text.c_str(), {textX, textY}, 14, 2, WHITE);
    }
};

// ================= VISUAL EFFECTS CLASS (GUI ONLY - FIXED) =================
class VisualEffects {
private:
    float stormOverlayAlpha = 0;
    float lightningTimer = 0;
    int rainIntensity = 0;
    float rainTimer = 0;  // ADD THIS
    
    float wolfFlashTimer = 0;
    Vector2 shakeOffset = {0, 0};
    
public:
    void triggerStorm(float duration) {
        stormOverlayAlpha = 150;
        rainIntensity = 40;
        rainTimer = 0;  // ADD THIS
    }
    
    void triggerWolfAttack(float duration) {
        wolfFlashTimer = duration;
    }
    
    void update(float deltaTime) {
        stormOverlayAlpha *= 0.97f;
        rainIntensity = (int)(stormOverlayAlpha / 150.0f * 40);
        rainTimer += deltaTime;  // ADD THIS
        
        wolfFlashTimer -= deltaTime;
        
        if (wolfFlashTimer > 0) {
            shakeOffset.x = (float)(rand() % 10 - 5) * 0.3f;
            shakeOffset.y = (float)(rand() % 10 - 5) * 0.3f;
        } else {
            shakeOffset = {0, 0};
        }
    }
    
    void drawStormEffects() {
        if (stormOverlayAlpha <= 1) return;
        
        // Dark overlay
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                     Color{50, 50, 80, (unsigned char)stormOverlayAlpha});
        
        // Rain animation 
        for (int i = 0; i < rainIntensity; i++) {
            int rainX = ((int)(rainTimer * 100) + i * 50) % (GetScreenWidth() + 100);
            int rainY = ((int)(rainTimer * 200) + i * 30) % (GetScreenHeight() + 100);
            
            DrawLine(rainX, rainY, rainX + 15, rainY + 30,
                    Color{200, 200, 220, 200});
        }
        
        // Lightning flashes
        if (RandomGenerator::checkProbability(8)) {
            lightningTimer = 0.15f;
        }
        
        if (lightningTimer > 0) {
            int alpha = (int)(lightningTimer * 255 * 4);
            alpha = min(255, alpha);
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                         Color{255, 255, 255, (unsigned char)alpha});
            lightningTimer -= GetFrameTime();
        }
    }
    
    void drawWolfEffects() {
        if (wolfFlashTimer <= 0) return;
        
        float intensity = wolfFlashTimer * 2;
        intensity = min(1.0f, intensity);
        
        // Red flash overlay
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                     Color{255, 0, 0, (unsigned char)(intensity * 200)});
        
        // Wolf eyes
        int eyeX = GetScreenWidth() / 2;
        int eyeY = 150;
        
        // Left eye
        DrawCircle(eyeX - 80, eyeY, 30, Color{255, 50, 50, 200});
        DrawCircle(eyeX - 80, eyeY, 15, Color{100, 0, 0, 255});
        
        // Right eye
        DrawCircle(eyeX + 80, eyeY, 30, Color{255, 50, 50, 200});
        DrawCircle(eyeX + 80, eyeY, 15, Color{100, 0, 0, 255});
        
        // Red danger border
        DrawRectangleLinesEx({0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                            10, Color{255, 0, 0, (unsigned char)(intensity * 150)});
    }
    
    Vector2 getShakeOffset() const { return shakeOffset; }
};
 
// ================= UI RENDERER (GUI ONLY) =================
class UIRenderer {
private:
    Font font;
    
public:
    UIRenderer() {
        font = LoadFont("assets/fonts/PixelOperator.ttf");
    }
    
    void drawStatsPanel(const Player& player, int screenHeight) {
        // left middle
        float panelX = 10;
        float panelY = 90;
        
        DrawRectangle(panelX, panelY, 360, 400, Color{30, 60, 30, 255});
        DrawRectangleLinesEx({panelX, panelY, 360, 400}, 2, Color{100, 200, 100, 255});
        
        DrawTextEx(font,"PLAYER STATUS", (Vector2){panelX + 20, panelY + 15},24,2, Color{144, 238, 144, 255});
        
        float y = panelY + 55;
        float barWidth = 240;
        float barHeight = 22;
        float spacing = 55;
        
        // Health
        DrawTextEx(font,"HEALTH", (Vector2){panelX + 20, y}, 16 , 2 , Color{255, 100, 100, 255});
        DrawRectangleLinesEx({panelX + 20, y + 22, barWidth, barHeight}, 1, Color{255, 100, 100, 255});
        float healthPercent = player.getHealth() / 100.0f;
        DrawRectangle(panelX + 22, y + 24, (barWidth - 4) * healthPercent, barHeight - 4, 
                     Color{255, 100, 100, 255});
        DrawTextEx(font,(to_string(player.getHealth()) + "/100").c_str(),(Vector2) {panelX + 270, y + 24}, 14,2, WHITE);
        
        y += spacing;
        
        // Food
        DrawTextEx(font,"FOOD",(Vector2){ panelX + 20, y}, 16,2, Color{255, 180, 0, 255});
        DrawRectangleLinesEx({panelX + 20, y + 22, barWidth, barHeight}, 1, Color{255, 180, 0, 255});
        float foodPercent = player.getFood() / 100.0f;
        DrawRectangle(panelX + 22, y + 24, (barWidth - 4) * foodPercent, barHeight - 4, 
                     Color{255, 180, 0, 255});
        DrawTextEx(font,(to_string(player.getFood()) + "/100").c_str(),(Vector2){ panelX + 270, y + 24}, 14,2, WHITE);
        
        y += spacing;
        
        // Distance
        DrawTextEx(font,"DISTANCE", (Vector2){panelX + 20, y}, 16,2, Color{100, 200, 100, 255});
        DrawRectangleLinesEx({panelX + 20, y + 22, barWidth, barHeight}, 1, Color{100, 200, 100, 255});
        float distancePercent = player.getDistance() / 100.0f;
        DrawRectangle(panelX + 22, y + 24, (barWidth - 4) * distancePercent, barHeight - 4, 
                     Color{100, 200, 100, 255});
        DrawTextEx(font,(to_string(player.getDistance()) + "/100").c_str(),(Vector2){ panelX + 270, y + 24}, 14,2, WHITE);
        
        y += spacing;
        
        // Money
        DrawTextEx(font,"MONEY:",(Vector2){ panelX + 20, y}, 14,2, Color{184, 134, 11, 255});
        DrawTextEx(font,("Rs" + to_string(player.getMoney())).c_str(),(Vector2) {panelX + 150, y}, 16,2, 
                Color{218, 165, 32, 255});
        
        y += 35;
        
        // Tent
        DrawTextEx(font,"TENT:",(Vector2){ panelX + 20, y}, 14,2, Color{100, 200, 100, 255});
        DrawTextEx(font,player.hasTentBuilt() ? "[BUILT]" : "[NO TENT]",(Vector2){ panelX + 150, y}, 16,2, 
                player.hasTentBuilt() ? Color{100, 255, 100, 255} : Color{255, 100, 100, 255});
        
        y += 35;
        
        // Items
        DrawTextEx(font,"ITEMS:",(Vector2){ panelX + 20, y}, 14,2, Color{150, 150, 255, 255});
        DrawTextEx(font,(to_string(player.getInventorySize()) + "/10").c_str(),(Vector2){ panelX + 150, y}, 16,2, 
                Color{200, 200, 255, 255});
        
        // Sticks count
        y += 35;
        DrawTextEx(font,"STICKS:",(Vector2) {panelX + 20, y}, 14, 2,Color{200, 200, 100, 255});
        DrawTextEx(font,(to_string(player.getStickCount()) + "/3").c_str(),(Vector2){ panelX + 150, y}, 16,2,
                Color{255, 255, 100, 255});
    }
    void drawEventMessage(const string& message, float& duration) {
        // Middle Right
        float panelX = 10;
        float panelY = 500;
        float panelWidth = 350;
        float panelHeight = 150;
        
        if (duration > 0) {
            DrawRectangle(panelX, panelY, panelWidth, panelHeight, Color{50, 100, 50, 220});
            DrawRectangleLinesEx({panelX, panelY, panelWidth, panelHeight}, 2, Color{100, 200, 100, 255});
            
            DrawTextEx(font,"EVENT:",(Vector2) {panelX + 10, panelY + 10}, 22,2, Color{144, 238, 144, 255});
            
            // Word wrap for message text
            int charCount = 0;
            int line = 0;
            string currentLine = "";
            
            for (char c : message) {
                currentLine += c;
                charCount++;
                
                if (charCount > 18 || c == '\n') {
                    DrawTextEx(font,currentLine.c_str(),(Vector2){ panelX + 10, panelY + 35 + (line * 20)}, 18,2, Color{200, 255, 200, 255});
                    currentLine = "";
                    charCount = 0;
                    line++;
                    
                    if (line > 4) break;
                }
            }
            
            if (!currentLine.empty()) {
                DrawTextEx(font,currentLine.c_str(),(Vector2) {panelX + 10, panelY + 35 + (line * 20)}, 14,2, Color{200, 255, 200, 255});
            }
            
            duration -= GetFrameTime();
        }
    }
    
    void drawGameOver(const string& message, const Player& player) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 200});
        
        Vector2 screenSize = {(float)GetScreenWidth(), (float)GetScreenHeight()};
        float boxWidth = 600;
        float boxHeight = 300;
        float boxX = (screenSize.x - boxWidth) / 2;
        float boxY = (screenSize.y - boxHeight) / 2;
        
        DrawRectangle(boxX, boxY, boxWidth, boxHeight, Color{40, 40, 40, 255});
        DrawRectangleLinesEx({boxX, boxY, boxWidth, boxHeight}, 4, YELLOW);
        
        DrawTextEx(font,message.c_str(),(Vector2){ boxX + 50, boxY + 50}, 28,2, YELLOW);
        
        string finalStats = "Final Distance: " + to_string(player.getDistance()) + "/100";
        DrawTextEx(font,finalStats.c_str(),(Vector2){ boxX + 50, boxY + 150}, 20,2, WHITE);
        
        DrawTextEx(font,"Close window to exit",(Vector2){ boxX + 100, boxY + 230}, 18,2, WHITE);
    }
};
 
// ================= ACTION SYSTEM =================
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
    }
};
 
class HuntAction : public Action {
public:
    void execute(Player& player) override {
        player.updateStats(-GameConstants::HUNT_HEALTH_COST, 
                          GameConstants::HUNT_FOOD_GAIN, 0);
    }
};
 
class RestAction : public Action {
public:
    void execute(Player& player) override {
        player.updateStats(GameConstants::REST_HEALTH_GAIN, 0, 0);
    }
};
 
class SellFoodAction : public Action {
public:
    void execute(Player& player) override {
        if (player.getFood() >= GameConstants::FOOD_SELL_AMOUNT) {
            player.updateStats(0, -GameConstants::FOOD_SELL_AMOUNT, 0);
            player.addMoney(GameConstants::FOOD_SELL_PRICE);
        }
    }
};
 
class BuildTentAction : public Action {
public:
    void execute(Player& player) override {
        if (player.getMoney() >= GameConstants::TENT_COST) {
            player.addMoney(-GameConstants::TENT_COST);
            player.buildTent();
        }
    }
};
 
class EatFoodAction : public Action {
public:
    void execute(Player& player) override {
        player.eatFood();
    }
};
 
// ================= ACTION FACTORY =================
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
 
// ================= GAME ENGINE =================
class Game {
private:
    Player player;
    UIRenderer uiRenderer;
    VisualEffects effects; 
    bool isRunning;
    bool isDay;
    int turnCount;
    string eventMessage;
    float eventTimer;
    bool gameOver;
    bool playerWon;
    string gameOverMessage;


    
    void handleFindStick() {
        if (RandomGenerator::checkProbability(GameConstants::STICK_FIND_CHANCE)) {
            if (player.getStickCount() < GameConstants::MAX_STICKS) {
                player.giveStick();
                eventMessage = "You found a Stick!";
                eventTimer = 2.0f;
            }
        }
    }
    
    void handleStorm() {
        int chance = isDay ? 
            GameConstants::STORM_BASE_CHANCE + GameConstants::STORM_DAY_CHANCE_OFFSET :
            GameConstants::STORM_BASE_CHANCE;
            
        if (RandomGenerator::checkProbability(chance)) {
            if (!player.hasTentBuilt()) {
                int damage = isDay ? GameConstants::STORM_DAMAGE_DAY_NO_TENT : 
                                    GameConstants::STORM_DAMAGE_NIGHT_NO_TENT;
                player.updateStats(-damage, -GameConstants::STORM_FOOD_LOSS, 
                                 -GameConstants::STORM_DISTANCE_LOSS);
                                 effects.triggerStorm(2.5f);
                eventMessage = "STORM! You took " + to_string(damage) + " damage!";
            } else {
                player.updateStats(-GameConstants::STORM_DAMAGE_DAY_WITH_TENT, 0, 0);
                effects.triggerStorm(2.5f);
                eventMessage = "Storm hit but your tent protected you!";
            }
            eventTimer = 2.5f;
            
        }
    }
    
    void handleWolfAttack() {
        int chance = isDay ? 
            GameConstants::WOLF_BASE_CHANCE + GameConstants::WOLF_DAY_CHANCE_OFFSET :
            GameConstants::WOLF_BASE_CHANCE + GameConstants::WOLF_NIGHT_CHANCE_OFFSET;
            
        if (RandomGenerator::checkProbability(chance)) {
            if (player.hasWeapon()) {
                player.updateStats(-GameConstants::WOLF_STICK_DAMAGE, 0, 0);
                player.useStick();
                effects.triggerWolfAttack(0.3f);
                eventMessage = "Wolf attacked! You fought it off with your stick!";
            } else {
                int damage = isDay ? GameConstants::WOLF_DAMAGE_DAY : 
                                    GameConstants::WOLF_DAMAGE_NIGHT;
                player.updateStats(-damage, 0, 0);
                effects.triggerWolfAttack(0.5f);
                eventMessage = "WOLF! You took " + to_string(damage) + " damage!";
            }
            eventTimer = 2.5f;
        }
    }
    
    void checkGameStatus() {
        if (player.getHealth() <= 0) {
            gameOver = true;
            playerWon = false;
            gameOverMessage = "GAME OVER - You Died!";
        } else if (player.getDistance() >= GameConstants::MAX_DISTANCE) {
            gameOver = true;
            playerWon = true;
            gameOverMessage = "YOU WIN! Reached the end!";
        }
    }
    
public:
    Game() : isRunning(true), isDay(true), turnCount(0), eventTimer(0), 
             gameOver(false), playerWon(false) {}


    
    void updateEffects(float deltaTime) { effects.update(deltaTime); }
    VisualEffects& getEffects() { return effects; }
    
    
    void updateTime() {
        turnCount++;
        if (turnCount % 2 == 0) {
            isDay = !isDay;
        }
    }
    
    void travel() {
        player.updateStats(-GameConstants::TRAVEL_HEALTH_COST, 
                          -GameConstants::TRAVEL_FOOD_COST, 
                          GameConstants::TRAVEL_DISTANCE_GAIN);
        eventMessage = "You traveled 1.5 km";
        eventTimer = 1.5f;
        processNextTurn();
    }
    
    void hunt() {
        player.updateStats(-GameConstants::HUNT_HEALTH_COST, 
                          GameConstants::HUNT_FOOD_GAIN, 0);
        eventMessage = "You hunted and found food!";
        eventTimer = 1.5f;
        processNextTurn();
    }
    
    void rest() {
        player.updateStats(GameConstants::REST_HEALTH_GAIN, 0, 0);
        eventMessage = "You rested and recovered health";
        eventTimer = 1.5f;
        processNextTurn();
    }
    
    void sellFood() {
        if (player.getFood() >= GameConstants::FOOD_SELL_AMOUNT) {
            player.updateStats(0, -GameConstants::FOOD_SELL_AMOUNT, 0);
            player.addMoney(GameConstants::FOOD_SELL_PRICE);
            eventMessage = "Sold food for 150 Rs";
            eventTimer = 1.5f;
            processNextTurn();
        } else {
            eventMessage = "Not enough food to sell!";
            eventTimer = 1.5f;
        }
    }
    
    void buildTent() {
        if (player.getMoney() >= GameConstants::TENT_COST) {
            player.addMoney(-GameConstants::TENT_COST);
            player.buildTent();
            eventMessage = "Tent built! You're protected from storms";
            eventTimer = 1.5f;
            processNextTurn();
        } else {
            eventMessage = "Not enough money! Need 300 Rs";
            eventTimer = 1.5f;
        }
    }
    
    void eatFood() {
        if (player.eatFood()) {
            eventMessage = "You ate food and gained health!";
            eventTimer = 1.5f;
            processNextTurn();
        } else {
            eventMessage = "Not enough food to eat!";
            eventTimer = 1.5f;
        }
    }
    
    void saveGame() {
        player.saveToFile("save.txt");
        eventMessage = "Game saved successfully!";
        eventTimer = 2.0f;
    }
    
    void loadGame() {
        if (player.loadFromFile("save.txt")) {
            eventMessage = "Game loaded successfully!";
        } else {
            eventMessage = "No save file found!";
        }
        eventTimer = 2.0f;
    }
    
    void processNextTurn() {
        updateTime();
        handleFindStick();
        handleStorm();
        handleWolfAttack();
        checkGameStatus();
    }
    
    const Player& getPlayer() const { return player; }
    bool isGameOver() const { return gameOver; }
    bool hasPlayerWon() const { return playerWon; }
    string getGameOverMessage() const { return gameOverMessage; }
    bool isDayTime() const { return isDay; }
    string getEventMessage() const { return eventMessage; }
    float getEventTimerDuration() const { return eventTimer; }
};


 void DrawForestTree(int x, int groundY, int size)
{
    // ============= TRUNK =============
    int trunkWidth = size / 5;    
    int trunkHeight = size ;
    int trunkX = x - trunkWidth / 2;
    int trunkY = groundY - trunkHeight;

    DrawRectangle(trunkX, trunkY, trunkWidth, trunkHeight, Color{101, 67, 33, 255});

    // ============= FOLIAGE - TRIANGULAR =============
    Color leafDark   = Color{35, 120, 35, 255};
    Color leafMain   = Color{60, 150, 60, 255};
    Color leafLight  = Color{90, 180, 90, 255};

    float foliageX = x;
    float foliageY = trunkY - size / 4;

    // Triangle 1 (bottom - widest)
    DrawTriangle(
        {foliageX - size / 2, foliageY + size / 3},      // Left
        {foliageX + size / 2, foliageY + size / 3},      // Right
        {foliageX, foliageY - size / 3},                 // Top
        leafDark
    );

    // Triangle 2 (middle)
    DrawTriangle(
        {foliageX - size / 2.5f, foliageY - size / 6},
        {foliageX + size / 2.5f, foliageY - size / 6},
        {foliageX, foliageY - size / 2},
        leafMain
    );

    // Triangle 3 (top - narrow)
    DrawTriangle(
        {foliageX - size / 3.5f, foliageY - size / 2.5f},
        {foliageX + size / 3.5f, foliageY - size / 2.5f},
        {foliageX, foliageY - size},
        leafLight
    );

    // ============= BUSHES =============
    int bushRadius = size / 5;
    int bushY = groundY - bushRadius;

    // Left bushes
    DrawCircle(x - size / 2 - 10, bushY, bushRadius, leafDark);
    DrawCircle(x - size / 2 + 10, bushY - 5, bushRadius - 3, leafMain);

    // Center bushes
    DrawCircle(x - 15, bushY + 2, bushRadius + 2, leafMain);
    DrawCircle(x, bushY, bushRadius + 3, leafMain);
    DrawCircle(x + 15, bushY + 2, bushRadius + 2, leafMain);

    // Right bushes
    DrawCircle(x + size / 2 - 10, bushY - 5, bushRadius - 3, leafMain);
    DrawCircle(x + size / 2 + 10, bushY, bushRadius, leafDark);
}
    
// ================= MAIN GUI APPLICATION =================
int main() {
    const int screenWidth = 1450;
    const int screenHeight = 800;
    
    InitWindow(screenWidth, screenHeight, "Survival Trail Simulator - Forest Adventure");
    SetTargetFPS(60);
    
    Game game;
    UIRenderer renderer;
    
    // ===== BUTTON LAYOUT - HORIZONTAL BOTTOM ROW =====
    vector<Button> actionButtons;
    int buttonWidth = 130;
    int buttonHeight = 55;
    int startX = 20;
    int startY = 730;  // Bottom of screen
    int spacing = 10;
    
    // All 10 buttons in horizontal row at bottom
    actionButtons.push_back(Button(startX + 0*(buttonWidth + spacing), startY, buttonWidth, buttonHeight, 
                                   "TRAVEL", Color{34, 177, 76, 255}, Color{52, 211, 94, 255}));
    actionButtons.push_back(Button(startX + 1*(buttonWidth + spacing), startY, buttonWidth, buttonHeight,
                                   "REST", Color{102, 153, 102, 255}, Color{153, 204, 102, 255}));
    actionButtons.push_back(Button(startX + 2*(buttonWidth + spacing), startY, buttonWidth, buttonHeight,
                                   "HUNT", Color{139, 69, 19, 255}, Color{184, 92, 24, 255}));
    actionButtons.push_back(Button(startX + 3*(buttonWidth + spacing), startY, buttonWidth, buttonHeight,
                                   "SELL", Color{184, 134, 11, 255}, Color{218, 165, 32, 255}));
    actionButtons.push_back(Button(startX + 4*(buttonWidth + spacing), startY, buttonWidth, buttonHeight,
                                   "TENT", Color{70, 140, 70, 255}, Color{100, 180, 100, 255}));
    actionButtons.push_back(Button(startX + 5*(buttonWidth + spacing), startY, buttonWidth, buttonHeight, 
                                   "EAT", Color{200, 100, 50, 255}, Color{240, 150, 80, 255}));
    actionButtons.push_back(Button(startX + 6*(buttonWidth + spacing), startY, buttonWidth, buttonHeight,
                                   "INV", Color{100, 150, 200, 255}, Color{150, 200, 255, 255}));
    actionButtons.push_back(Button(startX + 7*(buttonWidth + spacing), startY, buttonWidth, buttonHeight,
                                   "SAVE", Color{100, 100, 150, 255}, Color{150, 150, 200, 255}));
    actionButtons.push_back(Button(startX + 8*(buttonWidth + spacing), startY, buttonWidth, buttonHeight,
                                   "LOAD", Color{100, 100, 150, 255}, Color{150, 150, 200, 255}));
    actionButtons.push_back(Button(startX + 9*(buttonWidth + spacing), startY, buttonWidth, buttonHeight,
                                   "EXIT", Color{150, 50, 50, 255}, Color{200, 100, 100, 255}));

    
    
    // Main game loop
    while (!WindowShouldClose() && !game.isGameOver()) {
        // Update buttons
        for (auto& btn : actionButtons) {
            btn.update();
        }
        
        // Handle button clicks
        if (actionButtons[0].isClicked()) game.travel();
        if (actionButtons[1].isClicked()) game.rest();
        if (actionButtons[2].isClicked()) game.hunt();
        if (actionButtons[3].isClicked()) game.sellFood();
        if (actionButtons[4].isClicked()) game.buildTent();
        if (actionButtons[5].isClicked()) game.eatFood();
        if (actionButtons[6].isClicked()) {} 
        if (actionButtons[7].isClicked()) game.saveGame();
        if (actionButtons[8].isClicked()) game.loadGame();
        if (actionButtons[9].isClicked()) break;


         game.updateEffects(GetFrameTime());
        
        // ===== DRAW PHASE =====
        BeginDrawing();
        Font font = LoadFont("assets/fonts/PixelOperator.ttf");
        // Background based on day/night
        if (game.isDayTime()) {
            ClearBackground(Color{135, 206, 235, 255});
            
            // Sunlight
            DrawCircle(1350, 50, 80, Color{255, 255, 150, 80});
            DrawCircle(1350, 50, 60, Color{255, 255, 200, 100});
            
            
                DrawForestTree(350, 700, 100);   // Tree 1
                DrawForestTree(600, 700, 100);   // Tree 2
                DrawForestTree(900, 700, 100);   // Tree 3
                DrawForestTree(1200, 700, 100);  // Tree 4
             game.getEffects().drawStormEffects();
             game.getEffects().drawWolfEffects();
            
            
            // Ground
            DrawRectangle(50, 700, 2000 , 100, Color{34, 139, 34, 255});
            
        } else {
            // Night time
            ClearBackground(Color{10, 10, 20, 255});
            
            // Moon
            DrawCircle(1300, 80, 70, Color{255, 255, 150, 255});
            DrawCircle(1300, 80, 70, Color{200, 200, 100, 150});
            
            // Stars
            for (int i = 0; i < 15; i++) {
                int starX = (250 + i * 75) % (screenWidth - 200) + 220;
                int starY = 50 + (i * 20) % 200;
                int starSize = (i % 3 == 0) ? 2 : 1;
                Color starColor = (i % 2 == 0) ? Color{255, 255, 255, 255} : Color{200, 200, 255, 255};
                DrawCircle(starX, starY, starSize, starColor);
            }
            
                DrawForestTree(350, 700, 100);   // Tree 1
                DrawForestTree(600, 700, 100);   // Tree 2
                DrawForestTree(900, 700, 100);   // Tree 3
                DrawForestTree(1200, 700, 100);  // Tree 4
            game.getEffects().drawStormEffects();
             game.getEffects().drawWolfEffects();

             
            
            // Dark ground
            DrawRectangle(50, 700, 2000 , 100, Color{20, 30, 20, 255});
        }
        
        // ===== TITLE BAR =====
        DrawRectangle(15, 10, screenWidth - 240, 60, Color{34, 102, 34, 255});
        DrawRectangleLinesEx({15, 10, screenWidth - 240, 60}, 2, Color{144, 238, 144, 255});
        DrawTextEx(font, "SURVIVAL TRAIL SIMULATOR - Reach 100 to Escape",(Vector2){ 70, 25}, 18,2, Color{144, 238, 144, 255});
        
        // Day/Night indicator
        float dayNightX = screenWidth - 400;
        if (game.isDayTime()) {
            DrawRectangle(dayNightX, 15, 170, 50, Color{255, 200, 0, 100});
            DrawTextEx(font,"DAY", (Vector2) {dayNightX + 20, 30}, 20,2, Color{255, 255, 0, 255});
        } else {
            DrawRectangle(dayNightX, 15, 170, 50, Color{50, 50, 150, 100});
            DrawTextEx(font,"NIGHT",(Vector2){ dayNightX + 35, 30}, 20,2, Color{150, 200, 255, 255});
        }
        
        // ===== DRAW UI PANELS =====
        renderer.drawStatsPanel(game.getPlayer(),780);
        
        float msgTimer = game.getEventTimerDuration();
        renderer.drawEventMessage(game.getEventMessage(), msgTimer);
        
        // ===== DRAW BOTTOM BUTTONS =====
        Font defaultFont = GetFontDefault();
        DrawRectangle(10, 720, screenWidth - 20, 80, Color{40, 70, 40, 150});
        DrawRectangleLinesEx({10, 720, screenWidth - 20, 80}, 2, Color{100, 200, 100, 255});
        
        for (auto btn : actionButtons) {
            btn.draw(defaultFont);
        }
        
        EndDrawing();
    }
    
    // Game over screen
    if (game.isGameOver()) {
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(Color{20, 20, 30, 255});
            
            renderer.drawGameOver(game.getGameOverMessage(), game.getPlayer());
            
            EndDrawing();
        }
    }
    
    CloseWindow();
    return 0;
}