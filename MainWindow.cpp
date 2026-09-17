#include <QGridLayout>
#include <QSerialPortInfo>
#include <QCloseEvent>
#include <QSplitter>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QDir>
#include <QListIterator>
#include <QGroupBox>
#include <QWidgetAction>
#include <QtMath>

#include "MainWindow.h"
#include "Macro.h"
#include "HexEncoder.h"
#include "AsciiEncoder.h"
#include "DecEncoder.h"

#include <QDebug>

const int BLINK_TIME_TX = 200;
const int BLINK_TIME_RX = 200;

#define CR 0x0D
#define LF 0x0A

const QString PORT = QObject::tr("Port: ");
const QString BAUD = QObject::tr("Baud rate: ");
const QString BITS = QObject::tr("Data bits: ");
const QString PARITY = QObject::tr("Parity: ");
const QString STOP_BITS = QObject::tr("Stop bits: ");
const int DEFAULT_LOG_ROWS = 1000;
const int DEFAULT_MODE = 0; // ASCII
const int DEFAULT_LOG_TIMEOUT = 600000; // ms
const int DEFAULT_READ_DELAY = 10; // ms
const QString DEFAULT_SEPARATOR = " ";
const int DEFAULT_SEND_TIME = 0; // ms
const bool DEFAULT_CR_LF = false;
const Qt::DockWidgetArea DEFAULT_MACROS_AREA = Qt::RightDockWidgetArea;
const bool DEFAULT_MACROS_HIDDEN = true;
const QString DEFAULT_SEND_MODE = QObject::tr("Single send");
const QString DEFAULT_WRITE_DISPLAYING = QObject::tr("Displaying write data");
const QString DEFAULT_READ_DISPLAYING = QObject::tr("Displaying read data");

