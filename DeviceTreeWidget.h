#pragma once

#include <memory>
#include <QTreeWidget>
#include "Device.h"

class DeviceTreeWidget : public QTreeWidget
{
    Q_OBJECT

    bool wantPortsShown_=false;
    bool wantVenProdIdsShown_=false;
    std::vector<std::unique_ptr<Device>> deviceTree_;

    void insertChildren(QTreeWidgetItem* item, Device const* dev);
    QString formatName(Device const& dev) const;
    void onSelectionChanged();
    void updateDeviceTree();

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
