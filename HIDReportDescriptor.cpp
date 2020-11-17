#include "HIDReportDescriptor.h"
#include <stack>
#include <queue>
#include <deque>
#include <cassert>
#include <optional>
#include <QFont>
#include <QTreeWidgetItem>
#include "util.hpp"

namespace
{

enum ItemType
{
    IT_MAIN,
    IT_GLOBAL,
    IT_LOCAL,
    IT_RESERVED,
};
enum MainItemTag
{
    MIT_INPUT         =0x8,
    MIT_OUTPUT        =0x9,
    MIT_COLLECTION    =0xA,
    MIT_FEATURE       =0xB,
    MIT_END_COLLECTION=0xC,
};
enum GlobalItemTag
{
    GIT_USAGE_PAGE,
    GIT_LOG_MIN,
    GIT_LOG_MAX,
    GIT_PHYS_MIN,
    GIT_PHYS_MAX,
    GIT_UNIT_EXP,
    GIT_UNIT,
    GIT_REP_SIZE,
    GIT_REP_ID,
    GIT_REP_COUNT,
    GIT_PUSH,
    GIT_POP,
};
enum LocalItemTag
{
    LIT_USAGE,
    LIT_USAGE_MIN,
    LIT_USAGE_MAX,
    LIT_DESIG_IDX,
    LIT_DESIG_MIN,
    LIT_DESIG_MAX,
    LIT_,
    LIT_STRING_IDX,
    LIT_STR_MIN,
    LIT_STR_MAX,
    LIT_DELIM,
};
enum CollectionType
{
    COL_PHYSICAL,
    COL_APPLICATION,
    COL_LOGICAL,
    COL_REPORT,
    COL_NAMED_ARRAY,
    COL_USAGE_SWITCH,
    COL_USAGE_MODIFIER,
};
enum UsagePage
{
    UP_GENERIC_DESKTOP      = 0x01,
    UP_SIM_CONTROLS         = 0x02,
    UP_VR_CONTROLS          = 0x03,
    UP_SPORT_CONTROLS       = 0x04,
    UP_GAME_CONTROLS        = 0x05,
    UP_GENERIC_DEV_CONTROLS = 0x06,
    UP_KEYBOARD_KEYPAD      = 0x07,
    UP_LED                  = 0x08,
    UP_BUTTON               = 0x09,
    UP_ORDINAL              = 0x0A,
    UP_TELEPHONY_DEVICE     = 0x0B,
    UP_CONSUMER             = 0x0C,
    UP_DIGITIZERS           = 0x0D,
    UP_HAPTICS              = 0x0E,
    UP_UNICODE              = 0x10,
    UP_EYE_HEAD_TRACKERS    = 0x12,
    UP_AUX_DISPLAY          = 0x14,
    UP_SENSORS              = 0x20,
    UP_MEDICAL_INSTRUMENT   = 0x40,
    UP_BRAILLE_DISPLAY      = 0x41,
    UP_LIGHTING_ILLUMINATION= 0x59,
    UP_CAMERA_CONTROL       = 0x90,
    UP_GAMING_DEVICE        = 0x92,
    UP_FIDO_ALLIANCE        = 0xF1D0,
};

const std::unordered_map<uint16_t, QString> genericDesktopPage={
    {0x0000, "Undefined"},
    {0x0001, "Pointer"},
    {0x0002, "Mouse"},
// 3 Reserved
    {0x0004, "Joystick"},
    {0x0005, "Gamepad"},
    {0x0006, "Keyboard"},
    {0x0007, "Keypad"},
    {0x0008, "Multi-axis Controller"},
    {0x0009, "Tablet PC System Controls"},
    {0x000a, "Water Cooling Device"},
    {0x000b, "Computer Chassis Device"},
    {0x000c, "Wireless Radio Controls"},
    {0x000d, "Portable Device Control"},
    {0x000e, "System Multi-Axis Controller"},
    {0x000f, "Spatial Controller"},
    {0x0010, "Assistive Control"},
    {0x0011, "Device Dock"},
    {0x0012, "Dockable Device"},
// 13-2f Reserved
    {0x0030, "X"},
    {0x0031, "Y"},
    {0x0032, "Z"},
    {0x0033, "Rx"},
    {0x0034, "Ry"},
    {0x0035, "Rz"},
    {0x0036, "Slider"},
    {0x0037, "Dial"},
    {0x0038, "Wheel"},
    {0x0039, "Hat Switch"},
    {0x003a, "Counted Buffer"},
    {0x003b, "Byte Count"},
    {0x003c, "Motion Wakeup"},
    {0x003d, "Start"},
    {0x003e, "Select"},
// 3f reserved
    {0x0040, "Vx"},
    {0x0041, "Vy"},
    {0x0042, "Vz"},
    {0x0043, "Vbrx"},
    {0x0044, "Vbry"},
    {0x0045, "Vbrz"},
    {0x0046, "Vno"},
    {0x0047, "Feature Notification"},
    {0x0048, "Resolution Multiplier"},
    {0x0049, "Qx"},
    {0x004a, "Qy"},
    {0x004b, "Qz"},
    {0x004c, "Qw"},
// 4d-7f Reserved
    {0x0080, "System Control"},
    {0x0081, "System Power Down"},
    {0x0082, "System Sleep"},
    {0x0083, "System Wake Up"},
// TODO: complete
};

const std::unordered_map<uint16_t, QString> ledPage={
    {0x0000, "Undefined"},
    {0x0001, "Num Lock"},
    {0x0002, "Caps Lock"},
    {0x0003, "Scroll Lock"},
    {0x0004, "Compose"},
    {0x0005, "Kana"},
    {0x0006, "Power"},
    {0x0007, "Shift"},
// TODO: complete
    {0x0047, "Usage Indicator Color"},
    {0x0048, "Indicator Red"},
    {0x0049, "Indicator Green"},
    {0x004A, "Indicator Amber"},
    {0x004B, "Generic Indicator"},
    {0x004C, "System Suspend"},
    {0x004D, "External Power Connected"},
    {0x004E, "Indicator Blue"},
    {0x004F, "Indicator Orange"},
// TODO: complete
};

QString usagePageName(const unsigned page, bool defaultToHex=true)
{
    switch(page)
    {
    case UP_GENERIC_DESKTOP:
        return QObject::tr("Generic Desktop Controls");
        break;
    case UP_SIM_CONTROLS:
        return QObject::tr("Simulation Controls");
        break;
    case UP_VR_CONTROLS:
        return QObject::tr("VR Controls");
        break;
    case UP_SPORT_CONTROLS:
        return QObject::tr("Sport Controls");
        break;
    case UP_GAME_CONTROLS:
        return QObject::tr("Game Controls");
        break;
    case UP_GENERIC_DEV_CONTROLS:
        return QObject::tr("Generic Device Controls");
        break;
    case UP_KEYBOARD_KEYPAD:
        return QObject::tr("Keyboard/Keypad");
        break;
    case UP_LED:
        return QObject::tr("LED");
        break;
    case UP_BUTTON:
        return QObject::tr("Button");
        break;
    case UP_ORDINAL:
        return QObject::tr("Ordinal");
        break;
    case UP_TELEPHONY_DEVICE:
        return QObject::tr("Telephony Device");
        break;
    case UP_CONSUMER:
        return QObject::tr("Consumer Control");
        break;
    case UP_DIGITIZERS:
        return QObject::tr("Digitizers");
        break;
    case UP_HAPTICS:
        return QObject::tr("Haptics");
        break;
    case UP_UNICODE:
        return QObject::tr("Unicode");
        break;
    case UP_EYE_HEAD_TRACKERS:
        return QObject::tr("Eye and Head Trackers");
        break;
    case UP_AUX_DISPLAY:
        return QObject::tr("Auxiliary Display");
        break;
    case UP_SENSORS:
        return QObject::tr("Sensors");
        break;
    case UP_MEDICAL_INSTRUMENT:
        return QObject::tr("Medical Instrument");
        break;
    case UP_BRAILLE_DISPLAY:
        return QObject::tr("Braille Display");
        break;
    case UP_LIGHTING_ILLUMINATION:
        return QObject::tr("Lighting and Illumination");
        break;
    case UP_CAMERA_CONTROL:
        return QObject::tr("Camera Control");
        break;
    case UP_GAMING_DEVICE:
        return QObject::tr("Gaming Device");
        break;
    case UP_FIDO_ALLIANCE:
        return QObject::tr("FIDO Alliance");
        break;
    }
    if(defaultToHex)
        return QString("0x%1").arg(page, 2, 16, QChar('0'));
    return {};
}

const std::unordered_map<uint16_t, QString>* usagePageMap(const uint16_t usagePage)
{
    switch(usagePage)
    {
    case 0x01: return &genericDesktopPage;
    case 0x08: return &ledPage;
    default:   return nullptr;
    }
}

DEFINE_EXPLICIT_BOOL(IncludeHex);
DEFINE_EXPLICIT_BOOL(ShowPage);
QString usageName(const uint32_t usage, const IncludeHex includeHex=IncludeHex{false}, const ShowPage showPage=ShowPage{true})
{
    const auto page = usage>>16;

    if(const auto pageMap=usagePageMap(page))
    {
        if(const auto it=pageMap->find(usage); it!=pageMap->end())
        {
            const auto pageName=usagePageName(page, true);
            const auto hex = includeHex ?  QString(" (0x%3)").arg(usage, 2,16,QChar('0')) : "";
            if(showPage)
                return QString("%1 {%2%3}").arg(pageName).arg(it->second).arg(hex);
            else
                return QString("%2%3").arg(it->second).arg(hex);
        }
    }

    if(showPage)
    {
        if(const auto pageName=usagePageName(page, false); !pageName.isEmpty())
        {
            const auto hex = includeHex ?  QString(" (ext. usage 0x%1)").arg(usage, 6,16,QChar('0')) : "";
            return QString("%1 {id=0x%2}%3").arg(pageName).arg(usage&0xffff, 2,16,QChar('0')).arg(hex);
        }
        return QString("0x%1").arg(usage, 2,16,QChar('0'));
    }

    return QString("0x%1").arg(usage&0xffff, 2,16,QChar('0'));
}

int32_t getItemDataSigned(const uint8_t*const data, const unsigned size)
{
    assert(size<=4); // This is only for Short items

    switch(size)
    {
    case 0: return 0; // a valid case that may encode a zero value
    case 1: return int8_t(data[0]); // sign-extend
    case 2: return int16_t(data[1]<<8 | data[0]);
    case 4: return uint32_t(data[3])<<24 | data[2]<<16 | data[1]<<8 | data[0]; // cast prevents UB
    }

    throw std::logic_error("getItemData() called for long item");
}

uint32_t getItemDataUnsigned(const uint8_t*const data, const unsigned size)
{
    assert(size<=4); // This is only for Short items

    switch(size)
    {
    case 0: return 0; // a valid case that may encode a zero value
    case 1: return data[0];
    case 2: return data[1]<<8 | data[0];
    case 4: return uint32_t(data[3])<<24 | data[2]<<16 | data[1]<<8 | data[0]; // cast prevents UB
    }

    throw std::logic_error("getItemData() called for long item");
}

QString formatInOutFeatItem(const MainItemTag tag, const unsigned dataValue)
{
    return QString("%0 (%1, %2, %3%4%5%6%7%8%9)").arg(tag==MIT_INPUT ? QObject::tr("Input") :
                                                      tag==MIT_OUTPUT? QObject::tr("Output"):
                                                      tag==MIT_FEATURE ? QObject::tr("Feature") : "(my bug, please report)")
                                                 .arg((dataValue&1) ? QObject::tr("Constant") : QObject::tr("Data"))
                                                 .arg((dataValue&2) ? QObject::tr("Variable") : QObject::tr("Array"))
                                                 .arg((dataValue&4) ? QObject::tr("Relative") : QObject::tr("Absolute"))
                                                 .arg((dataValue&8)     ? QObject::tr(", Wrap")                 : "")
                                                 .arg((dataValue&0x10)  ? QObject::tr(", Nonlinear")            : "")
                                                 .arg((dataValue&0x20)  ? QObject::tr(", No Preferred State")   : "")
                                                 .arg((dataValue&0x40)  ? QObject::tr(", Has Null State")       : "")
                                                 .arg((dataValue&0x80)  ? QObject::tr(", Volatile")             : "")
                                                 .arg((dataValue&0x100) ? QObject::tr(", Is Buffered Bytes")    : "");
}

struct DescriptionState
{
    // Main items
    int collectionNestingLevel=0;

