#include <SDL.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace std;

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int EZ_SIZE = 50;
const float EZ_SPEED = 300.0f;
const int ENEMY_SIZE = 50;
const float ENEMY_SPEED = 150.0f;

struct Enemy {
    float x, y;

    Enemy(float startX, float startY) {
        x = startX;
        y = startY;
    }
};

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
vector<Enemy> enemies;

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

    SDL_ShowCursor(SDL_ENABLE);
    srand(time(0));
    return true;
}

void close() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void spawnEnemy() {
    float startX = rand() % SCREEN_WIDTH;
    float startY = 0; // Xuất hiện từ trên màn hình
    enemies.push_back(Enemy(startX, startY));
}

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    bool quit = false;
    SDL_Event e;

    float x = SCREEN_WIDTH / 2 - EZ_SIZE / 2;
    float y = SCREEN_HEIGHT / 2 - EZ_SIZE / 2;

    int lastTick = SDL_GetTicks();

    while (!quit) {
        int currentTick = SDL_GetTicks();
        float deltaTime = (currentTick - lastTick) / 1000.0f;
        lastTick = currentTick;

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
        }

        // Sinh kẻ địch ngẫu nhiên
        if (rand() % 100 < 2) {
            spawnEnemy();
        }

        // Di chuyển kẻ địch về phía Ez
        for (auto& enemy : enemies) {
            float dx = x - enemy.x;
            float dy = y - enemy.y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance > 0) {
                enemy.x += (dx / distance) * ENEMY_SPEED * deltaTime;
                enemy.y += (dy / distance) * ENEMY_SPEED * deltaTime;
            }
        }

        // Vẽ màn hình
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Vẽ Ez
        SDL_Rect player = {(int)x, (int)y, EZ_SIZE, EZ_SIZE};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &player);

        // Vẽ kẻ địch
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        for (auto& enemy : enemies) {
            SDL_Rect enemyRect = {(int)enemy.x, (int)enemy.y, ENEMY_SIZE, ENEMY_SIZE};
            SDL_RenderFillRect(renderer, &enemyRect);
        }

        SDL_RenderPresent(renderer);
    }

    close();
    return 0;
}
