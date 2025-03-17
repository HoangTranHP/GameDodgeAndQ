#include <SDL.h>
#include <iostream>

using namespace std;

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int EZ_SIZE = 80;
const int EZ_SPEED = 15;


SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

bool init(){
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        cerr << "Lỗi! SDL Error: " << SDL_GetError() << "\n";
        return false;
    }

    window = SDL_CreateWindow("DodgeAndQ", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        cerr << "Lỗi tạo cửa sổ! SDL Error: " << SDL_GetError() << endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        cerr << "Lỗi tạo renderer! SDL Error: " << SDL_GetError() << endl;
        return false;
    }

    return true;
}

void close(){
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *argv[]){
    if (!init()){
        return 1;
    }

    bool quit = false;
    SDL_Event e;

    int x = SCREEN_WIDTH/2 - EZ_SIZE/2;
    int y = SCREEN_HEIGHT/2 - EZ_SIZE/2;

    while(!quit){
        while(SDL_PollEvent(&e) != 0){
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_KEYDOWN){
                switch(e.key.keysym.sym){
                    case SDLK_UP: y -= EZ_SPEED; break;
                    case SDLK_RIGHT: x += EZ_SPEED; break;
                    case SDLK_LEFT: x -= EZ_SPEED; break;
                    case SDLK_DOWN: y += EZ_SPEED; break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);

        SDL_Rect rect = {x ,y, EZ_SIZE, EZ_SIZE};
        SDL_SetRenderDrawColor(renderer, 255,0,0,255);
        SDL_RenderFillRect(renderer, &rect);

        SDL_RenderPresent(renderer);
    }

    close();
    return 0;
}
