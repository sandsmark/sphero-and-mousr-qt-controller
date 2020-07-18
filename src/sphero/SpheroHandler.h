#pragma once

#include "BasicTypes.h"

#include <QObject>
#include <QPointer>
#include <QBluetoothUuid>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyController>
#include <QColor>

class QLowEnergyController;
class QBluetoothDeviceInfo;

namespace sphero {

class SpheroHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)

    Q_PROPERTY(QString statusString READ statusString NOTIFY connectedChanged)

    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectedChanged)

    Q_PROPERTY(QString deviceType READ deviceType CONSTANT)

    Q_PROPERTY(float signalStrength READ signalStrength NOTIFY rssiChanged)
    Q_PROPERTY(SpheroType robotType MEMBER m_robotType CONSTANT)

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)

    Q_PROPERTY(bool autoStabilize READ autoStabilize WRITE setAutoStabilize NOTIFY autoStabilizeChanged)
    Q_PROPERTY(bool detectCollisions READ detectCollisions WRITE setDetectCollisions NOTIFY detectCollisionsChanged)

public:
    enum class SpheroType {
        Unknown,
        Bb8,
        ForceBand,
        Lmq,
        Ollie,
        Spkr,
        R2d2,
        R2Q5,
        Bb9e,
        SpheroMini,
    };
    Q_ENUM(SpheroType)

public:
    explicit SpheroHandler(const QBluetoothDeviceInfo &deviceInfo, QObject *parent);
    ~SpheroHandler();

    bool isConnected();

    QString statusString();
    void writeData(const QByteArray &data);

    static QString deviceType() { return "Sphero"; }

    float signalStrength() const {
        return rssiToStrength(m_rssi);
    }

    void setColor(const QColor &color) { setColor(color.red(), color.green(), color.blue()); }
    void setColor(const int r, const int g, const int b);
    QColor color() const { return m_color; }

    void setSpeedAndAngle(int angle, int speed);

    void setSpeed(int speed) { setSpeedAndAngle(speed, m_angle); }
    void setAngle(int angle) { setSpeedAndAngle(m_speed, angle); }
    int speed() const { return m_speed; }
    int angle() const { return m_angle; }

    void setAutoStabilize(const bool enabled);
    bool autoStabilize() const { return m_autoStabilize; }

    void setDetectCollisions(const bool enabled);
    bool detectCollisions() const { return m_detectCollisions; }

    void goToSleep();

signals:
    void connectedChanged();
    void rssiChanged();
    void disconnected(); // TODO
    void statusMessageChanged(const QString &message);

    void colorChanged();
    void angleChanged();
    void speedChanged();
    void autoStabilizeChanged();
    void detectCollisionsChanged();

public slots:
    void disconnectFromRobot();

private slots:
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerError(QLowEnergyController::Error newError);

    void onServiceDiscoveryFinished();
    void onMainServiceChanged(QLowEnergyService::ServiceState newState);
    void onServiceError(QLowEnergyService::ServiceError error);

    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void onRadioServiceChanged(QLowEnergyService::ServiceState newState);

private:
    bool sendRadioControlCommand(const QBluetoothUuid &characteristicUuid, const QByteArray &data);
    void sendCommand(const uint8_t deviceId, const uint8_t commandID, const QByteArray &data);


    QPointer<QLowEnergyController> m_deviceController;

    QLowEnergyCharacteristic m_commandsCharacteristic;

    QLowEnergyDescriptor m_readDescriptor;

    QPointer<QLowEnergyService> m_mainService;
    QPointer<QLowEnergyService> m_radioService;

    QByteArray m_receiveBuffer;

    QString m_name;
    int8_t m_rssi = 0;

    int m_angle = 0;
    int m_speed = 0;
    bool m_autoStabilize = false;
    bool m_detectCollisions = false;

    QColor m_color;

    SpheroType m_robotType = SpheroType::Unknown;
};

} // namespace sphero
