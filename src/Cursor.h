#pragma once

#include <QObject>
#include <QCursor>

class Cursor : public QObject
{
    Q_OBJECT
public:
    explicit Cursor(QObject *parent = nullptr);

public slots:
    void setPosition(const int x, const int y) { m_cursor.setPos(x, y); }

private:
    QCursor m_cursor;
};

