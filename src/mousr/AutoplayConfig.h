#pragma once

#include <cstdint>
#include <QObject>
#include <QDebug>

namespace mousr {

struct AutoplayConfig {
    Q_GADGET

public:
    enum StandardGameModes {
        OpenWanderCalm = 0,
        OpenWanderAggressive = 1,
        OpenWanderCustom = 2,

        WallhuggerCalm = 3,
        WallhuggerAggressive = 4,
        WallhuggerCustom = 5,

        BackAndForthCalm = 6,
        BackAndForthAggressive = 7,
        BackAndForthCustom = 8,

        StationaryCalm = 9,
        StationaryAggressive = 10,
        StationaryCustom = 11,

        StandardGameModeCount = 12
    };
    Q_ENUM(StandardGameModes)
    static AutoplayConfig createConfig(StandardGameModes gamemode);

    enum Surface : uint8_t {
        Carpet = 0,
        BareFloor = 1,
    };
    Q_ENUM(Surface)

    enum TailType : uint8_t {
        BounceTail = 0,
        FlickTail = 1,
        ChaseTail = 2
    };
    Q_ENUM(TailType)

    enum PresetMode : uint8_t {
        CalmPreset = 0,
        AggressivePreset = 1,
        CustomMode = 2
    };
    Q_ENUM(PresetMode)

        // Don't know if this is used
    enum DrivingMode : uint8_t {
        InvalidDrivingMode = 255,
        DriveStraight = 0,
        Snaking = 1,
    };
    Q_ENUM(DrivingMode)

    enum GameMode : uint8_t {
        OffMode = 255,
        OpenWander = 0,
        WallHugger = 1,
        BackAndForth = 2,
        Stationary = 3
    };
    Q_ENUM(GameMode)

    static QStringList gameModeNames() {
        return {
            "Off",
            "Open Wandering",
            "Wall Hugger",
            "Back and Forth",
            "Stationary"
        };
    }

    ///////////////////////
    /// Actual contents ///
    ///////////////////////

    uint8_t enabled = 0; // 0
    uint8_t surface = 0; // 1
    uint8_t tail = 0; // 2
    uint8_t speed = 0; // 3
    uint8_t gameMode = 0; // 4

private: // don't touch these directly, different depending on game mode
    union { // 5
        uint8_t struggleWhenCaught;
        uint8_t m_drivingMode = 0; // Don't think this is used?
    };
    union { // 6
        // Values used: 0, 5, 10, 20, 30, 60, 255 (off)
        uint8_t pauseFrequency = 0;
        uint8_t pauseDurationWallHugger;
    };
    union { // 7
        // Values used for pause time; 3, 6, 10, 15, 20, 0 (all day mode)
        uint8_t pauseDuration = 0;
        uint8_t confineSingleRoom; // WallHugger
    };
    union { // 8
        uint8_t allDay = 0;
        uint8_t autoReverse; // Stationary
    };
    uint8_t stationaryAllDay = 0; // 9, only in stationary mode

public:
    /// Getters and setters
    uint8_t isPauseFrequencyAvailable() const { return gameMode != WallHugger; }
    uint8_t getPauseFrequency() const {
        if (!isPauseFrequencyAvailable()) {
            qWarning() << "Pause frequency not available";
        }
        return pauseFrequency;
    }
    void setPauseFrequency(const uint8_t frequency) {
        if (!isPauseFrequencyAvailable()) {
            qWarning() << "Pause frequency not available";
        }
        pauseFrequency = frequency;
    }

    uint8_t getPauseDuration() const {
        if (gameMode == WallHugger) {
            return pauseDurationWallHugger;
        } else {
            return pauseDuration;
        }
    }
    void setPauseDuration(uint8_t time) {
        if (gameMode == WallHugger) {
            pauseDurationWallHugger = time;
        } else {
            pauseDuration = time;
        }
    }

    bool getAllDay() const {
        if (gameMode == Stationary) {
            return stationaryAllDay;
        } else {
            return allDay;
        }
    }
    void setAllDay(const bool enabled) {
        if (gameMode == Stationary) {
            stationaryAllDay = enabled;
        } else {
            allDay = enabled;
        }
    }

    bool confineSingleRoomAvailable() const { return gameMode == WallHugger; }
    bool getConfineSingleRoom() const {
        if (!confineSingleRoomAvailable()) {
            qWarning() << "Confined is only used in WallHugger mode, not" << gameMode;
        }
        return confineSingleRoom;
    }
    void setConfineSingleRoom(bool isConfined) {
        if (!confineSingleRoomAvailable()) {
            qWarning() << "Confined is only used in WallHugger mode, not" << gameMode;
        }
        confineSingleRoom = isConfined;
    }

    bool autoReverseAvailable() const { return gameMode == Stationary; }
    bool getAutoReverse() const { return autoReverse; }
    void setAutoReverse(bool isAutoReverse) {
        if (!autoReverseAvailable()) {
            qWarning() << "AutoReverse only used in Stationary, not in" << gameMode;
        }
        autoReverse = isAutoReverse;
    }

    bool drivingModeAvailable() const { return gameMode == BackAndForth; }
    DrivingMode drivingMode() const { return DrivingMode(m_drivingMode); }
    void setDrivingMode(uint8_t mode) {
        if (!drivingModeAvailable()) {
            qWarning() << "AutoReverse only used in Stationary, not in" << gameMode;
        }
        m_drivingMode = mode;
    }

    GameMode getGameMode() const { return GameMode(gameMode); }
    void setGameMode(const GameMode mode) { gameMode = mode; }

    QString modeName() const {
        switch (gameMode) {
        case GameMode::OffMode:
            return "Off";
        case GameMode::OpenWander:
            return "OpenWander";
        case GameMode::WallHugger:
            return "WallHugger";
        case GameMode::BackAndForth:
            return "BackAndForth";
        case GameMode::Stationary:
            return "Stationary";
        default:
            break;
        }
        return "Unknown mode (" + QString::number(gameMode) + ")";
    }


    Surface getSurface() const { return Surface(surface); }
    void setSurface(const Surface newSurface) { surface = newSurface; }
    void setTailType(const TailType type) { tail = type; }
    TailType tailType() const { return TailType(tail); }

} __attribute__((packed));
static_assert(sizeof(AutoplayConfig) == 10);

inline QDebug operator<<(QDebug debug, const AutoplayConfig &c) {
    QDebugStateSaver saver(debug);
    debug.nospace() << "AutoPlayConfig ("
                    << "Enabled: " << c.enabled << ", "
                    << "Surface: " << c.getSurface() << ", "
                    << "Tail: " << AutoplayConfig::TailType(c.tail) << ", "
                    << "Speed: " << c.speed << ", "
                    << "Game: " << c.modeName() << ", "
                    << "PauseFrequency: " << c.getPauseFrequency() << ", "
                    << "PauseDuration: ";

    if (c.getPauseDuration() == 0) {
        debug.nospace() << "AllDay, ";
    } else {
        debug.nospace() << c.getPauseDuration() << ", ";
    }

    switch (c.gameMode) {
    case AutoplayConfig::GameMode::WallHugger:
        debug.nospace() << "Confined " << c.getConfineSingleRoom() << ", ";
        break;
    case AutoplayConfig::GameMode::BackAndForth:
        debug.nospace() << "Driving " << c.drivingMode() << ", ";
        break;
    default:
        break;
    }
    debug.nospace() << "AllDay2: " << c.getAllDay() << ", "
                    << ")";

    return debug.maybeSpace();
}


}//namespace mousr
