#pragma once

#include "BasicTypes.h"

#include <QObject>
#include <cstdint>

namespace sphero {

#pragma pack(push,1)

struct CommandPacketHeader {
    enum TimeoutHandling : uint8_t {
        KeepTimeout = 0,
        ResetTimeout = 1 << 0
    };

    enum SynchronousType : uint8_t {
        Asynchronous = 0,
        Synchronous = 1 << 1
    };

    enum CommandTarget : uint8_t {
        Internal = 0x00,
        Bootloader = 0x01,
        HardwareControl = 0x02,
    };

    enum BootloaderCommand : uint8_t {
        BeginReflash = 0x02,
        HereIsPage = 0x03,
        JumpToMain = 0x04,
        IsPageBlank = 0x05,
        EraseUserConfig = 0x06
    };

    enum InternalCommand : uint8_t {
        Ping = 0x01,
        GetVersion = 0x02,
        SetBtName = 0x10,
        GetBtName = 0x11,
        SetAutoReconnect = 0x12,
        GetAutoReconnect = 0x13,
        GetPwrState = 0x20,
        SetPwrNotify = 0x21,
        Sleep = 0x22,
        GetVoltageTrip = 0x23,
        SetVoltageTrip = 0x24,
        SetInactiveTimeout = 0x22,
        GotoBl = 0x30,
        RunL1Diags = 0x40,
        RunL2Diags = 0x41,
        ClearCounters = 0x42,
        AssignCounter = 0x50,
        PollTimes = 0x51
    };

    enum HardwareCommand : uint8_t {
        SetHeading = 0x01,
        SetStabilization = 0x02,
        SetRotationRate = 0x03,
        SetAppConfigBlk = 0x04,
        GetAppConfigBlk = 0x05,
        SelfLevel = 0x09,
        SetDataStreaming = 0x11,
        ConfigureCollisionDetection = 0x12,
        ConfigureLocator = 0x13,
        GetLocatorData = 0x15,
        SetRGBLed = 0x20,
        SetBackLED = 0x21,
        GetRGBLed = 0x22,
        Roll = 0x30,
        Boost = 0x31,
        RawMotorValues = 0x33,
        SetMotionTimeout = 0x34,
        SetOptionFlags = 0x35,
        GetOptionFlags = 0x36,
        SetNonPersistentOptionFlags = 0x37,
        GetNonPersistentOptionFlags = 0x38,
        GetConfigurationBlock = 0x40,
        SetDeviceMode = 0x42,
        SetConfigurationBlock = 0x43,
        GetDeviceMode = 0x44,
        SetFactoryDeviceMode = 0x45,
        GetSSB = 0x46,
        SetSSB = 0x47,
        RefillBank = 0x48,
        BuyConsumable = 0x49,
        AddXp = 0x4C,
        LevelUpAttribute = 0x4D,
        RunMacro = 0x50,
        SaveTempMacro = 0x51,
        SaveMacro = 0x52,
        DelMacro = 0x53,
        GetMacroStatus = 0x56,
        SetMacroStatus = 0x57,
        SaveTempMacroChunk = 0x58,
        InitMacroExecutive = 0x54,
        AbortMacro = 0x55,
        GetConfigBlock = 0x40,
        OrbBasicEraseStorage = 0x60,
        OrbBasicAppendFragment = 0x61,
        OrbBasicExecute = 0x62,
        OrbBasicAbort = 0x63,
        OrbBasicCommitRamProgramToFlash = 0x65,
        RemoveCores = 0x71,
        SetSSBUnlockFlagsBlock = 0x72,
        ResetSoulBlock = 0x73,
        ReadOdometer = 0x75,
        WritePersistentPage = 0x90
    };
    enum SoulCommands : uint8_t {
        ReadSoulBlock = 0xf0,
        SoulAddXP = 0xf1
    };

    const uint8_t magic = 0xFF;
    uint8_t flags = 0xFC;

    uint8_t deviceID = 0;
    uint8_t commandID = 0;
    uint8_t sequenceNumber = 0;
    uint8_t dataLength = 0;
};

struct SleepCommandPacket
{
    CommandPacketHeader header; // Just free-wheeling it here

