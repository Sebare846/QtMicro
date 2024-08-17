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

    void openSerialPort();

    void closeSerialPort();

    void myTimerOnTime();

    void dataRecived();

    void decodeData();

    void sendData();

private:
    Ui::MainWindow *ui;

    QSerialPort *mySerial;
    QTimer *myTimer;
    SettingsDialog *mySettings;

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
        GET_IR = 0xA0,
        SET_POWER = 0xA1,
        SET_SERVO = 0xA2,
        GET_DISTANCE = 0xA3,
        GET_SPEED = 0xA4,
        OTHERS
    }_eID;

    _eID estadoComandos;

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
