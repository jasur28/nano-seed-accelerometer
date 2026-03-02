#pragma once

#include <QObject>
#include <QSerialPort>
#include <QElapsedTimer>

class SerialMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int samplesPerSecond READ samplesPerSecond NOTIFY statsChanged)

public:
    explicit SerialMonitor(QObject *parent = nullptr);

    Q_INVOKABLE void openPort(const QString &portName);
    Q_INVOKABLE void closePort();

    int samplesPerSecond() const;

signals:
    void statsChanged();
    void newSample(int ax, int ay, int az);

private slots:
    void onReadyRead();

private:
    QSerialPort m_serial;
    QByteArray m_buffer;

    qint64 m_bytesReceived = 0;
    int m_samplesPerSecond = 0;
    QElapsedTimer m_timer;
};
