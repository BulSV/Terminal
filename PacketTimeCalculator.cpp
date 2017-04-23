#include "PacketTimeCalculator.h"

PacketTimeCalculator::PacketTimeCalculator(const QSerialPort *port)
{
    this->port = port;
}

bool PacketTimeCalculator::isValid() const
{
    if(port == 0 || !port->isOpen()) {
        return false;
    }

    return true;
}

double PacketTimeCalculator::calculateTime(int bytesCount)
{
    double startBit = 1;
    double dataBits = port->dataBits();
    double parityBit = port->parity() == QSerialPort::NoParity ? 0 : 1;
    double stopBits = port->stopBits() == QSerialPort::OneAndHalfStop ? 1.5 : static_cast<double>(port->stopBits());
    double speed = port->baudRate();
    double packetTime = (startBit + dataBits + parityBit + stopBits) * bytesCount / speed;

    return packetTime * 1000;
}
