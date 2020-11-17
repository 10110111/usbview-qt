#pragma once

#include <QTreeWidget>

struct Device;
class PropertiesWidget : public QTreeWidget
{
    bool wantExtToolOutput_=false;
    bool wantWrapRawDumps_=false;
    Device const* device_=nullptr;

    void updateTree();
public:
    PropertiesWidget(QWidget* parent=nullptr);
    void showDevice(Device const* dev);
    void setShowExtToolOutput(bool enable);
    void setWrapRawDumps(bool enable);
};
