/**************************************************************************************
*  Tracker GPS (prototype)  v1.0                                                      *
*                                                                                     *
*  Author: Daniel Gómez López                                                         *
*                                                                                     *
*                                                                                     *
*  This code processes the information sent by the ESP32 in JSON format through       *
*  MQTT, allowing interaction with different widgets and a map to locate the position *
*  of the device and know its speed, among other parameters.                          *
***************************************************************************************/

#include "mainwindow.h"
#include <QtWidgets/QApplication>
#include <QApplication>
#include <QtQuick/QQuickView>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlComponent>
#include <qqml.h>
#include <QJsonObject>
#include <QJsonDocument>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
