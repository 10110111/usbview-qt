#pragma once

#include <stdint.h>
#include <memory>
#include <QTreeWidget>
#include "Device.h"

class DeviceTreeWidget : public QTreeWidget
{
    Q_OBJECT

    bool wantPortsShown_=false;
    bool wantVenProdIdsShown_=false;
    static constexpr uint64_t INVALID_UNIQUE_DEVICE_ADDRESS=-1;
    uint64_t currentSelectionUniqueAddress_=INVALID_UNIQUE_DEVICE_ADDRESS;
    std::vector<std::unique_ptr<Device>> deviceTree_;

    void insertChildren(QTreeWidgetItem* item, Device const* dev);
    QString formatName(Device const& dev) const;
    void onSelectionChanged();
    void updateDeviceTree();
    void onItemSelectionChanged();

public:
    DeviceTreeWidget(QWidget* parent=nullptr);
    void setTree(std::vector<std::unique_ptr<Device>>&& tree);
    void setShowPorts(bool enable);
    void setShowVendorProductIds(bool enable);
    QSize sizeHint() const override;

signals:
    void deviceSelected(Device*);
    void devicesUnselected();
    void treeUpdated();
};
