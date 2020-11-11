#include "PropertiesWidget.h"
#include "Device.h"

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
    };
    const auto typeNameIt=types.find(type);
    if(typeNameIt==types.end())
        return QObject::tr("unknown type 0x%1").arg(static_cast<unsigned>(type), 2, 16, QChar('0'));
    return typeNameIt->second;
}

QString formatBytes(std::vector<uint8_t> const& data)
{
    QString str;
    for(const auto byte : data)
        str += QString("%1 ").arg(+byte, 2, 16, QChar('0'));
    return str.trimmed();
}

}

PropertiesWidget::PropertiesWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabels({"Property", "Value"});
}

void PropertiesWidget::showDevice(Device const* dev)
{
    clear();
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Device path", dev->devicePath}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Vendor Id", QString("0x%1").arg(dev->vendorId, 4, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Product Id", QString("0x%1").arg(dev->productId, 4, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Revision", dev->revision}});
    if(!dev->manufacturer.isNull())
        addTopLevelItem(new QTreeWidgetItem{QStringList{"Manufacturer", dev->manufacturer}});
    if(!dev->product.isNull())
        addTopLevelItem(new QTreeWidgetItem{QStringList{"Product", dev->product}});
    if(!dev->serialNum.isNull())
        addTopLevelItem(new QTreeWidgetItem{QStringList{"Serial number", dev->serialNum}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Bus", QString::number(dev->busNum)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Address", QString::number(dev->devNum)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Port", QString::number(dev->port)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"USB version", dev->usbVersion}});
    {
        QString speed;
        const auto speedX10=dev->speed*10;
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
            if(dev->speed<1000)
                speed=tr(u8"%1\u202fMb/s").arg(dev->speed);
            else
                speed=tr(u8"%1\u202fGb/s").arg(dev->speed/1000);
        }
        addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Speed"), speed}});
    }
    addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Device class"), QString("0x%1 (%2)").arg(dev->devClass, 2, 16, QLatin1Char('0'))
                                                                                      .arg(dev->devClassStr)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Device subclass"), QString("0x%1").arg(dev->devSubClass, 2, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Device protocol"), QString("0x%1").arg(dev->devProtocol, 2, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{tr("Max default endpoint packet size"), QString::number(dev->maxPacketSize)}});
    const auto configsItem=new QTreeWidgetItem{QStringList{tr("Configurations")}};
    addTopLevelItem(configsItem);
	configsItem->setFirstColumnSpanned(true);
    for(const auto& config : dev->configs)
    {
        const auto configItem=new QTreeWidgetItem{QStringList{tr("Configuration %1").arg(config.configNum)}};
        configsItem->addChild(configItem);
		configItem->setFirstColumnSpanned(true);
        const auto attribItem=new QTreeWidgetItem{QStringList{tr("Attributes"),
                                                              QString("0x%1").arg(config.attributes, 2, 16, QLatin1Char('0'))}};
        configItem->addChild(attribItem);
        {
            const auto poweringItem=new QTreeWidgetItem{QStringList{config.attributes&1<<6 ? tr("Self-powered") : tr("Bus-powered")}};
            attribItem->addChild(poweringItem);
            poweringItem->setFirstColumnSpanned(true);
            const auto remoteWakeupItem=new QTreeWidgetItem{QStringList{config.attributes&1<<5 ? tr("Supports remote wakeup") :
                                                                                                 tr("No remote wakeup support")}};
            attribItem->addChild(remoteWakeupItem);
            remoteWakeupItem->setFirstColumnSpanned(true);
        }
        configItem->addChild(new QTreeWidgetItem{QStringList{tr("Max power needed"),
                                                 QString(tr(u8"%1\u202fmA")).arg(config.maxPowerMilliAmp)}});
        const auto ifacesItem=new QTreeWidgetItem{QStringList{tr("Interfaces")}};
        configItem->addChild(ifacesItem);
		ifacesItem->setFirstColumnSpanned(true);
        for(const auto& iface : config.interfaces)
        {
            const auto ifaceItem=new QTreeWidgetItem{QStringList{tr("Interface %1").arg(iface.ifaceNum)}};
            ifacesItem->addChild(ifaceItem);
			ifaceItem->setFirstColumnSpanned(true);
//            ifaceItem->addChild(new QTreeWidgetItem{QStringList{tr("Active alternate setting"), iface.activeAltSetting ? tr("yes") : tr("no")}});
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
            if(iface.numEPs!=0 || !iface.endpoints.empty())
            {
                const auto endpointsItem=new QTreeWidgetItem{QStringList{
                                            iface.endpoints.empty() ? tr("Endpoints (%1)").arg(iface.numEPs) : tr("Endpoints")}};
                ifaceItem->addChild(endpointsItem);
                endpointsItem->setFirstColumnSpanned(true);
                for(const auto& ep : iface.endpoints)
                {
                    const auto epItem=new QTreeWidgetItem{QStringList{tr("Endpoint 0x%1").arg(ep.address, 2, 16, QLatin1Char('0'))}};
                    endpointsItem->addChild(epItem);
                    epItem->setFirstColumnSpanned(true);
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
    rawDescriptorsItem->setFirstColumnSpanned(true);
    for(const auto& desc : dev->rawDescriptors)
    {
        QString name;
        if(desc.size()<2)
            name=tr("(broken)");
        else
            name=::name(static_cast<DescriptorType>(desc[1]));
        const auto descItem=new QTreeWidgetItem{QStringList{name, formatBytes(desc)}};
        rawDescriptorsItem->addChild(descItem);
    }


    expandAll();
    resizeColumnToContents(0);
}
