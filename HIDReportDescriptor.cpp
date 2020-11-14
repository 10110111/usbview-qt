#include "HIDReportDescriptor.h"
#include <stack>
#include <QFont>
#include <QTreeWidgetItem>

void parseHIDReportDescriptor(QTreeWidgetItem* root, QFont const& baseFont, std::vector<uint8_t> const& data)
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
            }
            else
            {
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
                switch(type)
                {
                case IT_MAIN:
                    switch(tag)
                    {
                    case MIT_INPUT:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Input"));
                        item->setData(1, Qt::FontRole, boldFont);
                        break;
                    case MIT_OUTPUT:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Output"));
                        item->setData(1, Qt::FontRole, boldFont);
                        break;
                    case MIT_COLLECTION:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Collection"));
                        prevRoots.push(root);
                        root->addChild(item);
                        root=item;
                        break;
                    case MIT_FEATURE:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Feature"));
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
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage Page"));
                        break;
                    case GIT_LOG_MIN:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Logical Minimum"));
                        break;
                    case GIT_LOG_MAX:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Logical Maximum"));
                        break;
                    case GIT_PHYS_MIN:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Physical Minimum"));
                        break;
                    case GIT_PHYS_MAX:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Physical Maximum"));
                        break;
                    case GIT_UNIT_EXP:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Unit Exponent"));
                        break;
                    case GIT_UNIT:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Unit"));
                        break;
                    case GIT_REP_SIZE:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Report Size"));
                        break;
                    case GIT_REP_ID:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Report ID"));
                        break;
                    case GIT_REP_COUNT:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Report Count"));
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
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage"));
                        break;
                    case LIT_USAGE_MIN:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage Minimum"));
                        break;
                    case LIT_USAGE_MAX:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Usage Maximum"));
                        break;
                    case LIT_DESIG_IDX:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Designator Index"));
                        break;
                    case LIT_DESIG_MIN:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Designator Minimum"));
                        break;
                    case LIT_DESIG_MAX:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Designator Maximum"));
                        break;
                    case LIT_STRING_IDX:
                        item->setData(1, Qt::DisplayRole, QObject::tr("String Index"));
                        break;
                    case LIT_STR_MIN:
                        item->setData(1, Qt::DisplayRole, QObject::tr("String Minimum"));
                        break;
                    case LIT_STR_MAX:
                        item->setData(1, Qt::DisplayRole, QObject::tr("String Maximum"));
                        break;
                    case LIT_DELIM:
                        item->setData(1, Qt::DisplayRole, QObject::tr("Delimiter"));
                        break;
                    }
                    break;
                }

                i += dataSize+1;
            }
        }
    }
    catch(std::out_of_range const&)
    {
        root->addChild(new QTreeWidgetItem{{QObject::tr("(broken item: too few bytes)")}});
    }
}
