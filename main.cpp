#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <cfloat>
#include <direct.h>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int PLAYER_SIZE = 168;
const int PROJECTILE_SIZE = 100;
const float PLAYER_SPEED = 300.0f;
const float ENEMY_SPEED = 150.0f;
const float PROJECTILE_SPEED = 1200.0f;
const float Q_COOLDOWN = 0.5f;
const int SCORE_PER_KILL = 10;
const float COMBO_TIMEOUT = 3.0f;
const float LEVEL_DURATION = 30.0f;
const float SPAWN_RATE_BASE = 40.0f;
const float SPAWN_RATE_DECREASE = 15.0f;
const float ENEMY_SPEED_INCREASE = 30.0f;
const int NUM_MAPS = 4;
const float MIN_SPAWN_DISTANCE = 150.0f;
const float SPAWN_DELAY = 0.2f;
const float SLASHING_DISTANCE = 80.0f;
const int MAX_HEALTH = 100;
const float ENEMY_ATTACK_COOLDOWN = 1.5f;
const float PRE_LEVEL_UP_DELAY = 3.0f;

enum GameState { MENU, SETTINGS, PLAYING, PAUSED, LEVEL_UP, UPGRADE_MENU, GAME_OVER, PRE_LEVEL_UP };
enum EnemyType { BASIC, FAST, CHASER };
enum WeaponType { SINGLE, SHOTGUN };
enum PlayerState { IDLE, WALK, ATTACK, HURT, DEAD };
enum EnemyState { WALKING, SLASHING, DYING };

SDL_Texture* projectileTexture = nullptr;
SDL_Texture* maps[NUM_MAPS] = {nullptr};
SDL_Texture* menuBackground = nullptr;
int currentMap = 0;

Mix_Music* gameMusic = nullptr;
Mix_Chunk* shootSound = nullptr;
Mix_Chunk* hurtSound = nullptr;
Mix_Chunk* deathSound = nullptr;
Mix_Chunk* enemyAttackSound = nullptr;
Mix_Chunk* enemyDeathSound = nullptr;
Mix_Chunk* spawnSound = nullptr;
Mix_Chunk* levelUpSound = nullptr;
Mix_Chunk* upgradeSound = nullptr;
Mix_Chunk* clickSound = nullptr;


struct Animation {
    std::vector<SDL_Texture*> textures; // Thay vì chỉ một texture
    std::vector<SDL_Rect> frames;
    float frameTime;
    size_t currentFrame;
    float elapsedTime;
    SDL_RendererFlip flip;
    int frameWidth;
    int frameHeight;
};

struct PlayerAnimations {
    Animation idle;
    Animation walk;
    Animation attack;
    Animation hurt;
    Animation dead;
};

struct EnemyAnimations {
    Animation walking;
    Animation slashing;
    Animation dying;
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
    float hitEffectTimer;
    float hurtTimer;
    float attackCooldown;

    void updateHitbox() {
        float hitboxScale = 0.5f;
        float offsetX = (rect.w - rect.w * hitboxScale) / 2.0f;
        float offsetY = (rect.h - rect.h * hitboxScale) / 2.0f;
        hitbox = {rect.x + offsetX, rect.y + offsetY, rect.w * hitboxScale, rect.h * hitboxScale};
    }
};

struct Marker {
    SDL_FPoint position;
    float timer;
    bool isSpawnMarker;
};

struct Button {
    SDL_Rect rect;
    std::string text;
    SDL_Color color;
    bool hovered;
};

std::vector<GameObject> enemies;
std::vector<GameObject> projectiles;
std::vector<Marker> markers;
GameState previousState = MENU; // Biến mới để lưu trạng thái trước khi vào SETTINGS
GameObject player;
int score = 0;
int combo = 0;
float comboTime = 0;
float qCooldown = 0;
bool qReady = true;
float gameTime = 0.0f;
int level = 1;
float spawnRate = SPAWN_RATE_BASE;
int upgradePoints = 0;
int projectileDamage = 1;
float playerSpeed = PLAYER_SPEED;
float qCooldownMax = Q_COOLDOWN;
WeaponType currentWeapon = SINGLE;
GameState gameState = MENU;
float levelUpTimer = 0.0f;
float preLevelUpTimer = 0.0f;
float deathTimer = 0.0f;
float lastDamageTime = 0.0f;
int musicVolume = 64;
int sfxVolume = 64;
bool shotgunUnlocked = false; // Theo dõi xem shotgun đã được chọn chưa
bool draggingMusicSlider = false;
bool draggingSFXSlider = false;



void loadAnimation(Animation& anim, const std::string& path, int frameCount, float frameTime, bool isSpriteSheet = false) {
    anim.frameTime = frameTime;
    anim.currentFrame = 0;
    anim.elapsedTime = 0.0f;
    anim.flip = SDL_FLIP_NONE;

    if (isSpriteSheet) {
        // Xử lý sprite sheet (cho player) - giữ nguyên
        SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
        if (!texture) std::cout << "ERROR: Failed to load texture: " << path << " - " << IMG_GetError() << std::endl;
        anim.textures.resize(1); // Chỉ cần 1 texture cho sprite sheet
        anim.textures[0] = texture;
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        anim.frameWidth = w / frameCount;
        anim.frameHeight = h;
        anim.frames.resize(frameCount);
        for (int i = 0; i < frameCount; i++) anim.frames[i] = {i * anim.frameWidth, 0, anim.frameWidth, anim.frameHeight};
    } else {
        // Xử lý nhiều file PNG (cho enemy)
        anim.frames.clear();
        anim.textures.clear();
        std::string basePath = path.substr(0, path.find_last_of('_') + 1);
        for (int i = 0; i < frameCount; i++) {
            std::string framePath = basePath + std::to_string(i + 1) + ".png";
            SDL_Texture* frameTexture = IMG_LoadTexture(renderer, framePath.c_str());
            if (!frameTexture) std::cout << "ERROR: Failed to load frame: " << framePath << " - " << IMG_GetError() << std::endl;
            int w, h;
            SDL_QueryTexture(frameTexture, nullptr, nullptr, &w, &h);
            anim.frameWidth = w;
            anim.frameHeight = h;
            anim.frames.push_back({0, 0, w, h});
            anim.textures.push_back(frameTexture); // Lưu texture của từng frame
        }
    }
}

