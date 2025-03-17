// Unit Include
#include "mainwindow.h"
#include "ui_mainwindow.h"

// Qt Includes
#include <QApplication>
#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QShowEvent>

// Qx Includes
#include <qx/widgets/qx-treeinputdialog.h>
#include <qx/gui/qx-color.h>

// Magic Enum Includes
#include <magic_enum_utility.hpp>

// Project Includes
#include "import/properties.h"
#include "launcher/abstract/lr-registration.h"
#include "kernel/clifp.h"

//===============================================================================================================
// MainWindow::SelectionList
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
MainWindow::SelectionList::SelectionList(QListWidget* widget) :
    mWidget(widget)
{
    connect(mWidget, &QListWidget::itemChanged, mWidget, [this](QListWidgetItem* item){ handleCheckChange(item); });
}

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
void MainWindow::SelectionList::handleCheckChange(QListWidgetItem* item)
{
    bool checked = item->checkState() == Qt::Checked;
    int selCount = mSelCount.valueBypassingBindings();
    if(checked){ ++selCount; } else { --selCount; }
    mSelCount = selCount;

    // Handle existing special case
    QVariant vExisting = item->data(USER_ROLE_EXISTING);
    if(vExisting.isValid() && vExisting.toBool())
    {
        int eSelCount = mExistSelCount.valueBypassingBindings();
        if(checked){ ++eSelCount; } else { --eSelCount; }
        mExistSelCount = eSelCount;
    }
}

//Public:
void MainWindow::SelectionList::fill(const QList<Import::Importee>& imps)
{
    // Fill list widget
    for(const auto& imp : imps)
    {
        // Configure item before adding to avoid unwanted triggering of handleCheckChange()
        auto* item = new QListWidgetItem(imp.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);

        if(imp.existing)
        {
            item->setBackground(QBrush(smExistingItemColor));
            item->setData(USER_ROLE_EXISTING, true);
        }
        else
            item->setData(USER_ROLE_EXISTING, false);

        mWidget->addItem(item);
    }
}

void MainWindow::SelectionList::clear()
{
    mSelCount = 0;
    mExistSelCount = 0;
    mWidget->clear();
}

int MainWindow::SelectionList::selectedCount() const { return mSelCount; }
int MainWindow::SelectionList::existingSelectedCount() const { return mExistSelCount; }

//===============================================================================================================
// MainWindow
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
MainWindow::MainWindow(const Import::Properties& importProperties, QWidget* parent) :
    QMainWindow(parent),
    ui([this]{ auto ui = new Ui::MainWindow; ui->setupUi(this); return ui;}()),
    mPlatformSelections(ui->listWidget_platformChoices),
    mPlaylistSelections(ui->listWidget_playlistChoices),
    mImportProperties(importProperties),
    mImageModeMap(initializeImageModeMap()),
    mPlaylistGameModeMap(initializePlaylistGameModeMap()),
    mUpdateModeMap(initializeUpdateModeMap())
{
    // Prepare tag model
    mTagModel.setAutoTristate(true);
    mTagModel.setSortRole(Qt::DisplayRole);

    // General setup
    setWindowTitle(QApplication::applicationName());
    initializeForms();
    initializeLauncherHelpActions();
    initializeBindings();
}

//-Destructor----------------------------------------------------------------------------------------------------
MainWindow::~MainWindow() { delete ui; }

