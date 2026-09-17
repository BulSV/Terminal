#include <QAction>
#include <QGroupBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QGridLayout>
#include <QIntValidator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QtMath>
#include <QFileInfo>

#include "MacroEdit.h"

const int TABLE_WIDTH = 250;
const int GROUP_BYTES_COUNT = 8;
const char CR = 0x0A;
const char LF = 0x0D;
const QString TITLE = QObject::tr("Macro edit - %1");

#include <QDebug>

MacroEdit::MacroEdit(QWidget *parent)
    : QMainWindow(parent, Qt::WindowCloseButtonHint)
    , toolBar(new QToolBar(this))
    , actionNew(new QAction(QIcon(":/Resources/New.png"), tr("New"), this))
    , actionLoad(new QAction(QIcon(":/Resources/Open.png"), tr("Load"), this))
    , actionSave(new QAction(QIcon(":/Resources/Save.png"), tr("Save"), this))
    , actionSaveAs(new QAction(QIcon(":/Resources/SaveAs.png"), tr("Save as"), this))
    , actionRawEdit(new QAction(QIcon(":/Resources/RawEdit.png"), tr("Raw Edit"), this))
    , actionClearSelectedGroup(new QAction(QIcon(":/Resources/Clear.png"), tr("Clear Selected Group"), this))
    , actionCR(new QAction(QIcon(":/Resources/CR.png"), tr("CR"), this))
    , actionLF(new QAction(QIcon(":/Resources/LF.png"), tr("LF"), this))
    , actionAddGroup(new QAction(QIcon(":/Resources/Add.png"), tr("Add Group"), this))
    , actionDeleteGroup(new QAction(QIcon(":/Resources/Delete.png"), tr("Delete Group"), this))
    , tableAscii(new DataTable(1, this))
    , tableHex(new DataTable(GROUP_BYTES_COUNT, this))
    , tableDec(new DataTable(GROUP_BYTES_COUNT, this))
    , macroGroupsLayout(new QVBoxLayout)
    , asciiEncoder(new AsciiEncoder)
    , hexEncoder(new HexEncoder)
    , decEncoder(new DecEncoder)
    , macroRawEdit(new MacroRawEdit(0))
    , macroOpenDir(".")
    , macroSaveDir(".")
    , fileOpenDialog(new QFileDialog(this, tr("Open Macro File"), macroOpenDir, "Terminal Macro File (*.tmf)"))
    , fileSaveAsDialog(new QFileDialog(this, tr("Save Macro File"), macroSaveDir, "Terminal Macro File (*.tmf)"))
{
    setWindowTitle(TITLE.arg("No name"));

    emit titleChanged(QString(tr("No name")));

    actionCR->setCheckable(true);
    actionLF->setCheckable(true);

    QFont font;
    font.setFamily("Lucida Console");
    font.setPixelSize(12);
    tableAscii->setFont(font);
    tableHex->setFont(font);
    tableDec->setFont(font);

    tableAscii->setMouseSelectEnable(false);
    tableAscii->setDataAlignment(Qt::AlignLeft);
    tableAscii->setValidator(new QRegExpValidator(QRegExp(".{0,8}"), this));
    tableHex->setValidator(new QRegExpValidator(QRegExp("([0-9]|[a-f]|[A-F]){1,2}"), this));
    tableDec->setValidator(new QIntValidator(0, 255, this));

    tableAscii->setFixedWidth(TABLE_WIDTH);
    tableHex->setFixedWidth(TABLE_WIDTH);
    tableDec->setFixedWidth(TABLE_WIDTH);

    fileSaveAsDialog->setDefaultSuffix("tmf");
    fileSaveAsDialog->setAcceptMode(QFileDialog::AcceptSave);

    addGroup();

    macroGroups.last()->setSelected(true);

    currentEditGroup = macroGroups.last();

    macroGroupsLayout->setSpacing(0);
    macroGroupsLayout->setContentsMargins(0, 0, 0, 0);

    view();
    connections();
}

MacroEdit::~MacroEdit()
{
    delete macroRawEdit;
    macroRawEdit = 0;
}

void MacroEdit::addCR_LF()
{
    packet_CR_LF = packet;
    if(actionCR->isChecked()) {
        packet_CR_LF.append(CR);
    }
    if(actionLF->isChecked()) {
        packet_CR_LF.append(LF);
    }
}

