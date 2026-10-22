/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_wifi.h"
#include "assets/scan_big.h"
#include "assets/scan_small.h"
#include <apps/utils/audio/audio.h>
#include <apps/utils/common.h>
#include <apps/utils/theme.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <mooncake_log.h>

using namespace mooncake;

AppWifi::AppWifi()
{
    setAppInfo().name     = "WiFi";
    setAppInfo().userData = new AppIcon_t(image_data_scan_big, image_data_scan_small);
}

AppWifi::~AppWifi()
{
    delete static_cast<AppIcon_t*>(getAppInfo().userData);
}

void AppWifi::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    _state     = STATE_SCANNING;
    _selected  = 0;
    _scroll    = 0;
    _connected = GetHAL().isWifiConnected();

    _key_slot_id = GetHAL().keyboard.onKeyEvent.connect(
        [this](const Keyboard::KeyEvent_t& keyEvent) { handle_key_event(keyEvent); });

    scan();
}

void AppWifi::onRunning()
{
    if (GetHAL().homeButton.wasClicked()) {
        audio::play_random_tone();
        close();
    }
}

void AppWifi::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    if (_key_slot_id >= 0) {
        GetHAL().keyboard.onKeyEvent.disconnect(_key_slot_id);
        _key_slot_id = -1;
    }
}

void AppWifi::scan()
{
    _state = STATE_SCANNING;
    render();

    GetHAL().wifiInit();
    _networks.clear();
    GetHAL().wifiScan(_networks);

    _state    = STATE_LIST;
    _selected = 0;
    _scroll   = 0;
    render();
}

void AppWifi::render()
{
    switch (_state) {
        case STATE_SCANNING:
            GetHAL().canvas.setFont(FONT_BASIC);
            GetHAL().canvas.setTextSize(1);
            GetHAL().canvas.fillScreen(THEME_COLOR_BG);
            GetHAL().canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
            GetHAL().canvas.drawString("WiFi", 6, 2);
            GetHAL().canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
            GetHAL().canvas.drawString("Scanning...", 6, 42);
            GetHAL().canvas.drawString("Please wait", 6, 62);
            GetHAL().pushCanvas();
            break;

        case STATE_LIST:
            render_list();
            break;

        case STATE_PASSWORD:
            render_password();
            break;

        case STATE_CONNECTING:
            render_connecting();
            break;

        case STATE_RESULT:
            render_result();
            break;
    }
}

void AppWifi::render_list()
{
    auto& canvas = GetHAL().canvas;
    canvas.setBaseColor(THEME_COLOR_BG);
    canvas.fillScreen(THEME_COLOR_BG);
    canvas.setFont(FONT_BASIC);
    canvas.setTextSize(1);

    canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    canvas.drawString("WiFi", 6, 2);
    canvas.setFont(&fonts::Font0);
    canvas.setTextColor(TFT_DARKGREY, THEME_COLOR_BG);
    canvas.drawString("R rescan", canvas.width() - 48, 6);

    canvas.setFont(FONT_BASIC);

    if (_networks.empty()) {
        canvas.setTextColor(TFT_RED, THEME_COLOR_BG);
        canvas.drawString("No networks found", 6, 42);
        canvas.setTextColor(TFT_DARKGREY, THEME_COLOR_BG);
        canvas.drawString("R to rescan", 6, 62);
        GetHAL().pushCanvas();
        return;
    }

    for (int row = 0; row < VISIBLE_ROWS; row++) {
        const int index = _scroll + row;
        if (index >= static_cast<int>(_networks.size())) {
            break;
        }

        const auto& network = _networks[index];
        const int y         = 20 + row * 18;
        const bool selected = index == _selected;

        if (selected) {
            canvas.fillRect(0, y - 2, canvas.width(), 18, (uint32_t)0x555555);
        }

        canvas.setTextColor(selected ? TFT_WHITE : TFT_LIGHTGREY, selected ? (uint32_t)0x555555 : THEME_COLOR_BG);

        char rssi[8];
        snprintf(rssi, sizeof(rssi), "%d", network.rssi);
        canvas.drawString(rssi, 4, y);

        std::string ssid = network.ssid;
        if (ssid.size() > 20) {
            ssid = ssid.substr(0, 19) + "~";
        }
        canvas.drawString(ssid.c_str(), 32, y);

        if (network.authmode != 0) {
            canvas.drawString("*", canvas.width() - 10, y);
        }
    }

    canvas.setFont(&fonts::Font0);
    canvas.setTextColor(TFT_DARKGREY, THEME_COLOR_BG);
    canvas.drawString("ENTER connect  UP/DOWN select", 2, 96);

    GetHAL().pushCanvas();
}

