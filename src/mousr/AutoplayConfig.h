#pragma once

#include <cstdint>
#include <QObject>
#include <QDebug>

namespace mousr
{

struct Autoplay {
    Q_GADGET

public:
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
        Wander = 0,
        CornerFinder = 1,
        BackAndForth = 2,
        Stationary = 3
    };
    Q_ENUM(GameMode)

    struct Config
    {
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
            case Autoplay::CornerFinder:
                return pauseFrequency;
            case Autoplay::Wander:
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
            case CornerFinder:
                pauseFrequency = time;
            case Wander:
            case BackAndForth:
            case Stationary:
                pauseTimeOrConfined = time;
            default:
                qWarning() << "unhandled game mode" << gameMode;
                pauseTimeOrConfined = time;
            }
        }

        void setConfineArea(bool isConfined) {
            if (gameMode != CornerFinder) {
                qWarning() << "Confined is only used in CornerFinder mode, not" << gameMode;
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

        DrivingMode drivingMode() const { return DrivingMode(m_drivingMode); }
        void setDrivingMode(uint8_t mode) {
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

        QString modeName() const {
            switch (gameMode) {
            case GameMode::OffMode:
                return "Off";
            default:
                break;
            }
            return "Unknown mode";
        }
        Surface surface() const { return Surface(m_surface); }
    } __attribute__((packed));
};

}//namespace mousr
