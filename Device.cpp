#include "Device.h"
#include <map>
#include <regex>
#include <string>
#include <cassert>
#include <clocale>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <QObject>
#include <QFileInfo>
#include "util.hpp"
#include "common.hpp"
#include "usbids.h"
#include <libudev.h>

#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stat.h>
#undef major
#undef minor

namespace fs=std::filesystem;

namespace
{

QString devClassName(const uint8_t classId)
{
    switch(classId)
    {
    case 0x00: return QObject::tr("defined by interface descriptor");
    case 0x01: return QObject::tr("audio");
	case 0x02: return QObject::tr("communications and CDC control");
	case 0x03: return QObject::tr("HID");
	case 0x05: return QObject::tr("physical");
	case 0x06: return QObject::tr("image");
	case 0x07: return QObject::tr("printer");
	case 0x08: return QObject::tr("mass storage");
	case 0x09: return QObject::tr("hub");
	case 0x0a: return QObject::tr("CDC data");
	case 0x0b: return QObject::tr("smart card");
	case 0x0d: return QObject::tr("content security");
	case 0x0e: return QObject::tr("video");
	case 0x0f: return QObject::tr("personal healthcare");
	case 0x10: return QObject::tr("audio/video");
	case 0x11: return QObject::tr("billboard");
	case 0x12: return QObject::tr("USB Type-C bridge");
	case 0xdc: return QObject::tr("diagnostic");
	case 0xe0: return QObject::tr("wireless controller");
	case 0xef: return QObject::tr("misc.");
	case 0xfe: return QObject::tr("app-specific");
	case 0xff: return QObject::tr("vendor-specific");
    default  : return QObject::tr("unknown");
    }
}

QString getDevString(fs::path const& filePath)
{
    // This one may not exist
    if(!fs::exists(filePath))
        return QString();
    return getString(filePath);
}

bool matches(std::string const& str, std::string const& pattern)
{
    return std::regex_match(str, std::regex(pattern));
}

std::vector<QString> findDeviceNodes(const unsigned major, const unsigned minor)
{
    std::vector<QString> nodes;
    for(const auto& entry : fs::recursive_directory_iterator("/dev", fs::directory_options::skip_permission_denied))
    {
        const auto& path=entry.path();
        if(is_symlink(path)) continue;
        if(!is_block_file(path) && !is_character_file(path)) continue;

        struct stat st;
        if(stat(path.string().c_str(), &st)<0)
        {
            perror(("stat("+path.string()+")").c_str());
            continue;
        }
        if(gnu_dev_major(st.st_rdev)!=major || gnu_dev_minor(st.st_rdev)!=minor) continue;

        nodes.emplace_back(QString::fromStdString(canonical(path).string()));
    }
    if(nodes.empty())
        std::cerr << "Warning: couldn't find device node with major=" << major << ", minor=" << minor << "\n";
    return nodes;
}

const char* hwdb_get(udev_hwdb* hwdb, const char* modalias, const char* key)
{
    udev_list_entry* entry;

    udev_list_entry_foreach(entry, udev_hwdb_get_properties_list_entry(hwdb, modalias, 0))
    {
        if(!strcmp(udev_list_entry_get_name(entry), key))
            return udev_list_entry_get_value(entry);
    }

    return nullptr;
}
}

void Device::parseEndpoint(fs::path const& epPath, Endpoint& ep)
{
    // Radices are defined in linux-4.14.157/core/endpoint.c
    ep.address=getUInt(epPath/"bEndpointAddress", 16);
    ep.attributes=getUInt(epPath/"bmAttributes", 16);
    ep.direction=getString(epPath/"direction");
    ep.type=getString(epPath/"type");
    ep.maxPacketSize=getUInt(epPath/"wMaxPacketSize", 16);

    {
        const auto intervalStr=getString(epPath/"interval").toStdString();
        std::size_t pos;
        ep.intervalBetweenTransfers=std::stoul(intervalStr, &pos);
        ep.intervalUnit=QString::fromStdString(intervalStr.substr(pos));
    }
}

