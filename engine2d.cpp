// engine2d.cpp
// Compile: g++ engine2d.cpp -lSDL2 -std=c++17 -O2 -o engine2d
// Run: ./engine2d

#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>

// ----- Configurações -----
const int WINDOW_W = 800;
const int WINDOW_H = 600;
const char* WINDOW_TITLE = "Motor 2D - Revisado";

// ----- Componentes -----
struct Vec2 { float x, y; };
struct Transform {
    Vec2 pos{0,0};
    float rot{0};
    Vec2 scale{1,1};
};
struct Sprite {
    SDL_Texture* tex = nullptr;
    int w=0, h=0;
};
struct Rigidbody {
    Vec2 vel{0,0};
    float mass{1.0f};
};

// ----- Entidades -----
using Entity = int;
struct EntityManager {
    Entity nextId = 1;
    std::vector<Entity> entities;
    std::unordered_map<Entity, Transform> transforms;
    std::unordered_map<Entity, Sprite> sprites;
    std::unordered_map<Entity, Rigidbody> bodies;

    Entity create() {
        Entity e = nextId++;
        entities.push_back(e);
        transforms[e] = Transform();
        return e;
    }

    void destroyAll() {
        entities.clear();
        transforms.clear();
        sprites.clear();
        bodies.clear();
        nextId = 1;
    }
};

// ----- Recursos -----
struct ResourceManager {
    SDL_Renderer* renderer = nullptr;
    std::unordered_map<std::string, SDL_Texture*> textures;

    SDL_Texture* loadTexture(const std::string& path) {
        if (textures.count(path)) return textures[path];
        SDL_Surface* surf = SDL_LoadBMP(path.c_str());
        if (!surf) {
            std::cerr << "Erro ao carregar BMP: " << SDL_GetError() << " (" << path << ")\n";
            return nullptr;
        }
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (!tex) {
            std::cerr << "Erro ao criar textura: " << SDL_GetError() << "\n";
            return nullptr;
        }
        textures[path] = tex;
        return tex;
    }

    void cleanup() {
        for (auto& p : textures) SDL_DestroyTexture(p.second);
        textures.clear();
    }
};

// ----- Input -----
struct InputState {
    bool quit = false;
    float axisX = 0.0f;
    float axisY = 0.0f;
    bool left = false, right = false, up = false, down = false;
};

void processInput(InputState& in) {
    SDL_Event e;
    in.axisX = in.axisY = 0;
    in.left = in.right = in.up = in.down = false;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) in.quit = true;
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            bool down = (e.type == SDL_KEYDOWN);
            switch (e.key.keysym.sym) {
                case SDLK_a: in.left = down; break;
                case SDLK_d: in.right = down; break;
                case SDLK_w: in.up = down; break;
                case SDLK_s: in.down = down; break;
                case SDLK_ESCAPE: if (down) in.quit = true; break;
            }
        }
    }
    if (in.left) in.axisX -= 1.0f;
    if (in.right) in.axisX += 1.0f;
    if (in.up) in.axisY -= 1.0f;
    if (in.down) in.axisY += 1.0f;
}

// ----- Sistemas -----
void physicsSystem(EntityManager& em, float dt) {
    for (auto e : em.entities) {
        if (!em.bodies.count(e) || !em.transforms.count(e)) continue;
        Rigidbody& rb = em.bodies[e];
        Transform& t = em.transforms[e];
        t.pos.x += rb.vel.x * dt;
        t.pos.y += rb.vel.y * dt;
        rb.vel.x *= 0.98f;
        rb.vel.y *= 0.98f;
    }
}

void playerControlSystem(Entity player, EntityManager& em, const InputState& in, float speed) {
    if (!em.bodies.count(player)) return;
    Rigidbody& rb = em.bodies[player];
    rb.vel.x += in.axisX * speed;
    rb.vel.y += in.axisY * speed;
}

void renderSystem(SDL_Renderer* renderer, EntityManager& em) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
    SDL_RenderClear(renderer);

    for (auto e : em.entities) {
        if (!em.sprites.count(e) || !em.transforms.count(e)) continue;
        Sprite& sp = em.sprites[e];
        Transform& tr = em.transforms[e];
        if (!sp.tex) continue;

        SDL_Rect dst;
        dst.w = int(sp.w * tr.scale.x);
        dst.h = int(sp.h * tr.scale.y);
        dst.x = int(std::round(tr.pos.x - dst.w / 2));
        dst.y = int(std::round(tr.pos.y - dst.h / 2));
        SDL_RenderCopyEx(renderer, sp.tex, nullptr, &dst, tr.rot, nullptr, SDL_FLIP_NONE);
    }

    SDL_RenderPresent(renderer);
}

// ----- Main -----
int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "Erro SDL_Init: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Erro SDL_CreateWindow: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Erro SDL_CreateRenderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    EntityManager em;
    ResourceManager resources; resources.renderer = renderer;

    // Criar player
    Entity player = em.create();
    em.transforms[player].pos = {WINDOW_W / 2.0f, WINDOW_H / 2.0f};
    em.transforms[player].scale = {1.0f, 1.0f};
    em.bodies[player] = Rigidbody{{0,0}, 1.0f};

    // Carregar sprite
    SDL_Texture* tex = resources.loadTexture("player.bmp");
    if (!tex) {
        SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, 64, 64, 32, SDL_PIXELFORMAT_RGBA32);
        if (surf) {
            SDL_FillRect(surf, nullptr, SDL_MapRGB(surf->format, 200, 80, 80));
            tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_FreeSurface(surf);
        }
    }
    em.sprites[player] = Sprite{tex, 64, 64};

    // Objetos estáticos
    for (int i = 0; i < 5; i++) {
        Entity e = em.create();
        em.transforms[e].pos = {100.0f + i * 120.0f, 150.0f + (i % 2) * 80.0f};
        em.transforms[e].scale = {0.8f, 0.8f};
        em.sprites[e] = em.sprites[player];
    }

    InputState input;
    const float TARGET_DT = 1.0f / 60.0f;
    Uint64 prev = SDL_GetPerformanceCounter();
    float accumulator = 0.0f;

    while (!input.quit) {
        Uint64 now = SDL_GetPerformanceCounter();
        double elapsed = (now - prev) / (double)SDL_GetPerformanceFrequency();
        prev = now;
        accumulator += (float)elapsed;

        processInput(input);

        while (accumulator >= TARGET_DT) {
            playerControlSystem(player, em, input, 200.0f * TARGET_DT);
            physicsSystem(em, TARGET_DT);
            accumulator -= TARGET_DT;
        }

        renderSystem(renderer, em);
    }

    resources.cleanup();
    em.destroyAll();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}