    struct GlobalItems
    {
        std::optional<uint16_t> usagePage;
        std::optional<int> logicalMin;
        std::optional<int> logicalMax;
        std::optional<int> physicalMin;
        std::optional<int> physicalMax;
        std::optional<int> unitExp;
        std::optional<unsigned> unit;
        std::optional<unsigned> reportSize;
        std::optional<uint8_t> reportId;
        std::optional<unsigned> reportCount;
    } global;

    struct LocalItems
    {
        std::deque<uint32_t> usages;
        std::optional<uint32_t> usageMin;
        std::optional<uint32_t> usageMax;
        std::optional<unsigned> designatorIndex;
        std::optional<unsigned> designatorMin;
        std::optional<unsigned> designatorMax;
        std::optional<unsigned> stringIndex;
        std::optional<unsigned> stringMin;
        std::optional<unsigned> stringMax;
        bool delimiterOpened=false;

        void clear() { *this = {}; }
    } local;

    void closeCollection()
    {
        local.clear();
    }
};
struct ReportElement
{
    unsigned bitSize;
    std::vector<uint32_t> usages; // discrete usages for array element; single usage for variable
	std::optional<uint32_t> usageMin, usageMax; // these are used only for array elements
    unsigned multiplicity=1; // if the element is an array, this is its number of elements
    bool isArray=false;
};
struct ReportStructure
{
    std::optional<uint8_t> reportId;
    std::vector<ReportElement> elements;
    void addItem(DescriptionState& state, const unsigned itemData)
    {
        reportId=state.global.reportId;
        if(!state.local.usageMax != !state.local.usageMin)
            throw std::invalid_argument("Usage min & max must be either both present, or both not present");
        const bool isArray=!(itemData&2);
        if(isArray)
        {
            ReportElement elem;
            elem.bitSize=state.global.reportSize.value();
            elem.isArray=true;
            elem.multiplicity=state.global.reportCount.value();
            elem.usages.resize(state.local.usages.size());
            std::copy(state.local.usages.begin(), state.local.usages.end(), elem.usages.begin());
            elem.usageMin=state.local.usageMin;
            elem.usageMax=state.local.usageMax;
            elements.emplace_back(std::move(elem));
        }
        else
        {
            for(unsigned k=0; k<state.global.reportCount.value(); ++k)
            {
                ReportElement elem;
                elem.bitSize=state.global.reportSize.value();
                if(state.local.usageMin)
                {
                    if(!state.local.usages.empty())
                    {
                        elem.usages={state.local.usages.front()};
                        state.local.usages.pop_front();
                    }
                    else if(*state.local.usageMin+k <= *state.local.usageMax)
                        elem.usages = {*state.local.usageMin+k};
                    else
                    {
                        // padding
                        elem.usages={};
                        elem.multiplicity = state.global.reportCount.value() - k;
                        elements.emplace_back(std::move(elem));
                        break;
                    }
                }
                else
                {
                    if(!state.local.usages.empty())
                    {
                        elem.usages={state.local.usages.front()};
                        if(state.local.usages.size() > 1) // otherwise the last usage applies to next elements too
                            state.local.usages.pop_front();
                    }
                    else
                    {
                        // padding
                        elem.usages={};
                        elem.multiplicity = state.global.reportCount.value() - k;
                        elements.emplace_back(std::move(elem));
                        break;
                    }
                }
                elements.emplace_back(std::move(elem));
            }
        }
        state.local.clear();
    }
};

void check(const bool correct)
{
    if(!correct) throw std::invalid_argument("HID report check failed");
}

void addReportsTreeItem(QTreeWidgetItem*const root, std::vector<ReportStructure> const& reports, QString const& label)
{
    const auto reportsItem=new QTreeWidgetItem{{label}};
    root->addChild(reportsItem);
    for(const auto& report : reports)
    {
        const auto repItem=new QTreeWidgetItem{{report.reportId ? QString("0x%1").arg(*report.reportId, 2,16,QChar('0'))
                                                                : QObject::tr("Without Report ID")}};
        reportsItem->addChild(repItem);
        if(report.reportId)
            repItem->addChild(new QTreeWidgetItem{{QObject::tr("Report ID: 0x%1").arg(*report.reportId, 2,16,QChar('0'))}});
        for(const auto elem : report.elements)
        {
            if(!elem.usages.empty() || elem.usageMin)
            {
                if(elem.isArray)
                {
                    QString possibleUsages;
                    int32_t prevPage=-1;
                    bool needClosingBrace=false;
                    for(const auto usage : elem.usages)
                    {
                        const int32_t currPage = usage>>16;
                        if(prevPage != currPage)
                        {
                            if(needClosingBrace)
                            {
                                possibleUsages.chop(2); // ", "
                                possibleUsages  += "}, ";
                            }
                            possibleUsages += QString("%1 {").arg(usagePageName(currPage));
                            needClosingBrace=true;
                            prevPage = currPage;
                        }
                        possibleUsages += QString("%1, ").arg(usageName(usage, IncludeHex{false}, ShowPage{false}));
                    }
                    if(needClosingBrace)
                    {
                        possibleUsages.chop(2); // ", "
                        possibleUsages += "}, ";
                    }

                    if(elem.usageMin)
                        possibleUsages += QString("%1 — %2").arg(usageName(*elem.usageMin,IncludeHex{}))
                                                            .arg(usageName(*elem.usageMax,IncludeHex{}));
                    if(possibleUsages.endsWith(", "))
                        possibleUsages.chop(2);
                    repItem->addChild(new QTreeWidgetItem{{QObject::tr("%1-element array of %2-bit items, possible usages: %3")
                                                          .arg(elem.multiplicity)
                                                          .arg(elem.bitSize)
                                                          .arg(possibleUsages)}});
                }
                else
                {
                    repItem->addChild(new QTreeWidgetItem{{QObject::tr("%1-bit data, usage: %2")
                                                          .arg(elem.bitSize)
                                                          .arg(usageName(elem.usages[0], IncludeHex{false}, ShowPage{true}))}});
                }
            }
            else
            {
                repItem->addChild(new QTreeWidgetItem{{QObject::tr("%1-bit padding").arg(elem.bitSize*elem.multiplicity)}});
            }
        }
    }
}

}

