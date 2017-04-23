#include <QGridLayout>
#include <QScrollBar>
#include <QMessageBox>
#include <QToolBar>
#include <QWidgetAction>
#include <algorithm>
#include <QtMath>
#include <QStatusBar>
#include <QSet>

#include "Macros.h"

#include <QDebug>

const int DEFAULT_TIME = 50; // ms
const int DEFAULT_COUNT = 0;
const QString MULTI_SEND_TIME = QObject::tr("Multiple send time, ms: %1");
const bool DEFAULT_CYCLE_MODE = false;

Macros::Macros(QWidget *parent)
    : QMainWindow(parent)
    , actionPause(new QAction(QIcon(":/Resources/Pause.png"), tr("Pause sending macros"), this))
    , actionStartStop(new QAction(QIcon(":/Resources/Play.png"), tr("Start sending macros"), this))
    , actionDelete(new QAction(QIcon(":/Resources/Delete.png"), tr("Delete macros"), this))
    , actionNew(new QAction(QIcon(":/Resources/Add.png"), tr("Add empty macro"), this))
    , actionLoad(new QAction(QIcon(":/Resources/Open.png"), tr("Load macros"), this))
    , spinBoxTime(new QSpinBox(this))
    , actionSelectMacros(new QAction(QIcon(":/Resources/Select.png"), tr("Select macros"), this))
    , actionDeselectMacros(new QAction(QIcon(":/Resources/Deselect.png"), tr("Deselect macros"), this))
    , actionCycleSend(new QAction(QIcon(":/Resources/Cycle.png"), tr("Cycle sending mode"), this))
    , mainWidget(new QWidget(this))
    , scrollAreaLayout(new QVBoxLayout)
    , scrollArea(new QScrollArea(this))
    , fileDialog(new QFileDialog(this))
    , intervalTimer(new QTimer(this))
    , currentIntervalIndex(0)
    , packetTimeCalculator(0)
    , multiSentTime(new QLabel(MULTI_SEND_TIME.arg("None")))
{
    actionPause->setCheckable(true);
    actionPause->setEnabled(false);
    actionCycleSend->setCheckable(true);
    spinBoxTime->setRange(1, 60000);
    spinBoxTime->setToolTip(tr("Time, ms"));
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setStyleSheet("spacing:2px");
    QWidgetAction *actionTime = new QWidgetAction(toolBar);
    actionTime->setDefaultWidget(spinBoxTime);
    QList<QAction*> actions;
    actions << actionDelete << actionSelectMacros << actionDeselectMacros << actionTime
            << actionNew << actionLoad << actionStartStop << actionPause << actionCycleSend;
    toolBar->addActions(actions);
    toolBar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, toolBar);

    QStatusBar *statusBar = new QStatusBar(this);
    statusBar->addWidget(multiSentTime);
    setStatusBar(statusBar);

    setCentralWidget(scrollArea);
    scrollArea->setWidget(mainWidget);
    scrollArea->setVerticalScrollBar(new QScrollBar(Qt::Vertical, scrollArea));
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);

    scrollAreaLayout->setSpacing(0);
    scrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    scrollAreaLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    mainWidget->setLayout(scrollAreaLayout);

    QDir dir;
    if(!dir.exists(dir.currentPath() + "/Macros")) {
        dir.mkpath(dir.currentPath() + "/Macros");
    }
    fileDialog->setDirectory(dir.currentPath() + "/Macros");
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setNameFilter(tr("Terminal Macro File (*.tmf)"));

    connect(actionNew, &QAction::triggered, this, &Macros::addMacro);
    connect(actionDelete, &QAction::triggered,
            this, static_cast<void (Macros::*)()>(&Macros::deleteMacros));
    connect(actionLoad, &QAction::triggered, this, &Macros::loadMacros);
    connect(actionStartStop, &QAction::triggered, this, &Macros::startOrStop);
    connect(actionPause, &QAction::triggered, this, &Macros::pause);
    connect(actionCycleSend, &QAction::toggled, this, &Macros::cycleSingleSendMode);
    connect(intervalTimer, &QTimer::timeout, this, &Macros::sendNextMacro);
}

void Macros::saveSettings(QSettings *settings)
{
    settings->remove("macros");
    settings->setValue("macros/time", spinBoxTime->value());
    settings->setValue("macros/cycle_send_mode", actionCycleSend->isChecked());

    int macroIndex = 1;
    QListIterator<Macro*> it(macros);
    Macro *m = 0;
    while(it.hasNext()) {
        m = it.next();
        m->saveSettings(settings, macroIndex);
        macroIndex++;
    }
    settings->setValue("macros/count", macroIndex - 1);
}

