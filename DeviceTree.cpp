#include <filesystem>
#include <libudev.h>
#include "DeviceTree.h"
#include "util.hpp"

std::vector<std::unique_ptr<Device>> readDeviceTree()
{
    static const auto udev=udev_new();
    static const auto hwdb = udev ? udev_hwdb_new(udev) : nullptr;

    std::vector<std::unique_ptr<Device>> devices;
    namespace fs=std::filesystem;
    for(const auto& entry : fs::directory_iterator(fs::u8path(u8"/sys/bus/usb/devices/")))
    {
        if(!startsWith(entry.path().filename().string(), "usb"))
            continue;
        devices.emplace_back(std::make_unique<Device>(entry.path(), hwdb));
    }

    std::sort(devices.begin(), devices.end(), [](auto const& d1, auto const& d2)
              {return d1->busNum < d2->busNum;});

    return devices;
}
