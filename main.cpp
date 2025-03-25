#include <SDL.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

using namespace std;

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int EZ_SIZE = 50;
const float EZ_SPEED = 300.0f;
const int ENEMY_SIZE = 40;
const int BULLET_SIZE = 10;
const float BULLET_SPEED = 500.0f;
const float ENEMY_SPEED = 150.0f;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

struct GameObject {
    float x, y;
    float vx, vy;
    bool active;
};

vector<GameObject> enemies;
vector<GameObject> bullets;

float ezX = SCREEN_WIDTH / 2 - EZ_SIZE / 2;
float ezY = SCREEN_HEIGHT / 2 - EZ_SIZE / 2;
// Thêm biến để theo dõi vị trí đích khi di chuyển
float targetX = ezX;
float targetY = ezY;
bool isMoving = false;
int cursorX, cursorY;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "Lỗi! SDL Error: " << SDL_GetError() << "\n";
        return false;
    }
    window = SDL_CreateWindow("DodgeAndQ", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        cerr << "Lỗi tạo cửa sổ! SDL Error: " << SDL_GetError() << endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        cerr << "Lỗi tạo renderer! SDL Error: " << SDL_GetError() << endl;
        return false;
    }
    SDL_ShowCursor(SDL_DISABLE);
    return true;
}

void spawnEnemy() {
    GameObject enemy;
    enemy.x = rand() % SCREEN_WIDTH;
    enemy.y = rand() % SCREEN_HEIGHT;
    enemy.active = true;
    enemies.push_back(enemy);
}

void shootBullet() {
    // Tính toán vị trí trung tâm của nhân vật
    float playerCenterX = ezX + EZ_SIZE / 2;
    float playerCenterY = ezY + EZ_SIZE / 2;

    // Tính toán hướng đi của đạn dựa trên trung tâm nhân vật thay vì góc trên bên trái
    float dx = cursorX - playerCenterX;
    float dy = cursorY - playerCenterY;
    float len = sqrt(dx * dx + dy * dy);

    if (len == 0) return;

    // Tạo đạn từ vị trí trung tâm của nhân vật
    GameObject bullet = {
        playerCenterX - BULLET_SIZE / 2,  // Điều chỉnh để trung tâm đạn nằm ở trung tâm nhân vật
        playerCenterY - BULLET_SIZE / 2,
        (dx / len) * BULLET_SPEED,
        (dy / len) * BULLET_SPEED,
        true
    };

    bullets.push_back(bullet);
}

void update(float deltaTime) {
    // Cập nhật vị trí nhân vật (di chuyển mượt mà đến vị trí đích)
    if (isMoving) {
        float dx = targetX - ezX;
        float dy = targetY - ezY;
        float distance = sqrt(dx * dx + dy * dy);

        // Nếu đã gần đến đích, đặt vị trí chính xác và dừng di chuyển
        if (distance < EZ_SPEED * deltaTime) {
            ezX = targetX;
            ezY = targetY;
            isMoving = false;
        } else {
            // Di chuyển theo hướng của đích với tốc độ EZ_SPEED
            float moveX = dx / distance * EZ_SPEED * deltaTime;
            float moveY = dy / distance * EZ_SPEED * deltaTime;
            ezX += moveX;
            ezY += moveY;
        }
    }

    // Cập nhật vị trí viên đạn
    for (auto &bullet : bullets) {
        if (bullet.active) {
            bullet.x += bullet.vx * deltaTime;
            bullet.y += bullet.vy * deltaTime;
            if (bullet.x < 0 || bullet.x > SCREEN_WIDTH || bullet.y < 0 || bullet.y > SCREEN_HEIGHT) {
                bullet.active = false;
            }
        }
    }

    // Cập nhật kẻ địch
    for (auto &enemy : enemies) {
        if (enemy.active) {
            float dx = ezX - enemy.x;
            float dy = ezY - enemy.y;
            float len = sqrt(dx * dx + dy * dy);
            enemy.vx = (dx / len) * ENEMY_SPEED;
            enemy.vy = (dy / len) * ENEMY_SPEED;
            enemy.x += enemy.vx * deltaTime;
            enemy.y += enemy.vy * deltaTime;
        }
    }

    // Kiểm tra va chạm
    for (auto &bullet : bullets) {
        for (auto &enemy : enemies) {
            if (bullet.active && enemy.active) {
                if (abs(bullet.x - enemy.x) < ENEMY_SIZE / 2 && abs(bullet.y - enemy.y) < ENEMY_SIZE / 2) {
                    bullet.active = false;
                    enemy.active = false;
                }
            }
        }
    }

    bullets.erase(remove_if(bullets.begin(), bullets.end(), [](GameObject b) { return !b.active; }), bullets.end());
    enemies.erase(remove_if(enemies.begin(), enemies.end(), [](GameObject e) { return !e.active; }), enemies.end());
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Rect player = {(int)ezX, (int)ezY, EZ_SIZE, EZ_SIZE};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &player);

    // Hiển thị đường đi khi đang di chuyển
    if (isMoving) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
        SDL_RenderDrawLine(renderer, ezX + EZ_SIZE/2, ezY + EZ_SIZE/2, targetX + EZ_SIZE/2, targetY + EZ_SIZE/2);
    }

    for (auto &enemy : enemies) {
        SDL_Rect rect = {(int)enemy.x, (int)enemy.y, ENEMY_SIZE, ENEMY_SIZE};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
    }

    for (auto &bullet : bullets) {
        SDL_Rect rect = {(int)bullet.x, (int)bullet.y, BULLET_SIZE, BULLET_SIZE};
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
    }

    SDL_Rect cursor = {cursorX - 5, cursorY - 5, 10, 10};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &cursor);

    SDL_RenderPresent(renderer);
}

void close() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    if (!init()) return 1;
    bool quit = false;
    SDL_Event e;
    int lastTick = SDL_GetTicks();

    while (!quit) {
        int currentTick = SDL_GetTicks();
        float deltaTime = (currentTick - lastTick) / 1000.0f;
        lastTick = currentTick;

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_MOUSEMOTION) {
                SDL_GetMouseState(&cursorX, &cursorY);
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
                // Thay vì dịch chuyển ngay, chỉ cập nhật vị trí đích và bật cờ di chuyển
                targetX = cursorX - EZ_SIZE / 2;
                targetY = cursorY - EZ_SIZE / 2;
                isMoving = true;
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q) {
                shootBullet();
            }
        }

        if (rand() % 100000 < 2) spawnEnemy();

        update(deltaTime);
        render();
    }
    close();
    return 0;
}