void MacroEdit::newMacroFile()
{
    int groupsCount = macroGroups.size();
    for(int i = 0; i < groupsCount - 1; ++i) {
        deleteGroup();
    }
    clearSelectedGroup();
    macroFileName.clear();
    packet.clear();
    setWindowTitle(TITLE.arg("No name"));

    emit titleChanged(QString(tr("No name")));
}

void MacroEdit::openMacroFile()
{
    openMacroFile(fileOpenDialog->selectedFiles().first());
}

void MacroEdit::saveMacroFile()
{
    if(macroFileName.isEmpty()) {
        fileSaveAsDialog->show();

        return;
    }
    formPacket();
    saveMacro.setData(packet);
    saveMacro.save(macroFileName);
}

void MacroEdit::saveAsMacroFile()
{
    QString fileName = fileSaveAsDialog->selectedFiles().first();
    if(fileName.isEmpty()) {
        return;
    }
    formPacket();
    saveMacro.setData(packet);
    if(!saveMacro.save(fileName)) {
        return;
    }
    macroFileName = fileName;
    QFileInfo fileInfo(fileName);
    fileName = fileInfo.fileName();
    fileName.chop(4);
    setWindowTitle(TITLE.arg(fileName));

    emit titleChanged(fileName);
}

void MacroEdit::onPacketChanged()
{
    emit packetChanged(getPacket());
}

void MacroEdit::formPacket()
{
    QListIterator<DataTable*> itMacroGroups(macroGroups);
    QString hexData;
    while(itMacroGroups.hasNext()) {
        QListIterator<QString> itHexData(itMacroGroups.next()->getData());
        while(itHexData.hasNext()) {
            QString tempString = itHexData.next();
            if(tempString.isEmpty()) {
                break;
            }
            hexData.append(tempString);
            hexData.append(" ");
        }
    }
    hexData.chop(1);
    hexEncoder->setData(hexData, " ");
    packet = hexEncoder->encodedByteArray();
}

const QByteArray &MacroEdit::getPacket()
{
    addCR_LF();

    return packet_CR_LF;
}

void MacroEdit::saveSettings(QSettings *settings, int macroIndex)
{
    QString macroIndexString = QString::number(macroIndex);
    if(macroFileName.isEmpty()) {
        formPacket();
        QString packetString;
        int packageSize = packet.size();
        for(int i = 0; i < packageSize; ++i) {
            packetString.append(QString::number((unsigned char)packet.at(i), 16).toUpper());
            packetString.append(" ");
        }
        packetString.chop(1);
        settings->setValue("macros/" + macroIndexString + "/packet", packetString);
    } else {
        settings->setValue("macros/" + macroIndexString + "/path", macroFileName);
    }
    settings->setValue("macros/" + macroIndexString + "/CR", actionCR->isChecked());
    settings->setValue("macros/" + macroIndexString + "/LF", actionLF->isChecked());
    settings->setValue("macros/" + macroIndexString + "/position", pos());
    settings->setValue("macros/" + macroIndexString + "/size", size());

    macroRawEdit->saveSettings(settings, macroIndex);
}

void MacroEdit::loadSettings(QSettings *settings, int macroIndex)
{
    QString macroIndexString = QString::number(macroIndex);
    QString fileName = settings->value("macros/" + macroIndexString + "/path").toString();
    if(!fileName.isEmpty()) {
        if(!openMacro.open(fileName)) {
            return;
        }
        macroFileName = fileName;
        setRawData(openMacro.getData());
        QFileInfo fileInfo(fileName);
        fileName = fileInfo.fileName();
        fileName.chop(4);
        setWindowTitle(TITLE.arg(fileName));

        emit titleChanged(fileName);
    } else {
        QList<QString> packageList = settings->value("macros/" + macroIndexString + "/packet").toString().split(" ");
        int dataSize = packageList.size();
        bool ok;
        for(int i = 0; i < dataSize; ++i) {
            char byte = static_cast<char>(packageList.at(i).toInt(&ok, 16));
            if(ok) {
                packet.append(byte);
            }
        }
        setRawData(packet);
    }
    actionCR->setChecked(settings->value("macros/" + macroIndexString + "/CR").toBool());
    actionLF->setChecked(settings->value("macros/" + macroIndexString + "/LF").toBool());
    QPoint pos = settings->value("macros/" + macroIndexString + "/position").toPoint();
    if(!pos.isNull()) {
        move(pos);
    }
    QSize size = settings->value("macros/" + macroIndexString + "/size").toSize();
    if(size.isValid()) {
        resize(size);
    }

    macroRawEdit->loadSettings(settings, macroIndex);
}

