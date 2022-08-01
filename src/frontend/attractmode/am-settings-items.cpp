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
// DisplayGlobalFilterBuilder
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
DisplayGlobalFilterBuilder::DisplayGlobalFilterBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
DisplayGlobalFilterBuilder& DisplayGlobalFilterBuilder::wRule(QString rule) { mItemBlueprint.mRules.append(rule); return *this; }
DisplayGlobalFilterBuilder& DisplayGlobalFilterBuilder::wException(QString exception)
{
    mItemBlueprint.mExceptions.append(exception); return *this;
}
//===============================================================================================================
// DisplayFilter
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
DisplayFilter::DisplayFilter() :
    DisplayFilter(QString())
{}

DisplayFilter::DisplayFilter(QString name) :
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
// DisplayFilterBuilder
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
DisplayFilterBuilder::DisplayFilterBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
DisplayFilterBuilder& DisplayFilterBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this; }
DisplayFilterBuilder& DisplayFilterBuilder::wRule(QString rule) { mItemBlueprint.mRules.append(rule); return *this; }
DisplayFilterBuilder& DisplayFilterBuilder::wException(QString exception) { mItemBlueprint.mExceptions.append(exception); return *this; }
DisplayFilterBuilder& DisplayFilterBuilder::wSortBy(DisplayFilter::Sort sortBy) { mItemBlueprint.mSortBy = sortBy; return *this; }
DisplayFilterBuilder& DisplayFilterBuilder::wReverseOrder(bool reverseOrder) { mItemBlueprint.mReverseOrder = reverseOrder; return *this; }
DisplayFilterBuilder& DisplayFilterBuilder::wListLimit(int listLimit) { mItemBlueprint.mListLimit = listLimit; return *this; }

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
// DisplayBuilder
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
DisplayBuilder::DisplayBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
DisplayBuilder& DisplayBuilder::wName(QString name) { mItemBlueprint.mName = name; return *this; }
DisplayBuilder& DisplayBuilder::wLayout(QString layout) { mItemBlueprint.mLayout = layout; return *this; }
DisplayBuilder& DisplayBuilder::wRomlist(QString romlist) { mItemBlueprint.mRomlist = romlist; return *this; }
DisplayBuilder& DisplayBuilder::wInCycle(bool inCycle) { mItemBlueprint.mInCycle = inCycle; return *this; }
DisplayBuilder& DisplayBuilder::wInMenu(bool inMenu) { mItemBlueprint.mInMenu = inMenu; return *this; }
DisplayBuilder& DisplayBuilder::wGlobalFilter(DisplayGlobalFilter globalFilter)
{
    mItemBlueprint.mGlobalFilter = globalFilter; return *this;
}
DisplayBuilder& DisplayBuilder::wFilter(DisplayFilter filter) { mItemBlueprint.mFilters.append(filter); return *this; }

//===============================================================================================================
// OtherSetting
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
OtherSetting::OtherSetting() :
    OtherSetting(QString(), QString())
{}

OtherSetting::OtherSetting(QString type, QString name) :
    SettingsItem(META_NAME),
    mType(type),
    mName(name)
{}

//-Class Functions--------------------------------------------------------------------------------------------------
//Public:
QUuid OtherSetting::equivalentId(QString type, QString name)
{
    return QUuid::createUuidV5(NAMESPACE_SEED, type + name);
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString OtherSetting::type() const{ return mType; }
QString OtherSetting::name() const{ return mName; }
QList<OtherSetting::ContentLine> OtherSetting::contents() const{ return mContents; }

//===============================================================================================================
// OtherSettingBuilder
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
OtherSettingBuilder::OtherSettingBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
OtherSettingBuilder& OtherSettingBuilder::wTypeAndName(QString type, QString name)
{
    mItemBlueprint.mType = type;
    mItemBlueprint.mName = name;
    return *this;
}

OtherSettingBuilder& OtherSettingBuilder::wContent(const OtherSetting::ContentLine& line)
{
    mItemBlueprint.mContents.append(line); return *this;
}


}
