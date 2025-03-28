#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <cfloat>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int PLAYER_SIZE = 300;
const int PROJECTILE_SIZE = 100;
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
const int NUM_MAPS = 4;

enum EnemyType { BASIC, FAST, CHASER };
enum WeaponType { SINGLE, SHOTGUN };
enum PlayerState { IDLE, WALK, RUN, ATTACK, HURT, DEAD };
enum EnemyState { WALKING, SLASHING, DYING, HIT };

SDL_Texture* projectileTexture = nullptr;
SDL_Texture* speedBoostTexture = nullptr;
SDL_Texture* damageBoostTexture = nullptr;
SDL_Texture* shieldTexture = nullptr;
SDL_Texture* maps[NUM_MAPS] = {nullptr};
int currentMap = 0;

struct Animation {
    SDL_Texture* texture;
    std::vector<SDL_Rect> frames;
    float frameTime;
    size_t currentFrame;
    float elapsedTime;
    SDL_RendererFlip flip;
    int frameWidth;
    int frameHeight;
};

struct EnemyAnimations {
    Animation walking;
    Animation slashing;
    Animation dying;
};

struct PlayerAnimations {
    Animation idle;
    Animation walk;
    Animation run;
    Animation attack;
    Animation hurt;
    Animation dead;
};

EnemyAnimations enemyBasicAnim;
EnemyAnimations enemyFastAnim;
EnemyAnimations enemyChaserAnim;
PlayerAnimations playerAnim;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;

struct GameObject {
    SDL_FRect rect;
    SDL_FRect hitbox;
    float vx, vy;
    bool active;
    EnemyType type;
    int health;
    float angle;
    union {
        EnemyState enemyState;
        PlayerState playerState;
    };
    float animTime;
    float hitEffectTimer; // For enemy hit effect

