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
#include "util.hpp"

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

unsigned getUInt(fs::path const& filePath, const int base)
{
    std::ifstream file(filePath);
    switch(base)
    {
    case 16: file >> std::hex; break;
    case 10: file >> std::dec; break;
    case 8:  file >> std::oct; break;
    default: assert(!"Invalid base");
    }
    unsigned value;
    file >> value;
    if(!file)
        throw std::invalid_argument("Failed to parse integer from file \""+filePath.string()+"\"");
    if(file.get()!='\n')
        throw std::invalid_argument("Trailing characters after integer in file \""+filePath.string()+"\"");
    return value;
}

unsigned getUInt(std::string const& str, const int base)
{
    assert(base==8 || base==10 || base==16);
    std::size_t pos=0;
    const auto value=std::stoul(str, &pos, base);
    if(pos!=str.size())
        throw std::invalid_argument("Trailing characters after integer \""+str+"\"");
    return value;
}

double getDouble(fs::path const& filePath)
{
    std::ifstream file(filePath);
    double value;
    file >> value;
    if(!file)
        throw std::invalid_argument("Failed to parse floating-point number from file \""+filePath.string()+"\"");
    if(file.get()!='\n')
        throw std::invalid_argument("Trailing characters after floating-point number in file \""+filePath.string()+"\"");
    return value;
}

double getDouble(std::string const& str)
{
    std::size_t pos=0;
    const auto value=std::stod(str, &pos);
    if(pos!=str.size())
        throw std::invalid_argument("Trailing characters after floating-point number \""+str+"\"");
    return value;
}

QString getString(fs::path const& filePath)
{
    char data[4097];
    std::ifstream file(filePath);
    file.read(data, sizeof data);
    if(!file.gcount())
        throw std::invalid_argument("Failed to read file \""+filePath.string()+"\"");
    auto str=QLatin1String(data, file.gcount());
    if(str.size() && str.back()=='\n')
        str.chop(1);
    return str;
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

    for(const auto& entry : fs::directory_iterator(intPath))
    {
        const auto epPath=entry.path();
        const auto filename=epPath.filename().string();
        if(!startsWith(filename, "ep_") || !matches(filename, "ep_..$"))
            continue;
        auto& ep=iface.endpoints.emplace_back();
        parseEndpoint(epPath, ep);
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

Device::Device(fs::path const& devpath)
{
	// Force '.' as the radix point, which is used by sysfs. We don't care to restore it
	// after parsing, since we never display any numbers that should be localized.
	std::setlocale(LC_NUMERIC,"C");

    const auto& path=devpath.string();

    devicePath=QString::fromStdString(path);

    busNum=getUInt(devpath/"busnum", 10);
    devNum=getUInt(devpath/"devnum", 10);

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

    parseConfigs(devpath);

    readBinaryDescriptors(devpath);

    const auto busNumStr=std::to_string(busNum);
    for(const auto& entry : fs::directory_iterator(devpath))
    {
        const auto subDevPath=entry.path();
        const auto filename=subDevPath.filename().string();
        if(!startsWith(filename, busNumStr) || !matches(filename, busNumStr+"-[0-9]+(?:\\.[0-9]+)*$"))
            continue;
        children.emplace_back(std::make_unique<Device>(subDevPath));
    }

    name = product.isEmpty() ? devClassStr : product;
}
