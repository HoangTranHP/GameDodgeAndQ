#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int PLAYER_SIZE = 60;
const int ENEMY_SIZE = 36;
const int PROJECTILE_SIZE = 10;
const float PLAYER_SPEED = 100.0f;
const float ENEMY_SPEED = 95.0f;
const float PROJECTILE_SPEED = 1000.0f;
const float Q_COOLDOWN = 1.0f;
const int SCORE_PER_KILL = 10;
const float COMBO_TIMEOUT = 3.0f;
const float LEVEL_DURATION = 30.0f;
const float SPAWN_RATE_BASE = 120.0f;
const float SPAWN_RATE_DECREASE = 10.0f;
const float ENEMY_SPEED_INCREASE = 10.0f;

enum EnemyType { BASIC, FAST, CHASER };
enum WeaponType { SINGLE, SHOTGUN };

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;

struct GameObject {
    SDL_FRect rect;
    float vx, vy;
    bool active;
    EnemyType type;
    int health;
};

struct Marker {
    SDL_FPoint position;
    float timer;
};

struct PowerUp {
    SDL_FRect rect;
    enum Type { SPEED_BOOST, DAMAGE_BOOST, SHIELD } type;
    float timer;
    bool active;
};

std::vector<GameObject> enemies;
std::vector<GameObject> projectiles;
std::vector<Marker> markers;
std::vector<PowerUp> powerUps;
GameObject player;
int score = 0;
int combo = 0;
float comboTime = 0;
float qCooldown = 0;
bool qReady = true;
SDL_FPoint targetPos = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
float gameTime = 0.0f;
int level = 1;
float spawnRate = SPAWN_RATE_BASE;
int upgradePoints = 0;
int projectileDamage = 1;
float playerSpeed = PLAYER_SPEED;
float qCooldownMax = Q_COOLDOWN;
float speedBoostTimer = 0.0f;
float damageBoostTimer = 0.0f;
float shieldTimer = 0.0f;
bool speedBoostActive = false;
bool damageBoostActive = false;
bool shieldActive = false;
int playerHealth = 100;
const int MAX_HEALTH = 100;
WeaponType currentWeapon = SINGLE;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cout << "TTF init failed: " << TTF_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Dodge Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;

    font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
        return false;
    }

    SDL_ShowCursor(SDL_ENABLE);
    return true;
}

void spawnEnemy() {
    GameObject enemy;
    enemy.type = static_cast<EnemyType>(rand() % 3);
    enemy.rect.w = ENEMY_SIZE;
    enemy.rect.h = ENEMY_SIZE;

    switch(enemy.type) {
        case BASIC: enemy.health = 1; break;
        case FAST: enemy.health = 1; break;
        case CHASER: enemy.health = 2; break;
    }

    if (rand() % 2 == 0) {
        enemy.rect.x = (rand() % 2 == 0) ? -ENEMY_SIZE : SCREEN_WIDTH;
        enemy.rect.y = rand() % SCREEN_HEIGHT;
    } else {
        enemy.rect.x = rand() % SCREEN_WIDTH;
        enemy.rect.y = (rand() % 2 == 0) ? -ENEMY_SIZE : SCREEN_HEIGHT;
    }

    enemy.active = true;
    enemies.push_back(enemy);
}

void spawnPowerUp(float x, float y) {
    if (rand() % 10 != 0) return;
    PowerUp pu;
    pu.rect = {x, y, 20, 20};
    pu.type = static_cast<PowerUp::Type>(rand() % 3);
    pu.timer = 10.0f;
    pu.active = true;
    powerUps.push_back(pu);
}

void shootProjectile() {
    if (!qReady) return;

    GameObject proj;
    proj.rect.w = PROJECTILE_SIZE;
    proj.rect.h = PROJECTILE_SIZE;
    proj.rect.x = player.rect.x + player.rect.w / 2 - PROJECTILE_SIZE / 2;
    proj.rect.y = player.rect.y + player.rect.h / 2 - PROJECTILE_SIZE / 2;

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    float dx = mouseX - (proj.rect.x + proj.rect.w / 2);
    float dy = mouseY - (proj.rect.y + proj.rect.h / 2);
    float length = std::sqrt(dx * dx + dy * dy);

    if (length > 0) {
        proj.vx = (dx / length) * PROJECTILE_SPEED;
        proj.vy = (dy / length) * PROJECTILE_SPEED;
    } else {
        proj.vx = 0;
        proj.vy = PROJECTILE_SPEED; // Mặc định bắn lên nếu không có hướng
    }

    proj.active = true;
    projectiles.push_back(proj);
    qReady = false;
    qCooldown = qCooldownMax;
}

