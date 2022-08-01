#ifndef AM_SETTINGS_DATA_H
#define AM_SETTINGS_DATA_H

// Project Includes
#include "am-data.h"
#include "am-settings-items.h"

namespace Am
{

/* This setup of AM config parsing basically inverts the approach of the standard Fe items in that instead of using a builder
 * to work on an in-progress item and then building the item when done and finally adding it to its destination, instead the
 * item is constructed in a default state first and immediately added to its destination and then what parses the item works
 * on it in-place. This is basically required due to the need for polymorphism, itself needed due to how the AM config is
 * written, as once an item's builder get assigned to a base class pointer there would be no way of knowing where to place
 * the built object once finished without either assigning function pointers to the builder or doing dynamic conversion tests
 * for all types (since at that point it's not known from the reader's perspective what specific builder it is), both of which
 * are really hacky/jank and should be avoided.
 *
 * In the end this approach is similar to how AM itself reads attract.cfg and so it is for the best despite the slight consistency
 * break with the Fe defaults. It's probably a good thing that different frontend implementations can be this flexible anyway.
 *
 * TODO: This also acts as an experiment for having an item's "creator" (in this case its a builder/parser combo, but in the rest of
 * FIL it would just be the builder) exist as a inner class of the item (see other todos). So, if this goes well then we should move
 * on changing the rest of the codebase to do the same where possible.
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
    virtual bool parse(QString key, QString value, int depth) = 0;
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
    bool parse(QString key, QString value, int depth) override;
};

class DisplayFilter::Parser : public SettingParser<DisplayFilter>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Parser(DisplayFilter* display);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    bool parse(QString key, QString value, int depth) override;
};

class Display::Parser : public SettingParser<Display>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Parser(Display* display);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    bool parse(QString key, QString value, int depth) override;
};

class OtherSetting::Parser : public SettingParser<OtherSetting>
{
//-Constructor-------------------------------------------------------------------------------------------------
public:
    Parser(OtherSetting* otherSetting);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    bool parse(QString key, QString value, int depth) override;
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
        static inline const QString DISPLAY = "display";
        class Display
        {
        public:
            static inline const QString LAYOUT = "layout";
            static inline const QString ROMLIST = "romlist";
            static inline const QString IN_CYCLE = "in_cycle";
            static inline const QString IN_MENU = "in_menu";
            static inline const QString GLOBAL_FILTER = "global_filter";
            class GlobalFilter
            {
            public:
                static inline const QString RULE = "rule";
                static inline const QString EXCEPTION = "exception";
            };


            static inline const QString FILTER = "filter";
            class Filter
            {
            public:
                static inline const QString RULE = "rule";
                static inline const QString EXCEPTION = "exception";
                static inline const QString SORT_BY = "sort_by";
                static inline const QString REVERSE_ORDER = "reverse_order";
                static inline const QString LIST_LIMIT = "list_limit";
            };

        };
    };

//-Class Variables-----------------------------------------------------------------------------------------------------
public:
    static inline const QString STD_NAME = "attract";

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

    bool containsDisplay(QString name);
    void addDisplay(const Display& display);
    Display& display(QString name);

    bool containsOtherSetting(QString type, QString name);
    void addOtherSetting(const OtherSetting& setting);
};

class CrudeSettingsReader : public ConfigDocReader
{
//-Class Variables--------------------------------------------------------------------------------------------------
private:
    static inline const QString UNKNOWN_KEY_ERROR = R"(Unknown key "%1" for setting "%2".)";

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
    Qx::GenericError readTargetDoc() override;
    void initializeGenericSubSetting(QString key, QString value);
};

class CrudeSettingsWriter : public ConfigDocWriter
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