void MacroEdit::openMacroFile(const QString &fileName)
{
    if(fileName.isEmpty()) {
        return;
    }
    if(!openMacro.open(fileName)) {
        return;
    }
    macroFileName = fileName;
    setRawData(openMacro.getData());
    QFileInfo fileInfo(fileName);
    QString title = fileInfo.fileName();
    title.chop(4);
    setWindowTitle(TITLE.arg(title));

    emit titleChanged(title);
}

void MacroEdit::connections()
{
    connect(actionNew, &QAction::triggered, this, &MacroEdit::newMacroFile);

    connect(actionLoad, &QAction::triggered, fileOpenDialog, &QFileDialog::show);
    connect(fileOpenDialog, &QFileDialog::accepted, [this](){openMacroFile();});

    connect(actionSave, &QAction::triggered, this, &MacroEdit::saveMacroFile);

    connect(actionSaveAs, &QAction::triggered, fileSaveAsDialog, &MacroEdit::show);
    connect(fileSaveAsDialog, &QFileDialog::accepted, this, &MacroEdit::saveAsMacroFile);

    connect(actionRawEdit, &QAction::triggered, this, &MacroEdit::onEditRawData);
    connect(actionClearSelectedGroup, &QAction::triggered, this, &MacroEdit::clearSelectedGroup);
    connect(actionCR, &QAction::triggered, this, &MacroEdit::onPacketChanged);
    connect(actionLF, &QAction::triggered, this, &MacroEdit::onPacketChanged);
    connect(actionAddGroup, &QAction::triggered, this, &MacroEdit::addGroup);
    connect(actionDeleteGroup, &QAction::triggered, this, &MacroEdit::deleteGroup);

    connect(tableAscii, &DataTable::editingFinished, this, &MacroEdit::fromAsciiInput);
    connect(tableHex, &DataTable::editingFinished, this, &MacroEdit::fromHexInput);
    connect(tableDec, &DataTable::editingFinished, this, &MacroEdit::fromDecInput);

    connect(macroRawEdit, &MacroRawEdit::dataInputted, this, &MacroEdit::setRawData);
}

void MacroEdit::view()
{
    QList<QAction*> actions;
    actions << actionNew
            << actionLoad
            << actionSave
            << actionSaveAs
            << actionRawEdit
            << actionClearSelectedGroup
            << actionCR
            << actionLF
            << actionAddGroup
            << actionDeleteGroup;
    addToolBar(Qt::TopToolBarArea, toolBar);
    toolBar->setMovable(false);
    toolBar->addActions(actions);

    QGridLayout *editGroupLayout = new QGridLayout;
    editGroupLayout->setContentsMargins(5, 5, 5, 5);
    editGroupLayout->addWidget(new QLabel("ASCII", this), 1, 0, Qt::AlignTop);
    editGroupLayout->addWidget(tableAscii, 0, 1, 2, 8, Qt::AlignVCenter);
    editGroupLayout->addWidget(new QLabel("HEX", this), 3, 0, Qt::AlignTop);
    editGroupLayout->addWidget(tableHex, 2, 1, 2, 8, Qt::AlignVCenter);
    editGroupLayout->addWidget(new QLabel("DEC", this), 5, 0, Qt::AlignTop);
    editGroupLayout->addWidget(tableDec, 4, 1, 2, 8, Qt::AlignVCenter);

    QGroupBox *editGroup = new QGroupBox(tr("Edit selected group"), this);
    editGroup->setLayout(editGroupLayout);

    macroGroupsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QWidget *scrollWidget = new QWidget(this);
    scrollWidget->setLayout(macroGroupsLayout);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setVerticalScrollBar(new QScrollBar(Qt::Vertical, scrollArea));
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFixedWidth(TABLE_WIDTH + 30);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *scrollLayout = new QVBoxLayout;
    scrollLayout->setContentsMargins(5, 5, 5, 5);
    scrollLayout->addWidget(scrollArea);

    QGroupBox *packageGroups = new QGroupBox(tr("Packet groups"), this);
    packageGroups->setLayout(scrollLayout);

    QGridLayout *editLayout = new QGridLayout;
    editLayout->setSpacing(5);
    editLayout->addWidget(editGroup, 0, 0, 6, 9, Qt::AlignCenter);
    editLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 6, 0);

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addItem(editLayout, 0, 0, 8, 9);
    mainLayout->addWidget(packageGroups, 0, 9, 8, 9);
    mainLayout->setSpacing(5);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    setMaximumWidth(minimumWidth());
}

