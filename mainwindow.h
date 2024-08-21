#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
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

    void openSerialPort(SettingsDialog *mySettings, QSerialPort *mySerial, uint8_t port);

    void closeSerialPort(QSerialPort *mySerial, uint8_t port);

    void myTimerOnTime();

    void dataRecived(QSerialPort *mySerial, uint8_t port);

    void decodeData(uint8_t port);

    void sendData(QSerialPort *mySerial);

private slots:
    void on_pushButtonSend_clicked();

    void on_messageBox_currentIndexChanged(int index);

    void on_pushButtonSend_2_clicked();

private:
    Ui::MainWindow *ui;

    QSerialPort *mySerialUSB, *mySerialUSART;
    QTimer *myTimer;
    SettingsDialog *mySettingsUSB, *mySettingsUSART;

    typedef enum{
        START,
        HEADER_1,
        HEADER_2,
        HEADER_3,
        NBYTES,
        TOKEN,
        PAYLOAD
    }_eProtocolo;

    _eProtocolo estadoProtocolo;

    typedef enum{
        ACK = 0x0D,
        ALIVE=0xF0,
        FIRMWARE = 0xF1,
        OTHERS
    }_eID;

    _eID estadoComandos;

    typedef enum{
        USB = 0,
        USART = 1
    }_ePort;

    _ePort comPorts;

    typedef struct{
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[50];
        uint8_t nBytes;
        uint8_t index;
    }_sDatos ;

    _sDatos rxData, txData;

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



};
#endif // MAINWINDOW_H
