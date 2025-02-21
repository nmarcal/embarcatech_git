#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h> 
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"

// Definições dos pinos
#define LED_RED_PIN       13    // Pino do LED vermelho (GPIO 13)
#define LED_GREEN_PIN     11    // Pino do LED verde (GPIO 11)
#define LED_BLUE_PIN      12    // Pino do LED azul (GPIO 12)
#define BUZZER_PIN        10    // Pino do buzzer (GPIO 10)
#define BUTTON_A_PIN       5    // Pino do botão A (GPIO 5)
#define BUTTON_B_PIN       6    // Pino do botão B (GPIO 6)
#define JOYSTICK_X_PIN    26    // Pino do eixo X do joystick (GPIO 26, ADC0)
#define JOYSTICK_Y_PIN    27    // Pino do eixo Y do joystick (GPIO 27, ADC1)
#define I2C_SDA           14    // Pino SDA do I2C (GPIO 14)
#define I2C_SCL           15    // Pino SCL do I2C (GPIO 15)

// Frequências dos sons (em Hz)
#define NOTE_C4  2000  // Dó
#define NOTE_D4  2500  // Ré
#define NOTE_E4  3000  // Mi
#define NOTE_F4  3500  // Fá
#define NOTE_G4  4000  // Sol (som de início)
#define NOTE_A4  4500  // Lá (som de erro)

// Duração dos sons (em ms)
#define NOTE_DURATION 200

// Estados das cores
typedef enum {
    MAGENTA, 
    GREEN,
    BLUE,
    YELLOW,
    NUM_COLORS  // Número total de cores
} ColorState;

// Variáveis globais para PWM
uint slice_num;
uint channel;

// Variável global para o número de rodadas
int total_rounds = 1;  // Número de rodadas escolhido pelo usuário

// Função para acender o LED RGB com base no estado
void set_rgb_color(ColorState color) {
    switch (color) {
        case MAGENTA:
            gpio_put(LED_RED_PIN, 1);   // Vermelho
            gpio_put(LED_GREEN_PIN, 0); // Verde desligado
            gpio_put(LED_BLUE_PIN, 1);  // Azul (combina com vermelho para magenta)
            break;
        case GREEN:
            gpio_put(LED_RED_PIN, 0);   // Vermelho desligado
            gpio_put(LED_GREEN_PIN, 1); // Verde
            gpio_put(LED_BLUE_PIN, 0);  // Azul desligado
            break;
        case BLUE:
            gpio_put(LED_RED_PIN, 0);   // Vermelho desligado
            gpio_put(LED_GREEN_PIN, 0); // Verde desligado
            gpio_put(LED_BLUE_PIN, 1);  // Azul
            break;
        case YELLOW:
            gpio_put(LED_RED_PIN, 1);   // Vermelho
            gpio_put(LED_GREEN_PIN, 1); // Verde (combina com vermelho para amarelo)
            gpio_put(LED_BLUE_PIN, 0);  // Azul desligado
            break;
        default:
            gpio_put(LED_RED_PIN, 0);   // Desliga todos os LEDs
            gpio_put(LED_GREEN_PIN, 0);
            gpio_put(LED_BLUE_PIN, 0);
            break;
    }
}

// Função para tocar um tom no buzzer
void play_tone(uint32_t frequency, uint32_t duration_ms) {
    uint32_t clock = 125000000;  // Clock do sistema (125 MHz)
    uint32_t wrap = clock / frequency;  // Valor de "wrap" para o PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f);  // Divisor de clock = 1
    pwm_config_set_wrap(&config, wrap);
    pwm_init(slice_num, &config, true);

    // Define o nível do PWM para 30% (volume mais baixo)
    pwm_set_chan_level(slice_num, channel, wrap * 0.3);

    // Mantém o som pelo tempo especificado
    sleep_ms(duration_ms);

    // Desliga o buzzer
    pwm_set_chan_level(slice_num, channel, 0);
}

// Função para tocar o som correspondente à cor
void play_color_sound(ColorState color) {
    switch (color) {
        case MAGENTA:
            play_tone(NOTE_C4, NOTE_DURATION);  // Dó
            break;
        case GREEN:
            play_tone(NOTE_D4, NOTE_DURATION);  // Ré
            break;
        case BLUE:
            play_tone(NOTE_E4, NOTE_DURATION);  // Mi
            break;
        case YELLOW:
            play_tone(NOTE_F4, NOTE_DURATION);  // Fá
            break;
        default:
            break;
    }
}

