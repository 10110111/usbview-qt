#include "PropertiesWidget.h"
#include <stack>
#include <iostream>
#include <QProcess>
#include <QFontDatabase>
#include "Device.h"
#include "ExtDescription.h"
#include "common.hpp"

namespace
{

enum DescriptorType
{
    DT_DEVICE = 1,
    DT_CONFIG,
    DT_STRING,
    DT_INTERFACE,
    DT_ENDPOINT,
    DT_DEVICE_QUALIFIER,
    DT_OTHER_SPEED_CONFIG,
    DT_INTERFACE_POWER,
    DT_OTG,
    DT_DEBUG,
    DT_INTERFACE_ASSOC,
    DT_HID=0x21,
    DT_CS_INTERFACE=0x24, // class-specific interface
    DT_CS_ENDPOINT=0x25,  // class-specific endpoint
    DT_HUB=0x29,
    DT_ENHANCED_SUPERSPEED_HUB=0x2a,
    DT_SS_ENDPOINT_COMPANION=0x30,
};

QString name(const DescriptorType type)
{
    static const std::map<DescriptorType, QString> types={
        {DT_DEVICE,             QObject::tr("device")},
        {DT_CONFIG,             QObject::tr("config")},
        {DT_STRING,             QObject::tr("string")},
        {DT_INTERFACE,          QObject::tr("interface")},
        {DT_ENDPOINT,           QObject::tr("endpoint")},
        {DT_DEVICE_QUALIFIER,   QObject::tr("device qualifier")},
        {DT_OTHER_SPEED_CONFIG, QObject::tr("other speed config")},
        {DT_INTERFACE_POWER,    QObject::tr("interface power")},
        {DT_OTG,                QObject::tr("OTG")},
        {DT_DEBUG,              QObject::tr("debug")},
        {DT_INTERFACE_ASSOC,    QObject::tr("interface association")},
        {DT_HID,                QObject::tr("HID")},
        {DT_CS_INTERFACE,       QObject::tr("class-specific interface")},
        {DT_CS_ENDPOINT,        QObject::tr("class-specific endpoint")},
        {DT_HUB,                QObject::tr("hub")},
        {DT_ENHANCED_SUPERSPEED_HUB,QObject::tr("enhanced SuperSpeed hub")},
        {DT_SS_ENDPOINT_COMPANION,QObject::tr("SuperSpeed endpoint companion")},
    };
    const auto typeNameIt=types.find(type);
    if(typeNameIt==types.end())
        return QObject::tr("unknown type 0x%1").arg(static_cast<unsigned>(type), 2, 16, QChar('0'));
    return typeNameIt->second;
}

QString formatBytes(std::vector<uint8_t> const& data, const bool wrap)
{
    QString str;
    for(unsigned i=0; i<data.size(); ++i)
	{
        str += QString("%1 ").arg(+data[i], 2, 16, QChar('0'));
		if(wrap && (i+1)%16 == 0)
			str += '\n';
	}
    return str.trimmed();
}

void setFirstColumnSpannedForAllSingleColumnItems(QTreeWidgetItem* item)
{
    if(item->columnCount()==1)
        item->setFirstColumnSpanned(true);
    for(int i=0; i<item->childCount(); ++i)
        setFirstColumnSpannedForAllSingleColumnItems(item->child(i));
}

bool isFixedPitch(QFont const& font)
{
    return QFontInfo(font).fixedPitch();
}

QFont getMonospaceFont(QFont const& base)
{
    if(const auto font=QFontDatabase::systemFont(QFontDatabase::FixedFont); isFixedPitch(font)) return font;
    auto font(base);
    if(isFixedPitch(font)) return font;
    font.setFamily("monospace");
    if(isFixedPitch(font)) return font;
    font.setStyleHint(QFont::Monospace);
    if(isFixedPitch(font)) return font;
    font.setStyleHint(QFont::TypeWriter);
    if(isFixedPitch(font)) return font;
    font.setFamily("courier");
    if(isFixedPitch(font)) return font;
    std::cerr << "Warning: failed to get a monospace font\n";
    return font;
}

void parseHIDReportDescriptor(QTreeWidgetItem* root, std::vector<uint8_t> const& data)
{
    enum ItemType
    {
        IT_MAIN,
        IT_GLOBAL,
        IT_LOCAL,
        IT_RESERVED,
    };
    enum MainItemTag
    {
        MIT_INPUT         =0x8,
        MIT_OUTPUT        =0x9,
        MIT_COLLECTION    =0xA,
        MIT_FEATURE       =0xB,
        MIT_END_COLLECTION=0xC,
    };
    enum GlobalItemTag
    {
        GIT_USAGE_PAGE,
        GIT_LOG_MIN,
        GIT_LOG_MAX,
        GIT_PHYS_MIN,
        GIT_PHYS_MAX,
        GIT_UNIT_EXP,
        GIT_UNIT,
        GIT_REP_SIZE,
        GIT_REP_ID,
        GIT_REP_COUNT,
        GIT_PUSH,
        GIT_POP,
    };
    try
    {
        std::stack<QTreeWidgetItem*> prevRoots;
        for(unsigned i=0; i<data.size();)
        {
            const auto head=data[i];
            auto str=QString("%1").arg(head, 2,16,QChar('0'));
            if(head==0xfe)
            {
                // Long item
                const unsigned dataSize=data.at(i+1);
                const unsigned tag=data.at(i+2);
                str+=QString(": %1 %2: ").arg(dataSize, 2,16,QChar('0')).arg(tag, 2,16,QChar('0'));
                for(unsigned k=0; k<dataSize; ++k)
                    str+=QString(" %1").arg(data.at(i+3+k), 2,16,QChar('0'));
                str += " (Long)";

                root->addChild(new QTreeWidgetItem{{str}});
                i += dataSize+3;
            }
            else
            {
                // Short item
                const unsigned dataSize = (head&3)==3 ? 4 : (head&3);
                if(dataSize>0)
                    str+=":";
                for(unsigned k=0; k<dataSize; ++k)
                    str+=QString(" %1").arg(data.at(i+1+k), 2,16,QChar('0'));
                const auto type = head>>2 & 3;
                const QString types[]={QObject::tr("Main"), QObject::tr("Global"), QObject::tr("Local"), QObject::tr("Reserved")};
                str += QString(" (%1)").arg(types[type]);

                auto item=new QTreeWidgetItem{{str}};
                root->addChild(item);
                const auto tag=head>>4;
                switch(type)
                {
                case IT_MAIN:
                    switch(tag)
                    {
                    case MIT_INPUT:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Input"));
                        break;
                    case MIT_OUTPUT:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Output"));
                        break;
                    case MIT_COLLECTION:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Collection"));
                        prevRoots.push(root);
                        root->addChild(item);
                        root=item;
                        break;
                    case MIT_FEATURE:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Feature"));
                        break;
                    case MIT_END_COLLECTION:
                        if(prevRoots.empty())
                        {
                            item->setData(1, Qt::DisplayRole, QObject::tr("*** Stray End Collection"));
                            break;
                        }
                        item->setData(1, Qt::DisplayRole, QObject::tr("End Collection"));
                        root=prevRoots.top();
                        prevRoots.pop();
                        break;
                    }
                    break;
                case IT_GLOBAL:
                    switch(tag)
                    {
                    case GIT_USAGE_PAGE:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage Page"));
                        break;
                    case GIT_LOG_MIN:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Logical Minimum"));
                        break;
                    case GIT_LOG_MAX:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Logical Maximum"));
                        break;
                    case GIT_PHYS_MIN:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Physical Minimum"));
                        break;
                    case GIT_PHYS_MAX:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Physical Maximum"));
                        break;
                    case GIT_UNIT_EXP:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Unit Exponent"));
                        break;
                    case GIT_UNIT:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Unit"));
                        break;
                    case GIT_REP_SIZE:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Report Size"));
                        break;
                    case GIT_REP_ID:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Report ID"));
                        break;
                    case GIT_REP_COUNT:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Report Count"));
                        break;
                    case GIT_PUSH:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Push"));
                        break;
                    case GIT_POP:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Pop"));
                        break;
                    }
                }

                i += dataSize+1;
            }
        }
    }
    catch(std::out_of_range const&)
    {
        root->addChild(new QTreeWidgetItem{{QObject::tr("(broken item: too few bytes)")}});
    }
}

}

