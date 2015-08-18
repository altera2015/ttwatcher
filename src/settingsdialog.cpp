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

    connect(ttManager, SIGNAL(ttArrived(QString)), this, SLOT(display()));
    connect(ttManager, SIGNAL(ttRemoved(QString)), this, SLOT(display()));

    connect(ui->enabledChecked, SIGNAL(clicked()), this, SLOT(onExporterChanged()));
    connect(ui->autoOpenCheckBox, SIGNAL(clicked()), this, SLOT(onExporterChanged()));

    connect(ui->autoDownloadCheckbox, SIGNAL(clicked()), this, SLOT(onSettingChanged()));


    ui->okButton->setEnabled(false);

    WatchExportersMap::iterator i = m_TTManager->exporters().begin();
    for(;i!=m_TTManager->exporters().end();i++)
    {
        WatchExportersPtr wp = i.value();
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
    WatchExportersPtr defPref = m_TTManager->defaultExporters();

    int index = 0;
    int selectIndex = -1;
    WatchExportersMap::iterator i = m_TTManager->exporters().begin();
    for(;i!=m_TTManager->exporters().end();i++)
    {
        WatchExportersPtr p = i.value();

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

    ui->enabledChecked->setChecked( exp->config().isValid() );
    ui->autoOpenCheckBox->setChecked( exp->config().isAutoOpen() );
    ui->setupButton->setEnabled( exp->hasSetup() );
}

IActivityExporterPtr SettingsDialog::currentExporter()
{
    QString serial = ui->watchBox->currentData().toString();
    WatchExportersPtr pref = m_TTManager->exporters(serial);

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

    exp->config().setValid( ui->enabledChecked->isChecked() );
    exp->config().setAutoOpen( ui->autoOpenCheckBox->isChecked() );
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
    m_TTManager->saveAllConfig(true);
    m_Settings->save();
    ui->okButton->setEnabled(false);
}

void SettingsDialog::on_SettingsDialog_rejected()
{
    m_TTManager->loadAllConfig();
    m_Settings->load();
    ui->okButton->setEnabled(false);
}