    void updateHitbox() {
        float hitboxScale = 0.7f;
        float offset = (1.0f - hitboxScale) * rect.w / 2.0f;
        hitbox = {rect.x + offset, rect.y + offset, rect.w * hitboxScale, rect.h * hitboxScale};
    }
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

void loadAnimation(Animation& anim, const std::string& path, int frameCount, float frameTime, bool isSpriteSheet = false) {
    anim.frameTime = frameTime;
    anim.currentFrame = 0;
    anim.elapsedTime = 0.0f;
    anim.flip = SDL_FLIP_NONE;

    if (isSpriteSheet) {
        anim.texture = IMG_LoadTexture(renderer, path.c_str());
        if (!anim.texture) {
            std::cout << "ERROR: Failed to load texture: " << path << " - " << IMG_GetError() << std::endl;
            return;
        }
        int w, h;
        SDL_QueryTexture(anim.texture, nullptr, nullptr, &w, &h);
        anim.frameWidth = w / frameCount;
        anim.frameHeight = h;
        anim.frames.resize(frameCount);
        for (int i = 0; i < frameCount; i++) {
            anim.frames[i] = {i * anim.frameWidth, 0, anim.frameWidth, anim.frameHeight};
        }
    } else {
        anim.frames.resize(frameCount);
        for (int i = 0; i < frameCount; i++) {
            std::string framePath = path.substr(0, path.find_last_of('_')) + "_" + std::to_string(i + 1) + ".png";
            SDL_Texture* frameTexture = IMG_LoadTexture(renderer, framePath.c_str());
            if (!frameTexture) {
                std::cout << "ERROR: Failed to load frame: " << framePath << " - " << IMG_GetError() << std::endl;
                continue;
            }
            int w, h;
            SDL_QueryTexture(frameTexture, nullptr, nullptr, &w, &h);
            anim.frameWidth = w;
            anim.frameHeight = h;
            anim.frames[i] = {0, 0, w, h};
            if (i == 0) anim.texture = frameTexture;
            else SDL_DestroyTexture(frameTexture);
        }
    }
}

void updateAnimation(Animation& anim, float deltaTime, bool loop = true) {
    anim.elapsedTime += deltaTime;
    if (anim.elapsedTime >= anim.frameTime) {
        anim.currentFrame = (anim.currentFrame + 1) % anim.frames.size();
        anim.elapsedTime = 0.0f;
        if (!loop && anim.currentFrame == 0) {
            anim.currentFrame = anim.frames.size() - 1;
        }
    }
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        std::cout << "SDL_image init failed: " << IMG_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cout << "TTF init failed: " << TTF_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("DodgeAndQ", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cout << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cout << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
        return false;
    }

    loadAnimation(playerAnim.idle, "assets/player/idle.png", 6, 0.1f, true);
    loadAnimation(playerAnim.walk, "assets/player/walk.png", 7, 0.07f, true);
    loadAnimation(playerAnim.run, "assets/player/run.png", 8, 0.05f, true);
    loadAnimation(playerAnim.attack, "assets/player/attack.png", 7, 0.03f, true);
    loadAnimation(playerAnim.hurt, "assets/player/hurt.png", 4, 0.05f, true);
    loadAnimation(playerAnim.dead, "assets/player/dead.png", 4, 0.07f, true);

    loadAnimation(enemyBasicAnim.walking, "assets/enemy_basic/Walking/enemy_basic_Walking_1.png", 24, 0.05f, false);
    loadAnimation(enemyBasicAnim.slashing, "assets/enemy_basic/Slashing/enemy_basic_Slashing_1.png", 12, 0.06f, false);
    loadAnimation(enemyBasicAnim.dying, "assets/enemy_basic/Dying/enemy_basic_Dying_1.png", 15, 0.05f, false);

    loadAnimation(enemyFastAnim.walking, "assets/enemy_fast/Walking/enemy_fast_Walking_1.png", 24, 0.04f, false);
    loadAnimation(enemyFastAnim.slashing, "assets/enemy_fast/Slashing/enemy_fast_Slashing_1.png", 12, 0.06f, false);
    loadAnimation(enemyFastAnim.dying, "assets/enemy_fast/Dying/enemy_fast_Dying_1.png", 15, 0.05f, false);

    loadAnimation(enemyChaserAnim.walking, "assets/enemy_chaser/Walking/enemy_chaser_Walking_1.png", 24, 0.05f, false);
    loadAnimation(enemyChaserAnim.slashing, "assets/enemy_chaser/Slashing/enemy_chaser_Slashing_1.png", 12, 0.06f, false);
    loadAnimation(enemyChaserAnim.dying, "assets/enemy_chaser/Dying/enemy_chaser_Dying_1.png", 15, 0.05f, false);

    maps[0] = IMG_LoadTexture(renderer, "assets/map1.png");
    if (!maps[0]) std::cout << "Failed to load map1.png: " << IMG_GetError() << std::endl;
    maps[1] = IMG_LoadTexture(renderer, "assets/map2.png");
    if (!maps[1]) std::cout << "Failed to load map2.png: " << IMG_GetError() << std::endl;
    maps[2] = IMG_LoadTexture(renderer, "assets/map3.png");
    if (!maps[2]) std::cout << "Failed to load map3.png: " << IMG_GetError() << std::endl;
    maps[3] = IMG_LoadTexture(renderer, "assets/map4.png");
    if (!maps[3]) std::cout << "Failed to load map4.png: " << IMG_GetError() << std::endl;
    currentMap = rand() % NUM_MAPS;

    projectileTexture = IMG_LoadTexture(renderer, "assets/projectile.png");
    if (!projectileTexture) std::cout << "Failed to load projectile.png: " << IMG_GetError() << std::endl;
    speedBoostTexture = IMG_LoadTexture(renderer, "assets/speed_boost.png");
    if (!speedBoostTexture) std::cout << "Failed to load speed_boost.png: " << IMG_GetError() << std::endl;
    damageBoostTexture = IMG_LoadTexture(renderer, "assets/damage_boost.png");
    if (!damageBoostTexture) std::cout << "Failed to load damage_boost.png: " << IMG_GetError() << std::endl;
    shieldTexture = IMG_LoadTexture(renderer, "assets/shield.png");
    if (!shieldTexture) std::cout << "Failed to load shield.png: " << IMG_GetError() << std::endl;

    SDL_ShowCursor(SDL_ENABLE);
    return true;
}

void spawnEnemy() {
    GameObject enemy;
    enemy.type = static_cast<EnemyType>(rand() % 3);
    enemy.rect.w = PLAYER_SIZE;
    enemy.rect.h = PLAYER_SIZE;
    enemy.updateHitbox();

    switch(enemy.type) {
        case BASIC: enemy.health = 1; break;
        case FAST: enemy.health = 1; break;
        case CHASER: enemy.health = 2; break;
    }

    if (rand() % 2 == 0) {
        enemy.rect.x = -enemy.rect.w;
    } else {
        enemy.rect.x = SCREEN_WIDTH;
    }
    enemy.rect.y = rand() % SCREEN_HEIGHT;

    enemy.active = true;
    enemy.enemyState = WALKING;
    enemy.animTime = 0.0f;
    enemy.angle = 0.0f;
    enemy.hitEffectTimer = 0.0f;
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

GameObject* findNearestEnemy(float x, float y) {
    GameObject* nearest = nullptr;
    float minDist = FLT_MAX;
    for (auto& enemy : enemies) {
        if (!enemy.active || enemy.enemyState == DYING) continue;
        float dx = enemy.rect.x + enemy.rect.w / 2 - x;
        float dy = enemy.rect.y + enemy.rect.h / 2 - y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < minDist) {
            minDist = dist;
            nearest = &enemy;
        }
    }
    return nearest;
}

void shootProjectile() {
    if (!qReady) return;

    GameObject proj;
    proj.rect.w = PROJECTILE_SIZE;
    proj.rect.h = PROJECTILE_SIZE;
    proj.rect.x = player.rect.x + player.rect.w / 2 - PROJECTILE_SIZE / 2;
    proj.rect.y = player.rect.y + player.rect.h / 2 - PROJECTILE_SIZE / 2;
    proj.updateHitbox();

    GameObject* target = findNearestEnemy(proj.rect.x + proj.rect.w / 2, proj.rect.y + proj.rect.h / 2);
    if (target) {
        float dx = (target->rect.x + target->rect.w / 2) - (proj.rect.x + proj.rect.w / 2);
        float dy = (target->rect.y + target->rect.h / 2) - (proj.rect.y + proj.rect.h / 2);
        float length = std::sqrt(dx * dx + dy * dy);
        if (length > 0) {
            proj.vx = (dx / length) * PROJECTILE_SPEED;
            proj.vy = (dy / length) * PROJECTILE_SPEED;
            proj.angle = std::atan2(dy, dx) * 180 / M_PI;
        }
    } else {
        proj.vx = PROJECTILE_SPEED;
        proj.vy = 0;
        proj.angle = 0;
    }

    proj.active = true;
    projectiles.push_back(proj);
    qReady = false;
    qCooldown = qCooldownMax;
}

void shootShotgun() {
    if (!qReady) return;

    float baseX = player.rect.x + player.rect.w / 2 - PROJECTILE_SIZE / 2;
    float baseY = player.rect.y + player.rect.h / 2 - PROJECTILE_SIZE / 2;
    GameObject* target = findNearestEnemy(baseX + PROJECTILE_SIZE / 2, baseY + PROJECTILE_SIZE / 2);

    for (int i = -2; i <= 2; i++) {
        GameObject proj;
        proj.rect.w = PROJECTILE_SIZE;
        proj.rect.h = PROJECTILE_SIZE;
        proj.rect.x = baseX;
        proj.rect.y = baseY;
        proj.updateHitbox();

        if (target) {
            float dx = (target->rect.x + target->rect.w / 2) - (proj.rect.x + proj.rect.w / 2);
            float dy = (target->rect.y + target->rect.h / 2) - (proj.rect.y + proj.rect.h / 2);
            float length = std::sqrt(dx * dx + dy * dy);
            float angleOffset = i * 10.0f;
            float rad = std::atan2(dy, dx) + angleOffset * M_PI / 180.0f;
            if (length > 0) {
                proj.vx = std::cos(rad) * PROJECTILE_SPEED;
                proj.vy = std::sin(rad) * PROJECTILE_SPEED;
                proj.angle = rad * 180 / M_PI;
            }
        } else {
            proj.vx = PROJECTILE_SPEED + i * 100.0f;
            proj.vy = 0;
            proj.angle = 0;
        }

        proj.active = true;
        projectiles.push_back(proj);
    }
    qReady = false;
    qCooldown = qCooldownMax * 1.5f;
}

void renderText(const std::string& text, int x, int y, SDL_Color color = {255, 255, 255}) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

bool checkCollision(const SDL_FRect& a, const SDL_FRect& b) {
    return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y);
}

