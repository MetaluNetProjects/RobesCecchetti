#pragma once

#include "fraise.h"
#include "fraise_eeprom.h"

class Settings {
private:
    // data:
    int run_ms;
    int stop_ms;
    int accel_ms;
    int decel_ms;
    int speed; // 0 - 32767

    void send_param(int which) {
        switch(which) {
            case 1: fraise_printf("run_ms %d\n", run_ms); break;
            case 2: fraise_printf("stop_ms %d\n", stop_ms); break;
            case 3: fraise_printf("accel_ms %d\n", accel_ms); break;
            case 4: fraise_printf("decel_ms %d\n", decel_ms); break;
            case 5: fraise_printf("speed %d\n", speed); break;
        }
    }

public:
    void eeprom_declare() {
        eeprom_declare_data((char*)this, sizeof(Settings));
    }
    void receivebytes(const char* data, uint8_t len) {
        char command = fraise_get_uint8();
        data++; len--;
        switch(command) {
            case 1: run_ms = fraise_get_uint32(); break;
            case 2: stop_ms = fraise_get_uint32(); break;
            case 3: accel_ms = fraise_get_uint32(); break;
            case 4: decel_ms = fraise_get_uint32(); break;
            case 5: speed = fraise_get_uint32(); break;

            case 200: send_param(fraise_get_uint8()); break;
            case 255: // save
                if(!strncmp(data, "EEPROM_SAVE", MIN(len, sizeof("EEPROM_SAVE")))) {
                    eeprom_save();
                    fraise_printf("l eeprom saved\n");
                } else {
                    fraise_printf("l eesave error: %*s", len, data);
                }
                break;
        }
        if(command < 200) send_param(command);
    }
    const int get_run_ms() {
        return run_ms;
    }
    const int get_stop_ms() {
        return stop_ms;
    }
    const int get_accel_ms() {
        return accel_ms;
    }
    const int get_decel_ms() {
        return decel_ms;
    }
    const int get_speed() {
        return speed;
    }
};

