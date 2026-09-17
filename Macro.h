#ifndef MACRO_H
#define MACRO_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QTimer>
#include <QSettings>
#include <QByteArray>
#include <QSerialPort>

#include "MacroEdit.h"
#include "ClickableLabel.h"
#include "PacketTimeCalculator.h"

class Macro : public QWidget
{
    Q_OBJECT
public:
    explicit Macro(QWidget *parent = 0);

    void saveSettings(QSettings *settings, int macroIndex);
    void loadSettings(QSettings *settings, int macroIndex);

    void setPacketTimeCalculator(PacketTimeCalculator *packetTimeCalculator);

    void select();
    void deselect();
    bool selectState() const;

    void enableSelectState(bool enable);
    bool isEnabledSelectState() const;

    void setTime(int time);
    int getTime() const;

    const QByteArray &getPacket() const;
    void sendPacket();

    void openMacroFile(const QString &fileName);
signals:
    void deleted();
    void selected(bool select);
    void packetSended(const QByteArray &packet);
    void timeChanged(int time);
    void movedUp();
    void movedDown();
private:
    ClickableLabel *buttonDelete;
    QCheckBox *checkBoxSelect;
    QSpinBox *spinBoxTime;
    ClickableLabel *buttonSend;
    ClickableLabel *buttonUp;
    ClickableLabel *buttonDown;
    MacroEdit *macroEdit;
    PacketTimeCalculator *packetTimeCalculator;

    void view();
    void connections();
    void deleteMacro();
    void titleChanged();
    void selectTrigger();
    void onPacketChanged();
};

#endif // MACRO_H