PropertiesWidget::PropertiesWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabels({"Property", "Value"});
}

void PropertiesWidget::showDevice(Device const* dev)
{
    device_=dev;
    updateTree();
}

void PropertiesWidget::updateTree()
{
    clear();
    if(!device_) return;

    const auto monoFont=getMonospaceFont(font());
    addTopLevelItem(new QTreeWidgetItem{QStringList{"SYSFS path", device_->sysfsPath}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Device path", device_->devicePath}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Vendor Id", QString("0x%1").arg(device_->vendorId, 4, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Product Id", QString("0x%1").arg(device_->productId, 4, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Revision", device_->revision}});
    if(!device_->manufacturer.isNull())
        addTopLevelItem(new QTreeWidgetItem{QStringList{"Manufacturer", device_->manufacturer}});
    if(!device_->product.isNull())
        addTopLevelItem(new QTreeWidgetItem{QStringList{"Product", device_->product}});
    if(!device_->serialNum.isNull())
        addTopLevelItem(new QTreeWidgetItem{QStringList{"Serial number", device_->serialNum}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Bus", QString::number(device_->busNum)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Address", QString::number(device_->devNum)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Port", QString::number(device_->port)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"USB version", device_->usbVersion}});
    {
        QString speed;
        const auto speedX10=device_->speed*10;
        switch(int(speedX10))
        {
        case 15:     speed=tr(u8"1.5\u202fMb/s (low)");   break;
        case 120:    speed=tr(u8"12\u202fMb/s (full)");   break;
        case 4800:   speed=tr(u8"480\u202fMb/s (high)");  break;
        case 50000:  speed=tr(u8"5\u202fGb/s (super)");   break;
        case 100000: speed=tr(u8"10\u202fGb/s (super+)"); break;
        }
        if(speedX10!=int(speedX10) || speed.isNull())
        {
            if(device_->speed<1000)
                speed=tr(u8"%1\u202fMb/s").arg(device_->speed);
            else
                speed=tr(u8"%1\u202fGb/s").arg(device_->speed/1000);
        }
        addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Speed"), speed}});
    }
    addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Device class"), QString("0x%1 (%2)").arg(device_->devClass, 2, 16, QLatin1Char('0'))
                                                                                      .arg(device_->devClassStr)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Device subclass"), QString("0x%1").arg(device_->devSubClass, 2, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Device protocol"), QString("0x%1").arg(device_->devProtocol, 2, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Max default endpoint packet size"), QString::number(device_->maxPacketSize)}});
    const auto configsItem=new QTreeWidgetItem{QStringList{tr("Configurations")}};
    addTopLevelItem(configsItem);
    configsItem->setExpanded(true);
    for(const auto& config : device_->configs)
    {
        const auto configItem=new QTreeWidgetItem{QStringList{tr("Configuration %1").arg(config.configNum)}};
        configsItem->addChild(configItem);
        configItem->setExpanded(true);
        const auto attribItem=new QTreeWidgetItem{QStringList{tr("Attributes"),
                                                              QString("0x%1").arg(config.attributes, 2, 16, QLatin1Char('0'))}};
        configItem->addChild(attribItem);
        {
            const auto poweringItem=new QTreeWidgetItem{QStringList{config.attributes&1<<6 ? tr("Self-powered") : tr("Bus-powered")}};
            attribItem->addChild(poweringItem);
            const auto remoteWakeupItem=new QTreeWidgetItem{QStringList{config.attributes&1<<5 ? tr("Supports remote wakeup") :
                                                                                                 tr("No remote wakeup support")}};
            attribItem->addChild(remoteWakeupItem);
        }
        configItem->addChild(new QTreeWidgetItem{QStringList{tr("Max power needed"),
                                                 QString(tr(u8"%1\u202fmA")).arg(config.maxPowerMilliAmp)}});
        const auto ifacesItem=new QTreeWidgetItem{QStringList{tr("Interfaces")}};
        configItem->addChild(ifacesItem);
        ifacesItem->setExpanded(true);
        for(const auto& iface : config.interfaces)
        {
            const auto ifaceItem=new QTreeWidgetItem{QStringList{tr("Interface %1").arg(iface.ifaceNum)}};
            ifacesItem->addChild(ifaceItem);
            ifaceItem->setExpanded(true);
//            ifaceItem->addChild(new QTreeWidgetItem{QStringList{tr("Active alternate setting"), iface.activeAltSetting ? tr("yes") : tr("no")}});
            ifaceItem->addChild(new QTreeWidgetItem{QStringList{tr("SYSFS path"), iface.sysfsPath}});
            if(!iface.deviceNodes.empty())
            {
                const auto devNodesItem=new QTreeWidgetItem{QStringList{tr("Device nodes")}};
                ifaceItem->addChild(devNodesItem);
                for(const auto& node : iface.deviceNodes)
                    devNodesItem->addChild(new QTreeWidgetItem{{node}});
            }
            ifaceItem->addChild(new QTreeWidgetItem{QStringList{tr("Alternate setting number"), QString::number(iface.altSettingNum)}});
            ifaceItem->addChild(new QTreeWidgetItem{QStringList{tr("Class"), QString("0x%1 (%2)")
                                        .arg(iface.ifaceClass, 2, 16, QLatin1Char('0')).arg(iface.ifaceClassStr)}});
            QString subclassStr, protocolStr;
            constexpr unsigned classHID=0x03;
            if(iface.ifaceClass==classHID)
            {
                if(iface.ifaceSubClass==1)
                    subclassStr=tr("0x%1 (boot interface)").arg(iface.ifaceSubClass, 2, 16, QLatin1Char('0'));
                if(iface.protocol==1)
                    protocolStr=QString("0x%1 (keyboard)").arg(iface.protocol, 2, 16, QLatin1Char('0'));
                else if(iface.protocol==2)
                    protocolStr=QString("0x%1 (mouse)").arg(iface.protocol, 2, 16, QLatin1Char('0'));
            }
            if(subclassStr.isNull())
                subclassStr=QString("0x%1").arg(iface.ifaceSubClass, 2, 16, QLatin1Char('0'));
            if(protocolStr.isNull())
                protocolStr=QString("0x%1").arg(iface.protocol, 2, 16, QLatin1Char('0'));
            ifaceItem->addChild(new QTreeWidgetItem{QStringList{tr("Subclass"), subclassStr}});
            ifaceItem->addChild(new QTreeWidgetItem{QStringList{tr("Protocol"), protocolStr}});
            ifaceItem->addChild(new QTreeWidgetItem{QStringList{tr("Driver"),
                                                    QString("%1").arg(iface.driver)}});
            if(iface.ifaceClass==CLASS_HID)
            {
                const auto hidReportDescriptorsItem=new QTreeWidgetItem{QStringList{tr("HID report descriptors")}};
                ifaceItem->addChild(hidReportDescriptorsItem);
                for(const auto& desc : iface.hidReportDescriptors)
                {
                    const auto descItem=new QTreeWidgetItem{QStringList{formatBytes(desc, wantWrapRawDumps_)}};
                    if(wantWrapRawDumps_)
                        descItem->setFont(0, monoFont);
                    hidReportDescriptorsItem->addChild(descItem);
                    parseHIDReportDescriptor(descItem, desc);
                }

            }
            if(iface.numEPs!=0 || !iface.endpoints.empty())
            {
                const auto endpointsItem=new QTreeWidgetItem{QStringList{
                                            iface.endpoints.empty() ? tr("Endpoints (%1)").arg(iface.numEPs) : tr("Endpoints")}};
                ifaceItem->addChild(endpointsItem);
                endpointsItem->setExpanded(true);
                for(const auto& ep : iface.endpoints)
                {
                    const auto epItem=new QTreeWidgetItem{QStringList{tr("Endpoint 0x%1").arg(ep.address, 2, 16, QLatin1Char('0'))}};
                    endpointsItem->addChild(epItem);
                    epItem->setExpanded(true);
                    epItem->addChild(new QTreeWidgetItem{QStringList{tr("Direction"),
                                                                     ep.direction=="in"  ? tr("In")   :
                                                                     ep.direction=="out" ? tr("Out")  :
                                                                     ep.direction=="both"? tr("Both") :
                                                                     ep.direction}});
                    const auto attribItem=new QTreeWidgetItem{QStringList{tr("Attributes"),
                                                                          QString("0x%1").arg(ep.attributes, 2, 16, QLatin1Char('0'))}};
                    epItem->addChild(attribItem);
                    {
                        attribItem->addChild(new QTreeWidgetItem{QStringList{tr("Transfer type"),
                                                                             ep.type=="Control"   ? tr("Control")     :
                                                                             ep.type=="Isoc"      ? tr("Isochronous") :
                                                                             ep.type=="Bulk"      ? tr("Bulk")        :
                                                                             ep.type=="Interrupt" ? tr("Interrupt")   :
                                                                             ep.type}});
                        if(ep.type=="Isoc")
                        {
                            attribItem->addChild(new QTreeWidgetItem{QStringList{tr("Synchronization type"),
                                                                     (ep.attributes&0xc)==0x0 ? tr("No synchronization") :
                                                                     (ep.attributes&0xc)==0x4 ? tr("Asynchronous")       :
                                                                     (ep.attributes&0xc)==0x8 ? tr("Adaptive")           :
                                                                     (ep.attributes&0xc)==0xc ? tr("Synchronous")        :
                                                                     "(my bug, please report)"}});
                            attribItem->addChild(new QTreeWidgetItem{QStringList{tr("Usage type"),
                                                                     (ep.attributes&0x30)==0x00 ? tr("Data")                    :
                                                                     (ep.attributes&0x30)==0x10 ? tr("Feedback")                :
                                                                     (ep.attributes&0x30)==0x20 ? tr("Explicit feedback data")  :
                                                                     (ep.attributes&0x30)==0x30 ? tr("(Reserved value)")        :
                                                                     "(my bug, please report)"}});
                        }
                    }
                    epItem->addChild(new QTreeWidgetItem{QStringList{tr("Max packet size"), QString::number(ep.maxPacketSize)}});
                    epItem->addChild(new QTreeWidgetItem{QStringList{tr("Interval between transfers"),
                                            QString(u8"%1\u202f%2").arg(ep.intervalBetweenTransfers).arg(ep.intervalUnit)}});
                }
            }
        }
    }

    const auto rawDescriptorsItem=new QTreeWidgetItem{QStringList{tr("Raw descriptors")}};
    addTopLevelItem(rawDescriptorsItem);
    for(const auto& desc : device_->rawDescriptors)
    {
        QString name;
        if(desc.size()<2)
            name=tr("(broken)");
        else
            name=::name(static_cast<DescriptorType>(desc[1]));
        const auto descItem=new QTreeWidgetItem{QStringList{name, formatBytes(desc, wantWrapRawDumps_)}};
        descItem->setFont(1, monoFont);
        rawDescriptorsItem->addChild(descItem);
    }

	QTreeWidgetItem* extToolOutputItem=nullptr;
    if(wantExtToolOutput_)
    {
        extToolOutputItem=getExternalDescription(*device_);
        addTopLevelItem(extToolOutputItem);
    }

    for(int i=0; i<topLevelItemCount(); ++i)
        setFirstColumnSpannedForAllSingleColumnItems(topLevelItem(i));

    resizeColumnToContents(0);
}

void PropertiesWidget::setShowExtToolOutput(const bool enable)
{
    wantExtToolOutput_=enable;
    showDevice(device_);
}

void PropertiesWidget::setWrapRawDumps(const bool enable)
{
    wantWrapRawDumps_=enable;
    showDevice(device_);
}
