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
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Manufacturer", dev->manufacturer}});
    addTopLevelItem(new QTreeWidgetItem{QStringList{"Product", dev->product}});
}
