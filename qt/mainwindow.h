#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>                  // Main application window
#include <QVariant>                     // Store values ​​of any data type
#include <QWidget>                      // For application widgets
#include <QtSerialPort/qserialport.h>   // To interact with devices attached to the serial port
#include "qmqtt.h"                      // To work with MQTT in QT
#include <QObject>                      // For slots and signals
#include <QDebug>                       // To print debugging messages to the console
#include <QJsonObject>                  // To define JSON objects
#include <QJsonDocument>                // To read and write JSON documents
#include <qqml.h>                       // To work with QML in QT

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT                            // Macro: Slots and Signals functionality
    QML_ELEMENT                         // Macro: QML functionality
public:
    MainWindow(QWidget *parent = nullptr);  // Constuctor
    ~MainWindow();                          // Destructor

private:
    Ui::MainWindow *ui;
    int transactionCount;
    QMQTT::Client *_client;
    bool MQTTconnected;

    // MQTT
    void startClient(); // Init MQTT
    void activateRunMQTTButton();

private slots:

    // MQTT
    void on_runMQTTButton_clicked();
    void on_pushButton_clicked();
    void onMQTT_Received(const QMQTT::Message &message);
    void onMQTT_Connected(void);
    void onMQTT_subscribed(const QString &topic);
    void onMQTT_error(QMQTT::ClientError err);

    // Track
    void on_visibleTrackButton_clicked(bool checked);
    void on_visibleTrackButton_2_clicked();

    // Alarms
    void circleAlarm();
    void speedAlarm();
    void on_circleButton_ONOFF_clicked(bool checked);
    void on_speedButton_ONOFF_clicked(bool checked);

signals:

    // Map signals
    void setCenter(QVariant, QVariant);
    void addMarker(QVariant, QVariant);
    void setVisibleTrack(QVariant);
    void setResetTrack(QVariant);
    // Location signals
    void setLAT(QVariant);
    void setLON(QVariant);
    // Perimeter Alarm signal
    void setLimit(QVariant);
};

#endif // MAINWINDOW_H



