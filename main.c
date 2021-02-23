/*
Pico RR Crossing Signals
Copyright (C) 2021  goballstate at gmail

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

#define NOTIFIER    17

#define BLINK_DELAY 1000

static bool g_signalsOn;
static bool g_fourSensorMode;
static bool g_timerActive;
static uint g_lastEventGpio;
static uint g_waitingForEventSignal;
repeating_timer_t g_timer;

void turnoff()
{
    gpio_put(SIGNAL_1, false);
    gpio_put(SIGNAL_3, false);
    gpio_put(SIGNAL_2, false);
    gpio_put(SIGNAL_4, false);
    gpio_put(NOTIFIER, false);

}

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

void sensor1(uint gpio, uint32_t events)
{
    if (g_timerActive) {
        if (g_waitingForEventSignal == DETECT_1) {
            turnoff();
            cancel_repeating_timer(&g_timer);
            g_timerActive = false;
            g_waitingForEventSignal = 0;
        }
    }
    else {
        g_timerActive = add_repeating_timer_ms(BLINK_DELAY, &alternate, NULL, &g_timer);
        gpio_put(NOTIFIER, true);
        if (g_fourSensorMode) {
            g_waitingForEventSignal = DETECT_3;
        }
        else {
            g_waitingForEventSignal = DETECT_4;
        }
    }
}

void sensor2(uint gpio, uint32_t events)
{
    if (g_timerActive) {
        if (g_waitingForEventSignal == DETECT_2) {
            turnoff();
            cancel_repeating_timer(&g_timer);
            g_timerActive = false;
            g_waitingForEventSignal = 0;
        }
    }
}

void sensor3(uint gpio, uint32_t events)
{
    if (g_timerActive) {
        if (g_waitingForEventSignal == DETECT_3) {
            turnoff();
            cancel_repeating_timer(&g_timer);
            g_timerActive = false;
            g_waitingForEventSignal = 0;
        }
    }
}

void sensor4(uint gpio, uint32_t events)
{
    if (g_timerActive) {
        if (g_waitingForEventSignal == DETECT_4) {
            turnoff();
            cancel_repeating_timer(&g_timer);
            g_timerActive = false;
            g_waitingForEventSignal = 0;
        }
    }
    else {
        g_timerActive = add_repeating_timer_ms(BLINK_DELAY, &alternate, NULL, &g_timer);
        gpio_put(NOTIFIER, true);
        if (g_fourSensorMode) {
            g_waitingForEventSignal = DETECT_2;
        }
        else {
            g_waitingForEventSignal = DETECT_1;
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
    gpio_init(NOTIFIER);

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
    gpio_set_dir(NOTIFIER, GPIO_OUT);

    gpio_put(SIGNAL_1, false);
    gpio_put(SIGNAL_2, false);
    gpio_put(SIGNAL_3, false);
    gpio_put(SIGNAL_4, false);
    gpio_put(NOTIFIER, false);
}

/**
 * Check to see if we have a HIGH on DETECT 2 and 3, and if so, we
 * are four sensor enabled. This will fail if there is an obstruction
 * during startup, so when powering on, make sure no train is in the way
 */
bool check_for_four_sensors()
{
    gpio_set_irq_enabled_with_callback(DETECT_1, GPIO_IRQ_EDGE_FALL, true, &sensor1);
    gpio_set_irq_enabled_with_callback(DETECT_4, GPIO_IRQ_EDGE_FALL, true, &sensor4);

    if (gpio_get(DETECT_2) == 1 && gpio_get(DETECT_3) == 1) {
        printf("%s: Enabling 4 detector mode", __FUNCTION__);
        g_fourSensorMode = true;
        gpio_set_irq_enabled_with_callback(DETECT_2, GPIO_IRQ_EDGE_RISE, true, &sensor2);
        gpio_set_irq_enabled_with_callback(DETECT_3, GPIO_IRQ_EDGE_RISE, true, &sensor3);
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

