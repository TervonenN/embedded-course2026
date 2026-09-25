/*
 * Tavoite: Maksimipisteet, koska miksi ei?
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

#define STACKSIZE 500
#define PRIORITY 5
#define UART_MESSAGE_SIZE 32
#define DEBUG_MESSAGE_SIZE 96

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

static const struct device *const uart_dev =
    DEVICE_DT_GET(UART_DEVICE_NODE);

struct uart_message {
    void *fifo_reserved;
    char text[UART_MESSAGE_SIZE];
};

struct light_command {
    void *fifo_reserved;
    int duration_ms;
};

struct parsed_command {
    char color;
    int duration_ms;
};

struct debug_message {
    void *fifo_reserved;
    char text[DEBUG_MESSAGE_SIZE];
};

K_FIFO_DEFINE(uart_fifo);
K_FIFO_DEFINE(red_fifo);
K_FIFO_DEFINE(yellow_fifo);
K_FIFO_DEFINE(green_fifo);
K_FIFO_DEFINE(debug_fifo);

K_MUTEX_DEFINE(red_mutex);
K_MUTEX_DEFINE(yellow_mutex);
K_MUTEX_DEFINE(green_mutex);

K_CONDVAR_DEFINE(red_condition);
K_CONDVAR_DEFINE(yellow_condition);
K_CONDVAR_DEFINE(green_condition);

K_SEM_DEFINE(release_signal, 0, 1);

static uint64_t last_task_elapsed_us;
static volatile bool debug_enabled = true;

/* MÄÄRITTELYT */

static const struct gpio_dt_spec red =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct gpio_dt_spec green =
    GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static void debug_log(const char *format, ...)
{
    struct debug_message *message;
    va_list args;

    if (!debug_enabled) {
        return;
    }

    message = k_malloc(sizeof(struct debug_message));
    if (message == NULL) {
        return;
    }

    va_start(args, format);
    vsnprintk(message->text, sizeof(message->text), format, args);
    va_end(args);

    k_fifo_put(&debug_fifo, message);
}

static int init_leds(void)
{
    int ret;

    if (!gpio_is_ready_dt(&red) ||
        !gpio_is_ready_dt(&green)) {
        debug_log("Ledit eivät ole valmiina\n");
        return -1;
    }

    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        debug_log("Punaisen LEDin konfigurointi epäonnistui\n");
        return ret;
    }

    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        debug_log("Vihreän LEDin konfigurointi epäonnistui\n");
        return ret;
    }

    return 0;
}

static int init_uart(void)
{
    if (!device_is_ready(uart_dev)) {
        debug_log("UART ei ole valmis\n");
        return -1;
    }

    return 0;
}

static bool send_red_command(int duration_ms)
{
    struct light_command *command =
        k_malloc(sizeof(struct light_command));

    if (command == NULL) {
        debug_log("Punaisen komennon muistin varaus epäonnistui\n");
        return false;
    }

    command->duration_ms = duration_ms;

    k_mutex_lock(&red_mutex, K_FOREVER);
    k_fifo_put(&red_fifo, command);
    k_condvar_signal(&red_condition);
    k_mutex_unlock(&red_mutex);

    return true;
}

static bool send_green_command(int duration_ms)
{
    struct light_command *command =
        k_malloc(sizeof(struct light_command));

    if (command == NULL) {
        debug_log("Vihreän komennon muistin varaus epäonnistui\n");
        return false;
    }

    command->duration_ms = duration_ms;

    k_mutex_lock(&green_mutex, K_FOREVER);
    k_fifo_put(&green_fifo, command);
    k_condvar_signal(&green_condition);
    k_mutex_unlock(&green_mutex);

    return true;
}

static bool send_yellow_command(int duration_ms)
{
    struct light_command *command =
        k_malloc(sizeof(struct light_command));

    if (command == NULL) {
        debug_log("Keltaisen komennon muistin varaus epäonnistui\n");
        return false;
    }

    command->duration_ms = duration_ms;

    k_mutex_lock(&yellow_mutex, K_FOREVER);
    k_fifo_put(&yellow_fifo, command);
    k_condvar_signal(&yellow_condition);
    k_mutex_unlock(&yellow_mutex);

    return true;
}

/* TASKIT */

