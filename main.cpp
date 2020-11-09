#include "Device.h"
#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <QScreen>
#include <QMenuBar>
#include <QSplitter>
#include <QMainWindow>
#include <QFontMetrics>
#include <QApplication>
#include "DeviceTreeWidget.h"
#include "PropertiesWidget.h"

std::vector<Device*> buildTree(std::vector<Device>& devices)
{
    for(auto& dev : devices)
    {
        for(auto& potentialParent : devices)
        {
            if(potentialParent.devNum != dev.parentDevNum || potentialParent.busNum != dev.busNum)
                continue;
            dev.parent=&potentialParent;
            potentialParent.children.emplace_back(&dev);
        }
    }
    std::vector<Device*> rootChildren;
    for(auto& dev : devices)
        if(!dev.parent)
            rootChildren.emplace_back(&dev);
    return rootChildren;
}

void createMenuBar(QMainWindow& mainWindow, DeviceTreeWidget*const treeWidget)
{
    const auto menuBar = mainWindow.menuBar();
    const auto fileMenu = menuBar->addMenu(QObject::tr("&File"));
    const auto exitAction = fileMenu->addAction(QObject::tr("E&xit"));
    QObject::connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    const auto view = menuBar->addMenu(QObject::tr("&View"));

    {
        const auto showPortsAction = view->addAction(QObject::tr("Show &ports in hubs"));
        showPortsAction->setCheckable(true);
        showPortsAction->setChecked(true);
        QObject::connect(showPortsAction, &QAction::toggled, treeWidget, &DeviceTreeWidget::setShowPorts);
    }

    {
        const auto showVenProdIdsAction = view->addAction(QObject::tr("Show &VendorId:ProductId in device tree"));
        showVenProdIdsAction->setCheckable(true);
        showVenProdIdsAction->setChecked(false);
        QObject::connect(showVenProdIdsAction, &QAction::toggled, treeWidget, &DeviceTreeWidget::setShowVendorProductIds);
    }

    menuBar->show();
}

int main(int argc, char** argv)
try
{
    const auto stream=popen("usb-devices", "r");
    if(!stream)
    {
        std::cerr << "Failed to run usb-devices\n";
        return 1;
    }
    std::vector<Device> devices;
    std::vector<std::string> description;
    char buf[4096];
    while(true)
    {
        const bool needToStop = !fgets(buf, sizeof buf - 1, stream);
        if(buf[0]=='\n' || needToStop)
        {
            if(!description.empty())
                devices.emplace_back(Device(description));
            description.clear();
            if(needToStop) break;
            continue;
        }
        description.emplace_back(buf);
        if(needToStop) break;
    }
    pclose(stream);

    for(auto& device : devices)
    {
        if(!device.valid())
        {
            std::cerr << "Device " << std::hex << device.vendorId << ":" << device.productId << std::dec
                      << " has invalid/incomplete description: " << device.reasonIfInvalid.toStdString() << "\n";
            return 1;
        }
    }

    QApplication app(argc, argv);

    const auto topLevel=buildTree(devices);
    const auto treeWidget=new DeviceTreeWidget;
    treeWidget->setTree(topLevel);
    const auto propsWidget=new PropertiesWidget;
    const auto centralWidget=new QSplitter;
    centralWidget->addWidget(treeWidget);
    centralWidget->addWidget(propsWidget);
    QMainWindow mainWindow;
    mainWindow.setWindowTitle(QObject::tr("USB Device Tree"));
    mainWindow.setCentralWidget(centralWidget);
    {
        const auto screenAvailableSize = app.primaryScreen()->availableSize();
        treeWidget->ensurePolished();
        const QFontMetrics fm(treeWidget->font());
        const auto fontHeight=fm.height();
        const auto fontWidth=fm.averageCharWidth();
        mainWindow.resize(std::min(screenAvailableSize.width(), fontWidth*120),
                          std::min(screenAvailableSize.height(), fontHeight*50));
    }
    mainWindow.show();
    createMenuBar(mainWindow, treeWidget);

    QObject::connect(treeWidget, &DeviceTreeWidget::deviceSelected, propsWidget, &PropertiesWidget::showDevice);
    QObject::connect(treeWidget, &DeviceTreeWidget::devicesUnselected, propsWidget, &PropertiesWidget::clear);

    return app.exec();
}
catch(std::exception const& ex)
{
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
}