void applyUpgrade(int choice) {
    if (upgradePoints < 10) return;
    switch (choice) {
        case 1: playerSpeed *= 1.5f; break;
        case 2: qCooldownMax *= 0.5f; break;
        case 3: projectileDamage += 3; break;
        case 4: currentWeapon = SHOTGUN; break;
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
        enemies.clear();
        int newMap;
        do {
            newMap = rand() % NUM_MAPS;
        } while (newMap == currentMap);
        currentMap = newMap;
        SDL_Delay(500);
    }

    // Move player to center on target position
    float dx = targetPos.x - (player.rect.x + player.rect.w / 2);
    float dy = targetPos.y - (player.rect.y + player.rect.h / 2);
    float distance = std::sqrt(dx * dx + dy * dy);
    if (distance > 5.0f) {
        float speed = playerSpeed * deltaTime;
        float vx = (dx / distance) * speed;
        float vy = (dy / distance) * speed;
        player.rect.x += vx;
        player.rect.y += vy;
        player.updateHitbox();
        player.playerState = distance > 150.0f ? RUN : WALK;
    } else {
        player.playerState = IDLE;
    }

    Animation* currentPlayerAnim = nullptr;
    switch (player.playerState) {
        case IDLE: currentPlayerAnim = &playerAnim.idle; break;
        case WALK: currentPlayerAnim = &playerAnim.walk; break;
        case RUN: currentPlayerAnim = &playerAnim.run; break;
        case ATTACK: currentPlayerAnim = &playerAnim.attack; break;
        case HURT: currentPlayerAnim = &playerAnim.hurt; break;
        case DEAD: currentPlayerAnim = &playerAnim.dead; break;
    }
    if (currentPlayerAnim) {
        updateAnimation(*currentPlayerAnim, deltaTime, player.playerState != DEAD);
        currentPlayerAnim->flip = (dx < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    }

    float currentEnemySpeed = ENEMY_SPEED + (level - 1) * ENEMY_SPEED_INCREASE;
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;

        float dx = player.rect.x - enemy.rect.x;
        float dy = player.rect.y - enemy.rect.y;
        float length = std::sqrt(dx * dx + dy * dy);

        Animation* currentAnim = nullptr;
        switch (enemy.type) {
            case BASIC:
                if (enemy.enemyState == WALKING || enemy.enemyState == HIT) currentAnim = &enemyBasicAnim.walking;
                else if (enemy.enemyState == SLASHING) currentAnim = &enemyBasicAnim.slashing;
                else if (enemy.enemyState == DYING) currentAnim = &enemyBasicAnim.dying;
                break;
            case FAST:
                if (enemy.enemyState == WALKING || enemy.enemyState == HIT) currentAnim = &enemyFastAnim.walking;
                else if (enemy.enemyState == SLASHING) currentAnim = &enemyFastAnim.slashing;
                else if (enemy.enemyState == DYING) currentAnim = &enemyFastAnim.dying;
                break;
            case CHASER:
                if (enemy.enemyState == WALKING || enemy.enemyState == HIT) currentAnim = &enemyChaserAnim.walking;
                else if (enemy.enemyState == SLASHING) currentAnim = &enemyChaserAnim.slashing;
                else if (enemy.enemyState == DYING) currentAnim = &enemyChaserAnim.dying;
                break;
        }

        if (currentAnim) {
            updateAnimation(*currentAnim, deltaTime, enemy.enemyState != DYING);
            currentAnim->flip = (dx < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            if (enemy.enemyState == DYING && currentAnim->currentFrame == currentAnim->frames.size() - 1) {
                enemy.active = false;
                score += SCORE_PER_KILL * (combo + 1);
                combo++;
                comboTime = COMBO_TIMEOUT;
                upgradePoints += 1;
                spawnPowerUp(enemy.rect.x, enemy.rect.y);
            }
        }

        if (enemy.enemyState != DYING) {
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
            enemy.updateHitbox();
        }

        if (enemy.hitEffectTimer > 0) {
            enemy.hitEffectTimer -= deltaTime;
            if (enemy.hitEffectTimer <= 0) {
                enemy.enemyState = WALKING;
            } else {
                enemy.rect.y -= 100.0f * deltaTime; // Jump effect
            }
        }

        if (length < 50.0f && enemy.enemyState != DYING && enemy.enemyState != HIT) {
            enemy.enemyState = SLASHING;
        } else if (enemy.enemyState != DYING && enemy.enemyState != HIT) {
            enemy.enemyState = WALKING;
        }
    }

    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        proj.rect.x += proj.vx * deltaTime;
        proj.rect.y += proj.vy * deltaTime;
        proj.updateHitbox();
        if (proj.rect.x + proj.rect.w < 0 || proj.rect.x > SCREEN_WIDTH ||
            proj.rect.y + proj.rect.h < 0 || proj.rect.y > SCREEN_HEIGHT) {
            proj.active = false;
        }
    }

    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        for (auto& enemy : enemies) {
            if (!enemy.active || enemy.enemyState == DYING) continue;
            if (checkCollision(proj.hitbox, enemy.hitbox)) {
                proj.active = false;
                enemy.health -= projectileDamage;
                qReady = true;
                qCooldown = 0;
                if (enemy.health > 0) {
                    enemy.enemyState = HIT;
                    enemy.hitEffectTimer = 0.2f; // Red flash and jump duration
                } else if (enemy.enemyState != DYING) {
                    enemy.enemyState = DYING;
                    enemy.vx = 0;
                    enemy.vy = 0;
                }
                break;
            }
        }
    }

    static float lastDamageTime = 0.0f;
    if (!shieldActive) {
        for (auto& enemy : enemies) {
            if (!enemy.active || enemy.enemyState != SLASHING) continue;
            if (checkCollision(player.hitbox, enemy.hitbox) && gameTime - lastDamageTime >= 0.5f) {
                playerHealth -= 20;
                lastDamageTime = gameTime;
                if (playerHealth > 0 && player.playerState != DEAD) {
                    player.playerState = HURT;
                    player.animTime = 0.0f;
                }
                if (playerHealth <= 0) {
                    player.playerState = DEAD;
                }
            }
        }
    }

    for (auto& pu : powerUps) {
        if (pu.active && checkCollision(player.hitbox, pu.rect)) {
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
            player.playerState = ATTACK;
            player.animTime = 0.0f;
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

    if (maps[currentMap]) {
        SDL_RenderCopy(renderer, maps[currentMap], nullptr, nullptr);
    }

    Animation* currentPlayerAnim = nullptr;
    switch (player.playerState) {
        case IDLE: currentPlayerAnim = &playerAnim.idle; break;
        case WALK: currentPlayerAnim = &playerAnim.walk; break;
        case RUN: currentPlayerAnim = &playerAnim.run; break;
        case ATTACK: currentPlayerAnim = &playerAnim.attack; break;
        case HURT: currentPlayerAnim = &playerAnim.hurt; break;
        case DEAD: currentPlayerAnim = &playerAnim.dead; break;
    }
    if (currentPlayerAnim && currentPlayerAnim->texture && !currentPlayerAnim->frames.empty()) {
        SDL_Rect* frame = &currentPlayerAnim->frames[currentPlayerAnim->currentFrame];
        SDL_FRect renderRect = {player.rect.x, player.rect.y, PLAYER_SIZE, PLAYER_SIZE};
        SDL_RenderCopyExF(renderer, currentPlayerAnim->texture, frame, &renderRect, player.angle, nullptr, currentPlayerAnim->flip);
    }

    for (const auto& enemy : enemies) {
        if (!enemy.active) continue;
        Animation* currentAnim = nullptr;
        switch (enemy.type) {
            case BASIC:
                if (enemy.enemyState == WALKING || enemy.enemyState == HIT) currentAnim = &enemyBasicAnim.walking;
                else if (enemy.enemyState == SLASHING) currentAnim = &enemyBasicAnim.slashing;
                else if (enemy.enemyState == DYING) currentAnim = &enemyBasicAnim.dying;
                break;
            case FAST:
                if (enemy.enemyState == WALKING || enemy.enemyState == HIT) currentAnim = &enemyFastAnim.walking;
                else if (enemy.enemyState == SLASHING) currentAnim = &enemyFastAnim.slashing;
                else if (enemy.enemyState == DYING) currentAnim = &enemyFastAnim.dying;
                break;
            case CHASER:
                if (enemy.enemyState == WALKING || enemy.enemyState == HIT) currentAnim = &enemyChaserAnim.walking;
                else if (enemy.enemyState == SLASHING) currentAnim = &enemyChaserAnim.slashing;
                else if (enemy.enemyState == DYING) currentAnim = &enemyChaserAnim.dying;
                break;
        }
        if (currentAnim && currentAnim->texture && !currentAnim->frames.empty()) {
            SDL_Rect* frame = &currentAnim->frames[currentAnim->currentFrame];
            SDL_FRect renderRect = {enemy.rect.x, enemy.rect.y, PLAYER_SIZE, PLAYER_SIZE};
            if (enemy.hitEffectTimer > 0) {
                SDL_SetTextureColorMod(currentAnim->texture, 255, 0, 0); // Red flash
            }
            SDL_RenderCopyExF(renderer, currentAnim->texture, frame, &renderRect, 0, nullptr, currentAnim->flip);
            if (enemy.hitEffectTimer > 0) {
                SDL_SetTextureColorMod(currentAnim->texture, 255, 255, 255); // Reset color
            }
        }
    }

    for (const auto& proj : projectiles) {
        if (!proj.active) continue;
        if (projectileTexture) {
            SDL_RenderCopyExF(renderer, projectileTexture, nullptr, &proj.rect, proj.angle, nullptr, SDL_FLIP_NONE);
        }
    }

    for (const auto& pu : powerUps) {
        if (!pu.active) continue;
        SDL_Texture* texture = nullptr;
        SDL_Color fallbackColor;
        switch (pu.type) {
            case PowerUp::SPEED_BOOST: texture = speedBoostTexture; fallbackColor = {0, 255, 255, 255}; break;
            case PowerUp::DAMAGE_BOOST: texture = damageBoostTexture; fallbackColor = {255, 0, 0, 255}; break;
            case PowerUp::SHIELD: texture = shieldTexture; fallbackColor = {255, 255, 255, 255}; break;
        }
        if (texture) {
            SDL_RenderCopyF(renderer, texture, nullptr, &pu.rect);
        }
    }

    int healthBarX = 10, healthBarY = 10, healthBarWidth = 200, healthBarHeight = 20;
    float healthRatio = static_cast<float>(playerHealth) / MAX_HEALTH;
    SDL_Rect healthBg = {healthBarX, healthBarY, healthBarWidth, healthBarHeight};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &healthBg);
    SDL_Rect healthFill = {healthBarX, healthBarY, static_cast<int>(healthBarWidth * healthRatio), healthBarHeight};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &healthFill);
    SDL_Rect healthBorder = {healthBarX - 2, healthBarY - 2, healthBarWidth + 4, healthBarHeight + 4};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &healthBorder);

    renderText("Score: " + std::to_string(score), 10, 40);
    renderText("Level: " + std::to_string(level), 10, 70);
    renderText("Upgrade Points: " + std::to_string(upgradePoints), 10, 100);
    if (upgradePoints >= 10) {
        renderText("1: Speed | 2: Cooldown | 3: Damage | 4: Shotgun", 10, 130);
    }

    renderText("Speed Boost: " + std::string(speedBoostActive ? "Active" : "Inactive") + " (" +
               std::to_string(static_cast<int>(speedBoostTimer)) + "s)", 10, 160, {0, 255, 255});
    renderText("Damage Boost: " + std::string(damageBoostActive ? "Active" : "Inactive") + " (" +
               std::to_string(static_cast<int>(damageBoostTimer)) + "s)", 10, 190, {255, 0, 0});
    renderText("Shield: " + std::string(shieldActive ? "Active" : "Inactive") + " (" +
               std::to_string(static_cast<int>(shieldTimer)) + "s)", 10, 220, {255, 255, 255});
    renderText("Weapon: " + std::string(currentWeapon == SINGLE ? "Single" : "Shotgun"), 10, 250);
    renderText("Combo: " + std::to_string(combo), 10, 280);

    const int barWidth = 200, barHeight = 15;
    float cooldownRatio = qCooldown / qCooldownMax;
    SDL_Rect cooldownBg = {10, SCREEN_HEIGHT - 30, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &cooldownBg);
    SDL_Rect cooldownFill = {10, SCREEN_HEIGHT - 30, static_cast<int>(barWidth * (1 - cooldownRatio)), barHeight};
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderFillRect(renderer, &cooldownFill);
    SDL_Color qColor = qReady ? SDL_Color{0, 255, 0, 255} : SDL_Color{255, 0, 0, 255};
    renderText("[Q] Shoot", SCREEN_WIDTH - 120, SCREEN_HEIGHT - 30, qColor);

    SDL_RenderPresent(renderer);
}

