#ifndef PROGRESSPRESENTER_H
#define PROGRESSPRESENTER_H

// Qt Includes
#include <QProgressDialog>

// Qx Includes
#ifdef _WIN32
#include <qx/windows-gui/qx-taskbarbutton.h>
#endif

using namespace Qt::StringLiterals;

class ProgressPresenter : public QObject
{
    Q_OBJECT
//-Class Variables--------------------------------------------------------------------------------------------
private:
        static inline const QString CAPTION_IMPORTING = u"FP Import"_s;

//-Instance Variables--------------------------------------------------------------------------------------------
private:
#ifdef _WIN32
        Qx::TaskbarButton mButton;
#endif
    QProgressDialog mDialog;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    ProgressPresenter(QWidget* parent);

//-Instance Functions--------------------------------------------------------------------------------------------
private:
    void setupProgressDialog();

public:
    void attachWindow(QWindow* window);
    void setErrorState();
    void setBusyState();
    void resetState();
    void reset();

//-Slots---------------------------------------------------------------------------------------------------------
public slots:
    void setLabelText(const QString& text);
    void setValue(int value);
    void setMinimum(int min);
    void setMaximum(int max);

//-Signals---------------------------------------------------------------------------------------------------------
signals:
    void canceled();
};

#endif // PROGRESSPRESENTER_H
