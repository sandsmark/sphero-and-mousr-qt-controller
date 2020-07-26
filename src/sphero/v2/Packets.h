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

template <typename PACKET>
PACKET byteArrayToPacket(const QByteArray &data, bool *ok)
{
    if (size_t(data.size()) < sizeof(PACKET)) {
        qWarning() << "Invalid packet size, need" << sizeof(PACKET) << "but got" << data.size();
        *ok = false;
        return {};
    }
    PACKET ret;
    memcpy(&ret, data.data(), sizeof(PACKET));
    *ok = true;
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
    QByteArray raw = packetToByteArray(packet);
    uint8_t checksum = 0;
    for (const char c : raw) {
        checksum += c;
    }
    raw.append(checksum xor 0xFF);

    QByteArray encoded(1, StartOfPacket);
    for (const char c : raw) {
        switch(c) {
        case Escape:
            encoded.append(Escape);
            encoded.append(EscapedEscape);
            break;
        case StartOfPacket:
            encoded.append(Escape);
            encoded.append(EscapedStartOfPacket);
            break;
        case EndOfPacket:
            encoded.append(Escape);
            encoded.append(EscapedEndOfPacket);
            break;
        default:
            encoded.append(c);
            break;
        }
    }



    encoded.append(EndOfPacket);

    qDebug() << " + Packet:";
    qDebug() << "  ] Device id:" << packet.deviceID;
    qDebug() << "  ] command id:" << packet.commandID;

    return encoded;
}
template <typename PACKET>
PACKET decode(const QByteArray &input, bool *ok)
{
    if (!input.startsWith(StartOfPacket) || !input.endsWith(EndOfPacket)) {
        qWarning() << "invalid start or end";
        *ok = false;
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
            *ok = false;
            qWarning() << "Invalid escape sequence" << c << input[i];
            qDebug() << input.toHex(':');
            return;
        }
    }

    uint8_t checksum = 0;
    for (int i=0; i<decoded.size() - 1; i++) {
        checksum += decoded[i];
    }

    if (!decoded.endsWith(checksum)) {
        qWarning() <<  "invalid checksum" << decoded.back() << "expected" << checksum;
        *ok = false;
        return {};
    }

    decoded.chop(1); // remove the checksum at the end

    return byteArrayToPacket<PACKET>(decoded, ok);
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
