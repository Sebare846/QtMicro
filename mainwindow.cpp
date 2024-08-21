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
    mySerialUSART = new QSerialPort(this);
    mySerialUSB = new QSerialPort(this);
    mySettingsUSB = new SettingsDialog();
    mySettingsUSART = new SettingsDialog();

    estadoProtocolo=START; //Recibe
    estadoComandos=ALIVE; //Envia

    ///Conexión de eventos USART
    connect(ui->actionConfiguracion_3, &QAction::triggered, mySettingsUSART, &SettingsDialog::show); //Esaneo de puerto
    connect(mySerialUSART, &QSerialPort::readyRead, this, [this]() {
        this->dataRecived(mySerialUSART, USART);
    });
    connect(ui->actionConectar_USART,&QAction::triggered, this, [this]() {
        this->openSerialPort(mySettingsUSART, mySerialUSART, USART);
    });
    connect(ui->actionDesconectar_USART, &QAction::triggered, this, [this]() {
        this->closeSerialPort(mySerialUSART, USART);
    });
    ///Conexión de eventos USB
    connect(ui->actionConfiguracion_2, &QAction::triggered, mySettingsUSB, &SettingsDialog::show); //Esaneo de puerto
    connect(mySerialUSB, &QSerialPort::readyRead, this, [this]() {
        this->dataRecived(mySerialUSB, USB);
    });
    connect(ui->actionConectar_USB,&QAction::triggered, this, [this]() {
        this->openSerialPort(mySettingsUSB, mySerialUSB, USB);
    });
    connect(ui->actionDesconectar_USB, &QAction::triggered, this, [this]() {
        this->closeSerialPort(mySerialUSB, USB);
    });
    ///Conexion de eventos WiFi


    ///Otras conexiones
    connect(myTimer, &QTimer::timeout,this, &MainWindow::myTimerOnTime); //intervalo de tiempo
    connect(ui->actionSalir,&QAction::triggered,this,&MainWindow::close ); //Cerrar programa


    ///Definicion mensajes
    ui->messageBox->addItem("ALIVE");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSerialPort(SettingsDialog *mySettings, QSerialPort *mySerial, uint8_t port)
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
        if(port == USB){
            ui->actionConectar_USB->setEnabled(false);
            ui->actionDesconectar_USB->setEnabled(true);
            ui->estadoUSB->setText(tr("Conectado a  %1 : %2, %3, %4, %5, %6  %7")
                                       .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                       .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl).arg(p.fabricante));
        }
        if(port == USART){
            ui->actionConectar_USART->setEnabled(false);
            ui->actionDesconectar_USART->setEnabled(true);
            ui->estadoUSART->setText(tr("Conectado a  %1 : %2, %3, %4, %5, %6  %7")
                                       .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                       .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl).arg(p.fabricante));
        }
    }
    else{
        QMessageBox::warning(this,"Menu Conectar","No se pudo abrir el puerto Serie!!!!");
    }
}

//Tareas a realizar cuando se desconecta
void MainWindow::closeSerialPort(QSerialPort *mySerial, uint8_t port)
{
    if(mySerial->isOpen()){
        mySerial->close();
        if(port == USB){
            ui->actionDesconectar_USB->setEnabled(false);
            ui->actionConectar_USB->setEnabled(true);
            ui->estadoUSB->setText("Desconectado................");
        }
        if(port == USART){
            ui->actionDesconectar_USART->setEnabled(false);
            ui->actionConectar_USART->setEnabled(true);
            ui->estadoUSART->setText("Desconectado................");
        }
    }
    else{
        if(port == USB){
            ui->estadoUSB->setText("Desconectado................");
        }
        if(port == USART){
            ui->estadoUSART->setText("Desconectado................");
        }
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
void MainWindow::dataRecived(QSerialPort *mySerial, uint8_t port)
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
                    decodeData(port);
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

void MainWindow::decodeData(uint8_t port)
{

    switch (rxData.payLoad[1]) {
    case ALIVE:
        if(port == USB){
            ui->textUSB->append("ALIVE");
        }else{
            ui->textUSART->append("ALIVE");
        }
        break;
    default:
        break;
    }
}

//Enviar datos, elaborar protocolo
void MainWindow::sendData(QSerialPort *mySerial)
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




void MainWindow::on_pushButtonSend_clicked()
{
    sendData(mySerialUSB);
}

void MainWindow::on_pushButtonSend_2_clicked()
{
    sendData(mySerialUSART);
}

void MainWindow::on_messageBox_currentIndexChanged(int index)
{
    switch(ui->messageBox->currentIndex()){
    case 0:
        estadoComandos = ALIVE;
    break;
    default:
        ui->textUSB->setText("Mensaje Incorrecto");
        break;
    }
}




