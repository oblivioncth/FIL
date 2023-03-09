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
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;
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

class DisplayGlobalFilter::Builder : public Fe::Item::Builder<DisplayGlobalFilter::Builder, DisplayGlobalFilter>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wRule(QString rule);
    Builder& wException(QString exception);
};

class DisplayFilter : public SettingsItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;
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

class DisplayFilter::Builder : public Fe::Item::Builder<DisplayFilter::Builder, DisplayFilter>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(QString name);
    Builder& wRule(QString rule);
    Builder& wException(QString exception);
    Builder& wSortBy(DisplayFilter::Sort sortBy);
    Builder& wReverseOrder(bool reverseOrder);
    Builder& wListLimit(int listLimit);
};

class Display : public SettingsItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;
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
    QList<DisplayFilter>& filters();
    const QList<DisplayFilter>& filters() const;
};

class Display::Builder : public Fe::Item::Builder<Display::Builder, Display>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(QString name);
    Builder& wLayout(QString layout);
    Builder& wRomlist(QString romlist);
    Builder& wInCycle(bool inCycle);
    Builder& wInMenu(bool inMenu);
    Builder& wGlobalFilter(DisplayGlobalFilter globalFilter);
    Builder& wFilter(DisplayFilter filter);
};

class OtherSetting : public SettingsItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;
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

class OtherSetting::Builder : public Fe::Item::Builder<OtherSetting::Builder, OtherSetting>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wTypeAndName(QString name, QString type);
    Builder& wContent(const OtherSetting::ContentLine& line);
};

}
#endif // AM_CONFIG_ITEMS_H