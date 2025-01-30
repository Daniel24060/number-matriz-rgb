#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include "ws2818b.pio.h"
#define LED_R 13  // LED RGB (vermelho)
#define LED_G 11  // LED RGB (verde)
#define LED_B 12  // LED RGB (azul)
#define LED_COUNT 25
#define LED_PIN 7
#define BUTTON_A 5
#define BUTTON_B 6

struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;
int current_number = 0;

// Inicializa a matriz de LEDs NeoPixel
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Define a cor de um LED específico na matriz
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

// Limpa todos os LEDs da matriz
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

// Envia os dados dos LEDs para a matriz
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

// Calcula o índice linear a partir das coordenadas x e y
int getIndex(int x, int y) {
    return (y % 2 == 0) ? (24 - (y * 5 + x)) : (24 - (y * 5 + (4 - x)));
}

// Exibe um número na matriz de LEDs
void displayNumber(int number) {
    static int numbers[10][5][5] = {
        {{1,1,1,1,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {1,1,1,1,1}}, // 0
        {{0,0,1,1,0}, {0,1,1,1,0}, {0,0,1,1,0}, {0,0,1,1,0}, {0,1,1,1,1}}, // 1
        {{1,1,1,1,1}, {0,0,0,0,1}, {1,1,1,1,1}, {1,0,0,0,0}, {1,1,1,1,1}}, // 2
        {{1,1,1,1,1}, {0,0,0,0,1}, {0,1,1,1,1}, {0,0,0,0,1}, {1,1,1,1,1}}, // 3
        {{1,0,0,1,1}, {1,0,0,1,1}, {1,1,1,1,1}, {0,0,0,1,1}, {0,0,0,1,1}}, // 4
        {{1,1,1,1,1}, {1,0,0,0,0}, {1,1,1,1,1}, {0,0,0,0,1}, {1,1,1,1,1}}, // 5
        {{1,1,1,1,1}, {1,0,0,0,0}, {1,1,1,1,1}, {1,0,0,0,1}, {1,1,1,1,1}}, // 6
        {{1,1,1,1,1}, {0,0,0,0,1}, {0,0,0,1,0}, {0,0,1,0,0}, {0,1,0,0,0}}, // 7
        {{1,1,1,1,1}, {1,0,0,0,1}, {1,1,1,1,1}, {1,0,0,0,1}, {1,1,1,1,1}}, // 8
        {{1,1,1,1,1}, {1,0,0,0,1}, {1,1,1,1,1}, {0,0,0,0,1}, {1,1,1,1,1}}  // 9
    };

    npClear();
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            if (numbers[number][y][x]) {
                int pos = getIndex(x, y);
                npSetLED(pos, 50, 50, 50);
            }
        }
    }
    npWrite();
}

// Manipula os botões para mudar o número exibido
void buttonHandler() {
    if (!gpio_get(BUTTON_A)) {
        current_number = (current_number + 1) % 10;
        displayNumber(current_number);
        sleep_ms(200);
    }
    if (!gpio_get(BUTTON_B)) {
        current_number = (current_number - 1 + 10) % 10;
        displayNumber(current_number);
        sleep_ms(200);
    }
}

// Pisca o LED vermelho continuamente 5 vezes por segundo
int64_t blinkRedLED(alarm_id_t id, void *user_data) {
    static bool led_state = false;
    gpio_put(LED_R, led_state);
    led_state = !led_state;
    return 200 * 1000;  // Retorna o tempo em microssegundos para o próximo alarme
}

int main() {
    stdio_init_all();  // Inicializa todas as entradas e saídas padrão
    npInit(LED_PIN);  // Inicializa a matriz de LEDs NeoPixel
    npClear();  // Limpa a matriz de LEDs

    // Inicializa os botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    // Inicializa os LEDs RGB
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);

    displayNumber(current_number);  // Exibe o número inicial

    // Configura um alarme para piscar o LED vermelho 5 vezes por segundo
    add_alarm_in_ms(200, blinkRedLED, NULL, true);

    while (true) {
        buttonHandler();  // Manipula os botões
        sleep_ms(50);  // Aguarda 50 ms
    }
}