void AppWifi::render_password()
{
    auto& canvas = GetHAL().canvas;
    canvas.setBaseColor(THEME_COLOR_BG);
    canvas.fillScreen(THEME_COLOR_BG);
    canvas.setFont(FONT_BASIC);
    canvas.setTextSize(1);

    canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    canvas.drawString("WiFi password", 6, 2);

    std::string ssid = _ssid;
    if (ssid.size() > 22) {
        ssid = ssid.substr(0, 21) + "~";
    }
    canvas.setTextColor(TFT_CYAN, THEME_COLOR_BG);
    canvas.drawString(ssid.c_str(), 6, 22);

    canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
    std::string masked(std::min<size_t>(_input.size(), 20), '*');
    canvas.drawString(masked.c_str(), 6, 45);
    canvas.drawString("_", 6 + canvas.textWidth(masked.c_str()), 45);

    canvas.setFont(&fonts::Font0);
    canvas.setTextColor(TFT_DARKGREY, THEME_COLOR_BG);
    canvas.drawString("ENTER connect  ESC back", 2, 96);

    GetHAL().pushCanvas();
}

void AppWifi::render_connecting()
{
    auto& canvas = GetHAL().canvas;
    canvas.setBaseColor(THEME_COLOR_BG);
    canvas.fillScreen(THEME_COLOR_BG);
    canvas.setFont(FONT_BASIC);
    canvas.setTextSize(1);

    canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    canvas.drawString("Connecting...", 6, 2);

    std::string ssid = _ssid;
    if (ssid.size() > 22) {
        ssid = ssid.substr(0, 21) + "~";
    }
    canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
    canvas.drawString(ssid.c_str(), 6, 42);

    GetHAL().pushCanvas();
}

void AppWifi::render_result()
{
    auto& canvas = GetHAL().canvas;
    canvas.setBaseColor(THEME_COLOR_BG);
    canvas.fillScreen(THEME_COLOR_BG);
    canvas.setFont(FONT_BASIC);
    canvas.setTextSize(1);

    canvas.setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    canvas.drawString("WiFi", 6, 2);

    if (_connected) {
        canvas.setTextColor(TFT_GREEN, THEME_COLOR_BG);
        canvas.drawString("Connected", 6, 28);

        std::string ssid = _ssid;
        if (ssid.size() > 22) {
            ssid = ssid.substr(0, 21) + "~";
        }
        canvas.setTextColor(TFT_WHITE, THEME_COLOR_BG);
        canvas.drawString(ssid.c_str(), 6, 50);
    } else {
        canvas.setTextColor(TFT_RED, THEME_COLOR_BG);
        canvas.drawString("Connection failed", 6, 28);
        canvas.setTextColor(TFT_LIGHTGREY, THEME_COLOR_BG);
        canvas.drawString("ENTER edit password  ESC list", 6, 50);
    }

    canvas.setFont(&fonts::Font0);
    canvas.setTextColor(TFT_DARKGREY, THEME_COLOR_BG);
    canvas.drawString("R rescan  HOME exit", 2, 96);

    GetHAL().pushCanvas();
}