void Macros::loadSettings(QSettings *settings)
{
    spinBoxTime->setValue(settings->value("macros/time", DEFAULT_TIME).toInt());
    actionCycleSend->setChecked(settings->value("macros/cycle_send_mode", DEFAULT_CYCLE_MODE).toBool());

    int macrosCount = settings->value("macros/count", DEFAULT_COUNT).toInt();
    for(int macroIndex = 1; macroIndex <= macrosCount; ++macroIndex) {
        addMacro();
        macros.last()->loadSettings(settings, macroIndex);
    }
}

void Macros::setWorkState(bool work)
{
    actionStartStop->setEnabled(work);
    if(actionStartStop->toolTip() == tr("Stop sending macros")) {
        actionStartStop->setIcon(QIcon(":/Resources/Play.png"));
        actionStartStop->setToolTip(tr("Start sending macros"));
        actionPause->setChecked(false);
        actionPause->setToolTip("Pause sending macros");
        actionPause->setEnabled(false);
        intervalTimer->stop();
        currentIntervalIndex = 0;
        blockForMultiSend(false);
    }
    QListIterator<Macro*> it(macros);
    Macro *m = 0;
    while(it.hasNext()) {
        m = it.next();
        m->setTime(m->getTime());
    }
    if(!work) {
        multiSentTime->setText(MULTI_SEND_TIME.arg("None"));
    }
}

void Macros::setPacketTimeCalculator(PacketTimeCalculator *packetTimeCalculator)
{
    this->packetTimeCalculator = packetTimeCalculator;
}

void Macros::addMacro()
{
    Macro *macro = new Macro(this);
    macro->setPacketTimeCalculator(packetTimeCalculator);
    macros.append(macro);
    scrollAreaLayout->insertWidget(scrollAreaLayout->count() - 1, macro);

    connect(macro, &Macro::deleted, this, static_cast<void (Macros::*)()>(&Macros::deleteMacro));
    connect(macro, &Macro::packetSended, this, &Macros::packetSended);
    connect(macro, &Macro::selected, this, &Macros::updateIntervals);
    connect(macro, &Macro::timeChanged, this, &Macros::calculateMultiSendCeiledTime);
    connect(macro, &Macro::movedUp, this, &Macros::moveMacroUp);
    connect(macro, &Macro::movedDown, this, &Macros::moveMacroDown);
    connect(actionSelectMacros, &QAction::triggered, macro, &Macro::select);
    connect(actionDeselectMacros, &QAction::triggered, macro, &Macro::deselect);
    connect(spinBoxTime, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Macros::setSelectedMacrosTime);
}

void Macros::deleteMacro()
{
    deleteMacro(qobject_cast<Macro*>(sender()));
}

void Macros::deleteMacro(Macro *macro)
{
    if(macro == 0) {
        return;
    }
    macro->deselect();
    macros.removeOne(macro);
    scrollAreaLayout->removeWidget(macro);
    disconnect(macro, &Macro::deleted, this, static_cast<void (Macros::*)()>(&Macros::deleteMacro));
    disconnect(macro, &Macro::packetSended, this, &Macros::packetSended);
    disconnect(macro, &Macro::selected, this, &Macros::updateIntervals);
    disconnect(macro, &Macro::timeChanged, this, &Macros::calculateMultiSendCeiledTime);
    disconnect(macro, &Macro::movedUp, this, &Macros::moveMacroUp);
    disconnect(macro, &Macro::movedDown, this, &Macros::moveMacroDown);
    disconnect(actionSelectMacros, &QAction::triggered, macro, &Macro::select);
    disconnect(actionDeselectMacros, &QAction::triggered, macro, &Macro::deselect);
    disconnect(spinBoxTime, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Macros::setSelectedMacrosTime);
    delete macro;
    macro = 0;
}

void Macros::deleteMacros()
{
    int button = QMessageBox::question(this, tr("Warning"),
                                       tr("Delete macros?"),
                                       QMessageBox::Yes | QMessageBox::No);
    if(button == QMessageBox::Yes) {
        QListIterator<Macro*> it(macros);
        while(it.hasNext()) {
            deleteMacro(it.next());
        }
        macros.clear();
        indexesOfIntervals.clear();
        currentIntervalIndex = 0;
    }
}

void Macros::moveMacroUp()
{
    moveMacro(qobject_cast<Macro*>(sender()), MoveUp);
}

void Macros::moveMacroDown()
{
    moveMacro(qobject_cast<Macro*>(sender()), MoveDown);
}