//-Instance Functions--------------------------------------------------------------------------------------------
//Private:
void MainWindow::initializeForms()
{
    // Capture existing item color from label for use in platform/playlist selection lists
    smExistingItemColor = ui->label_existingItemColor->palette().color(QPalette::Window);

    // Add CLIFp version to deploy option
    ui->action_deployCLIFp->setText(ui->action_deployCLIFp->text() + ' ' + CLIFp::internalVersion().normalized(2).toString());

    // Prepare help messages
    mArgedPlaylistGameModeHelp = MSG_PLAYLIST_GAME_MODE_HELP.arg(ui->radioButton_selectedPlatformsOnly->text(),
                                                                 ui->radioButton_forceAll->text());

    mArgedUpdateModeHelp = MSG_UPDATE_MODE_HELP.arg(ui->radioButton_onlyAdd->text(),
                                                    ui->radioButton_updateExisting->text(),
                                                    ui->checkBox_removeMissing->text());

    mArgedImageModeHelp = MSG_IMAGE_MODE_HELP.arg(ui->radioButton_copy->text(),
                                                   ui->radioButton_reference->text(),
                                                   ui->radioButton_link->text());

    // If no link permissions, inform user
    if(!mImportProperties.hasLinkPermissions())
        ui->radioButton_link->setText(ui->radioButton_link->text().append(REQUIRE_ELEV));
}

Qx::Bimap<Import::ImageMode, QRadioButton*> MainWindow::initializeImageModeMap() const
{
    return{
        {Import::ImageMode::Link, ui->radioButton_link},
        {Import::ImageMode::Copy, ui->radioButton_copy},
        {Import::ImageMode::Reference, ui->radioButton_reference},
    };
}

QHash<QRadioButton*, Import::PlaylistGameMode> MainWindow::initializePlaylistGameModeMap() const
{
    return{
        {ui->radioButton_selectedPlatformsOnly, Import::PlaylistGameMode::SelectedPlatform},
        {ui->radioButton_forceAll, Import::PlaylistGameMode::ForceAll}
    };
}

QHash<QRadioButton*, Import::UpdateMode> MainWindow::initializeUpdateModeMap() const
{
    return{
        {ui->radioButton_onlyAdd, Import::UpdateMode::OnlyNew},
        {ui->radioButton_updateExisting, Import::UpdateMode::NewAndExisting}
    };
}

