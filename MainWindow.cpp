#include <QDebug>
#include "MainWindow.h"
#include "MiniMacros.h"
#include <QGridLayout>
#include <QSerialPortInfo>
#include <QCloseEvent>
#include <QSplitter>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QDir>

static unsigned short int BLINKTIMETX = 200;
static unsigned short int BLINKTIMERX = 200;

MainWindow::MainWindow(QString title, QWidget *parent)
    : QMainWindow(parent)
    , m_cbPort(new QComboBox(this))
    , m_cbBaud(new QComboBox(this))
    , m_cbBits(new QComboBox(this))
    , m_cbParity(new QComboBox(this))
    , m_cbStopBits(new QComboBox(this))
    , m_cbSendMode(new QComboBox(this))
    , m_cbReadMode(new QComboBox(this))
    , m_cbWriteMode(new QComboBox(this))
    , m_tSend(new QTimer(this))
    , m_tEcho(new QTimer(this))
    , m_tWriteLog(new QTimer(this))
    , m_tReadLog(new QTimer(this))
    , m_tIntervalSending(new QTimer(this))
    , m_tDelay(new QTimer(this))
    , m_tTx(new QTimer(this))
    , m_tRx(new QTimer(this))
    , m_bStart(new QPushButton("Start", this))
    , m_bStop(new QPushButton("Stop", this))
    , m_abPause(new QPushButton("Pause", this))
    , m_bWriteLogClear(new QPushButton("Clear", this))
    , m_bReadLogClear(new QPushButton("Clear", this))
    , m_bSaveWriteLog(new QPushButton("Save", this))
    , m_bSaveReadLog(new QPushButton("Save", this))
    , m_bHiddenGroup(new QPushButton(">", this))
    , m_bDeleteAllMacroses(new QPushButton(this))
    , m_bAddMacros(new QPushButton(this))
    , m_bLoadMacroses(new QPushButton(this))
    , m_abSaveWriteLog(new QPushButton(this))
    , m_abSaveReadLog(new QPushButton(this))
    , m_abSendPackage(new QPushButton("Send", this))
    , m_lTx(new QLabel("        Tx", this))
    , m_lRx(new QLabel("        Rx", this))
    , m_lTxCount(new QLabel("Tx: 0", this))
    , m_lRxCount(new QLabel("Rx: 0", this))
    , m_eLogRead(new MyListWidget(this))
    , m_eLogWrite(new MyListWidget(this))
    , m_sbRepeatSendInterval(new QSpinBox(this))
    , m_sbEchoInterval(new QSpinBox(this))
    , m_sbDelay(new QSpinBox(this))
    , m_sbAllDelay(new QSpinBox(this))
    , m_leSendPackage(new QLineEdit(this))
    , m_cbEchoMaster(new QCheckBox("Echo master", this))
    , m_cbEchoSlave(new QCheckBox("Echo slave", this))
    , m_cbReadScroll(new QCheckBox("Scrolling", this))
    , m_cbWriteScroll(new QCheckBox("Scrolling", this))
    , m_cbAllIntervals(new QCheckBox("Interval", this))
    , m_cbAllPeriods(new QCheckBox("Period", this))
    , m_cbDisplayWrite(new QCheckBox("Display", this))
    , m_cbDisplayRead(new QCheckBox("Display", this))
    , m_cbUniformSizes(new QCheckBox("Uniform sizes", this))
    , m_Port(new QSerialPort(this))
    , settings(new QSettings("settings.ini", QSettings::IniFormat))
    , fileDialog(new QFileDialog(this))
    , m_gbHiddenGroup(new QGroupBox(this))
    , spacer(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding))
    , scrollAreaLayout(new QVBoxLayout)
    , scrollArea(new QScrollArea(m_gbHiddenGroup))
    , widgetScroll(new QWidget(scrollArea))
    , HiddenLayout(new QVBoxLayout(widgetScroll))
{
    m_Port->setSettingsRestoredOnClose(false);
    setWindowTitle(title);
    resize(settings->value("config/width").toInt(),
           settings->value("config/height").toInt());
    view();
    connections();
    setMinimumWidth(665);

    m_Port->setReadBufferSize(1);

    txCount = 0;
    rxCount = 0;
    logWrite = false;
    logRead = false;
    index = 0;
    echoWaiting = false;
    sendCount = 0;
    sendIndex = 0;

    m_bAddMacros->setStyleSheet("border-image: url(:/Resources/add.png) stretch;");
    m_bLoadMacroses->setStyleSheet("border-image: url(:/Resources/open.png) stretch;");
    m_bAddMacros->setFixedSize(20, 20);
    m_bLoadMacroses->setFixedSize(20, 20);
    m_sbAllDelay->setValue(50);
    m_abSaveReadLog->setIcon(QIcon(":/Resources/startRecToFile.png"));
    m_abSaveWriteLog->setIcon(QIcon(":/Resources/startRecToFile.png"));
    m_abSaveWriteLog->setCheckable(true);
    m_abSaveReadLog->setCheckable(true);
    m_abSendPackage->setCheckable(true);
    m_abPause->setCheckable(true);
    m_abSendPackage->setEnabled(false);
    m_bStop->setEnabled(false);
    m_abPause->setEnabled(false);
    m_cbPort->setEditable(true);
    m_sbRepeatSendInterval->setRange(0, 100000);
    m_sbEchoInterval->setRange(0, 100000);
    m_sbDelay->setRange(1, 100000);
    m_sbDelay->setValue(10);
    m_sbAllDelay->setRange(0, 999999);

    m_lTxCount->setStyleSheet("border-style: outset; border-width: 1px; border-color: black;");
    m_lRxCount->setStyleSheet("border-style: outset; border-width: 1px; border-color: black;");

    m_lTx->setStyleSheet("font: bold; font-size: 10pt");
    m_lRx->setStyleSheet("font: bold; font-size: 10pt");

    QStringList buffer;
    foreach(QSerialPortInfo portsAvailable, QSerialPortInfo::availablePorts())
    {
        buffer << portsAvailable.portName();
    }
    m_cbPort->addItems(buffer);
    m_cbPort->setEditable(true);

    buffer.clear();
    buffer << "921600" << "115200" << "57600" << "38400" << "19200"
           << "9600" << "4800" << "2400" << "1200";
    m_cbBaud->addItems(buffer);
    buffer.clear();
    buffer << "8" << "7" << "6" << "5";
    m_cbBits->addItems(buffer);
    buffer.clear();
    buffer << "None" << "Odd" << "Even" << "Mark" << "Space";
    m_cbParity->addItems(buffer);
    buffer.clear();
    buffer << "1" << "1.5" << "2";
    m_cbStopBits->addItems(buffer);
    buffer.clear();
    buffer << "HEX" << "ASCII" << "DEC";
    m_cbSendMode->addItems(buffer);
    m_cbReadMode->addItems(buffer);
    m_cbWriteMode->addItems(buffer);

    QDir dir;
    if (!dir.exists(dir.currentPath()+"/Macros"))
        dir.mkpath(dir.currentPath()+"/Macros");
    fileDialog->setDirectory(dir.currentPath()+"/Macros");
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setNameFilter(trUtf8("Macro Files (*.rsmc)"));

    loadSession();
}