// Função para gerar uma sequência incremental de cores
void generate_sequence(ColorState sequence[], int length) {
    if (length == 1) {
        // Na primeira rodada, gera uma única cor aleatória
        sequence[0] = rand() % NUM_COLORS;
    } else {
        // Nas rodadas subsequentes, mantém a sequência anterior e adiciona uma nova cor
        sequence[length - 1] = rand() % NUM_COLORS;
    }
}

// Função para mostrar a sequência de cores e sons no LED RGB
void show_sequence(ColorState sequence[], int length) {
    for (int i = 0; i < length; i++) {
        set_rgb_color(sequence[i]);  // Mostra a cor no LED RGB
        play_color_sound(sequence[i]);  // Toca o som correspondente
        sleep_ms(500);  // Intervalo entre as cores
        set_rgb_color(NUM_COLORS);  // Desliga o LED RGB
        sleep_ms(200);  // Intervalo entre as cores
    }
}

// Função para verificar a sequência do jogador
bool check_sequence(ColorState sequence[], ColorState player_sequence[], int length) {
    for (int i = 0; i < length; i++) {
        if (sequence[i] != player_sequence[i]) {
            return false;  // Sequência incorreta
        }
    }
    return true;  // Sequência correta
}

// Função para piscar os LEDs e tocar sons como feedback de vitória
void victory_animation() {
    ColorState victory_colors[] = {MAGENTA, GREEN, BLUE, YELLOW};  // Sequência de cores da vitória
    for (int i = 0; i < 3; i++) {  // Repete a animação 3 vezes
        for (int j = 0; j < 4; j++) {  // Percorre as 4 cores
            set_rgb_color(victory_colors[j]);  // Mostra a cor
            play_color_sound(victory_colors[j]);  // Toca o som correspondente
            sleep_ms(100);  // Intervalo entre as cores (reduzido para 100 ms)
            set_rgb_color(NUM_COLORS);  // Desliga o LED
            sleep_ms(100);  // Intervalo entre as cores (reduzido para 100 ms)
        }
    }
}

// Função para animação de erro
void error_animation() {
    for (int i = 0; i < 3; i++) {  // Repete a animação 3 vezes
        gpio_put(LED_RED_PIN, 1);   // Acende o LED vermelho
        gpio_put(LED_GREEN_PIN, 0); // Desliga o verde
        gpio_put(LED_BLUE_PIN, 0);  // Desliga o azul
        play_tone(NOTE_A4, 100);  // Toca o som de erro (Lá)
        sleep_ms(100);  // Mantém o LED aceso por 100 ms
        gpio_put(LED_RED_PIN, 0);   // Desliga o LED
        sleep_ms(100);  // Intervalo entre as piscadas
    }
}

// Função para exibir mensagem no display OLED
void display_message(char *message, int line) {
    struct render_area frame_area = {
        start_column: 0,
        end_column: ssd1306_width - 1,
        start_page: 0,
        end_page: ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);

    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);  // Limpa o buffer
    ssd1306_draw_string(ssd, 5, line * 8, message);  // Desenha a mensagem
    render_on_display(ssd, &frame_area);  // Renderiza no display
}

// Função para exibir duas mensagens no display OLED
void display_two_messages(char *message1, int line1, char *message2, int line2) {
    struct render_area frame_area = {
        start_column: 0,
        end_column: ssd1306_width - 1,
        start_page: 0,
        end_page: ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);

    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);  // Limpa o buffer

    // Desenha a primeira mensagem
    ssd1306_draw_string(ssd, 5, line1 * 8, message1);

    // Desenha a segunda mensagem
    ssd1306_draw_string(ssd, 5, line2 * 8, message2);

    // Renderiza as duas mensagens no display
    render_on_display(ssd, &frame_area);
}

void startup_animation() {
    ColorState startup_colors[] = {MAGENTA, GREEN, BLUE, YELLOW};  // Sequência de cores da animação
    for (int i = 0; i < 2; i++) {  // Repete a animação 2 vezes
        for (int j = 0; j < 4; j++) {  // Percorre as 4 cores
            set_rgb_color(startup_colors[j]);  // Mostra a cor
            play_color_sound(startup_colors[j]);  // Toca o som correspondente
            sleep_ms(150);  // Intervalo entre as cores (150 ms)
            set_rgb_color(NUM_COLORS);  // Desliga o LED
            sleep_ms(150);  // Intervalo entre as cores (150 ms)
        }
    }
}

