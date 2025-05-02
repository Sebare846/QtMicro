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
#include "qpaintbox.h"


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
        uint8_t payLoad[64];
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

    void drawBackground();

    void drawCircuit();

private slots:
    void on_pushButtonSend_clicked();

    void on_messageBox_currentIndexChanged();

    void on_UDP_Conectar_clicked();

    void on_pushButton_sendWifi_clicked();

    void on_messageBox_Redes_currentIndexChanged(int index);

    void on_pushButton_Power_clicked();

private:
    Ui::MainWindow *ui;

    #define USBID 0
    #define UDPID 1

    QSerialPort *mySerialUSB, *mySerialUSART;
    QTimer *myTimer, *paintTimer;
    SettingsDialog *mySettingsUSB, *mySettingsUSART;
    QUdpSocket *myUDP;
    QPaintBox *myPaintBox;

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
        ACK         =0x0D,           //Respuesta de confirmacion
        ALIVE       =0xF0,         //Chequeo de conexion
        ESPSETUP    =0XF1,      //Conexion WiFi y UDP
        SETPID      =0xF2,      //Configuracion PID
        SETOFFSET   =0xF3,   //Configuracion Offset error
        SETPOWER    =0xF4,    //Asignar potencia a los motores
        IR_SENSOR   =0xF5,     //Valores sensores infrarrojos
        MPUDATA     =0xF6,     //Valores acelerometro y giroscopio
        PIDDATA     =0xF7,     //Valores de error y potencia
        ALLDATA     =0xF8,     //Valores de todos los sensores y variables de control
        OTHERS
    }_eID;

    _eID estadoComandos;

    typedef enum{
        iALIVE,
        iESPSETUP,
        iSETPID,
        iSETOFFSET,
        iSETPOWER
    }_eIndex;

    _eIndex cmdIndex;

    typedef enum{
        iDEPTO,
        iFCAL,
        iLABORATORIO,
        iDEPTOSOFI,
        iCELUAP,
        iCASARO
    }_eIndexRed;

    _eIndexRed redIndex;

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


    int16_t mpuAccX, mpuAccY, mpuAccZ, mpuGyroX, mpuGyroY, mpuGyroZ;
    float mpuAXg = 0, mpuAYg = 0, mpuAZg = 0, aX = 0, aY = 0, Xtras = 0, Ytras = 0, mpuGX, mpuGY, mpuGZ = 0, tita = 0, velX = 0, velY = 0, velZ = 0, posX = 0, posY = 0, posZ;
    float aXlast = 0, aYlast = 0, vXlast = 0, vYlast = 0, gZlast = 0;

    QHostAddress targetIP, listenIP;
    QPointF posPoint, testPoint;

    uint16_t targetPort, localPort, irBuf[8], irLast[8];
    uint8_t inLine = 0, power = 0;
    uint8_t irOn, kpD = 1, tdD = 0, tiD = 0,kpI = 1, tdI = 0, tiI = 0, onLine;
    int32_t ep, ed,ei, etD, etI, ep_last, ed_last,sumaerror,  velD, velI, sumaPond, peso[8], denominador,fx23, fx12, dx12, dx23;
    uint32_t bk_value, potBase, sensorAux[10], sensorMinimo, potMax;
    int8_t ponderacion[10] = {-70,-60,-45,-30,-15,15,30,45,60,70}, posCen, posDer, posIzq;

};
#endif // MAINWINDOW_H
