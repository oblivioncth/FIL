// Unit Includes
#include "progresspresenter.h"

//===============================================================================================================
// ProgressPresenter
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
ProgressPresenter::ProgressPresenter(QWidget* parent) :
    QObject(parent),
#ifdef _WIN32
    mButton(parent),
#endif
    mDialog(parent)
{
    setupProgressDialog();

}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
void ProgressPresenter::setupProgressDialog()
{
    // Initialize dialog
    mDialog.setCancelButtonText(u"Cancel"_s);
    mDialog.setWindowModality(Qt::WindowModal);
    mDialog.setWindowTitle(CAPTION_IMPORTING);
    mDialog.setWindowFlags(mDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    mDialog.setAutoReset(false);
    mDialog.setAutoClose(false);
    mDialog.setMinimumDuration(0); // Always show pd
    mDialog.reset(); // Stops the auto-show timer that is started by QProgressDialog's ctor
    connect(&mDialog, &QProgressDialog::canceled, this, &ProgressPresenter::canceled);
}

//Public:
void ProgressPresenter::attachWindow(QWindow* window)
{
#ifdef _WIN32
    mButton.setWindow(window);
#else
    Q_UNUSED(window);
#endif
}

void ProgressPresenter::setErrorState()
{
#ifdef _WIN32
    // Get taskbar progress and indicate error
    mButton.setProgressState(Qx::TaskbarButton::Stopped);
#endif
}

void ProgressPresenter::setBusyState()
{
#ifdef _WIN32
    // Get taskbar progress and indicate error
    mButton.setProgressState(Qx::TaskbarButton::Busy);
#endif
}

void ProgressPresenter::resetState()
{
#ifdef _WIN32
    // Clear taskbar error
    mButton.setProgressState(Qx::TaskbarButton::Normal);
#endif
}

void ProgressPresenter::reset()
{
    mDialog.close();
#ifdef _WIN32
    mButton.resetProgress();
    mButton.setProgressState(Qx::TaskbarButton::Hidden);
#endif
}

//-Slots---------------------------------------------------------------------------------------------------------
//Public:
void ProgressPresenter::setLabelText(const QString& text) { mDialog.setLabelText(text); }

void ProgressPresenter::setValue(int value)
{
    mDialog.setValue(value);
#ifdef _WIN32
    mButton.setProgressValue(value);
#endif
}

void ProgressPresenter::setMaximum(int max)
{
    mDialog.setMaximum(max);
#ifdef _WIN32
    mButton.setProgressMaximum(max);
#endif
}

void ProgressPresenter::setMinimum(int min)
{
    mDialog.setMinimum(min);
#ifdef _WIN32
    mButton.setProgressMinimum(min);
#endif
}