static void red_task(void *p1, void *p2, void *p3)
{
    while (true) {
        k_mutex_lock(&red_mutex, K_FOREVER);

        while (k_fifo_is_empty(&red_fifo)) {
            k_condvar_wait(&red_condition,
                       &red_mutex,
                       K_FOREVER);
        }

        struct light_command *command =
            k_fifo_get(&red_fifo, K_NO_WAIT);

        k_mutex_unlock(&red_mutex);

        if (command != NULL) {
            uint64_t start_time_us =
                k_uptime_get() * 1000ULL;

            gpio_pin_set_dt(&red, 1);
            k_sleep(K_MSEC(command->duration_ms));
            gpio_pin_set_dt(&red, 0);

            uint64_t end_time_us =
                k_uptime_get() * 1000ULL;

            uint64_t elapsed_us =
                end_time_us - start_time_us;

            last_task_elapsed_us = elapsed_us;

                 debug_log("Punaisen taskin kesto: %llu us\n",
                     (unsigned long long)elapsed_us);

            k_free(command);
            k_sem_give(&release_signal);
        }
    }
}

static void green_task(void *p1, void *p2, void *p3)
{
    while (true) {
        k_mutex_lock(&green_mutex, K_FOREVER);

        while (k_fifo_is_empty(&green_fifo)) {
            k_condvar_wait(&green_condition,
                       &green_mutex,
                       K_FOREVER);
        }

        struct light_command *command =
            k_fifo_get(&green_fifo, K_NO_WAIT);

        k_mutex_unlock(&green_mutex);

        if (command != NULL) {
            uint64_t start_time_us =
                k_uptime_get() * 1000ULL;

            gpio_pin_set_dt(&green, 1);
            k_sleep(K_MSEC(command->duration_ms));
            gpio_pin_set_dt(&green, 0);

            uint64_t end_time_us =
                k_uptime_get() * 1000ULL;

            uint64_t elapsed_us =
                end_time_us - start_time_us;

            last_task_elapsed_us = elapsed_us;

                 debug_log("Vihreän taskin kesto: %llu us\n",
                     (unsigned long long)elapsed_us);

            k_free(command);
            k_sem_give(&release_signal);
        }
    }
}

static void yellow_task(void *p1, void *p2, void *p3)
{
    while (true) {
        k_mutex_lock(&yellow_mutex, K_FOREVER);

        while (k_fifo_is_empty(&yellow_fifo)) {
            k_condvar_wait(&yellow_condition,
                       &yellow_mutex,
                       K_FOREVER);
        }

        struct light_command *command =
            k_fifo_get(&yellow_fifo, K_NO_WAIT);

        k_mutex_unlock(&yellow_mutex);

        if (command != NULL) {
            uint64_t start_time_us =
                k_uptime_get() * 1000ULL;

            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 1);
            k_sleep(K_MSEC(command->duration_ms));
            gpio_pin_set_dt(&red, 0);
            gpio_pin_set_dt(&green, 0);

            uint64_t end_time_us =
                k_uptime_get() * 1000ULL;

            uint64_t elapsed_us =
                end_time_us - start_time_us;

            last_task_elapsed_us = elapsed_us;

                 debug_log("Keltaisen taskin kesto: %llu us\n",
                     (unsigned long long)elapsed_us);

            k_free(command);
            k_sem_give(&release_signal);
        }
    }
}

static void uart_task(void *p1, void *p2, void *p3)
{
    char message[UART_MESSAGE_SIZE] = {};
    int length = 0;

    while (true) {
        unsigned char received;

        if (uart_poll_in(uart_dev, &received) == 0) {
            if (received == '\r' ||
                received == '\n') {
                if (length > 0) {
                    struct uart_message *item =
                        k_malloc(sizeof(struct uart_message));

                    if (item != NULL) {
                        memcpy(item->text,
                               message,
                               length);

                        item->text[length] = '\0';

                        k_fifo_put(&uart_fifo, item);

                        /* printk("UART-viesti vastaanotettu %s\n",
                               item->text);*/
                    }

                    length = 0;
                    message[0] = '\0';
                }
            } else if (length < UART_MESSAGE_SIZE - 1) {
                message[length] = received;
                length++;
                message[length] = '\0';
            }
        }

        k_sleep(K_MSEC(10));
    }
}

static bool parse_command(const char *text,
              struct parsed_command *command)
{
    char extra;

    __ASSERT(text != NULL, "Komennon teksti puuttuu");
    __ASSERT(command != NULL, "Komentorakenne puuttuu");

