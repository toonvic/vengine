#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <iostream>
#include <cmath> // Para calcular distância entre jogador e monstro

// Definições da tela
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PLAYER_WIDTH = 50;
const int PLAYER_HEIGHT = 50;
const int MONSTRO_WIDTH = 50;
const int MONSTRO_HEIGHT = 50;
const int CHAO_Y = SCREEN_HEIGHT - 100; // Posição do chão
const int FRAME_TIME = 100; // Tempo entre frames (milissegundos)

// Estados da animação
enum EstadoAnimacao { IDLE, ANDANDO_DIREITA, ANDANDO_ESQUERDA };

// Estrutura do jogador
struct Jogador {
    int x, y;
    int velocidadeX;
    int velocidadeY;
    bool noChao;
    EstadoAnimacao estado;
    SDL_Texture* textura;
    int frameAtual;
    Uint32 tempoUltimoFrame;
};

// Estrutura do monstro
struct Entidade {
    int x, y;
    SDL_Texture* textura;
};

// Função para carregar textura
SDL_Texture* carregarTextura(const char* caminho, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(caminho);
    if (!surface) {
        std::cerr << "Erro ao carregar imagem: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* textura = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return textura;
}

// Atualizar animação do jogador
void atualizarAnimacao(Jogador& jogador, int totalFrames) {
    Uint32 tempoAtual = SDL_GetTicks();
    if (tempoAtual > jogador.tempoUltimoFrame + FRAME_TIME) {
        jogador.tempoUltimoFrame = tempoAtual;
        jogador.frameAtual = (jogador.frameAtual + 1) % totalFrames;
    }
}

// Calcular distância entre jogador e monstro
float calcularDistancia(int x1, int y1, int x2, int y2) {
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

int main(int argc, char* argv[]) {
    // Inicializar SDL e SDL_image
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Erro ao inicializar SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "Erro ao inicializar SDL_image: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Criar janela e renderizador
    SDL_Window* window = SDL_CreateWindow("Fuja do Monstro!",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        std::cerr << "Erro ao criar janela/renderizador: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Criar jogador
    Jogador jogador = { 50, CHAO_Y - PLAYER_HEIGHT, 0, 0, true, IDLE, nullptr, 0, 0 };
    jogador.textura = carregarTextura("player_spritesheet.png", renderer);
    if (!jogador.textura) return 1;

    // Criar monstro
    Entidade monstro = { SCREEN_WIDTH - MONSTRO_WIDTH - 50, CHAO_Y - MONSTRO_HEIGHT, nullptr };
    monstro.textura = carregarTextura("enemy.png", renderer);
    if (!monstro.textura) {
        std::cerr << "Erro ao carregar a textura do monstro!" << std::endl;
        return 1;
    }

    bool rodando = true;
    bool pausado = false;
    SDL_Event evento;
    int totalFrames = 4; // Número de frames por animação

    while (rodando) {
        // Processar eventos
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                rodando = false;
            } else if (evento.type == SDL_KEYDOWN) {
                if (evento.key.keysym.sym == SDLK_x && pausado) {
                    // Reiniciar jogo ao pressionar "X"
                    jogador.x = 50;
                    jogador.y = CHAO_Y - PLAYER_HEIGHT;
                    monstro.x = SCREEN_WIDTH - MONSTRO_WIDTH - 50;
                    pausado = false;
                }
            }
        }

        if (!pausado) {
            // Capturar teclas pressionadas
            const Uint8* state = SDL_GetKeyboardState(NULL);
            jogador.velocidadeX = 0;

            if (state[SDL_SCANCODE_LEFT]) {
                jogador.velocidadeX = -5;
                jogador.estado = ANDANDO_ESQUERDA;
            }
            else if (state[SDL_SCANCODE_RIGHT]) {
                jogador.velocidadeX = 5;
                jogador.estado = ANDANDO_DIREITA;
            }
            else {
                jogador.estado = IDLE;
            }

            if (state[SDL_SCANCODE_UP] && jogador.noChao) { 
                jogador.velocidadeY = -15; // Velocidade para cima (pulo)
                jogador.noChao = false; // Não está mais no chão
            }

            // Atualizar posição e gravidade
            jogador.velocidadeY += 1;
            jogador.x += jogador.velocidadeX;
            jogador.y += jogador.velocidadeY;

            // Limites da tela e chão
            if (jogador.x < 0) jogador.x = 0;
            if (jogador.x > SCREEN_WIDTH - PLAYER_WIDTH) jogador.x = SCREEN_WIDTH - PLAYER_WIDTH;
            if (jogador.y >= CHAO_Y - PLAYER_HEIGHT) {
                jogador.y = CHAO_Y - PLAYER_HEIGHT;
                jogador.velocidadeY = 0;
                jogador.noChao = true;
            }

            // Monstro persegue jogador
            if (monstro.x > jogador.x) monstro.x -= 2;
            if (monstro.x < jogador.x) monstro.x += 2;

            // Checa colisão
            if (calcularDistancia(jogador.x, jogador.y, monstro.x, monstro.y) < 50) {
                pausado = true;
            }

            // Atualiza a animação do jogador
            atualizarAnimacao(jogador, totalFrames);
        }

        // Renderização
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Fundo preto
        SDL_RenderClear(renderer);

        // Desenhar chão (retângulo cinza)
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_Rect chao = { 0, CHAO_Y, SCREEN_WIDTH, 100 };
        SDL_RenderFillRect(renderer, &chao);

        // Desenhar jogador com animação
        SDL_Rect frameAtual = { jogador.frameAtual * PLAYER_WIDTH, jogador.estado * PLAYER_HEIGHT, PLAYER_WIDTH, PLAYER_HEIGHT };
        SDL_Rect destino = { jogador.x, jogador.y, PLAYER_WIDTH, PLAYER_HEIGHT };
        SDL_RenderCopy(renderer, jogador.textura, &frameAtual, &destino);

        // Desenhar monstro
        SDL_Rect destinoMonstro = { monstro.x, monstro.y, MONSTRO_WIDTH, MONSTRO_HEIGHT };
        SDL_RenderCopy(renderer, monstro.textura, NULL, &destinoMonstro);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Limpeza
    SDL_DestroyTexture(jogador.textura);
    SDL_DestroyTexture(monstro.textura);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
