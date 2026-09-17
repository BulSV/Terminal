#include "HexEncoder.h"

void HexEncoder::setData(const QString &data, const QString &sep)
{
    DataEncoder::setData(data, sep);

    QStringList byteList = data.split(sep, Qt::SkipEmptyParts);
    int count = byteList.size();
    for(int i = 0; i < count; i++) {
        bool ok;
        int n = byteList.at(i).toInt(&ok, 16);
        byteData.append(static_cast<char>(n));
        stringListData.append(QString::number(n, 16));
    }
}

void HexEncoder::setData(const QByteArray &data)
{
    DataEncoder::setData(data);
    int currentByte = 0;
    QString currentByteString;
    int byteDataSize = byteData.size();
    for(int i = 0; i < byteDataSize; ++i) {
        currentByte = static_cast<unsigned char>(byteData.at(i));
        if(currentByte < 0x10) {
            currentByteString.append("0");
        }
        currentByteString.append(QString::number(currentByte, 16).toUpper());
        stringListData.append(currentByteString);
        currentByteString.clear();
    }
}
