#include <SDL.h>
#include <iostream>
#include <cmath>

using namespace std;

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int EZ_SIZE = 80;
const float EZ_SPEED = 500.0f;  // Tốc độ cố định (pixel/giây)
const int FPS = 60;
const int frameDelay = 1000 / FPS;

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

    Uint32 frameStart;
    float deltaTime = 0.0f;

    while (!quit) {
        frameStart = SDL_GetTicks();  // Lưu thời gian bắt đầu khung hình

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    SDL_GetMouseState(&targetX, &targetY);
                    moving = true;
                }
            }
        }

        // Di chuyển dựa trên deltaTime
        if (moving) {
            float dx = targetX - x;
            float dy = targetY - y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance > 5) {
                x += (dx / distance) * EZ_SPEED * deltaTime;
                y += (dy / distance) * EZ_SPEED * deltaTime;
            } else {
                moving = false; // Dừng khi đến gần vị trí click
            }
        }

        // Vẽ màn hình
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect rect = { (int)x, (int)y, EZ_SIZE, EZ_SIZE };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);

        SDL_RenderPresent(renderer);

        // Điều chỉnh tốc độ khung hình
        int frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }

        // Cập nhật deltaTime sau khi điều chỉnh tốc độ
        deltaTime = (SDL_GetTicks() - frameStart) / 1000.0f;
    }

    close();
    return 0;
}
