/* Tavoitteena kurssiarvosanaksi on 5, eli parhaani yritän niinkuin aina*/


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)
#define STACKSIZE 500
#define PRIORITY 5


static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static const struct gpio_dt_spec buttons[] = {
        GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0}),
        GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0}),
        GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0}),
        GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {0}),
        GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {0}),
};
static struct gpio_callback button_cb_data;

volatile int led_state = 0;
volatile int saved_state = 0;

volatile int red_manual = 0;
volatile int yellow_manual = 0;
volatile int green_manual = 0;
volatile int yellow_mode = 0;

int init_buttons(void);
int init_led(void);
void button_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);


void red_led_task(void *, void *, void *);
void yellow_led_task(void *, void *, void *);
void green_led_task(void *, void *, void *);
///void blue_led_task(void *, void *, void *);

K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
///K_THREAD_DEFINE(blue_thread, STACKSIZE, blue_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
	init_led();
        init_buttons();
	return 0;
}

int init_led(void)
{
	int ret;

	ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Red led configure failed\n");
		return ret;
	}

	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Green led configure failed\n");
		return ret;
	}

	gpio_pin_set_dt(&red, 0);
	gpio_pin_set_dt(&green, 0);

	printk("Led initialized ok\n");
	return 0;
}

int init_buttons(void)
{
        uint32_t mask = 0;

        for (int i = 0; i < 5; i++) {
                int ret;

                if (!gpio_is_ready_dt(&buttons[i])) {
                        printk("Button %d is not ready\n", i + 1);
                        return -1;
                }

                ret = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT);
                if (ret < 0) {
                        return ret;
                }

                ret = gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_TO_ACTIVE);
                if (ret < 0) {
                        return ret;
                }

                mask |= BIT(buttons[i].pin);
        }

        gpio_init_callback(&button_cb_data, button_handler, mask);
        gpio_add_callback(buttons[0].port, &button_cb_data);
        return 0;
}

void button_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
        if (pins & BIT(buttons[0].pin)) {
                if (led_state != 4) {
                        saved_state = led_state;
                        led_state = 4;
                } else {
                        led_state = saved_state;
                }
                printk("Button pressed\n");
        } else if (pins & BIT(buttons[1].pin)) {
                red_manual = !red_manual;
                if (!red_manual) {
                        gpio_pin_set_dt(&red, 0);
                }
        } else if (pins & BIT(buttons[2].pin)) {
                yellow_manual = !yellow_manual;
                if (!yellow_manual) {
                        gpio_pin_set_dt(&red, 0);
                        gpio_pin_set_dt(&green, 0);
                }
        } else if (pins & BIT(buttons[3].pin)) {
                green_manual = !green_manual;
                if (!green_manual) {
                        gpio_pin_set_dt(&green, 0);
                }
        } else if (pins & BIT(buttons[4].pin)) {
                if (!yellow_mode) {
                        saved_state = led_state;
                        led_state = 5;
                        yellow_mode = 1;
                } else {
                        yellow_mode = 0;
                        led_state = saved_state;
                }
        }
}

void red_led_task(void *, void *, void *)
{
	while (true) {
		if (red_manual) {
			gpio_pin_set_dt(&red, 1);
		} else if (led_state == 0 && !yellow_mode) {
			gpio_pin_set_dt(&red, 1);
			k_sleep(K_SECONDS(1));
			gpio_pin_set_dt(&red, 0);
			if (led_state == 0) {
				led_state = 1;
			}
		}
		k_msleep(100);
        }
}

void green_led_task(void *, void *, void *)
{
	while (true) {
		if (green_manual) {
			gpio_pin_set_dt(&green, 1);
		} else if (led_state == 2 && !yellow_mode) {
			gpio_pin_set_dt(&green, 1);
			k_sleep(K_SECONDS(1));
			gpio_pin_set_dt(&green, 0);
			if (led_state == 2) {
				led_state = 0;
			}
		}
		k_msleep(100);
        }
}

void yellow_led_task(void *, void *, void *)
{
	while (true) {
		if (yellow_manual) {
			gpio_pin_set_dt(&red, 1);
			gpio_pin_set_dt(&green, 1);
		} else if (yellow_mode) {
			gpio_pin_set_dt(&red, 1);
			gpio_pin_set_dt(&green, 1);
			k_msleep(500);
			gpio_pin_set_dt(&red, 0);
			gpio_pin_set_dt(&green, 0);
			k_msleep(500);
		} else if (led_state == 1) {
			gpio_pin_set_dt(&red, 1);
			gpio_pin_set_dt(&green, 1);
			k_sleep(K_SECONDS(1));
			gpio_pin_set_dt(&red, 0);
			gpio_pin_set_dt(&green, 0);
			if (led_state == 1) {
				led_state = 2;
			}
		}
		k_msleep(100);
        }
}