void MacroEdit::selectGroup()
{
    DataTable *group = qobject_cast<DataTable*>(sender());
    if(group == 0) {
        return;
    }
    currentEditGroup = group;
    fillEditGroup();
}

void MacroEdit::clearSelectedGroup()
{
    tableAscii->clearData();
    tableHex->clearData();
    tableDec->clearData();
    currentEditGroup->clearData();
}

void MacroEdit::addGroup()
{
    DataTable *group = new DataTable(GROUP_BYTES_COUNT, this);
    group->setFixedWidth(TABLE_WIDTH);
    group->setReadOnly(true);
    group->setMouseSelectEnable(false);
    group->setLabels(getLabels());
    QFont font;
    font.setFamily("Lucida Console");
    font.setPixelSize(12);
    group->setFont(font);
    connect(group, &DataTable::tableSelected, this, &MacroEdit::selectGroup);

    macroGroups.append(group);
    macroGroupsLayout->insertWidget(macroGroupsLayout->count() - 1, group, 0, Qt::AlignCenter);
}

void MacroEdit::deleteGroup()
{
    if(macroGroups.size() < 2) {
        return;
    }
    if(currentEditGroup == macroGroups.last()) {
        currentEditGroup = macroGroups[macroGroups.size() - 2];
        fillEditGroup();

    }
    disconnect(macroGroups.last(), &DataTable::tableSelected, this, &MacroEdit::selectGroup);
    macroGroupsLayout->removeWidget(macroGroups.last());
    delete macroGroups.takeLast();
}

QList<QString> MacroEdit::getLabels() const
{
    QList<QString> labels;
    int index = macroGroups.size() * GROUP_BYTES_COUNT + 1;
    for(int i = index; i < index + GROUP_BYTES_COUNT; ++i) {
        labels.append(QString::number(i));
    }

    return labels;
}

QList<QString> MacroEdit::toUpper(const QList<QString> &source) const
{
    QList<QString> result;
    QListIterator<QString> it(source);
    QString tempString;
    while(it.hasNext()) {
        tempString = it.next();
        if(tempString.toInt() < 0x10 && tempString.size() == 1) {
            result.append("0" + tempString.toUpper());

            continue;
        }
        result.append(tempString.toUpper());
    }

    return result;
}

void MacroEdit::fromAsciiInput()
{
    QListIterator<QString> it(tableAscii->getData());
    QString asciiData;
    while(it.hasNext()) {
        asciiData.append(it.next());
    }
    asciiEncoder->setData(asciiData);
    hexEncoder->setData(asciiEncoder->encodedByteArray());
    decEncoder->setData(asciiEncoder->encodedByteArray());
    tableHex->setData(hexEncoder->encodedStringList());
    tableDec->setData(decEncoder->encodedStringList());
    tableHex->setData(toUpper(tableHex->getData()));
    currentEditGroup->setData(tableHex->getData());

    formPacket();
}

void MacroEdit::fromHexInput()
{
    fillEmptyBytes(tableHex);
    hexEncoder->setData(fromStringListToString(tableHex), " ");
    asciiEncoder->setData(hexEncoder->encodedByteArray());
    decEncoder->setData(hexEncoder->encodedByteArray());
    tableAscii->setData(getAsciiString());
    nonPrintableCharacters();
    tableDec->setData(decEncoder->encodedStringList());
    tableHex->setData(toUpper(tableHex->getData()));
    currentEditGroup->setData(tableHex->getData());

    formPacket();
}