void MainWindow::initializeBindings()
{
    Bindings& b = mBindings;

    // Import mode
    b.forceAllModeChecked = Qx::Bindable<bool>(ui->radioButton_forceAll, "checked");

    // Indirect Enabled
    mImportProperties.bindableImageModeOrder().addLifetimeNotifier([&]{
        if(!mImportProperties.isLauncherReady())
            return;

        auto validModes = mImportProperties.imageModeOrder();
        Q_ASSERT(!validModes.isEmpty());

        // Move selection to valid option if no longer valid
        if(!validModes.contains(getSelectedImageMode()))
            mImageModeMap.from(validModes.front())->setChecked(true);

        // Disable invalid mode buttons
        for(const auto[mode, button] : mImageModeMap)
                button->setEnabled(validModes.contains(mode));
    });

    // Enabled
    b.importSelectionEnabled.setBinding([&]{ return mImportProperties.isLauncherReady() && mImportProperties.isFlashpointReady(); });
    b.importSelectionEnabled.subscribeLifetime([&]{ ui->groupBox_importSelection->setEnabled(b.importSelectionEnabled); });
    b.playlistGameModeEnabled.setBinding([&]{ return mPlaylistSelections.selectedCount() > 0; });
    b.playlistGameModeEnabled.subscribeLifetime([&]{ ui->groupBox_playlistGameMode->setEnabled(b.playlistGameModeEnabled); });
    b.updateModeEnabled.setBinding([&]{ return selectionsMayModify(); });
    b.updateModeEnabled.subscribeLifetime([&]{ ui->groupBox_updateMode->setEnabled(b.updateModeEnabled); });
    b.imageModeEnabled.setBinding([&]{ return mImportProperties.isLauncherReady() && mImportProperties.isFlashpointReady(); });
    b.imageModeEnabled.subscribeLifetime([&]{ ui->groupBox_imageMode->setEnabled(b.imageModeEnabled); });
    b.startImportEnabled.setBinding([&]{
        return mPlatformSelections.selectedCount() > 0 || (getSelectedPlaylistGameMode() == Import::PlaylistGameMode::ForceAll && mPlaylistSelections.selectedCount() > 0);
    });
    b.startImportEnabled.subscribeLifetime([&]{ ui->pushButton_startImport->setEnabled(b.startImportEnabled); });
    b.forceDownloadImagesEnabled.setBinding([&]{ return mImportProperties.isImageDownloadable(); });
    b.forceDownloadImagesEnabled.subscribeLifetime([&]{
        bool e = b.forceDownloadImagesEnabled;
        ui->action_forceDownloadImages->setEnabled(e);
        if(!e)
            ui->action_forceDownloadImages->setChecked(false);
    });
    b.editTagFilterEnabled.setBinding([&]{ return mImportProperties.isFlashpointReady(); });
    b.editTagFilterEnabled.subscribeLifetime([&]{ ui->action_editTagFilter->setEnabled(b.editTagFilterEnabled); });

    // Label text
    b.launcherVersion.setBinding([&]{ return mImportProperties.launcherInfo(); });
    b.launcherVersion.subscribeLifetime([&]{ ui->label_launcherVersion->setText(b.launcherVersion); });
    b.flashpointVersion.setBinding([&]{ return mImportProperties.flashpointInfo(); });
    b.flashpointVersion.subscribeLifetime([&]{ ui->label_flashpointVersion->setText(b.flashpointVersion); });

    // Icon
    b.launcherStatus.setBinding([&]{
        return QPixmap(!b.launcherPathTouched ? u":/ui/No_Install.png"_s :
                       !mImportProperties.isLauncherReady() ? u":/ui/Invalid_Install.png"_s :
                       u":/ui/Valid_Install.png"_s);
    });
    b.launcherStatus.subscribeLifetime([&]{ ui->icon_launcher_install_status->setPixmap(b.launcherStatus); });

    b.flashpointStatus.setBinding([&]{
        return QPixmap(!b.flashpointPathTouched ? u":/ui/No_Install.png"_s :
                       !mImportProperties.isFlashpointReady() ? u":/ui/Invalid_Install.png"_s :
                       !mImportProperties.isFlashpointTargetSeries() ? u":/ui/Mismatch_Install.png"_s :
                       u":/ui/Valid_Install.png"_s);
    });
    b.flashpointStatus.subscribeLifetime([&]{ ui->icon_flashpoint_install_status->setPixmap(b.flashpointStatus); });

    // Tag map
    mImportProperties.bindableTagMap().subscribeLifetime([&]{
        // Populate model
        const auto tagMap = mImportProperties.tagMap();
        if(tagMap.isEmpty())
        {
            mTagModel.clear();
            return;
        }

        QStandardItem* modelRoot = mTagModel.invisibleRootItem();

        // Add root tag categories
        for(const Fp::Db::TagCategory& tc : tagMap)
        {
            QStandardItem* rootItem = new QStandardItem(QString(tc.name));
            rootItem->setData(QBrush(tc.color), Qt::BackgroundRole);
            rootItem->setData(QBrush(Qx::Color::textFromBackground(tc.color)), Qt::ForegroundRole);
            rootItem->setCheckState(Qt::CheckState::Checked);
            rootItem->setCheckable(true);

            // Add child tags
            for(const Fp::Db::Tag& tag : tc.tags)
            {
                QStandardItem* childItem = new QStandardItem(QString(tag.primaryAlias));
                childItem->setData(tag.id, USER_ROLE_TAG_ID);
                childItem->setCheckState(Qt::CheckState::Checked);
                childItem->setCheckable(true);

                rootItem->appendRow(childItem);
            }

            modelRoot->appendRow(rootItem);
        }

        // Sort
        mTagModel.sort(0);
    });

    // List widget items
    mImportProperties.bindablePlatforms().subscribeLifetime([&]{
        // Always clear any existing
        mPlatformSelections.clear();

        auto platforms = mImportProperties.platforms();
        if(!platforms.isEmpty())
            mPlatformSelections.fill(platforms);
    });
    mImportProperties.bindablePlaylists().subscribeLifetime([&]{
        // Always clear any existing
        mPlaylistSelections.clear();

        auto playlists = mImportProperties.playlists();
        if(!playlists.isEmpty())
            mPlaylistSelections.fill(playlists);
    });
}

