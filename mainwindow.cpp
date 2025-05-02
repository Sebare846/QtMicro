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
    paintTimer = new QTimer(this);
    mySerialUSART = new QSerialPort(this);
    mySerialUSB = new QSerialPort(this);
    mySettingsUSB = new SettingsDialog();
    mySettingsUSART = new SettingsDialog();
    myUDP = new QUdpSocket(this);
    myPaintBox = new QPaintBox(0,0,ui->myPaintBox);

    targetIP = QHostAddress("192.168.100.29"); //depto
    targetPort = 30001;
    localPort = 30010;
    ui->lineEdit_IP->setText(targetIP.toString());
    ui->lineEdit_RP->setText(QString().number(targetPort));
    ui->lineEdit_LP->setText(QString().number(localPort));

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
    ui->messageBox->addItem("CONECTAR ESP01");
    ui->messageBox->addItem("CONFIGURAR PID");
    ui->messageBox->addItem("CONFIGURAR OFFSET");
    ui->messageBox->addItem("CONFIGURAR POTENCIA");

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

    myTimer->start(50);
    paintTimer->start(50);


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
        estadoComandos = ALIVE;
        sendData(USBtxData);
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
        ui->estadoUSB->setText("Desconectado");

    }
    else{
        ui->estadoUSB->setText("Desconectado");
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


    drawCircuit();

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
        //ui->testLabel->setNum(count);
        if(count <= 0)
            return;

        for(uint8_t i = 0; i < count; i++){
            UDPrxData.buffer[i] = UDPdata.data()[i];
            UDPrxData.indexw++;
            UDPrxData.indexw &= 255;
        }

    }

}

