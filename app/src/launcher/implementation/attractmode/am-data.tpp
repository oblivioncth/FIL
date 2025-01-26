#ifndef ATTRACTMODE_DATA_TPP
#define ATTRACTMODE_DATA_TPP

#include "am-data.h" // Ignore recursive error, doesn't actually cause problem

#ifndef ATTRACTMODE_DATA_H
#error __FILE__ should only be included from am-data.h.
#endif // ATTRACTMODE_DATA_H

namespace Am
{

//===============================================================================================================
// CommonDocReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
CommonDocReader<DocT>::CommonDocReader(DocT* targetDoc) :
    Lr::DataDocReader<DocT>(targetDoc),
    mStreamReader(targetDoc->path())
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
bool CommonDocReader<DocT>::lineIsComment(const QString& line) { return line.front() == '#'; }

template<class DocT>
QString CommonDocReader<DocT>::readLineIgnoringComments(qint64 maxlen)
{
    QString line;

    do
        line = mStreamReader.readLine(maxlen);
    while(!line.isEmpty() && line.front() == '#'); // Must check for empty string due to QString::front() constraints

    return line;
}

//Public:
template<class DocT>
Lr::DocHandlingError CommonDocReader<DocT>::readInto()
{
    // Open file
    Qx::IoOpReport openError =  mStreamReader.openFile();
    if(openError.isFailure())
        return Lr::DocHandlingError(*target(), Lr::DocHandlingError::DocCantOpen, openError.outcomeInfo());

    // Check that doc is valid
    bool isValid = false;
    if(!checkDocValidity(isValid))
        return Lr::DocHandlingError(*target(), Lr::DocHandlingError::DocWriteFailed, mStreamReader.status().outcomeInfo());
    else if(!isValid)
        return Lr::DocHandlingError(*target(), Lr::DocHandlingError::DocInvalidType);

    // Read doc
    Lr::DocHandlingError parseError = readTargetDoc();

    // Close file
    mStreamReader.closeFile();

    // Return outcome
    if(parseError.isValid())
        return parseError;
    else if(mStreamReader.hasError())
        return Lr::DocHandlingError(*target(), Lr::DocHandlingError::DocWriteFailed, mStreamReader.status().outcomeInfo());
    else
        return Lr::DocHandlingError();
}

//===============================================================================================================
// CommonDocWriter
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
CommonDocWriter<DocT>::CommonDocWriter(DocT* sourceDoc) :
    Lr::DataDocWriter<DocT>(sourceDoc),
    mStreamWriter(sourceDoc->path(), Qx::WriteMode::Truncate)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
template<class DocT>
Lr::DocHandlingError CommonDocWriter<DocT>::writeOutOf()
{
    // Open file
    Qx::IoOpReport openError =  mStreamWriter.openFile();
    if(openError.isFailure())
        return Lr::DocHandlingError(*source(), Lr::DocHandlingError::DocCantOpen, openError.outcomeInfo());

    // Write doc
    bool writeSuccess = writeSourceDoc();

    // Close file
    mStreamWriter.closeFile();

    // Return outcome
    return writeSuccess ? Lr::DocHandlingError() :
               Lr::DocHandlingError(*source(), Lr::DocHandlingError::DocWriteFailed, mStreamWriter.status().outcomeInfo());
}

//===============================================================================================================
// ConfigDoc::Reader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
ConfigDoc::Reader<DocT>::Reader(DocT* targetDoc) :
    CommonDocReader<DocT>(targetDoc)
{}

//-Class Functions-------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
bool ConfigDoc::Reader<DocT>::splitKeyValue(const QString& line, QString& key, QString& value)
{
    /* TODO: The result from this function is currently unused due to no easy way to raise a custom
     * error with the stream reader in this class (and how the current paradigm is to return bools
     * for each step and then use the reader status if one is found). If used properly this should
     * never error, but ideally it should be checked for anyway. Might need to have all read functions
     * return Qx::GenericError to allow non stream related errors to be returned.
     */

    // Null out return buffers
    key = QString();
    value = QString();

    QRegularExpressionMatch keyValueCheck = KEY_VALUE_REGEX.match(line);
    if(keyValueCheck.hasMatch())
    {
        key = keyValueCheck.captured(u"key"_s);
        value = keyValueCheck.captured(u"value"_s);
        return true;
    }
    else
    {
        qWarning("Invalid key value string");
        return false;
    }
}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Protected:
template<class DocT>
bool ConfigDoc::Reader<DocT>::checkDocValidity(bool& isValid)
{
    // Check for config "header"
    QString firstLine = mStreamReader.readLine();
    QString secondLine = mStreamReader.readLine();

    bool hasTagline = firstLine.left(ConfigDoc::TAGLINE.length()) == ConfigDoc::TAGLINE;

    isValid = hasTagline && lineIsComment(secondLine);

    // Return status
    return !mStreamReader.hasError();
}

//===============================================================================================================
// ConfigDoc::Writer
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
template<class DocT>
ConfigDoc::Writer<DocT>::Writer(DocT* sourceDoc) :
    CommonDocWriter<DocT>(sourceDoc)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
template<class DocT>
bool ConfigDoc::Writer<DocT>::writeSourceDoc()
{
    // Write config doc "header"
    mStreamWriter.writeLine(source()->versionedTagline());
    mStreamWriter.writeLine(u"#"_s);

    if(mStreamWriter.hasError())
        return false;

    // Perform custom writing
    return writeConfigDoc();
}

}

#endif // ATTRACTMODE_DATA_TPP