MainWindow::MainWindow(QString title, QWidget *parent)
    : QMainWindow(parent)
    , actionPortConfigure(new QAction(QIcon(":/Resources/ComPort.png"), tr("COM-port configure"), this))
    , actionStartStop(new QAction(QIcon(":/Resources/Play.png"), tr("Open COM-port"), this))
    , actionMacros(new QAction(QIcon(":/Resources/Macros.png"), tr("Macros"), this))
    , portName(new QLabel(PORT + tr("None"), this))
    , baud(new QLabel(BAUD + tr("None"), this))
    , bits(new QLabel(BITS + tr("None"), this))
    , parity(new QLabel(PARITY + tr("None"), this))
    , stopBits(new QLabel(STOP_BITS + tr("None"), this))
    , manualSendEncodingMode(new QComboBox(this))
    , readEncodingMode(new QComboBox(this))
    , writeEncodingMode(new QComboBox(this))
    , manualSendTimer(new QTimer(this))
    , writeLogTimer(new QTimer(this))
    , readLogTimer(new QTimer(this))
    , delayBetweenPacketsTimer(new QTimer(this))
    , txTimer(new QTimer(this))
    , rxTimer(new QTimer(this))
    , macros(new Macros)
    , macrosDockWidget(new QDockWidget(tr("Macros"), this))
    , clearWriteLog(new QAction(QIcon(":/Resources/Clear.png"), tr("Clear displayed write data"), this))
    , clearReadLog(new QAction(QIcon(":/Resources/Clear.png"), tr("Clear displayed read data"), this))
    , saveWriteLog(new QAction(QIcon(":/Resources/Save.png"), tr("Save write log"), this))
    , saveReadLog(new QAction(QIcon(":/Resources/Save.png"), tr("Save read log"), this))
    , recordWriteLog(new QAction(QIcon(":/Resources/Record.png"), tr("Record write log"), this))
    , recordReadLog(new QAction(QIcon(":/Resources/Record.png"), tr("Record read log"), this))
    , manualSendPacket(new QAction(QIcon(":/Resources/Send.png"), tr("Send packet"), this))
    , txLabel(new QLabel("Tx: 0", this))
    , rxLabel(new QLabel("Rx: 0", this))
    , sheetRead(new LimitedTextEdit(this))
    , sheetWrite(new LimitedTextEdit(this))
    , manualRepeatSendTime(new QSpinBox(this))
    , readDelayBetweenPackets(new QSpinBox(this))
    , manualPacketEdit(new QLineEdit(this))
    , manualSeparatorEdit(new QLineEdit(" ", this))
    , displayWrite(new QAction(QIcon(":/Resources/Display.png"), DEFAULT_WRITE_DISPLAYING, this))
    , displayRead(new QAction(QIcon(":/Resources/Display.png"), DEFAULT_READ_DISPLAYING, this))
    , manualCR(new QAction(QIcon(":/Resources/CR.png"), tr("Add CR"), this))
    , manualLF(new QAction(QIcon(":/Resources/LF.png"), tr("Add LF"), this))
    , manualSendMode(new QAction(QIcon(":Resources/Single.png"), DEFAULT_SEND_MODE, this))
    , port(new QSerialPort(this))
    , comPortConfigure(new ComPortConfigure(port, this))
    , settings(new QSettings("settings.ini", QSettings::IniFormat))
    , fileDialog(new QFileDialog(this))
    , packetTimeCalculator(new PacketTimeCalculator(port))
    , txCount(0)
    , rxCount(0)
    , logWrite(false)
    , logRead(false)
    , hexEncoder(new HexEncoder)
    , decEncoder(new DecEncoder)
    , asciiEncoder(new AsciiEncoder)
{
    setWindowTitle(title);

    view();
    connections();

    manualCR->setCheckable(true);
    manualLF->setCheckable(true);

    sheetRead->displayTime("hh:mm:ss.zzz");
    sheetRead->setReadOnly(true);
    sheetWrite->displayTime("hh:mm:ss.zzz");
    sheetWrite->setReadOnly(true);

    comPortConfigure->setWindowTitle(tr("Port configure"));
    comPortConfigure->setModal(true);

    macros->setPacketTimeCalculator(packetTimeCalculator);
    macros->setWorkState(false);
    macrosDockWidget->setWidget(macros);
    macrosDockWidget->setFixedWidth(310);
    macrosDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setDockOptions(AllowNestedDocks | AllowTabbedDocks);

    recordWriteLog->setCheckable(true);
    recordReadLog->setCheckable(true);
    manualSendPacket->setCheckable(true);
    manualRepeatSendTime->setRange(1, 100000);
    manualRepeatSendTime->setEnabled(false);
    readDelayBetweenPackets->setRange(0, 10);
    readDelayBetweenPackets->setValue(10);

    portName->setFrameStyle(QFrame::Sunken);
    portName->setFrameShape(QFrame::Box);
    baud->setFrameStyle(QFrame::Sunken);
    baud->setFrameShape(QFrame::Box);
    bits->setFrameStyle(QFrame::Sunken);
    bits->setFrameShape(QFrame::Box);
    parity->setFrameStyle(QFrame::Sunken);
    parity->setFrameShape(QFrame::Box);
    stopBits->setFrameStyle(QFrame::Sunken);
    stopBits->setFrameShape(QFrame::Box);
    txLabel->setFrameStyle(QFrame::Sunken);
    txLabel->setFrameShape(QFrame::Box);
    rxLabel->setFrameStyle(QFrame::Sunken);
    rxLabel->setFrameShape(QFrame::Box);

    QStringList buffer;
    buffer << "ASCII" << "HEX" << "DEC";
    manualSendEncodingMode->addItems(buffer);
    readEncodingMode->addItems(buffer);
    writeEncodingMode->addItems(buffer);

    QDir dir;
    if(!dir.exists(dir.currentPath() + "/Macros")) {
        dir.mkpath(dir.currentPath() + "/Macros");
    }
    if(!dir.exists(dir.currentPath() + "/WriteLogs")) {
        dir.mkpath(dir.currentPath() + "/WriteLogs");
    }
    if(!dir.exists(dir.currentPath() + "/ReadLogs")) {
        dir.mkpath(dir.currentPath() + "/ReadLogs");
    }
    fileDialog->setDirectory(dir.currentPath() + "/Macros");
    fileDialog->setFileMode(QFileDialog::ExistingFiles);

    loadSession();
}

