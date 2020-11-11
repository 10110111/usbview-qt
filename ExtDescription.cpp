#include "ExtDescription.h"
#include <QList>
#include <QProcess>
#include <QByteArray>
#include <QTreeWidgetItem>
#include <QRegularExpression>
#include "Device.h"

namespace
{

unsigned lineLevel(QByteArray const& line)
{
    int numLeadingSpaces;
    for(numLeadingSpaces=0; numLeadingSpaces<line.size(); ++numLeadingSpaces)
        if(line[numLeadingSpaces]!=' ')
            break;
    // lsusb indents each new level by 2 spaces, but sometimes unindents by 1 space (e.g. for HID report descriptors).
    // We want to ignore these small unindentations, thus rounding up.
    return (numLeadingSpaces+1)/2;
}

unsigned makeTree(QTreeWidgetItem*const root, QList<QByteArray> const& lines, const int startingIndex, const unsigned startingLevel)
{
    unsigned level=startingLevel;
    QTreeWidgetItem* prevItem=root;
    for(int index=startingIndex; index<lines.size(); ++index)
    {
        const auto& line=lines[index];
        if(line.isEmpty()) continue;
        const auto newLevel=lineLevel(line);
        if(newLevel>level)
        {
            index=makeTree(prevItem, lines, index, newLevel);
            --index;
            continue;
        }
        else if(newLevel<level)
            return index;

        auto str=QString(line).trimmed();
        if(str.startsWith("Item("))
            str.replace(QRegularExpression("\\bItem\\((Local|Main|Global) *\\):"), "Item (\\1)  ");
        if(str.startsWith("Port "))
            str.replace(QRegularExpression("\\bPort ([0-9]+): "), "Port \\1  ");

        QTreeWidgetItem* lineItem=nullptr;
        if(str.back()==':')
        {
            lineItem=new QTreeWidgetItem{{str.chopped(1)}};
            root->addChild(lineItem);
        }
        else
        {
            const auto prop=str.split("  ").front();
            const auto value=str.mid(prop.size()).trimmed();
            lineItem=new QTreeWidgetItem{{prop,value}};
            root->addChild(lineItem);
        }
        prevItem=lineItem;
    }
    return lines.size();
}

}

QTreeWidgetItem* getExternalDescription(Device const& dev)
{
    const auto cmd=QString("lsusb -D %1").arg(dev.devicePath);
    QProcess process;
    process.start("sh", {"-c",cmd}, QIODevice::ReadOnly);
    process.waitForFinished(5000);
    if(process.exitStatus()!=QProcess::NormalExit)
        return new QTreeWidgetItem{QStringList{QObject::tr("Failed to run \"%1\"").arg(cmd), QObject::tr("Error %1").arg(process.error())}};
    const auto output=process.readAllStandardOutput().split('\n');
    const auto root=new QTreeWidgetItem{QStringList{QObject::tr("Output of \"%1\"").arg(cmd)}};
    makeTree(root, output, 0, 0);
    return root;
}

