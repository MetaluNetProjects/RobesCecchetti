// Robes firmware

#define BOARD pico
#include "fraise.h"
#include "string.h"
#include "fraise_eeprom.h"

#include "pico/rand.h"
//#include "remote.h"
#include "motor.h"
#include "settings.h"
#include "sequence.h"

#define printf fraise_printf

Settings settings;
Motor motors[4]{ {0, 1, 2}, {4, 5, 6}, {8, 9, 10}, {12, 13, 14} };

Sequence sequence(settings, motors, 4);

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
int ledPeriod = 250;

bool monitor = true;

void setup() {
    eeprom_load();
}

void print_pwm(bool force = false) {
    static absolute_time_t next_time = get_absolute_time();
    static int16_t pwm[4];
    bool changed = force;
    if((!force) && (!time_reached(next_time))) return;
    next_time = make_timeout_time_ms(100);
    for(int i = 0; i < 4; i++) {
        if(pwm[i] != motors[i].get_pwm()) {
            pwm[i] = motors[i].get_pwm();
            changed = true;
        }
    }
    if(changed) printf("M %d %d %d %d\n", pwm[0], pwm[1], pwm[2], pwm[3]);
}

void loop() {
    static absolute_time_t nextLed;
    static bool led = false;

    if(time_reached(nextLed)) {
        gpio_put(LED_PIN, led = !led);
        nextLed = make_timeout_time_ms(ledPeriod);
    }

    //remote_update();
    sequence.update();
    for(int i = 0; i < 4; i++) motors[i].update();
    if(monitor) print_pwm();
}

void fraise_receivebytes(const char *data, uint8_t len) {
    uint8_t command = fraise_get_uint8();
    //fraise_printf("l get command %d\n", command);
    switch(command) {
    case 1:
        ledPeriod = (int)fraise_get_uint8() * 10;
        break;
    case 2:
        print_pwm();
        break;
    case 3: {
            int16_t pwm = fraise_get_int16();
            int16_t ms = fraise_get_int16();
            for(int i = 0; i < 4; i++) motors[i].goto_pwm_ms(pwm, ms);
        }
        break;
    case 40: {
            bool run = (fraise_get_uint8() != 0);
            sequence.set_run(run);
            printf("run %d\n", run);
        }
        break;
    case 45: {
            printf("run %d\n", sequence.get_run());
            printf("monitor %d\n", monitor);
            print_pwm(true);
        }
        break;
    case 50:
        settings.receivebytes(data + 1, len - 1);
        break;
    case 100:
        fraise_print_status();
        break;
    default:
        printf("rcvd ");
        for(int i = 0; i < len; i++) printf("%d ", (uint8_t)data[i]);
        putchar('\n');
    }
}

bool string_equal(const char *in, uint8_t len, const char *str) {
    return len >= strlen(str) && !strncmp(in, str, strlen(str));
}

void fraise_receivechars(const char *data, uint8_t len) {
    const char eesave_str[] = "SAVE_EEPROM";
    if(data[0] == 'E') { // Echo
        printf("E%s\n", data + 1);
    }
    /*if(data[0] == 'R') { // Remote command
        remote_command(data + 1);
    }*/
    else if(string_equal(data, len, eesave_str)) {
        printf("l saving eeprom\n");
        eeprom_save();
    }
}

void eeprom_declare_main() {
    settings.eeprom_declare();
}