void MainWindow::view()
{
    QList<QAction*> actions;
    actions << actionPortConfigure << actionStartStop << actionMacros;
    QToolBar* toolBar = new QToolBar(this);
    addToolBar(Qt::TopToolBarArea, toolBar);
    toolBar->setMovable(false);
    toolBar->addActions(actions);

    QWidgetAction *actionReadDelayBetweenPackets = new QWidgetAction(this);
    actionReadDelayBetweenPackets->setDefaultWidget(readDelayBetweenPackets);
    readDelayBetweenPackets->setToolTip(tr("Read delay\nbetween packets, ms"));
    QWidgetAction *actionManualSendMode = new QWidgetAction(this);
    actionManualSendMode->setDefaultWidget(manualSendEncodingMode);
    manualSendEncodingMode->setToolTip(tr("Mode"));
    QWidgetAction *actionManualPacketEdit = new QWidgetAction(this);
    actionManualPacketEdit->setDefaultWidget(manualPacketEdit);
    QWidgetAction *actionManualRepeatSendTime = new QWidgetAction(this);
    actionManualRepeatSendTime->setDefaultWidget(manualRepeatSendTime);
    manualRepeatSendTime->setToolTip(tr("Repeat send time, ms"));
    QWidgetAction *actionSeparatorEdit = new QWidgetAction(this);
    actionSeparatorEdit->setDefaultWidget(manualSeparatorEdit);
    manualSeparatorEdit->setMaximumWidth(20);
    manualSeparatorEdit->setToolTip(tr("Separator symbol"));
    actions.clear();
    actions << actionReadDelayBetweenPackets << actionSeparatorEdit << actionManualSendMode << actionManualPacketEdit
            << actionManualPacketEdit << manualCR << manualLF << manualSendMode << actionManualRepeatSendTime << manualSendPacket;
    QToolBar* manualSendToolBar = new QToolBar(this);
    addToolBar(Qt::BottomToolBarArea, manualSendToolBar);
    manualSendToolBar->setMovable(false);
    manualSendToolBar->addActions(actions);
    manualSendToolBar->setStyleSheet("spacing:2px");

    txLabel->setToolTip(tr("Written bytes count"));
    rxLabel->setToolTip(tr("Read bytes count"));

    QStatusBar *statusBar = new QStatusBar(this);
    statusBar->addWidget(portName);
    statusBar->addWidget(baud);
    statusBar->addWidget(bits);
    statusBar->addWidget(parity);
    statusBar->addWidget(stopBits);
    statusBar->addWidget(txLabel);
    statusBar->addWidget(rxLabel);
    setStatusBar(statusBar);

    QWidgetAction *actionWriteMode = new QWidgetAction(this);
    actionWriteMode->setDefaultWidget(writeEncodingMode);
    actions.clear();
    actions << actionWriteMode << displayWrite << recordWriteLog
            << saveWriteLog << clearWriteLog;
    QToolBar *writeToolBar = new QToolBar(this);
    writeToolBar->setAllowedAreas(Qt::TopToolBarArea);
    writeToolBar->setMovable(false);
    writeToolBar->setStyleSheet("spacing:2px");
    writeToolBar->addActions(actions);
    QMainWindow *writeWindow = new QMainWindow(this);
    writeWindow->addToolBar(writeToolBar);
    writeWindow->setCentralWidget(sheetWrite);

    QGridLayout *writeLayout = new QGridLayout;
    writeLayout->addWidget(writeWindow, 0, 0);
    writeLayout->setContentsMargins(5, 5, 5, 5);

    QGroupBox *gbWrite = new QGroupBox(tr("Write"), this);
    gbWrite->setLayout(writeLayout);


    QWidgetAction *actionReadMode = new QWidgetAction(this);
    actionReadMode->setDefaultWidget(readEncodingMode);
    actions.clear();
    actions << actionReadMode << displayRead << recordReadLog
            << saveReadLog << clearReadLog;
    QToolBar *readToolBar = new QToolBar(this);
    readToolBar->setAllowedAreas(Qt::TopToolBarArea);
    readToolBar->setMovable(false);
    readToolBar->setStyleSheet("spacing:2px");
    readToolBar->addActions(actions);
    QMainWindow *readWindow = new QMainWindow(this);
    readWindow->addToolBar(readToolBar);
    readWindow->setCentralWidget(sheetRead);

    QGridLayout *readLayout = new QGridLayout;
    readLayout->addWidget(readWindow, 0, 0);
    readLayout->setContentsMargins(5, 5, 5, 5);

    QGroupBox *gbRead = new QGroupBox(tr("Read"), this);
    gbRead->setLayout(readLayout);


    QSplitter *splitter = new QSplitter;
    splitter->addWidget(gbWrite);
    splitter->addWidget(gbRead);
    splitter->setHandleWidth(1);
    splitter->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    QGridLayout *dataLayout = new QGridLayout;
    dataLayout->addWidget(splitter, 0, 0);
    dataLayout->setSpacing(0);
    dataLayout->setContentsMargins(0, 0, 0, 0);

    QGridLayout *mainLayouts = new QGridLayout;
    mainLayouts->setSpacing(5);
    mainLayouts->setContentsMargins(5, 5, 5, 5);
    mainLayouts->addLayout(dataLayout, 0, 0);
    QWidget *widget = new QWidget(this);
    widget->setLayout(mainLayouts);
    setCentralWidget(widget);
}