void Device::parseInterface(std::filesystem::path const& intPath, Interface& iface)
{
    iface.activeAltSetting=false; //FIXME: where is this info located in /sys/bus/usb/devices?

    iface.sysfsPath=QString::fromStdString(fs::canonical(intPath).string());

    // Radices are defined in linux-4.14.157/core/sysfs.c
    iface.ifaceNum=getUInt(intPath/"bInterfaceNumber", 16);
    iface.altSettingNum=getUInt(intPath/"bAlternateSetting", 10);
    iface.numEPs=getUInt(intPath/"bNumEndpoints", 16);
    iface.ifaceClass=getUInt(intPath/"bInterfaceClass", 16);
    iface.ifaceClassStr=devClassName(iface.ifaceClass);
    iface.ifaceSubClass=getUInt(intPath/"bInterfaceSubClass", 16);
    iface.protocol=getUInt(intPath/"bInterfaceProtocol", 16);

    // If no such link, then there's no associated driver
    const auto driverLink=intPath/"driver";
    if(fs::exists(driverLink) && fs::is_symlink(driverLink))
        iface.driver=QString::fromStdString(fs::read_symlink(driverLink).filename().string());

	// 0003 means BUS_USB; the directory name itself is generated in linux-4.14.157/drivers/hid/hid-core.c
	// with format %04X:%04X:%04X.%04X
    const auto hidDirNamePrefix=QString("0003:%2:%3.").arg(vendorId, 4, 16, QChar('0'))
                                                      .arg(productId, 4, 16, QChar('0'))
                                                      .toUpper();
    for(const auto& entry : fs::directory_iterator(intPath))
    {
        const auto epPath=entry.path();
        const auto filename=epPath.filename().string();
        if(startsWith(filename, "ep_") && matches(filename, "ep_..$"))
        {
            auto& ep=iface.endpoints.emplace_back();
            parseEndpoint(epPath, ep);
        }

        if(startsWith(filename, hidDirNamePrefix.toStdString().c_str()))
            iface.hidReportDescriptors.emplace_back(getFileData(entry.path()/"report_descriptor"));

        if(is_directory(epPath))
        {
            for(const auto& entry : fs::recursive_directory_iterator(epPath, fs::directory_options::skip_permission_denied))
            {
                const auto& path=entry.path();
                if(path.filename().string()!="dev") continue;
                if(!is_regular_file(path)) continue;
                const auto majorMinorStr=getString(path).split(':');
                if(majorMinorStr.size()!=2)
                {
                    std::cerr << "Warning: unexpected contents of " << path << "\n";
                    continue;
                }
                bool ok=false;
                const int major=majorMinorStr[0].toUInt(&ok);
                if(!ok)
                {
                    std::cerr << "Warning: failed to parse major device number " << path << "\n";
                    continue;
                }
                const int minor=majorMinorStr[1].toUInt(&ok);
                if(!ok)
                {
                    std::cerr << "Warning: failed to parse minor device number " << path << "\n";
                    continue;
                }
                auto nodes=findDeviceNodes(major,minor);
                iface.deviceNodes.insert(iface.deviceNodes.end(), std::make_move_iterator(nodes.begin()),
                                                                  std::make_move_iterator(nodes.end()));
            }
            std::sort(iface.deviceNodes.begin(), iface.deviceNodes.end());
        }
    }
}

void Device::parseConfigs(fs::path const& devpath)
{
    auto& config=configs.emplace_back();
    config.active=true; // FIXME: where can inactive configs be found? Answer: all config descriptors in binary form are in devpath/"descriptors" file.
    config.numInterfaces=getUInt(devpath/"bNumInterfaces", 10);
    config.configNum=getUInt(devpath/"bConfigurationValue", 10);
    config.attributes=getUInt(devpath/"bmAttributes", 16);

    const auto maxPower=getString(devpath/"bMaxPower");
    if(!maxPower.endsWith("mA"))
        throw std::invalid_argument("bMaxPower doesn't end in mA in file \""+devpath.string()+"\"");
    config.maxPowerMilliAmp=getDouble(maxPower.chopped(2).toStdString());

    parseEndpoint(devpath/"ep_00", endpoint00);

    const auto busNumStr=std::to_string(busNum);
    for(const auto& entry : fs::directory_iterator(devpath))
    {
        const auto intPath=entry.path();
        const auto filename=intPath.filename().string();
        if(!startsWith(filename, busNumStr) || !matches(filename, busNumStr+"-.*:.\\..*"))
            continue;
        auto& iface=config.interfaces.emplace_back();
        parseInterface(intPath, iface);
    }
    std::sort(config.interfaces.begin(), config.interfaces.end(), [](const auto& if1, const auto& if2)
              { return if1.ifaceNum < if2.ifaceNum; });
}

void Device::readBinaryDescriptors(std::filesystem::path const& devpath)
{
    std::ifstream file(devpath/"descriptors");
    if(!file)
        throw std::invalid_argument("Failed to open descriptors file under \""+devpath.string()+"\"");
    file.seekg(0, std::ios_base::end);
    const auto size=file.tellg();
    file.seekg(0);
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    if(!file)
    {
        if(file.gcount()==0)
            throw std::invalid_argument("Failed to read descriptors file under \""+devpath.string()+"\"");
        data.resize(file.gcount());
    }
    for(unsigned off=0; off<data.size();)
    {
        const unsigned len=data[off];
        if(data.size() < off+len)
            throw std::invalid_argument("Bad descriptor: length at offset "+std::to_string(off)+" overflows data size");
        rawDescriptors.emplace_back(std::vector<uint8_t>(data.data()+off, data.data()+off+len));
        off+=len;
    }
}

