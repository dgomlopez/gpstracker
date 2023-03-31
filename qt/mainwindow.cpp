#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QQuickItem>       // Display QML content in the user interface.
#include <QSerialPort>      // Communicate with a serial port on the computer.
#include <QSerialPortInfo>
#include <QMessageBox>      // Display alert messages in the user interface.
#include <QJsonObject>      // Used to work with JSON objects and documents.
#include <QJsonDocument>
#include <stdint.h>
#include <stdbool.h>
#include <QDebug>           // Used to print debugging messages to the console.
#include <QGeoCoordinate>   // Used to work with location data and geographic coordinates.
#include <QGeoPositionInfo>

QJsonValue espLat;
QJsonValue espLon;
QJsonValue espHour;
QJsonValue espMinutes;
QJsonValue espMonth;
QJsonValue espYear;
QJsonValue espVoltageBat;
QJsonValue espPercBat;
QJsonValue espSpeed;

double espLat_qml;
double espLon_qml;
double circleRadio;
double circleCenterLAT;
double circleCenterLON;
double speed;

MainWindow::MainWindow(QWidget *parent) // Constructor

    : QMainWindow(parent),
    ui(new Ui::MainWindow)

{
    ui->setupUi(this);  // Interface elements are configured.
    setWindowTitle(tr("Control interface"));
    _client=new QMQTT::Client(QHostAddress::LocalHost, 1883);
    connect(_client, SIGNAL(connected()), this, SLOT(onMQTT_Connected())); // Emitted when the MQTT client successfully connects to the MQTT server.
    connect(_client, SIGNAL(received(const QMQTT::Message &)), this, SLOT(onMQTT_Received(const QMQTT::Message &))); // Emitted when an MQTT message is received.
    connect(_client, SIGNAL(subscribed(const QString &)), this, SLOT(onMQTT_subscribed(const QString &))); // Emitted when the MQTT client subscribes to an MQTT topic.
    connect(_client, SIGNAL(error(const QMQTT::ClientError)), this, SLOT(onMQTT_error(QMQTT::ClientError))); // Emitted when an error occurs in the MQTT connection.
    MQTTconnected=false;

    ui->quickWidget->setSource(QUrl(QStringLiteral("qrc:/map.qml"))); // Prepare and display the map.
    ui->quickWidget->show();

    // Signals and slots to interact with the map, interface elements and the ESP32.
    auto obj = ui->quickWidget->rootObject();
    connect(this, SIGNAL(setCenter(QVariant, QVariant)), obj, SLOT(setCenter(QVariant, QVariant))); // Update the center of the map
    connect(this, SIGNAL(addMarker(QVariant, QVariant)), obj, SLOT(addMarker(QVariant, QVariant))); // Update the marker of the map
    connect(this, SIGNAL(setLAT(QVariant)), obj, SLOT(setLAT(QVariant)));                           // Update latitude
    connect(this, SIGNAL(setLON(QVariant)), obj, SLOT(setLON(QVariant)));                           // Update longitude
    connect(this, SIGNAL(setLimit(QVariant)), obj, SLOT(setLimit(QVariant)));                       // Update perimeter limit
    connect(this, SIGNAL(setVisibleTrack(QVariant)), obj, SLOT(setVisibleTrack(QVariant)));         // Update Visible track or not
    connect(this, SIGNAL(setResetTrack(QVariant)), obj, SLOT(setResetTrack(QVariant)));             // Update Reset track or not

}

MainWindow::~MainWindow()   // Destructor
{
    delete ui;
}

// MQTT
void MainWindow::on_pushButton_clicked() // Status Button
{
    ui->statusLabel->setText(tr(""));
}

void MainWindow::on_runMQTTButton_clicked() // Init MQTT and interface
{
    startClient();
}

void MainWindow::activateRunMQTTButton()    // Enabled runMQTTButton
{
    ui->runMQTTButton->setEnabled(true);
}