void MainWindow::connections()
{
    connect(actionPortConfigure, &QAction::triggered, comPortConfigure, &ComPortConfigure::show);
    connect(displayWrite, &QAction::triggered, this, &MainWindow::toggleWriteDisplay);
    connect(displayRead, &QAction::triggered, this, &MainWindow::toggleReadDisplay);
    connect(clearReadLog, &QAction::triggered, sheetRead, &LimitedTextEdit::clear);
    connect(clearWriteLog, &QAction::triggered, sheetWrite, &LimitedTextEdit::clear);
    connect(actionStartStop, &QAction::triggered, this, &MainWindow::startStop);
    connect(actionStartStop, &QAction::triggered, this, &MainWindow::manualPacketEdited);
    connect(actionMacros, &QAction::triggered, this, &MainWindow::toggleMacrosView);
    connect(saveWriteLog, &QAction::triggered, this, &MainWindow::saveWrite);
    connect(saveReadLog, &QAction::triggered, this, &MainWindow::saveRead);
    connect(recordWriteLog, &QAction::triggered, this, &MainWindow::startWriteLog);
    connect(recordReadLog, &QAction::triggered, this, &MainWindow::startReadLog);
    connect(manualSendEncodingMode, &QComboBox::currentTextChanged, this, &MainWindow::onManualModeSelect);
    connect(manualSendEncodingMode, &QComboBox::currentTextChanged, this, &MainWindow::manualPacketEdited);
    connect(manualSendPacket, &QAction::triggered, this, &MainWindow::startSending);
    connect(manualPacketEdit, &QLineEdit::returnPressed, [this](){startSending();});
    connect(manualCR, &QAction::triggered, this, &MainWindow::manualPacketEdited);
    connect(manualLF, &QAction::triggered, this, &MainWindow::manualPacketEdited);
    connect(manualSendTimer, SIGNAL(timeout()), this, SLOT(singleSend()));
    connect(manualSendMode, &QAction::triggered, this, &MainWindow::toggleManualSendMode);
    connect(delayBetweenPacketsTimer, &QTimer::timeout, this, &MainWindow::delayBetweenPacketsEnded);
    connect(writeLogTimer, SIGNAL(timeout()), this, SLOT(writeLogTimeout()));
    connect(readLogTimer, SIGNAL(timeout()), this, SLOT(readLogTimeout()));
    connect(port, SIGNAL(readyRead()), this, SLOT(received()));
    connect(macros, &Macros::packetSended, this, static_cast<void (MainWindow::*)(const QByteArray &)>(&MainWindow::sendPacket));
    connect(macrosDockWidget, &QDockWidget::topLevelChanged, this, &MainWindow::setMacrosMinimizeFeature);
    connect(macrosDockWidget, &QDockWidget::dockLocationChanged, this, &MainWindow::saveCurrentMacrosArea);
    connect(manualPacketEdit, &QLineEdit::textEdited, this, &MainWindow::manualPacketEdited);
}

void MainWindow::startStop()
{
    if(actionStartStop->text() == tr("Open COM-port")) {
        start();
    } else {
        stop();
    }
}

void MainWindow::sendPacket(const QByteArray &data)
{
    Macros *macros = qobject_cast<Macros*>(sender());
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if(macros == 0 && b == 0) {
        return;
    }
    bool macro = b == 0 ? true : false;
    sendPacket(data, macro);
}

void MainWindow::writeLogTimeout()
{
    writeLogTimer->stop();
    writeLogFile.close();
    logWrite = false;
}

void MainWindow::readLogTimeout()
{
    readLogTimer->stop();
    readLogFile.close();
    logRead = false;
}

void MainWindow::startWriteLog(bool check)
{
    if(!check) {
        writeLogTimeout();

        return;
    }
    QString path = QDir::currentPath() + "/WriteLogs" + "/(WRITE)_" + QDateTime::currentDateTime().toString("yyyy.MM.dd_HH.mm.ss") + ".txt";
    writeLogFile.setFileName(path);
    if(!writeLogFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save file"));

        return;
    }
    writeLogTimer->start();
    logWrite = true;
}

void MainWindow::startReadLog(bool check)
{
    if(!check) {
        readLogTimeout();

        return;
    }

    QString path = QDir::currentPath() + "/ReadLogs" + "/(READ)_" + QDateTime::currentDateTime().toString("yyyy.MM.dd_HH.mm.ss") + ".txt";
    readLogFile.setFileName(path);
    if(!readLogFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save file"));

        return;
    }
    readLogTimer->start();
    logRead = true;
}

