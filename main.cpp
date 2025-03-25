#include <SDL.h>
#include <iostream>
#include <cmath>

using namespace std;

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int EZ_SIZE = 50;
const float EZ_SPEED = 300.0f;  // Tốc độ di chuyển (pixel/giây)

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

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
    return true;
}

void close() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    bool quit = false;
    SDL_Event e;

    float x = SCREEN_WIDTH / 2 - EZ_SIZE / 2;
    float y = SCREEN_HEIGHT / 2 - EZ_SIZE / 2;

    int targetX = x, targetY = y;
    bool moving = false;

    int lastTick = SDL_GetTicks();

    while (!quit) {
        int currentTick = SDL_GetTicks();
        float deltaTime = (currentTick - lastTick) / 1000.0f;  // Thời gian giữa 2 frame
        lastTick = currentTick;

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_RIGHT) {  // Click chuột phải
                    SDL_GetMouseState(&targetX, &targetY);
                    moving = true;
                }
            }
        }

        // Nếu đang di chuyển, tiến dần về đích
        if (moving) {
            float dx = targetX - x;
            float dy = targetY - y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance > 5) {  // Nếu chưa tới đích thì di chuyển
                x += (dx / distance) * EZ_SPEED * deltaTime;
                y += (dy / distance) * EZ_SPEED * deltaTime;
            } else {
                moving = false;  // Đến nơi thì dừng
            }
        }

        // Giữ nhân vật trong màn hình
        if (x < 0) x = 0;
        if (x > SCREEN_WIDTH - EZ_SIZE) x = SCREEN_WIDTH - EZ_SIZE;
        if (y < 0) y = 0;
        if (y > SCREEN_HEIGHT - EZ_SIZE) y = SCREEN_HEIGHT - EZ_SIZE;

        // Vẽ màn hình
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect player = {(int)x, (int)y, EZ_SIZE, EZ_SIZE};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &player);

        SDL_RenderPresent(renderer);
    }

    close();
    return 0;
}
