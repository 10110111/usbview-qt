#include "Device.h"
#include <stdio.h>
#include <memory>
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

std::vector<std::unique_ptr<Device>> buildTree(std::vector<std::unique_ptr<Device>>&& devices)
{
    std::vector<Device*> unmovableDeviceList;
    for(const auto& dev : devices)
        unmovableDeviceList.emplace_back(dev.get());
    for(auto&& dev : devices)
    {
        for(auto& potentialParent : unmovableDeviceList)
        {
            if(potentialParent->devNum == dev->parentDevNum && potentialParent->busNum == dev->busNum)
            {
                dev->parent=potentialParent;
                potentialParent->children.emplace_back(std::move(dev));
                break;
            }
        }
    }
    std::vector<std::unique_ptr<Device>> rootChildren;
    for(auto&& dev : devices)
    {
        if(!dev) continue;
        assert(!dev->parent);
        rootChildren.emplace_back(std::move(dev));
    }
    return rootChildren;
}

std::vector<std::unique_ptr<Device>> readDeviceTree()
{
    const auto stream=popen("usb-devices", "r");
    if(!stream)
    {
        std::cerr << "Failed to run usb-devices\n";
        return {};
    }
    std::vector<std::unique_ptr<Device>> devices;
    std::vector<std::string> description;
    char buf[4096];
    while(true)
    {
        const bool needToStop = !fgets(buf, sizeof buf - 1, stream);
        if(buf[0]=='\n' || needToStop)
        {
            if(!description.empty())
                devices.emplace_back(std::make_unique<Device>(description));
            description.clear();
            if(needToStop) break;
            continue;
        }
        description.emplace_back(buf);
        if(needToStop) break;
    }
    pclose(stream);

    for(auto&& device : devices)
    {
        if(!device->valid())
        {
            std::cerr << "Device " << std::hex << device->vendorId << ":" << device->productId << std::dec
                      << " has invalid/incomplete description: " << device->reasonIfInvalid.toStdString() << "\n";
            return {};
        }
    }

    return buildTree(std::move(devices));
}

void createMenuBar(QMainWindow& mainWindow, DeviceTreeWidget*const treeWidget)
{
    const auto menuBar = mainWindow.menuBar();
    const auto fileMenu = menuBar->addMenu(QObject::tr("&File"));
    const auto exitAction = fileMenu->addAction(QObject::tr("E&xit"));
    QObject::connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    const auto view = menuBar->addMenu(QObject::tr("&View"));

    {
        const auto action = view->addAction(QObject::tr("&Refresh"));
        action->setShortcut(QKeySequence::Refresh);
        QObject::connect(action, &QAction::triggered, [treeWidget]{
                         treeWidget->setTree(readDeviceTree()); });
        action->trigger();
    }
    {
        const auto action = view->addAction(QObject::tr("Show &ports in hubs"));
        QObject::connect(action, &QAction::toggled, treeWidget, &DeviceTreeWidget::setShowPorts);
        action->setCheckable(true);
        action->setChecked(false);
    }
    {
        const auto action = view->addAction(QObject::tr("Show &VendorId:ProductId in device tree"));
        QObject::connect(action, &QAction::toggled, treeWidget, &DeviceTreeWidget::setShowVendorProductIds);
        action->setCheckable(true);
        action->setChecked(false);
    }

    menuBar->show();
}

int main(int argc, char** argv)
try
{
    QApplication app(argc, argv);

    const auto treeWidget=new DeviceTreeWidget;
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
