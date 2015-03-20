#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(Settings *settings, TTManager * ttManager, QWidget *parent) :
    QDialog(parent),
    m_Settings(settings),
    m_TTManager(ttManager),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    display();

    connect(ttManager, SIGNAL(ttArrived()), this, SLOT(display()));
    connect(ttManager, SIGNAL(ttRemoved()), this, SLOT(display()));

    connect(ui->enabledChecked, SIGNAL(clicked()), this, SLOT(onExporterChanged()));
    connect(ui->autoOpenCheckBox, SIGNAL(clicked()), this, SLOT(onExporterChanged()));

    connect(ui->autoDownloadCheckbox, SIGNAL(clicked()), this, SLOT(onSettingChanged()));


    ui->okButton->setEnabled(false);

    PreferencesMap::iterator i = m_TTManager->preferences().begin();
    for(;i!=m_TTManager->preferences().end();i++)
    {
        WatchPreferencesPtr wp = i.value();
        foreach( IActivityExporterPtr exp, wp->exporters())
        {
            connect(exp.data(), SIGNAL(setupFinished(IActivityExporter *,bool)), this, SLOT(onSetupFinished(IActivityExporter*,bool)));
        }
    }
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}



void SettingsDialog::display()
{
    ui->watchBox->clear();
    WatchPreferencesPtr defPref = m_TTManager->defaultPreferences();

    int index = 0;
    int selectIndex = -1;
    PreferencesMap::iterator i = m_TTManager->preferences().begin();
    for(;i!=m_TTManager->preferences().end();i++)
    {
        WatchPreferencesPtr p = i.value();

        // only allow preferences to be changed for a watch that is connected.

        if ( p->serial() == "DEFAULT" || m_TTManager->watch( p->serial() ) )
        {
            ui->watchBox->addItem( p->name(), p->serial() );
            if ( p != defPref )
            {
                selectIndex = index;
            }
            index++;
        }
    }

    if ( selectIndex>=0 )
    {
        ui->watchBox->setCurrentIndex(selectIndex);
    }
    ui->exporterList->clear();

    foreach ( IActivityExporterPtr exp, defPref->exporters() )
    {
        QListWidgetItem * item = new QListWidgetItem(exp->icon(), exp->name(), ui->exporterList);
        ui->exporterList->addItem(item);
    }
    ui->exporterList->setCurrentRow(0);
    displayWatchPreferences();


    ui->autoDownloadCheckbox->setChecked( m_Settings->autoDownload() );
}

void SettingsDialog::displayWatchPreferences()
{

    IActivityExporterPtr exp = currentExporter();
    if ( !exp )
    {
        return;
    }

    ui->enabledChecked->setChecked( exp->isEnabled() );
    ui->autoOpenCheckBox->setChecked( exp->autoOpen() );
    ui->setupButton->setEnabled( exp->hasSetup() );
}

IActivityExporterPtr SettingsDialog::currentExporter()
{
    QString serial = ui->watchBox->currentData().toString();
    WatchPreferencesPtr pref = m_TTManager->preferences(serial);

    if ( !ui->exporterList->currentItem() )
    {
        return IActivityExporterPtr();
    }

    QString exportName = ui->exporterList->currentItem()->text();

    return pref->exporter( exportName );
}

void SettingsDialog::on_exporterList_currentRowChanged(int currentRow)
{
    Q_UNUSED(currentRow);
    displayWatchPreferences();
}

void SettingsDialog::on_watchBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    displayWatchPreferences();
}

void SettingsDialog::onSetupFinished(IActivityExporter *exporter, bool success)
{
    if ( success )
    {
        ui->okButton->setEnabled(true);
        ui->statusLabel->setText( tr("Setup done for %1").arg(exporter->name()));
    }
    else
    {
        ui->statusLabel->setText( tr("Setup failed for %1").arg(exporter->name()));
    }
}

void SettingsDialog::onExporterChanged()
{
    IActivityExporterPtr exp = currentExporter();
    if ( !exp )
    {
        return;
    }

    exp->setEnabled( ui->enabledChecked->isChecked() );
    exp->setAutoOpen( ui->autoOpenCheckBox->isChecked() );
    ui->okButton->setEnabled(true);
}

void SettingsDialog::onSettingChanged()
{
    m_Settings->setAutoDownload( ui->autoDownloadCheckbox->isChecked() );
    ui->okButton->setEnabled(true);
}

void SettingsDialog::on_setupButton_clicked()
{
    IActivityExporterPtr exp = currentExporter();
    if ( !exp )
    {
        return;
    }

    ui->statusLabel->setText( tr("Starting setup for %1").arg(exp->name()));
    exp->setup(this);
}

void SettingsDialog::on_cancelButton_clicked()
{
    reject();
}

void SettingsDialog::on_okButton_clicked()
{
    accept();
}

void SettingsDialog::on_SettingsDialog_accepted()
{
    m_TTManager->savePreferences();
    m_Settings->save();
    ui->okButton->setEnabled(false);
}

void SettingsDialog::on_SettingsDialog_rejected()
{
    m_TTManager->loadPreferences();
    m_Settings->load();
    ui->okButton->setEnabled(false);
}
