#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtNetwork/QUdpSocket>
#include <QNetworkDatagram>
#include <QTimer>
#include <QTime>
#include <QLabel>
#include "settingsdialog.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    typedef struct{
        uint8_t comID;
        uint8_t buffer[256];
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[50];
        uint8_t indexP;
        uint8_t nBytes;
        uint8_t indexr;
        uint8_t indexw;
    }_sDatos ;

    void openSerialPort(SettingsDialog *mySettings, QSerialPort *mySerial);

    void closeSerialPort(QSerialPort *mySerial);

    void myTimerOnTime();

    void dataRecived(_sDatos data);

    void decodeData(_sDatos data);

    void sendData(_sDatos data);

    void onRXUDP();

    void onRXUSB(QSerialPort *mySerial);

private slots:
    void on_pushButtonSend_clicked();

    void on_messageBox_currentIndexChanged(int index);

    void on_UDP_Conectar_clicked();

    void on_pushButton_sendWifi_clicked();

private:
    Ui::MainWindow *ui;

    #define USBID 0
    #define UDPID 1

    QSerialPort *mySerialUSB, *mySerialUSART;
    QTimer *myTimer;
    SettingsDialog *mySettingsUSB, *mySettingsUSART;
    QUdpSocket *myUDP;

    typedef enum{
        START,
        HEADER_1,
        HEADER_2,
        HEADER_3,
        NBYTES,
        TOKEN,
        PAYLOAD,
        DBGSTR
    }_eProtocolo;

    _eProtocolo estadoProtocolo;

    typedef enum{
        ACK=0x0D,
        ALIVE=0xF0,
        UDPCON=0xF1,
        ESPMSG=0xF2,
        IR_SENSOR=0xF3,
        ESPSETUP=0XF4,
        SETPID = 0xF5,
        DATAPID = 0xF6,
        MPUDATA = 0xF7,
        OTHERS
    }_eID;

    _eID estadoComandos;

    typedef enum{
        iALIVE,
        iTOESP,
        iESPMSG,
        iIR_SENSOR,
        iESPSETUP,
        iSETPID,
        iPIDERROR
    }_eIndex;

    _eIndex cmdIndex;

    _sDatos USBrxData, USBtxData, UDPrxData, UDPtxData;

    typedef union {
        float f32;
        int i32;
        int8_t i8[4];
        unsigned int ui32;
        unsigned short ui16[2];
        short i16[2];
        uint8_t ui8[4];
        char chr[4];
        unsigned char uchr[4];
    }_udat;

    _udat myWord;

    float error, vBase = 7000, velD, velI, deltaV;
    int16_t mpuAccX, mpuAccY, mpuAccZ, mpuGyroX, mpuGyroY, mpuGyroZ;
    float mpuAXg, mpuAYg, mpuAZg, mpuAXg_last, mpuAYg_last, mpuAZg_last, mpuGX, mpuGY, mpuGZ, velX, velY, velZ;

    QHostAddress targetIP;

    uint16_t targetPort;

};
#endif // MAINWINDOW_H