// ************************* Perimeter Alarm functionality
void MainWindow::circleAlarm()
{

    QGeoCoordinate geo_center;  // Set coordinates for perimeter center.
    geo_center.setLatitude(circleCenterLAT);
    geo_center.setLongitude(circleCenterLON);
    QGeoCoordinate geo_current; // Obtain current coordinates.
    geo_current.setLatitude(espLat_qml);
    geo_current.setLongitude(espLon_qml);
    double distanceInMeters = geo_center.distanceTo(geo_current);   // Distance between the center of the previous perimeter and the current location.

    qDebug() << "Distance" << distanceInMeters << "m";


    if (circleRadio != (ui->circleLcd->value())){ // Check if the radius has changed. If so, then the center of the perimeter is updated.
        circleCenterLAT = espLat_qml;
        circleCenterLON = espLon_qml;
        circleRadio = ui->circleLcd->value();
    }

    if (distanceInMeters >= circleRadio){ // If the distance between the center of the perimeter and the current location is greater than the radius, an alarm is set.
        qDebug() << "Perimeter Alarm: activated";
        ui->led->setChecked(1);
    }else{
        qDebug() << "Perimeter Alarm: disactivated";
        ui->led->setChecked(0);
    }

    emit setLimit(circleRadio); // The radius is sent to the map (qml)

}

void MainWindow::on_circleButton_ONOFF_clicked(bool checked) // Pressing the radio button sets the center of the perimeter.
{
    if (checked){
        circleCenterLAT = espLat_qml;
        circleCenterLON = espLon_qml;

    }else{
        circleRadio = 0;  // The perimeter of the map is removed.
        emit setLimit(circleRadio);
        ui->led->setChecked(0);
    }
}

// ************************* Speed Alarm functionality
void MainWindow::speedAlarm()   // Check if the current speed exceeds the set speed and set the alarm.
{
    qDebug() << "Speed" << espSpeed.toDouble() << "km/h";
    double speedLimit = ui->speedLcd->value();

    if (speed >= speedLimit){
        qDebug() << "Speed Alarm: activated";
        ui->ledSpeed->setChecked(1);
    }else{
        qDebug() << "Speed Alarm: disactivated";
        ui->ledSpeed->setChecked(0);
    }
}

void MainWindow::on_speedButton_ONOFF_clicked(bool checked) // Pressing the speed button sets the speed limit.
{
    if (checked){

    }else{
        qDebug() << "Speed Alarm: disactivated";
        ui->ledSpeed->setChecked(0);
    }
}

// ************************* Visible Track functionality
void MainWindow::on_visibleTrackButton_clicked(bool checked)    // Set if the track is visible.
{
    if (checked){
        emit setVisibleTrack(1);
    }else{
        emit setVisibleTrack(0);
    }
}

void MainWindow::on_visibleTrackButton_2_clicked()  // Reset track (to start a new)
{
    emit setResetTrack(1);
}

// MQTT

void MainWindow::startClient()                  // Init MQTT
{
    _client->setHostName(ui->leHost->text());   // Obtain broker MQTT name
    _client->setPort(1883);                     // MQTT port (without TLS)
    _client->setKeepAlive(300);                 // Send messages at least once every 300 ms
    _client->setCleanSession(true);             // Messages are deleted when disconnected
    _client->connectToHost();                   // Connect MQTT client with MQTT server
}

/* -----------------------------------------------------------
 MQTT Client Slots
 -----------------------------------------------------------*/
void MainWindow::onMQTT_Connected()
{
    QString topic(ui->topic->text());

    ui->runMQTTButton->setEnabled(false);
    ui->statusLabel->setText(tr("Connecting to server...r"));

    MQTTconnected=true;
    _client->subscribe(topic,0); // Subscribe to the set topic
}

void MainWindow::onMQTT_subscribed(const QString &topic) // Shows the subscribed topic in Status
{
     ui->statusLabel->setText(tr("subscribed %1").arg(topic));
}

