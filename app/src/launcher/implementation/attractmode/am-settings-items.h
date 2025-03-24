#ifndef AM_SETTINGS_ITEMS_H
#define AM_SETTINGS_ITEMS_H

// Project Includes
#include "launcher/interface/lr-items-interface.h"

using namespace Qt::Literals::StringLiterals;

namespace Am
{

class SettingsItem
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
    static inline const QString META_NAME = u"DisplayGlobalFilter"_s;

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

class DisplayGlobalFilter::Builder : public Lr::Builder<DisplayGlobalFilter>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wRule(const QString& rule);
    Builder& wException(const QString& exception);
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
    static inline const QString META_NAME = u"DisplayFilter"_s;

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
    DisplayFilter(const QString& name);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const;
    QStringList rules() const;
    QStringList exceptions() const;
    Sort sortBy() const;
    bool reverseOrder() const;
    int listLimit() const;
};

class DisplayFilter::Builder : public Lr::Builder<DisplayFilter>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(const QString& name);
    Builder& wRule(const QString& rule);
    Builder& wException(const QString& exception);
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
    static inline const QString META_NAME = u"Display"_s;

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

class Display::Builder : public Lr::Builder<Display>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wName(const QString& name);
    Builder& wLayout(const QString& layout);
    Builder& wRomlist(const QString& romlist);
    Builder& wInCycle(bool inCycle);
    Builder& wInMenu(bool inMenu);
    Builder& wGlobalFilter(const DisplayGlobalFilter& globalFilter);
    Builder& wFilter(const DisplayFilter& filter);
};

class OtherSetting : public SettingsItem
{
//-Inner Classes---------------------------------------------------------------------------------------------------
public:
    class Builder;
    class Parser;

//-Class Variables--------------------------------------------------------------------------------------------------
private:
    static inline const QUuid NAMESPACE_SEED = QUuid(u"13d88ae7-9e9f-4736-bff4-e330e76c3b12"_s);
    static inline const QString META_NAME = u"OtherSetting"_s;

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
    OtherSetting(const QString& type, const QString& name);

//-Class Functions--------------------------------------------------------------------------------------------------
public:
    static QUuid equivalentId(const QString& type, const QString& name);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString type() const;
    QString name() const;
    QList<ContentLine> contents() const;
};

class OtherSetting::Builder : public Lr::Builder<OtherSetting>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Builder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    Builder& wTypeAndName(const QString& name, const QString& type);
    Builder& wContent(const OtherSetting::ContentLine& line);
};

}
#endif // AM_CONFIG_ITEMS_H
