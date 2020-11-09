#pragma once

#include <QTreeWidget>

class Device;
class PropertiesWidget : public QTreeWidget
{
public:
    PropertiesWidget(QWidget* parent=nullptr);
    void showDevice(Device const* dev);
};
