#include "PropertiesWidget.h"
#include "Device.h"

PropertiesWidget::PropertiesWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabels({"Property", "Value"});
}

void PropertiesWidget::showDevice(Device const* dev)
{
    clear();
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Vendor Id", QString("%1").arg(dev->vendorId, 4, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Product Id", QString("%1").arg(dev->productId, 4, 16, QLatin1Char('0'))}});
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
        addTopLevelItem(new QTreeWidgetItem{QStringList{"Speed", speed}});
    }
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Device class", QString("%1 (%2)").arg(dev->devClass, 2, 16, QLatin1Char('0'))
                                                                                      .arg(dev->devClassStr)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Device subclass", QString("%1").arg(dev->devSubClass, 2, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Device protocol", QString("%1").arg(dev->devProtocol, 2, 16, QLatin1Char('0'))}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Max default endpoint packet size", QString::number(dev->maxPacketSize)}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Number of configurations", QString::number(dev->numConfigs)}});
}