void shootShotgun() {
    if (!qReady) return;

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    float baseX = player.rect.x + player.rect.w / 2 - PROJECTILE_SIZE / 2;
    float baseY = player.rect.y + player.rect.h / 2 - PROJECTILE_SIZE / 2;
    float dx = mouseX - (baseX + PROJECTILE_SIZE / 2);
    float dy = mouseY - (baseY + PROJECTILE_SIZE / 2);
    float length = std::sqrt(dx * dx + dy * dy);

    for (int i = -2; i <= 2; i++) {
        GameObject proj;
        proj.rect.w = PROJECTILE_SIZE;
        proj.rect.h = PROJECTILE_SIZE;
        proj.rect.x = baseX;
        proj.rect.y = baseY;

        if (length > 0) {
            float angle = atan2(dy, dx) + i * 0.2f; // Góc lệch cho shotgun
            proj.vx = cos(angle) * PROJECTILE_SPEED;
            proj.vy = sin(angle) * PROJECTILE_SPEED;
        } else {
            proj.vx = 0;
            proj.vy = PROJECTILE_SPEED; // Mặc định bắn lên
        }

        proj.active = true;
        projectiles.push_back(proj);
    }
    qReady = false;
    qCooldown = qCooldownMax * 1.5f;
}

void renderText(const std::string& text, int x, int y, SDL_Color color = {255, 255, 255}) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

bool checkCollision(const SDL_FRect& a, const SDL_FRect& b) {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}

void applyUpgrade(int choice) {
    if (upgradePoints < 10) return;
    switch (choice) {
        case 1:
            playerSpeed *= 1.5f;
            std::cout << "Upgraded Speed: " << playerSpeed << std::endl;
            break;
        case 2:
            qCooldownMax *= 0.5f;
            std::cout << "Upgraded Q Cooldown: " << qCooldownMax << std::endl;
            break;
        case 3:
            projectileDamage += 3;
            std::cout << "Upgraded Damage: " << projectileDamage << std::endl;
            break;
        case 4:
            currentWeapon = SHOTGUN;
            std::cout << "Upgraded to Shotgun!" << std::endl;
            break;
    }
    upgradePoints -= 10;
}

void applyPowerUp(PowerUp& pu) {
    switch (pu.type) {
        case PowerUp::SPEED_BOOST:
            speedBoostTimer = std::max(speedBoostTimer, pu.timer);
            if (!speedBoostActive) {
                playerSpeed = PLAYER_SPEED * 1.5f;
                speedBoostActive = true;
            }
            break;
        case PowerUp::DAMAGE_BOOST:
            damageBoostTimer = std::max(damageBoostTimer, pu.timer);
            if (!damageBoostActive) {
                projectileDamage = 2;
                damageBoostActive = true;
            }
            break;
        case PowerUp::SHIELD:
            shieldTimer = std::max(shieldTimer, pu.timer);
            if (!shieldActive) {
                playerHealth = std::min(MAX_HEALTH, playerHealth + 50);
                shieldActive = true;
            }
            break;
    }
    pu.active = false;
}