void updateAnimation(Animation& anim, float deltaTime, bool loop = true) {
    anim.elapsedTime += deltaTime;
    if (anim.elapsedTime >= anim.frameTime) {
        anim.elapsedTime -= anim.frameTime;
        anim.currentFrame = (anim.currentFrame + 1) % (loop ? anim.frames.size() : anim.frames.size() + 1);
        if (!loop && anim.currentFrame >= anim.frames.size()) anim.currentFrame = anim.frames.size() - 1;
    }
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
    if ((IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG)) != (IMG_INIT_PNG | IMG_INIT_JPG)) return false;
    if (TTF_Init() == -1) return false;
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 1, 512) < 0) return false;

    window = SDL_CreateWindow("DodgeAndQ", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return false;

    font = TTF_OpenFont("assets/arial.ttf", 24);
    if (!font) return false;

    gameMusic = Mix_LoadMUS("assets/audio/game_music.wav");
    if (gameMusic) Mix_VolumeMusic(musicVolume);

    shootSound = Mix_LoadWAV("assets/audio/shoot.wav");
    if (shootSound) Mix_VolumeChunk(shootSound, sfxVolume);
    hurtSound = Mix_LoadWAV("assets/audio/hurt.wav");
    if (hurtSound) Mix_VolumeChunk(hurtSound, sfxVolume);
    deathSound = Mix_LoadWAV("assets/audio/death.wav");
    if (deathSound) Mix_VolumeChunk(deathSound, sfxVolume);
    enemyAttackSound = Mix_LoadWAV("assets/audio/enemy_attack.wav");
    if (enemyAttackSound) Mix_VolumeChunk(enemyAttackSound, sfxVolume);
    enemyDeathSound = Mix_LoadWAV("assets/audio/enemy_death.wav");
    if (enemyDeathSound) Mix_VolumeChunk(enemyDeathSound, sfxVolume);
    spawnSound = Mix_LoadWAV("assets/audio/spawn.wav");
    if (spawnSound) Mix_VolumeChunk(spawnSound, sfxVolume);
    levelUpSound = Mix_LoadWAV("assets/audio/level_up.wav");
    if (levelUpSound) Mix_VolumeChunk(levelUpSound, sfxVolume);
    upgradeSound = Mix_LoadWAV("assets/audio/upgrade.wav");
    if (upgradeSound) Mix_VolumeChunk(upgradeSound, sfxVolume);
    clickSound = Mix_LoadWAV("assets/audio/click.wav");
    if (clickSound) Mix_VolumeChunk(clickSound, sfxVolume);

    loadAnimation(playerAnim.idle, "assets/player/idle.png", 6, 0.1f, true);
    loadAnimation(playerAnim.walk, "assets/player/walk.png", 7, 0.07f, true);
    loadAnimation(playerAnim.attack, "assets/player/attack.png", 7, 0.03f, true);
    loadAnimation(playerAnim.hurt, "assets/player/hurt.png", 4, 0.05f, true);
    loadAnimation(playerAnim.dead, "assets/player/dead.png", 4, 0.07f, true);

    loadAnimation(enemyBasicAnim.walking, "assets/enemy_basic/Walking/enemy_basic_Walking_1.png", 24, 0.05f, false);
    loadAnimation(enemyBasicAnim.slashing, "assets/enemy_basic/Slashing/enemy_basic_Slashing_1.png", 12, 0.0833f, false); // 1.0/12
    loadAnimation(enemyBasicAnim.dying, "assets/enemy_basic/Dying/enemy_basic_Dying_1.png", 15, 0.05f, false); // 15 frame, 0.75 giây tổng

    loadAnimation(enemyFastAnim.walking, "assets/enemy_fast/Walking/enemy_fast_Walking_1.png", 24, 0.05f, false);
    loadAnimation(enemyFastAnim.slashing, "assets/enemy_fast/Slashing/enemy_fast_Slashing_1.png", 12, 0.0417f, false); // 0.5/12
    loadAnimation(enemyFastAnim.dying, "assets/enemy_fast/Dying/enemy_fast_Dying_1.png", 15, 0.05f, false); // 15 frame, 0.75 giây tổng

    loadAnimation(enemyChaserAnim.walking, "assets/enemy_chaser/Walking/enemy_chaser_Walking_1.png", 24, 0.05f, false);
    loadAnimation(enemyChaserAnim.slashing, "assets/enemy_chaser/Slashing/enemy_chaser_Slashing_1.png", 12, 0.0667f, false); // 0.8/12
    loadAnimation(enemyChaserAnim.dying, "assets/enemy_chaser/Dying/enemy_chaser_Dying_1.png", 15, 0.05f, false); // 15 frame, 0.75 giây tổng

    maps[0] = IMG_LoadTexture(renderer, "assets/map1.png");
    maps[1] = IMG_LoadTexture(renderer, "assets/map2.png");
    maps[2] = IMG_LoadTexture(renderer, "assets/map3.png");
    maps[3] = IMG_LoadTexture(renderer, "assets/map4.png");
    currentMap = rand() % NUM_MAPS;

    menuBackground = IMG_LoadTexture(renderer, "assets/menu_background.png");
    projectileTexture = IMG_LoadTexture(renderer, "assets/projectile.png");

    SDL_ShowCursor(SDL_ENABLE);
    return true;
}

void spawnEnemyMarker() {
    SDL_FPoint spawnPos;
    float distance;
    do {
        spawnPos.x = rand() % (SCREEN_WIDTH - PLAYER_SIZE);
        spawnPos.y = rand() % (SCREEN_HEIGHT - PLAYER_SIZE);
        float dx = spawnPos.x - (player.rect.x + player.rect.w / 2);
        float dy = spawnPos.y - (player.rect.y + player.rect.h / 2);
        distance = std::sqrt(dx * dx + dy * dy);
    } while (distance < MIN_SPAWN_DISTANCE);

    markers.push_back({spawnPos, SPAWN_DELAY, true});
}