void MainWindow::start()
{
    port->close();

    if(!port->open(QSerialPort::ReadWrite)) {
        txLabel->setStyleSheet("background: none");
        rxLabel->setStyleSheet("background: none");

        return;
    }

    port->setFlowControl(QSerialPort::NoFlowControl);

    actionPortConfigure->setEnabled(false);

    txLabel->setStyleSheet("background: yellow");
    rxLabel->setStyleSheet("background: yellow");

    txCount = 0;
    txLabel->setText("Tx: 0");
    rxCount = 0;
    rxLabel->setText("Rx: 0");
    portName->setText(PORT + port->portName());
    baud->setText(BAUD + baudToString(port->baudRate()));
    bits->setText(BITS + bitsToString(port->dataBits()));
    parity->setText(PARITY + parityToString(port->parity()));
    stopBits->setText(STOP_BITS + stopBitsToString(port->stopBits()));

    actionStartStop->setIcon(QIcon(":/Resources/Stop.png"));
    actionStartStop->setText(tr("Close COM-port"));

    macros->setWorkState(true);
}

void MainWindow::stop()
{
    port->close();
    txLabel->setStyleSheet("background: none");
    rxLabel->setStyleSheet("background: none");

    actionPortConfigure->setEnabled(true);
    manualSendPacket->setChecked(false);
    manualSendTimer->stop();
    delayBetweenPacketsTimer->stop();
    portName->setText(PORT + tr("None"));
    baud->setText(BAUD + tr("None"));
    bits->setText(BITS + tr("None"));
    parity->setText(PARITY + tr("None"));
    stopBits->setText(STOP_BITS + tr("None"));

    actionStartStop->setIcon(QIcon(":/Resources/Play.png"));
    actionStartStop->setText(tr("Open COM-port"));

    macros->setWorkState(false);
}

void MainWindow::received()
{
    readBuffer += port->readAll();
    int delay = readDelayBetweenPackets->value();
    if(delay == 0) {
        delayBetweenPacketsEnded();

        return;
    }
    delayBetweenPacketsTimer->start(delay);
}

void MainWindow::singleSend()
{
    DataEncoder *dataEncoder = getEncoder(manualSendEncodingMode->currentIndex());
    dataEncoder->setData(manualPacketEdit->text(), manualSeparatorEdit->text());
    sendPacket(dataEncoder->encodedByteArray(), false);
}

void MainWindow::saveReadWriteLog(bool writeLog)
{
    QString defaultFileName = (writeLog ? "(WRITE)_" : "(READ)_")
            + QDateTime::currentDateTime().toString("yyyy.MM.dd_HH.mm.ss")
            + ".txt";
    QString fileName = fileDialog->getSaveFileName(this,
                                                   tr("Save File"),
                                                   QDir::currentPath()
                                                   + (writeLog ? "/WriteLogs/" : "/ReadLogs/")
                                                   + defaultFileName,
                                                   tr("Log Files (*.txt)"));
    if(fileName.isEmpty()) {
        return;
    }
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save file"));

        return;
    }
    QTextStream stream(&file);
    if(writeLog) {
        stream << sheetWrite->toPlainText();
    } else {
        stream << sheetRead->toPlainText();
    }
    file.close();
}

void MainWindow::saveWrite()
{
    saveReadWriteLog(true);
}

void MainWindow::saveRead()
{
    saveReadWriteLog(false);
}

void MainWindow::startSending(bool checked)
{
    if(!checked) {
        manualSendTimer->stop();

        return;
    }

    if(port->isOpen()) {
        if(manualSendMode->toolTip() != DEFAULT_SEND_MODE) {
            manualSendTimer->setInterval(manualRepeatSendTime->value());
            manualSendTimer->start();

            return;
        }
        singleSend();
    }
    manualSendPacket->setChecked(false);
}

void MainWindow::sendPacket(const QByteArray &writeData, bool macro)
{
    if(!port->isOpen() || writeData.isEmpty()) {
        return;
    }

    QByteArray modifiedData = writeData;
    if(!macro) {
        manualSendTimer->setInterval(manualRepeatSendTime->value());

        if(manualCR->isChecked()) {
            modifiedData.append(CR);
        }
        if(manualLF->isChecked()) {
            modifiedData.append(LF);
        }
    }

    displayWrittenData(modifiedData);
    txCount += port->write(modifiedData);
    txLabel->setText("Tx: " + QString::number(txCount));

    if(!txTimer->isSingleShot()) {
        txLabel->setStyleSheet("background: green");
        txTimer->singleShot(BLINK_TIME_TX, this, &MainWindow::txNone);
        txTimer->setSingleShot(true);
    }
}