void MainWindow::view()
{
    QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::Minimum,
                                          QSizePolicy::Expanding);
    QGridLayout *configLayout = new QGridLayout;
    configLayout->addWidget(new QLabel("<img src=':/Resources/elisat.png' height='40' width='160'/>", this), 0, 0, 2, 2, Qt::AlignCenter);
    configLayout->addWidget(m_lTx, 2, 0);
    configLayout->addWidget(m_lRx, 2, 1);
    configLayout->addWidget(new QLabel("Port:", this), 3, 0);
    configLayout->addWidget(m_cbPort, 3, 1);
    configLayout->addWidget(new QLabel("Baud:", this), 4, 0);
    configLayout->addWidget(m_cbBaud, 4, 1);
    configLayout->addWidget(new QLabel("Data bits:", this), 5, 0);
    configLayout->addWidget(m_cbBits, 5, 1);
    configLayout->addWidget(new QLabel("Parity:", this), 6, 0);
    configLayout->addWidget(m_cbParity, 6, 1);
    configLayout->addWidget(new QLabel("Stop bits:", this), 7, 0);
    configLayout->addWidget(m_cbStopBits, 7, 1);
    configLayout->addWidget(m_cbEchoMaster, 8, 0);
    configLayout->addWidget(m_sbEchoInterval, 8, 1);
    configLayout->addWidget(m_cbEchoSlave, 9, 0);
    configLayout->addWidget(new QLabel("Delay:", this), 10, 0);
    configLayout->addWidget(m_sbDelay, 10, 1);
    configLayout->addWidget(m_bStart, 11, 0);
    configLayout->addWidget(m_bStop, 11, 1);
    configLayout->addWidget(m_cbUniformSizes, 13, 0, 1, 2);
    configLayout->addWidget(m_lTxCount, 14, 0);
    configLayout->addWidget(m_lRxCount, 14, 1);
    configLayout->addItem(spacer, 12, 0);
    configLayout->setSpacing(5);

    QHBoxLayout *sendPackageLayout = new QHBoxLayout;
    sendPackageLayout->addWidget(new QLabel("Mode:", this));
    sendPackageLayout->addWidget(m_cbSendMode);
    sendPackageLayout->addWidget(m_leSendPackage);
    sendPackageLayout->addWidget(m_sbRepeatSendInterval);
    sendPackageLayout->addWidget(m_abSendPackage);

    QGridLayout *WriteLayout = new QGridLayout;
    WriteLayout->addWidget(new QLabel("Write :", this), 0, 0);
    m_cbWriteMode->setFixedWidth(55);
    WriteLayout->addWidget(m_cbWriteMode, 0, 1);
    m_cbWriteScroll->setFixedWidth(65);
    WriteLayout->addWidget(m_cbWriteScroll, 0, 2);
    WriteLayout->addWidget(m_cbDisplayWrite, 0, 3);
    m_abSaveWriteLog->setFixedWidth(35);
    WriteLayout->addWidget(m_abSaveWriteLog, 1, 0);
    m_bSaveWriteLog->setFixedWidth(50);
    WriteLayout->addWidget(m_bSaveWriteLog, 1, 1);
    m_bWriteLogClear->setFixedWidth(50);
    WriteLayout->addWidget(m_bWriteLogClear, 1, 2);
    WriteLayout->addWidget(m_eLogWrite, 2, 0, 1, 6);
    WriteLayout->setSpacing(5);
    WriteLayout->setMargin(5);

    QGridLayout *ReadLayout = new QGridLayout;
    ReadLayout->addWidget(new QLabel("Read:", this), 0, 0);
    m_cbReadMode->setFixedWidth(55);
    ReadLayout->addWidget(m_cbReadMode, 0, 1);
    m_cbReadScroll->setFixedWidth(65);
    ReadLayout->addWidget(m_cbReadScroll, 0, 2);
    ReadLayout->addWidget(m_cbDisplayRead, 0, 3);
    m_abSaveReadLog->setFixedWidth(35);
    ReadLayout->addWidget(m_abSaveReadLog, 1, 0);
    m_bSaveReadLog->setFixedWidth(50);
    ReadLayout->addWidget(m_bSaveReadLog, 1, 1);
    m_bReadLogClear->setFixedWidth(50);
    ReadLayout->addWidget(m_bReadLogClear, 1, 2);
    ReadLayout->addWidget(m_eLogRead, 2, 0, 1, 6);
    ReadLayout->setSpacing(5);
    ReadLayout->setMargin(5);

    QWidget *wWrite = new QWidget;
    wWrite->setLayout(WriteLayout);
    QWidget *wRead = new QWidget;
    wRead->setLayout(ReadLayout);

    QSplitter *splitter = new QSplitter;
    splitter->addWidget(wWrite);
    splitter->addWidget(wRead);
    splitter->setHandleWidth(1);

    QGridLayout *dataLayout = new QGridLayout;
    dataLayout->addWidget(splitter, 0, 0);
    dataLayout->addLayout(sendPackageLayout, 1, 0);
    dataLayout->setSpacing(0);
    dataLayout->setMargin(0);

    QHBoxLayout *hiddenAllCheck = new QHBoxLayout;
    m_bDeleteAllMacroses->setFixedSize(15, 15);
    m_bDeleteAllMacroses->setStyleSheet("border-image: url(:/Resources/del.png) stretch;");
    hiddenAllCheck->addWidget(m_bDeleteAllMacroses);
    m_cbAllIntervals->setFixedWidth(58);
    hiddenAllCheck->addWidget(m_cbAllIntervals);
    m_cbAllPeriods->setFixedWidth(50);
    hiddenAllCheck->addWidget(m_cbAllPeriods);
    hiddenAllCheck->addWidget(m_sbAllDelay);
    hiddenAllCheck->addWidget(m_bAddMacros);
    hiddenAllCheck->addWidget(m_bLoadMacroses);
    m_abPause->setFixedWidth(38);
    hiddenAllCheck->addWidget(m_abPause);
    scrollAreaLayout->addLayout(hiddenAllCheck);

    scrollArea->setWidget(widgetScroll);
    scrollArea->show();
    scrollArea->setVisible(true);
    scrollArea->setVerticalScrollBar(new QScrollBar(Qt::Vertical, scrollArea));
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);
    scrollAreaLayout->addWidget(scrollArea);
    HiddenLayout->setSpacing(0);
    HiddenLayout->setMargin(2);
    m_gbHiddenGroup->setLayout(scrollAreaLayout);
    m_gbHiddenGroup->setFixedWidth(300);

    QGridLayout *allLayouts = new QGridLayout;
    allLayouts->setSpacing(5);
    allLayouts->setMargin(5);
    allLayouts->addLayout(configLayout, 0, 0);
    allLayouts->addLayout(dataLayout, 0, 1);
    m_bHiddenGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_bHiddenGroup->setFixedWidth(15);
    allLayouts->addWidget(m_bHiddenGroup, 0, 2);
    allLayouts->addWidget(m_gbHiddenGroup, 0, 3);
    widget = new QWidget(this);
    widget->setLayout(allLayouts);
    setCentralWidget(widget);
}

