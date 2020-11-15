#pragma once

#include <stdint.h>
#include <unordered_map>
#include <QString>

class USBIDS
{
    std::unordered_map<uint16_t, QString> vendors;
    std::unordered_map<uint32_t, QString> products;
    bool tryParse(QString const& path);
public:
    USBIDS();
    bool parse(QString const& path);
    QString vendor(uint16_t vendorId) const;
    QString product(uint16_t vendorId, uint16_t productId) const;
};