Device::Device(fs::path const& devpath, struct udev_hwdb* hwdb, USBIDS const& usbids)
{
	// Force '.' as the radix point, which is used by sysfs. We don't care to restore it
	// after parsing, since we never display any numbers that should be localized.
	std::setlocale(LC_NUMERIC,"C");

    const auto& path=devpath.string();

    sysfsPath=QString::fromStdString(fs::canonical(devpath).string());

    busNum=getUInt(devpath/"busnum", 10);
    devNum=getUInt(devpath/"devnum", 10);

	devicePath=QString("/dev/bus/usb/%1/%2").arg(busNum, 3, 10, QChar('0')).arg(devNum, 3, 10, QChar('0'));
	if(!QFileInfo(devicePath).exists())
		devicePath+=" (error: doesn't actually exist)";

    {
        const auto sep=path.find_last_of(".-");
        if(sep==path.npos)
            port=0;
        else
            port=getUInt(path.substr(sep+1), 10);
    }

    speed=getDouble(devpath/"speed");
    maxChildren=getUInt(devpath/"maxchild", 10);

    usbVersion=getString(devpath/"version").trimmed();
    devClass=getUInt(devpath/"bDeviceClass", 16);
    devClassStr=devClassName(devClass);
    devSubClass=getUInt(devpath/"bDeviceSubClass", 16);
    devProtocol=getUInt(devpath/"bDeviceProtocol", 16);
    maxPacketSize=getUInt(devpath/"bMaxPacketSize0", 10);
    numConfigs=getUInt(devpath/"bNumConfigurations", 10);

    vendorId=getUInt(devpath/"idVendor", 16);
    productId=getUInt(devpath/"idProduct", 16);

    const auto revBCD=getUInt(devpath/"bcdDevice", 16);
    if(revBCD&0xf000)
    {
        revision=QString("%1%2.%3%4").arg(QChar((revBCD>>12)+'0')).arg(QChar((revBCD>>8&0xf)+'0'))
                                     .arg(QChar((revBCD>>4&0xf)+'0')).arg(QChar((revBCD&0xf)+'0'));
    }
    else
    {
        revision=QString("%1.%2%3").arg(QChar((revBCD>>8&0xf)+'0')).arg(QChar((revBCD>>4&0xf)+'0')).arg(QChar((revBCD&0xf)+'0'));
    }

    manufacturer=getDevString(devpath/"manufacturer");
    product=getDevString(devpath/"product");
    serialNum=getDevString(devpath/"serial");

    const auto vendorIdStr=QString("%1").arg(vendorId, 4, 16, QChar('0')).toUpper();
    const auto productIdStr=QString("%1").arg(productId, 4, 16, QChar('0')).toUpper();
    if(const auto s=hwdb_get(hwdb, QString("usb:v%1*").arg(vendorIdStr).toStdString().c_str(), "ID_VENDOR_FROM_DATABASE"))
        hwdbVendorName=s;
    if(const auto s=hwdb_get(hwdb, QString("usb:v%1p%2").arg(vendorIdStr, productIdStr).toStdString().c_str(), "ID_PRODUCT_FROM_DATABASE"))
        hwdbProductName=s;

    usbidsVendorName=usbids.vendor(vendorId);
    usbidsProductName=usbids.product(vendorId, productId);

    parseConfigs(devpath);

    readBinaryDescriptors(devpath);

    const auto busNumStr=std::to_string(busNum);
    for(const auto& entry : fs::directory_iterator(devpath))
    {
        const auto subDevPath=entry.path();
        const auto filename=subDevPath.filename().string();
        if(!startsWith(filename, busNumStr) || !matches(filename, busNumStr+"-[0-9]+(?:\\.[0-9]+)*$"))
            continue;
        children.emplace_back(std::make_unique<Device>(subDevPath, hwdb, usbids));
    }

    {
        // Make up the name
        QString vendorName;
        if(manufacturer=="Generic" || manufacturer.isEmpty())
        {
            if(usbidsVendorName.isEmpty())
                vendorName=hwdbVendorName;
            else
                vendorName=usbidsVendorName;
        }
        else
            vendorName=manufacturer;

        QString productName;
        if(!usbidsProductName.isEmpty())
            productName=usbidsProductName;
        else if(!hwdbProductName.isEmpty())
            productName=hwdbProductName;
        else
            productName=devClassStr;
        // After all above, we still prefer product string, but only if manufacturer isn't "Generic" nor empty.
        if(!manufacturer.isEmpty() && manufacturer!="Generic" && !product.isEmpty())
            productName=product;

        name = QString("%1 %2").arg(vendorName, productName).trimmed();
    }

    uniqueAddress=uint64_t(busNum)<<48 | uint64_t(devNum)<<32 | vendorId<<16 | productId;
}

bool Device::isHub() const
{
    return devClass==CLASS_HUB;
}