void MainWindow::connections()
{
    connect(m_bReadLogClear, SIGNAL(clicked()), m_eLogRead, SLOT(clear()));
    connect(m_bWriteLogClear, SIGNAL(clicked()), m_eLogWrite, SLOT(clear()));
    connect(m_bStart, SIGNAL(clicked()), this, SLOT(start()));
    connect(m_bStop, SIGNAL(clicked()), this, SLOT(stop()));
    connect(m_abPause, SIGNAL(toggled(bool)), this, SLOT(pause(bool)));
    connect(m_bSaveWriteLog, SIGNAL(clicked()), this, SLOT(saveWrite()));
    connect(m_bSaveReadLog, SIGNAL(clicked()), this, SLOT(saveRead()));
    connect(m_bHiddenGroup, SIGNAL(clicked()), this, SLOT(hiddenClick()));
    connect(m_abSaveWriteLog, SIGNAL(toggled(bool)), this, SLOT(startWriteLog(bool)));
    connect(m_abSaveReadLog, SIGNAL(toggled(bool)), this, SLOT(startReadLog(bool)));
    connect(m_bDeleteAllMacroses, SIGNAL(clicked()), this, SLOT(deleteAllMacroses()));
    connect(m_abSendPackage, SIGNAL(toggled(bool)), this, SLOT(startSending(bool)));
    connect(m_cbEchoMaster, SIGNAL(toggled(bool)), this, SLOT(echoCheckMaster(bool)));
    connect(m_cbEchoSlave, SIGNAL(toggled(bool)), this, SLOT(echoCheckSlave(bool)));
    connect(m_leSendPackage, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    connect(m_leSendPackage, SIGNAL(returnPressed()), m_abSendPackage, SLOT(click()));
    connect(m_bAddMacros, SIGNAL(clicked()), this, SLOT(addMacros()));
    connect(m_bLoadMacroses, SIGNAL(clicked()), this, SLOT(openDialog()));
    connect(m_cbAllIntervals, SIGNAL(toggled(bool)), this, SLOT(checkAllIntervals(bool)));
    connect(m_cbAllPeriods, SIGNAL(toggled(bool)), this, SLOT(checkAllPeriods(bool)));
    connect(m_sbAllDelay, SIGNAL(valueChanged(int)), this, SLOT(changeAllDelays(int)));
    connect(m_tIntervalSending, SIGNAL(timeout()), this, SLOT(sendInterval()));
    connect(m_tSend, SIGNAL(timeout()), this, SLOT(sendSingle()));
    connect(m_tEcho, SIGNAL(timeout()), this, SLOT(echo()));
    connect(m_tDelay, SIGNAL(timeout()), this, SLOT(breakLine()));
    connect(m_tWriteLog, SIGNAL(timeout()), this, SLOT(writeLogTimeout()));
    connect(m_tReadLog, SIGNAL(timeout()), this, SLOT(readLogTimeout()));
    connect(m_Port, SIGNAL(readyRead()), this, SLOT(received()));
    connect(m_cbUniformSizes, SIGNAL(toggled(bool)), this, SLOT(setUniformSizes(bool)));
}

void MainWindow::setUniformSizes(bool check)
{
    m_eLogRead->setUniformItemSizes(check);
    m_eLogWrite->setUniformItemSizes(check);
}

void MainWindow::changeAllDelays(int n)
{
    foreach (MiniMacros *m, MiniMacrosList) {
        m->time->setValue(n);
    }
}

void MainWindow::checkAllIntervals(bool check)
{
    m_cbAllPeriods->setChecked(false);
    m_cbAllIntervals->setChecked(check);
    foreach (MiniMacros *m, MiniMacrosList) {
        m->period->setChecked(false);
        m->interval->setChecked(check);
    }
}

void MainWindow::checkAllPeriods(bool check)
{
    m_cbAllIntervals->setChecked(false);
    m_cbAllPeriods->setChecked(check);
    foreach (MiniMacros *m, MiniMacrosList) {
        m->interval->setChecked(false);
        m->period->setChecked(check);
    }
}

void MainWindow::deleteAllMacroses()
{
    int button = QMessageBox::question(this, "Warning",
                                       "Delete ALL macroses?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (button == QMessageBox::Yes) {
        checkAllIntervals(false);
        checkAllPeriods(false);
        foreach (MiniMacros *m, MiniMacrosList) {
            delMacros(m->index);
        }
    }
}

int MainWindow::findIntervalItem(unsigned short int start)
{
    foreach (MiniMacros *m, MiniMacrosList)
    {
        if (m->interval->isChecked() && m->index >= start)
            return m->index;
    }
    start = 0;
    foreach (MiniMacros *m, MiniMacrosList)
    {
        if (m->interval->isChecked() && m->index >= start)
            return m->index;
    }
    return start;
}

void MainWindow::sendInterval()
{
    sendPackage(MiniMacrosList[sendIndex]->editing->package->text(),
                MiniMacrosList[sendIndex]->mode);
    sendIndex++;
    sendIndex = findIntervalItem(sendIndex);
    m_tIntervalSending->setInterval(MiniMacrosList[sendIndex]->time->value());
}

void MainWindow::hiddenClick()
{
    if (m_gbHiddenGroup->isHidden())
    {
        m_gbHiddenGroup->show();
        m_bHiddenGroup->setText("<");
        if (!isMaximized())
            resize(width() + m_gbHiddenGroup->width() + 5, height());
        setMinimumWidth(665 + m_gbHiddenGroup->width() + 5);
    }
    else
    {
        setMinimumWidth(665);
        m_gbHiddenGroup->hide();
        m_bHiddenGroup->setText(">");
        if (!isMaximized())
            resize(width() - m_gbHiddenGroup->width() - 5, height());
    }
}

void MainWindow::openDialog()
{
    QStringList fileNames;
    if (fileDialog->exec())
        fileNames = fileDialog->selectedFiles();

    foreach (QString s, fileNames) {
        addMacros();
        MiniMacrosList.last()->editing->openPath(s);
    }
}

void MainWindow::addMacros()
{
    HiddenLayout->removeItem(spacer);
    MiniMacrosList.insert(index, new MiniMacros(index, this));
    HiddenLayout->addWidget(MiniMacrosList[index]);
    HiddenLayout->addSpacerItem(spacer);

    connect(MiniMacrosList[index], SIGNAL(deleteSignal(int)), this, SLOT(delMacros(int)));
    connect(MiniMacrosList[index], SIGNAL(setSend(QString, int)), this, SLOT(sendPackage(QString, int)));
    connect(MiniMacrosList[index], SIGNAL(setIntervalSend(int, bool)), this, SLOT(intervalSendAdded(int, bool)));
    connect(MiniMacrosList[index], SIGNAL(moveUp(int)), this, SLOT(moveMacUp(int)));
    connect(MiniMacrosList[index], SIGNAL(moveDown(int)),this, SLOT(moveMacDown(int)));
    index++;
}

bool MainWindow::moveMacros(QWidget *widget, MoveDirection direction)
{
  QVBoxLayout* myLayout = qobject_cast<QVBoxLayout*>(widget->parentWidget()->layout());
  const int index = myLayout->indexOf(widget);

  if (direction == MoveUp && index == 0) {
    return false;
  }

  if (direction == MoveDown && index == myLayout->count()-1 ) {
    return false;
  }

  const int newIndex = direction == MoveUp ? index - 1 : index + 1;
  HiddenLayout->removeItem(spacer);
  myLayout->removeWidget(widget);
  myLayout->insertWidget(newIndex , widget);
  HiddenLayout->addSpacerItem(spacer);

  return true;
}

void MainWindow::moveMacUp(int index)
{
   if (moveMacros(MiniMacrosList[index], MoveUp))
   {
       for (int i = index - 1; i >= MiniMacrosList.first()->index; i--)
           if (MiniMacrosList.contains(i))
           {
               MiniMacrosList[index]->index = i;
               MiniMacrosList[i]->index = index;
               MiniMacros *temp = MiniMacrosList[index];
               MiniMacrosList[index] = MiniMacrosList[i];
               MiniMacrosList[i] = temp;
               break;
           }
   }
}

void MainWindow::moveMacDown(int index)
{
   if (moveMacros(MiniMacrosList[index], MoveDown))
       for (int i = index + 1; i <= MiniMacrosList.last()->index; i++)
           if (MiniMacrosList.contains(i))
           {
               MiniMacrosList[index]->index = i;
               MiniMacrosList[i]->index = index;
               MiniMacros *temp = MiniMacrosList[index];
               MiniMacrosList[index] = MiniMacrosList[i];
               MiniMacrosList[i] = temp;
               break;
           }
}

void MainWindow::intervalSendAdded(int index, bool check)
{
    if (check)
    {
        sendCount++;
        if (sendCount == 1)
        {
            sendIndex = index;
            m_tIntervalSending->setInterval(MiniMacrosList[index]->time->value());
            if (m_Port->isOpen())
                m_tIntervalSending->start();
        }
    }
    else
    {
        sendCount--;
        if (sendCount == 0)
            m_tIntervalSending->stop();
    }
}

void MainWindow::delMacros(int index)
{
    MiniMacrosList[index]->interval->setChecked(false);
    MiniMacrosList[index]->period->setChecked(false);
    delete MiniMacrosList.take(index);
}

void MainWindow::writeLogTimeout()
{
    writeLog.close();
    m_abSaveWriteLog->setChecked(false);
    logWrite = false;
    m_tWriteLog->stop();
}

void MainWindow::readLogTimeout()
{
    readLog.close();
    m_abSaveReadLog->setChecked(false);
    logRead = false;
    m_tReadLog->stop();
}

void MainWindow::startWriteLog(bool check)
{
    if (check)
    {
        QString path = fileDialog->getSaveFileName(this,
                                                   tr("Save File"),
                                                   QDir::currentPath() + "/(WRITE)" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + ".txt",
                                                   tr("Log Files (*.txt)"));
        if (path.isEmpty())
        {
            m_abSaveWriteLog->setChecked(false);
            return;
        }
        writeLog.setFileName(path);
        writeLog.open(QIODevice::WriteOnly);
        m_tWriteLog->start();
        logWrite = true;
        m_abSaveWriteLog->setIcon(QIcon(":/Resources/startRecToFileBlink.png"));
    } else
    {
        m_tWriteLog->stop();
        writeLog.close();
        logWrite = false;
        m_abSaveWriteLog->setIcon(QIcon(":/Resources/startRecToFile.png"));
    }
}

void MainWindow::startReadLog(bool check)
{
    if (check)
    {
        QString path = fileDialog->getSaveFileName(this,
                                                   tr("Save File"),
                                                   QDir::currentPath() + "/(READ)" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + ".txt",
                                                   tr("Log Files (*.txt)"));
        if (path.isEmpty())
        {
            m_abSaveReadLog->setChecked(false);
            return;
        }
        readLog.setFileName(path);
        readLog.open(QIODevice::WriteOnly);
        m_tReadLog->start();
        logRead = true;
        m_abSaveReadLog->setIcon(QIcon(":/Resources/startRecToFileBlink.png"));
    } else
    {
        m_tReadLog->stop();
        readLog.close();
        logRead = false;
        m_abSaveReadLog->setIcon(QIcon(":/Resources/startRecToFile.png"));
    }
}

void MainWindow::saveWrite()
{
    QString fileName = fileDialog->getSaveFileName(this,
                                                   tr("Save File"),
                                                   QDir::currentPath() + "/(WRITE)" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + ".txt",
                                                   tr("Log Files (*.txt)"));

        if (fileName != "") {
            QFile file(fileName);
            if (!file.open(QIODevice::WriteOnly)) {
                QMessageBox::critical(this, tr("Error"), tr("Could not save file"));
                return;
            } else {
                QTextStream stream(&file);
                for(int i = 0; i < m_eLogWrite->count(); ++i)
                {
                    stream << m_eLogWrite->item(i)->text() + "\n";
                }
                file.close();
            }
        }
}

void MainWindow::saveRead()
{
    QString fileName = fileDialog->getSaveFileName(this,
                                                   tr("Save File"),
                                                   QDir::currentPath() + "/(READ)" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + ".txt",
                                                   tr("Log Files (*.txt)"));

        if (fileName != "") {
            QFile file(fileName);
            if (!file.open(QIODevice::WriteOnly)) {
                QMessageBox::critical(this, tr("Error"), tr("Could not save file"));
                return;
            } else {
                QTextStream stream(&file);
                for(int i = 0; i < m_eLogRead->count(); ++i)
                {
                    stream << m_eLogRead->item(i)->text() + "\n";
                }
                file.close();
            }
        }
}

void MainWindow::textChanged(QString text)
{
    if (!text.isEmpty() && m_bStop->isEnabled())
    {
        m_abSendPackage->setEnabled(true);
        m_abSendPackage->setCheckable(true);
    }
    else
    {
        m_abSendPackage->setEnabled(false);
        m_abSendPackage->setCheckable(false);
    }
}

void MainWindow::echoCheckMaster(bool check)
{
    m_cbEchoSlave->setChecked(false);
    m_cbEchoMaster->setChecked(check);
    if (check)
    {
        m_abSendPackage->setChecked(false);
        foreach (MiniMacros *m, MiniMacrosList.values()) {
            m->interval->setChecked(false);
            m->period->setChecked(false);
            m->interval->setEnabled(false);
            m->period->setEnabled(false);
        }
    }
    else
    {
        foreach (MiniMacros *m, MiniMacrosList.values()) {
            m->interval->setEnabled(true);
            m->period->setEnabled(true);
        }
    }
}

void MainWindow::echoCheckSlave(bool check)
{
    m_cbEchoMaster->setChecked(false);
    m_cbEchoSlave->setChecked(check);
}

void MainWindow::start()
{
    m_Port->close();
    m_Port->setPortName(m_cbPort->currentText());

    if(m_Port->open(QSerialPort::ReadWrite))
    {
        switch (m_cbBaud->currentIndex()) {
        case 0:
            m_Port->setBaudRate(QSerialPort::Baud921600);
            break;
        case 1:
            m_Port->setBaudRate(QSerialPort::Baud115200);
            break;
        case 2:
            m_Port->setBaudRate(QSerialPort::Baud57600);
            break;
        case 3:
            m_Port->setBaudRate(QSerialPort::Baud38400);
            break;
        case 4:
            m_Port->setBaudRate(QSerialPort::Baud19200);
            break;
        case 5:
            m_Port->setBaudRate(QSerialPort::Baud9600);
            break;
        case 6:
            m_Port->setBaudRate(QSerialPort::Baud4800);
            break;
        case 7:
            m_Port->setBaudRate(QSerialPort::Baud2400);
            break;
        case 8:
            m_Port->setBaudRate(QSerialPort::Baud1200);
            break;
        }

        switch (m_cbBits->currentIndex()) {
        case 0:
            m_Port->setDataBits(QSerialPort::Data5);
            break;
        case 1:
            m_Port->setDataBits(QSerialPort::Data6);
            break;
        case 2:
            m_Port->setDataBits(QSerialPort::Data7);
            break;
        case 3:
            m_Port->setDataBits(QSerialPort::Data8);
            break;
        }

        switch (m_cbParity->currentIndex()) {
        case 0:
            m_Port->setParity(QSerialPort::NoParity);
            break;
        case 1:
            m_Port->setParity(QSerialPort::OddParity);
            break;
        case 2:
            m_Port->setParity(QSerialPort::EvenParity);
            break;
        case 3:
            m_Port->setParity(QSerialPort::MarkParity);
            break;
        case 4:
            m_Port->setParity(QSerialPort::SpaceParity);
            break;
        }

        switch (m_cbStopBits->currentIndex()) {
        case 0:
            m_Port->setStopBits(QSerialPort::OneStop);
            break;
        case 1:
            m_Port->setStopBits(QSerialPort::OneAndHalfStop);
            break;
        case 2:
            m_Port->setStopBits(QSerialPort::TwoStop);
            break;
        }


        m_Port->setDataBits(QSerialPort::Data8);
        m_Port->setParity(QSerialPort::NoParity);
        m_Port->setFlowControl(QSerialPort::NoFlowControl);

        m_bStart->setEnabled(false);
        m_bStop->setEnabled(true);
        m_abPause->setEnabled(true);
        m_cbPort->setEnabled(false);
        m_cbBaud->setEnabled(false);
        m_cbBits->setEnabled(false);
        m_cbParity->setEnabled(false);
        m_cbStopBits->setEnabled(false);
        m_lTx->setStyleSheet("background: none; font: bold; font-size: 10pt");
        m_lRx->setStyleSheet("background: none; font: bold; font-size: 10pt");
        if (!m_leSendPackage->text().isEmpty())
            m_abSendPackage->setEnabled(true);
        if (sendCount)
        {
            sendIndex = MiniMacrosList.first()->index;
            m_tIntervalSending->start();
        }
    }
    else
    {
        m_lTx->setStyleSheet("background: yellow; font: bold; font-size: 10pt");
        m_lRx->setStyleSheet("background: yellow; font: bold; font-size: 10pt");
    }
}

void MainWindow::stop()
{
    m_Port->close();
    m_lTx->setStyleSheet("background: none; font: bold; font-size: 10pt");
    m_lRx->setStyleSheet("background: none; font: bold; font-size: 10pt");
    m_bStop->setEnabled(false);
    m_abPause->setEnabled(false);
    m_bStart->setEnabled(true);
    m_cbPort->setEnabled(true);
    m_cbBaud->setEnabled(true);
    m_cbBits->setEnabled(true);
    m_cbParity->setEnabled(true);
    m_cbStopBits->setEnabled(true);
    m_abSendPackage->setEnabled(false);
    m_abSendPackage->setChecked(false);
    m_tSend->stop();
    m_tEcho->stop();
    m_tDelay->stop();
    m_tIntervalSending->stop();
    txCount = 0;
    m_lTxCount->setText("Tx: 0");
    rxCount = 0;
    m_lRxCount->setText("Rx: 0");
}

void MainWindow::pause(bool check)
{
    if (check)
    {
        m_tIntervalSending->stop();

        foreach (MiniMacros *m, MiniMacrosList) {
            m->interval->setEnabled(false);
            m->period->setEnabled(false);
        }
        m_cbAllIntervals->setEnabled(false);
        m_cbAllPeriods->setEnabled(false);
    }
    else
    {
        foreach (MiniMacros *m, MiniMacrosList) {
            m->interval->setEnabled(true);
            m->period->setEnabled(true);
        }
        m_cbAllIntervals->setEnabled(true);
        m_cbAllPeriods->setEnabled(true);

        if (sendCount != 0)
            m_tIntervalSending->start();
    }
}

void MainWindow::received()
{
    m_tDelay->start(m_sbDelay->value());
    readBuffer += m_Port->readAll();
    rxCount++;
    m_lRxCount->setText("Rx: " + QString::number(rxCount));
}

void MainWindow::sendSingle()
{
    sendPackage(m_leSendPackage->text(), m_cbSendMode->currentIndex());
}

void MainWindow::echo()
{
    if (m_cbEchoMaster->isChecked())
        sendPackage(echoBuffer.join(" "), 2);
    if (m_cbEchoSlave->isChecked())
    {
        sendPackage(echoSlave.takeFirst(), 2);
        m_tEcho->setInterval(m_sbEchoInterval->value());
        if (echoSlave.isEmpty())
            m_tEcho->stop();
    }
}

void MainWindow::startSending(bool checked)
{
    if (checked)
        {
            if (m_Port->isOpen())
            {
                if (m_sbRepeatSendInterval->value() != 0 && !m_cbEchoMaster->isChecked())
                {
                    m_tSend->setInterval(m_sbRepeatSendInterval->value());
                    m_tSend->start();

                } else
                {
                    sendPackage(m_leSendPackage->text(), m_cbSendMode->currentIndex());
                    m_abSendPackage->setChecked(false);
                }
            }
        } else
        {
            m_tSend->stop();
        }
}

void MainWindow::sendPackage(QString string, int mode)
{
    if (m_Port->isOpen() && m_Port->openMode() != QSerialPort::ReadOnly)
    {
        if (!string.isEmpty())
        {
            m_tSend->setInterval(m_sbRepeatSendInterval->value());

            if (!m_tTx->isSingleShot())
            {
                m_lTx->setStyleSheet("background: green; font: bold; font-size: 10pt");
                m_tTx->singleShot(BLINKTIMETX, this, SLOT(txNone()));
                m_tTx->setSingleShot(true);
            }

            QStringList out;
            switch (mode) {
                case 0:
                {
                    QStringList byteList = string.split(" ", QString::SkipEmptyParts);
                    QString hex = string.remove(" ", Qt::CaseSensitive);
                    QByteArray writeArray;
                    writeArray.append(hex);
                    m_Port->write(QByteArray::fromHex(writeArray));
                    unsigned short int count = byteList.length();
                    for (int i = 0; i < count; i++)
                    {
                        bool ok;
                        int n = byteList[i].toInt(&ok, 16);
                        out.append(QString::number(n));
                    }
                    break;
                }
                case 1:
                {
                    m_Port->write(string.toLocal8Bit());
                    unsigned short int count = string.length();
                    for (int i = 0; i < count; i++)
                    {
                        int ascii = string[i].toLatin1();
                        out.append(QString::number(ascii, 10));
                    }
                    break;
                }
                case 2:
                {
                    QStringList byteList = string.split(" ", QString::SkipEmptyParts);
                    QByteArray writeArray;
                    unsigned short int count = byteList.length();
                    for (int i = 0; i < count; i++)
                    {
                        bool ok;
                        int n = byteList[i].toInt(&ok, 10);
                        writeArray.append(QChar(n));
                        out.append(QString::number(n));
                    }
                    m_Port->write(writeArray);
                    break;
                }

            }
            displayWriteData(out);
            if (m_cbEchoMaster->isChecked())
            {
                echoBuffer = out;
                echoWaiting = true;
            }
            txCount += out.count();
            m_lTxCount->setText("Tx: " + QString::number(txCount));
       }
    }
}

void MainWindow::displayWriteData(QStringList list)
{
    if (!m_cbDisplayWrite->isChecked())
        return;

    QTextStream writeStream (&writeLog);
    QString out;
    unsigned short int count = list.length();
    for (int i = 0; i < count; i++)
    {
        int num = list[i].toInt();
        switch (m_cbWriteMode->currentIndex())
        {
        case 0:
        {
            QString hex = QString::number(num, 16).toUpper();
            if (hex.length() < 2)
                hex.insert(0, "0");
            out.append(hex + " ");
            break;
        }
        case 1:
        {
            QChar ch(num);
            out.append(ch);
            break;
        }
        case 2:
        {
            out.append(list[i] + " ");
        }
        }
    }

    m_eLogWrite->addPackage(out);
    if (logWrite)
        writeStream << out + "\n";

    if (m_cbWriteScroll->isChecked())
        m_eLogWrite->scrollToBottom();
}

void MainWindow::breakLine()
{
    m_tDelay->stop();

    if (!m_tRx->isSingleShot())
    {
        m_lRx->setStyleSheet("background: red; font: bold; font-size: 10pt");
        m_tRx->singleShot(BLINKTIMERX, this, SLOT(rxNone()));
        m_tRx->setSingleShot(true);
    }

    QTextStream readStream(&readLog);
    QString in = QString(readBuffer.toHex()).toUpper();
    for (int i = 2; !(i >= in.length()); i += 3)
        in.insert(i, " ");
    QStringList list = in.split(" ", QString::SkipEmptyParts);
    QString outDEC;
    unsigned short int count = list.length();
    for (int i = 0; i < count; i++)
    {
        bool ok;
        int dec = list[i].toInt(&ok, 16);
        if (ok)
            outDEC.append(QString::number(dec) + " ");
    }
    if (m_cbDisplayRead->isChecked())
    {
        switch (m_cbReadMode->currentIndex())
        {
        case 0:
        {
            m_eLogRead->addPackage(in);
            if (logRead)
                readStream << in + "\n";
            break;
        }
        case 1:
        {
            m_eLogRead->addPackage(QString(readBuffer));
            if (logRead)
                readStream << QString(readBuffer) + "\n";
            break;
        }
        case 2:
        {
            m_eLogRead->addPackage(outDEC);
            if (logRead)
                readStream << outDEC + "\n";
            break;
        }
        }
    }
    if (m_cbEchoMaster->isChecked() && echoWaiting)
    {
        if (QString::compare(echoBuffer.join(" "), outDEC.remove(outDEC.length() - 1, 1), Qt::CaseInsensitive) == 0)
        {
            m_eLogWrite->setItemColor(m_eLogWrite->count() - 1, Qt::green);
            m_eLogRead->setItemColor(m_eLogWrite->count() - 1, Qt::green);
        }
        else
        {
            m_eLogWrite->setItemColor(m_eLogWrite->count() - 1, Qt::red);
            m_eLogRead->setItemColor(m_eLogWrite->count() - 1, Qt::red);
        }
        if (m_sbEchoInterval->value() != 0)
            m_tEcho->singleShot(m_sbEchoInterval->value(), this, SLOT(echo()));
        echoWaiting = false;
    }
    if (m_cbEchoSlave->isChecked())
    {
        echoSlave.append(outDEC);
        if (!m_tEcho->isActive())
        {
            m_tEcho->setInterval(m_sbEchoInterval->value());
            m_tEcho->start();
        }
    }
    readBuffer.clear();

    if (m_cbReadScroll->isChecked())
        m_eLogRead->scrollToBottom();
}

void MainWindow::rxNone()
{
    m_lRx->setStyleSheet("background: none; font: bold; font-size: 10pt");
    m_tRx->singleShot(BLINKTIMERX, this, SLOT(rxHold()));
}

void MainWindow::txNone()
{
    m_lTx->setStyleSheet("background: none; font: bold; font-size: 10pt");
    m_tTx->singleShot(BLINKTIMETX, this, SLOT(txHold()));
}

void MainWindow::rxHold()
{
    m_tRx->setSingleShot(false);
}

void MainWindow::txHold()
{
    m_tTx->setSingleShot(false);
}

void MainWindow::saveSession()
{

    settings->setValue("config/height", height());
    settings->setValue("config/width", width());
    settings->setValue("config/position", pos());
    settings->setValue("config/isMaximized", isMaximized());

    settings->setValue("config/max_write_log_rows", m_eLogWrite->getMaxCount());
    settings->setValue("config/max_read_log_rows", m_eLogRead->getMaxCount());
    settings->setValue("config/port", m_cbPort->currentText());
    settings->setValue("config/baud", m_cbBaud->currentIndex());
    settings->setValue("config/data_bits", m_cbBits->currentIndex());
    settings->setValue("config/parity", m_cbParity->currentIndex());
    settings->setValue("config/stop_bits", m_cbStopBits->currentIndex());
    settings->setValue("config/echo_interval", m_sbEchoInterval->value());
    settings->setValue("config/single_send_interval", m_sbRepeatSendInterval->value());
    settings->setValue("config/write_autoscroll", m_cbWriteScroll->isChecked());
    settings->setValue("config/read_autoscroll", m_cbReadScroll->isChecked());
    settings->setValue("config/write_log_timeout", m_tWriteLog->interval());
    settings->setValue("config/read_log_timeout", m_tReadLog->interval());
    settings->setValue("config/hidden_group_isHidden", m_gbHiddenGroup->isHidden());
    settings->setValue("config/mode", m_cbSendMode->currentIndex());
    settings->setValue("config/write_display", m_cbDisplayWrite->isChecked());
    settings->setValue("config/read_display", m_cbDisplayRead->isChecked());

    settings->remove("macros");
    int i = 1;
    foreach (MiniMacros *m, MiniMacrosList.values()) {
        if (!m->editing->isFromFile)
        {
            QString mode;
            if (m->editing->rbHEX->isChecked())
                mode = "HEX";
            if (m->editing->rbDEC->isChecked())
                mode = "DEC";
            if (m->editing->rbASCII->isChecked())
                mode = "ASCII";
            settings->setValue("macros/"+QString::number(i)+"/mode", mode);
            settings->setValue("macros/"+QString::number(i)+"/packege", m->editing->package->text());
            settings->setValue("macros/"+QString::number(i)+"/interval", m->time->value());
        }
        settings->setValue("macros/"+QString::number(i)+"/checked_interval", m->interval->isChecked());
        settings->setValue("macros/"+QString::number(i)+"/checked_period", m->period->isChecked());
        settings->setValue("macros/"+QString::number(i)+"/path", m->editing->path);
        i++;
    }
    settings->setValue("macros/size", i-1);
}

void MainWindow::loadSession()
{
    const QPoint pos = settings->value ("config/position").toPoint();
        if (!pos.isNull())
            move (pos);
    if (settings->value("config/isMaximized").toBool())
        showMaximized();

    m_eLogRead->setMaxCount(settings->value("config/max_write_log_rows", 1000).toInt());
    m_eLogWrite->setMaxCount(settings->value("config/max_read_log_rows", 1000).toInt());
    m_cbPort->setCurrentText(settings->value("config/port").toString());
    m_cbBaud->setCurrentIndex(settings->value("config/baud").toInt());
    m_cbBits->setCurrentIndex(settings->value("config/data_bits").toInt());
    m_cbParity->setCurrentIndex(settings->value("config/parity").toInt());
    m_cbStopBits->setCurrentIndex(settings->value("config/stop_bits").toInt());
    m_sbEchoInterval->setValue(settings->value("config/echo_interval").toInt());
    m_sbRepeatSendInterval->setValue(settings->value("config/single_send_interval").toInt());
    m_cbWriteScroll->setChecked(settings->value("config/write_autoscroll", true).toBool());
    m_cbReadScroll->setChecked(settings->value("config/read_autoscroll", true).toBool());
    m_tWriteLog->setInterval(settings->value("config/write_log_timeout", 600000).toInt());
    m_tReadLog->setInterval(settings->value("config/read_log_timeout", 600000).toInt());
    m_gbHiddenGroup->setHidden(settings->value("config/hidden_group_isHidden", true).toBool());
    if (!m_gbHiddenGroup->isHidden())
    {
        m_bHiddenGroup->setText("<");
        setMinimumWidth(665 + m_gbHiddenGroup->width() + 5);
    }
    m_cbSendMode->setCurrentIndex(settings->value("config/mode", 0).toInt());
    m_cbDisplayWrite->setChecked(settings->value("config/write_display", true).toBool());
    m_cbDisplayRead->setChecked(settings->value("config/read_display", true).toBool());

    unsigned short int size = settings->value("macros/size", 0).toInt();
    if (!size)
    {
        addMacros();
        return;
    }
    for (int i = 1; i <= size; ++i) {
        addMacros();
        if (!MiniMacrosList.last()->editing->openPath(settings->value("macros/"+QString::number(i)+"/path").toString()))
        {
            QString mode = settings->value("macros/"+QString::number(i)+"/mode").toString();
            if (mode == "HEX")
                MiniMacrosList.last()->editing->rbHEX->setChecked(true);
            if (mode == "DEC")
                MiniMacrosList.last()->editing->rbDEC->setChecked(true);
            if (mode == "ASCII")
                MiniMacrosList.last()->editing->rbASCII->setChecked(true);
            MiniMacrosList.last()->editing->package->setText(settings->value("macros/"+QString::number(i)+"/packege").toString());
            MiniMacrosList.last()->time->setValue(settings->value("macros/"+QString::number(i)+"/interval").toInt());
        }
         MiniMacrosList.last()->interval->setChecked(settings->value("macros/"+QString::number(i)+"/checked_interval").toBool());
         MiniMacrosList.last()->period->setChecked(settings->value("macros/"+QString::number(i)+"/checked_period").toBool());
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    saveSession();
    foreach (MiniMacros *m, MiniMacrosList.values()) {
        m->editing->close();
    }
    e->accept();
}
