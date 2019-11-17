#include "devicediscoverer.h"
#include "MousrHandler.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterUncreatableType<MousrHandler>("com.iskrembilen", 1, 0, "DeviceHandler", "Only valid when discovered");

    qmlRegisterSingletonType<DeviceDiscoverer>("com.iskrembilen", 1, 0, "DeviceDiscoverer", [](QQmlEngine *, QJSEngine*) -> QObject* {
        return new DeviceDiscoverer;
    });

    QQmlApplicationEngine engine(":qml/main.qml");
//    QQmlApplicationEngine engine(":RobotView.qml");

    return app.exec();
}

