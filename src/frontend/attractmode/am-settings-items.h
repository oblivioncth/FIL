#ifndef AM_SETTINGS_ITEMS_H
#define AM_SETTINGS_ITEMS_H

// Project Includes
#include "../fe-items.h"

namespace Am
{

class SettingsItem : public Fe::Item
{
//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mSettingName;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    SettingsItem(const QString& settingName);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString settingName() const;
};

class DisplayGlobalFilter : public SettingsItem
{
    friend class DisplayGlobalFilterBuilder;

//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Parser;

//-Class Variables---------------------------------------------------------------------------------------------------
private:
    static inline const QString META_NAME = "DisplayGlobalFilter";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QStringList mRules;
    QStringList mExceptions;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    DisplayGlobalFilter();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QStringList rules() const;
    QStringList exceptions() const;
};

class DisplayGlobalFilterBuilder : public Fe::ItemBuilder<DisplayGlobalFilterBuilder, DisplayGlobalFilter>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    DisplayGlobalFilterBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    DisplayGlobalFilterBuilder& wRule(QString rule);
    DisplayGlobalFilterBuilder& wException(QString exception);
};

class DisplayFilter : public SettingsItem
{
    friend class DisplayFilterBuilder;

//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Parser;

//-Class Enums--------------------------------------------------------------------------------------------------
public:
    enum class Sort {
        NoSort, // NOTE: If this value is set, don't write the field; also don't write the "reverse_order" field
        Name,
        Title,
        Emulator,
        CloneOf,
        Year,
        Manufacturer,
        Category,
        Players,
        Rotation,
        Control,
        Status,
        DisplayCount,
        DisplayType,
        AltRomname,
        AltTitle,
        Extra,
        Buttons,
        Series,
        Language,
        Region,
        Rating,
        Favourite,
        Tags,
        PlayedCount,
        PlayedTime,
        FileIsAvailable
    };

//-Class Variables---------------------------------------------------------------------------------------------------
private:
    static inline const QString META_NAME = "DisplayFilter";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mName;
    QStringList mRules;
    QStringList mExceptions;
    Sort mSortBy;
    bool mReverseOrder;
    int mListLimit;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    DisplayFilter();
    DisplayFilter(QString name);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const;
    QStringList rules() const;
    QStringList exceptions() const;
    Sort sortBy() const;
    bool reverseOrder() const;
    int listLimit() const;
};

class DisplayFilterBuilder : public Fe::ItemBuilder<DisplayFilterBuilder, DisplayFilter>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    DisplayFilterBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    DisplayFilterBuilder& wName(QString name);
    DisplayFilterBuilder& wRule(QString rule);
    DisplayFilterBuilder& wException(QString exception);
    DisplayFilterBuilder& wSortBy(DisplayFilter::Sort sortBy);
    DisplayFilterBuilder& wReverseOrder(bool reverseOrder);
    DisplayFilterBuilder& wListLimit(int listLimit);
};

class Display : public SettingsItem
{
    friend class DisplayBuilder;

//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Parser;

//-Class Variables---------------------------------------------------------------------------------------------------
private:
    static inline const QString META_NAME = "Display";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mName;
    QString mLayout;
    QString mRomlist;
    bool mInCycle;
    bool mInMenu;
    std::optional<DisplayGlobalFilter> mGlobalFilter;
    QList<DisplayFilter> mFilters;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Display();
    Display(QString name);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const;
    QString layout() const;
    QString romlist() const;
    bool inCycle() const;
    bool inMenu() const;
    std::optional<DisplayGlobalFilter> globalFilter() const;
    QList<DisplayFilter> filters() const;
};

class DisplayBuilder : public Fe::ItemBuilder<DisplayBuilder, Display>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    DisplayBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    DisplayBuilder& wName(QString name);
    DisplayBuilder& wLayout(QString layout);
    DisplayBuilder& wRomlist(QString romlist);
    DisplayBuilder& wInCycle(bool inCycle);
    DisplayBuilder& wInMenu(bool inMenu);
    DisplayBuilder& wGlobalFilter(DisplayGlobalFilter globalFilter);
    DisplayBuilder& wFilter(DisplayFilter filter);
};

class OtherSetting : public SettingsItem
{
    friend class OtherSettingBuilder;

//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Parser;

//-Class Variables--------------------------------------------------------------------------------------------------
private:
    static inline const QUuid NAMESPACE_SEED = QUuid("13d88ae7-9e9f-4736-bff4-e330e76c3b12");
    static inline const QString META_NAME = "OtherSetting";

//-Class Structs---------------------------------------------------------------------------------------------------
public:
    struct ContentLine
    {
        QString key;
        QString value;
        int depth;
    };

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QString mType;
    QString mName;
    QList<ContentLine> mContents;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    OtherSetting();
    OtherSetting(QString type, QString name);

//-Class Functions--------------------------------------------------------------------------------------------------
public:
    static QUuid equivalentId(QString type, QString name);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString type() const;
    QString name() const;
    QList<ContentLine> contents() const;
};

class OtherSettingBuilder : public Fe::ItemBuilder<OtherSettingBuilder, OtherSetting>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    OtherSettingBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    OtherSettingBuilder& wTypeAndName(QString name, QString type);
    OtherSettingBuilder& wContent(const OtherSetting::ContentLine& line);
};

}
#endif // AM_CONFIG_ITEMS_H
