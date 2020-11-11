#pragma once

#include <QMainWindow>

class DeviceTreeWidget;
class PropertiesWidget;
class QSplitter;
class MainWindow : public QMainWindow
{
    DeviceTreeWidget* treeWidget_;
    PropertiesWidget* propsWidget_;
    QSplitter* splitter_;

    void createMenuBar();
    void onTreeUpdated();
    void refresh();
public:
    MainWindow();
};
