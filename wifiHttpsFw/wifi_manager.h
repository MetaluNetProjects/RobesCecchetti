#pragma once

#include "fraise.h"
#include "http_session.h"

// pico_https
#include "tls_listener.h"
#include "listener.h"
#include "request_handler.h"
#include "pico/cyw43_arch.h"
#include "lwip/timeouts.h"

#ifndef WIFI_FORCE_STA
// pico_ap
extern "C" {
#include "dhcpserver.h"
#include "dnsserver.h"
}
#endif

#define printf fraise_printf

class WifiManager {
private:
    int sta_switch_pin;
    Settings &settings;
    bool last_pin;
    absolute_time_t pin_timeout = at_the_end_of_time;
#ifndef WIFI_FORCE_STA
    dhcp_server_t dhcp_server;
    dns_server_t dns_server;
#endif
    //Listener tcp_listener;

    enum {NONE, AP, STA} state = NONE;

    void switch_changed() {
        /*switch(state) {
        case NONE: break;
        case AP: 
            RequestHandler::closeAll();
            dns_server_deinit(&dns_server);
            dhcp_server_deinit(&dhcp_server);
            cyw43_arch_disable_ap_mode();
            cyw43_arch_deinit();
            break;
        case STA:
            RequestHandler::closeAll();
            cyw43_arch_disable_sta_mode();
            cyw43_arch_deinit();
            break;
        }*/
        state = NONE;
#ifndef WIFI_FORCE_STA
        if(gpio_get(sta_switch_pin) == 1) { // switch isn't closed: go to AP mode
            printf("l switching to AP mode\n");
            cyw43_arch_enable_ap_mode(settings.get_ap_ssid(), settings.get_ap_password(), CYW43_AUTH_WPA2_AES_PSK);

            cyw43_arch_lwip_begin();
            struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
            netif_set_hostname(n, settings.get_hostname());
            netif_set_up(n);
            cyw43_arch_lwip_end();

            ip4_addr_t ip;
            ip4_addr_t mask;
            IP4_ADDR(ip_2_ip4(&ip), 192, 168, 4, 1);
            IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

            // Start the dhcp server
            dhcp_server_init(&dhcp_server, &ip, &mask);
            // Start the dns server
            dns_server_init(&dns_server, &ip);
            state = AP;
        } else 
#endif
        { // station mode
            printf("l switching to STATION mode\n");
            cyw43_arch_enable_sta_mode();

            cyw43_arch_lwip_begin();
            struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
            netif_set_hostname(n, settings.get_hostname());
            netif_set_up(n);
            cyw43_arch_lwip_end();

            printf("Connecting to Wi-Fi...\n");
            if (cyw43_arch_wifi_connect_timeout_ms(settings.get_sta_ssid(), settings.get_sta_password(), CYW43_AUTH_WPA2_AES_PSK, 30000)) {
                printf("failed to connect.\n");
                return;
            }
            printf("Connected.\n");
            state = STA;
        }

        // Now configure the server:
        cyw43_wifi_pm(&cyw43_state, cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 2000, 1, 1, 1));

        start_logging_server("build/" PICO_BOARD "/docker/hello_world/hello_world_https", PICO_BOARD, "1.0", 15000);
        trace("Starting");
        // Listens on 433 for https
        //TLSListener *tls_listener = new TLSListener();
        //tls_listener->listen(443, RequestHandler::create);

        // Listens on 80 for http
        Listener *tcp_listener = new Listener();
        tcp_listener->listen(80, RequestHandler::create);
    }
public:
    WifiManager(int pin, Settings &s): sta_switch_pin(pin), settings(s) {}
    void init() {
        gpio_init(sta_switch_pin);
        gpio_set_dir(sta_switch_pin, GPIO_IN);
        gpio_pull_up(sta_switch_pin);
        sleep_ms(50); // wait for the pin to settle
        last_pin = gpio_get(sta_switch_pin);
        pin_timeout = at_the_end_of_time;
        switch_changed();
    }

    void update() {
        if(last_pin != gpio_get(sta_switch_pin)) {
            last_pin = gpio_get(sta_switch_pin);
            pin_timeout = make_timeout_time_ms(1000);
        }
        if(time_reached(pin_timeout)) {
            pin_timeout = at_the_end_of_time;
            printf("l wifi switch_changed\n");
            sleep_ms(100);
            switch_changed();
            last_pin = gpio_get(sta_switch_pin);
        }

        //cyw43_arch_lwip_begin();
        cyw43_arch_poll();
        sys_check_timeouts();
        update_watchdog();
        //cyw43_arch_lwip_end();
    }
};

