#include "DeviceTreeWidget.h"
#include <iostream>
#include <QHeaderView>
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

}

DeviceTreeWidget::DeviceTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabel("USB devices");
    header()->setStretchLastSection(false);
    connect(this, &QTreeWidget::itemSelectionChanged, this, &DeviceTreeWidget::onSelectionChanged);
}

QString DeviceTreeWidget::formatName(Device const& dev) const
{
    if(wantVenProdIdsShown_)
        return QString("%1:%2 %3").arg(dev.vendorId, 4, 16, QLatin1Char('0'))
                                  .arg(dev.productId, 4, 16, QLatin1Char('0'))
                                  .arg(dev.name);
    else
        return dev.name;
}

void DeviceTreeWidget::insertChildren(QTreeWidgetItem* item, Device const* dev)
{
    if(wantPortsShown_ && dev->maxChildren)
    {
        // dev is a hub, show all its ports between the hub and the children
        for(unsigned port=1; port <= dev->maxChildren; ++port)
        {
            const auto portItem=new QTreeWidgetItem{QStringList{QString("[Port %1]").arg(port)}};
            item->addChild(portItem);
            const auto child=std::find_if(dev->children.begin(),dev->children.end(),
                                          [port](const auto& child) { return child->port == port; });
            if(child==dev->children.end())
                continue;
            const auto childItem=new QTreeWidgetItem{QStringList{formatName(**child)}};
            setDevice(childItem, child->get());
            portItem->addChild(childItem);
            insertChildren(childItem, child->get());
            if((*child)->uniqueAddress==currentSelectionUniqueAddress_)
                childItem->setSelected(true);
        }
    }
    else
    {
        for(const auto& childDev : dev->children)
        {
            const auto childItem=new QTreeWidgetItem{QStringList{formatName(*childDev)}};
            setDevice(childItem, childDev.get());
            item->addChild(childItem);
            insertChildren(childItem, childDev.get());
            if(childDev->uniqueAddress==currentSelectionUniqueAddress_)
                childItem->setSelected(true);
        }
    }
}

void DeviceTreeWidget::updateDeviceTree()
{
    {
        // Clearing will invoke the signal, which we don't want to actually
        // take effect now, so we simply save and restore the value.
        const auto oldSelectionAddr=currentSelectionUniqueAddress_;
        clear();
        currentSelectionUniqueAddress_=oldSelectionAddr;
    }

    const auto rootItem=new QTreeWidgetItem{QStringList{"Computer"}};
    addTopLevelItem(rootItem);
    for(const auto& dev : deviceTree_)
    {
        const auto topLevelItem=new QTreeWidgetItem{QStringList{formatName(*dev)}};
        setDevice(topLevelItem, dev.get());
        rootItem->addChild(topLevelItem);
        insertChildren(topLevelItem, dev.get());
        if(dev->uniqueAddress==currentSelectionUniqueAddress_)
            topLevelItem->setSelected(true);
    }
    expandAll();

    resizeColumnToContents(0);
    emit treeUpdated();
}

void DeviceTreeWidget::setTree(std::vector<std::unique_ptr<Device>>&& tree)
{
    deviceTree_=std::move(tree);
    updateDeviceTree();
}

QSize DeviceTreeWidget::sizeHint() const
{
    // FIXME: dunno what size exactly we need to avoid scrollbars. Will request a bit larger than the section size.
    return QSize(header()->sectionSize(0)*1.05, QTreeWidget::sizeHint().height());
}

void DeviceTreeWidget::setShowPorts(const bool enable)
{
    wantPortsShown_=enable;
    updateDeviceTree();
}
void DeviceTreeWidget::setShowVendorProductIds(const bool enable)
{
    wantVenProdIdsShown_=enable;
    updateDeviceTree();
}

void DeviceTreeWidget::onSelectionChanged()
{
    const auto selected=selectedItems();
    if(selected.isEmpty())
    {
        currentSelectionUniqueAddress_=INVALID_UNIQUE_DEVICE_ADDRESS;
        emit devicesUnselected();
        return;
    }
    const auto device=getDevice(selected[0]);
    if(!device)
    {
        currentSelectionUniqueAddress_=INVALID_UNIQUE_DEVICE_ADDRESS;
        emit devicesUnselected();
        return;
    }
    currentSelectionUniqueAddress_=device->uniqueAddress;
    emit deviceSelected(device);
}
