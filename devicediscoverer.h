#ifndef DEVICEDISCOVERER_H
#define DEVICEDISCOVERER_H

#include <QObject>

class DeviceDiscoverer : public QObject
{
    Q_OBJECT
public:
    explicit DeviceDiscoverer(QObject *parent = nullptr);

signals:

public slots:
};

#endif // DEVICEDISCOVERER_H