    enum SleepType {
        SleepHighPower = 0x00,
        SleepDeep = 0x01,
        SleepLowPower = 0x02
    };

    uint16_t sleepTime;
    uint8_t wakeMacro;
    uint16_t basicLineNumber;
};

struct RotateCommandPacket
{
    CommandPacketHeader header;
    float rate;
};

struct SetOptionsCommandPacket
{
    CommandPacketHeader header;

    enum Options : uint32_t {
        /// Prevent Sphero from going to sleep when placed in the charger and connected over Bluetooth
        PreventSleepInCharger = 1 << 0,

        /// Enable Vector Drive, when Sphero is stopped and a new roll command is issued */
        EnableVectorDrive = 1 << 1,

        /// Disable self-leveling when Sphero is inserted into the charger
        DisableSelfLevelInCharger = 1 << 2,

        /// Force the tail LED always on
        TailLightAlwaysOn = 1 << 3,

        /// Enable motion timeouts, DID 0x02, CID 0x34
        EnableMotionTimeout = 1 << 4,

        DemoMode = 1 << 5,

        LightDoubleTap = 1 << 6,
        HeavyDoubleTap = 1 << 7,
        GyroMaxAsync = 1 << 8, // new in firmware 1.47 (sphero)
        EnableSoul   = 1 << 9,
        SlewRawMotors  = 1 << 10,
    };

    uint32_t optionsBitmask;
};

struct DataStreamingCommandPacket
{
    enum SourceMask : uint32_t {
        NoMask = 0x00000000,
        LeftMotorBackEMFFiltered = 0x00000060,
        RightMotorBackEMFFiltered = 0x00000060,

        MagnetometerZFiltered = 0x00000080,
        MagnetometerYFiltered = 0x00000100,
        MagnetometerXFiltered = 0x00000200,

        GyroZFiltered = 0x00000400,
        GyroYFiltered = 0x00000800,
        GyroXFiltered = 0x00001000,

        AccelerometerZFiltered = 0x00002000,
        AccelerometerYFiltered = 0x00004000,
        AccelerometerXFiltered = 0x00008000,

        IMUYawAngleFiltered = 0x00010000,
        IMURollAngleFiltered = 0x00020000,
        IMUPitchAngleFiltered = 0x00040000,

        LeftMotorBackEMFRaw =  0x00600000,
        RightMotorBackEMFRaw = 0x00600000,

        GyroFilteredAll = 0x00001C00,
        IMUAnglesFilteredAll = 0x00070000,
        AccelerometerFilteredAll = 0x0000E000,

        MotorPWM =  0x00100000 | 0x00080000, // -2048 -> 2047

        Magnetometer = 0x02000000,

        GyroZRaw = 0x04000000,
        GyroYRaw = 0x08000000,
        GyroXRaw = 0x10000000,

        AccelerometerZRaw = 0x20000000,
        AccelerometerYRaw = 0x40000000,
        AccelerometerXRaw = 0x80000000,
        AccelerometerRaw =  0xE0000000,

        AllSources = 0xFFFFFFFF,
    };

    uint16_t maxRateDivisor = 10;
    uint16_t framesPerPacket = 1;
    uint32_t sourceMask = AllSources;
    uint8_t packetCount = 0; // 0 == forever
};

struct DataStreamingCommandPacket1_17 : DataStreamingCommandPacket
{
    enum SourceMask : uint64_t {
        Quaternion0 = 0x80000000,
        Quaternion1 = 0x40000000,
        Quaternion2 = 0x20000000,
        Quaternion3 = 0x10000000,
        LocatorX = 0x0800000,
        LocatorY = 0x0400000,

        // TODO:
        //MaskAccelOne                  = 0x0200000000000000,

        VelocityX = 0x0100000,
        VelocityY = 0x0080000,

        LocatorAll = 0x0D80000,
        QuaternionAll = 0xF0000000,

        AllSourcesHigh = 0xFFFFFFFF,

    };
    uint32_t sourceMaskHighBits = AllSourcesHigh; // firmware >= 1.17
};

static_assert(sizeof(CommandPacketHeader) == 6);

#pragma pack(pop)

} // namespace sphero