void MacroEdit::fromDecInput()
{
    fillEmptyBytes(tableDec);
    decEncoder->setData(fromStringListToString(tableDec), " ");
    asciiEncoder->setData(decEncoder->encodedByteArray());
    hexEncoder->setData(decEncoder->encodedByteArray());
    tableAscii->setData(getAsciiString());
    nonPrintableCharacters();
    tableHex->setData(hexEncoder->encodedStringList());
    tableHex->setData(toUpper(tableHex->getData()));
    currentEditGroup->setData(tableHex->getData());

    formPacket();
}

void MacroEdit::nonPrintableCharacters()
{
    bool wasNonPrintingChar = false;
    connect(tableAscii, &DataTable::editingFinished, this, &MacroEdit::fromAsciiInput);
    QString asciiString = tableAscii->getData().first();
    for(int i = 0; i < asciiString.size(); ++i) {
        if(!asciiString.at(i).isPrint()) {
            wasNonPrintingChar = true;
        }
    }
    tableAscii->setReadOnly(wasNonPrintingChar);
    if(wasNonPrintingChar) {
        disconnect(tableAscii, &DataTable::editingFinished, this, &MacroEdit::fromAsciiInput);
    }
}

QList<QString> MacroEdit::getAsciiString()
{
    QString asciiString;
    QListIterator<QString> itAscii(asciiEncoder->encodedStringList());
    while(itAscii.hasNext()) {
        asciiString.append(itAscii.next());
    }
    QList<QString> asciiList;
    asciiList.append(asciiString);

    return asciiList;
}

void MacroEdit::fillEmptyBytes(DataTable *dataTable)
{
    QListIterator<QString> it(dataTable->getData());
    QString dataString;
    QList<QString> result;
    bool wasNotEmpty = false;
    it.toBack();
    while(it.hasPrevious()) {
        dataString = it.previous();
        if(dataString.isEmpty() && !wasNotEmpty) {
            continue;
        }
        wasNotEmpty = true;
        if(dataString.isEmpty()) {
            result.prepend(QString::number(0));

            continue;
        }
        result.prepend(dataString);
    }
    dataTable->setData(result);
}

QString MacroEdit::fromStringListToString(DataTable *dataTable)
{
    QListIterator<QString> it(dataTable->getData());
    QString data;
    while(it.hasNext()) {
        data.append(it.next());
        data.append(" ");
    }
    data.chop(1);

    return data;
}

void MacroEdit::fillEditGroup()
{
    QList<QString> labels;
    labels.append(currentEditGroup->getLabels().first() + "-" + currentEditGroup->getLabels().last());
    tableAscii->setLabels(labels);
    tableAscii->clearData();
    tableHex->setLabels(currentEditGroup->getLabels());
    tableHex->clearData();
    tableHex->setData(currentEditGroup->getData());
    tableDec->setLabels(currentEditGroup->getLabels());
    tableDec->clearData();
    fromHexInput();
}

void MacroEdit::setRawData(const QByteArray &rawData)
{
    currentEditGroup->setFocus();

    int rawDataGroupsCount = qCeil(static_cast<qreal>(rawData.size()) / GROUP_BYTES_COUNT);
    int dataGroupsCount = macroGroups.size();
    if(dataGroupsCount < rawDataGroupsCount) {
        for(int i = dataGroupsCount; i < rawDataGroupsCount; ++i) {
            addGroup();
        }
    }
    if(dataGroupsCount > rawDataGroupsCount) {
        for(int i = rawDataGroupsCount; i < dataGroupsCount; ++i) {
            deleteGroup();
        }
    }
    QListIterator<DataTable*> it(macroGroups);
    hexEncoder->setData(rawData);
    QList<QString> dataList = hexEncoder->encodedStringList();
    int i = 0;
    while(it.hasNext()) {
        it.next()->setData(dataList.mid(i, GROUP_BYTES_COUNT));
        i += 8;
    }
    currentEditGroup = macroGroups.first();
    fillEditGroup();
}

void MacroEdit::onEditRawData()
{
    formPacket();
    macroRawEdit->setData(packet);
    macroRawEdit->setWindowTitle(windowTitle());
    macroRawEdit->show();
}
