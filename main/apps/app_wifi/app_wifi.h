/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <cstdint>
#include <hal/hal.h>
#include <string>
#include <vector>

class AppWifi : public mooncake::AppAbility {
public:
    AppWifi();
    ~AppWifi();

    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    enum State_t {
        STATE_SCANNING = 0,
        STATE_LIST,
        STATE_PASSWORD,
        STATE_CONNECTING,
        STATE_RESULT,
    };

    static constexpr int VISIBLE_ROWS = 4;
    static constexpr size_t INPUT_MAX = 64;

    std::vector<Hal::ScanResult_t> _networks;
    State_t _state  = STATE_SCANNING;
    int _selected   = 0;
    int _scroll     = 0;
    bool _connected = false;
    std::string _ssid;
    std::string _password;
    std::string _input;
    int _key_slot_id = -1;

    void scan();
    void render();
    void render_list();
    void render_password();
    void render_connecting();
    void render_result();
    void handle_key_event(const Keyboard::KeyEvent_t& keyEvent);
    void handle_list_key(const Keyboard::KeyEvent_t& keyEvent);
    void handle_password_key(const Keyboard::KeyEvent_t& keyEvent);
    void handle_result_key(const Keyboard::KeyEvent_t& keyEvent);
    void select_network();
    void start_connect();
    void save_credentials();
};
