#include "AutoplayConfig.h"

#include <QtEndian>

static constexpr uint8_t defaultModes[12][9] = {
    //           . gamemode (look at the dot)
    {1, 0, 0, 0, 0, 0, 10, 10, 0}, // OpenWander, Calm
    {1, 0, 0, 2, 0, 1, 20,  6, 0}, // OpenWander, Aggressive
    {1, 0, 0, 0, 0, 0,  0,  0, 0}, // OpenWander, Custom
    {1, 0, 0, 0, 1, 0, 15,  1, 0}, // WallHugger, Calm
    {1, 0, 0, 1, 1, 1,  6,  0, 0}, // WallHugger, Aggressive
    {1, 0, 0, 0, 1, 0,  0,  0, 0}, // WallHugger, Custom
    {1, 0, 0, 0, 2, 0,  3,  0, 0}, // BackAndForth, Calm
    {1, 0, 0, 1, 2, 1, 10,  0, 0}, // BackAndForth, Aggressive
    {1, 0, 0, 0, 2, 0,  0,  0, 0}, // BackAndForth, Custom
    {1, 0, 0, 0, 3, 0, 10,  6, 1}, // Stationary, Calm
    {1, 0, 0, 2, 3, 0, 20,  3, 1}, // Stationary, Aggressive
    {1, 0, 0, 0, 3, 0,  0,  0, 0}, // Stationary, Custom
};

namespace mousr {

AutoplayConfig AutoplayConfig::createConfig(AutoplayConfig::StandardGameModes gamemode)
{
    size_t index = size_t(gamemode);
    if (index >= sizeof(defaultModes)) {
        qWarning() << gamemode << "out of range";
        index = 0;
    }
    static_assert(sizeof(defaultModes[0]) + 1 == sizeof(AutoplayConfig));

    AutoplayConfig config;
    memcpy(reinterpret_cast<uint8_t*>(&config) + 1, defaultModes[index], sizeof(defaultModes[index]));

    return config;
}

} // namespace mousr
