#include "devicediscoverer.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);


    qmlRegisterSingletonType<DeviceDiscoverer>("com.iskrembilen", 1, 0, "DeviceDiscoverer", [](QQmlEngine *, QJSEngine*) -> QObject* {
        return new DeviceDiscoverer;
    });

    QQmlApplicationEngine engine(":/main.qml");

    return app.exec();
}

