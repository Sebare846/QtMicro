#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QPainterPath>
#include <math.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    myTimer = new QTimer(this);
    mySerial = new QSerialPort(this);
    mySettings = new SettingsDialog();

    estadoProtocolo=START; //Recibe
    estadoComandos=ALIVE; //Envia

    ///Conexión de eventos
    connect(mySerial,&QSerialPort::readyRead,this, &MainWindow::dataRecived ); //Si llega recibir
    connect(myTimer, &QTimer::timeout,this, &MainWindow::myTimerOnTime); //intervalo de tiempo
    connect(ui->actionEscaneo_de_puertos, &QAction::triggered, mySettings, &SettingsDialog::show); //Esaneo de puerto
    connect(ui->actionConectar,&QAction::triggered,this, &MainWindow::openSerialPort); //Abrir puerto
    connect(ui->actionDesconectar, &QAction::triggered, this, &MainWindow::closeSerialPort); //Cerrar puerto
    connect(ui->actionSalir,&QAction::triggered,this,&MainWindow::close ); //Cerrar programa
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = mySettings->settings();
    //Configuracion de comunicacion
    mySerial->setPortName(p.name);
    mySerial->setBaudRate(p.baudRate);
    mySerial->setDataBits(p.dataBits);
    mySerial->setParity(p.parity);
    mySerial->setStopBits(p.stopBits);
    mySerial->setFlowControl(p.flowControl);
    mySerial->open(QSerialPort::ReadWrite);
    if(mySerial->isOpen()){
        ui->actionConectar->setEnabled(false);
        ui->actionDesconectar->setEnabled(true);
        ui->estado->setText(tr("Conectado a  %1 : %2, %3, %4, %5, %6  %7")
                            .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                            .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl).arg(p.fabricante));
    }
    else{
        QMessageBox::warning(this,"Menu Conectar","No se pudo abrir el puerto Serie!!!!");
    }
}

//Tareas a realizar cuando se desconecta
void MainWindow::closeSerialPort()
{
    if(mySerial->isOpen()){
        mySerial->close();
        ui->actionDesconectar->setEnabled(false);
        ui->actionConectar->setEnabled(true);
        ui->estado->setText("Desconectado................");
    }
    else{
        ui->estado->setText("Desconectado................");
    }

}

void MainWindow::myTimerOnTime()
{
    //Si timeout verificar si hay datos para recibir
    if(rxData.timeOut!=0){
        rxData.timeOut--;
    }else{
        estadoProtocolo=START;
    }
}

//Verificar protocolo
void MainWindow::dataRecived()
{

    unsigned char *incomingBuffer;
    int count;
    //numero de bytes
    count = mySerial->bytesAvailable();

    if(count<=0)
        return;

    incomingBuffer = new unsigned char[count];

    mySerial->read((char *)incomingBuffer,count);

    //ui->label->setText((char *)incomingBuffer);
    rxData.timeOut=5;
    for(int i=0;i<count; i++){
        switch (estadoProtocolo) {
        case START:
            if (incomingBuffer[i]=='U'){
                estadoProtocolo=HEADER_1;
                rxData.cheksum=0;
            }
            break;
        case HEADER_1:
            if (incomingBuffer[i]=='N')
                estadoProtocolo=HEADER_2;
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case HEADER_2:
            if (incomingBuffer[i]=='E')
                estadoProtocolo=HEADER_3;
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case HEADER_3:
            if (incomingBuffer[i]=='R')
                estadoProtocolo=NBYTES;
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case NBYTES:
            rxData.nBytes=incomingBuffer[i];
            estadoProtocolo=TOKEN;
            break;
        case TOKEN:
            if (incomingBuffer[i]==':'){
                estadoProtocolo=PAYLOAD;
                rxData.cheksum='U'^'N'^'E'^'R'^ rxData.nBytes^':';
                rxData.payLoad[0]=rxData.nBytes;
                rxData.index=1;
            }
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case PAYLOAD:
            if (rxData.nBytes>1){
                rxData.payLoad[rxData.index++]=incomingBuffer[i];
                rxData.cheksum^=incomingBuffer[i];
            }
            rxData.nBytes--;
            if(rxData.nBytes==0){
                estadoProtocolo=START;
                if(rxData.cheksum==incomingBuffer[i]){
                    decodeData();
                }
            }
            break;
        default:
            estadoProtocolo=START;
            break;
        }
    }
    delete [] incomingBuffer;
}

void MainWindow::decodeData()
{

    switch (rxData.payLoad[1]) {
    case ALIVE:
        //ui->aliveEdit->setAlignment(Qt::AlignCenter);
        //ui->aliveEdit->setText("I´M ALIVE");

        break;
    default:
        break;
    }
}

//Enviar datos, elaborar protocolo
void MainWindow::sendData()
{
    //carga el header y token
    txData.index=0;
    txData.payLoad[txData.index++]='U';
    txData.payLoad[txData.index++]='N';
    txData.payLoad[txData.index++]='E';
    txData.payLoad[txData.index++]='R';
    txData.payLoad[txData.index++]=0;
    txData.payLoad[txData.index++]=':';
    //carga el ID y nBytes
    switch (estadoComandos) {
    case ALIVE:
        txData.payLoad[txData.index++]=ALIVE;
        txData.payLoad[NBYTES]=0x02;

        break;
    default:
        break;
    }

    txData.cheksum=0;

    //recuenta los bytes y carga el checksum
    for(int a=0 ;a<txData.index;a++)
        txData.cheksum^=txData.payLoad[a];
    txData.payLoad[txData.index]=txData.cheksum;
    if(mySerial->isWritable()){
        mySerial->write((char *)txData.payLoad,txData.payLoad[NBYTES]+6);

    }


}



