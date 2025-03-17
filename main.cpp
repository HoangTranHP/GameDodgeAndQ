
#include <SDL.h>
#include <iostream>

using namespace std;

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;

int main(int argc, char *argv[]){

    //Khoi tao SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        cerr << "Loi! SDL Error: " << SDL_GetError() << "\n";
        return 1;
    }

    //Tao cua so
    SDL_Window *window = SDL_CreateWindow("DodgeAndQ", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (!window){
        cerr << "Loi! SDL Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Không thể tạo renderer! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int r = 0, g = 0, b = 0;

    bool quit = false;
    SDL_Event e;
    while(!quit){
        while(SDL_PollEvent(&e) != 0){
            if (e.type == SDL_QUIT){
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN){
                switch(e.key.keysym.sym){
                    case SDLK_SPACE:
                        r = rand() % 256;
                        g = rand() % 256;
                        b = rand() % 256;
                        break;
                }

            }
            else if (e.type == SDL_MOUSEBUTTONDOWN){
                switch(e.button.button){
                    case SDL_BUTTON_LEFT:
                        r = rand() % 256;
                        g = rand() % 256;
                        b = rand() % 256;
                        break;
                }
            }

        }

        SDL_SetRenderDrawColor(renderer, r , g , b , 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    //Giai phong tai nguyen
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;

}
