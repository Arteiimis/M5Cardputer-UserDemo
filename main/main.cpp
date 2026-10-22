/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <smooth_ui_toolkit.h>
#include <M5Unified.hpp>
#include <algorithm>
#include <mooncake_log.h>
#include <mooncake.h>
#include <apps.h>
#include <hal.h>

using namespace mooncake;
using namespace smooth_ui_toolkit;

extern "C" void app_main(void)
{
    // Setup logger
    mclog::set_level(mclog::level_debug);
    mclog::set_time_format(mclog::time_format_unix_milliseconds);

    // HAL init
    GetHAL().init();

    // Apply persisted device settings before any app draws or plays sound.
    {
        auto& settings = GetHAL().getSettings();
        int brightness = std::clamp(static_cast<int>(settings.GetInt("brightness", 100)), 10, 100);
        int volume     = std::clamp(static_cast<int>(settings.GetInt("volume", audio::DEFAULT_VOLUME)), 0, 255);
        GetHAL().display.setBrightness(brightness * 255 / 100);
        audio::set_volume(volume);
        audio::set_quiet_mode(settings.GetBool("quiet_mode", false));
        mclog::tagInfo("main", "settings: brightness={} volume={} quiet={}", brightness, volume,
                       settings.GetBool("quiet_mode", false));
    }

    // Setup ui hal
    ui_hal::on_delay([](uint32_t ms) { GetHAL().delay(ms); });
    ui_hal::on_get_tick([]() { return GetHAL().millis(); });

    // Install apps
    GetMooncake().installApp(std::make_unique<Launcher>());
    GetMooncake().installApp(std::make_unique<AppWifi>());
    GetMooncake().installApp(std::make_unique<AppRecord>());
    GetMooncake().installApp(std::make_unique<AppChat>());
    GetMooncake().installApp(std::make_unique<AppRemote>());
    GetMooncake().installApp(std::make_unique<AppREPL>());
    GetMooncake().installApp(std::make_unique<AppClock>());
    GetMooncake().installApp(std::make_unique<AppSettings>());
    GetMooncake().installApp(std::make_unique<AppKeyboard>());
    GetMooncake().installApp(std::make_unique<AppImu>());
    GetMooncake().installApp(std::make_unique<AppSdcard>());
    GetMooncake().installApp(std::make_unique<AppStringIRToolKit>());
    GetMooncake().installApp(std::make_unique<AppLoraChat>());
    GetMooncake().installApp(std::make_unique<AppGPS>());
    // GetMooncake().installApp(std::make_unique<AppDummy>());

    // Main loop
    audio::set_keyboard_sfx_enable(true);
    while (1) {
        GetHAL().feedTheDog();
        GetHAL().update();
        GetMooncake().update();
    }
}