void spawnEnemyAtMarker(const SDL_FPoint& pos) {
    if (spawnSound) Mix_PlayChannel(-1, spawnSound, 0);
    GameObject enemy;
    enemy.type = static_cast<EnemyType>(rand() % 3);
    enemy.rect = {pos.x - PLAYER_SIZE / 2, pos.y - PLAYER_SIZE / 2, (float)PLAYER_SIZE, (float)PLAYER_SIZE};
    enemy.updateHitbox();
    enemy.health = (enemy.type == CHASER) ? 2 : 1;
    enemy.active = true;
    enemy.enemyState = WALKING;
    enemy.animTime = 0.0f;
    enemy.angle = 0.0f;
    enemy.hitEffectTimer = 0.0f;
    enemy.hurtTimer = 0.0f;
    // Tùy chỉnh attackCooldown và frameTime theo loại enemy
    Animation* slashingAnim = nullptr;
    switch (enemy.type) {
        case BASIC:
            enemy.attackCooldown = 1.0f;
            slashingAnim = &enemyBasicAnim.slashing;
            slashingAnim->frameTime = 1.0f / slashingAnim->frames.size(); // 1.0 / 12 ≈ 0.0833 giây
            break;
        case FAST:
            enemy.attackCooldown = 0.5f;
            slashingAnim = &enemyFastAnim.slashing;
            slashingAnim->frameTime = 0.5f / slashingAnim->frames.size(); // 0.5 / 12 ≈ 0.0417 giây
            break;
        case CHASER:
            enemy.attackCooldown = 0.8f;
            slashingAnim = &enemyChaserAnim.slashing;
            slashingAnim->frameTime = 0.8f / slashingAnim->frames.size(); // 0.8 / 12 ≈ 0.0667 giây
            break;
    }
    enemies.push_back(enemy);
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
    if (shootSound) Mix_PlayChannel(-1, shootSound, 0);
    GameObject proj;
    proj.rect = {player.rect.x + player.rect.w - PROJECTILE_SIZE, player.rect.y + player.rect.h / 2 - PROJECTILE_SIZE / 2, PROJECTILE_SIZE, PROJECTILE_SIZE};
    proj.updateHitbox();

    GameObject* target = findNearestEnemy(player.rect.x + player.rect.w / 2, player.rect.y + player.rect.h / 2);
    if (target) {
        float targetCenterX = target->rect.x + target->rect.w / 2;
        float targetCenterY = target->rect.y + target->rect.h / 2;
        float dx = targetCenterX - (player.rect.x + player.rect.w / 2);
        float dy = targetCenterY - (player.rect.y + player.rect.h / 2);
        float length = std::sqrt(dx * dx + dy * dy);
        if (length > 0) {
            proj.vx = (dx / length) * PROJECTILE_SPEED;
            proj.vy = (dy / length) * PROJECTILE_SPEED;
            proj.angle = std::atan2(dy, dx) * 180 / M_PI;
            playerAnim.attack.flip = (dx < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            playerAnim.idle.flip = playerAnim.attack.flip;
            playerAnim.walk.flip = playerAnim.attack.flip;
        }
    } else {
        proj.vx = PROJECTILE_SPEED;
        proj.vy = 0;
        proj.angle = 0;
        playerAnim.attack.flip = SDL_FLIP_NONE;
        playerAnim.idle.flip = SDL_FLIP_NONE;
        playerAnim.walk.flip = SDL_FLIP_NONE;
    }
    proj.active = true;
    projectiles.push_back(proj);
    qReady = false;
    qCooldown = qCooldownMax;
}

void shootShotgun() {
    if (!qReady) return;
    if (shootSound) Mix_PlayChannel(-1, shootSound, 0);
    float baseX = player.rect.x + player.rect.w - PROJECTILE_SIZE;
    float baseY = player.rect.y + player.rect.h / 2 - PROJECTILE_SIZE / 2;
    GameObject* target = findNearestEnemy(player.rect.x + player.rect.w / 2, player.rect.y + player.rect.h / 2);

    for (int i = -2; i <= 2; i++) {
        GameObject proj;
        proj.rect = {baseX, baseY, PROJECTILE_SIZE, PROJECTILE_SIZE};
        proj.updateHitbox();
        if (target) {
            float targetCenterX = target->rect.x + target->rect.w / 2;
            float targetCenterY = target->rect.y + target->rect.h / 2;
            float dx = targetCenterX - (player.rect.x + player.rect.w / 2);
            float dy = targetCenterY - (player.rect.y + player.rect.h / 2);
            float length = std::sqrt(dx * dx + dy * dy);
            float angleOffset = i * 15.0f;
            float rad = std::atan2(dy, dx) + angleOffset * M_PI / 180.0f;
            if (length > 0) {
                proj.vx = std::cos(rad) * PROJECTILE_SPEED;
                proj.vy = std::sin(rad) * PROJECTILE_SPEED;
                proj.angle = rad * 180 / M_PI;
                playerAnim.attack.flip = (dx < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
                playerAnim.idle.flip = playerAnim.attack.flip;
                playerAnim.walk.flip = playerAnim.attack.flip;
            }
        } else {
            proj.vx = PROJECTILE_SPEED + i * 100.0f;
            proj.vy = 0;
            proj.angle = 0;
            playerAnim.attack.flip = SDL_FLIP_NONE;
            playerAnim.idle.flip = SDL_FLIP_NONE;
            playerAnim.walk.flip = SDL_FLIP_NONE;
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
    if (upgradePoints < 1) return; // Chỉ cần 1 điểm để nâng cấp
    if (upgradeSound) Mix_PlayChannel(-1, upgradeSound, 0);
    switch (choice) {
        case 1: playerSpeed += 50.0f; break;
        case 2: qCooldownMax *= 0.8f; break;
        case 3: projectileDamage += 1; break;
        case 4: if (!shotgunUnlocked) { currentWeapon = SHOTGUN; shotgunUnlocked = true; } break;
    }
    upgradePoints -= 1;
    gameState = PLAYING;
}

void resetGame() {
    enemies.clear();
    projectiles.clear();
    markers.clear();

    player.rect = {SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2.0f, SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2.0f, (float)PLAYER_SIZE, (float)PLAYER_SIZE};
    player.updateHitbox();
    player.active = true;
    player.health = MAX_HEALTH;
    player.playerState = IDLE;
    player.animTime = 0.0f;
    player.angle = 0.0f;
    player.hurtTimer = 0.0f;
    player.vx = 0.0f;
    player.vy = 0.0f;

    score = 0;
    combo = 0;
    comboTime = 0.0f;
    qCooldown = 0.0f;
    qReady = true;
    gameTime = 0.0f;
    level = 1;
    spawnRate = SPAWN_RATE_BASE;
    upgradePoints = 0;
    projectileDamage = 1;
    playerSpeed = PLAYER_SPEED;
    qCooldownMax = Q_COOLDOWN;
    currentWeapon = SINGLE;
    gameState = PLAYING;
    levelUpTimer = 0.0f;
    preLevelUpTimer = 0.0f;
    deathTimer = 0.0f;
    lastDamageTime = 0.0f;
    currentMap = rand() % NUM_MAPS;
    shotgunUnlocked = false;

    playerAnim.idle.currentFrame = 0;
    playerAnim.idle.elapsedTime = 0.0f;
    playerAnim.idle.flip = SDL_FLIP_NONE;
    SDL_SetTextureColorMod(playerAnim.idle.textures[0], 255, 255, 255); // Sửa từ texture thành textures[0]

    for (int i = 0; i < 2; i++) spawnEnemyMarker();

    Mix_HaltMusic();
    if (gameMusic) Mix_PlayMusic(gameMusic, -1);
}
void update(float deltaTime) {
    switch (gameState) {
        case MENU: {
            if (!Mix_PlayingMusic() && gameMusic) Mix_PlayMusic(gameMusic, -1);
            break;
        }
        case SETTINGS: {
            if (!Mix_PlayingMusic() && gameMusic) Mix_PlayMusic(gameMusic, -1);
            break;
        }
        case PAUSED: {
            break;
        }
        case PRE_LEVEL_UP: {
            preLevelUpTimer -= deltaTime;
            for (auto& enemy : enemies) {
                if (enemy.active && enemy.enemyState != DYING) {
                    enemy.enemyState = DYING;
                    enemy.health = 0;
                    enemy.vx = 0;
                    enemy.vy = 0;
                }
            }
            if (preLevelUpTimer <= 0) {
                if (levelUpSound) Mix_PlayChannel(-1, levelUpSound, 0);
                gameState = LEVEL_UP;
                levelUpTimer = 2.0f;
            }
            break;
        }
        case LEVEL_UP: {
            levelUpTimer -= deltaTime;
            if (player.health < MAX_HEALTH) player.health = MAX_HEALTH;
            if (levelUpTimer <= 0) {
                level++;
                upgradePoints++;
                spawnRate = std::max(SPAWN_RATE_BASE - (level - 1) * SPAWN_RATE_DECREASE, 10.0f);
                enemies.clear();
                int newMap;
                do { newMap = rand() % NUM_MAPS; } while (newMap == currentMap);
                currentMap = newMap;
                gameState = (upgradePoints >= 1) ? UPGRADE_MENU : PLAYING;
            }
            break;
        }
        case UPGRADE_MENU: {
            break;
        }
        case PLAYING: {
            if (!Mix_PlayingMusic() && gameMusic) Mix_PlayMusic(gameMusic, -1);
            gameTime += deltaTime;

            if (gameTime > LEVEL_DURATION * level) {
                gameState = PRE_LEVEL_UP;
                preLevelUpTimer = PRE_LEVEL_UP_DELAY;
                break;
            }

            const Uint8* keys = SDL_GetKeyboardState(nullptr);
            float speed = playerSpeed * deltaTime;
            bool moving = false;
            Animation* currentPlayerAnim = nullptr;
            float currentEnemySpeed = ENEMY_SPEED + (level - 1) * ENEMY_SPEED_INCREASE;

            player.vx = 0;
            player.vy = 0;

            if (keys[SDL_SCANCODE_W] && player.rect.y > 0) { player.vy = -speed; moving = true; }
            if (keys[SDL_SCANCODE_S] && player.rect.y + player.rect.h < SCREEN_HEIGHT) { player.vy = speed; moving = true; }
            if (keys[SDL_SCANCODE_A] && player.rect.x > 0) {
                player.vx = -speed;
                moving = true;
                if (player.playerState != ATTACK && player.playerState != HURT && player.playerState != DEAD)
                    playerAnim.walk.flip = SDL_FLIP_HORIZONTAL;
            }
            if (keys[SDL_SCANCODE_D] && player.rect.x + player.rect.w < SCREEN_WIDTH) {
                player.vx = speed;
                moving = true;
                if (player.playerState != ATTACK && player.playerState != HURT && player.playerState != DEAD)
                    playerAnim.walk.flip = SDL_FLIP_NONE;
            }

            if (moving && player.playerState != ATTACK && player.playerState != HURT && player.playerState != DEAD) {
                float length = std::sqrt(player.vx * player.vx + player.vy * player.vy);
                if (length > 0) {
                    player.vx = (player.vx / length) * speed;
                    player.vy = (player.vy / length) * speed;
                }
                player.rect.x += player.vx;
                player.rect.y += player.vy;
                player.updateHitbox();
                player.playerState = WALK;
            } else if (player.playerState != ATTACK && player.playerState != HURT && player.playerState != DEAD) {
                player.playerState = IDLE;
            }

            switch (player.playerState) {
                case IDLE: currentPlayerAnim = &playerAnim.idle; break;
                case WALK: currentPlayerAnim = &playerAnim.walk; break;
                case ATTACK: currentPlayerAnim = &playerAnim.attack; break;
                case HURT: currentPlayerAnim = &playerAnim.hurt; break;
                case DEAD: currentPlayerAnim = &playerAnim.dead; break;
            }
            if (currentPlayerAnim) {
                float frameTime = (player.playerState == ATTACK && !qReady) ? 0.015f : currentPlayerAnim->frameTime;
                currentPlayerAnim->frameTime = frameTime;
                updateAnimation(*currentPlayerAnim, deltaTime, player.playerState != DEAD && player.playerState != HURT);
                if ((player.playerState == ATTACK || player.playerState == HURT) &&
                    currentPlayerAnim->currentFrame == currentPlayerAnim->frames.size() - 1) {
                    player.playerState = IDLE;
                }
            }

            for (auto& enemy : enemies) {
                if (!enemy.active) continue;

                float dx = player.rect.x + player.rect.w/2 - (enemy.rect.x + enemy.rect.w/2);
                float dy = player.rect.y + player.rect.h/2 - (enemy.rect.y + enemy.rect.h/2);
                float length = std::sqrt(dx * dx + dy * dy);

                Animation* currentAnim = nullptr;
                switch (enemy.type) {
                    case BASIC:
                        if (enemy.enemyState == WALKING) currentAnim = &enemyBasicAnim.walking;
                        else if (enemy.enemyState == SLASHING) currentAnim = &enemyBasicAnim.slashing;
                        else if (enemy.enemyState == DYING) currentAnim = &enemyBasicAnim.dying;
                        break;
                    case FAST:
                        if (enemy.enemyState == WALKING) currentAnim = &enemyFastAnim.walking;
                        else if (enemy.enemyState == SLASHING) currentAnim = &enemyFastAnim.slashing;
                        else if (enemy.enemyState == DYING) currentAnim = &enemyFastAnim.dying;
                        break;
                    case CHASER:
                        if (enemy.enemyState == WALKING) currentAnim = &enemyChaserAnim.walking;
                        else if (enemy.enemyState == SLASHING) currentAnim = &enemyChaserAnim.slashing;
                        else if (enemy.enemyState == DYING) currentAnim = &enemyChaserAnim.dying;
                        break;
                }

                if (currentAnim && !currentAnim->frames.empty()) {
                    updateAnimation(*currentAnim, deltaTime, enemy.enemyState != DYING);
                    currentAnim->flip = (dx < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

                    if (enemy.enemyState == DYING && currentAnim->currentFrame == currentAnim->frames.size() - 1) {
                        enemy.active = false;
                        if (enemyDeathSound) Mix_PlayChannel(-1, enemyDeathSound, 0);
                        score += SCORE_PER_KILL * (combo + 1);
                        combo++;
                        comboTime = COMBO_TIMEOUT;
                    }
                }

                if (enemy.enemyState != DYING) {
                    if (length <= SLASHING_DISTANCE) {
                        if (enemy.enemyState != SLASHING) {
                            enemy.enemyState = SLASHING;
                            if (enemyAttackSound) Mix_PlayChannel(-1, enemyAttackSound, 0);
                            Animation* slashingAnim = nullptr;
                            switch (enemy.type) {
                                case BASIC: slashingAnim = &enemyBasicAnim.slashing; break;
                                case FAST: slashingAnim = &enemyFastAnim.slashing; break;
                                case CHASER: slashingAnim = &enemyChaserAnim.slashing; break;
                            }
                            if (slashingAnim) {
                                slashingAnim->currentFrame = 0;
                                slashingAnim->elapsedTime = 0.0f;
                            }
                        }
                    } else {
                        enemy.enemyState = WALKING;
                        float speedMultiplier = enemy.type == FAST ? 1.5f : (enemy.type == CHASER ? 0.8f : 1.0f);
                        float targetX = player.rect.x + player.rect.w / 2 - dx / length * SLASHING_DISTANCE;
                        float targetY = player.rect.y + player.rect.h / 2 - dy / length * SLASHING_DISTANCE;
                        float moveDx = targetX - (enemy.rect.x + enemy.rect.w / 2);
                        float moveDy = targetY - (enemy.rect.y + enemy.rect.h / 2);
                        float moveLength = std::sqrt(moveDx * moveDx + moveDy * moveDy);
                        if (moveLength > 0) {
                            enemy.rect.x += (moveDx / moveLength) * currentEnemySpeed * speedMultiplier * deltaTime;
                            enemy.rect.y += (moveDy / moveLength) * currentEnemySpeed * speedMultiplier * deltaTime;
                        }
                        enemy.updateHitbox();
                    }
                }

                if (enemy.hitEffectTimer > 0) {
                    enemy.hitEffectTimer -= deltaTime;
                    if (enemy.hitEffectTimer <= 0) enemy.hitEffectTimer = 0.0f;
                }

                if (enemy.attackCooldown > 0) {
                    enemy.attackCooldown -= deltaTime;
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
                        enemy.hitEffectTimer = 0.2f;
                        qReady = true;
                        qCooldown = 0;
                        if (enemy.health <= 0 && enemy.enemyState != DYING) {
                            enemy.enemyState = DYING;
                            enemy.vx = 0;
                            enemy.vy = 0;
                        }
                        break;
                    }
                }
            }

            for (auto& enemy : enemies) {
                if (!enemy.active || enemy.enemyState != SLASHING) continue;
                if (checkCollision(player.hitbox, enemy.hitbox) && enemy.attackCooldown <= 0) {
                    player.health -= 20;
                    switch (enemy.type) {
                        case BASIC: enemy.attackCooldown = 1.0f; break;
                        case FAST: enemy.attackCooldown = 0.5f; break;
                        case CHASER: enemy.attackCooldown = 0.8f; break;
                    }
                    lastDamageTime = gameTime;
                    if (player.health > 0 && player.playerState != DEAD && player.playerState != HURT) {
                        if (hurtSound) Mix_PlayChannel(-1, hurtSound, 0);
                        player.playerState = HURT;
                        player.animTime = 0.0f;
                        player.hurtTimer = 0.3f;
                        playerAnim.hurt.currentFrame = 0;
                        playerAnim.hurt.elapsedTime = 0.0f;
                    }
                    if (player.health <= 0 && player.playerState != DEAD) {
                        if (deathSound) Mix_PlayChannel(-1, deathSound, 0);
                        player.playerState = DEAD;
                        player.animTime = 0.0f;
                        deathTimer = playerAnim.dead.frames.size() * playerAnim.dead.frameTime + 1.0f;
                    }
                }
            }

            if (player.hurtTimer > 0) {
                player.hurtTimer -= deltaTime;
                if (player.hurtTimer <= 0) player.hurtTimer = 0.0f;
            }

            if (comboTime > 0) comboTime -= deltaTime;
            else combo = 0;

            if (!qReady) {
                qCooldown -= deltaTime;
                if (qCooldown <= 0) qReady = true;
            }

            for (auto& marker : markers) {
                marker.timer -= deltaTime;
                if (marker.isSpawnMarker && marker.timer <= 0) spawnEnemyAtMarker(marker.position);
            }
            markers.erase(std::remove_if(markers.begin(), markers.end(),
                [](const Marker& m) { return m.timer <= 0; }), markers.end());

            enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                [](const GameObject& e) { return !e.active; }), enemies.end());
            projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                [](const GameObject& p) { return !p.active; }), projectiles.end());

            if (player.health <= 0 && deathTimer > 0) {
                deathTimer -= deltaTime;
                if (deathTimer <= 0) gameState = GAME_OVER;
            }
            break;
        }
        case GAME_OVER: {
            if (!Mix_PlayingMusic() && gameMusic) Mix_PlayMusic(gameMusic, -1);
            deathTimer -= deltaTime;
            for (auto& enemy : enemies) {
                enemy.vx = 0;
                enemy.vy = 0;
            }
            break;
        }
    }
}