void MainWindow::onMQTT_error(QMQTT::ClientError err) // Shows information about problems with MQTT in Status
{
    QString errInfo;
    switch(err) {
    // 0	The connection was refused by the peer (or timed out).
    case QAbstractSocket::ConnectionRefusedError:
        errInfo = tr("Connection Refused");
        break;
    //	 1	The remote host closed the connection. Note that the client socket (i.e., this socket) will be closed after the remote close notification has been sent.
    case QAbstractSocket::RemoteHostClosedError:
        errInfo = tr("Remote Host Closed");
        break;
    //	2	The host address was not found.
    case QAbstractSocket::HostNotFoundError:
        errInfo = tr("Host Not Found Error");
        break;
    // 	3	The socket operation failed because the application lacked the required privileges.
    case QAbstractSocket::SocketAccessError:
        errInfo = tr("Socket Access Error");
        break;
    // 	4	The local system ran out of resources (e.g., too many sockets).
    case QAbstractSocket::SocketResourceError:
        errInfo = tr("Socket Resource Error");
        break;
    // 	5	The socket operation timed out.
    case QAbstractSocket::SocketTimeoutError:
        errInfo = tr("Socket Timeout Error");
        break;
    default:
        errInfo = tr("Socket Error");
    }
    ui->statusLabel->setText(tr("%1").arg(errInfo));
    ui->runMQTTButton->setEnabled(true);
}

void MainWindow::onMQTT_Received(const QMQTT::Message &message) // Messages are received by MQTT and the information is processed
{
    bool previousblockinstate;
    if (MQTTconnected)
    {
        QJsonParseError error;
        QJsonDocument mensaje=QJsonDocument::fromJson(message.payload(),&error);

        if ((error.error==QJsonParseError::NoError)&&(mensaje.isObject())) // Check that the message has a valid format
        {

            QJsonObject objeto_json=mensaje.object();
            espLat=objeto_json["latitude"];
            espLon=objeto_json["longitude"];
            espSpeed=objeto_json["speed"];
            espHour=objeto_json["hour"];
            espMinutes=objeto_json["minutes"];
            espMonth=objeto_json["month"];
            espYear=objeto_json["year"];
            espVoltageBat=objeto_json["voltageBat"];
            espPercBat=objeto_json["percBat"];

            // Location
            if(espLat.isDouble()){
                previousblockinstate=ui->lcdLat->blockSignals(true);
                ui->lcdLat->display(espLat.toDouble());
                ui->lcdLat->blockSignals(previousblockinstate);

                espLat_qml = espLat.toDouble();

                emit setLAT(espLat_qml);
            }

            if(espLon.isDouble()){
                previousblockinstate=ui->lcdLon->blockSignals(true);
                ui->lcdLon->display(espLon.toDouble());
                ui->lcdLon->blockSignals(previousblockinstate);

                espLon_qml = espLon.toDouble();

                emit setLON(espLon_qml);
            }

            // Battery
            if(espVoltageBat.isDouble()){
                previousblockinstate=ui->lcdBatVolt->blockSignals(true);
                ui->lcdBatVolt->display(espVoltageBat.toDouble());
                ui->lcdBatVolt->blockSignals(previousblockinstate);
            }

            if(espPercBat.isDouble()){
                previousblockinstate=ui->lcdBatPerc->blockSignals(true);
                ui->lcdBatPerc->display(espPercBat.toDouble());
                ui->lcdBatPerc->blockSignals(previousblockinstate);
            }

            // Speed
            if(espSpeed.isDouble()){
                previousblockinstate=ui->lcdSpeed->blockSignals(true);
                ui->lcdSpeed->display(espSpeed.toDouble());
                ui->lcdSpeed->blockSignals(previousblockinstate);

                speed = espSpeed.toDouble();
            }

            if (ui->speedButton_ONOFF->isChecked()){
                speedAlarm();
            }

            // Perimeter
            if (ui->circleButton_ONOFF->isChecked()){
                circleAlarm();
            }

            // Center and Marker
            emit setCenter(espLat_qml, espLon_qml);
            emit addMarker(espLat_qml, espLon_qml);
        }
    }

}




