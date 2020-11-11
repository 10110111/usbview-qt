#include "Device.h"
#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <QApplication>
#include "DeviceTreeWidget.h"
#include "PropertiesWidget.h"
#include "MainWindow.h"
#include "util.hpp"

int main(int argc, char** argv)
try
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
catch(std::exception const& ex)
{
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
}