void parseHIDReportDescriptor(QTreeWidgetItem*const root, QFont const& baseFont, std::vector<uint8_t> const& data)
{
    auto boldFont(baseFont);
    boldFont.setBold(true);

    auto descriptorDetailsItem=new QTreeWidgetItem{{QObject::tr("Detailed view")}};
    root->addChild(descriptorDetailsItem);
    std::vector<ReportStructure> reportsIn, reportsOut, reportsFeat;
    try
    {
        std::stack<DescriptionState> dscStates;
        dscStates.push({});
        std::stack<QTreeWidgetItem*> prevRoots;
        for(unsigned i=0; i<data.size();)
        {
            const auto head=data[i];
            auto str=QString("%1").arg(head, 2,16,QChar('0'));
            if(head==0xfe)
            {
                // Long item
                const unsigned dataSize=data.at(i+1);
                const unsigned tag=data.at(i+2);
                str+=QString(": %1 %2: ").arg(dataSize, 2,16,QChar('0')).arg(tag, 2,16,QChar('0'));
                for(unsigned k=0; k<dataSize; ++k)
                    str+=QString(" %1").arg(data.at(i+3+k), 2,16,QChar('0'));
                str += " (Long)";

                descriptorDetailsItem->addChild(new QTreeWidgetItem{{str}});
                i += dataSize+3;
                continue;
            }
            // Short item
            const unsigned dataSize = (head&3)==3 ? 4 : (head&3);
            if(dataSize>0)
                str+=":";
            for(unsigned k=0; k<dataSize; ++k)
                str+=QString(" %1").arg(data.at(i+1+k), 2,16,QChar('0'));
            const auto type = head>>2 & 3;
            const QString types[]={QObject::tr("Main"), QObject::tr("Global"), QObject::tr("Local"), QObject::tr("Reserved")};
            str += QString(" (%1)").arg(types[type]);

            auto item=new QTreeWidgetItem{{str}};
            descriptorDetailsItem->addChild(item);
            const auto tag=head>>4;
            // NOTE: we've already checked above that we don't overflow data.size()
            const auto dataValueS = getItemDataSigned(data.data()+i+1, dataSize);
            const auto dataValueU = getItemDataUnsigned(data.data()+i+1, dataSize);
            switch(type)
            {
            case IT_MAIN:
                switch(tag)
                {
                case MIT_INPUT:
                case MIT_OUTPUT:
                case MIT_FEATURE:
                {
                    item->setData(1, Qt::DisplayRole, formatInOutFeatItem(static_cast<MainItemTag>(tag), dataValueU));
                    item->setData(1, Qt::FontRole, boldFont);
                    auto& reports = tag==MIT_INPUT ? reportsIn
                                  : tag==MIT_OUTPUT ? reportsOut
                                  :                   reportsFeat;
                    if(reports.empty() || (!reports.back().elements.empty() && reports.back().reportId!=dscStates.top().global.reportId))
                        reports.push_back({});
                    reports.back().addItem(dscStates.top(), dataValueU);
                    dscStates.top().local.clear();
                    break;
                }
                case MIT_COLLECTION:
                {
                    QString typeStr;
                    switch(dataValueU)
                    {
                    case COL_PHYSICAL:
                        typeStr=QObject::tr("Physical");
                        break;
                    case COL_APPLICATION:
                        typeStr=QObject::tr("Application");
                        break;
                    case COL_LOGICAL:
                        typeStr=QObject::tr("Logical");
                        break;
                    case COL_REPORT:
                        typeStr=QObject::tr("Report");
                        break;
                    case COL_NAMED_ARRAY:
                        typeStr=QObject::tr("Named Array");
                        break;
                    case COL_USAGE_SWITCH:
                        typeStr=QObject::tr("Usage Switch");
                        break;
                    case COL_USAGE_MODIFIER:
                        typeStr=QObject::tr("Usage Modifier");
                        break;
                    default:
                        if(dataValueU<0x7f)
                            typeStr=QObject::tr("Reserved type 0x%1").arg(dataValueU, 2, 16, QChar('0'));
                        else
                            typeStr=QObject::tr("Vendor-defined type 0x%1").arg(dataValueU, 2, 16, QChar('0'));
                        break;
                    }
                    item->setData(1, Qt::DisplayRole, QObject::tr("Collection (%1)").arg(typeStr));
                    prevRoots.push(descriptorDetailsItem);
                    descriptorDetailsItem->addChild(item);
                    descriptorDetailsItem=item;
                    dscStates.top().local.clear();
                    break;
                }
                case MIT_END_COLLECTION:
                    if(prevRoots.empty())
                    {
                        item->setData(1, Qt::DisplayRole, QObject::tr("*** Stray End Collection"));
                        break;
                    }
                    item->setData(1, Qt::DisplayRole, QObject::tr("End Collection"));
                    descriptorDetailsItem=prevRoots.top();
                    prevRoots.pop();

                    dscStates.top().closeCollection();
                    break;
                }
                break;
            case IT_GLOBAL:
                switch(tag)
                {
                case GIT_USAGE_PAGE:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Usage Page: %1").arg(usagePageName(dataValueU)));
                    dscStates.top().global.usagePage=dataValueU;
                    break;
                case GIT_LOG_MIN:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Logical Minimum: %1").arg(dataValueS));
                    dscStates.top().global.logicalMin=dataValueS;
                    break;
                case GIT_LOG_MAX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Logical Maximum: %1").arg(dataValueS));
                    dscStates.top().global.logicalMax=dataValueS;
                    break;
                case GIT_PHYS_MIN:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Physical Minimum: %1").arg(dataValueS));
                    dscStates.top().global.physicalMin=dataValueS;
                    break;
                case GIT_PHYS_MAX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Physical Maximum: %1").arg(dataValueS));
                    dscStates.top().global.physicalMax=dataValueS;
                    break;
                case GIT_UNIT_EXP:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Unit Exponent: %1").arg(dataValueS));
                    dscStates.top().global.unitExp=dataValueS;
                    break;
                case GIT_UNIT:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Unit"));
                    dscStates.top().global.unit=dataValueU;
                    break;
                case GIT_REP_SIZE:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Report Size: %1").arg(dataValueU));
                    dscStates.top().global.reportSize=dataValueU;
                    break;
                case GIT_REP_ID:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Report ID: 0x%1").arg(dataValueU, 2, 16, QChar('0')));
                    check(dataValueU<256);
                    dscStates.top().global.reportId=dataValueU;
                    break;
                case GIT_REP_COUNT:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Report Count: %1").arg(dataValueU));
                    dscStates.top().global.reportCount=dataValueU;
                    break;
                case GIT_PUSH:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Push"));
                    dscStates.push(dscStates.top());
                    break;
                case GIT_POP:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Pop"));
                    check(dscStates.size()>1);
                    dscStates.pop();
                    break;
                }
                break;
            case IT_LOCAL:
                switch(tag)
                {
                case LIT_USAGE:
                    if(dataSize>2)
                    {
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage: %1").arg(usageName(dataValueU, IncludeHex{false}, ShowPage{})));
                        dscStates.top().local.usages.push_back(dataValueU);
                    }
                    else
                    {
                        const auto page = dscStates.top().global.usagePage.value();
                        const auto usage = uint32_t(page)<<16 | dataValueU;
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage: %1").arg(usageName(usage, IncludeHex{false}, ShowPage{false})));
                        dscStates.top().local.usages.push_back(usage);
                    }
                    break;
                case LIT_USAGE_MIN:
                    if(dataSize>2)
                    {
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage Minimum: %1").arg(usageName(dataValueU, IncludeHex{})));
                        dscStates.top().local.usageMin=dataValueU;
                    }
                    else
                    {
                        const auto page = dscStates.top().global.usagePage.value();
                        const auto usage = uint32_t(page)<<16 | dataValueU;
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage Minimum: %1").arg(usageName(usage, IncludeHex{})));
                        dscStates.top().local.usageMin = uint32_t(page)<<16 | dataValueU;
                    }
                    break;
                case LIT_USAGE_MAX:
                    if(dataSize>2)
                    {
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage Maximum: %1").arg(usageName(dataValueU, IncludeHex{})));
                        dscStates.top().local.usageMax=dataValueU;
                    }
                    else
                    {
                        const auto page = dscStates.top().global.usagePage.value();
                        const auto usage = uint32_t(page)<<16 | dataValueU;
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage Maximum: %1").arg(usageName(usage, IncludeHex{})));
                        dscStates.top().local.usageMax = uint32_t(page)<<16 | dataValueU;
                    }
                    break;
                case LIT_DESIG_IDX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Designator Index: %1").arg(dataValueU));
                    dscStates.top().local.designatorIndex=dataValueU;
                    break;
                case LIT_DESIG_MIN:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Designator Minimum: 0x%1").arg(dataValueU, 2, 16, QChar('0')));
                    dscStates.top().local.designatorMin=dataValueU;
                    break;
                case LIT_DESIG_MAX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Designator Maximum: 0x%1").arg(dataValueU, 2, 16, QChar('0')));
                    dscStates.top().local.designatorMax=dataValueU;
                    break;
                case LIT_STRING_IDX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("String Index: %1").arg(dataValueU));
                    dscStates.top().local.stringIndex=dataValueU;
                    break;
                case LIT_STR_MIN:
                    item->setData(1, Qt::DisplayRole, QObject::tr("String Minimum: %1").arg(dataValueU));
                    dscStates.top().local.stringMin=dataValueU;
                    break;
                case LIT_STR_MAX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("String Maximum: %1").arg(dataValueU));
                    dscStates.top().local.stringMax=dataValueU;
                    break;
                case LIT_DELIM:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Delimiter: %1").arg(dataValueU));
                    break;
                }
                break;
            }

            i += dataSize+1;
        }

        const auto reportsItem=new QTreeWidgetItem{{QObject::tr("Supported reports")}};
        root->addChild(reportsItem);
        if(!reportsIn.empty())
            addReportsTreeItem(reportsItem,reportsIn,QObject::tr("Input reports"));
        if(!reportsOut.empty())
            addReportsTreeItem(reportsItem,reportsOut,QObject::tr("Output reports"));
        if(!reportsFeat.empty())
            addReportsTreeItem(reportsItem,reportsFeat,QObject::tr("Feature reports"));
    }
    catch(std::out_of_range const&)
    {
        descriptorDetailsItem->addChild(new QTreeWidgetItem{{QObject::tr("(broken item: too few bytes)")}});
    }
    catch(std::bad_optional_access const&)
    {
        descriptorDetailsItem->addChild(new QTreeWidgetItem{{QObject::tr("(broken item: some fields missing)")}});
    }
}