void MainWindow::displayWrittenData(const QByteArray &writeData)
{
    if(displayWrite->toolTip() != DEFAULT_WRITE_DISPLAYING) {
        return;
    }

    DataEncoder *dataEncoder = getEncoder(writeEncodingMode->currentIndex());
    dataEncoder->setData(writeData);
    QString displayString = dataEncoder->encodedStringList().join(" ");
    sheetWrite->addLine(displayString);
    QScrollBar *sb = sheetWrite->verticalScrollBar();
    sb->setValue(sb->maximum());

    if(logWrite) {
        QTextStream writeStream (&writeLogFile);
        writeStream << displayString + "\n";
    }
}

DataEncoder *MainWindow::getEncoder(int mode)
{
    DataEncoder *dataEncoder = 0;
    switch(mode) {
    case ASCII:
        dataEncoder = asciiEncoder;
        break;
    case HEX:
        dataEncoder = hexEncoder;
        break;
    default:
        dataEncoder = decEncoder;
    }

    return dataEncoder;
}

QString MainWindow::baudToString(int baud)
{
    QString baudString;
    switch(baud) {
    case QSerialPort::Baud1200:
        baudString = "1200";
        break;
    case QSerialPort::Baud2400:
        baudString = "2400";
        break;
    case QSerialPort::Baud4800:
        baudString = "4800";
        break;
    case QSerialPort::Baud9600:
        baudString = "9600";
        break;
    case QSerialPort::Baud19200:
        baudString = "19200";
        break;
    case QSerialPort::Baud38400:
        baudString = "38400";
        break;
    case QSerialPort::Baud57600:
        baudString = "57600";
        break;
    case QSerialPort::Baud115200:
        baudString = "115200";
        break;
    case 230400:
        baudString = "230400";
        break;
    case 460800:
        baudString = "460800";
        break;
    default:
        baudString = "921600";
        break;
    }

    return baudString;
}

QString MainWindow::bitsToString(int bits)
{
    QString bitsString;
    switch(bits) {
    case QSerialPort::Data5:
       bitsString = "5";
        break;
    case QSerialPort::Data6:
        bitsString = "6";
        break;
    case QSerialPort::Data7:
        bitsString = "7";
        break;
    default:
        bitsString = "8";
        break;
    }

    return bitsString;
}

QString MainWindow::parityToString(int parity)
{
    QString parityString;
    switch(parity) {
    case QSerialPort::NoParity:
        parityString = "No";
        break;
    case QSerialPort::OddParity:
        parityString = "Odd";
        break;
    case QSerialPort::EvenParity:
        parityString = "Even";
        break;
    case QSerialPort::MarkParity:
        parityString = "Mark";
        break;
    default:
        parityString = "Space";
        break;
    }

    return parityString;
}

QString MainWindow::stopBitsToString(int stopBits)
{
    QString stopBitsString;
    switch(stopBits) {
    case QSerialPort::OneStop:
        stopBitsString = "1";
        break;
    case QSerialPort::OneAndHalfStop:
        stopBitsString = "1.5";
        break;
    default:
        stopBitsString = "2";
        break;
    }

    return stopBitsString;
}

