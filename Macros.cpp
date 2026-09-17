#include <QGridLayout>
#include <QScrollBar>
#include <QMessageBox>
#include <QToolBar>
#include <QWidgetAction>
#include <algorithm>
#include <QtMath>
#include <QStatusBar>
#include <QSet>
#include <QDir>
#include <QFileDialog>
#include <QTimer>

#include "Macros.h"

#include <QDebug>

const int DEFAULT_TIME = 50; // ms
const int DEFAULT_COUNT = 0;
const QString MULTI_SEND_TIME = QObject::tr("Multiple send time, ms: %1");
const QString DEFAULT_SEND_MODE = QObject::tr("Single shot sending mode");

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
    , actionSendMode(new QAction(QIcon(":/Resources/Cycle.png"), tr("Cycle sending mode"), this))
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
    spinBoxTime->setRange(1, 60000);
    spinBoxTime->setToolTip(tr("Time, ms"));
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setStyleSheet("spacing:2px");
    QWidgetAction *actionTime = new QWidgetAction(toolBar);
    actionTime->setDefaultWidget(spinBoxTime);
    QList<QAction*> actions;
    actions << actionDelete << actionSelectMacros << actionDeselectMacros << actionTime
            << actionNew << actionLoad << actionStartStop << actionPause << actionSendMode;
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
    scrollAreaLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding));

    mainWidget->setLayout(scrollAreaLayout);

    QDir dir;
    if(!dir.exists(QDir::currentPath() + "/Macros")) {
        dir.mkpath(QDir::currentPath() + "/Macros");
    }
    fileDialog->setDirectory(QDir::currentPath() + "/Macros");
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setNameFilter(tr("Terminal Macro File (*.tmf)"));

    connect(actionNew, &QAction::triggered, this, &Macros::addMacro);
    connect(actionDelete, &QAction::triggered, this, qOverload<>(&Macros::deleteMacros));
    connect(actionLoad, &QAction::triggered, this, &Macros::loadMacros);
    connect(actionStartStop, &QAction::triggered, this, &Macros::startOrStop);
    connect(actionPause, &QAction::triggered, this, &Macros::pause);
    connect(actionSendMode, &QAction::triggered, this, &Macros::cycleSingleSendMode);
    connect(intervalTimer, &QTimer::timeout, this, &Macros::sendNextMacro);
}

void Macros::saveSettings(QSettings *settings)
{
    settings->remove("macros");
    settings->setValue("macros/time", spinBoxTime->value());
    settings->setValue("macros/send_mode", actionSendMode->toolTip() == DEFAULT_SEND_MODE ? 0 : 1);

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
    if(settings->value("macros/send_mode", 0).toInt() == 0) {
        actionSendMode->setIcon(QIcon(":/Resources/SingleShot.png"));
        actionSendMode->setToolTip(tr("Single shot sending mode"));
    } else {
        actionSendMode->setIcon(QIcon(":/Resources/Cycle.png"));
        actionSendMode->setToolTip(tr("Cycle sending mode"));
    }

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

    connect(macro, &Macro::deleted, this, qOverload<>(&Macros::deleteMacro));
    connect(macro, &Macro::packetSended, this, &Macros::packetSended);
    connect(macro, &Macro::selected, this, &Macros::updateIntervals);
    connect(macro, &Macro::timeChanged, this, &Macros::calculateMultiSendCeiledTime);
    connect(macro, &Macro::movedUp, this, &Macros::moveMacroUp);
    connect(macro, &Macro::movedDown, this, &Macros::moveMacroDown);
    connect(actionSelectMacros, &QAction::triggered, macro, &Macro::select);
    connect(actionDeselectMacros, &QAction::triggered, macro, &Macro::deselect);
    connect(spinBoxTime, &QSpinBox::valueChanged, this, &Macros::setSelectedMacrosTime);
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
    disconnect(macro, &Macro::deleted, this, qOverload<>(&Macros::deleteMacro));
    disconnect(macro, &Macro::packetSended, this, &Macros::packetSended);
    disconnect(macro, &Macro::selected, this, &Macros::updateIntervals);
    disconnect(macro, &Macro::timeChanged, this, &Macros::calculateMultiSendCeiledTime);
    disconnect(macro, &Macro::movedUp, this, &Macros::moveMacroUp);
    disconnect(macro, &Macro::movedDown, this, &Macros::moveMacroDown);
    disconnect(actionSelectMacros, &QAction::triggered, macro, &Macro::select);
    disconnect(actionDeselectMacros, &QAction::triggered, macro, &Macro::deselect);
    disconnect(spinBoxTime, &QSpinBox::valueChanged, this, &Macros::setSelectedMacrosTime);
    delete macro;
    macro = 0;
}

void Macros::moveMacro(Macro *macro, MacrosMoveDirection direction)
{
    int macroIndex = macros.indexOf(macro);
    if(macroIndex == -1) {
        return;
    }

    if(direction == MacrosMoveDirection::Up && macroIndex > 0) {
        macros.swapItemsAt(macroIndex, macroIndex - 1);
        scrollAreaLayout->removeWidget(macro);
        scrollAreaLayout->insertWidget(macroIndex - 1, macro);
    } else if(direction == MacrosMoveDirection::Down && macroIndex < macros.count() - 1) {
        macros.swapItemsAt(macroIndex, macroIndex + 1);
        scrollAreaLayout->removeWidget(macro);
        scrollAreaLayout->insertWidget(macroIndex + 1, macro);
    }
    
    std::sort(indexesOfIntervals.begin(), indexesOfIntervals.end(), std::less<int>());
}

void Macros::updateIntervals(bool)
{
    indexesOfIntervals.clear();
    for(int i = 0; i < macros.count(); ++i) {
        if(macros.at(i)->selectState()) {
            indexesOfIntervals.append(i);
        }
    }
    
    QSet<int> set(indexesOfIntervals.begin(), indexesOfIntervals.end());
    indexesOfIntervals = set.values(); 
    
    std::sort(indexesOfIntervals.begin(), indexesOfIntervals.end(), std::less<int>());
    calculateMultiSendCeiledTime();
}

void Macros::deleteMacros()
{
    int button = QMessageBox::question(this, tr("Warning"), tr("Delete macros?"), QMessageBox::Yes | QMessageBox::No);
    if (button == QMessageBox::Yes) {
        while(!macros.isEmpty()) {
            deleteMacro(macros.first());
        }
    }
}
