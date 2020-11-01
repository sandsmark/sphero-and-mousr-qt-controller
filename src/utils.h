#pragma once

#include <QDebug>
#include <QMetaEnum>
#include <QtEndian>

namespace EnumHelper {

template <typename T> static const char *toKey(const T val) {
    const QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    return metaEnum.valueToKey(val);
}

template <typename T> static QString toString(const T val) {
    return QString::fromUtf8(EnumHelper::toKey(val));
}

}


template<typename T>
static inline T parseBytes(const char **data)
{
    T ret;
    memcpy(reinterpret_cast<char*>(&ret), reinterpret_cast<const void*>(*data), sizeof(T));
    *data += sizeof(T);

    return ret;
}

template<typename T>
static inline void setBytes(char **data, const T val)
{
    memcpy(reinterpret_cast<void*>(*data), reinterpret_cast<const char*>(&val), sizeof(T));
    *data += sizeof(T);
}

template <typename PACKET>
QByteArray packetToByteArray(const PACKET &packet)
{
    QByteArray ret(reinterpret_cast<const char*>(&packet), sizeof(PACKET));
    return ret;
}

// data needs to have correct endinanness before calling this cuz im lazy
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
