#ifndef DEBUG_H
#define DEBUG_H

#include "def.h"

#include "SDL_FontCache.h"

class GameBoy;

class Debug {
public:
    Debug(GameBoy* gb, SDL_Renderer* renderer);
    ~Debug();

    void draw(int x, int y);

public:
    GameBoy* m_gb;
    SDL_Renderer* m_renderer;
    FC_Font* m_font;
};

#endif