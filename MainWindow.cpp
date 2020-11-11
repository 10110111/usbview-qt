#include "MainWindow.h"
#include <QScreen>
#include <QMenuBar>
#include <QSplitter>
#include <QFontMetrics>
#include <QApplication>
#include "PropertiesWidget.h"
#include "DeviceTreeWidget.h"
#include "DeviceTree.h"

void MainWindow::createMenuBar()
{
    const auto menuBar = this->menuBar();
    const auto fileMenu = menuBar->addMenu(QObject::tr("&File"));
    const auto exitAction = fileMenu->addAction(QObject::tr("E&xit"));
    QObject::connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    const auto view = menuBar->addMenu(QObject::tr("&View"));

    {
        const auto action = view->addAction(QObject::tr("&Refresh"));
        action->setShortcut(QKeySequence::Refresh);
        QObject::connect(action, &QAction::triggered, this, &MainWindow::refresh);
        action->trigger();
    }
    {
        const auto action = view->addAction(QObject::tr("Show &ports in hubs"));
        QObject::connect(action, &QAction::toggled, treeWidget_, &DeviceTreeWidget::setShowPorts);
        action->setCheckable(true);
        action->setChecked(false);
    }
    {
        const auto action = view->addAction(QObject::tr("Show &VendorId:ProductId in device tree"));
        QObject::connect(action, &QAction::toggled, treeWidget_, &DeviceTreeWidget::setShowVendorProductIds);
        action->setCheckable(true);
        action->setChecked(false);
    }

    menuBar->show();
}

void MainWindow::refresh()
{
    treeWidget_->setTree(readDeviceTree());
}

MainWindow::MainWindow()
    : treeWidget_(new DeviceTreeWidget)
    , propsWidget_(new PropertiesWidget)
    , splitter_(new QSplitter)
{
    setWindowTitle(QObject::tr("USB Device Tree"));
    splitter_->addWidget(treeWidget_);
    splitter_->addWidget(propsWidget_);
    setCentralWidget(splitter_);

    QObject::connect(treeWidget_, &DeviceTreeWidget::deviceSelected, propsWidget_, &PropertiesWidget::showDevice);
    QObject::connect(treeWidget_, &DeviceTreeWidget::devicesUnselected, propsWidget_, &PropertiesWidget::clear);

    createMenuBar();

    const auto screenAvailableSize = qApp->primaryScreen()->availableSize();
    treeWidget_->ensurePolished();
    const QFontMetrics fm(treeWidget_->font());
    const auto fontHeight=fm.height();
    const auto fontWidth=fm.averageCharWidth();
    resize(std::min(screenAvailableSize.width(), fontWidth*120),
           std::min(screenAvailableSize.height(), fontHeight*50));
}
