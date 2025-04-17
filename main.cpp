#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

// Vector2 for 2D coordinates
struct Vector2 {
    float x, y;
    Vector2(float x_ = 0, float y_ = 0) : x(x_), y(y_) {} // y__ yerine y_ kullanıldı
    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
    Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
    // Yeni: Vector2 * float için çarpma operatörü
    Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
};

// Abstract GameObject class
class GameObject {
protected:
    Vector2 position;
    Vector2 velocity;
public:
    GameObject(float x, float y) : position(x, y), velocity(0, 0) {}
    virtual void update(float deltaTime) = 0;
    virtual void render() const = 0;
    Vector2 getPosition() const { return position; }
    Vector2 getVelocity() const { return velocity; }
    void setPosition(const Vector2& pos) { position = pos; }
    void setVelocity(const Vector2& vel) { velocity = vel; }
    virtual ~GameObject() {}
};

// Player class (Mario-like)
class Player : public GameObject {
public:
    Player(float x, float y) : GameObject(x, y) {}
    void update(float deltaTime) override {
        position += velocity * deltaTime;
        // Boundary check
        if (position.x < 0) position.x = 0;
        if (position.x > 9) position.x = 9;
        if (position.y < 0) position.y = 0;
        if (position.y > 9) position.y = 9;
    }
    void render() const override {
        std::cout << "Player: Position=(" << position.x << ", " << position.y 
                  << "), Velocity=(" << velocity.x << ", " << velocity.y << ")" << std::endl;
    }
    void jump() { velocity.y = -5.0f; }
    void moveLeft() { velocity.x = -2.0f; }
    void moveRight() { velocity.x = 2.0f; }
    void stopHorizontal() { velocity.x = 0; }
};

// Platform class
class Platform : public GameObject {
public:
    Platform(float x, float y) : GameObject(x, y) {}
    void update(float deltaTime) override {}
    void render() const override {
        std::cout << "Platform: Position=(" << position.x << ", " << position.y << ")" << std::endl;
    }
};

// Enemy class (Goomba-like)
class Enemy : public GameObject {
public:
    Enemy(float x, float y) : GameObject(x, y) { velocity.x = -1.0f; }
    void update(float deltaTime) override {
        position += velocity * deltaTime;
        if (position.x < 0) { position.x = 0; velocity.x = 1.0f; }
        if (position.x > 9) { position.x = 9; velocity.x = -1.0f; }
    }
    void render() const override {
        std::cout << "Enemy: Position=(" << position.x << ", " << position.y 
                  << "), Velocity=(" << velocity.x << ", " << velocity.y << ")" << std::endl;
    }
};

// Scene class
class Scene {
protected:
    std::vector<GameObject*> objects;
public:
    virtual ~Scene() { for (auto obj : objects) delete obj; }
    virtual void update(float deltaTime) {
        for (auto obj : objects) obj->update(deltaTime);
    }
    virtual void render() const {
        std::cout << "=== Scene State ===\n";
        for (auto obj : objects) obj->render();
        char grid[10][10];
        for (int i = 0; i < 10; ++i)
            for (int j = 0; j < 10; ++j)
                grid[i][j] = '.';
        for (auto obj : objects) {
            int x = static_cast<int>(obj->getPosition().x);
            int y = static_cast<int>(obj->getPosition().y);
            if (x >= 0 && x < 10 && y >= 0 && y < 10) {
                if (dynamic_cast<Player*>(obj)) grid[y][x] = 'P';
                else if (dynamic_cast<Platform*>(obj)) grid[y][x] = '#';
                else if (dynamic_cast<Enemy*>(obj)) grid[y][x] = 'E';
            }
        }
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) std::cout << grid[i][j] << ' ';
            std::cout << std::endl;
        }
    }
    void addObject(GameObject* obj) { objects.push_back(obj); }
    const std::vector<GameObject*>& getObjects() const { return objects; }
};

// Menu scene
class MenuScene : public Scene {
public:
    MenuScene() {
        std::cout << "Menu Scene: Press 's' to start the game.\n";
    }
    void update(float deltaTime) override {
        // movement 20x10 bounds
        if (position.x < 0) position.x = 0;
        if (position.x > 19) position.x = 19; // 9 -> 19
        if (position.y < 0) position.y = 0;
        if (position.y > 9) position.y = 9;
        }
    void render() const override {
        std::cout << "=== Menu Scene ===\n";
        std::cout << "Press 's' to start.\n";
    }
};

