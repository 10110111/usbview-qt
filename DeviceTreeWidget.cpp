#include "DeviceTreeWidget.h"
#include <iostream>
#include "Device.h"

namespace
{

enum
{
    DeviceRole=Qt::UserRole+0,
};

void setDevice(QTreeWidgetItem*const item, Device*const dev)
{
    item->setData(0,DeviceRole,reinterpret_cast<unsigned long long>(dev));
}

Device* getDevice(QTreeWidgetItem const*const item)
{
    const auto data=static_cast<uintptr_t>(item->data(0,DeviceRole).toULongLong());
    return reinterpret_cast<Device*>(data);
}

QString formatName(Device const& dev)
{
    return QString("%1:%2 %3").arg(dev.vendorId, 4, 16, QLatin1Char('0'))
                              .arg(dev.productId, 4, 16, QLatin1Char('0'))
                              .arg(dev.name);
}

}

DeviceTreeWidget::DeviceTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabel("USB devices");
    connect(this, &QTreeWidget::itemSelectionChanged, this, &DeviceTreeWidget::onSelectionChanged);
}

void DeviceTreeWidget::insertChildren(QTreeWidgetItem* item, Device const* dev)
{
    if(wantPortsShown_ && dev->maxChildren)
    {
        // dev is a hub, show all its ports between the hub and the children
        for(unsigned port=0; port < dev->maxChildren; ++port)
        {
            const auto portItem=new QTreeWidgetItem{QStringList{QString("[Port %1]").arg(port)}};
            item->addChild(portItem);
            const auto child=std::find_if(dev->children.begin(),dev->children.end(),
                                          [port](const auto& child) { return child->port == port; });
            if(child==dev->children.end())
                continue;
            const auto childItem=new QTreeWidgetItem{QStringList{formatName(**child)}};
            setDevice(childItem, *child);
            portItem->addChild(childItem);
            insertChildren(childItem, *child);
        }
    }
    else
    {
        for(const auto childDev : dev->children)
        {
            const auto childItem=new QTreeWidgetItem{QStringList{formatName(*childDev)}};
            setDevice(childItem, childDev);
            item->addChild(childItem);
            insertChildren(childItem, childDev);
        }
    }
}

void DeviceTreeWidget::setTree(std::vector<Device*> const& tree)
{
    deviceTree_=tree;
    clear();
    const auto rootItem=new QTreeWidgetItem{QStringList{"Computer"}};
    addTopLevelItem(rootItem);
    for(const auto dev : tree)
    {
        const auto topLevelItem=new QTreeWidgetItem{QStringList{formatName(*dev)}};
        setDevice(topLevelItem, dev);
        rootItem->addChild(topLevelItem);
        insertChildren(topLevelItem, dev);
    }
    expandItem(rootItem);
}

void DeviceTreeWidget::setShowPorts(const bool enable)
{
    wantPortsShown_=enable;
    setTree(deviceTree_);
}

void DeviceTreeWidget::onSelectionChanged()
{
    const auto selected=selectedItems();
    if(selected.isEmpty())
    {
        emit devicesUnselected();
        return;
    }
    const auto device=getDevice(selected[0]);
    if(!device)
    {
        emit devicesUnselected();
        return;
    }
    emit deviceSelected(device);
}
