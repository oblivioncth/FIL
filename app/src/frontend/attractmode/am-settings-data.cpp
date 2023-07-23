// Unit Include
#include "am-settings-data.h"

// Qx Includes
#include <qx/utility/qx-helpers.h>

// magic_enum Includes
#include "magic_enum.hpp"

namespace Am
{

//===============================================================================================================
// ISettingParser
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ISettingParser::ISettingParser() {}

//-Destructor--------------------------------------------------------------------------------------------------------
//Public:
ISettingParser::~ISettingParser() {}

//===============================================================================================================
// DisplayGlobalFilter::Parser
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
DisplayGlobalFilter::Parser::Parser(DisplayGlobalFilter* display) :
    SettingParser(display)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool DisplayGlobalFilter::Parser::parse(QStringView key, const QString& value, int depth)
{
    Q_UNUSED(depth);

    if(key == CrudeSettings::Keys::Display::GlobalFilter::RULE)
        mSetting->mRules.append(value);
    else if(key == CrudeSettings::Keys::Display::GlobalFilter::EXCEPTION)
        mSetting->mExceptions.append(value);
    else
        return false;

    // Value accepted
    return true;
}

//===============================================================================================================
// DisplayFilter::Parser
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
DisplayFilter::Parser::Parser(DisplayFilter* display) :
    SettingParser(display)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool DisplayFilter::Parser::parse(QStringView key, const QString& value, int depth)
{
    Q_UNUSED(depth);

    if(key == CrudeSettings::Keys::Display::Filter::RULE)
        mSetting->mRules.append(value);
    else if(key == CrudeSettings::Keys::Display::Filter::EXCEPTION)
        mSetting->mExceptions.append(value);
    else if(key == CrudeSettings::Keys::Display::Filter::SORT_BY)
    {
        auto sort = magic_enum::enum_cast<DisplayFilter::Sort>(value.toStdString());
        if(sort.has_value())
            mSetting->mSortBy = sort.value();
        else
            mSetting->mSortBy = DisplayFilter::Sort::NoSort;
    }
    else if(key == CrudeSettings::Keys::Display::Filter::REVERSE_ORDER)
        mSetting->mReverseOrder = value == u"true"_s ? true : false;
    else if(key == CrudeSettings::Keys::Display::Filter::LIST_LIMIT)
        mSetting->mListLimit = value.toInt();
    else
        return false;

    // Value accepted
    return true;
}

//===============================================================================================================
// Display::Parser
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Display::Parser::Parser(Display* display) :
    SettingParser(display)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool Display::Parser::parse(QStringView key, const QString& value, int depth)
{
    if(key == CrudeSettings::Keys::Display::LAYOUT)
        mSetting->mLayout = value;
    else if(key == CrudeSettings::Keys::Display::ROMLIST)
        mSetting->mRomlist = value;
    else if(key == CrudeSettings::Keys::Display::IN_CYCLE)
        mSetting->mInCycle = value == u"yes"_s ? true : false;
    else if(key == CrudeSettings::Keys::Display::IN_MENU)
        mSetting->mInMenu = value == u"yes"_s ? true : false;
    else if(key == CrudeSettings::Keys::Display::GLOBAL_FILTER)
    {
        // Set empty global filter to display
        mSetting->mGlobalFilter = DisplayGlobalFilter();
        DisplayGlobalFilter* addedFilter = &mSetting->mGlobalFilter.value();

        // Create parser and set to current
        mCurrentSubSettingParser = std::make_unique<DisplayGlobalFilter::Parser>(addedFilter);
    }
    else if(key == CrudeSettings::Keys::Display::FILTER)
    {
        // Add empty global filter to display
        mSetting->mFilters.append(DisplayFilter(value));
        DisplayFilter* addedFilter = &mSetting->mFilters.last();

        // Create parser and set to current
        mCurrentSubSettingParser = std::make_unique<DisplayFilter::Parser>(addedFilter);
    }
    else if(mCurrentSubSettingParser)
        mCurrentSubSettingParser->parse(key, value, depth);
    else
        return false;

    // Value accepted
    return true;
}

//===============================================================================================================
// OtherSetting::Parser
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
OtherSetting::Parser::Parser(OtherSetting* otherSetting) :
    SettingParser(otherSetting)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool OtherSetting::Parser::parse(QStringView key, const QString& value, int depth)
{
    mSetting->mContents.append({.key = key.toString(), .value = value, .depth = depth});
    return true;
}

//===============================================================================================================
// CrudeSettings
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
CrudeSettings::CrudeSettings(Install* const parent, const QString& filePath, const DocKey&) :
    ConfigDoc(parent, filePath, STD_NAME)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool CrudeSettings::isEmpty() const { return mDisplays.isEmpty() && mOtherSettings.isEmpty(); };
Fe::DataDoc::Type CrudeSettings::type() const { return Type::Config; }

bool CrudeSettings::containsDisplay(const QString& name) { return mDisplays.contains(name); }
void CrudeSettings::addDisplay(const Display& display) { mDisplays.insert(display.name(), display); }
Display& CrudeSettings::display(const QString& name) { return mDisplays[name]; }

bool CrudeSettings::containsOtherSetting(const QString& type, const QString& name)
{
    return mOtherSettings.contains(OtherSetting::equivalentId(type, name));
}

void CrudeSettings::addOtherSetting(const OtherSetting& setting)
{
    QUuid id = OtherSetting::equivalentId(setting.type(), setting.name());
    mOtherSettings.insert(id, setting);
}

//===============================================================================================================
// CrudeSettings::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
CrudeSettingsReader::CrudeSettingsReader(CrudeSettings* targetDoc) :
    ConfigDoc::Reader(targetDoc)
{}

//-Class Functions--------------------------------------------------------------------------------------------------
//Private:
int CrudeSettingsReader::checkTabDepth(const QString& line)
{
    quint8 count = 0;
    QString::const_iterator itr = line.cbegin();
    while(itr != line.cend() && *itr == '\t')
    {
        count++;
        itr++;
    }

    return count;
}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
CrudeSettings* CrudeSettingsReader::targetCrudeSettings() const
{
    return static_cast<CrudeSettings*>(mTargetDocument);
}

Fe::DocHandlingError CrudeSettingsReader::readTargetDoc()
{
    Fe::DocHandlingError errorStatus;

    // Got through all entries
    while(!mStreamReader.atEnd())
    {
        QString setting = readLineIgnoringComments();

        if(setting.isEmpty())// Must assume space ends a subsection with current crude design
            mCurrentSubSettingParser.reset();
        else
        {
            // Get depth (for "other settings")
            quint8 depth = checkTabDepth(setting);

            // Get key/value
            QString key;
            QString value;
            splitKeyValue(setting, key, value);

            // Check for known keys
            if(key == CrudeSettings::Keys::DISPLAY)
            {
                // Add empty display to doc
                targetCrudeSettings()->mDisplays[value] = Display(value);
                Display* addedDisplay = &targetCrudeSettings()->mDisplays[value];

                // Create parser and set to current
                mCurrentSubSettingParser = std::make_unique<Display::Parser>(addedDisplay);
            }
            else if(mCurrentSubSettingParser)
            {
                if(!mCurrentSubSettingParser->parse(key, value, depth))
                {
                    QString setting = mCurrentSubSettingParser->settingName();
                    errorStatus = Fe::DocHandlingError(*mTargetDocument, Fe::DocHandlingError::DocReadFailed, UNKNOWN_KEY_ERROR.arg(key, setting));
                    break;
                }
            }
            else
                initializeGenericSubSetting(key, value);
        }
    }

    // Cleanup sub setting parser
    mCurrentSubSettingParser.reset();

    // Return error status
    return errorStatus;
}

void CrudeSettingsReader::initializeGenericSubSetting(const QString& key, const QString& value)
{
    // Add empty generic
    QUuid id = OtherSetting::equivalentId(key, value);
    targetCrudeSettings()->mOtherSettings[id] = OtherSetting(key, value);
    OtherSetting* addedSetting = &targetCrudeSettings()->mOtherSettings[id];

    // Create parser and set to current
    mCurrentSubSettingParser = std::make_unique<OtherSetting::Parser>(addedSetting);
}

//===============================================================================================================
// CrudeSettings::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
CrudeSettingsWriter::CrudeSettingsWriter(CrudeSettings* sourceDoc) :
    ConfigDoc::Writer(sourceDoc),
    mTabDepth(0)
{
    // Global alignment
    mStreamWriter.setFieldAlignment(QTextStream::AlignLeft);
}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
CrudeSettings* CrudeSettingsWriter::sourceCrudeSettings() const
{
    return static_cast<CrudeSettings*>(mSourceDocument);
}

void CrudeSettingsWriter::writeKeyValue(const QString& key, const QString& value)
{
    mStreamWriter << QString(mTabDepth, '\t');
    mStreamWriter.setFieldWidth(KEY_FIELD_WIDTH);
    mStreamWriter << key;
    mStreamWriter.setFieldWidth(0);
    mStreamWriter << value << '\n';
}

bool CrudeSettingsWriter::writeConfigDoc()
{
    // Write all display entries
    for(const Display& display : qAsConst(sourceCrudeSettings()->mDisplays))
    {
        if(!writeDisplay(display))
            return false;
    }

    // Write all other settings
    for(const OtherSetting& setting : qAsConst(sourceCrudeSettings()->mOtherSettings))
    {
        if(!writeOtherSetting(setting))
            return false;
    }

    // Return true on success
    return true;
}

bool CrudeSettingsWriter::writeDisplay(const Display& display)
{
    // Write identifier
    mStreamWriter << CrudeSettings::Keys::DISPLAY << ' ' << display.name() << '\n';

    // Set tab depth
    mTabDepth++;

    // Write basic values
    if(!display.layout().isEmpty())
        writeKeyValue(CrudeSettings::Keys::Display::LAYOUT, display.layout());
    if(!display.romlist().isEmpty())
        writeKeyValue(CrudeSettings::Keys::Display::ROMLIST, display.romlist());
    writeKeyValue(CrudeSettings::Keys::Display::IN_CYCLE, display.inCycle() ? u"yes"_s : u"no"_s);
    writeKeyValue(CrudeSettings::Keys::Display::IN_MENU, display.inMenu() ? u"yes"_s : u"no"_s);

    // Write global filter, if present
    std::optional<DisplayGlobalFilter> globalFilter = display.globalFilter();
    if(globalFilter.has_value())
        writeDisplayGlobalFilter(globalFilter.value());

    // Write filters
    for(const DisplayFilter& filter : display.filters())
    {
        if(!writeDisplayFilter(filter))
            return false;
    }

    // Revert tab depth
    mTabDepth--;

    // Write trailing line break
    mStreamWriter << '\n';

    // Return stream status
    return !mStreamWriter.hasError();
}

bool CrudeSettingsWriter::writeDisplayGlobalFilter(const DisplayGlobalFilter& globalFilter)
{
    // Write identifier
    writeKeyValue(CrudeSettings::Keys::Display::GLOBAL_FILTER);

    // Set tab depth
    mTabDepth++;

    // Write rules
    for(const QString& rule : qxAsConst(globalFilter.rules()))
        writeKeyValue(CrudeSettings::Keys::Display::GlobalFilter::RULE, rule);

    // Write exceptions
    for(const QString& exception : qxAsConst(globalFilter.exceptions()))
        writeKeyValue(CrudeSettings::Keys::Display::GlobalFilter::EXCEPTION, exception);

    // Revert tab depth
    mTabDepth--;

    // Return stream status
    return !mStreamWriter.hasError();
}

bool CrudeSettingsWriter::writeDisplayFilter(const DisplayFilter& filter)
{
    // Write identifier
    writeKeyValue(CrudeSettings::Keys::Display::FILTER, filter.name());

    // Set tab depth
    mTabDepth++;

    // Write basic values
    if(filter.sortBy() != DisplayFilter::Sort::NoSort)
    {
        QString sortByValue = QString(magic_enum::enum_name(filter.sortBy()).data());
        writeKeyValue(CrudeSettings::Keys::Display::Filter::SORT_BY, sortByValue);
    }

    if(filter.reverseOrder() != false)
        writeKeyValue(CrudeSettings::Keys::Display::Filter::REVERSE_ORDER, u"true"_s);
    if(filter.listLimit() != 0)
        writeKeyValue(CrudeSettings::Keys::Display::Filter::LIST_LIMIT, QString::number(filter.listLimit()));

    // Write rules
    for(const QString& rule : qxAsConst(filter.rules()))
        writeKeyValue(CrudeSettings::Keys::Display::Filter::RULE, rule);

    // Write exceptions
    for(const QString& exception : qxAsConst(filter.exceptions()))
        writeKeyValue(CrudeSettings::Keys::Display::Filter::EXCEPTION, exception);

    // Revert tab depth
    mTabDepth--;

    // Return stream status
    return !mStreamWriter.hasError();
}

bool CrudeSettingsWriter::writeOtherSetting(const OtherSetting& setting)
{
    // Write identifier
    mStreamWriter << setting.type() << '\t' << setting.name() << '\n';

    // Write contents
    int origTabDepth = mTabDepth;
    for(const OtherSetting::ContentLine& line : qxAsConst(setting.contents()))
    {
        mTabDepth = line.depth;
        writeKeyValue(line.key, line.value);
    }
    mTabDepth = origTabDepth;

    // Write spacer after setting
    mStreamWriter << '\n';

    // Return status
    return !mStreamWriter.hasError();
}


}
