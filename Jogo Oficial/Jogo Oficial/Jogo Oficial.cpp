#include <stdio.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h> 

#define SCREEN_W 800
#define SCREEN_H 600
#define FPS 60.0

// 
enum GameState { STATE_PLAYING, STATE_QUESTION, STATE_GAMEOVER, STATE_NEXT_PHASE, STATE_PHASE_2, STATE_QUESTION_2, STATE_PHASE_3, STATE_QUESTION_3 };
enum { UP, DOWN, LEFT, RIGHT };

typedef struct {
    float x, y;
    float dx, dy;
    float width, height;
    bool is_jumping;
    float speed;
    float jump_power;
    int hp;
} Player;

typedef struct {
    float x, y;
    float width, height;
    float speed;
    int dir;
} Villain;

typedef struct {
    const char* text;
    const char* options[3];
    int correct_option;
} Question;

bool check_collision(float x1, float y1, float w1, float h1,
    float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 &&
        y1 < y2 + h2 && y1 + h1 > y2);
}

int main(void) {
    // Inicialização
    if (!al_init() || !al_install_keyboard() || !al_init_font_addon() || !al_init_image_addon()) {
        fprintf(stderr, "Falha ao inicializar subsistemas do Allegro.\n");
        return -1;
    }

    al_init_primitives_addon();

    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
    ALLEGRO_FONT* font = al_create_builtin_font();

    if (!display || !timer || !event_queue || !font) {
        fprintf(stderr, "Falha ao alocar recursos visuais/eventos.\n");
        return -1;
    }

    // Carregamento das Imagens (incluindo o terceiro cenário)
    ALLEGRO_BITMAP* img_cenario1 = al_load_bitmap("cenario.png");
    ALLEGRO_BITMAP* img_cenario2 = al_load_bitmap("edito pixilart.png");
    ALLEGRO_BITMAP* img_cenario3 = al_load_bitmap("grecia_antiga.png"); 
    ALLEGRO_BITMAP* img_jogador = al_load_bitmap("jogador.png");
    ALLEGRO_BITMAP* img_vilao = al_load_bitmap("vilao.png");

    if (!img_cenario1 || !img_cenario2 || !img_cenario3 || !img_jogador || !img_vilao) {
        fprintf(stderr, "ATENCAO: Alguma imagem nao foi encontrada!\n");
    }

    // Variáveis de ambiente
    const float floor_y = 460.0f; // altura do chão com a ponte
    const float gravity = 0.8f;
    const float goal_x = SCREEN_W - 90.0f;

    // Inicialização do Jogador
    Player player = {
        .x = 20.0f,
        .y = floor_y - 100.0f,
        .dx = 0, .dy = 0,
        .width = 80, .height = 120,
        .is_jumping = false,
        .speed = 5.0f, .jump_power = -18.0f,
        .hp = 3
    };

    // Inicialização do Inimigo
    Villain enemy = {
        .x = SCREEN_W / 2.0f,
        .y = floor_y - 110.0f,
        .width = 100, .height = 95,
        .speed = 4.0f, .dir = -1
    };

    // Perguntas da Fase 1 (Civilização Maia)
    Question trivia_phase1[2] = {
        { "Em qual região geográfica a civilização maia se desenvolveu?",{ "1. Península de Iucatã", "2. Norte da América do Norte", "3. Região Andina" }, 0 },
        { "Qual era o principal produto agrícola cultivado pelos maias, considerado sagrado?",{ "1. Batata", "2. Trigo", "3. Milho" }, 2 }
    };

    // Perguntas da Fase 2 (Egito Antigo)
    Question trivia_phase2[2] = {
        { "Qual rio permitiu o florescimento da civilização egípcia no deserto?",{ "1. Rio Amazonas", "2. Rio Nilo", "3. Rio Tigre" }, 1 },
        { "Qual era a principal finalidade das monumentais pirâmides no Egito Antigo?",{ "1. Torres de vigia", "2. Estoque para guardar grãos", "3. Tumbas para os faraós" }, 2 }
    };

    // Perguntas da Fase 3 (Grécia Antiga)
    Question trivia_phase3[2] = {
        { "Qual cidade-estado grega é considerada o berço da democracia?",{ "1. Esparta", "2. Atenas", "3. Troia" }, 1 },
        { "Qual famosa competição esportiva teve origem na Grécia Antiga?",{ "1. Jogos Olímpicos", "2. Torneios de gladiadores", "3. Corridas de bigas romanas" }, 0 }
    };

    int current_state = STATE_PLAYING;
    int current_question = 0;
    int correct_answers = 0;

    bool keys[4] = { false };

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    al_start_timer(timer);
    bool running = true;
    bool redraw = true;

    // Loop Principal
    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(event_queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            running = false;
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) running = false;

            if (current_state == STATE_PLAYING || current_state == STATE_PHASE_2 || current_state == STATE_PHASE_3) {
                switch (ev.keyboard.keycode) {
                case ALLEGRO_KEY_UP: keys[UP] = true; break;
                case ALLEGRO_KEY_DOWN: keys[DOWN] = true; break;
                case ALLEGRO_KEY_LEFT: keys[LEFT] = true; break;
                case ALLEGRO_KEY_RIGHT: keys[RIGHT] = true; break;
                }
            }
            // Resposta 1 (Fase 1)
            else if (current_state == STATE_QUESTION) {
                if (ev.keyboard.keycode >= ALLEGRO_KEY_1 && ev.keyboard.keycode <= ALLEGRO_KEY_3) {
                    int answer = ev.keyboard.keycode - ALLEGRO_KEY_1;
                    if (answer == trivia_phase1[current_question].correct_option) {
                        correct_answers++;
                        current_question++;
                        if (correct_answers == 2) {
                            current_state = STATE_PHASE_2; // Avança para a Fase 2
                            player.x = 20.0f;
                            correct_answers = 0;
                            current_question = 0;
                        }
                    }
                    else {
                        player.hp--;
                        player.x = 50.0f;
                        correct_answers = 0;
                        current_question = 0;
                        current_state = player.hp > 0 ? STATE_PLAYING : STATE_GAMEOVER;
                    }
                }
            }
            // Resposta 2 (Fase 2)
            else if (current_state == STATE_QUESTION_2) {
                if (ev.keyboard.keycode >= ALLEGRO_KEY_1 && ev.keyboard.keycode <= ALLEGRO_KEY_3) {
                    int answer = ev.keyboard.keycode - ALLEGRO_KEY_1;
                    if (answer == trivia_phase2[current_question].correct_option) {
                        correct_answers++;
                        current_question++;
                        if (correct_answers == 2) {
                            current_state = STATE_PHASE_3; // Avança para a Fase 3
                            player.x = 20.0f;
                            correct_answers = 0;
                            current_question = 0;
                        }
                    }
                    else {
                        player.hp--;
                        player.x = 20.0f;
                        correct_answers = 0;
                        current_question = 0;
                        current_state = player.hp > 0 ? STATE_PHASE_2 : STATE_GAMEOVER;
                    }
                }
            }
            // Resposta 3 (Fase 3)
            else if (current_state == STATE_QUESTION_3) {
                if (ev.keyboard.keycode >= ALLEGRO_KEY_1 && ev.keyboard.keycode <= ALLEGRO_KEY_3) {
                    int answer = ev.keyboard.keycode - ALLEGRO_KEY_1;
                    if (answer == trivia_phase3[current_question].correct_option) {
                        correct_answers++;
                        current_question++;
                        if (correct_answers == 2) {
                            current_state = STATE_NEXT_PHASE; // Jogo Concluído!
                        }
                    }
                    else {
                        player.hp--;
                        player.x = 20.0f;
                        correct_answers = 0;
                        current_question = 0;
                        current_state = player.hp > 0 ? STATE_PHASE_3 : STATE_GAMEOVER;
                    }
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            if (current_state == STATE_PLAYING || current_state == STATE_PHASE_2 || current_state == STATE_PHASE_3) {
                switch (ev.keyboard.keycode) {
                case ALLEGRO_KEY_UP: keys[UP] = false; break;
                case ALLEGRO_KEY_DOWN: keys[DOWN] = false; break;
                case ALLEGRO_KEY_LEFT: keys[LEFT] = false; break;
                case ALLEGRO_KEY_RIGHT: keys[RIGHT] = false; break;
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            if (current_state == STATE_PLAYING || current_state == STATE_PHASE_2 || current_state == STATE_PHASE_3) {
                // Movimento do Jogador
                if (keys[LEFT]) player.x -= player.speed;
                if (keys[RIGHT]) player.x += player.speed;

                if (keys[UP] && !player.is_jumping) {
                    player.dy = player.jump_power;
                    player.is_jumping = true;
                }

                player.dy += gravity;
                player.y += player.dy;

                // Chão Limite
                if (player.y + player.height >= floor_y) {
                    player.y = floor_y - player.height;
                    player.dy = 0;
                    player.is_jumping = false;
                }
                if (player.x < 0) player.x = 0;
                if (player.x + player.width > SCREEN_W) player.x = SCREEN_W - player.width;

                // Inimigo
                enemy.x += enemy.speed * enemy.dir;
                if (enemy.x <= 100 || enemy.x + enemy.width >= goal_x - 20) {
                    enemy.dir *= -1;
                }

                // Colisão Jogador e Inimigo
                if (check_collision(player.x, player.y, player.width, player.height,
                    enemy.x, enemy.y, enemy.width, enemy.height)) {
                    player.hp--;
                    player.x = 50.0f;
                    if (player.hp <= 0) current_state = STATE_GAMEOVER;
                }

                // Objetivo Invisível no fim da fase
                if (player.x >= goal_x) {
                    if (current_state == STATE_PLAYING) {
                        current_state = STATE_QUESTION;     // Abre perguntas da Fase 1
                    }
                    else if (current_state == STATE_PHASE_2) {
                        current_state = STATE_QUESTION_2;   // Abre perguntas da Fase 2
                    }
                    else if (current_state == STATE_PHASE_3) {
                        current_state = STATE_QUESTION_3;   // Abre perguntas da Fase 3
                    }
                    keys[LEFT] = keys[RIGHT] = keys[UP] = false;
                }
            }
            redraw = true;
        }

        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;

            // Fundo com a fase certa
            if (current_state == STATE_PHASE_2 || current_state == STATE_QUESTION_2) {
                if (img_cenario2) {
                    al_draw_scaled_bitmap(img_cenario2,
                        0, 0, al_get_bitmap_width(img_cenario2), al_get_bitmap_height(img_cenario2),
                        0, 0, SCREEN_W, SCREEN_H,
                        0);
                }
                else {
                    al_clear_to_color(al_map_rgb(20, 40, 20));
                }
            }
            else if (current_state == STATE_PHASE_3 || current_state == STATE_QUESTION_3) {
                if (img_cenario3) {
                    al_draw_scaled_bitmap(img_cenario3,
                        0, 0, al_get_bitmap_width(img_cenario3), al_get_bitmap_height(img_cenario3),
                        0, 0, SCREEN_W, SCREEN_H,
                        0);
                }
                else {
                    al_clear_to_color(al_map_rgb(40, 20, 40));
                }
            }
            else {
                if (img_cenario1) {
                    al_draw_scaled_bitmap(img_cenario1,
                        0, 0, al_get_bitmap_width(img_cenario1), al_get_bitmap_height(img_cenario1),
                        0, 0, SCREEN_W, SCREEN_H,
                        0);
                }
                else {
                    al_clear_to_color(al_map_rgb(30, 30, 30));
                    al_draw_filled_rectangle(0, floor_y, SCREEN_W, SCREEN_H, al_map_rgb(120, 120, 120));
                }
            }

            if (current_state == STATE_PLAYING || current_state == STATE_PHASE_2 || current_state == STATE_PHASE_3) {
                // Inimigo 
                if (img_vilao) {
                    al_draw_scaled_bitmap(img_vilao,
                        0, 0, al_get_bitmap_width(img_vilao), al_get_bitmap_height(img_vilao),
                        enemy.x, enemy.y, enemy.width, enemy.height,
                        0);
                }
                else {
                    al_draw_filled_rectangle(enemy.x, enemy.y, enemy.x + enemy.width, enemy.y + enemy.height, al_map_rgb(200, 0, 0));
                }

                // Jogador 
                if (img_jogador) {
                    al_draw_scaled_bitmap(img_jogador,
                        0, 0, al_get_bitmap_width(img_jogador), al_get_bitmap_height(img_jogador),
                        player.x, player.y, player.width, player.height,
                        0);
                }
                else {
                    al_draw_filled_rectangle(player.x, player.y, player.x + player.width, player.y + player.height, al_map_rgb(0, 200, 0));
                }

                // Vidas
                char hud[32];
                snprintf(hud, sizeof(hud), "Vidas: %d", player.hp);
                al_draw_text(font, al_map_rgb(255, 255, 255), 10, 10, 0, hud);
            }
            // Perguntas da Fase 1
            else if (current_state == STATE_QUESTION) {
                al_draw_filled_rectangle(0, 150, SCREEN_W, 350, al_map_rgba(0, 0, 0, 200));
                al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W / 2, 200, ALLEGRO_ALIGN_CENTER, trivia_phase1[current_question].text);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 250, ALLEGRO_ALIGN_CENTER, trivia_phase1[current_question].options[0]);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 280, ALLEGRO_ALIGN_CENTER, trivia_phase1[current_question].options[1]);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 310, ALLEGRO_ALIGN_CENTER, trivia_phase1[current_question].options[2]);
            }
            // Perguntas da Fase 2
            else if (current_state == STATE_QUESTION_2) {
                al_draw_filled_rectangle(0, 150, SCREEN_W, 350, al_map_rgba(0, 0, 0, 200));
                al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W / 2, 200, ALLEGRO_ALIGN_CENTER, trivia_phase2[current_question].text);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 250, ALLEGRO_ALIGN_CENTER, trivia_phase2[current_question].options[0]);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 280, ALLEGRO_ALIGN_CENTER, trivia_phase2[current_question].options[1]);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 310, ALLEGRO_ALIGN_CENTER, trivia_phase2[current_question].options[2]);
            }
            // Perguntas da Fase 3
            else if (current_state == STATE_QUESTION_3) {
                al_draw_filled_rectangle(0, 150, SCREEN_W, 350, al_map_rgba(0, 0, 0, 200));
                al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W / 2, 200, ALLEGRO_ALIGN_CENTER, trivia_phase3[current_question].text);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 250, ALLEGRO_ALIGN_CENTER, trivia_phase3[current_question].options[0]);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 280, ALLEGRO_ALIGN_CENTER, trivia_phase3[current_question].options[1]);
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 310, ALLEGRO_ALIGN_CENTER, trivia_phase3[current_question].options[2]);
            }
            else if (current_state == STATE_NEXT_PHASE) {
                al_draw_text(font, al_map_rgb(0, 255, 0), SCREEN_W / 2, SCREEN_H / 2, ALLEGRO_ALIGN_CENTER, "Parabens! Você venceu!");
            }
            else if (current_state == STATE_GAMEOVER) {
                al_draw_text(font, al_map_rgb(255, 0, 0), SCREEN_W / 2, SCREEN_H / 2, ALLEGRO_ALIGN_CENTER, "GAME OVER!");
            }

            al_flip_display();
        }
    }

    // Limpeza de memória
    if (img_cenario1) al_destroy_bitmap(img_cenario1);
    if (img_cenario2) al_destroy_bitmap(img_cenario2);
    if (img_cenario3) al_destroy_bitmap(img_cenario3);
    if (img_jogador) al_destroy_bitmap(img_jogador);
    if (img_vilao) al_destroy_bitmap(img_vilao);

    al_destroy_font(font);
    al_destroy_event_queue(event_queue);
    al_destroy_timer(timer);
    al_destroy_display(display);

    return 0;
}