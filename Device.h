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

    QString name;
    Device* parent=nullptr;
    std::vector<Device*> children;

    explicit Device(std::vector<std::string> const& descriptionLines);
};
