#pragma once

#include <QMetaEnum>

namespace EnumHelper {

template <typename T> static const char *toKey(const T val) {
    const QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    return metaEnum.valueToKey(val);
}

template <typename T> static QString toString(const T val) {
    return QString::fromUtf8(EnumHelper::toKey(val));
}

}


// Fuck endiannes (todo: use qFromBigEndian)
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
