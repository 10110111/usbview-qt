#pragma once

#include <vector>
#include <stdint.h>
class QTreeWidgetItem;
class QFont;
void parseHIDReportDescriptor(QTreeWidgetItem* root, QFont const& baseFont, std::vector<uint8_t> const& data);
