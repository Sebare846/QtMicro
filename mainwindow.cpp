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
    myUDP = new QUdpSocket(this);

    estadoProtocolo=START; //Recibe
    estadoComandos=ALIVE; //Envia

    ///Conexión de eventos USB
    connect(ui->USB_Config, &QAbstractButton::clicked, mySettingsUSB, &SettingsDialog::show); //Esaneo de puerto
    connect(mySerialUSB, &QSerialPort::readyRead, this, [this]() {
        this->dataRecived(mySerialUSB);
    });
    connect(ui->USB_Conectar,&QAbstractButton::clicked, this, [this]() {
        this->openSerialPort(mySettingsUSB, mySerialUSB);
        ui->USB_Conectar->hide();
        ui->USB_Desconectar->show();
    });
    connect(ui->USB_Desconectar, &QAbstractButton::clicked, this, [this]() {
        this->closeSerialPort(mySerialUSB);
        ui->USB_Desconectar->hide();
        ui->USB_Conectar->show();
    });


    ///Otras conexiones
    connect(myTimer, &QTimer::timeout,this, &MainWindow::myTimerOnTime); //intervalo de tiempo
    connect(ui->actionSalir,&QAction::triggered,this,&MainWindow::close ); //Cerrar programa

    ///Definicion mensajes
    ui->messageBox->addItem("ALIVE");
    ui->messageBox->addItem("ENVIAR A ESP");
    ui->messageBox->addItem("COMENZAR TRANSMISION");
    ui->messageBox->addItem("LECTURA SENSORES");
    ui->messageBox->addItem("CONECTAR ESP01");
    ui->messageBox->addItem("CONFIGURAR PID");

    //Definicion redes
    ui->messageBox_Redes->addItem("DEPTO");
    ui->messageBox_Redes->addItem("FCAL");
    ui->messageBox_Redes->addItem("LABORATORIO");
    ui->messageBox_Redes->addItem("DEPTO SOFI");
    ui->messageBox_Redes->addItem("CELU AP");
    ui->messageBox_Redes->addItem("CASA RO");

    ui->USB_Desconectar->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSerialPort(SettingsDialog *mySettings, QSerialPort *mySerial)
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
        ui->actionConectar_USB->setEnabled(false);
        ui->actionDesconectar_USB->setEnabled(true);
        ui->estadoUSB->setText(tr("Conectado a  %1 : %2, %3, %4, %5, %6  %7")
                                   .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                   .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl).arg(p.fabricante));
    }
    else{
        QMessageBox::warning(this,"Menu Conectar","No se pudo abrir el puerto Serie!!!!");
    }
}

