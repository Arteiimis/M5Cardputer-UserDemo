/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_settings.h"
#include "assets/settings_big.h"
#include "assets/settings_small.h"
#include <apps/utils/audio/audio.h>
#include <apps/utils/common.h>
#include <apps/utils/theme.h>
#include <algorithm>
#include <cstdio>
#include <hal/hal.h>
#include <mooncake_log.h>

using namespace mooncake;

static int brightness_to_raw(int percent)
{
    percent = std::clamp(percent, 10, 100);
    return percent * 255 / 100;
}

static int volume_to_percent(int volume)
{
    return (volume * 100 + 127) / 255;
}

AppSettings::AppSettings()
{
    setAppInfo().name     = "Settings";
    setAppInfo().userData = new AppIcon_t(image_data_settings_big, image_data_settings_small);
}

AppSettings::~AppSettings()
{
    delete static_cast<AppIcon_t*>(getAppInfo().userData);
}

void AppSettings::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    load_settings();

    _key_slot_id = GetHAL().keyboard.onKeyEvent.connect(
        [this](const Keyboard::KeyEvent_t& keyEvent) { handle_key_event(keyEvent); });

    render();
}

void AppSettings::onRunning()
{
    if (GetHAL().homeButton.wasClicked()) {
        audio::play_random_tone();
        close();
    }
}

void AppSettings::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    if (_key_slot_id >= 0) {
        GetHAL().keyboard.onKeyEvent.disconnect(_key_slot_id);
        _key_slot_id = -1;
    }
    persist();
}

void AppSettings::load_settings()
{
    auto& settings = GetHAL().getSettings();

    _brightness = std::clamp(static_cast<int>(settings.GetInt("brightness", 100)), 10, 100);
    _volume     = std::clamp(static_cast<int>(settings.GetInt("volume", audio::DEFAULT_VOLUME)), 0, 255);
    _quiet      = settings.GetBool("quiet_mode", false);

    apply_brightness();
    apply_volume();
    audio::set_quiet_mode(_quiet);
}

void AppSettings::apply_brightness()
{
    GetHAL().display.setBrightness(brightness_to_raw(_brightness));
}

void AppSettings::apply_volume()
{
    audio::set_volume(_volume);
}

void AppSettings::persist()
{
    auto& settings = GetHAL().getSettings();
    settings.SetInt("brightness", _brightness);
    settings.SetInt("volume", _volume);
    settings.SetBool("quiet_mode", _quiet);
    settings.Commit();
}

void AppSettings::adjust(int delta)
{
    switch (_selected) {
        case ROW_BRIGHTNESS:
            _brightness = std::clamp(_brightness + delta * 5, 10, 100);
            apply_brightness();
            break;
        case ROW_VOLUME:
            _volume = std::clamp(_volume + delta * 15, 0, 255);
            apply_volume();
            break;
        case ROW_QUIET:
            _quiet = !_quiet;
            audio::set_quiet_mode(_quiet);
            break;
        default:
            return;
    }

    persist();
    render();
}

void AppSettings::handle_key_event(const Keyboard::KeyEvent_t& keyEvent)
{
    if (!keyEvent.state || keyEvent.isModifier) {
        return;
    }

    // KEY_UP/DOWN/LEFT/RIGHT are Fn-combinations on the Cardputer keyboard.
    // Also accept the unmodified keys so the page works without holding Fn.
    const bool up    = keyEvent.keyCode == KEY_UP || keyEvent.keyCode == KEY_SEMICOLON;
    const bool down  = keyEvent.keyCode == KEY_DOWN || keyEvent.keyCode == KEY_DOT;
    const bool left  = keyEvent.keyCode == KEY_LEFT || keyEvent.keyCode == KEY_COMMA;
    const bool right = keyEvent.keyCode == KEY_RIGHT || keyEvent.keyCode == KEY_SLASH;

    if (up) {
        _selected = (_selected + ROW_COUNT - 1) % ROW_COUNT;
        render();
    } else if (down) {
        _selected = (_selected + 1) % ROW_COUNT;
        render();
    } else if (left) {
        adjust(-1);
    } else if (right) {
        adjust(1);
    } else if (keyEvent.keyCode == KEY_ENTER || keyEvent.keyCode == KEY_SPACE) {
        if (_selected == ROW_QUIET) {
            adjust(1);
        }
    }
}

void AppSettings::render()
{
    auto& canvas = GetHAL().canvas;
    canvas.setBaseColor(THEME_COLOR_BG);
    canvas.fillScreen(THEME_COLOR_BG);
    canvas.setFont(FONT_BASIC);
    canvas.setTextSize(1);

    canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    canvas.drawString("Settings", 6, 2);

    const char* labels[ROW_COUNT] = {"Brightness", "Volume", "Quiet mode"};
    const int row_y[ROW_COUNT]    = {25, 47, 69};

    for (int i = 0; i < ROW_COUNT; i++) {
        const bool selected = (i == _selected);
        const int y         = row_y[i];

        if (selected) {
            canvas.fillRect(0, y - 3, canvas.width(), 20, (uint32_t)0x555555);
        }

        canvas.setTextColor(selected ? TFT_WHITE : TFT_LIGHTGREY, selected ? (uint32_t)0x555555 : THEME_COLOR_BG);
        canvas.drawString(labels[i], 6, y);

        char value[24];
        if (i == ROW_BRIGHTNESS) {
            snprintf(value, sizeof(value), "%d%%", _brightness);
        } else if (i == ROW_VOLUME) {
            snprintf(value, sizeof(value), "%d%%", volume_to_percent(_volume));
        } else {
            snprintf(value, sizeof(value), "%s", _quiet ? "On" : "Off");
        }

        const int value_width = canvas.textWidth(value);
        canvas.drawString(value, canvas.width() - value_width - 6, y);
    }

    canvas.setFont(&fonts::Font0);
    canvas.setTextColor(TFT_DARKGREY, THEME_COLOR_BG);
    canvas.drawString("UP/DOWN select  LEFT/RIGHT change", 2, 96);

    GetHAL().pushCanvas();
}
