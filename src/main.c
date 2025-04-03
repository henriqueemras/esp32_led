#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/timing/timing.h>
#include <zephyr/devicetree.h>
//teste de edição online
#define LED_GREEN_PIN 2
#define LED_YELLOW_PIN 4
#define LED_RED_PIN 15
#define BUTTON_PIN 12

#define STACK_SIZE 1024
#define THREAD_PRIORITY_SEMAPHORE 5
#define THREAD_PRIORITY_PEDESTRIAN 6

struct k_sem semaphore;
volatile bool pedestrian_request = false;

uint64_t last_context_switch_time = 0;
uint64_t button_press_time = 0;

void blink_led(const struct device *gpio_dev, uint32_t pin, uint32_t delay_ms) {
    gpio_pin_set(gpio_dev, pin, 1);
    k_msleep(delay_ms);
    gpio_pin_set(gpio_dev, pin, 0);
}

void pedestrian_mode(const struct device *gpio_dev) {
    k_sem_take(&semaphore, K_FOREVER);
    printk("Modo pedestre ativado\n");

    uint64_t start_time = k_uptime_get();
    printk("Latência de interrupção: %llu ms\n", start_time - button_press_time);
    
    // Pisca o LED vermelho por 8 segundos (tempo de travessia do pedestre)
    uint32_t pedestrian_crossing_time = 8000; // 8 segundos
    uint32_t blink_interval = 500; // Intervalo de piscar em milissegundos
    uint32_t elapsed_time = 0;

    while (elapsed_time < pedestrian_crossing_time) {
        gpio_pin_set(gpio_dev, LED_RED_PIN, 1);
        k_msleep(blink_interval / 2);
        gpio_pin_set(gpio_dev, LED_RED_PIN, 0);
        k_msleep(blink_interval / 2);
        elapsed_time += blink_interval;
    }

    pedestrian_request = false;
    k_sem_give(&semaphore); 
}

void button_thread(void *a, void *b, void *c) {
    const struct device *gpio_btn_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    if (!device_is_ready(gpio_btn_dev)) {
        printk("Erro: Dispositivo GPIO botão não encontrado.\n");
        return;
    }
    gpio_pin_configure(gpio_btn_dev, BUTTON_PIN, GPIO_INPUT);

    while (1) {
        if (gpio_pin_get(gpio_btn_dev, BUTTON_PIN)) {
            button_press_time = k_uptime_get();
            pedestrian_request = true;
            k_msleep(200); // Debounce
        }
        k_msleep(100);
    }
}

void traffic_light_thread(void *a, void *b, void *c) {
    const struct device *gpio_led_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    if (!device_is_ready(gpio_led_dev)) {
        printk("Erro: Dispositivo GPIO LED não encontrado.\n");
        return;
    }
    uint64_t current_time;

    while (1) {
        k_sem_take(&semaphore, K_FOREVER);
        blink_led(gpio_led_dev, LED_GREEN_PIN, 10000);
        k_sem_give(&semaphore);

        k_sem_take(&semaphore, K_FOREVER);
        blink_led(gpio_led_dev, LED_YELLOW_PIN, 1000);
        k_sem_give(&semaphore); 

        if (pedestrian_request) {
            pedestrian_mode(gpio_led_dev);
        } else {
            k_sem_take(&semaphore, K_FOREVER);
            blink_led(gpio_led_dev, LED_RED_PIN, 5000);
            k_sem_give(&semaphore);
        }

        current_time = k_uptime_get();
        if (last_context_switch_time != 0) {
            printk("Tempo entre trocas de contexto: %llu us\n", 
                   current_time - last_context_switch_time);
        }
        last_context_switch_time = current_time;
    }
}

// Define as threads
K_THREAD_STACK_DEFINE(traffic_light_thread_stack, STACK_SIZE);
struct k_thread traffic_light_thread_data;
k_tid_t traffic_light_thread_id;

K_THREAD_STACK_DEFINE(button_thread_stack, STACK_SIZE);
struct k_thread button_thread_data;
k_tid_t button_thread_id;

void main(void) {
    printk("INICIANDO APP DE SEMAFORO\n");
    const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    if (!device_is_ready(gpio_dev)) {
        printk("Erro: Dispositivo GPIO main não encontrado.\n");
        return;
    }

    gpio_pin_configure(gpio_dev, LED_GREEN_PIN, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, LED_YELLOW_PIN, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, LED_RED_PIN, GPIO_OUTPUT);

    k_sem_init(&semaphore, 1, 1); 

    // Cria as threads
    traffic_light_thread_id = k_thread_create(&traffic_light_thread_data, traffic_light_thread_stack, STACK_SIZE,
                                              (k_thread_entry_t)traffic_light_thread, NULL, NULL, NULL,
                                              THREAD_PRIORITY_SEMAPHORE, 0, K_NO_WAIT);

    button_thread_id = k_thread_create(&button_thread_data, button_thread_stack, STACK_SIZE,
                                       (k_thread_entry_t)button_thread, NULL, NULL, NULL,
                                       THREAD_PRIORITY_PEDESTRIAN, 0, K_NO_WAIT);
}