void MainWindow::setMacrosMinimizeFeature(bool floating)
{
    if(floating) {
        macrosDockWidget->setWindowFlags(Qt::Dialog | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
        macrosDockWidget->show();
    }
}

void MainWindow::toggleMacrosView()
{
    if(macrosDockWidget->toggleViewAction()->isChecked()) {
        macrosDockWidget->close();

        return;
    }

    macrosDockWidget->show();
}

void MainWindow::toggleWriteDisplay()
{
    if(displayWrite->toolTip() == DEFAULT_WRITE_DISPLAYING) {
        displayWrite->setIcon(QIcon(":/Resources/Hide.png"));
        displayWrite->setToolTip(tr("Hiding write data"));
    } else {
        displayWrite->setIcon(QIcon(":/Resources/Display.png"));
        displayWrite->setToolTip(DEFAULT_WRITE_DISPLAYING);
    }
}

void MainWindow::toggleReadDisplay()
{
    if(displayRead->toolTip() == DEFAULT_READ_DISPLAYING) {
        displayRead->setIcon(QIcon(":/Resources/Hide.png"));
        displayRead->setToolTip(tr("Hiding read data"));
    } else {
        displayRead->setIcon(QIcon(":/Resources/Display.png"));
        displayRead->setToolTip(DEFAULT_READ_DISPLAYING);
    }
}

void MainWindow::saveCurrentMacrosArea(Qt::DockWidgetArea area)
{
    macrosDockWidgetArea = area;
}

void MainWindow::onManualModeSelect()
{
    if(manualSendEncodingMode->currentIndex() == ASCII) {
        manualSeparatorEdit->setEnabled(false);

        return;
    }

    manualSeparatorEdit->setEnabled(true);
}

void MainWindow::toggleManualSendMode()
{
    if(manualSendMode->toolTip() == DEFAULT_SEND_MODE) {
        manualSendMode->setIcon(QIcon(":Resources/Period.png"));
        manualSendMode->setToolTip(tr("Periodic send"));
        manualRepeatSendTime->setEnabled(true);
    } else {
        manualSendMode->setIcon(QIcon(":Resources/Single.png"));
        manualSendMode->setToolTip(tr("Single send"));
        manualRepeatSendTime->setEnabled(false);
        if(manualSendPacket->isChecked()) {
            manualSendPacket->setChecked(false);
            manualSendTimer->stop();
        }
    }
}

void MainWindow::manualPacketEdited()
{
    int minimumTime = 1;
    if(packetTimeCalculator != 0 && packetTimeCalculator->isValid()) {
        DataEncoder *dataEncoder = getEncoder(manualSendEncodingMode->currentIndex());
        dataEncoder->setData(manualPacketEdit->text(), manualSeparatorEdit->text());
        int additionalBytes = (manualCR->isChecked() ? 1 : 0) + (manualLF->isChecked() ? 1 : 0);
        minimumTime = qCeil(packetTimeCalculator->calculateTime(dataEncoder->encodedByteArray().size() + additionalBytes));
    }
    manualRepeatSendTime->setMinimum(minimumTime);
}

// Перевод строки при приеме данных
// Срабатывает по таймеру m_timerDelayBetweenPackets
// Определяет отображаемую длину принятого пакета
void MainWindow::delayBetweenPacketsEnded()
{
    delayBetweenPacketsTimer->stop();

    if(!rxTimer->isSingleShot()) {
        rxLabel->setStyleSheet("background: red");
        rxTimer->singleShot(BLINK_TIME_RX, this, &MainWindow::rxNone);
        rxTimer->setSingleShot(true);
    }
    rxCount+=readBuffer.size();
    rxLabel->setText("Rx: " + QString::number(rxCount));

    if(displayRead->toolTip() == DEFAULT_READ_DISPLAYING || logRead) {
        DataEncoder *dataEncoder = getEncoder(readEncodingMode->currentIndex());
        dataEncoder->setData(readBuffer);

        if(displayRead->toolTip() == DEFAULT_READ_DISPLAYING) {
            sheetRead->addLine(dataEncoder->encodedStringList().join(" "));
            QScrollBar *sb = sheetRead->verticalScrollBar();
            sb->setValue(sb->maximum());
        }

        if(logRead) {
            QTextStream readStream(&readLogFile);
            readStream << dataEncoder->encodedStringList().join(" ") + "\n";
        }
    }
    readBuffer.clear();
}

void MainWindow::rxNone()
{
    rxLabel->setStyleSheet("background: yellow");
    rxTimer->singleShot(BLINK_TIME_RX, this, SLOT(rxHold()));
}

void MainWindow::txNone()
{
    txLabel->setStyleSheet("background: yellow");
    txTimer->singleShot(BLINK_TIME_TX, this, SLOT(txHold()));
}

void MainWindow::rxHold()
{
    rxTimer->setSingleShot(false);
}

void MainWindow::txHold()
{
    txTimer->setSingleShot(false);
}

void MainWindow::saveSession()
{
    settings->remove("main");
    settings->setValue("main/size", currentWindowSize);
    settings->setValue("main/position", currentWindowPos);

    settings->setValue("main/write_encoding_mode", writeEncodingMode->currentIndex());
    settings->setValue("main/read_encoding_mode", readEncodingMode->currentIndex());
    settings->setValue("main/max_write_sheet_rows", sheetWrite->linesLimit());
    settings->setValue("main/max_read_sheet_rows", sheetRead->linesLimit());
    settings->setValue("main/write_display", displayWrite->toolTip() == DEFAULT_WRITE_DISPLAYING ? 0 : 1);
    settings->setValue("main/read_display", displayRead->toolTip() == DEFAULT_READ_DISPLAYING ? 0 : 1);
    settings->setValue("main/write_log_timeout", writeLogTimer->interval());
    settings->setValue("main/read_log_timeout", readLogTimer->interval());

    comPortConfigure->saveSettings(settings);
    macros->saveSettings(settings);

    settings->setValue("main/read_delay_between_packets", readDelayBetweenPackets->value());
    settings->setValue("main/separator", manualSeparatorEdit->text().isEmpty() ? DEFAULT_SEPARATOR : manualSeparatorEdit->text());
    settings->setValue("main/manual_encoding_send_mode", manualSendEncodingMode->currentIndex());
    settings->setValue("main/repeat_send_interval", manualRepeatSendTime->value());
    settings->setValue("main/send_mode", manualSendMode->toolTip() == DEFAULT_SEND_MODE ? 0 : 1);
    settings->setValue("main/CR", manualCR->isChecked());
    settings->setValue("main/LF", manualLF->isChecked());

    settings->setValue("main/macros_dock_widget_area", macrosDockWidgetArea);
    settings->setValue("main/macros_dock_widget_hidden", macrosDockWidget->isHidden());
}

void MainWindow::loadSession()
{
    QSize size = settings->value("main/size").toSize();
    if(size.isValid()) {
        resize(size);
    }
    QPoint pos = settings->value("main/position").toPoint();
    if(!pos.isNull()) {
        move(pos);
    }

    sheetRead->setLinesLimit(settings->value("main/max_write_sheet_rows", DEFAULT_LOG_ROWS).toInt());
    sheetWrite->setLinesLimit(settings->value("main/max_read_sheet_rows", DEFAULT_LOG_ROWS).toInt());

    comPortConfigure->loadSettings(settings);
    macros->loadSettings(settings);

    writeEncodingMode->setCurrentIndex(settings->value("main/write_encoding_mode", DEFAULT_MODE).toInt());
    readEncodingMode->setCurrentIndex(settings->value("main/read_encoding_mode", DEFAULT_MODE).toInt());
    if(settings->value("main/write_display", 0).toInt() == 0) {
        displayWrite->setIcon(QIcon(":/Resources/Display.png"));
        displayWrite->setToolTip(DEFAULT_WRITE_DISPLAYING);
    } else {
        displayWrite->setIcon(QIcon(":/Resources/Hide.png"));
        displayWrite->setToolTip(tr("Hiding write data"));
    }
    if(settings->value("main/read_display", 0).toInt() == 0) {
        displayRead->setIcon(QIcon(":/Resources/Display.png"));
        displayRead->setToolTip(DEFAULT_READ_DISPLAYING);
    } else {
        displayRead->setIcon(QIcon(":/Resources/Hide.png"));
        displayRead->setToolTip(tr("Hiding read data"));
    }
    writeLogTimer->setInterval(settings->value("main/write_log_timeout", DEFAULT_LOG_TIMEOUT).toInt());
    readLogTimer->setInterval(settings->value("main/read_log_timeout", DEFAULT_LOG_TIMEOUT).toInt());

    readDelayBetweenPackets->setValue(settings->value("main/read_delay_between_packets", DEFAULT_READ_DELAY).toInt());
    manualSeparatorEdit->setText(settings->value("main/separator", DEFAULT_SEPARATOR).toString());
    manualSendEncodingMode->setCurrentIndex(settings->value("main/manual_encoding_send_mode", DEFAULT_MODE).toInt());
    manualRepeatSendTime->setValue(settings->value("main/repeat_send_interval", DEFAULT_SEND_TIME).toInt());
    if(settings->value("main/send_mode", 0).toInt() == 0) {
        manualSendMode->setIcon(QIcon(":Resources/Single.png"));
        manualSendMode->setToolTip(tr("Single send"));
        manualRepeatSendTime->setEnabled(false);
    } else {
        manualSendMode->setIcon(QIcon(":Resources/Period.png"));
        manualSendMode->setToolTip(tr("Periodic send"));
        manualRepeatSendTime->setEnabled(true);
    }
    manualCR->setChecked(settings->value("main/CR", DEFAULT_CR_LF).toBool());
    manualLF->setChecked(settings->value("main/LF", DEFAULT_CR_LF).toBool());

    addDockWidget(static_cast<Qt::DockWidgetArea>(settings->value("main/macros_dock_widget_area",
                                                                  DEFAULT_MACROS_AREA).toInt()),
                  macrosDockWidget);
    macrosDockWidget->setHidden(settings->value("main/macros_dock_widget_hidden", DEFAULT_MACROS_HIDDEN).toBool());
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    saveSession();

    QWidget::closeEvent(e);
}

void MainWindow::moveEvent(QMoveEvent *e)
{
    if(isMaximized()) {
        currentWindowPos = e->pos();
    } else {
        currentWindowPos = e->oldPos();
    }

    QWidget::moveEvent(e);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    if(isMaximized()) {
        currentWindowSize = e->size();
    } else {
        currentWindowSize = e->oldSize();
    }

    QWidget::resizeEvent(e);
}

void MainWindow::showEvent(QShowEvent *e)
{
    currentWindowSize = size();

    QWidget::showEvent(e);
}
