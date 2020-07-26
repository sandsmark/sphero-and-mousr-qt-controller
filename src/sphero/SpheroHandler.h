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

// BB-8 at least
static constexpr int manufacturerID = 12339;
bool isValidRobot(const QString &name, const QString &address);

QString displayName(const QString &id);

class SpheroHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)

    Q_PROPERTY(QString statusString READ statusString NOTIFY connectedChanged)

    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectedChanged)

    Q_PROPERTY(QString deviceType READ deviceType CONSTANT)

    Q_PROPERTY(float signalStrength READ signalStrength NOTIFY rssiChanged)
    Q_PROPERTY(RobotType robotType MEMBER m_robotType CONSTANT)

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)

    Q_PROPERTY(bool autoStabilize READ autoStabilize WRITE setAutoStabilize NOTIFY autoStabilizeChanged)
    Q_PROPERTY(bool detectCollisions READ detectCollisions WRITE setDetectCollisions NOTIFY detectCollisionsChanged)

public:
    enum class RobotType {
        Unknown,
        BB8,
        ForceBand,
        LMQ,
        Ollie,
        SPRK,
        R2D2,
        R2Q5,
        BB9E,
        SpheroMini,
        WeBall,
    };
    Q_ENUM(RobotType)

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
    void setAngle(int angle);
    int speed() const { return m_speed; }
    int angle() const { return m_angle; }

    void setAutoStabilize(const bool enabled);
    bool autoStabilize() const { return m_autoStabilize; }

    void setDetectCollisions(const bool enabled);
    bool detectCollisions() const { return m_detectCollisions; }

    void goToSleep();
    void goToDeepSleep();
    void enablePowerNotifications();

    void setEnableAsciiShell(const bool enabled);

    void faceLeft();
    void faceRight();
    void faceForward();
    void boost(const int angle, int duration);

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
    void sendCommand(const uint8_t deviceId, const uint8_t commandID, const QByteArray &data = QByteArray());

    template<typename PACKET> void sendCommand(const PACKET &packet) {
        sendCommand(PACKET::deviceId, PACKET::commandId, packetToByteArray(packet));
    }


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

    struct RobotDefinition {
        RobotDefinition() = default;
        explicit RobotDefinition(const RobotType type);

        QBluetoothUuid mainService;
        QBluetoothUuid radioService;
        QBluetoothUuid batteryService;

        QBluetoothUuid commandsCharacteristic;

        QByteArray radioPassword;
    };

    RobotType m_robotType = RobotType::Unknown;

    QMap<uint8_t, QPair<uint8_t, uint8_t>> m_pendingSyncRequests;
    uint8_t m_nextSequenceNumber = 0;
};

using RobotType = SpheroHandler::RobotType;
RobotType typeFromName(const QString &name);

} // namespace sphero
