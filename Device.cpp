#include "Device.h"
#include <iostream>
#include <stdexcept>
#include <string_view>

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
            char rev[11]={};
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
            devClassStr=cls;
            devClassStr=devClassStr.trimmed();
            break;
        }
    }

    name = product.isEmpty() ? devClassStr : product;
}
