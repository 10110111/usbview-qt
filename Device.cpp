#include "Device.h"
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <QObject>

bool startsWith(std::string_view line, std::string_view beginning)
{
    if(line.size() < beginning.size()) return false;
    return std::string_view(line.data(), beginning.size()) == beginning;
}

Device::Device(std::vector<std::string> const& descriptionLines)
{
    for(const auto& line : descriptionLines)
    {
        if(line.size() < 2)
            throw std::invalid_argument("Too short device description line: \""+line+"\"");
        if(line[1] != ':')
            throw std::invalid_argument("Missing colon in device description line: \""+line+"\"");
        switch(line[0])
        {
        case 'P':
        {
            char rev[11];
            if(sscanf(line.c_str(), "P:  Vendor=%x ProdID=%x Rev=%10s\n", &vendorId, &productId, rev)!=3)
                throw std::invalid_argument("Failed to parse line \""+line+"\"");
            revision=rev;
            break;
        }
        case 'S':
            if(startsWith(line, "S:  Manufacturer="))
            {
                manufacturer=line.data()+sizeof("S:  Manufacturer=")-1;
                manufacturer.chop(1); // '\n'
            }
            else if(startsWith(line, "S:  Product="))
            {
                product=line.data()+sizeof("S:  Product=")-1;
                product.chop(1); // '\n'
            }
            else if(startsWith(line, "S:  SerialNumber="))
            {
                serialNum=line.data()+sizeof("S:  SerialNumber=")-1;
                serialNum.chop(1); // '\n'
            }
            break;
        case 'T':
            if(sscanf(line.c_str(), "T:  Bus=%u Lev=%u Prnt=%u Port=%u Cnt=%u Dev#=%u Spd=%lf MxCh=%u\n",
                      &busNum, &level, &parentDevNum, &port, &numDevsAtThisLevel, &devNum, &speed, &maxChildren) != 8)
                throw std::invalid_argument("Failed to parse line \""+line+"\"");
            break;
        case 'D':
            char ver[11];
            char cls[31];
            if(const auto ndone=sscanf(line.c_str(), "D:  Ver=%10s Cls=%x(%30[^)]) Sub=%x Prot=%x MxPS=%u #Cfgs=%u\n",
                      ver, &devClass, cls, &devSubClass, &devProtocol, &maxPacketSize, &numConfigs); ndone != 7)
                throw std::invalid_argument("Failed to parse line \""+line+"\": "+std::to_string(ndone));
            usbVersion=ver;
            devClassStr=QString(cls).trimmed();
            break;
        case 'C':
        {
            Config conf;
            char activeConfChar;
            if(sscanf(line.c_str(), "C:%c #Ifs=%u Cfg#=%u Atr=%x MxPwr=%dmA",
                      &activeConfChar, &conf.numInterfaces, &conf.configNum, &conf.attributes, &conf.maxPowerMilliAmp) != 5)
                throw std::invalid_argument("Failed to parse line \""+line+"\"");

            switch(activeConfChar)
            {
            case ' ': conf.active=false; break;
            case '*': conf.active=true;  break;
            default: throw std::invalid_argument(std::string("Bad activity indicator '")+activeConfChar+
                                                 "' in configuration description \""+line+"\"");
            }
            configs.emplace_back(std::move(conf));
            break;
        }
        case 'I':
        {
            Interface iface;
            char activeAltSettingChar, cls[31], driver[31];
            if(sscanf(line.c_str(), "I:%c If#=%u Alt=%u #EPs=%u Cls=%x(%30[^)]) Sub=%x Prot=%x Driver=%30[^\n]",
                      &activeAltSettingChar, &iface.ifaceNum, &iface.altSettingNum, &iface.numEPs, &iface.ifaceClass,
                      cls, &iface.ifaceSubClass, &iface.protocol, driver) != 9)
                throw std::invalid_argument("Failed to parse line \""+line+"\"");
            if(configs.empty())
                throw std::invalid_argument("Interface description found before any config: \""+line+"\"");

            switch(activeAltSettingChar)
            {
            case ' ': iface.activeAltSetting=false; break;
            case '*': iface.activeAltSetting=true;  break;
            default: throw std::invalid_argument(std::string("Bad activity indicator '")+activeAltSettingChar+
                                                 "' in interface description \""+line+"\"");
            }

            iface.ifaceClassStr=QString(cls).trimmed();
            iface.driver=QString(driver).trimmed();

            configs.back().interfaces.emplace_back(std::move(iface));
            break;
        }
        case 'E':
        {
            Endpoint ep;
            char inOutTypeChar, type[31], unit[11];
            if(sscanf(line.c_str(), "E:  Ad=%x(%30[^)]) Atr=%x(%30[^)]) MxPS=%u Ivl=%u%10s\n",
                      &ep.address, &inOutTypeChar, &ep.attributes, type, &ep.maxPacketSize, &ep.intervalBetweenTransfers, unit) != 7)
                throw std::invalid_argument("Failed to parse line \""+line+"\"");

            switch(inOutTypeChar)
            {
            case 'I': ep.isOut=false; break;
            case 'O': ep.isOut=true;  break;
            default: throw std::invalid_argument(std::string("Bad In/Out indicator '")+inOutTypeChar+"' in endpoint description \""+line+"\"");
            }
            ep.type=QString(type).trimmed();
            ep.intervalUnit=QString(unit).trimmed();

            if(configs.empty())
                throw std::invalid_argument("Endpoint description found before any config: \""+line+"\"");
            auto& ifaces=configs.back().interfaces;
            if(ifaces.empty())
                throw std::invalid_argument("Endpoint description found before any interface: \""+line+"\"");

            ifaces.back().endpoints.emplace_back(std::move(ep));
            break;
        }
        }
    }

    name = product.isEmpty() ? devClassStr : product;
}

bool Device::valid()
{
    if(configs.size() != numConfigs)
    {
        reasonIfInvalid=QObject::tr("Number of configs in the description, %1, is different from number of actual configs read, %2")
                                    .arg(numConfigs).arg(configs.size());
        return false;
    }
    for(const auto& config : configs)
    {
        if(config.interfaces.size() != config.numInterfaces)
        {
            reasonIfInvalid=QObject::tr("Number of interfaces in the description, %1, is different from number of actual interfaces read, %2")
                                        .arg(config.numInterfaces).arg(config.interfaces.size());
            return false;
        }
//        for(const auto& iface : config.interfaces)
//        {
//            if(iface.endpoints.size() != iface.numEPs)
//            {
//                reasonIfInvalid=QObject::tr("Number of endpoints in the description, %1, is different from number of actual endpoints read, %2")
//                                            .arg(iface.numEPs).arg(iface.endpoints.size());
//                return false;
//            }
//        }
    }
    return true;
}
