// Unit Include
#include "am-settings-items.h"

namespace Am
{

//===============================================================================================================
// SettingsItem
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
SettingsItem::SettingsItem(const QString& settingName) : mSettingName(settingName) {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString SettingsItem::settingName() const { return mSettingName; }

//===============================================================================================================
// DisplayGlobalFilter
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
DisplayGlobalFilter::DisplayGlobalFilter() :
    SettingsItem(META_NAME)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QStringList DisplayGlobalFilter::rules() const { return mRules; }
QStringList DisplayGlobalFilter::exceptions() const { return mExceptions; }

//===============================================================================================================
// DisplayGlobalFilter::Builder
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
DisplayGlobalFilter::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
DisplayGlobalFilter::Builder& DisplayGlobalFilter::Builder::wRule(const QString& rule) { mBlueprint.mRules.append(rule); return *this; }
DisplayGlobalFilter::Builder& DisplayGlobalFilter::Builder::wException(const QString& exception)
{
    mBlueprint.mExceptions.append(exception); return *this;
}
//===============================================================================================================
// DisplayFilter
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
DisplayFilter::DisplayFilter() :
    DisplayFilter(QString())
{}

DisplayFilter::DisplayFilter(const QString& name) :
    SettingsItem(META_NAME),
    mName(name),
    mSortBy(DisplayFilter::Sort::NoSort),
    mReverseOrder(false),
    mListLimit(0)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString DisplayFilter::name() const { return mName; }
QStringList DisplayFilter::rules() const { return mRules; }
QStringList DisplayFilter::exceptions() const { return mExceptions; }
DisplayFilter::Sort DisplayFilter::sortBy() const { return mSortBy; }
bool DisplayFilter::reverseOrder() const { return mReverseOrder; }
int DisplayFilter::listLimit() const { return mListLimit; }

//===============================================================================================================
// DisplayFilter::Builder
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
DisplayFilter::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
DisplayFilter::Builder& DisplayFilter::Builder::wName(const QString& name) { mBlueprint.mName = name; return *this; }
DisplayFilter::Builder& DisplayFilter::Builder::wRule(const QString& rule) { mBlueprint.mRules.append(rule); return *this; }
DisplayFilter::Builder& DisplayFilter::Builder::wException(const QString& exception) { mBlueprint.mExceptions.append(exception); return *this; }
DisplayFilter::Builder& DisplayFilter::Builder::wSortBy(DisplayFilter::Sort sortBy) { mBlueprint.mSortBy = sortBy; return *this; }
DisplayFilter::Builder& DisplayFilter::Builder::wReverseOrder(bool reverseOrder) { mBlueprint.mReverseOrder = reverseOrder; return *this; }
DisplayFilter::Builder& DisplayFilter::Builder::wListLimit(int listLimit) { mBlueprint.mListLimit = listLimit; return *this; }

//===============================================================================================================
// Display
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Display::Display() :
    Display(QString())
{}

Display::Display(QString name) :
    SettingsItem(META_NAME),
    mName(name),
    mInCycle(false),
    mInMenu(true)
{}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString Display::name() const { return mName; }
QString Display::layout() const { return mLayout; }
QString Display::romlist() const { return mRomlist; }
bool Display::inCycle() const { return mInCycle; }
bool Display::inMenu() const { return mInMenu; }
std::optional<DisplayGlobalFilter> Display::globalFilter() const { return mGlobalFilter; }
QList<DisplayFilter>& Display::filters() { return mFilters; }
const QList<DisplayFilter>& Display::filters() const { return mFilters; }

//===============================================================================================================
// Display::Builder
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Display::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
Display::Builder& Display::Builder::wName(const QString& name) { mBlueprint.mName = name; return *this; }
Display::Builder& Display::Builder::wLayout(const QString& layout) { mBlueprint.mLayout = layout; return *this; }
Display::Builder& Display::Builder::wRomlist(const QString& romlist) { mBlueprint.mRomlist = romlist; return *this; }
Display::Builder& Display::Builder::wInCycle(bool inCycle) { mBlueprint.mInCycle = inCycle; return *this; }
Display::Builder& Display::Builder::wInMenu(bool inMenu) { mBlueprint.mInMenu = inMenu; return *this; }
Display::Builder& Display::Builder::wGlobalFilter(const DisplayGlobalFilter& globalFilter)
{
    mBlueprint.mGlobalFilter = globalFilter; return *this;
}
Display::Builder& Display::Builder::wFilter(const DisplayFilter& filter) { mBlueprint.mFilters.append(filter); return *this; }

//===============================================================================================================
// OtherSetting
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
OtherSetting::OtherSetting() :
    OtherSetting(QString(), QString())
{}

OtherSetting::OtherSetting(const QString& type, const QString& name) :
    SettingsItem(META_NAME),
    mType(type),
    mName(name)
{}

//-Class Functions--------------------------------------------------------------------------------------------------
//Public:
QUuid OtherSetting::equivalentId(const QString& type, const QString& name)
{
    return QUuid::createUuidV5(NAMESPACE_SEED, type + name);
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString OtherSetting::type() const{ return mType; }
QString OtherSetting::name() const{ return mName; }
QList<OtherSetting::ContentLine> OtherSetting::contents() const{ return mContents; }

//===============================================================================================================
// OtherSetting::Builder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
OtherSetting::Builder::Builder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
OtherSetting::Builder& OtherSetting::Builder::wTypeAndName(const QString& type, const QString& name)
{
    mBlueprint.mType = type;
    mBlueprint.mName = name;
    return *this;
}

OtherSetting::Builder& OtherSetting::Builder::wContent(const OtherSetting::ContentLine& line)
{
    mBlueprint.mContents.append(line); return *this;
}


}
