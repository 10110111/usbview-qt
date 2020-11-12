#pragma once

#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <QString>

struct Device
{
    QString sysfsPath;
    QString devicePath;

	unsigned vendorId;
	unsigned productId;
    QString revision;

    QString manufacturer;
    QString product;
    QString serialNum;

	unsigned busNum;
	unsigned port;
	unsigned devNum;
    double speed;
	unsigned maxChildren;

    QString usbVersion;
    unsigned devClass;
    QString devClassStr;
    unsigned devSubClass;
    unsigned devProtocol;
	unsigned maxPacketSize;
	unsigned numConfigs;

    struct Endpoint
    {
        unsigned address;
        unsigned attributes;
        QString direction;
        QString type;
        unsigned maxPacketSize;
        unsigned intervalBetweenTransfers;
        QString intervalUnit;
    };
    struct Interface
    {
        QString sysfsPath;
        std::vector<QString> deviceNodes;
        bool activeAltSetting;
        unsigned ifaceNum;
        unsigned altSettingNum;
        unsigned numEPs;
        unsigned ifaceClass;
        QString ifaceClassStr;
        unsigned ifaceSubClass;
        unsigned protocol;
        QString driver;
        std::vector<std::vector<uint8_t>> hidReportDescriptors;
        std::vector<Endpoint> endpoints;
    };
    struct Config
    {
        bool active;
        unsigned numInterfaces;
        unsigned configNum;
        unsigned attributes;
        unsigned maxPowerMilliAmp;
        std::vector<Interface> interfaces;
    };

    Endpoint endpoint00;
    std::vector<Config> configs;

    std::vector<std::vector<uint8_t>> rawDescriptors;

    QString name;
    std::vector<std::unique_ptr<Device>> children;

    explicit Device(std::filesystem::path const& devpath);
private:
    void parseConfigs(std::filesystem::path const& devpath);
    void parseEndpoint(std::filesystem::path const& devpath, Endpoint& ep);
    void parseInterface(std::filesystem::path const& intPath, Interface& iface);
    void readBinaryDescriptors(std::filesystem::path const& devpath);
};