//Tareas a realizar cuando se desconecta
void MainWindow::closeSerialPort(QSerialPort *mySerial)
{
    if(mySerial->isOpen()){
        mySerial->close();
        ui->actionDesconectar_USB->setEnabled(false);
        ui->actionConectar_USB->setEnabled(true);
        ui->estadoUSB->setText("Desconectado................");

    }
    else{
        ui->estadoUSB->setText("Desconectado................");
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
void MainWindow::dataRecived(QSerialPort *mySerial)
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
    ui->textUSB->append((char *)incomingBuffer);
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
        ui->textUSB->append("ALIVE");
        break;
    case IR_SENSOR:
        myWord.ui8[0] = rxData.payLoad[2];
        myWord.ui8[1] = rxData.payLoad[3];
        ui->lcdIR0->display(myWord.ui16[0]);
        myWord.ui8[0] = rxData.payLoad[4];
        myWord.ui8[1] = rxData.payLoad[5];
        ui->lcdIR1->display(myWord.ui16[0]);
        myWord.ui8[0] = rxData.payLoad[6];
        myWord.ui8[1] = rxData.payLoad[7];
        ui->lcdIR2->display(myWord.ui16[0]);
        myWord.ui8[0] = rxData.payLoad[8];
        myWord.ui8[1] = rxData.payLoad[9];
        ui->lcdIR3->display(myWord.ui16[0]);
        myWord.ui8[0] = rxData.payLoad[10];
        myWord.ui8[1] = rxData.payLoad[11];
        ui->lcdIR4->display(myWord.ui16[0]);
        myWord.ui8[0] = rxData.payLoad[12];
        myWord.ui8[1] = rxData.payLoad[13];
        ui->lcdIR5->display(myWord.ui16[0]);
        myWord.ui8[0] = rxData.payLoad[14];
        myWord.ui8[1] = rxData.payLoad[15];
        ui->lcdIR6->display(myWord.ui16[0]);
        myWord.ui8[0] = rxData.payLoad[16];
        myWord.ui8[1] = rxData.payLoad[17];
        ui->lcdIR7->display(myWord.ui16[0]);
        break;
    case ESPMSG:
        ui->textUSB->append("TRANSMISION INICIADA");
        break;
    case ESPSETUP:
        ui->textUSB->append("CONFIGURACION ESP");
        break;
    case SETPID:
        ui->textUSB->append("PID SET");
    case DATAPID:
        myWord.ui8[0] = rxData.payLoad[2];
        myWord.ui8[1] = rxData.payLoad[3];
        myWord.ui8[2] = rxData.payLoad[4];
        myWord.ui8[3] = rxData.payLoad[5];
        error = myWord.f32;
        ui->lcdError->display(error);
        // deltaV = error*8000/100;
        // ui->lcdDv->display(deltaV);
        // velD += deltaV;
        // velI -= deltaV;

        // if(abs(velD) > 8000){velD = 8000;}
        // if(abs(velI) > 8000){velI = 8000;}

        // //Motor Derecho
        // if(velD >= 0){
        //     ui->lcdVD->display(velD);
        //     ui->lcdVDm->display(0);
        // }else{
        //     ui->lcdVD->display(0);
        //     ui->lcdVDm->display(velD*(-1));
        // }
        // //Motor Izquierdo
        // if(velD >= 0){
        //     ui->lcdVI->display(velI);
        //     ui->lcdVIm->display(0);
        // }else{
        //     ui->lcdVI->display(0);
        //     ui->lcdVIm->display(velI*(-1));
        // }

        myWord.ui8[0] = rxData.payLoad[6];
        myWord.ui8[1] = rxData.payLoad[7];
        myWord.ui8[2] = rxData.payLoad[8];
        myWord.ui8[3] = rxData.payLoad[9];
        ui->lcdVD->display(myWord.f32);
        myWord.ui8[0] = rxData.payLoad[10];
        myWord.ui8[1] = rxData.payLoad[11];
        myWord.ui8[2] = rxData.payLoad[12];
        myWord.ui8[3] = rxData.payLoad[13];
        ui->lcdVI->display(myWord.f32);
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
    case ESPSETUP:
        txData.payLoad[txData.index++]=ESPSETUP;
        txData.payLoad[txData.index++]=ui->messageBox_Redes->currentIndex();
        txData.payLoad[NBYTES]=0x03;
    break;
    case ESPMSG:
        txData.payLoad[txData.index++]=ESPMSG;
        txData.payLoad[NBYTES]=0x02;
    break;
    case SETPID:
        txData.payLoad[txData.index++]=SETPID;
        myWord.f32 = (float)ui->spinBox_Kp->value();
        txData.payLoad[txData.index++]=myWord.ui8[3];
        txData.payLoad[txData.index++]=myWord.ui8[2];
        txData.payLoad[txData.index++]=myWord.ui8[1];
        txData.payLoad[txData.index++]=myWord.ui8[0];

        myWord.f32 = (float)ui->spinBox_Td->value();
        txData.payLoad[txData.index++]=myWord.ui8[3];
        txData.payLoad[txData.index++]=myWord.ui8[2];
        txData.payLoad[txData.index++]=myWord.ui8[1];
        txData.payLoad[txData.index++]=myWord.ui8[0];

        myWord.f32 = (float)ui->spinBox_Ti->value();
        txData.payLoad[txData.index++]=myWord.ui8[3];
        txData.payLoad[txData.index++]=myWord.ui8[2];
        txData.payLoad[txData.index++]=myWord.ui8[1];
        txData.payLoad[txData.index++]=myWord.ui8[0];

        txData.payLoad[NBYTES]=0x0E;
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

void MainWindow::on_messageBox_currentIndexChanged(int index)
{
    switch(ui->messageBox->currentIndex()){
    case iALIVE:
        estadoComandos = ALIVE;
    break;
    case iESPMSG:
        estadoComandos = ESPMSG;
    break;
    case iIR_SENSOR:
        estadoComandos = IR_SENSOR;
    break;
    case iESPSETUP:
        estadoComandos = ESPSETUP;
    break;
    case iSETPID:
        estadoComandos = SETPID;
    break;
    default:
        ui->textUSB->setText("Mensaje Incorrecto");
        break;
    }
}

void MainWindow::onRXUDP(){
    while (myUDP->hasPendingDatagrams()) {
        QNetworkDatagram UDPdata = myUDP->receiveDatagram();
        ui->textWIFI->append("UDP RECEIVED:");
        ui->textWIFI->append(UDPdata.data().data());
    }
}

void MainWindow::on_UDP_Conectar_clicked()
{
    ///Conexion de eventos WiFi
    if(myUDP->bind(QHostAddress("192.168.100.39"), 30010,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)){
        connect(myUDP, &QUdpSocket::readyRead ,this, &MainWindow::onRXUDP);
        ui->textWIFI->append("UDP CONNECTED");
    }else{
        ui->textWIFI->append("CONECTION ERROR");
    }
}


void MainWindow::on_pushButton_sendWifi_clicked()
{
    char buf[10];
    buf[0] = 'U';
    buf[1] = 'N';
    buf[2] = 'E';
    buf[3] = 'R';
    buf[4] = 0x32;
    buf[5] = ':';
    buf[6] = 0xF0;
    buf[7] = 0xC4;

    myUDP->writeDatagram(buf,sizeof(buf),QHostAddress("192.168.100.39"),30010);
    //ui->textWIFI->append();
    //myUDP->write(QByteArray(buf));

}

