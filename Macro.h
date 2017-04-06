#ifndef MACRO_WIDGET_H
#define MACRO_WIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QTimer>
#include <QSettings>
#include <QByteArray>

#include "RightClickedButton.h"
#include "MacroEdit.h"
#include "ClickableLabel.h"

class Macro : public QWidget
{
    Q_OBJECT
public:
    explicit Macro(QWidget *parent = 0);

    void saveSettings(QSettings *settings, int macroIndex);
    void loadSettings(QSettings *settings, int macroIndex);

    void setCheckedInterval(bool check);
    bool intervalIsChecked() const;
    void setCheckedPeriod(bool check);
    bool periodIsChecked() const;

    void setEnabledInterval(bool enable);
    bool intervalIsEnabled() const;
    void setEnabledPeriod(bool enable);
    bool periodIsEnabled() const;

    void setTime(int time);
    int getTime() const;

    const QByteArray &getPackage() const;
    void openMacroFile(const QString &fileName);
signals:
    void deleted();
    void intervalChecked(bool checked);
    void periodChecked(bool checked);
    void packageSended(const QByteArray &package);
    void sendTimeChanged(int time);
    void movedUp();
    void movedDown();
private:
    ClickableLabel *buttonDelete;
    QCheckBox *checkBoxInterval;
    QCheckBox *checkBoxPeriod;
    QSpinBox *spinBoxTime;
    RightClickedButton *buttonSend;
    ClickableLabel *buttonUp;
    ClickableLabel *buttonDown;
    MacroEdit *macroEdit;
    QTimer *timerPeriod;

    void view();
    void connections();
    void deleteMacro();
    void sendPackage();
    void titleChanged();
    void intervalToggled(bool toggled);
    void periodToggled(bool toggled);
};

#endif // MACRO_WIDGET_H
