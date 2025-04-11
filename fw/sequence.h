// Motor control

#pragma once
#include "fraise.h"
#include "settings.h"
#include "motor.h"
#include "math.h"

class Sequence {
private:
    int last_time_ms = to_ms_since_boot(get_absolute_time());
    bool run = false;
    bool state = false;
    Settings &settings;
    Motor *motors;
    int motors_count;
public:
    Sequence(Settings &s, Motor *m, int count):
        settings(s), motors(m), motors_count(count) {}

    void update() {
        if(!run) return;
        int duration_ms;
        if(state) duration_ms = settings.get_run_ms();
        else duration_ms = settings.get_stop_ms();
        if(to_ms_since_boot(get_absolute_time()) < last_time_ms + duration_ms) return;
        int transition_ms;
        if(state) transition_ms = settings.get_decel_ms();
        else transition_ms = settings.get_accel_ms();
        last_time_ms = to_ms_since_boot(get_absolute_time()) + transition_ms;
        int speed = settings.get_speed();
        if(state) speed = 0;
        for(int i = 0; i < 4; i++) motors[i].goto_pwm_ms(speed, transition_ms);
        state = !state;
    }

    void set_run(bool r) {
        run = r;
        state = false;
        if(!run) for(int i = 0; i < 4; i++) motors[i].goto_pwm_ms(0, settings.get_decel_ms());
        else {
            last_time_ms = to_ms_since_boot(get_absolute_time()) - settings.get_stop_ms();
        }
    }

    bool get_run() {
        return run;
    }
};

