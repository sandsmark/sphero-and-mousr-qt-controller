#pragma once

#include "BasicTypes.h"

#include <QDebug>
#include <QObject>
#include <QtEndian>
#include <cstdint>

namespace sphero {
namespace v2 {

template <typename PACKET>
QByteArray packetToByteArray(const PACKET &packet)
{
    QByteArray ret(reinterpret_cast<const char*>(&packet), sizeof(PACKET));
//    qToLittleEndian<char>(ret.data(), sizeof(PACKET), ret.data());
    qToBigEndian<char>(ret.data(), sizeof(PACKET), ret.data());
    return ret;
}

static constexpr char Escape = 0xAB;
static constexpr char EscapedEscape = 0x23;
static constexpr char StartOfPacket = 0x8D;
static constexpr char EscapedStartOfPacket = 0x03;
static constexpr char EndOfPacket = 0xD8;
static constexpr char EscapedEndOfPacket = 0x50;

template <typename PACKET>
QByteArray encode(const PACKET &packet)
{
    QByteArray data(1, StartOfPacket);
    for (const char c : packetToByteArray(packet)) {
        switch(c) {
        case Escape:
            data.append(Escape);
            data.append(EscapedEscape);
            break;
        case StartOfPacket:
            data.append(Escape);
            data.append(EscapedStartOfPacket);
            break;
        case EndOfPacket:
            data.append(Escape);
            data.append(EscapedEndOfPacket);
            break;
        default:
            data.append(c);
            break;
        }
    }


    uint8_t checksum = 0;
    for (int i=2; i<data.size(); i++) {
        checksum += data[i];
    }

    data.append(checksum xor 0xFF);
    data.append(EndOfPacket);

    qDebug() << " + Packet:";
    qDebug() << "  ] Device id:" << packet.deviceId;
    qDebug() << "  ] command id:" << packet.commandId;

    return data;
}
template <typename PACKET>
PACKET decode(const QByteArray &input)
{
    if (!input.startsWith(StartOfPacket) || !input.endsWith(EndOfPacket)) {
        qWarning() << "invalid start or end";
        return {};
    }

    QByteArray decoded;
    for (int i=1; i<input.size()-1; i++){
        const char c = input[i];
        if (c != Escape) {
            decoded.append(c);
            continue;
        }
        i++;
        switch(input[i]) {
        case EscapedEscape:
            decoded.append(Escape);
            break;
        case EscapedStartOfPacket:
            decoded.append(StartOfPacket);
            break;
        case EscapedEndOfPacket:
            decoded.append(EndOfPacket);
            break;
        default:
            qWarning() << "Invalid escape sequence" << c << input[i];
            decoded.append(input[i]);
            break;
        }
    }
// blahlah re-use the bytes-to-struct thing

}

#pragma pack(push,1)

struct BasePacket {
    uint8_t m_flags = 0xff;

    // These are only used on the big robot I don't remember the name of
    // The Mini etc. don't have several systems so they don't need this
    // They are unset/unused unless the appropriate flags are set
    uint8_t sourceID = 0;
    uint8_t targetID = 0;

    uint8_t m_deviceID = 0;
    uint8_t m_commandID = 0;

    uint8_t m_sequenceNumber = 0;

    // Only if flag is set
    uint8_t errorCode = 0;

    Q_GADGET
public:
    // TODO: share with v1 (mostly compatible, ish)
    enum Flags {
        HasErrorCode = 1 << 0,
        Synchronous = 1 << 1,
        ReportError = 1 << 2,
        ResetTimeout = 1 << 3,
        HasTargetAddress = 1 << 4,
        HasSourceAddress = 1 << 5,
        Reserved = 1 << 6,
        TwoByteFlags = 1 << 7,
    };

    enum CommandTarget : uint8_t {
        Internal = 0x00,

        PingPong = 0x10,
        Info = 0x11,
        LimbControl = 0x12,
        Battery = 0x13,
        MotorControl = 0x16,
        AnimationControl = 0x17,
        Sensors = 0x18,
        AVControl = 0x1a,
        Unknown = 0x1f,
        InvalidTarget = 0xFF
    };
    Q_ENUM(CommandTarget)

    enum class Error {
        Success = 0x00,
        BadDeviceID = 0x01,
        BadCommandID = 0x02,
        NotYetImplemented = 0x03,
        CommandisRestricted = 0x04,
        BadDataLength = 0x05,
        CommandFailed = 0x06,
        BadParameterValue = 0x07,
        Busy = 0x08,
        BadTargetID = 0x09,
        TargetUnavailable = 0x0A,
    };
    Q_ENUM(Error)

    enum SoulCommands : uint8_t {
        ReadSoulBlock = 0xf0,
        SoulAddXP = 0xf1
    };
    Q_ENUM(SoulCommands)

    BasePacket(const uint8_t deviceID, const uint8_t commandID) :
        m_deviceID(deviceID),
        m_commandID(commandID)
    {

    }

    bool isValid() const {
        return m_flags != 0;
    }

    bool isSynchronous() const {
        return m_flags & Synchronous;
    }

    void setSequenceNumber(const uint8_t number) {
        m_sequenceNumber = number;
    }
};


#pragma pack(pop)

} // namespace v2
} // namespace sphero
