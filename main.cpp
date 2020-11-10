#include "Device.h"
#include <stdio.h>
#include <memory>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <QScreen>
#include <QMenuBar>
#include <QSplitter>
#include <QMainWindow>
#include <QFontMetrics>
#include <QApplication>
#include "DeviceTreeWidget.h"
#include "PropertiesWidget.h"
#include "util.hpp"

std::vector<std::unique_ptr<Device>> readDeviceTree()
{
    std::vector<std::unique_ptr<Device>> devices;
    namespace fs=std::filesystem;
    for(const auto& entry : fs::directory_iterator(fs::u8path(u8"/sys/bus/usb/devices/")))
    {
        if(!startsWith(entry.path().filename().string(), "usb"))
            continue;
        devices.emplace_back(std::make_unique<Device>(entry.path()));
    }

    return devices;
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
