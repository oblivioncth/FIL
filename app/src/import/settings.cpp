// Unit Include
#include "settings.h"

namespace Import
{

//===============================================================================================================
// ImageSources
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
ImagePaths::ImagePaths() {}
ImagePaths::ImagePaths(const QString& logoPath, const QString& screenshotPath) :
    mLogoPath(logoPath),
    mScreenshotPath(screenshotPath)
{}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Public:
bool ImagePaths::isNull() const { return mLogoPath.isEmpty() && mScreenshotPath.isEmpty(); }
QString ImagePaths::logoPath() const { return mLogoPath; }
QString ImagePaths::screenshotPath() const { return mScreenshotPath; }
void ImagePaths::setLogoPath(const QString& path) { mLogoPath = path; }
void ImagePaths::setScreenshotPath(const QString& path) { mScreenshotPath = path; }

}
