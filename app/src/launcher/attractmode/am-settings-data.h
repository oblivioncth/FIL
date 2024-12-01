#ifndef AM_SETTINGS_DATA_H
#define AM_SETTINGS_DATA_H

// Project Includes
#include "launcher/attractmode/am-data.h"
#include "launcher/attractmode/am-settings-items.h"

namespace Am
{

/* This setup of AM config parsing basically inverts the approach of the standard Lr items in that instead of using a builder
 * to work on an in-progress item and then building the item when done and finally adding it to its destination, instead the
 * item is constructed in a default state first and immediately added to its destination and then what parses the item works
 * on it in-place. This is basically required due to the need for polymorphism, itself needed due to how the AM config is
 * written, as once an item's builder get assigned to a base class pointer there would be no way of knowing where to place
 * the built object once finished without either assigning function pointers to the builder or doing dynamic conversion tests
 * for all types (since at that point it's not known from the reader's perspective what specific builder it is), both of which
 * are really hacky/jank and should be avoided.
 *
 * In the end this approach is similar to how AM itself reads attract.cfg and so it is for the best despite the slight consistency
 * break with the Lr defaults. It's probably a good thing that different launcher implementations can be this flexible anyway.
 */

class ISettingParser
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    ISettingParser();

//-Destructor-------------------------------------------------------------------------------------------------
public:
    virtual ~ISettingParser();
    /* NOTE: The first base class in a chain of inheritance should almost always have a virtual destructor.
     * This makes all derived class' destructors implicitly virtual as well (so those don't need to be
     * explicitly declared virtual). This allows for correct destruction when deleting a derived class
     * via a pointer to its base class.
     *
     * See https://stackoverflow.com/questions/461203/when-to-use-virtual-destructors
     */

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    virtual QString settingName() = 0;
    virtual bool parse(QStringView key, const QString& value, int depth) = 0;
};


template<typename T>
    requires std::derived_from<T, SettingsItem>
class SettingParser : public ISettingParser
{
//-Instance Variables-----------------------------------------------------------------------------------------------
protected:
    T* mSetting;
    std::unique_ptr<ISettingParser> mCurrentSubSettingParser;

//-Constructor-------------------------------------------------------------------------------------------------
protected:
    SettingParser(T* setting) :
        mSetting(setting)
    {}

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString settingName() override { return mSetting->settingName(); }
};

class DisplayGlobalFilter::Parser : public SettingParser<DisplayGlobalFilter>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Parser(DisplayGlobalFilter* display);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    bool parse(QStringView key, const QString& value, int depth) override;
};

class DisplayFilter::Parser : public SettingParser<DisplayFilter>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Parser(DisplayFilter* display);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    bool parse(QStringView key, const QString& value, int depth) override;
};

class Display::Parser : public SettingParser<Display>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Parser(Display* display);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    bool parse(QStringView key, const QString& value, int depth) override;
};

class OtherSetting::Parser : public SettingParser<OtherSetting>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Parser(OtherSetting* otherSetting);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    bool parse(QStringView key, const QString& value, int depth) override;
};

class CrudeSettings : public ConfigDoc
{
    friend class CrudeSettingsReader;
    friend class CrudeSettingsWriter;

//-Inner Classes-------------------------------------------------------------------------------------------------------
public:
    class Keys
    {
    public:
        static inline const QString DISPLAY = u"display"_s;
        class Display
        {
        public:
            static inline const QString LAYOUT = u"layout"_s;
            static inline const QString ROMLIST = u"romlist"_s;
            static inline const QString IN_CYCLE = u"in_cycle"_s;
            static inline const QString IN_MENU = u"in_menu"_s;
            static inline const QString GLOBAL_FILTER = u"global_filter"_s;
            class GlobalFilter
            {
            public:
                static inline const QString RULE = u"rule"_s;
                static inline const QString EXCEPTION = u"exception"_s;
            };


            static inline const QString FILTER = u"filter"_s;
            class Filter
            {
            public:
                static inline const QString RULE = u"rule"_s;
                static inline const QString EXCEPTION = u"exception"_s;
                static inline const QString SORT_BY = u"sort_by"_s;
                static inline const QString REVERSE_ORDER = u"reverse_order"_s;
                static inline const QString LIST_LIMIT = u"list_limit"_s;
            };

        };
    };

//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = u"attract"_s;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    // Using map because any sorting is better than nothing until a proper config parser is implemented
    QMap<QString, Display> mDisplays;
    QMap<QUuid, OtherSetting> mOtherSettings;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    explicit CrudeSettings(Install* const parent, const QString& filePath, const DocKey&);

//-Instance Functions--------------------------------------------------------------------------------------------------
public:
    bool isEmpty() const override;
    Type type() const override;

    bool containsDisplay(const QString& name);
    void addDisplay(const Display& display);
    Display& display(const QString& name);

    bool containsOtherSetting(const QString& type, const QString& name);
    void addOtherSetting(const OtherSetting& setting);
};

class CrudeSettingsReader : public ConfigDoc::Reader
{
//-Class Variables--------------------------------------------------------------------------------------------------
private:
    static inline const QString UNKNOWN_KEY_ERROR = uR"(Unknown key "%1" for setting "%2".)"_s;

//-Instance Variables--------------------------------------------------------------------------------------------------
private:
    std::unique_ptr<ISettingParser> mCurrentSubSettingParser;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    CrudeSettingsReader(CrudeSettings* targetDoc);

//-Class Functions-------------------------------------------------------------------------------------------------
private:
    int checkTabDepth(const QString& line);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    CrudeSettings* targetCrudeSettings() const;
    Lr::DocHandlingError readTargetDoc() override;
    void initializeGenericSubSetting(const QString& key, const QString& value);
};

class CrudeSettingsWriter : public ConfigDoc::Writer
{
//-Class Variables-------------------------------------------------------------------------------------------------
private:
   static const int KEY_FIELD_WIDTH = 21;

//-Instance Variables-------------------------------------------------------------------------------------------------
private:
   int mTabDepth;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    CrudeSettingsWriter(CrudeSettings* sourceDoc);

//-Instance Functions-------------------------------------------------------------------------------------------------
private:
    CrudeSettings* sourceCrudeSettings() const;
    void writeKeyValue(const QString& key, const QString& value = QString());

    bool writeConfigDoc() override;
    bool writeDisplay(const Display& display);
    bool writeDisplayGlobalFilter(const DisplayGlobalFilter& globalFilter);
    bool writeDisplayFilter(const DisplayFilter& filter);
    bool writeOtherSetting(const OtherSetting& setting);
};

}
#endif // AM_SETTINGS_DATA_H
