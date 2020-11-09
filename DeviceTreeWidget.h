#pragma once

#include <QTreeWidget>

class Device;
class DeviceTreeWidget : public QTreeWidget
{
    Q_OBJECT

    bool wantPortsShown_=true;
    std::vector<Device*> deviceTree_;

    void insertChildren(QTreeWidgetItem* item, Device const* dev);
    void onSelectionChanged();

public:
    DeviceTreeWidget(QWidget* parent=nullptr);
    void setTree(std::vector<Device*> const& tree);
    void setShowPorts(bool enable);

signals:
    void deviceSelected(Device*);
    void devicesUnselected();
};
