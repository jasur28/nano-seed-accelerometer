#include "SerialMonitor.h"
#include <QDebug>
#include <cstring>

SerialMonitor::SerialMonitor(QObject *parent)
    : QObject(parent)
{
    connect(&m_serial, &QSerialPort::readyRead,
            this, &SerialMonitor::onReadyRead);

    m_timer.start();
}

void SerialMonitor::openPort(const QString &portName)
{
    if (m_serial.isOpen())
        m_serial.close();

    m_serial.setPortName(portName);
    m_serial.setBaudRate(115200);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial.open(QIODevice::ReadOnly))
    {
        qDebug() << "Failed to open port!";
        qDebug() << m_serial.errorString();
    }
    else
    {
        qDebug() << "Port opened successfully";
        m_buffer.clear();
        m_bytesReceived = 0;
        m_timer.restart();
    }
}

void SerialMonitor::closePort()
{
    if (m_serial.isOpen())
        m_serial.close();
}

int SerialMonitor::samplesPerSecond() const
{
    return m_samplesPerSecond;
}

void SerialMonitor::onReadyRead()
{
    QByteArray data = m_serial.readAll();
    m_buffer.append(data);
    m_bytesReceived += data.size();

    // Парсим каждый полный sample (6 байт)
    while (m_buffer.size() >= 6)
    {
        int16_t ax, ay, az;

        std::memcpy(&ax, m_buffer.constData(), 2);
        std::memcpy(&ay, m_buffer.constData() + 2, 2);
        std::memcpy(&az, m_buffer.constData() + 4, 2);

        emit newSample(ax, ay, az);

        m_buffer.remove(0, 6);
    }

    // Подсчет частоты
    if (m_timer.elapsed() >= 1000)
    {
        m_samplesPerSecond = static_cast<int>(m_bytesReceived / 6);
        emit statsChanged();

        m_bytesReceived = 0;
        m_timer.restart();
    }
}
