#pragma once

#include <vector>
#include <string>
#include <QString>

struct Device
{
	unsigned vendorId;
	unsigned productId;
    QString revision;

    QString manufacturer;
    QString product;
    QString serialNum;

	unsigned busNum;
	unsigned level;
	unsigned parentDevNum;
	unsigned port;
	unsigned numDevsAtThisLevel;
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
        bool isOut;
        unsigned address;
        unsigned attributes;
        QString type;
        unsigned maxPacketSize;
        unsigned intervalBetweenTransfers;
        QString intervalUnit;
    };
    struct Interface
    {
        bool activeAltSetting;
        unsigned ifaceNum;
        unsigned altSettingNum;
        unsigned numEPs;
        unsigned ifaceClass;
        QString ifaceClassStr;
        unsigned ifaceSubClass;
        unsigned protocol;
        QString driver;
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

    std::vector<Config> configs;

    QString name;
    Device* parent=nullptr;
    std::vector<Device*> children;

    explicit Device(std::vector<std::string> const& descriptionLines);
    bool valid();
    QString reasonIfInvalid;
};
