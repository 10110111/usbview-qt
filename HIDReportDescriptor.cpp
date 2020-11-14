#include "HIDReportDescriptor.h"
#include <stack>
#include <cassert>
#include <QFont>
#include <QTreeWidgetItem>

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

}

void parseHIDReportDescriptor(QTreeWidgetItem* root, QFont const& baseFont, std::vector<uint8_t> const& data)
{
    auto boldFont(baseFont);
    boldFont.setBold(true);
    try
    {
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

                root->addChild(new QTreeWidgetItem{{str}});
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
            root->addChild(item);
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
                    item->setData(1, Qt::DisplayRole, formatInOutFeatItem(static_cast<MainItemTag>(tag), dataValueU));
                    item->setData(1, Qt::FontRole, boldFont);
                    break;
                case MIT_OUTPUT:
                    item->setData(1, Qt::DisplayRole, formatInOutFeatItem(static_cast<MainItemTag>(tag), dataValueU));
                    item->setData(1, Qt::FontRole, boldFont);
                    break;
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
                    prevRoots.push(root);
                    root->addChild(item);
                    root=item;
                    break;
                }
                case MIT_FEATURE:
                    item->setData(1, Qt::DisplayRole, formatInOutFeatItem(static_cast<MainItemTag>(tag), dataValueU));
                    item->setData(1, Qt::FontRole, boldFont);
                    break;
                case MIT_END_COLLECTION:
                    if(prevRoots.empty())
                    {
                        item->setData(1, Qt::DisplayRole, QObject::tr("*** Stray End Collection"));
                        break;
                    }
                    item->setData(1, Qt::DisplayRole, QObject::tr("End Collection"));
                    root=prevRoots.top();
                    prevRoots.pop();
                    break;
                }
                break;
            case IT_GLOBAL:
                switch(tag)
                {
                case GIT_USAGE_PAGE:
                {
                    QString name;
                    switch(dataValueU)
                    {
                    case UP_GENERIC_DESKTOP:
                        name=QObject::tr("Generic Desktop Controls");
                        break;
                    case UP_SIM_CONTROLS:
                        name=QObject::tr("Simulation Controls");
                        break;
                    case UP_VR_CONTROLS:
                        name=QObject::tr("VR Controls");
                        break;
                    case UP_SPORT_CONTROLS:
                        name=QObject::tr("Sport Controls");
                        break;
                    case UP_GAME_CONTROLS:
                        name=QObject::tr("Game Controls");
                        break;
                    case UP_GENERIC_DEV_CONTROLS:
                        name=QObject::tr("Generic Device Controls");
                        break;
                    case UP_KEYBOARD_KEYPAD:
                        name=QObject::tr("Keyboard/Keypad");
                        break;
                    case UP_LED:
                        name=QObject::tr("LED");
                        break;
                    case UP_BUTTON:
                        name=QObject::tr("Button");
                        break;
                    case UP_ORDINAL:
                        name=QObject::tr("Ordinal");
                        break;
                    case UP_TELEPHONY_DEVICE:
                        name=QObject::tr("Telephony Device");
                        break;
                    case UP_CONSUMER:
                        name=QObject::tr("Consumer Control");
                        break;
                    case UP_DIGITIZERS:
                        name=QObject::tr("Digitizers");
                        break;
                    case UP_HAPTICS:
                        name=QObject::tr("Haptics");
                        break;
                    case UP_UNICODE:
                        name=QObject::tr("Unicode");
                        break;
                    case UP_EYE_HEAD_TRACKERS:
                        name=QObject::tr("Eye and Head Trackers");
                        break;
                    case UP_AUX_DISPLAY:
                        name=QObject::tr("Auxiliary Display");
                        break;
                    case UP_SENSORS:
                        name=QObject::tr("Sensors");
                        break;
                    case UP_MEDICAL_INSTRUMENT:
                        name=QObject::tr("Medical Instrument");
                        break;
                    case UP_BRAILLE_DISPLAY:
                        name=QObject::tr("Braille Display");
                        break;
                    case UP_LIGHTING_ILLUMINATION:
                        name=QObject::tr("Lighting and Illumination");
                        break;
                    case UP_CAMERA_CONTROL:
                        name=QObject::tr("Camera Control");
                        break;
                    case UP_GAMING_DEVICE:
                        name=QObject::tr("Gaming Device");
                        break;
                    case UP_FIDO_ALLIANCE:
                        name=QObject::tr("FIDO Alliance");
                        break;
                    }
                    item->setData(1, Qt::DisplayRole, name.isEmpty() ? QObject::tr("Usage Page: 0x%1").arg(dataValueU, 2, 16, QChar('0'))
                                                                     : QObject::tr("Usage Page: %1").arg(name));
                    break;
                }
                case GIT_LOG_MIN:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Logical Minimum: %1").arg(dataValueS));
                    break;
                case GIT_LOG_MAX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Logical Maximum: %1").arg(dataValueS));
                    break;
                case GIT_PHYS_MIN:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Physical Minimum: %1").arg(dataValueS));
                    break;
                case GIT_PHYS_MAX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Physical Maximum: %1").arg(dataValueS));
                    break;
                case GIT_UNIT_EXP:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Unit Exponent: %1").arg(dataValueS));
                    break;
                case GIT_UNIT:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Unit"));
                    break;
                case GIT_REP_SIZE:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Report Size: %1").arg(dataValueU));
                    break;
                case GIT_REP_ID:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Report ID: 0x%1").arg(dataValueU, 2, 16, QChar('0')));
                    break;
                case GIT_REP_COUNT:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Report Count: %1").arg(dataValueU));
                    break;
                case GIT_PUSH:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Push"));
                    break;
                case GIT_POP:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Pop"));
                    break;
                }
                break;
            case IT_LOCAL:
                switch(tag)
                {
                case LIT_USAGE:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Usage: 0x%1").arg(dataValueU, 2, 16, QChar('0')));
                    break;
                case LIT_USAGE_MIN:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Usage Minimum: 0x%1").arg(dataValueU, 2, 16, QChar('0')));
                    break;
                case LIT_USAGE_MAX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Usage Maximum: 0x%1").arg(dataValueU, 2, 16, QChar('0')));
                    break;
                case LIT_DESIG_IDX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Designator Index: %1").arg(dataValueU));
                    break;
                case LIT_DESIG_MIN:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Designator Minimum: 0x%1").arg(dataValueU, 2, 16, QChar('0')));
                    break;
                case LIT_DESIG_MAX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Designator Maximum: 0x%1").arg(dataValueU, 2, 16, QChar('0')));
                    break;
                case LIT_STRING_IDX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("String Index: %1").arg(dataValueU));
                    break;
                case LIT_STR_MIN:
                    item->setData(1, Qt::DisplayRole, QObject::tr("String Minimum: %1").arg(dataValueU));
                    break;
                case LIT_STR_MAX:
                    item->setData(1, Qt::DisplayRole, QObject::tr("String Maximum: %1").arg(dataValueU));
                    break;
                case LIT_DELIM:
                    item->setData(1, Qt::DisplayRole, QObject::tr("Delimiter: %1").arg(dataValueU));
                    break;
                }
                break;
            }

            i += dataSize+1;
        }
    }
    catch(std::out_of_range const&)
    {
        root->addChild(new QTreeWidgetItem{{QObject::tr("(broken item: too few bytes)")}});
    }
}
