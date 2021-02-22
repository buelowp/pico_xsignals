#include <stdio.h> 
#include "pico/stdlib.h" 
#include "hardware/gpio.h"
#include "pico/binary_info.h"

#define DETECT_1    9
#define DETECT_2    4
#define DETECT_3    28
#define DETECT_4    26

#define SIGNAL_1    14
#define SIGNAL_2    13
#define SIGNAL_3    16
#define SIGNAL_4    18

#define BLINK_DELAY 1000

static bool g_signalsOn;
static bool g_fourSensorMode;
static bool g_timerActive;
static uint g_lastEventGpio;
repeating_timer_t g_timer;

bool alternate(repeating_timer_t *rt)
{
    static int lastside = 0;

    switch (lastside) {
        case 0:
            gpio_put(SIGNAL_1, false);
            gpio_put(SIGNAL_3, false);
            gpio_put(SIGNAL_2, true);
            gpio_put(SIGNAL_4, true);
            lastside = 1;
            break;
        case 1:
            gpio_put(SIGNAL_1, true);
            gpio_put(SIGNAL_3, true);
            gpio_put(SIGNAL_2, false);
            gpio_put(SIGNAL_4, false);
            lastside = 0;
            break;
    }

    return true;
}

/**
 * This assumes that creating a timer is safe in an IRQ context.
 */
void detection_callback(uint gpio, uint32_t events)
{
    if (gpio == DETECT_1 || gpio == DETECT_4) {
        if (!g_signalsOn) {
            g_timerActive = add_repeating_timer_ms(BLINK_DELAY, &alternate, NULL, &g_timer);
        }
        if (!g_fourSensorMode && g_timerActive && g_signalsOn) {
            cancel_repeating_timer(&g_timer);
        }
    }

    if (gpio == DETECT_2 || gpio == DETECT_3) {
        if (g_signalsOn && g_timerActive) {
            cancel_repeating_timer(&g_timer);
        }
    }
}

void setup_gpio()
{
/*
    gpio_init(DETECT_1);
    gpio_init(DETECT_2);
    gpio_init(DETECT_3);
    gpio_init(DETECT_4);
*/
    gpio_init(SIGNAL_1);
    gpio_init(SIGNAL_2);
    gpio_init(SIGNAL_3);
    gpio_init(SIGNAL_4);

/*
    It isn't clear if I have to do this prior to calling irq_enabled

    gpio_set_dir(DETECT_1, GPIO_IN);
    gpio_set_dir(DETECT_2, GPIO_IN);
    gpio_set_dir(DETECT_3, GPIO_IN);
    gpio_set_dir(DETECT_4, GPIO_IN);
*/
    gpio_set_dir(SIGNAL_1, GPIO_OUT);
    gpio_set_dir(SIGNAL_2, GPIO_OUT);
    gpio_set_dir(SIGNAL_3, GPIO_OUT);
    gpio_set_dir(SIGNAL_4, GPIO_OUT);

    gpio_put(SIGNAL_1, false);
    gpio_put(SIGNAL_2, false);
    gpio_put(SIGNAL_3, false);
    gpio_put(SIGNAL_4, false);
}

/**
 * Check to see if we have a HIGH on DETECT 2 and 3, and if so, we
 * are four sensor enabled. This will fail if there is an obstruction
 * during startup, so when powering on, make sure no train is in the way
 */
bool check_for_four_sensors()
{
    gpio_set_irq_enabled_with_callback(DETECT_1, GPIO_IRQ_EDGE_FALL, true, &detection_callback);
    gpio_set_irq_enabled_with_callback(DETECT_4, GPIO_IRQ_EDGE_FALL, true, &detection_callback);

    if (gpio_get(DETECT_2) == 1 && gpio_get(DETECT_3) == 1) {
        printf("%s: Enabling 4 detector mode", __FUNCTION__);
        g_fourSensorMode = true;
        gpio_set_irq_enabled_with_callback(DETECT_2, GPIO_IRQ_EDGE_FALL, true, &detection_callback);
        gpio_set_irq_enabled_with_callback(DETECT_3, GPIO_IRQ_EDGE_FALL, true, &detection_callback);
        return true;
    }
    return false;
}

int main()
{
    bi_decl(bi_program_description("Detect and control crossing signals on a model railroad"));

    g_signalsOn = false;
    g_fourSensorMode = false;
    g_timerActive = false;
    g_lastEventGpio = 0;

    stdio_init_all();
    setup_gpio();
    check_for_four_sensors();

    while (1) {}
}

