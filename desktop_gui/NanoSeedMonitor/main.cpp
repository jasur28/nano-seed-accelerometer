#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "SerialMonitor.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    SerialMonitor monitor;

    QQmlApplicationEngine engine;

    // Передаём объект в QML
    engine.rootContext()->setContextProperty("serialMonitor", &monitor);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("NanoSeedMonitor", "Main");

    return app.exec();
}
