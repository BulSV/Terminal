#include <QGridLayout>
#include <QLabel>
#include <QStringList>
#include <QSerialPortInfo>

#include "ComPortConfigure.h"

ComPortConfigure::ComPortConfigure(QSerialPort *port, QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint)
    , port(port)
    , m_cbPort(new QComboBox(this))
    , m_cbBaud(new QComboBox(this))
    , m_cbBits(new QComboBox(this))
    , m_cbParity(new QComboBox(this))
    , m_cbStopBits(new QComboBox(this))
{
    QStringList buffer;
    foreach(QSerialPortInfo availablePort, QSerialPortInfo::availablePorts()) {
        buffer << availablePort.portName();
    }
    m_cbPort->addItems(buffer);
    m_cbPort->setEditable(true);

    buffer.clear();
    buffer << "1200" << "2400" << "4800" << "9600" << "19200" << "38400"
           << "57600" << "115200" << "230400" << "460800" << "921600";
    m_cbBaud->addItems(buffer);
    buffer.clear();
    buffer << "5" << "6" << "7" << "8";
    m_cbBits->addItems(buffer);
    buffer.clear();
    buffer << "No" << "Odd" << "Even" << "Mark" << "Space";
    m_cbParity->addItems(buffer);
    buffer.clear();
    buffer << "1" << "1.5" << "2";
    m_cbStopBits->addItems(buffer);

    portNameSetting();
    portBaudSetting();
    portDataBitsSetting();
    portParitySetting();
    portStopBitsSetting();

    view();
    connections();
}

void ComPortConfigure::saveSettings(QSettings *settings)
{
    settings->remove("port");
    settings->setValue("port/port", m_cbPort->currentText());
    settings->setValue("port/baud_rate", m_cbBaud->currentIndex());
    settings->setValue("port/data_bits", m_cbBits->currentIndex());
    settings->setValue("port/parity", m_cbParity->currentIndex());
    settings->setValue("port/stop_bits", m_cbStopBits->currentIndex());
}

void ComPortConfigure::loadSettings(QSettings *settings)
{
    m_cbPort->setCurrentText(settings->value("port/port").toString());
    m_cbBaud->setCurrentIndex(settings->value("port/baud_rate").toInt());
    m_cbBits->setCurrentIndex(settings->value("port/data_bits").toInt());
    m_cbParity->setCurrentIndex(settings->value("port/parity").toInt());
    m_cbStopBits->setCurrentIndex(settings->value("port/stop_bits").toInt());

    portNameSetting();
    portBaudSetting();
    portDataBitsSetting();
    portParitySetting();
}

void ComPortConfigure::view()
{
    QGridLayout *configLayout = new QGridLayout;
    configLayout->addWidget(new QLabel(tr("Port:"), this), 0, 0);
    configLayout->addWidget(m_cbPort, 0, 1);
    configLayout->addWidget(new QLabel(tr("Baud rate:"), this), 1, 0);
    configLayout->addWidget(m_cbBaud, 1, 1);
    configLayout->addWidget(new QLabel(tr("Data bits:"), this), 2, 0);
    configLayout->addWidget(m_cbBits, 2, 1);
    configLayout->addWidget(new QLabel(tr("Parity:"), this), 3, 0);
    configLayout->addWidget(m_cbParity, 3, 1);
    configLayout->addWidget(new QLabel(tr("Stop bits:"), this), 4, 0);
    configLayout->addWidget(m_cbStopBits, 4, 1);
    configLayout->setSpacing(5);
    setLayout(configLayout);
}

void ComPortConfigure::connections()
{
    connect(m_cbPort, &QComboBox::currentTextChanged, this, &ComPortConfigure::portNameSetting);
    connect(m_cbBaud, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ComPortConfigure::portBaudSetting);
    connect(m_cbBits, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ComPortConfigure::portDataBitsSetting);
    connect(m_cbParity, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ComPortConfigure::portParitySetting);
    connect(m_cbStopBits, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ComPortConfigure::portStopBitsSetting);
}

void ComPortConfigure::portNameSetting()
{
    port->setPortName(m_cbPort->currentText());
}

void ComPortConfigure::portBaudSetting()
{
    switch(m_cbBaud->currentIndex()) {
    case 0:
        port->setBaudRate(QSerialPort::Baud1200);
        break;
    case 1:
        port->setBaudRate(QSerialPort::Baud2400);
        break;
    case 2:
        port->setBaudRate(QSerialPort::Baud4800);
        break;
    case 3:
        port->setBaudRate(QSerialPort::Baud9600);
        break;
    case 4:
        port->setBaudRate(QSerialPort::Baud19200);
        break;
    case 5:
        port->setBaudRate(QSerialPort::Baud38400);
        break;
    case 6:
        port->setBaudRate(QSerialPort::Baud57600);
        break;
    case 7:
        port->setBaudRate(QSerialPort::Baud115200);
        break;
    case 8:
        port->setBaudRate(230400);
        break;
    case 9:
        port->setBaudRate(460800);
        break;
    case 10:
        port->setBaudRate(921600);
        break;
    }
}

void ComPortConfigure::portDataBitsSetting()
{
    switch(m_cbBits->currentIndex()) {
    case 0:
        port->setDataBits(QSerialPort::Data5);
        break;
    case 1:
        port->setDataBits(QSerialPort::Data6);
        break;
    case 2:
        port->setDataBits(QSerialPort::Data7);
        break;
    case 3:
        port->setDataBits(QSerialPort::Data8);
        break;
    }
}

void ComPortConfigure::portParitySetting()
{
    switch(m_cbParity->currentIndex()) {
    case 0:
        port->setParity(QSerialPort::NoParity);
        break;
    case 1:
        port->setParity(QSerialPort::OddParity);
        break;
    case 2:
        port->setParity(QSerialPort::EvenParity);
        break;
    case 3:
        port->setParity(QSerialPort::MarkParity);
        break;
    case 4:
        port->setParity(QSerialPort::SpaceParity);
        break;
    }
}

void ComPortConfigure::portStopBitsSetting()
{
    switch(m_cbStopBits->currentIndex()) {
    case 0:
        port->setStopBits(QSerialPort::OneStop);
        break;
    case 1:
        port->setStopBits(QSerialPort::OneAndHalfStop);
        break;
    case 2:
        port->setStopBits(QSerialPort::TwoStop);
        break;
    }
}

