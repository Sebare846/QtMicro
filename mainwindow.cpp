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

    targetIP = QHostAddress("192.168.100.29");
    targetPort = 30001;

    estadoProtocolo=START; //Recibe
    estadoComandos=ALIVE; //Envia

    ///Conexión de eventos USB
    connect(ui->USB_Config, &QAbstractButton::clicked, mySettingsUSB, &SettingsDialog::show); //Esaneo de puerto
    connect(mySerialUSB, &QSerialPort::readyRead, this, [this]() {
        this->onRXUSB(mySerialUSB);
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

    //Inicializacion Variables
    USBrxData.comID = USBID;
    USBrxData.indexr = 0;
    USBrxData.indexw = 0;
    USBrxData.nBytes = 0;
    USBtxData.comID = USBID;
    USBtxData.indexr = 0;
    USBtxData.indexw = 0;
    UDPrxData.comID = UDPID;
    UDPrxData.indexr = 0;
    UDPrxData.indexw = 0;
    UDPrxData.nBytes = 0;
    UDPtxData.comID = UDPID;
    UDPtxData.indexr = 0;
    UDPtxData.indexr = 0;

    ui->USB_Desconectar->hide();

    myTimer->start(100);
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

    if(USBrxData.indexr != USBrxData.indexw){
        dataRecived(USBrxData);
    }

    if(UDPrxData.indexr != UDPrxData.indexw){
        dataRecived(UDPrxData);
    }

    //->testLabel->setNum(myUDP->state());
}

//Recepcion de datos USB
void MainWindow::onRXUSB(QSerialPort *mySerial)
{
    unsigned char *incomingBuffer;
    int count;
    //numero de bytes
    count = mySerial->bytesAvailable();

    if(count<=0)
        return;

    incomingBuffer = new unsigned char[count];

    mySerial->read((char *)incomingBuffer,count);

    //ui->textUSB->append((char *)incomingBuffer);

    for(uint8_t i = 0; i < count; i++){
        USBrxData.buffer[i] = uint8_t(incomingBuffer[i]);
        USBrxData.indexw++;
        USBrxData.indexw &= 255;
    }

}

//Recepcion de datos UDP
void MainWindow::onRXUDP(){
    uint8_t count = 0;
    while (myUDP->hasPendingDatagrams()) {
        QNetworkDatagram UDPdata = myUDP->receiveDatagram();
        count = UDPdata.data().size();
        ui->testLabel->setNum(count);

        if(count <= 0)
            return;

        for(uint8_t i = 0; i < count; i++){
            UDPrxData.buffer[i] = UDPdata.data()[i];
            UDPrxData.indexw++;
            UDPrxData.indexw &= 255;
        }

        //QString auxStr;
        //auxStr.append(UDPdata.data().toHex());
        //ui->textWIFI->append(auxStr);
    }

}

//Verificar protocolo
void MainWindow::dataRecived(_sDatos data)
{
    uint8_t bytes = data.indexw - data.indexr;
    for(int i=0;i<bytes; i++){
        switch (estadoProtocolo) {
        case START:
            if (data.buffer[i]=='U'){
                estadoProtocolo=HEADER_1;
                data.cheksum=0;
            }
            break;
        case HEADER_1:
            if (data.buffer[i]=='N')
                estadoProtocolo=HEADER_2;
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case HEADER_2:
            if (data.buffer[i]=='E')
                estadoProtocolo=HEADER_3;
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case HEADER_3:
            if (data.buffer[i]=='R')
                estadoProtocolo=NBYTES;
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case NBYTES:
            data.nBytes = data.buffer[i];
            estadoProtocolo=TOKEN;
            break;
        case TOKEN:
            if (data.buffer[i]==':'){
                estadoProtocolo=PAYLOAD;
                data.cheksum='U'^'N'^'E'^'R'^ data.nBytes ^':';
                data.payLoad[0]=data.nBytes;
                data.indexP=1;
            }
            else{
                i--;
                estadoProtocolo=START;
            }
            break;
        case PAYLOAD:
            if (data.nBytes>1){
                data.payLoad[data.indexP++]=data.buffer[i];
                data.cheksum^=data.buffer[i];
            }
            data.nBytes--;
            if(data.nBytes==0){
                estadoProtocolo=START;
                if(data.cheksum==data.buffer[i]){
                    decodeData(data);
                }
            }
            break;
        default:
            estadoProtocolo=START;
            break;
        }
    }

}

void MainWindow::decodeData(_sDatos data)
{

    QString text;
    switch (data.payLoad[1]) {
    case ALIVE:
        text = "ALIVE";
        break;
    case IR_SENSOR:
        myWord.ui8[0] = data.payLoad[2];
        myWord.ui8[1] = data.payLoad[3];
        ui->lcdIR0->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[4];
        myWord.ui8[1] = data.payLoad[5];
        ui->lcdIR1->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[6];
        myWord.ui8[1] = data.payLoad[7];
        ui->lcdIR2->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[8];
        myWord.ui8[1] = data.payLoad[9];
        ui->lcdIR3->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[10];
        myWord.ui8[1] = data.payLoad[11];
        ui->lcdIR4->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[12];
        myWord.ui8[1] = data.payLoad[13];
        ui->lcdIR5->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[14];
        myWord.ui8[1] = data.payLoad[15];
        ui->lcdIR6->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[16];
        myWord.ui8[1] = data.payLoad[17];
        ui->lcdIR7->display(myWord.ui16[0]);
        break;
    case ESPMSG:
        text = "TRANSMISION INICIADA";
        break;
    case ESPSETUP:
        text = "CONFIGURACION ESP";
        break;
    case SETPID:
        text = "PID SET";
    case DATAPID:
        myWord.ui8[0] = data.payLoad[2];
        myWord.ui8[1] = data.payLoad[3];
        myWord.ui8[2] = data.payLoad[4];
        myWord.ui8[3] = data.payLoad[5];
        error = myWord.f32;
        ui->lcdError->display(error);
        myWord.ui8[0] = data.payLoad[6];
        myWord.ui8[1] = data.payLoad[7];
        myWord.ui8[2] = data.payLoad[8];
        myWord.ui8[3] = data.payLoad[9];
        ui->lcdVD->display(myWord.f32);
        myWord.ui8[0] = data.payLoad[10];
        myWord.ui8[1] = data.payLoad[11];
        myWord.ui8[2] = data.payLoad[12];
        myWord.ui8[3] = data.payLoad[13];
        ui->lcdVI->display(myWord.f32);
    default:
        break;
    }

    switch(data.comID){
    case USBID:
        ui->textUSB->append("Received: ");
        ui->textUSB->insertPlainText(text);
        USBrxData.indexr = USBrxData.indexw;
        break;
    case UDPID:
        ui->textWIFI->append("Received: ");
        ui->textWIFI->append(text);
        UDPrxData.indexr = UDPrxData.indexw;
        break;
    default:
        break;
    }
}

//Enviar datos, elaborar protocolo
void MainWindow::sendData(_sDatos data)
{
    QString text;
    uint8_t auxIndex = 0;
    //carga el header y token
    data.buffer[auxIndex++]='U';
    data.buffer[auxIndex++]='N';
    data.buffer[auxIndex++]='E';
    data.buffer[auxIndex++]='R';
    data.buffer[auxIndex++]=0;
    data.buffer[auxIndex++]=':';
    //carga el ID y nBytes
    switch (estadoComandos) {
    case ALIVE:
        text = "ALIVE";
        data.buffer[auxIndex++]=ALIVE;
        data.buffer[NBYTES]=0x02;
        break;
    case ESPSETUP:
        text = "SET ESP";
        data.buffer[auxIndex++]=ESPSETUP;
        data.buffer[auxIndex++]=ui->messageBox_Redes->currentIndex();
        data.buffer[NBYTES]=0x03;
    break;
    case ESPMSG:
        text = "ESP MESSAGE";
        data.buffer[auxIndex++]=ESPMSG;
        data.buffer[NBYTES]=0x02;
    break;
    case SETPID:
        text = "SET PID";
        data.buffer[auxIndex++]=SETPID;
        myWord.f32 = (float)ui->spinBox_Kp->value();
        data.buffer[auxIndex++]=myWord.ui8[3];
        data.buffer[auxIndex++]=myWord.ui8[2];
        data.buffer[auxIndex++]=myWord.ui8[1];
        data.buffer[auxIndex++]=myWord.ui8[0];

        myWord.f32 = (float)ui->spinBox_Td->value();
        data.buffer[auxIndex++]=myWord.ui8[3];
        data.buffer[auxIndex++]=myWord.ui8[2];
        data.buffer[auxIndex++]=myWord.ui8[1];
        data.buffer[auxIndex++]=myWord.ui8[0];

        myWord.f32 = (float)ui->spinBox_Ti->value();
        data.buffer[auxIndex++]=myWord.ui8[3];
        data.buffer[auxIndex++]=myWord.ui8[2];
        data.buffer[auxIndex++]=myWord.ui8[1];
        data.buffer[auxIndex++]=myWord.ui8[0];

        data.buffer[NBYTES]=0x0E;
        break;
    default:
        break;
    }

    data.cheksum=0;

    //recuenta los bytes y carga el checksum
    for(int a=0 ;a<auxIndex;a++)
        data.cheksum^=data.buffer[a];
    data.buffer[auxIndex]=data.cheksum;

    data.nBytes = data.buffer[NBYTES] + 6;

    switch(data.comID){
    case USBID:
        if(mySerialUSB->isWritable()){
            ui->textUSB->append("Sent: ");
            ui->textUSB->insertPlainText(text);
            mySerialUSB->write((char *)data.buffer,data.nBytes);
            USBtxData.indexw += auxIndex;
            USBtxData.indexw &= 255;
        }
        break;
    case UDPID:
        char bytesSent, bytesToSend;
        bytesToSend = '0' + data.nBytes;
        bytesSent = '0' + myUDP->writeDatagram((char *)data.buffer,data.nBytes,targetIP,targetPort);
        if(bytesSent != 0){
            UDPtxData.indexw += auxIndex;
            UDPtxData.indexw &= 255;
            ui->textWIFI->append(QString("Sent %1 of %2 bytes: %3")
                                     .arg(bytesSent)
                                     .arg(bytesToSend)
                                     .arg(text));
        }else{
            ui->textWIFI->append("Error Sending");
        }
        break;
    default:
        break;
    }

}


void MainWindow::on_pushButtonSend_clicked()
{
    sendData(USBtxData);
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
        break;
    }
}



void MainWindow::on_UDP_Conectar_clicked()
{
    ///Conexion de eventos WiFi
    if(myUDP->bind(QHostAddress("192.168.100.39"), 30010,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)){ //,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
        connect(myUDP, &QUdpSocket::readyRead ,this, &MainWindow::onRXUDP);
        ui->textWIFI->append("UDP CONNECTED");
    }else{
        ui->textWIFI->append("CONECTION ERROR");
    }
}


void MainWindow::on_pushButton_sendWifi_clicked()
{

    // uint8_t bytesSent;
    // char buf[9];
    // buf[0] = 'U';
    // buf[1] = 'N';
    // buf[2] = 'E';
    // buf[3] = 'R';
    // buf[4] = 0x02;
    // buf[5] = ':';
    // buf[6] = 0xF0;
    // buf[7] = 0xC4;

    // bytesSent = myUDP->writeDatagram(buf,sizeof(buf),targetIP,targetPort);
    // ui->testLabel->setNum(bytesSent);
    sendData(UDPtxData);

}