//Verificar protocolo
void MainWindow::dataRecived(_sDatos data)
{
    uint8_t bytes = data.indexw - data.indexr;
    char auxBuffer[64];
    for(int i=0;i<bytes; i++){
        switch (estadoProtocolo) {
        case START:
            if (data.buffer[i]=='U'){
                estadoProtocolo=HEADER_1;
                data.cheksum=0;
            }
            if (data.buffer[i]=='+'){
                estadoProtocolo=DBGSTR;
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
        case DBGSTR:
            auxBuffer[i] = data.buffer[i];
            if(auxBuffer[i] == '\n'){
                ui->textUSB->insertPlainText(auxBuffer);
                USBrxData.indexr = USBrxData.indexw;
                estadoProtocolo=START;
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
    case ESPSETUP:
        text = "CONFIGURACION ESP";
        break;
    case SETPID:
        text = "PID SET";
        break;
    case SETOFFSET:
        text = "OFFSET SET";
        break;
    case SETPOWER:
        text = "POWER SET";
        break;
    case IR_SENSOR:
        text = "IR SENSOR DATA";
        myWord.ui8[0] = data.payLoad[2];
        myWord.ui8[1] = data.payLoad[3];
        irBuf[0] = myWord.ui16[0];
        ui->lcdIR0->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[4];
        myWord.ui8[1] = data.payLoad[5];
        irBuf[1] = myWord.ui16[0];
        ui->lcdIR1->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[6];
        myWord.ui8[1] = data.payLoad[7];
        irBuf[2] = myWord.ui16[0];
        ui->lcdIR2->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[8];
        myWord.ui8[1] = data.payLoad[9];
        irBuf[3] = myWord.ui16[0];
        ui->lcdIR3->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[10];
        myWord.ui8[1] = data.payLoad[11];
        irBuf[4] = myWord.ui16[0];
        ui->lcdIR4->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[12];
        myWord.ui8[1] = data.payLoad[13];
        irBuf[5] = myWord.ui16[0];
        ui->lcdIR5->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[14];
        myWord.ui8[1] = data.payLoad[15];
        irBuf[6] = myWord.ui16[0];
        ui->lcdIR6->display(myWord.ui16[0]);
        myWord.ui8[0] = data.payLoad[16];
        myWord.ui8[1] = data.payLoad[17];
        irBuf[7] = myWord.ui16[0];
        ui->lcdIR7->display(myWord.ui16[0]);
        break;
    case MPUDATA:
        text = "MPU DATA";
        myWord.ui8[0] = data.payLoad[2];
        myWord.ui8[1] = data.payLoad[3];
        mpuAccX = myWord.i16[0];
        mpuAXg = mpuAccX / 16384.0;
        if(abs(mpuAXg) < 0.2)
            mpuAXg = 0;
        ui->lcdAx->display(mpuAXg);
        myWord.ui8[0] = data.payLoad[4];
        myWord.ui8[1] = data.payLoad[5];
        mpuAccY = myWord.i16[0];
        mpuAYg = mpuAccY / 16384.0;
        if(abs(mpuAYg) < 0.2)
            mpuAYg = 0;
        ui->lcdAy->display(mpuAYg);
        myWord.ui8[0] = data.payLoad[6];
        myWord.ui8[1] = data.payLoad[7];
        mpuAccZ = myWord.i16[0];
        mpuAZg = mpuAccZ / 16384.0;
        if(abs(mpuAZg) < 0.2)
            mpuAZg = 0;
        ui->lcdAz->display(mpuAZg);

        ui->lcdVx->display(velX);
        ui->lcdVy->display(velY);
        ui->lcdVz->display(velZ);
        break;
    case PIDDATA:
        text = "PID DATA";
        myWord.ui8[0] = data.payLoad[2];
        myWord.ui8[1] = data.payLoad[3];
        myWord.ui8[2] = data.payLoad[4];
        myWord.ui8[3] = data.payLoad[5];
        ep = myWord.i32;
        ui->lcdEP->display(ep);
        myWord.ui8[0] = data.payLoad[6];
        myWord.ui8[1] = data.payLoad[7];
        myWord.ui8[2] = data.payLoad[8];
        myWord.ui8[3] = data.payLoad[9];
        ed = myWord.i32;
        ui->lcdED->display(ed);
        myWord.ui8[0] = data.payLoad[10];
        myWord.ui8[1] = data.payLoad[11];
        myWord.ui8[2] = data.payLoad[12];
        myWord.ui8[3] = data.payLoad[13];
        etD = myWord.i32;
        ui->lcdETD->display(etD);
        myWord.ui8[0] = data.payLoad[14];
        myWord.ui8[1] = data.payLoad[15];
        myWord.ui8[2] = data.payLoad[16];
        myWord.ui8[3] = data.payLoad[17];
        etI = myWord.i32;
        ui->lcdETI->display(etI);
        myWord.ui8[0] = data.payLoad[18];
        myWord.ui8[1] = data.payLoad[19];
        myWord.ui8[2] = data.payLoad[20];
        myWord.ui8[3] = data.payLoad[21];
        velD = myWord.i32;
        ui->lcdPowD->display(velD);
        myWord.ui8[0] = data.payLoad[22];
        myWord.ui8[1] = data.payLoad[23];
        myWord.ui8[2] = data.payLoad[24];
        myWord.ui8[3] = data.payLoad[25];
        velI = myWord.i32;
        ui->lcdPowI->display(velI);
        break;
    case ALLDATA:
        //text = "ALL DATA";
        //Infrarrojos
        myWord.ui8[0] = data.payLoad[2];
        myWord.ui8[1] = data.payLoad[3];
        irBuf[0] = myWord.ui16[0];
        ui->lcdIR0->display(irBuf[0]);
        myWord.ui8[0] = data.payLoad[4];
        myWord.ui8[1] = data.payLoad[5];
        irBuf[1] = myWord.ui16[0];
        ui->lcdIR1->display(irBuf[1]);
        myWord.ui8[0] = data.payLoad[6];
        myWord.ui8[1] = data.payLoad[7];
        irBuf[2] = myWord.ui16[0];
        ui->lcdIR2->display(irBuf[2]);
        myWord.ui8[0] = data.payLoad[8];
        myWord.ui8[1] = data.payLoad[9];
        irBuf[3] = myWord.ui16[0];
        ui->lcdIR3->display(irBuf[3]);
        myWord.ui8[0] = data.payLoad[10];
        myWord.ui8[1] = data.payLoad[11];
        irBuf[4] = myWord.ui16[0];
        ui->lcdIR4->display(irBuf[4]);
        myWord.ui8[0] = data.payLoad[12];
        myWord.ui8[1] = data.payLoad[13];
        irBuf[5] = myWord.ui16[0];
        ui->lcdIR5->display(irBuf[5]);
        myWord.ui8[0] = data.payLoad[14];
        myWord.ui8[1] = data.payLoad[15];
        irBuf[6] = myWord.ui16[0];
        ui->lcdIR6->display(irBuf[6]);
        myWord.ui8[0] = data.payLoad[16];
        myWord.ui8[1] = data.payLoad[17];
        irBuf[7] = myWord.ui16[0];
        ui->lcdIR7->display(irBuf[7]);
        //MPU
        myWord.ui8[0] = data.payLoad[18];
        myWord.ui8[1] = data.payLoad[19];
        mpuAccX = myWord.i16[0];
        mpuAXg = mpuAccX / 16384.0 * 981;
        ui->lcdAx_2->display(mpuAXg);
        myWord.ui8[0] = data.payLoad[20];
        myWord.ui8[1] = data.payLoad[21];
        mpuAccY = myWord.i16[0];
        mpuAYg = mpuAccY / 16384.0 * 981;
        ui->lcdAy_2->display(mpuAYg);
        myWord.ui8[0] = data.payLoad[22];
        myWord.ui8[1] = data.payLoad[23];
        mpuAccZ = myWord.i16[0];
        mpuAZg = mpuAccZ / 16384.0 * 981;
        ui->lcdAz_2->display(mpuAZg);
        myWord.ui8[0] = data.payLoad[24];
        myWord.ui8[1] = data.payLoad[25];
        mpuGX = myWord.i16[0] / 131.0 * (M_PI / 180.0);
        if(abs(mpuGX) < 0.3)
            mpuGX = 0;
        ui->lcdGx->display(mpuGX);
        myWord.ui8[0] = data.payLoad[26];
        myWord.ui8[1] = data.payLoad[27];
        mpuGY = myWord.i16[0] / 131.0 * (M_PI / 180.0);
        if(abs(mpuGY) < 0.3)
            mpuGY = 0;
        ui->lcdGy->display(mpuGY);
        myWord.ui8[0] = data.payLoad[28];
        myWord.ui8[1] = data.payLoad[29];
        mpuGZ = (myWord.i16[0] / 131.0) * (M_PI / 180); //Ya en radianes/s2
        if(abs(mpuGZ) < 0.3)
            mpuGZ = 0;
        ui->lcdGz->display(mpuGZ);
        //PID
        myWord.ui8[0] = data.payLoad[30];
        myWord.ui8[1] = data.payLoad[31];
        myWord.ui8[2] = data.payLoad[32];
        myWord.ui8[3] = data.payLoad[33];
        ep = myWord.i32;
        ui->lcdEP->display(ep);
        myWord.ui8[0] = data.payLoad[34];
        myWord.ui8[1] = data.payLoad[35];
        myWord.ui8[2] = data.payLoad[36];
        myWord.ui8[3] = data.payLoad[37];
        ed = myWord.i32;
        ui->lcdED->display(ed);
        myWord.ui8[0] = data.payLoad[38];
        myWord.ui8[1] = data.payLoad[39];
        myWord.ui8[2] = data.payLoad[40];
        myWord.ui8[3] = data.payLoad[41];
        etD = myWord.i32;
        ui->lcdETD->display(etD);
        myWord.ui8[0] = data.payLoad[42];
        myWord.ui8[1] = data.payLoad[43];
        myWord.ui8[2] = data.payLoad[44];
        myWord.ui8[3] = data.payLoad[45];
        etI = myWord.i32;
        ui->lcdETI->display(etI);
        myWord.ui8[0] = data.payLoad[46];
        myWord.ui8[1] = data.payLoad[47];
        myWord.ui8[2] = data.payLoad[48];
        myWord.ui8[3] = data.payLoad[49];
        velD = myWord.i32;
        ui->lcdPowD->display(velD);
        myWord.ui8[0] = data.payLoad[50];
        myWord.ui8[1] = data.payLoad[51];
        myWord.ui8[2] = data.payLoad[52];
        myWord.ui8[3] = data.payLoad[53];
        velI = myWord.i32;
        ui->lcdPowI->display(velI);
        break;
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
        //ui->textWIFI->append("Received: ");
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
        text = "CONFIGURACION ESP";
        data.buffer[auxIndex++]=ESPSETUP;
        data.buffer[auxIndex++]=ui->messageBox_Redes->currentIndex();
        data.buffer[NBYTES]=0x03;
    break;
    case SETPID:
        text = "SET PID";
        data.buffer[auxIndex++]=SETPID;
        kpD = (uint8_t)ui->spinBox_KpD->value();
        data.buffer[auxIndex++]= kpD;

        tdD = (uint8_t)ui->spinBox_TdD->value();
        data.buffer[auxIndex++]= tdD;

        tiD = (uint8_t)ui->spinBox_TiD->value();
        data.buffer[auxIndex++]= tiD;

        kpI = (uint8_t)ui->spinBox_KpI->value();
        data.buffer[auxIndex++]= kpI;

        tdI = (uint8_t)ui->spinBox_TdI->value();
        data.buffer[auxIndex++]= tdI;

        tiI = (uint8_t)ui->spinBox_TiI->value();
        data.buffer[auxIndex++]= tiI;

        bk_value = (uint32_t)ui->spinBox_Bk->value();
        myWord.ui32 = bk_value;
        data.buffer[auxIndex++]= myWord.ui8[0];
        data.buffer[auxIndex++]= myWord.ui8[1];
        data.buffer[auxIndex++]= myWord.ui8[2];
        data.buffer[auxIndex++]= myWord.ui8[3];

        potBase = (uint32_t)ui->spinBox_Pot->value();
        myWord.ui32 = potBase;
        data.buffer[auxIndex++]= myWord.ui8[0];
        data.buffer[auxIndex++]= myWord.ui8[1];
        data.buffer[auxIndex++]= myWord.ui8[2];
        data.buffer[auxIndex++]= myWord.ui8[3];

        potMax = (uint32_t)ui->spinBox_PotMax->value();
        myWord.ui32 = potMax;
        data.buffer[auxIndex++]= myWord.ui8[0];
        data.buffer[auxIndex++]= myWord.ui8[1];
        data.buffer[auxIndex++]= myWord.ui8[2];
        data.buffer[auxIndex++]= myWord.ui8[3];

        data.buffer[NBYTES]=0x14;
        break;
    case SETOFFSET:
        text = "SET OFFSET";
        data.buffer[auxIndex++]=SETOFFSET;
        data.buffer[NBYTES]=0x02;
        break;
    case SETPOWER:
        text = "SET POWER";
        data.buffer[auxIndex++]=SETPOWER;
        //data.buffer[auxIndex++]=ui->messageBox_Redes->currentIndex();
        potMax = (uint32_t)ui->spinBox_PotMax->value();
        myWord.ui32 = potMax;
        data.buffer[auxIndex++]= myWord.ui8[0];
        data.buffer[auxIndex++]= myWord.ui8[1];
        data.buffer[auxIndex++]= myWord.ui8[2];
        data.buffer[auxIndex++]= myWord.ui8[3];
        data.buffer[NBYTES]=0x06
        ;
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

void MainWindow::on_messageBox_currentIndexChanged()
{
    switch(ui->messageBox->currentIndex()){
    case iALIVE:
        estadoComandos = ALIVE;
        break;
    case iESPSETUP:
        estadoComandos = ESPSETUP;
        break;
    case iSETPID:
        estadoComandos = SETPID;
        break;
    case iSETOFFSET:
        estadoComandos = SETOFFSET;
        break;
    case iSETPOWER:
        estadoComandos = SETPOWER;
        break;
    default:
        break;
    }
}



void MainWindow::on_UDP_Conectar_clicked()
{
    ///Conexion de eventos WiFi
    if(myUDP->bind(listenIP, localPort,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)){ //,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
        //"192.168.2.112" lab "192.168.100.39" depto
        connect(myUDP, &QUdpSocket::readyRead ,this, &MainWindow::onRXUDP);
        ui->textWIFI->append("UDP CONNECTED");
        estadoComandos = ALIVE;
        sendData(USBtxData);
    }else{
        ui->textWIFI->append("CONECTION ERROR");
    }
}


void MainWindow::on_pushButton_sendWifi_clicked()
{
    sendData(UDPtxData);
}


void MainWindow::on_messageBox_Redes_currentIndexChanged(int index)
{
    switch(index){
    case iDEPTO:
        targetIP = QHostAddress("192.168.100.29");
        listenIP = QHostAddress("192.168.100.39");
        ui->lineEdit_IP->setText(targetIP.toString());
        break;
    case iFCAL:
        targetIP = QHostAddress("172.23.229.108");
        listenIP = QHostAddress("172.23.229.107");
        ui->lineEdit_IP->setText(targetIP.toString());
        break;
    case iLABORATORIO:
        targetIP = QHostAddress("192.168.2.103");
        listenIP = QHostAddress("192.168.2.102");
        ui->lineEdit_IP->setText(targetIP.toString());
        break;
    case iCELUAP:
        targetIP = QHostAddress("172.20.10.7");
        listenIP = QHostAddress("172.20.10.7");
        ui->lineEdit_IP->setText(targetIP.toString());
        break;
    default:
        ui->lineEdit_IP->setText("No IP");
        break;
    }
}


void MainWindow::on_pushButton_Power_clicked()
{
    estadoComandos = SETPOWER;
    sendData(USBtxData);
    power = !power;
    if(power){
        ui->pushButton_Power->setText("POWER ON");
        drawBackground();
    }
    else{
        ui->pushButton_Power->setText("POWER OFF");
        tita = 0;
        aX = 0;
        aY = 0;
        velX = 0;
        velY = 0;
        posX = 0;
        posY = 0;
    }

}

void MainWindow::drawBackground()
{
    //QPainter paint(myPaintBox->getCanvas());
    //QPen pen;
    myPaintBox->getCanvas()->fill(Qt::white);
    myPaintBox->update();
}

void MainWindow::drawCircuit()
{
    if(abs(mpuAXg) < 20)
        mpuAXg = 0;
    if(abs(mpuAYg) < 20)
        mpuAYg = 0;
    if(abs(mpuAZg) < 20)
        mpuAZg = 0;

    //Integracion de giro
    tita += mpuGZ * 0.05; //100ms
    if(abs(tita) > 6.3){
        tita = 0;
    }
    gZlast = mpuGZ;
    ui->lcdTita->display(tita);

    //Podria incluirse un filtro pasabajos

    //Aceleraciones del sistema
    aX = mpuAXg;// * cos(tita) - mpuAYg * sin(tita);
    aY = mpuAYg;// * sin(tita) + mpuAYg * cos(tita);

    //Limitaciones
    if(abs(aX) < 1)
        aX = 0;
    ui->lcdAx->display(aX);
    if(abs(aY) < 1)
        aY = 0;
    ui->lcdAy->display(aY);

    //Integracion de aceleraciones
    if(power){
        velX += aX * 0.05; // + aXlast)/2.0
        aXlast = aX;
        velY += aY * 0.05; // + aYlast)/2.0
        aYlast = aY;
    }else{
        aX = 0;
        aY = 0;
        velX = 0;
        velY = 0;
    }

    ui->lcdVx->display(velX);
    ui->lcdVy->display(velY);

    //Integraciones de velocidades
    posX += velX * 0.05; // + vXlast)/2.0
    vXlast = velX;
    posY += velY  * 0.05;//+ vYlast)/2.0
    vYlast = velY;

    //Traslacion de posiciones
    Xtras = (posX/10);///10 - (45 * cos(tita) - 25 * sin(tita)));
    Ytras = (posY/10);///10 - (45 * sin(tita) + 25 * cos(tita)));

    ui->lcdPosX->display(Xtras);
    ui->lcdPosY->display(Ytras),

    posPoint.setX(Xtras);
    posPoint.setY(Ytras);

    QPointF drawPoint;
    QPainter paint(myPaintBox->getCanvas());
    QPen pen;
    QBrush brush;
    drawPoint.setX(myPaintBox->width()/2 + posPoint.x());
    drawPoint.setY(myPaintBox->height()/2 + posPoint.y());
    brush.setColor(Qt::red);
    pen.setColor(Qt::red);
    pen.setWidth(6);
    paint.setPen(pen);
    paint.setBrush(brush);
    paint.drawEllipse(drawPoint,3,3);
    myPaintBox->update();
}