// Game scene
class GameScene : public Scene {
public:
    GameScene() {
        addObject(new Player(5, 5));
        addObject(new Platform(4, 7));
        addObject(new Platform(5, 7));
        addObject(new Platform(6, 7));
        addObject(new Enemy(8, 6));
    }
};

// Physics system
class PhysicsSystem {
    float gravity = 9.8f;
public:
    void applyPhysics(GameObject* obj, float deltaTime) {
        if (dynamic_cast<Platform*>(obj)) return;
        obj->setVelocity(Vector2(obj->getVelocity().x, obj->getVelocity().y + gravity * deltaTime));
        obj->setPosition(obj->getPosition() + obj->getVelocity() * deltaTime);
    }
    bool checkCollision(GameObject* a, GameObject* b) {
        Vector2 posA = a->getPosition();
        Vector2 posB = b->getPosition();
        float width = 1.0f, height = 1.0f;
        bool collided = (posA.x < posB.x + width && posA.x + width > posB.x &&
                         posA.y < posB.y + height && posA.y + height > posB.y);
        if (collided) {
            if (dynamic_cast<Player*>(a) && dynamic_cast<Platform*>(b)) {
                std::cout << "Player landed on platform!\n";
                a->setPosition(Vector2(posA.x, posB.y - height));
                a->setVelocity(Vector2(a->getVelocity().x, 0));
            }
            else if (dynamic_cast<Player*>(a) && dynamic_cast<Enemy*>(b)) {
                std::cout << "Player hit enemy! Game Over!\n";
                return true;
            }
        }
        return false;
    }
};

// Scene manager
class SceneManager {
    Scene* currentScene = nullptr;
public:
    ~SceneManager() { delete currentScene; }
    void setScene(Scene* scene) { delete currentScene; currentScene = scene; }
    void update(float deltaTime) { if (currentScene) currentScene->update(deltaTime); }
    void render() const { if (currentScene) currentScene->render(); }
    Scene* getCurrentScene() const { return currentScene; }
};

// Main game loop
int main() {
    SceneManager sceneManager;
    PhysicsSystem physics;
    sceneManager.setScene(new MenuScene());
    bool gameOver = false;

    while (!gameOver) {
        auto start = std::chrono::high_resolution_clock::now();

        std::cout << "Command (w: jump, a: left, d: right, s: start, q: quit): ";
        char input;
        std::cin >> input;
        // Input control         
        if (input != 'w' && input != 'a' && input != 'd' && input != 's' && input != 'q') {
            std::cout << "Invalid command! Please use w, a, d, s or q .\n";
            continue; // Back to beggining of the cycle
}

        if (dynamic_cast<MenuScene*>(sceneManager.getCurrentScene())) {
            if (input == 's') {
                sceneManager.setScene(new GameScene());
            }
            else if (input == 'q') {
                break;
            }
        }
        else if (dynamic_cast<GameScene*>(sceneManager.getCurrentScene())) {
            GameScene* gameScene = dynamic_cast<GameScene*>(sceneManager.getCurrentScene());
            Player* player = nullptr;
            for (auto obj : gameScene->getObjects()) {
                if (dynamic_cast<Player*>(obj)) {
                    player = dynamic_cast<Player*>(obj);
                    break;
                }
            }
            if (player) {
                if (input == 'w') player->jump();
                else if (input == 'a') player->moveLeft();
                else if (input == 'd') player->moveRight();
                else if (input == 'q') break;
                else player->stopHorizontal();
            }
        }

        if (dynamic_cast<GameScene*>(sceneManager.getCurrentScene())) {
            GameScene* gameScene = dynamic_cast<GameScene*>(sceneManager.getCurrentScene());
            for (auto obj : gameScene->getObjects()) {
                physics.applyPhysics(obj, 0.016f);
            }
            auto& objects = gameScene->getObjects();
            for (size_t i = 0; i < objects.size(); ++i) {
                for (size_t j = i + 1; j < objects.size(); ++j) {
                    if (physics.checkCollision(objects[i], objects[j])) {
                        if (dynamic_cast<Player*>(objects[i]) && dynamic_cast<Enemy*>(objects[j])) {
                            gameOver = true;
                            break;
                        }
                    }
                }
            }
        }

        sceneManager.update(0.016f);
        sceneManager.render();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        std::this_thread::sleep_for(std::chrono::milliseconds(16) - duration);
    }

    std::cout << "Game Ended!\n";
    return 0;
}