#include "DecEncoder.h"

void DecEncoder::setData(const QString &data, const QString &sep)
{
    DataEncoder::setData(data, sep);

    QStringList byteList = data.split(sep, Qt::SkipEmptyParts);
    int count = byteList.size();
    for(int i = 0; i < count; i++) {
        bool ok;
        int n = byteList.at(i).toInt(&ok, 10);
        byteData.append(static_cast<char>(n));
        stringListData.append(QString::number(n));
    }
}

void DecEncoder::setData(const QByteArray &data)
{
    DataEncoder::setData(data);

    for(int i = 0; i < byteData.size(); ++i) {
        stringListData.append(QString::number((unsigned char)byteData.at(i)));
    }
}
