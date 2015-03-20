#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "ttmanager.h"
#include "settings.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
    Settings * m_Settings;
    TTManager * m_TTManager;

public:
    explicit SettingsDialog(Settings * settings, TTManager * ttManager, QWidget *parent = 0);
    ~SettingsDialog();

private slots:

    void on_exporterList_currentRowChanged(int currentRow);

    void on_watchBox_currentIndexChanged(int index);

    void onSetupFinished( IActivityExporter *exporter, bool success );

    void onExporterChanged();
    void onSettingChanged();

    void on_setupButton_clicked();

    void on_cancelButton_clicked();

    void on_okButton_clicked();

    void on_SettingsDialog_accepted();

    void on_SettingsDialog_rejected();

private slots:
    void display();

private:
    Ui::SettingsDialog *ui;


    void displayWatchPreferences();
    IActivityExporterPtr currentExporter();
};

#endif // SETTINGSDIALOG_H