void MainWindow::initializeLauncherHelpActions()
{
    // Add install help link for each registered install
    for(auto eItr = Lr::Registry::entries(); eItr.hasNext();)
    {
        auto e = eItr.next();
        QAction* lrHelpAction = new QAction(ui->menu_launcherHelp);
        lrHelpAction->setObjectName(MENU_LR_HELP_OBJ_NAME_TEMPLATE.arg(e.key()));
        lrHelpAction->setText(e.key().toString());
        lrHelpAction->setIcon(QIcon(e.value().iconPath.toString()));
        ui->menu_launcherHelp->addAction(lrHelpAction);
    }
}
bool MainWindow::selectionsMayModify()
{
    if(mPlatformSelections.existingSelectedCount() > 0 || mPlaylistSelections.existingSelectedCount() > 0)
        return true;

    auto plats = mImportProperties.platforms();
    return std::any_of(plats.cbegin(), plats.cend(), [](const Import::Importee& i){
       return i.existing; // Checks if any platform is existing at all
    });
}

QStringList MainWindow::getSelectedPlatforms() const
{
    QStringList selectedPlatforms;

    for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
        if(ui->listWidget_platformChoices->item(i)->checkState() == Qt::Checked)
            selectedPlatforms.append(ui->listWidget_platformChoices->item(i)->text());

    return selectedPlatforms;
}

QStringList MainWindow::getSelectedPlaylists() const
{
    QStringList selectedPlaylists;

    for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
        if(ui->listWidget_playlistChoices->item(i)->checkState() == Qt::Checked)
            selectedPlaylists.append(ui->listWidget_playlistChoices->item(i)->text());

    return selectedPlaylists;
}

Import::PlaylistGameMode MainWindow::getSelectedPlaylistGameMode() const
{
    /* This is a trick to make UI options that depend on which playlist game mode
     * is selected dependent on the relevant button(s). Qx::ButtonGroup would be better
     * but the UI designer cannot use custom button groups and we don't want to give that
     * up.
     */
    Q_UNUSED(mBindings.forceAllModeChecked.value());

    QRadioButton* sel = static_cast<QRadioButton*>(ui->buttonGroup_playlistGameMode->checkedButton());
    Q_ASSERT(sel && mPlaylistGameModeMap.contains(sel));
    return mPlaylistGameModeMap[sel];
}

Fp::Db::InclusionOptions MainWindow::getSelectedInclusionOptions() const
{
    return {generateTagExlusionSet(), ui->action_includeAnimations->isChecked()};
}

Import::UpdateOptions MainWindow::getSelectedUpdateOptions() const
{
    QRadioButton* sel = static_cast<QRadioButton*>(ui->buttonGroup_updateMode->checkedButton());
    Q_ASSERT(sel && mUpdateModeMap.contains(sel));
    return {mUpdateModeMap[sel], ui->checkBox_removeMissing->isChecked()};
}

Import::ImageMode MainWindow::getSelectedImageMode() const
{
    QRadioButton* sel = static_cast<QRadioButton*>(ui->buttonGroup_imageMode->checkedButton());
    Q_ASSERT(sel && mImageModeMap.containsRight(sel));
    return mImageModeMap.from(sel);
}

bool MainWindow::getForceDownloadImages() const
{
    return ui->action_forceDownloadImages->isChecked();
}

void MainWindow::prepareImport()
{
    // Gather selection's and notify controller
    Import::Selections impSel{.platforms = getSelectedPlatforms(),
                              .playlists =getSelectedPlaylists()};
    Import::OptionSet optSet{
        getSelectedUpdateOptions(),
        getSelectedImageMode(),
        getForceDownloadImages(),
        getSelectedPlaylistGameMode(),
        getSelectedInclusionOptions()
    };

    emit importTriggered(impSel, optSet, selectionsMayModify());
}