// Função para ler a cor selecionada pelo joystick
ColorState read_joystick_color() {
    adc_select_input(0);  // Seleciona o canal ADC0 (eixo X, GPIO 26)
    uint16_t x_value = adc_read();  // Lê o valor do eixo X
    adc_select_input(1);  // Seleciona o canal ADC1 (eixo Y, GPIO 27)
    uint16_t y_value = adc_read();  // Lê o valor do eixo Y
    adc_select_input(0);  // Volta ao canal ADC0 (eixo X, GPIO 26)

    // Mapeia os valores do joystick para as cores
    if (x_value < 1000) {
        return MAGENTA;  // Esquerda
    } else if (x_value > 3000) {
        return BLUE;  // Direita
    } else if (y_value > 3000) {
        return GREEN;  // Cima
    } else if (y_value < 1000) {
        return YELLOW; // Baixo
    } else {
        return NUM_COLORS;  // Nenhuma cor selecionada (centro)
    }
}

// Função para ajustar o número de rodadas usando o joystick
void adjust_rounds() {
    char rounds_msg[20];
    int last_x_value = adc_read();  // Lê o valor inicial do eixo X

    while (true) {
        // Lê o valor do eixo X do joystick
        adc_select_input(0);  // Seleciona o canal ADC0 (eixo X, GPIO 26)
        uint16_t x_value = adc_read();  // Lê o valor do eixo X

        // Verifica se o joystick foi movido para a direita (aumentar rodadas)
        if (x_value > 3000 && last_x_value <= 3000) {
            if (total_rounds < 10) {
                total_rounds++;  // Incrementa o número de rodadas
            }
        }
        // Verifica se o joystick foi movido para a esquerda (diminuir rodadas)
        else if (x_value < 1000 && last_x_value >= 1000) {
            if (total_rounds > 1) {
                total_rounds--;  // Decrementa o número de rodadas
            }
        }

        // Atualiza o último valor lido do eixo X
        last_x_value = x_value;

        // Exibe o número de rodadas no display OLED
        snprintf(rounds_msg, sizeof(rounds_msg), "Num Rodadas: %d", total_rounds);
        display_message(rounds_msg, 3);

        // Verifica se o botão B foi pressionado para confirmar
        if (!gpio_get(BUTTON_B_PIN)) {
            sleep_ms(20);  // Debounce simples
            if (!gpio_get(BUTTON_B_PIN)) {  // Confirma o pressionamento
                return;  // Retorna ao jogo
            }
        }

        sleep_ms(100);  // Pequeno delay para evitar leituras muito rápidas
    }
}