    int fields = sscanf(text,
                " %c,%d %c",
                &command->color,
                &command->duration_ms,
                &extra);

    if (fields != 2) {
        return false;
    }

    if (command->color != 'R' &&
        command->color != 'G' &&
        command->color != 'Y') {
        return false;
    }

    if (command->duration_ms < 0) {
        return false;
    }

    return true;
}

static void debug_task(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (true) {
        struct debug_message *message =
            k_fifo_get(&debug_fifo, K_FOREVER);

        if (message != NULL) {
            printk("%s", message->text);
            k_free(message);
        }
    }
}

static uint64_t dispatch_command(struct parsed_command *command)
{
    bool command_sent = false;

    __ASSERT(command != NULL, "Lähetettävä komento puuttuu");
    __ASSERT(command->duration_ms >= 0,
             "Valon kesto ei voi olla negatiivinen");

    if (command->color == 'R') {
        command_sent =
            send_red_command(command->duration_ms);
    } else if (command->color == 'G') {
        command_sent =
            send_green_command(command->duration_ms);
    } else if (command->color == 'Y') {
        command_sent =
            send_yellow_command(command->duration_ms);
    }

    if (!command_sent) {
        return 0;
    }

    k_sem_take(&release_signal, K_FOREVER);

    return last_task_elapsed_us;
}

static void dispatcher_task(void *p1, void *p2, void *p3)
{
    static uint64_t sequence_total_us = 0;
    static unsigned int sequence_command_count = 0;

    while (true) {
        struct uart_message *message =
            k_fifo_get(&uart_fifo, K_FOREVER);
        uint64_t processing_start_us = k_uptime_get() * 1000ULL;

        __ASSERT(message != NULL, "UART-viesti puuttuu");

        if (strcmp(message->text, "D") == 0) {
            debug_enabled = !debug_enabled;
            if (debug_enabled) {
                debug_log("Debug-tulostukset päällä\n");
            }
            k_free(message);
            continue;
        }

       /* printk("Dispatcher sai viestin %s\n",
               message->text); */

        struct parsed_command command;

        if (parse_command(message->text, &command)) {
            if (command.color == 'R') {
                sequence_total_us = 0;
                sequence_command_count = 0;
            }

            /*printk("Väri: %c, aika: %d ms\n",
                   command.color,
                   command.duration_ms); */

            sequence_total_us +=
                dispatch_command(&command);
            sequence_command_count++;

            debug_log("Komennon käsittelyaika dispatcherissa: %llu us\n",
                      (unsigned long long)
                      (k_uptime_get() * 1000ULL - processing_start_us));

            if (command.color == 'G') {
                debug_log("Sekvenssin kokonaisaika (%u komentoa): %llu us\n",
                          sequence_command_count,
                          (unsigned long long)sequence_total_us);
            }
        } else {
            debug_log("Virheellinen komento: %s\n",
                      message->text);
        }

        k_free(message);
    }
}

K_THREAD_DEFINE(red_thread,
        STACKSIZE,
        red_task,
        NULL,
        NULL,
        NULL,
        PRIORITY,
        0,
        0);

K_THREAD_DEFINE(green_thread,
        STACKSIZE,
        green_task,
        NULL,
        NULL,
        NULL,
        PRIORITY,
        0,
        0);

K_THREAD_DEFINE(yellow_thread,
        STACKSIZE,
        yellow_task,
        NULL,
        NULL,
        NULL,
        PRIORITY,
        0,
        0);

K_THREAD_DEFINE(uart_thread,
        STACKSIZE,
        uart_task,
        NULL,
        NULL,
        NULL,
        PRIORITY,
        0,
        0);

K_THREAD_DEFINE(dispatcher_thread,
        STACKSIZE,
        dispatcher_task,
        NULL,
        NULL,
        NULL,
        PRIORITY,
        0,
        0);

    K_THREAD_DEFINE(debug_thread,
        STACKSIZE,
        debug_task,
        NULL,
        NULL,
        NULL,
        PRIORITY + 2,
        0,
        0);

int main(void)
{
    int ret = init_leds();

    if (ret < 0) {
        return ret;
    }

    ret = init_uart();

    if (ret < 0) {
        return ret;
    }

    /*printk("Liikennevalot valmiina\n");*/

    return 0;
}