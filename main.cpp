#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <iostream>
#include <cmath> // For distance calculation

// Screen settings
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PLAYER_WIDTH = 50;
const int PLAYER_HEIGHT = 50;
const int MONSTER_WIDTH = 50;
const int MONSTER_HEIGHT = 50;
const int GROUND_Y = SCREEN_HEIGHT - 100;
const int FRAME_TIME = 100;

// Animation states
enum AnimationState { IDLE, WALKING_RIGHT, WALKING_LEFT };

// Player structure
struct Player {
    int x, y;
    int velocityX;
    int velocityY;
    bool onGround;
    AnimationState state;
    SDL_Texture* texture;
    int currentFrame;
    Uint32 lastFrameTime;
};

// Monster structure
struct Entity {
    int x, y;
    SDL_Texture* texture;
};

// Load texture function
SDL_Texture* loadTexture(const char* path, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Render ground with repeating texture
void renderGround(SDL_Renderer* renderer, SDL_Texture* texture) {
    SDL_Rect src = { 0, 0, 50, 50 };  // Crop 50x50 from texture
    SDL_Rect dest = { 0, GROUND_Y, 50, 50 }; // Tile size

    for (int y = GROUND_Y; y < SCREEN_HEIGHT; y += 50) {
        for (int x = 0; x < SCREEN_WIDTH; x += 50) {
            dest.x = x;
            dest.y = y;
            SDL_RenderCopy(renderer, texture, &src, &dest);
        }
    }
}

void renderBackground(SDL_Renderer* renderer, SDL_Texture* texture) {
    SDL_Rect dest = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderCopy(renderer, texture, NULL, &dest);
}

// Update player animation
void updateAnimation(Player& player, int totalFrames) {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime > player.lastFrameTime + FRAME_TIME) {
        player.lastFrameTime = currentTime;
        player.currentFrame = (player.currentFrame + 1) % totalFrames;
    }
}

// Calculate distance between player and monster
float calculateDistance(int x1, int y1, int x2, int y2) {
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image initialization failed: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer initialization failed: " << Mix_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Vengine!",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        std::cerr << "Failed to create window/renderer: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    Player player = { 50, GROUND_Y - PLAYER_HEIGHT, 0, 0, true, IDLE, nullptr, 0, 0 };
    player.texture = loadTexture("player_spritesheet.png", renderer);
    if (!player.texture) return 1;

    Entity monster = { SCREEN_WIDTH - MONSTER_WIDTH - 50, GROUND_Y - MONSTER_HEIGHT, nullptr };
    monster.texture = loadTexture("enemy.png", renderer);
    if (!monster.texture) {
        std::cerr << "Failed to load monster texture!" << std::endl;
        return 1;
    }

    SDL_Texture* groundTexture = loadTexture("ground.png", renderer);
    if (!groundTexture) {
        std::cerr << "Failed to load ground texture!" << std::endl;
        return 1;
    }

    SDL_Texture* backgroundTexture = loadTexture("background.png", renderer);
    if (!backgroundTexture) {
        std::cerr << "Failed to load background texture!" << std::endl;
        return 1;
    }

    // Load jump sound
    Mix_Chunk* jumpSound = Mix_LoadWAV("jump.wav");
    if (!jumpSound) {
        std::cerr << "Failed to load jump sound: " << Mix_GetError() << std::endl;
        return 1;
    }
    Mix_VolumeChunk(jumpSound, MIX_MAX_VOLUME / 3);

    bool running = true;
    bool dead = false;
    bool paused = false;
    SDL_Event event;
    int totalFrames = 4;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_x && paused && dead) {
                    player.x = 50;
                    player.y = GROUND_Y - PLAYER_HEIGHT;
                    monster.x = SCREEN_WIDTH - MONSTER_WIDTH - 50;
                    paused = false;
                    dead = false;
                }

                if (event.key.keysym.sym == SDLK_p && !dead) {
                    paused = !paused;
                }
            }
        }

        if (!paused) {
            const Uint8* state = SDL_GetKeyboardState(NULL);
            player.velocityX = 0;

            if (state[SDL_SCANCODE_LEFT]) {
                player.velocityX = -5;
                player.state = WALKING_LEFT;
            }
            else if (state[SDL_SCANCODE_RIGHT]) {
                player.velocityX = 5;
                player.state = WALKING_RIGHT;
            }
            else {
                player.state = IDLE;
            }

            if (state[SDL_SCANCODE_UP] && player.onGround) { 
                player.velocityY = -15;
                player.onGround = false;
                Mix_PlayChannel(-1, jumpSound, 0); // 🔊 Play jump sound
            }

            player.velocityY += 1;
            player.x += player.velocityX;
            player.y += player.velocityY;

            if (player.x < 0) player.x = 0;
            if (player.x > SCREEN_WIDTH - PLAYER_WIDTH) player.x = SCREEN_WIDTH - PLAYER_WIDTH;
            if (player.y >= GROUND_Y - PLAYER_HEIGHT) {
                player.y = GROUND_Y - PLAYER_HEIGHT;
                player.velocityY = 0;
                player.onGround = true;
            }

            if (monster.x > player.x) monster.x -= 2;
            if (monster.x < player.x) monster.x += 2;

            if (calculateDistance(player.x, player.y, monster.x, monster.y) < 50) {
                paused = true;
                dead = true;
            }

            updateAnimation(player, totalFrames);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        renderBackground(renderer, backgroundTexture);
        renderGround(renderer, groundTexture);

        SDL_Rect currentFrame = { player.currentFrame * PLAYER_WIDTH, player.state * PLAYER_HEIGHT, PLAYER_WIDTH, PLAYER_HEIGHT };
        SDL_Rect dest = { player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT };
        SDL_RenderCopy(renderer, player.texture, &currentFrame, &dest);

        SDL_Rect monsterDest = { monster.x, monster.y, MONSTER_WIDTH, MONSTER_HEIGHT };
        SDL_RenderCopy(renderer, monster.texture, NULL, &monsterDest);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    Mix_FreeChunk(jumpSound);
    Mix_CloseAudio();
    SDL_DestroyTexture(player.texture);
    SDL_DestroyTexture(monster.texture);
    SDL_DestroyTexture(groundTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
