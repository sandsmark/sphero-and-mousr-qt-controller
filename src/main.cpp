#include "devicediscoverer.h"
#include "mousr/MousrHandler.h"
#include "sphero/SpheroHandler.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterUncreatableType<mousr::MousrHandler>("com.iskrembilen", 1, 0, "DeviceHandler", "Only valid when discovered");
    qmlRegisterUncreatableType<sphero::SpheroHandler>("com.iskrembilen", 1, 0, "SpheroHandler", "Only valid when discovered");

    qmlRegisterSingletonType<DeviceDiscoverer>("com.iskrembilen", 1, 0, "DeviceDiscoverer", [](QQmlEngine *, QJSEngine*) -> QObject* {
        return new DeviceDiscoverer;
    });

    QQmlApplicationEngine engine(":qml/main.qml");
//    QQmlApplicationEngine engine(":RobotView.qml");

    return app.exec();
}