void Macros::moveMacro(Macro *macro, Macros::MacrosMoveDirection direction)
{
    if(!macro) {
        return;
    }

    int macroIndex = macros.indexOf(macro);

    QVBoxLayout* tempLayout = qobject_cast<QVBoxLayout*>(macro->parentWidget()->layout());
    int index = tempLayout->indexOf(macro);

    if(direction == MoveUp) {
        if(macroIndex == 0) {
            return;
        }

        macros.swap(macroIndex, macroIndex - 1);
        --index;
    } else {
        if(macroIndex == macros.size() - 1) {
            return;
        }

        macros.swap(macroIndex, macroIndex + 1);
        ++index;
    }
    tempLayout->removeWidget(macro);
    tempLayout->insertWidget(index, macro);
    std::sort(indexesOfIntervals.begin(), indexesOfIntervals.end(), qLess<int>());
}

void Macros::loadMacros()
{
    QStringList fileNames;
    if(fileDialog->exec()) {
        fileNames = fileDialog->selectedFiles();
    }

    QListIterator<QString> it(fileNames);
    while(it.hasNext()) {
        addMacro();
        macros.last()->openMacroFile(it.next());
    }
}

void Macros::startOrStop()
{
    if(indexesOfIntervals.isEmpty()) {
        return;
    }
    if(actionStartStop->toolTip() == tr("Start sending macros")) {
        actionStartStop->setIcon(QIcon(":/Resources/Stop.png"));
        actionStartStop->setToolTip(tr("Stop sending macros"));
        actionPause->setEnabled(true);
        blockForMultiSend(true);
        sendNextMacro();
    } else {
        actionStartStop->setIcon(QIcon(":/Resources/Play.png"));
        actionStartStop->setToolTip(tr("Start sending macros"));
        actionPause->setChecked(false);
        actionPause->setToolTip("Pause sending macros");
        actionPause->setEnabled(false);
        intervalTimer->stop();
        currentIntervalIndex = 0;
        blockForMultiSend(false);
    }
}

void Macros::pause(bool check)
{
    if(check) {
        intervalTimer->stop();
        actionPause->setToolTip("Resume sending macros");
    } else {
        intervalTimer->start();
        actionPause->setToolTip("Pause sending macros");
    }
}

void Macros::updateIntervals(bool add)
{
    Macro *macro = qobject_cast<Macro*>(sender());
    if(macro == 0) {
        return;
    }

    int index = macros.indexOf(macro);

    if(add) {
        indexesOfIntervals.append(index);
        QSet<int> set = indexesOfIntervals.toSet();
        indexesOfIntervals = set.toList();
        std::sort(indexesOfIntervals.begin(), indexesOfIntervals.end(), qLess<int>());
    } else {
        indexesOfIntervals.removeOne(index);
    }

    calculateMultiSendCeiledTime();
}

void Macros::sendNextMacro()
{
    Macro *macro = macros.at(indexesOfIntervals.at(currentIntervalIndex++));
    if(currentIntervalIndex >= indexesOfIntervals.size()) {
        currentIntervalIndex = 0;
        if(actionCycleSend->isChecked()) {

            emit packetSended(macro->getPacket());

            startOrStop();

            return;
        }
    }

    emit packetSended(macro->getPacket());

    intervalTimer->start(macro->getTime());
}

void Macros::blockForMultiSend(bool block)
{
    actionDelete->setDisabled(block);
    actionSelectMacros->setDisabled(block);
    actionDeselectMacros->setDisabled(block);
    spinBoxTime->setDisabled(block);
    actionNew->setDisabled(block);
    actionLoad->setDisabled(block);
    actionCycleSend->setDisabled(block);

    QListIterator<Macro*> it(macros);
    while(it.hasNext()) {
        it.next()->setDisabled(block);
    }
}

void Macros::calculateMultiSendCeiledTime()
{
    if(!packetTimeCalculator->isValid()) {
        multiSentTime->setText(MULTI_SEND_TIME.arg("None"));

        return;
    }

    QListIterator<int> it(indexesOfIntervals);
    double totalTime = 0;
    while(it.hasNext()) {
        totalTime += macros.at(it.next())->getTime();
    }
    multiSentTime->setText(MULTI_SEND_TIME.arg(QString::number(totalTime, 'f', 3)));
}

void Macros::cycleSingleSendMode()
{
    if(actionCycleSend->isChecked()) {
        actionCycleSend->setIcon(QIcon(":/Resources/SingleShot.png"));
        actionCycleSend->setToolTip(tr("Single shot sending mode"));
    } else {
        actionCycleSend->setIcon(QIcon(":/Resources/Cycle.png"));
        actionCycleSend->setToolTip(tr("Cycle sending mode"));
    }
}

void Macros::setSelectedMacrosTime()
{
    QListIterator<int> it(indexesOfIntervals);
    while(it.hasNext()) {
        macros[it.next()]->setTime(spinBoxTime->value());
    }
}