void renderButton(const Button& button) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    SDL_Rect shadowRect = {button.rect.x + 5, button.rect.y + 5, button.rect.w, button.rect.h};
    SDL_RenderFillRect(renderer, &shadowRect);

    SDL_SetRenderDrawColor(renderer, button.hovered ? 150 : 100, button.hovered ? 150 : 100, 50, 255);
    SDL_RenderFillRect(renderer, &button.rect);

    SDL_SetRenderDrawColor(renderer, button.hovered ? 255 : 200, button.hovered ? 255 : 200, 100, 255);
    SDL_RenderDrawRect(renderer, &button.rect);

    renderText(button.text, button.rect.x + 10, button.rect.y + 10, button.color);
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    switch (gameState) {
        case MENU: {
            if (menuBackground) SDL_RenderCopy(renderer, menuBackground, nullptr, nullptr);
            Button startButton = {{SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 60, 200, 40}, "Start Game", {0, 255, 127}, false};
            Button settingsButton = {{SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2, 200, 40}, "Settings", {255, 215, 0}, false};
            Button quitButton = {{SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 60, 200, 40}, "Quit", {255, 69, 0}, false};

            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            startButton.hovered = (mouseX >= startButton.rect.x && mouseX <= startButton.rect.x + startButton.rect.w &&
                                  mouseY >= startButton.rect.y && mouseY <= startButton.rect.y + startButton.rect.h);
            settingsButton.hovered = (mouseX >= settingsButton.rect.x && mouseX <= settingsButton.rect.x + settingsButton.rect.w &&
                                     mouseY >= settingsButton.rect.y && mouseY <= settingsButton.rect.y + settingsButton.rect.h);
            quitButton.hovered = (mouseX >= quitButton.rect.x && mouseX <= quitButton.rect.x + quitButton.rect.w &&
                                 mouseY >= quitButton.rect.y && mouseY <= quitButton.rect.y + quitButton.rect.h);

            renderText("Dodge And Q", SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT/2 - 150, {255, 215, 0});
            renderButton(startButton);
            renderButton(settingsButton);
            renderButton(quitButton);
            break;
        }
        case SETTINGS: {
            if (menuBackground) SDL_RenderCopy(renderer, menuBackground, nullptr, nullptr);
            renderText("Settings", SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT/2 - 200, {255, 215, 0, 255});

            // Music Volume Slider
            SDL_Rect musicBar = {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 100, 200, 20};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &musicBar);
            int musicFillWidth = (musicVolume * 200) / 128;
            SDL_Rect musicFill = {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 100, musicFillWidth, 20};
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &musicFill);
            SDL_Rect musicKnob = {SCREEN_WIDTH/2 - 100 + musicFillWidth - 5, SCREEN_HEIGHT/2 - 105, 10, 30};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &musicKnob);
            renderText("Music: " + std::to_string(musicVolume), SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT/2 - 130, {255, 255, 255, 255});

            // SFX Volume Slider
            SDL_Rect sfxBar = {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 40, 200, 20};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &sfxBar);
            int sfxFillWidth = (sfxVolume * 200) / 128;
            SDL_Rect sfxFill = {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 40, sfxFillWidth, 20};
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &sfxFill);
            SDL_Rect sfxKnob = {SCREEN_WIDTH/2 - 100 + sfxFillWidth - 5, SCREEN_HEIGHT/2 - 45, 10, 30};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &sfxKnob);
            renderText("SFX: " + std::to_string(sfxVolume), SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT/2 - 70, {255, 255, 255, 255});

            Button backButton = {{SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 60, 200, 40}, "Back", {255, 255, 255, 255}, false};

            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            backButton.hovered = (mouseX >= backButton.rect.x && mouseX <= backButton.rect.x + backButton.rect.w &&
                                 mouseY >= backButton.rect.y && mouseY <= backButton.rect.y + backButton.rect.h);

            renderButton(backButton);
            break;
        }
        case PLAYING:
        case PRE_LEVEL_UP:
        case LEVEL_UP: {
            if (maps[currentMap]) SDL_RenderCopy(renderer, maps[currentMap], nullptr, nullptr);

            for (const auto& marker : markers) {
                if (marker.isSpawnMarker) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    int size = 40;
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_RenderDrawLine(renderer, marker.position.x - size, marker.position.y - size,
                                       marker.position.x + size, marker.position.y + size);
                    SDL_RenderDrawLine(renderer, marker.position.x + size, marker.position.y - size,
                                       marker.position.x - size, marker.position.y + size);
                    SDL_RenderDrawLine(renderer, marker.position.x - size + 1, marker.position.y - size + 1,
                                       marker.position.x + size + 1, marker.position.y + size + 1);
                    SDL_RenderDrawLine(renderer, marker.position.x + size - 1, marker.position.y - size - 1,
                                       marker.position.x - size - 1, marker.position.y + size - 1);
                }
            }

            Animation* currentPlayerAnim = nullptr;
            switch (player.playerState) {
                case IDLE: currentPlayerAnim = &playerAnim.idle; break;
                case WALK: currentPlayerAnim = &playerAnim.walk; break;
                case ATTACK: currentPlayerAnim = &playerAnim.attack; break;
                case HURT: currentPlayerAnim = &playerAnim.hurt; break;
                case DEAD: currentPlayerAnim = &playerAnim.dead; break;
            }
            if (currentPlayerAnim && !currentPlayerAnim->textures.empty() && !currentPlayerAnim->frames.empty()) {
                SDL_Texture* currentTexture = currentPlayerAnim->textures[0]; // Player dùng sprite sheet, chỉ có 1 texture
                SDL_Rect* frame = &currentPlayerAnim->frames[currentPlayerAnim->currentFrame];
                SDL_FRect renderRect = {player.rect.x, player.rect.y, PLAYER_SIZE, PLAYER_SIZE};
                if (player.hurtTimer > 0) SDL_SetTextureColorMod(currentTexture, 255, 100, 100);
                else SDL_SetTextureColorMod(currentTexture, 255, 255, 255);
                SDL_RenderCopyExF(renderer, currentTexture, frame, &renderRect, 0, nullptr, currentPlayerAnim->flip);
            }

            for (const auto& enemy : enemies) {
                if (!enemy.active) continue;
                Animation* currentAnim = nullptr;
                switch (enemy.type) {
                    case BASIC:
                        if (enemy.enemyState == WALKING) currentAnim = &enemyBasicAnim.walking;
                        else if (enemy.enemyState == SLASHING) currentAnim = &enemyBasicAnim.slashing;
                        else if (enemy.enemyState == DYING) currentAnim = &enemyBasicAnim.dying;
                        break;
                    case FAST:
                        if (enemy.enemyState == WALKING) currentAnim = &enemyFastAnim.walking;
                        else if (enemy.enemyState == SLASHING) currentAnim = &enemyFastAnim.slashing;
                        else if (enemy.enemyState == DYING) currentAnim = &enemyFastAnim.dying;
                        break;
                    case CHASER:
                        if (enemy.enemyState == WALKING) currentAnim = &enemyChaserAnim.walking;
                        else if (enemy.enemyState == SLASHING) currentAnim = &enemyChaserAnim.slashing;
                        else if (enemy.enemyState == DYING) currentAnim = &enemyChaserAnim.dying;
                        break;
                }
                if (currentAnim && !currentAnim->textures.empty() && !currentAnim->frames.empty()) {
                    SDL_Texture* currentTexture = currentAnim->textures[currentAnim->currentFrame]; // Enemy dùng nhiều texture
                    SDL_Rect* frame = &currentAnim->frames[currentAnim->currentFrame];
                    SDL_FRect renderRect = {enemy.rect.x, enemy.rect.y, PLAYER_SIZE, PLAYER_SIZE};
                    if (enemy.hitEffectTimer > 0) SDL_SetTextureColorMod(currentTexture, 255, 0, 0);
                    else SDL_SetTextureColorMod(currentTexture, 255, 255, 255);
                    SDL_RenderCopyExF(renderer, currentTexture, frame, &renderRect, 0, nullptr, currentAnim->flip);
                }
            }

            for (const auto& proj : projectiles) {
                if (!proj.active) continue;
                if (projectileTexture) {
                    SDL_FRect projRect = {proj.rect.x, proj.rect.y, proj.rect.w, proj.rect.h};
                    SDL_RenderCopyExF(renderer, projectileTexture, nullptr, &projRect, proj.angle, nullptr, SDL_FLIP_NONE);
                }
            }

            int healthBarX = 10;
            int healthBarY = 10;
            int healthBarWidth = 200;
            int healthBarHeight = 20;
            float healthRatio = static_cast<float>(player.health) / MAX_HEALTH;
            if (healthRatio < 0) healthRatio = 0.0f;
            SDL_Rect healthBg = {healthBarX, healthBarY, healthBarWidth, healthBarHeight};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &healthBg);
            SDL_Rect healthFill = {healthBarX, healthBarY, static_cast<int>(healthBarWidth * healthRatio), healthBarHeight};
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &healthFill);
            SDL_Rect healthBorder = {healthBarX - 2, healthBarY - 2, healthBarWidth + 4, healthBarHeight + 4};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &healthBorder);

            int levelBarX = 10;
            int levelBarY = 40;
            int levelBarWidth = 200;
            int levelBarHeight = 15;
            float levelProgress = gameTime / (LEVEL_DURATION * level);
            SDL_Rect levelBg = {levelBarX, levelBarY, levelBarWidth, levelBarHeight};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &levelBg);
            SDL_Rect levelFill = {levelBarX, levelBarY, static_cast<int>(levelBarWidth * levelProgress), levelBarHeight};
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &levelFill);
            SDL_Rect levelBorder = {levelBarX - 2, levelBarY - 2, levelBarWidth + 4, levelBarHeight + 4};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &levelBorder);

            renderText("Score: " + std::to_string(score), 10, 70);
            renderText("Level: " + std::to_string(level), 10, 100);
            renderText("Upgrade Points: " + std::to_string(upgradePoints), 10, 130);

            renderText("Combo: " + std::to_string(combo), 10, 280);

            const int barWidth = 200;
            const int barHeight = 15;
            float cooldownRatio = qCooldown / qCooldownMax;
            SDL_Rect cooldownBg = {10, SCREEN_HEIGHT - 30, barWidth, barHeight};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &cooldownBg);
            SDL_Rect cooldownFill = {10, SCREEN_HEIGHT - 30, static_cast<int>(barWidth * (1 - cooldownRatio)), barHeight};
            SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
            SDL_RenderFillRect(renderer, &cooldownFill);
            SDL_Color qColor = qReady ? SDL_Color{0, 255, 0, 255} : SDL_Color{255, 0, 0, 255};
            renderText("[Q] Shoot", SCREEN_WIDTH - 120, SCREEN_HEIGHT - 30, qColor);

            if (gameState == PRE_LEVEL_UP) {
                renderText("Level Up in " + std::to_string((int)preLevelUpTimer + 1) + "s", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, {255, 255, 0});
            } else if (gameState == LEVEL_UP) {
                renderText("Level Up! Level " + std::to_string(level + 1), SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, {255, 255, 0});
            }
            break;
        }
        case UPGRADE_MENU: {
            if (maps[currentMap]) SDL_RenderCopy(renderer, maps[currentMap], nullptr, nullptr);

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
            SDL_RenderFillRect(renderer, nullptr);

            SDL_Rect upgradeBox = {SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 200, 400, 400};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &upgradeBox);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &upgradeBox);

            Button speedButton = {{SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 120, 300, 40}, "1: Increase Speed", {0, 255, 0}, false};
            Button cooldownButton = {{SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 60, 300, 40}, "2: Reduce Cooldown", {0, 255, 0}, false};
            Button damageButton = {{SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2, 300, 40}, "3: Increase Damage", {0, 255, 0}, false};
            Button shotgunButton = {{SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 60, 300, 40}, shotgunUnlocked ? "4: Shotgun (Taken)" : "4: Shotgun", shotgunUnlocked ? SDL_Color{100, 100, 100} : SDL_Color{0, 255, 0}, false};

            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            speedButton.hovered = (mouseX >= speedButton.rect.x && mouseX <= speedButton.rect.x + speedButton.rect.w &&
                                  mouseY >= speedButton.rect.y && mouseY <= speedButton.rect.y + speedButton.rect.h);
            cooldownButton.hovered = (mouseX >= cooldownButton.rect.x && mouseX <= cooldownButton.rect.x + cooldownButton.rect.w &&
                                     mouseY >= cooldownButton.rect.y && mouseY <= cooldownButton.rect.y + cooldownButton.rect.h);
            damageButton.hovered = (mouseX >= damageButton.rect.x && mouseX <= damageButton.rect.x + damageButton.rect.w &&
                                   mouseY >= damageButton.rect.y && mouseY <= damageButton.rect.y + damageButton.rect.h);
            shotgunButton.hovered = (!shotgunUnlocked && mouseX >= shotgunButton.rect.x && mouseX <= shotgunButton.rect.x + shotgunButton.rect.w &&
                                    mouseY >= shotgunButton.rect.y && mouseY <= shotgunButton.rect.y + shotgunButton.rect.h);

            renderText("Choose an Upgrade", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 160, {255, 215, 0});
            renderButton(speedButton);
            renderButton(cooldownButton);
            renderButton(damageButton);
            renderButton(shotgunButton);
            break;
        }
        case PAUSED: {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
            SDL_RenderFillRect(renderer, nullptr);

            SDL_Rect pauseBox = {SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 150, 300, 300};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &pauseBox);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &pauseBox);

            Button resumeButton = {{SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 60, 200, 40}, "Resume", {0, 255, 0}, false};
            Button settingsButton = {{SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2, 200, 40}, "Settings", {255, 215, 0}, false};
            Button quitButton = {{SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 60, 200, 40}, "Quit", {255, 0, 0}, false};

            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            resumeButton.hovered = (mouseX >= resumeButton.rect.x && mouseX <= resumeButton.rect.x + resumeButton.rect.w &&
                                   mouseY >= resumeButton.rect.y && mouseY <= resumeButton.rect.y + resumeButton.rect.h);
            settingsButton.hovered = (mouseX >= settingsButton.rect.x && mouseX <= settingsButton.rect.x + settingsButton.rect.w &&
                                     mouseY >= settingsButton.rect.y && mouseY <= settingsButton.rect.y + settingsButton.rect.h);
            quitButton.hovered = (mouseX >= quitButton.rect.x && mouseX <= quitButton.rect.x + quitButton.rect.w &&
                                 mouseY >= quitButton.rect.y && mouseY <= quitButton.rect.y + quitButton.rect.h);

            renderText("Paused", SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2 - 120, {255, 255, 0});
            renderButton(resumeButton);
            renderButton(settingsButton);
            renderButton(quitButton);
            break;
        }
        case GAME_OVER: {
            if (maps[currentMap]) SDL_RenderCopy(renderer, maps[currentMap], nullptr, nullptr);

            Animation* currentPlayerAnim = nullptr;
            switch (player.playerState) {
                case IDLE: currentPlayerAnim = &playerAnim.idle; break;
                case WALK: currentPlayerAnim = &playerAnim.walk; break;
                case ATTACK: currentPlayerAnim = &playerAnim.attack; break;
                case HURT: currentPlayerAnim = &playerAnim.hurt; break;
                case DEAD: currentPlayerAnim = &playerAnim.dead; break;
            }
            if (currentPlayerAnim && !currentPlayerAnim->textures.empty() && !currentPlayerAnim->frames.empty()) {
                SDL_Texture* currentTexture = currentPlayerAnim->textures[0]; // Player dùng sprite sheet
                SDL_Rect* frame = &currentPlayerAnim->frames[currentPlayerAnim->currentFrame];
                SDL_FRect renderRect = {player.rect.x, player.rect.y, PLAYER_SIZE, PLAYER_SIZE};
                SDL_RenderCopyExF(renderer, currentTexture, frame, &renderRect, 0, nullptr, currentPlayerAnim->flip);
            }

            for (const auto& enemy : enemies) {
                if (!enemy.active) continue;
                Animation* currentAnim = nullptr;
                switch (enemy.type) {
                    case BASIC: currentAnim = (enemy.enemyState == WALKING) ? &enemyBasicAnim.walking : (enemy.enemyState == SLASHING) ? &enemyBasicAnim.slashing : &enemyBasicAnim.dying; break;
                    case FAST: currentAnim = (enemy.enemyState == WALKING) ? &enemyFastAnim.walking : (enemy.enemyState == SLASHING) ? &enemyFastAnim.slashing : &enemyFastAnim.dying; break;
                    case CHASER: currentAnim = (enemy.enemyState == WALKING) ? &enemyChaserAnim.walking : (enemy.enemyState == SLASHING) ? &enemyChaserAnim.slashing : &enemyChaserAnim.dying; break;
                }
                if (currentAnim && !currentAnim->textures.empty() && !currentAnim->frames.empty()) {
                    SDL_Texture* currentTexture = currentAnim->textures[currentAnim->currentFrame]; // Enemy dùng nhiều texture
                    SDL_Rect* frame = &currentAnim->frames[currentAnim->currentFrame];
                    SDL_FRect renderRect = {enemy.rect.x, enemy.rect.y, PLAYER_SIZE, PLAYER_SIZE};
                    if (enemy.hitEffectTimer > 0) SDL_SetTextureColorMod(currentTexture, 255, 0, 0);
                    else SDL_SetTextureColorMod(currentTexture, 255, 255, 255);
                    SDL_RenderCopyExF(renderer, currentTexture, frame, &renderRect, 0, nullptr, currentAnim->flip);
                }
            }

            if (deathTimer <= 0) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
                SDL_RenderFillRect(renderer, nullptr);

                // Adjusted Game Over box size and position
                SDL_Rect deathBox = {SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 200, 400, 400};
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &deathBox);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &deathBox);

                Button restartButton = {{SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 60, 300, 40}, "Restart", {0, 255, 0, 255}, false};
                Button settingsButton = {{SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 20, 300, 40}, "Settings", {255, 215, 0, 255}, false};
                Button quitButton = {{SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 100, 300, 40}, "Quit", {255, 0, 0, 255}, false};

                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                restartButton.hovered = (mouseX >= restartButton.rect.x && mouseX <= restartButton.rect.x + restartButton.rect.w &&
                                        mouseY >= restartButton.rect.y && mouseY <= restartButton.rect.y + restartButton.rect.h);
                settingsButton.hovered = (mouseX >= settingsButton.rect.x && mouseX <= settingsButton.rect.x + settingsButton.rect.w &&
                                         mouseY >= settingsButton.rect.y && mouseY <= settingsButton.rect.y + settingsButton.rect.h);
                quitButton.hovered = (mouseX >= quitButton.rect.x && mouseX <= quitButton.rect.x + quitButton.rect.w &&
                                     mouseY >= quitButton.rect.y && mouseY <= quitButton.rect.y + quitButton.rect.h);

                renderText("Game Over", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 160, {255, 0, 0, 255});
                renderText("Score: " + std::to_string(score), SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 120, {255, 255, 255, 255});
                renderButton(restartButton);
                renderButton(settingsButton);
                renderButton(quitButton);
            }
            break;
        }
    }

    SDL_RenderPresent(renderer);
}

