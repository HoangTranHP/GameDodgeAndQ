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

enum EnemyType { BASIC, FAST, CHASER };

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

std::vector<GameObject> enemies;
std::vector<GameObject> projectiles;
std::vector<Marker> markers;
GameObject player;
int score = 0;
int combo = 0;
float comboTime = 0;
float qCooldown = 0;
bool qReady = true;
SDL_FPoint targetPos = {SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f};

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

void shootProjectile() {
    if (!qReady) return;

    GameObject proj;
    proj.rect.w = PROJECTILE_SIZE;
    proj.rect.h = PROJECTILE_SIZE;
    proj.rect.x = player.rect.x + player.rect.w/2 - PROJECTILE_SIZE/2;
    proj.rect.y = player.rect.y + player.rect.h/2 - PROJECTILE_SIZE/2;

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    float dx = mouseX - proj.rect.x;
    float dy = mouseY - proj.rect.y;
    float length = std::sqrt(dx*dx + dy*dy);

    if (length > 0) {
        proj.vx = (dx / length) * PROJECTILE_SPEED;
        proj.vy = (dy / length) * PROJECTILE_SPEED;
    }

    proj.active = true;
    projectiles.push_back(proj);
    qReady = false;
    qCooldown = Q_COOLDOWN;
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

void update(float deltaTime) {
    // Di chuyển player đến vị trí click
    float dx = targetPos.x - player.rect.x;
    float dy = targetPos.y - player.rect.y;
    float distance = std::sqrt(dx*dx + dy*dy);

    if (distance > 5.0f) {
        player.rect.x += (dx / distance) * PLAYER_SPEED * deltaTime;
        player.rect.y += (dy / distance) * PLAYER_SPEED * deltaTime;
    }

    // Cập nhật marker
    for (auto& marker : markers) {
        marker.timer -= deltaTime;
    }
    markers.erase(std::remove_if(markers.begin(), markers.end(),
        [](const Marker& m) { return m.timer <= 0; }), markers.end());

    // Cập nhật enemy
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;

        float dx = player.rect.x - enemy.rect.x;
        float dy = player.rect.y - enemy.rect.y;
        float length = std::sqrt(dx*dx + dy*dy);

        switch(enemy.type) {
            case BASIC:
                enemy.rect.x += (dx / length) * ENEMY_SPEED * deltaTime;
                enemy.rect.y += (dy / length) * ENEMY_SPEED * deltaTime;
                break;
            case FAST:
                enemy.rect.x += (dx / length) * (ENEMY_SPEED * 1.5f) * deltaTime;
                enemy.rect.y += (dy / length) * (ENEMY_SPEED * 1.5f) * deltaTime;
                break;
            case CHASER:
                enemy.rect.x += (dx / length) * (ENEMY_SPEED * 0.8f) * deltaTime;
                enemy.rect.y += (dy / length) * (ENEMY_SPEED * 0.8f) * deltaTime;
                break;
        }
    }

    // Cập nhật đạn
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        proj.rect.x += proj.vx * deltaTime;
        proj.rect.y += proj.vy * deltaTime;
        if (proj.rect.x < 0 || proj.rect.x > SCREEN_WIDTH || proj.rect.y < 0 || proj.rect.y > SCREEN_HEIGHT)
            proj.active = false;
    }

    // Va chạm đạn - enemy
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;

            if (checkCollision(proj.rect, enemy.rect)) {
                proj.active = false;
                enemy.health--;

                // Reset cooldown ngay khi chạm enemy
                qReady = true;
                qCooldown = 0;

                if (enemy.health <= 0) {
                    enemy.active = false;
                    comboTime = COMBO_TIMEOUT;
                    combo++;
                    score += SCORE_PER_KILL * combo;
                }
            }
        }
    }

    // Va chạm player-enemy
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;

        if (checkCollision(player.rect, enemy.rect)) {
            std::cout << "Game Over! Score: " << score << std::endl;
            SDL_Quit();
            exit(0);
        }
    }

    // Combo system
    if (comboTime > 0) comboTime -= deltaTime;
    else combo = 0;

    // Cooldown Q
    if (!qReady) {
        qCooldown -= deltaTime;
        if (qCooldown <= 0) {
            qReady = true;
            qCooldown = 0;
        }
    }

    // Cleanup
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const GameObject& e) { return !e.active; }), enemies.end());
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](const GameObject& p) { return !p.active; }), projectiles.end());
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Vẽ player
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_FRect playerRect = player.rect;
    SDL_RenderFillRectF(renderer, &playerRect);

    // Vẽ marker di chuyển
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
    for (const auto& marker : markers) {
        SDL_FRect markRect = {marker.position.x - 5, marker.position.y - 5, 10, 10};
        SDL_RenderFillRectF(renderer, &markRect);
    }

    // Vẽ enemy
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

    // Vẽ đạn
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    for (const auto& proj : projectiles) {
        if (!proj.active) continue;
        SDL_FRect bulletRect = proj.rect;
        SDL_RenderFillRectF(renderer, &bulletRect);
    }

    // UI
    renderText("Score: " + std::to_string(score), 10, 10);

    // Thanh cooldown Q
    const int barWidth = 200;
    const int barHeight = 15;
    float cooldownRatio = qCooldown / Q_COOLDOWN;

    SDL_Rect cooldownBg = {10, SCREEN_HEIGHT - 30, barWidth, barHeight};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &cooldownBg);

    SDL_Rect cooldownFill = {10, SCREEN_HEIGHT - 30,
                            static_cast<int>(barWidth * (1 - cooldownRatio)), barHeight};
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderFillRect(renderer, &cooldownFill);

    // Hiển thị trạng thái Q
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
                   SCREEN_HEIGHT - 50.0f - PLAYER_SIZE / 2.0f,
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

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q) {
                shootProjectile();
            }
        }

        if (rand() % 120 == 0) spawnEnemy();
        update(deltaTime);
        render();
        SDL_Delay(16);
    }

    clean();
    return 0;
}
