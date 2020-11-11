#pragma once

#include <QTreeWidget>

class Device;
class PropertiesWidget : public QTreeWidget
{
    bool wantExtToolOutput_=false;
    Device const* device_=nullptr;

    void updateTree();
public:
    PropertiesWidget(QWidget* parent=nullptr);
    void showDevice(Device const* dev);
    void setShowExtToolOutput(bool enable);
};