void clean() {
    SDL_DestroyTexture(playerAnim.idle.texture);
    SDL_DestroyTexture(playerAnim.walk.texture);
    SDL_DestroyTexture(playerAnim.run.texture);
    SDL_DestroyTexture(playerAnim.attack.texture);
    SDL_DestroyTexture(playerAnim.hurt.texture);
    SDL_DestroyTexture(playerAnim.dead.texture);

    SDL_DestroyTexture(enemyBasicAnim.walking.texture);
    SDL_DestroyTexture(enemyBasicAnim.slashing.texture);
    SDL_DestroyTexture(enemyBasicAnim.dying.texture);
    SDL_DestroyTexture(enemyFastAnim.walking.texture);
    SDL_DestroyTexture(enemyFastAnim.slashing.texture);
    SDL_DestroyTexture(enemyFastAnim.dying.texture);
    SDL_DestroyTexture(enemyChaserAnim.walking.texture);
    SDL_DestroyTexture(enemyChaserAnim.slashing.texture);
    SDL_DestroyTexture(enemyChaserAnim.dying.texture);
    SDL_DestroyTexture(projectileTexture);
    SDL_DestroyTexture(speedBoostTexture);
    SDL_DestroyTexture(damageBoostTexture);
    SDL_DestroyTexture(shieldTexture);
    for (int i = 0; i < NUM_MAPS; i++) {
        SDL_DestroyTexture(maps[i]);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    srand(time(nullptr));
    if (!init()) return 1;

    player.rect = {SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2.0f, SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2.0f, (float)PLAYER_SIZE, (float)PLAYER_SIZE};
    player.updateHitbox();
    player.active = true;
    player.health = MAX_HEALTH;
    player.playerState = IDLE;
    player.animTime = 0.0f;
    player.angle = 0.0f;

    Uint32 lastTime = SDL_GetTicks();
    bool running = true;
    SDL_Event event;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                targetPos.x = event.button.x;
                targetPos.y = event.button.y;
                markers.push_back({{targetPos.x, targetPos.y}, 0.5f});
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

        if (player.playerState == DEAD) {
            running = false;
        }
    }

    clean();
    return 0;
}