// Função principal
int main() {
    stdio_init_all();  // Inicializa a comunicação serial

    // Configura os pinos
    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);  // Habilita resistor pull-up no botão A
    gpio_pull_up(BUTTON_B_PIN);  // Habilita resistor pull-up no botão B

    // Configura o PWM para o buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    channel = pwm_gpio_to_channel(BUZZER_PIN);

    // Configura o ADC para leitura do joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);  // Configura GPIO 26 como entrada analógica
    adc_gpio_init(JOYSTICK_Y_PIN);  // Configura GPIO 27 como entrada analógica
    adc_select_input(0);            // Seleciona o canal ADC0 (GPIO 26)

    // Configura o I2C para o display OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init();

    // Exibe a mensagem inicial
    display_message("Aperte Botao A", 3);

    // Configura a semente do gerador de números aleatórios
    srand(time_us_64());

    // Variáveis do jogo
    bool game_active = false;  // Estado do jogo (ligado/desligado)
    ColorState sequence[10];  // Sequência de cores (máximo de 10 cores)
    ColorState player_sequence[10];  // Sequência do jogador
    int sequence_length = 1;  // Tamanho inicial da sequência
    int player_index = 0;  // Índice da sequência do jogador
    int round = 1;  // Número da rodada atual
    int max_rounds = 0;  // Número máximo de rodadas completadas sem errar
    uint64_t game_start_time = 0;  // Tempo de início do jogo

    while (true) {
        // Verifica se o botão A foi pressionado (liga/desliga o jogo)
        if (!gpio_get(BUTTON_A_PIN)) {
            sleep_ms(20);  // Debounce simples
            if (!gpio_get(BUTTON_A_PIN)) {  // Confirma o pressionamento
                if (!game_active) {
                    // Inicia o jogo
                    game_active = true;
                    
                    // Exibe a mensagem de boas-vindas
                    display_message("Bem-vindo!", 3);

                    // Executa a animação de abertura
                    startup_animation();

                    // Ajusta o número de rodadas usando o joystick
                    adjust_rounds();
                    
                    // Gera a sequência inicial
                    generate_sequence(sequence, sequence_length);

                    // Armazena o tempo de início do jogo
                    game_start_time = time_us_64();

                    // Reinicia o número máximo de rodadas completadas
                    max_rounds = 0;
                } else {
                    // Desliga o jogo
                    game_active = false;
                    

                    // Exibe a mensagem inicial
                    display_message("Aperte Botao A", 3);

                    // Reinicia as variáveis do jogo
                    round = 1;
                    sequence_length = 1;
                    set_rgb_color(NUM_COLORS);  // Desliga o LED
                }

                while (!gpio_get(BUTTON_A_PIN));  // Aguarda o botão ser solto
            }
        }

        // Se o jogo estiver ativo, executa o loop do jogo
        if (game_active) {
           
            char round_msg[20];
            snprintf(round_msg, sizeof(round_msg), "Rodada %d", round);
            display_message(round_msg, 3);  // Exibe a rodada atual no display

            // Mostra a sequência para o jogador no LED RGB
            show_sequence(sequence, sequence_length);

            // Reseta o índice do jogador
            player_index = 0;

            // Loop para capturar a sequência do jogador
            while (player_index < sequence_length) {
                // Verifica se o botão A foi pressionado (desliga o jogo)
                if (!gpio_get(BUTTON_A_PIN)) {
                    sleep_ms(20);  // Debounce simples
                    if (!gpio_get(BUTTON_A_PIN)) {  // Confirma o pressionamento
                        game_active = false;  // Desliga o jogo
                        
                        // Exibe a mensagem inicial
                        display_message("Aperte Botao A", 3);

                        // Reinicia as variáveis do jogo
                        round = 1;
                        sequence_length = 1;
                        set_rgb_color(NUM_COLORS);  // Desliga o LED
                        while (!gpio_get(BUTTON_A_PIN));  // Aguarda o botão ser solto
                        break;  // Sai do loop de captura da sequência
                    }
                }

                // Lê a cor selecionada pelo joystick
                ColorState selected_color = read_joystick_color();
                if (selected_color != NUM_COLORS) {
                    set_rgb_color(selected_color);  // Mostra a cor selecionada
                    player_sequence[player_index] = selected_color;  // Armazena a cor selecionada
                }

                // Verifica se o botão B foi pressionado (confirmar cor)
                if (!gpio_get(BUTTON_B_PIN)) {
                    // Toca o som da cor selecionada
                    play_color_sound(player_sequence[player_index]);

                    player_index++;  // Avança para a próxima cor
                    sleep_ms(200);  // Debounce
                    while (!gpio_get(BUTTON_B_PIN));  // Aguarda o botão ser solto
                }
            }

            // Se o jogo foi interrompido, sai do loop do jogo
            if (!game_active) {
                continue;
            }

            // Verifica se a sequência do jogador está correta
            if (check_sequence(sequence, player_sequence, sequence_length)) {
                
                
                // Atualiza o número máximo de rodadas completadas
                if (round > max_rounds) {
                    max_rounds = round;
                }

                // Avança para a próxima rodada
                round++;
                if (round > total_rounds) {  // Usa o número de rodadas escolhido
                    // Exibe as mensagens de vitória
                    display_two_messages("Voce venceu!", 3, "Parabens!", 4);

                    // Animação de vitória com sons
                    victory_animation();

                    // Desliga o LED
                    set_rgb_color(NUM_COLORS);

                    // Reinicia o jogo
                    game_active = false;
                    display_message("Aperte Botao A", 3);  // Exibe a mensagem inicial
                    round = 1;
                    sequence_length = 1;
                } else {
                    // Aumenta a dificuldade (adiciona uma nova cor à sequência)
                    sequence_length++;
                    generate_sequence(sequence, sequence_length);  // Gera uma nova sequência
                }
            } else {
               
                // Exibe a mensagem de erro e o número máximo de rodadas
                char error_msg[30];
                snprintf(error_msg, sizeof(error_msg), "Rodadas: %d", (round == 1) ? 0 : max_rounds);  // Mostra 0 se for a primeira rodada
                display_two_messages("incorreto!", 3, error_msg, 4);

                // Executa a animação de erro
                error_animation();

                // Aguarda 2 segundos para dar tempo de ler as mensagens
                sleep_ms(2000);

                // Reinicia o jogo
                round = 1;
                sequence_length = 1;
                generate_sequence(sequence, sequence_length);  // Gera uma nova sequência
            }
        }
    }

    return 0;
}
