#pragma once

#include <cstdint>
#include <QObject>
#include <QDebug>

namespace mousr {

struct Autoplay {
    Q_GADGET

    Q_PROPERTY(uint8_t pauseTime READ pauseTime WRITE setPauseTime)
    Q_PROPERTY(uint8_t surface MEMBER m_surface)

public:
    static constexpr uint8_t defaultModes[12][12] = {
        //           . gamemode (look at the dot)
        {1, 0, 0, 0, 0, 0, 10, 10, 0, 0, 0, 0}, // OpenWander, Calm
        {1, 0, 0, 2, 0, 1, 20,  6, 0, 0, 0, 0}, // OpenWander, Aggressive
        {1, 0, 0, 0, 0, 0,  0,  0, 0, 0, 0, 0}, // OpenWander, Custom
        {1, 0, 0, 0, 1, 0, 15,  1, 0, 0, 0, 0}, // WallHugger, Calm
        {1, 0, 0, 1, 1, 1,  6,  0, 0, 0, 0, 0}, // WallHugger, Aggressive
        {1, 0, 0, 0, 1, 0,  0,  0, 0, 0, 0, 0}, // WallHugger, Custom
        {1, 0, 0, 0, 2, 0,  3,  0, 0, 0, 0, 0}, // BackAndForth, Calm
        {1, 0, 0, 1, 2, 1, 10,  0, 0, 0, 0, 0}, // BackAndForth, Aggressive
        {1, 0, 0, 0, 2, 0,  0,  0, 0, 0, 0, 0}, // BackAndForth, Custom
        {1, 0, 0, 0, 3, 0, 10,  6, 1, 0, 0, 0}, // Stationary, Calm
        {1, 0, 0, 2, 3, 0, 20,  3, 1, 0, 0, 0}, // Stationary, Aggressive
        {1, 0, 0, 0, 3, 0,  0,  0, 0, 0, 0, 0}, // Stationary, Custom
    };
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

    uint8_t enabled = 0; // 0

    uint8_t m_surface = 0; // 1
    uint8_t tail = 0; // 2

    uint8_t speed = 0; // 3

    uint8_t gameMode = 0; // 4
    uint8_t m_drivingMode = 0; // 5

    //GameMode gameMode = OffMode; // 4 --- also PresetMode?
    //PlayMode playMode = DriveStraight; // 5

    // Values used: 0, 5, 10, 20, 30, 60, 255
    uint8_t pauseFrequency = 0; // 6

    uint8_t pauseTimeOrConfined = 0; // 7

    // Values used for pause time; 3, 6, 10, 15, 20, 0 (all day mode)
    uint8_t pauseLengthOrBackUp = 0; // 8

    uint8_t allDay = 0; // 9

    uint8_t unknown1 = 0; // 10
    uint8_t unknown2 = 0; // 11
    uint8_t unknown3 = 0; // 12

    uint8_t m_responseTo = 0; // 13

    //Response response = Response::Nack; // 13
    uint8_t unknown4 = 0; // 14

    uint8_t pauseTime() const {
        switch(gameMode) {
        case Autoplay::WallHugger:
            return pauseFrequency;
        case Autoplay::OpenWander:
        case Autoplay::BackAndForth:
        case Autoplay::Stationary:
            return pauseTimeOrConfined;
        default:
            qWarning() << "unhandled game mode" << gameMode;
            return pauseTimeOrConfined;
        }
    }
    void setPauseTime(uint8_t time) {
        switch(gameMode) {
        case WallHugger:
            pauseFrequency = time;
            break;
        case OpenWander:
        case BackAndForth:
        case Stationary:
            pauseTimeOrConfined = time;
            break;
        default:
            qWarning() << "unhandled game mode" << gameMode;
            pauseTimeOrConfined = time;
        }
    }

    void setConfineArea(bool isConfined) {
        if (gameMode != WallHugger) {
            qWarning() << "Confined is only used in WallHugger mode, not" << gameMode;
        }
        pauseTimeOrConfined = isConfined;
    }

    bool backUp() const { return pauseLengthOrBackUp; }
    void setBackUp(bool isBackUp) {
        if (gameMode != GameMode::Stationary) {
            qWarning() << "BackUp only used in Stationary, not in" << gameMode;
        }
        pauseLengthOrBackUp = isBackUp;
    }

    Q_INVOKABLE DrivingMode drivingMode() const { return DrivingMode(m_drivingMode); }
    Q_INVOKABLE void setDrivingMode(uint8_t mode) {
        switch(gameMode) {
        case BackAndForth:
            m_drivingMode = DrivingMode(mode);
            break;
        default:
            qWarning() << "Driving mode not used in game mode" << gameMode;
            break;
        }

        m_drivingMode = DrivingMode(mode);
    }

    Q_INVOKABLE QString modeName() const {
        switch (gameMode) {
        case GameMode::OffMode:
            return "Off";
        default:
            break;
        }
        return "Unknown mode (" + QString::number(gameMode) + ")";
    }
    Surface surface() const { return Surface(m_surface); }
} __attribute__((packed));
static_assert(sizeof(Autoplay) == 15);

inline QDebug operator<<(QDebug debug, const Autoplay &c) {
    QDebugStateSaver saver(debug);
    debug.nospace() << "AutoPlayConfig ("
                    << "Enabled: " << c.enabled << ", "
                    << "Surface: " << c.surface() << ", "
                    << "Tail: " << Autoplay::TailType(c.tail) << ", "
                    << "Speed: " << c.speed << ", "
                    << "Game: " << c.modeName() << ", "
                    << "PauseFrequency: " << c.pauseFrequency << ", "
                    << "PauseTime: ";

    if (c.pauseTime() == 0) {
        debug.nospace() << "AllDay, ";
    } else {
        debug.nospace() << c.pauseTime() << ", ";
    }

    switch (c.gameMode) {
    case Autoplay::GameMode::WallHugger:
        debug.nospace() << "Confined " << c.pauseTime() << ", ";
        break;
    case Autoplay::GameMode::BackAndForth:
        debug.nospace() << "Driving " << c.drivingMode() << ", ";
        break;
    default:
        break;
    }
    debug.nospace() << "AllDay2: " << c.allDay << ", "
                    << "Unknown1: " << c.unknown1 << ", "
                    << "Unknown2: " << c.unknown2 << ", "
                    << "Unknown3: " << c.unknown3 << ", "
                    << "ResponseTo: " << c.m_responseTo << ", "
                    << "Unknown4: " << c.unknown4 << ", "
                    << ")";

    return debug.maybeSpace();
}

}//namespace mousr

Q_DECLARE_METATYPE(mousr::Autoplay*)
