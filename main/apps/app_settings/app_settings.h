/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <cstdint>
#include <hal/hal.h>

class AppSettings : public mooncake::AppAbility {
public:
    AppSettings();
    ~AppSettings();

    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    enum Row_t {
        ROW_BRIGHTNESS = 0,
        ROW_VOLUME,
        ROW_QUIET,
        ROW_COUNT,
    };

    int _selected    = ROW_BRIGHTNESS;
    int _brightness  = 100;
    int _volume      = 90;
    bool _quiet      = false;
    int _key_slot_id = -1;

    void load_settings();
    void apply_brightness();
    void apply_volume();
    void persist();
    void render();
    void handle_key_event(const Keyboard::KeyEvent_t& keyEvent);
    void adjust(int delta);
};
