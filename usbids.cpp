#include "usbids.h"
#include <iostream>
#include <QFile>
#include <QFileInfo>
#include "util.hpp"

namespace
{

bool isHexDigit(const char c)
{
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

bool areHexDigits(const char* chars, const unsigned count)
{
    for(unsigned i=0; i<count; ++i)
        if(!isHexDigit(chars[i]))
            return false;
    return true;
}

}

USBIDS::USBIDS()
{
    if(tryParse("/usr/share/usb.ids"))
        return;
    if(tryParse("/usr/share/misc/usb.ids"))
        return;
}

bool USBIDS::tryParse(QString const& path)
{
    return QFileInfo(path).exists() && parse(path);
}

bool USBIDS::parse(QString const& path)
{
    vendors.clear();
    products.clear();

    QFile file(path);
    if(!file.open(QFile::ReadOnly))
    {
        std::cerr << "Warning: failed to open \"" << path.toStdString() << "\"\n";
        return false;
    }

    uint16_t prevVendorId=0;
    for(unsigned lineNumber=1;; ++lineNumber)
    {
        const auto line=file.readLine();
        if(line.trimmed().isEmpty())
        {
            if(file.atEnd())
                break;
            if(!file.error())
                continue;
            std::cerr << "Failed to read line " << lineNumber << " of \"" << path.toStdString()
                      << "\": " << file.errorString().toStdString() << "\n";
            return false;
        }
        if(line[0]=='#')
            continue;
        if(line.size() < 7)
        {
            std::cerr << path.toStdString() << ":" << lineNumber << ": too short line:\n" << line.data() << "\n";
            std::cerr << "Stopping processing file\n";
            return false;
        }
        if(areHexDigits(line.data(), 4) && line[4]==' ' && line[5]==' ')
        {
            const auto vendorId=getUInt(std::string(line.left(4).data(), 4), 16);
            vendors[vendorId]=line.mid(6).trimmed();
            prevVendorId=vendorId;
            continue;
        }
        if(line[0]=='\t' && areHexDigits(line.data()+1, 4) && line[5]==' ' && line[6]==' ')
        {
            const auto productId=getUInt(std::string(line.mid(1,4).data(), 4), 16);
            products[uint32_t(prevVendorId)<<16 | productId]=line.mid(7).trimmed();
            continue;
        }
        // Assuming that all vendor and product descriptions are adjacent up to
        // empty lines & comments. Ignoring any following entries.
        break;
    }
    return true;
}

QString USBIDS::vendor(const uint16_t vendorId) const
{
    const auto it=vendors.find(vendorId);
    if(it==vendors.end()) return {};
    return it->second;
}

QString USBIDS::product(const uint16_t vendorId, const uint16_t productId) const
{
    const auto it=products.find(uint32_t(vendorId)<<16 | productId);
    if(it==products.end()) return {};
    return it->second;
}
