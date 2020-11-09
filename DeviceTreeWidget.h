#pragma once

#include <QTreeWidget>

class Device;
class DeviceTreeWidget : public QTreeWidget
{
    Q_OBJECT

    bool wantPortsShown_=true;
    bool wantVenProdIdsShown_=false;
    std::vector<Device*> deviceTree_;

    void insertChildren(QTreeWidgetItem* item, Device const* dev);
    QString formatName(Device const& dev) const;
    void onSelectionChanged();
    void updateDeviceTree();

public:
    DeviceTreeWidget(QWidget* parent=nullptr);
    void setTree(std::vector<Device*> const& tree);
    void setShowPorts(bool enable);
    void setShowVendorProductIds(bool enable);

signals:
    void deviceSelected(Device*);
    void devicesUnselected();
};