void AppWifi::handle_key_event(const Keyboard::KeyEvent_t& keyEvent)
{
    if (!keyEvent.state || keyEvent.isModifier) {
        return;
    }

    switch (_state) {
        case STATE_LIST:
            handle_list_key(keyEvent);
            break;
        case STATE_PASSWORD:
            handle_password_key(keyEvent);
            break;
        case STATE_RESULT:
            handle_result_key(keyEvent);
            break;
        default:
            break;
    }
}

void AppWifi::handle_list_key(const Keyboard::KeyEvent_t& keyEvent)
{
    if (_networks.empty()) {
        if (keyEvent.keyCode == KEY_R) {
            scan();
        }
        return;
    }

    const bool up   = keyEvent.keyCode == KEY_UP || keyEvent.keyCode == KEY_SEMICOLON;
    const bool down = keyEvent.keyCode == KEY_DOWN || keyEvent.keyCode == KEY_DOT;

    if (up) {
        _selected = (_selected + static_cast<int>(_networks.size()) - 1) % static_cast<int>(_networks.size());
    } else if (down) {
        _selected = (_selected + 1) % static_cast<int>(_networks.size());
    } else if (keyEvent.keyCode == KEY_R) {
        scan();
        return;
    } else if (keyEvent.keyCode == KEY_ENTER) {
        select_network();
        return;
    } else {
        return;
    }

    if (_selected < _scroll) {
        _scroll = _selected;
    } else if (_selected >= _scroll + VISIBLE_ROWS) {
        _scroll = _selected - VISIBLE_ROWS + 1;
    }
    render();
}

void AppWifi::handle_password_key(const Keyboard::KeyEvent_t& keyEvent)
{
    switch (keyEvent.keyCode) {
        case KEY_ENTER:
            start_connect();
            break;
        case KEY_ESC:
            _state = STATE_LIST;
            render();
            break;
        case KEY_BACKSPACE:
            if (!_input.empty()) {
                _input.pop_back();
                render();
            }
            break;
        case KEY_SPACE:
            if (_input.size() < INPUT_MAX) {
                _input += ' ';
                render();
            }
            break;
        default:
            if (keyEvent.keyName && strlen(keyEvent.keyName) == 1 && _input.size() < INPUT_MAX) {
                _input += keyEvent.keyName;
                render();
            }
            break;
    }
}

void AppWifi::handle_result_key(const Keyboard::KeyEvent_t& keyEvent)
{
    if (keyEvent.keyCode == KEY_R) {
        scan();
    } else if (keyEvent.keyCode == KEY_ESC) {
        _state = STATE_LIST;
        render();
    } else if (keyEvent.keyCode == KEY_ENTER) {
        if (_connected) {
            _state = STATE_LIST;
            render();
        } else {
            // Keep the failed password editable so a typo can be corrected.
            _input = _password;
            _state = STATE_PASSWORD;
            render();
        }
    }
}

void AppWifi::select_network()
{
    const auto& network = _networks[_selected];
    _ssid               = network.ssid;

    auto& settings = GetHAL().getSettings();
    if (settings.GetString("wifi_ssid", "") == _ssid) {
        _password = settings.GetString("wifi_password", "");
    } else {
        _password.clear();
    }

    if (network.authmode == 0) {
        // Open network; no password prompt.
        _password.clear();
        start_connect();
    } else if (!_password.empty()) {
        // Saved network; Enter connects immediately.
        start_connect();
    } else {
        _input.clear();
        _state = STATE_PASSWORD;
        render();
    }
}

void AppWifi::start_connect()
{
    if (_state == STATE_PASSWORD) {
        _password = _input;
    }

    _state = STATE_CONNECTING;
    render();

    _connected = GetHAL().wifiConnect(_ssid, _password);

    if (_connected) {
        save_credentials();
    }

    _state = STATE_RESULT;
    render();
}

void AppWifi::save_credentials()
{
    auto& settings = GetHAL().getSettings();
    settings.SetString("wifi_ssid", _ssid);
    settings.SetString("wifi_password", _password);
    settings.Commit();
}