void clean() {
    for (auto texture : playerAnim.idle.textures) SDL_DestroyTexture(texture);
    for (auto texture : playerAnim.walk.textures) SDL_DestroyTexture(texture);
    for (auto texture : playerAnim.attack.textures) SDL_DestroyTexture(texture);
    for (auto texture : playerAnim.hurt.textures) SDL_DestroyTexture(texture);
    for (auto texture : playerAnim.dead.textures) SDL_DestroyTexture(texture);

    for (auto texture : enemyBasicAnim.walking.textures) SDL_DestroyTexture(texture);
    for (auto texture : enemyBasicAnim.slashing.textures) SDL_DestroyTexture(texture);
    for (auto texture : enemyBasicAnim.dying.textures) SDL_DestroyTexture(texture);
    for (auto texture : enemyFastAnim.walking.textures) SDL_DestroyTexture(texture);
    for (auto texture : enemyFastAnim.slashing.textures) SDL_DestroyTexture(texture);
    for (auto texture : enemyFastAnim.dying.textures) SDL_DestroyTexture(texture);
    for (auto texture : enemyChaserAnim.walking.textures) SDL_DestroyTexture(texture);
    for (auto texture : enemyChaserAnim.slashing.textures) SDL_DestroyTexture(texture);
    for (auto texture : enemyChaserAnim.dying.textures) SDL_DestroyTexture(texture);

    SDL_DestroyTexture(projectileTexture);
    SDL_DestroyTexture(menuBackground);
    for (int i = 0; i < NUM_MAPS; i++) SDL_DestroyTexture(maps[i]);

    Mix_FreeMusic(gameMusic);
    Mix_FreeChunk(shootSound);
    Mix_FreeChunk(hurtSound);
    Mix_FreeChunk(deathSound);
    Mix_FreeChunk(enemyAttackSound);
    Mix_FreeChunk(enemyDeathSound);
    Mix_FreeChunk(spawnSound);
    Mix_FreeChunk(levelUpSound);
    Mix_FreeChunk(upgradeSound);
    Mix_FreeChunk(clickSound);
    Mix_CloseAudio();

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

    Uint32 lastTime = SDL_GetTicks();
    bool running = true;
    SDL_Event event;

    if (gameMusic) Mix_PlayMusic(gameMusic, -1);

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (gameState) {
                    case MENU: {
                        if (event.key.keysym.sym == SDLK_s) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            resetGame();
                        }
                        if (event.key.keysym.sym == SDLK_q) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            running = false;
                        }
                        break;
                    }
                    case SETTINGS: {
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            gameState = previousState; // Quay lại trạng thái trước đó
                        }
                        break;
                    }
                    case PLAYING: {
                        if (event.key.keysym.sym == SDLK_q && qReady && player.playerState != HURT && player.playerState != DEAD) {
                            player.playerState = ATTACK;
                            player.animTime = 0.0f;
                            if (currentWeapon == SINGLE) shootProjectile();
                            else if (currentWeapon == SHOTGUN) shootShotgun();
                        }
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            gameState = PAUSED;
                        }
                        break;
                    }
                    case UPGRADE_MENU: {
                        if (event.key.keysym.sym == SDLK_1) applyUpgrade(1);
                        if (event.key.keysym.sym == SDLK_2) applyUpgrade(2);
                        if (event.key.keysym.sym == SDLK_3) applyUpgrade(3);
                        if (event.key.keysym.sym == SDLK_4 && !shotgunUnlocked) applyUpgrade(4);
                        break;
                    }
                    case PAUSED: {
                        if (event.key.keysym.sym == SDLK_r) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            gameState = PLAYING;
                        }
                        if (event.key.keysym.sym == SDLK_q) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            running = false;
                        }
                        break;
                    }
                    case GAME_OVER: {
                        if (deathTimer <= 0 && event.key.keysym.sym == SDLK_r) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            resetGame();
                        }
                        if (deathTimer <= 0 && event.key.keysym.sym == SDLK_q) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            running = false;
                        }
                        break;
                    }
                    case PRE_LEVEL_UP:
                    case LEVEL_UP: {
                        break;
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;
                switch (gameState) {
                    case MENU: {
                        if (mouseX >= SCREEN_WIDTH/2 - 100 && mouseX <= SCREEN_WIDTH/2 + 100) {
                            if (mouseY >= SCREEN_HEIGHT/2 - 60 && mouseY <= SCREEN_HEIGHT/2 - 20) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                resetGame();
                            }
                            if (mouseY >= SCREEN_HEIGHT/2 && mouseY <= SCREEN_HEIGHT/2 + 40) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                previousState = MENU; // Lưu trạng thái trước khi vào SETTINGS
                                gameState = SETTINGS;
                            }
                            if (mouseY >= SCREEN_HEIGHT/2 + 60 && mouseY <= SCREEN_HEIGHT/2 + 100) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                running = false;
                            }
                        }
                        break;
                    }
                    case SETTINGS: {
                        // Music Slider Knob
                        int musicKnobX = SCREEN_WIDTH/2 - 100 + (musicVolume * 200) / 128 - 5;
                        if (mouseX >= musicKnobX && mouseX <= musicKnobX + 10 &&
                            mouseY >= SCREEN_HEIGHT/2 - 105 && mouseY <= SCREEN_HEIGHT/2 - 75) {
                            draggingMusicSlider = true;
                        }
                        // SFX Slider Knob
                        int sfxKnobX = SCREEN_WIDTH/2 - 100 + (sfxVolume * 200) / 128 - 5;
                        if (mouseX >= sfxKnobX && mouseX <= sfxKnobX + 10 &&
                            mouseY >= SCREEN_HEIGHT/2 - 45 && mouseY <= SCREEN_HEIGHT/2 - 15) {
                            draggingSFXSlider = true;
                        }
                        // Back Button
                        if (mouseX >= SCREEN_WIDTH/2 - 100 && mouseX <= SCREEN_WIDTH/2 + 100 &&
                            mouseY >= SCREEN_HEIGHT/2 + 60 && mouseY <= SCREEN_HEIGHT/2 + 100) {
                            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                            gameState = previousState; // Quay lại trạng thái trước đó
                        }
                        break;
                    }
                    case UPGRADE_MENU: {
                        if (mouseX >= SCREEN_WIDTH/2 - 150 && mouseX <= SCREEN_WIDTH/2 + 150) {
                            if (mouseY >= SCREEN_HEIGHT/2 - 120 && mouseY <= SCREEN_HEIGHT/2 - 80) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                applyUpgrade(1);
                            }
                            if (mouseY >= SCREEN_HEIGHT/2 - 60 && mouseY <= SCREEN_HEIGHT/2 - 20) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                applyUpgrade(2);
                            }
                            if (mouseY >= SCREEN_HEIGHT/2 && mouseY <= SCREEN_HEIGHT/2 + 40) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                applyUpgrade(3);
                            }
                            if (!shotgunUnlocked && mouseY >= SCREEN_HEIGHT/2 + 60 && mouseY <= SCREEN_HEIGHT/2 + 100) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                applyUpgrade(4);
                            }
                        }
                        break;
                    }
                    case PAUSED: {
                        if (mouseX >= SCREEN_WIDTH/2 - 100 && mouseX <= SCREEN_WIDTH/2 + 100) {
                            if (mouseY >= SCREEN_HEIGHT/2 - 60 && mouseY <= SCREEN_HEIGHT/2 - 20) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                gameState = PLAYING;
                            }
                            if (mouseY >= SCREEN_HEIGHT/2 && mouseY <= SCREEN_HEIGHT/2 + 40) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                previousState = PAUSED; // Lưu trạng thái trước khi vào SETTINGS
                                gameState = SETTINGS;
                            }
                            if (mouseY >= SCREEN_HEIGHT/2 + 60 && mouseY <= SCREEN_HEIGHT/2 + 100) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                running = false;
                            }
                        }
                        break;
                    }
                    case GAME_OVER: {
                        if (deathTimer <= 0 && mouseX >= SCREEN_WIDTH/2 - 150 && mouseX <= SCREEN_WIDTH/2 + 150) {
                            if (mouseY >= SCREEN_HEIGHT/2 - 60 && mouseY <= SCREEN_HEIGHT/2 - 20) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                resetGame();
                            }
                            if (mouseY >= SCREEN_HEIGHT/2 + 20 && mouseY <= SCREEN_HEIGHT/2 + 60) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                previousState = GAME_OVER; // Lưu trạng thái trước khi vào SETTINGS
                                gameState = SETTINGS;
                            }
                            if (mouseY >= SCREEN_HEIGHT/2 + 100 && mouseY <= SCREEN_HEIGHT/2 + 140) {
                                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                                running = false;
                            }
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                draggingMusicSlider = false;
                draggingSFXSlider = false;
            }
            if (event.type == SDL_MOUSEMOTION && gameState == SETTINGS) {
                int mouseX = event.motion.x;
                if (draggingMusicSlider) {
                    musicVolume = ((mouseX - (SCREEN_WIDTH/2 - 100)) * 128) / 200;
                    if (musicVolume < 0) musicVolume = 0;
                    if (musicVolume > 128) musicVolume = 128;
                    Mix_VolumeMusic(musicVolume);
                }
                if (draggingSFXSlider) {
                    sfxVolume = ((mouseX - (SCREEN_WIDTH/2 - 100)) * 128) / 200;
                    if (sfxVolume < 0) sfxVolume = 0;
                    if (sfxVolume > 128) sfxVolume = 128;
                    Mix_VolumeChunk(shootSound, sfxVolume);
                    Mix_VolumeChunk(hurtSound, sfxVolume);
                    Mix_VolumeChunk(deathSound, sfxVolume);
                    Mix_VolumeChunk(enemyAttackSound, sfxVolume);
                    Mix_VolumeChunk(enemyDeathSound, sfxVolume);
                    Mix_VolumeChunk(spawnSound, sfxVolume);
                    Mix_VolumeChunk(levelUpSound, sfxVolume);
                    Mix_VolumeChunk(upgradeSound, sfxVolume);
                    Mix_VolumeChunk(clickSound, sfxVolume);
                }
            }
        }

        if (gameState == PLAYING && rand() % static_cast<int>(spawnRate) == 0) spawnEnemyMarker();

        update(deltaTime);
        render();
    }

    clean();
    return 0;
}