void MainWindow::showTagSelectionDialog()
{
    // Ensure tags have been populated
    Q_ASSERT(mTagModel.rowCount() > 0);

    // Cache current selection states
    QHash<QStandardItem*,Qt::CheckState> originalCheckStates;
    mTagModel.forEachItem([&](QStandardItem* item) { originalCheckStates[item] = item->checkState(); });

    // Create dialog
    Qx::TreeInputDialog tagSelectionDialog(this);
    tagSelectionDialog.setModel(&mTagModel);
    tagSelectionDialog.setWindowFlags(tagSelectionDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    tagSelectionDialog.setWindowTitle(CAPTION_TAG_FILTER);
    connect(&tagSelectionDialog, &Qx::TreeInputDialog::selectNoneClicked, &mTagModel, &Qx::StandardItemModel::selectNone);
    connect(&tagSelectionDialog, &Qx::TreeInputDialog::selectAllClicked, &mTagModel, &Qx::StandardItemModel::selectAll);

    // Present dialog and capture commitment choice
    int dc = tagSelectionDialog.exec();

    // If new selections were canceled, restore previous ones
    if(dc == QDialog::Rejected)
        mTagModel.forEachItem([&](QStandardItem* item) { item->setCheckState(originalCheckStates[item]); });
}

QSet<int> MainWindow::generateTagExlusionSet() const
{
    QSet<int> exclusionSet;

    mTagModel.forEachItem([&exclusionSet](QStandardItem* item){
        if(item->data(USER_ROLE_TAG_ID).isValid() && item->checkState() == Qt::Unchecked)
            exclusionSet.insert(item->data(USER_ROLE_TAG_ID).toInt());
    });

    return exclusionSet;
}

//-Slots---------------------------------------------------------------------------------------------------------
//Private:
void MainWindow::all_on_action_triggered()
{
    // Get the object that called this slot
    QAction* senderAction = qobject_cast<QAction *>(sender());

    // Ensure the signal that triggered this slot belongs to the above class by checking for null pointer
    if(senderAction == nullptr)
        throw std::runtime_error("Pointer conversion to action failed");

    // Determine sender and take corresponding action
    if(senderAction == ui->action_exit)
        QApplication::quit();
    else if(senderAction == ui->action_deployCLIFp)
        emit standaloneDeployTriggered();
    else if(senderAction == ui->action_goToCLIFpGitHub)
        QDesktopServices::openUrl(URL_CLIFP_GITHUB);
    else if(senderAction == ui->action_goToFILGitHub)
        QDesktopServices::openUrl(URL_FIL_GITHUB);
    else if(senderAction == ui->action_about)
        QMessageBox::information(this, QApplication::applicationName(), MSG_ABOUT);
    else if(senderAction == ui->action_editTagFilter)
        showTagSelectionDialog();
    else
        throw std::runtime_error("Unhandled use of all_on_action_triggered() slot");
}

void MainWindow::all_on_pushButton_clicked()
{
    // Get the object that called this slot
    QPushButton* senderPushButton = qobject_cast<QPushButton *>(sender());
    Q_ASSERT(senderPushButton);

    // Determine sender and take corresponding action
    if(senderPushButton == ui->pushButton_launcherBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_LAUNCHER_BROWSE,
                                                                (QFileInfo::exists(ui->lineEdit_launcherPath->text()) ? ui->lineEdit_launcherPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            /* TODO: Here, and below, we simulate the user entering this text manually into the box so that
             * the text is placed there and the editingFinished() signal is emitted; however, this hinges on
             * a quirk of QLineEdit that I'm looking to remove in a patch in that the editingFinished() signal
             * is emitted on focus lost if the text is different from when the signal was last emitted, even if
             * the text was put there programatically. IMO it should only include user edits, as QWidgets usually
             * use the word "changed" to mean any change and "edited" to mean user changes only. So, when that patch
             * goes through this will need tweaking
             */
            ui->lineEdit_launcherPath->setFocus();
            ui->lineEdit_launcherPath->setText(QDir::toNativeSeparators(selectedDir));
            ui->pushButton_launcherBrowse->setFocus(); // Return focus to browse button
        }
    }
    else if(senderPushButton == ui->pushButton_flashpointBrowse)
    {
        QString selectedDir = QFileDialog::getExistingDirectory(this, CAPTION_FLASHPOINT_BROWSE,
                                                                (QFileInfo::exists(ui->lineEdit_flashpointPath->text()) ? ui->lineEdit_flashpointPath->text() : QDir::currentPath()));

        if(!selectedDir.isEmpty())
        {
            ui->lineEdit_flashpointPath->setFocus();
            ui->lineEdit_flashpointPath->setText(QDir::toNativeSeparators(selectedDir));
            ui->pushButton_launcherBrowse->setFocus(); // Return focus to browse button
        }
    }
    else if(senderPushButton == ui->pushButton_selectAll_platforms)
    {
        for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
            ui->listWidget_platformChoices->item(i)->setCheckState(Qt::Checked);
    }
    else if(senderPushButton == ui->pushButton_selectNone_platforms)
    {
        for(int i = 0; i < ui->listWidget_platformChoices->count(); i++)
            ui->listWidget_platformChoices->item(i)->setCheckState(Qt::Unchecked);
    }
    else if(senderPushButton == ui->pushButton_selectAll_playlists)
    {
        for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
            ui->listWidget_playlistChoices->item(i)->setCheckState(Qt::Checked);
    }
    else if(senderPushButton == ui->pushButton_selectNone_playlists)
    {
        for(int i = 0; i < ui->listWidget_playlistChoices->count(); i++)
            ui->listWidget_playlistChoices->item(i)->setCheckState(Qt::Unchecked);
    }
    else if(senderPushButton == ui->pushButton_playlistGameModeHelp)
        QMessageBox::information(this, CAPTION_PLAYLIST_GAME_MODE_HELP, mArgedPlaylistGameModeHelp);
    else if(senderPushButton == ui->pushButton_updateModeHelp)
        QMessageBox::information(this, CAPTION_UPDATE_MODE_HELP, mArgedUpdateModeHelp);
    else if(senderPushButton == ui->pushButton_imageModeHelp)
        QMessageBox::information(this, CAPTION_IMAGE_MODE_HELP, mArgedImageModeHelp);
    else if(senderPushButton == ui->pushButton_startImport)
        prepareImport();
    else if(senderPushButton == ui->pushButton_exit)
        QApplication::quit();
    else
        qFatal("Unhandled use of all_on_pushButton_clicked() slot");
}

void MainWindow::all_on_lineEdit_editingFinished()
{
    // Get the object that called this slot
    QLineEdit* senderLineEdit = qobject_cast<QLineEdit*>(sender());
    Q_ASSERT(senderLineEdit);

    // Determine sender and take corresponding action
    if(senderLineEdit == ui->lineEdit_launcherPath)
    {
        mBindings.launcherPathTouched = true;
        emit installPathChanged(ui->lineEdit_launcherPath->text(), Import::Install::Launcher);
    }
    else if(senderLineEdit == ui->lineEdit_flashpointPath)
    {
        mBindings.flashpointPathTouched = true;
        emit installPathChanged(ui->lineEdit_flashpointPath->text(), Import::Install::Flashpoint);
    }
    else
        qFatal("Unhandled use of all_on_linedEdit_textEdited() slot");
}

void MainWindow::all_on_menu_triggered(QAction *action)
{
    // Get the object that called this slot
    QMenu* senderMenu = qobject_cast<QMenu*>(sender());
    Q_ASSERT(senderMenu);

    if(senderMenu == ui->menu_launcherHelp)
    {
        // Get associated help URL and open it
        QRegularExpressionMatch launcherMatch = MENU_LR_HELP_KEY_REGEX.match(action->objectName());

        if(launcherMatch.hasMatch())
        {
            QString launcherName = launcherMatch.captured(u"launcher"_s);
            QUrl helpUrl = Lr::Registry::helpUrl(launcherName);
            if(helpUrl.isValid())
            {
                QDesktopServices::openUrl(helpUrl);
                return;
            }
        }

        qWarning("Launcher help action name could not be determined.");
    }
    else
        qFatal("Unhandled use of all_on_menu_triggered() slot");
}