void update(float deltaTime) {
    gameTime += deltaTime;

    if (gameTime > LEVEL_DURATION * level) {
        level++;
        spawnRate = std::max(SPAWN_RATE_BASE - (level - 1) * SPAWN_RATE_DECREASE, 20.0f);
        std::cout << "Level up! Current level: " << level << std::endl;
    }

    float dx = targetPos.x - player.rect.x;
    float dy = targetPos.y - player.rect.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance > 5.0f) {
        player.rect.x += (dx / distance) * playerSpeed * deltaTime;
        player.rect.y += (dy / distance) * playerSpeed * deltaTime;
    }

    for (auto& marker : markers) {
        marker.timer -= deltaTime;
    }
    markers.erase(std::remove_if(markers.begin(), markers.end(),
        [](const Marker& m) { return m.timer <= 0; }), markers.end());

    float currentEnemySpeed = ENEMY_SPEED + (level - 1) * ENEMY_SPEED_INCREASE;
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;

        float dx = player.rect.x - enemy.rect.x;
        float dy = player.rect.y - enemy.rect.y;
        float length = std::sqrt(dx * dx + dy * dy);

        switch(enemy.type) {
            case BASIC:
                enemy.rect.x += (dx / length) * currentEnemySpeed * deltaTime;
                enemy.rect.y += (dy / length) * currentEnemySpeed * deltaTime;
                break;
            case FAST:
                enemy.rect.x += (dx / length) * (currentEnemySpeed * 1.5f) * deltaTime;
                enemy.rect.y += (dy / length) * (currentEnemySpeed * 1.5f) * deltaTime;
                break;
            case CHASER:
                enemy.rect.x += (dx / length) * (currentEnemySpeed * 0.8f) * deltaTime;
                enemy.rect.y += (dy / length) * (currentEnemySpeed * 0.8f) * deltaTime;
                break;
        }
    }

    // Cập nhật đường đạn logic
    for (auto& proj : projectiles) {
        if (!proj.active) continue;

        // Di chuyển đạn dựa trên vận tốc đã chuẩn hóa
        proj.rect.x += proj.vx * deltaTime;
        proj.rect.y += proj.vy * deltaTime;

        // Kiểm tra biên màn hình (bao gồm cả kích thước đạn)
        if (proj.rect.x + proj.rect.w < 0 || proj.rect.x > SCREEN_WIDTH ||
            proj.rect.y + proj.rect.h < 0 || proj.rect.y > SCREEN_HEIGHT) {
            proj.active = false;
        }
    }

    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;

            if (checkCollision(proj.rect, enemy.rect)) {
                proj.active = false;
                enemy.health -= projectileDamage;

                qReady = true;
                qCooldown = 0;

                if (enemy.health <= 0) {
                    enemy.active = false;
                    spawnPowerUp(enemy.rect.x, enemy.rect.y);
                    comboTime = COMBO_TIMEOUT;
                    combo++;
                    score += SCORE_PER_KILL * combo;
                    upgradePoints += 1;
                }
            }
        }
    }

    if (!shieldActive) {
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            if (checkCollision(player.rect, enemy.rect)) {
                playerHealth -= 20;
                enemy.active = false;
                if (playerHealth <= 0) {
                    std::cout << "Game Over! Score: " << score << std::endl;
                    SDL_Quit();
                    exit(0);
                }
            }
        }
    }

    for (auto& pu : powerUps) {
        if (pu.active && checkCollision(player.rect, pu.rect)) {
            applyPowerUp(pu);
        }
    }
    powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
        [](const PowerUp& p) { return !p.active; }), powerUps.end());

    if (speedBoostTimer > 0) {
        speedBoostTimer -= deltaTime;
        if (speedBoostTimer <= 0) {
            speedBoostActive = false;
            playerSpeed = PLAYER_SPEED;
        }
    }
    if (damageBoostTimer > 0) {
        damageBoostTimer -= deltaTime;
        if (damageBoostTimer <= 0) {
            damageBoostActive = false;
            projectileDamage = 1;
        }
    }
    if (shieldTimer > 0) {
        shieldTimer -= deltaTime;
        if (shieldTimer <= 0) {
            shieldActive = false;
        }
    }

    if (comboTime > 0) comboTime -= deltaTime;
    else combo = 0;

    if (!qReady) {
        qCooldown -= deltaTime;
        if (qCooldown <= 0) {
            qReady = true;
            qCooldown = 0;
        }
    }

    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const GameObject& e) { return !e.active; }), enemies.end());
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](const GameObject& p) { return !p.active; }), projectiles.end());
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_FRect playerRect = player.rect;
    SDL_RenderFillRectF(renderer, &playerRect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
    for (const auto& marker : markers) {
        SDL_FRect markRect = {marker.position.x - 5, marker.position.y - 5, 10, 10};
        SDL_RenderFillRectF(renderer, &markRect);
    }

    for (const auto& enemy : enemies) {
        if (!enemy.active) continue;
        SDL_Color color;
        switch(enemy.type) {
            case BASIC: color = {255, 0, 0, 255}; break;
            case FAST: color = {255, 165, 0, 255}; break;
            case CHASER: color = {255, 0, 255, 255}; break;
        }
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_FRect enemyRect = enemy.rect;
        SDL_RenderFillRectF(renderer, &enemyRect);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    for (const auto& proj : projectiles) {
        if (!proj.active) continue;
        SDL_FRect bulletRect = proj.rect;
        SDL_RenderFillRectF(renderer, &bulletRect);
    }

    for (const auto& pu : powerUps) {
        if (!pu.active) continue;
        SDL_Color color;
        switch(pu.type) {
            case PowerUp::SPEED_BOOST: color = {0, 255, 255, 255}; break;
            case PowerUp::DAMAGE_BOOST: color = {255, 0, 0, 255}; break;
            case PowerUp::SHIELD: color = {255, 255, 255, 255}; break;
        }
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_FRect puRect = pu.rect;
        SDL_RenderFillRectF(renderer, &puRect);
    }

    renderText("Score: " + std::to_string(score), 10, 10);
    renderText("Level: " + std::to_string(level), 10, 40);
    renderText("Upgrade Points: " + std::to_string(upgradePoints), 10, 70);
    renderText("Health: " + std::to_string(playerHealth) + "/" + std::to_string(MAX_HEALTH), 10, 100);
    if (upgradePoints >= 10) {
        renderText("1: Speed (+50%) | 2: Cooldown (-50%) | 3: Damage (+3) | 4: Shotgun", 10, 130);
    }

    renderText("Speed Boost: " + std::string(speedBoostActive ? "Active" : "Inactive") + " (" +
               std::to_string(static_cast<int>(speedBoostTimer)) + "s)", 10, 160, {0, 255, 255});
    renderText("Damage Boost: " + std::string(damageBoostActive ? "Active" : "Inactive") + " (" +
               std::to_string(static_cast<int>(damageBoostTimer)) + "s)", 10, 190, {255, 0, 0});
    renderText("Shield: " + std::string(shieldActive ? "Active" : "Inactive") + " (" +
               std::to_string(static_cast<int>(shieldTimer)) + "s)", 10, 220, {255, 255, 255});
    renderText("Weapon: " + std::string(currentWeapon == SINGLE ? "Single" : "Shotgun"), 10, 250);

    const int barWidth = 200;
    const int barHeight = 15;
    float cooldownRatio = qCooldown / qCooldownMax;

    SDL_Rect cooldownBg = {10, SCREEN_HEIGHT - 30, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &cooldownBg);

    SDL_Rect cooldownFill = {10, SCREEN_HEIGHT - 30,
                            static_cast<int>(barWidth * (1 - cooldownRatio)), barHeight};
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderFillRect(renderer, &cooldownFill);

    SDL_Color qColor = qReady ? SDL_Color{0, 255, 0, 255} : SDL_Color{255, 0, 0, 255};
    renderText("[Q] Shoot", SCREEN_WIDTH - 120, SCREEN_HEIGHT - 30, qColor);

    SDL_RenderPresent(renderer);
}

void clean() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    srand(time(nullptr));
    if (!init()) return 1;

    player.rect = {SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2.0f,
                   SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2.0f,
                   PLAYER_SIZE, PLAYER_SIZE};
    player.active = true;

    Uint32 lastTime = SDL_GetTicks();
    bool running = true;
    SDL_Event event;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    targetPos.x = event.button.x;
                    targetPos.y = event.button.y;
                    markers.push_back({{(float)targetPos.x, (float)targetPos.y}, 0.5f});
                }
            }

            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_q) {
                    if (currentWeapon == SINGLE) shootProjectile();
                    else if (currentWeapon == SHOTGUN) shootShotgun();
                }
                if (upgradePoints >= 10) {
                    if (event.key.keysym.sym == SDLK_1) applyUpgrade(1);
                    if (event.key.keysym.sym == SDLK_2) applyUpgrade(2);
                    if (event.key.keysym.sym == SDLK_3) applyUpgrade(3);
                    if (event.key.keysym.sym == SDLK_4) applyUpgrade(4);
                }
            }
        }

        if (rand() % static_cast<int>(spawnRate) == 0) spawnEnemy();

        update(deltaTime);
        render();
        SDL_Delay(16);
    }

    clean();
    return 0;
}
