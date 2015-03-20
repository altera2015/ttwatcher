#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QUrl>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT
    QNetworkAccessManager m_Manager;
    QUrl m_LatestVersionUrl;
public:
    explicit AboutDialog(QWidget *parent = 0);
    ~AboutDialog();

private slots:
    void on_pushButton_clicked();
    void onFinished(QNetworkReply * reply);
    void on_downloadLatestButton_clicked();

private:
    Ui::AboutDialog *ui;
protected:
    // returns true if new version is available.
    bool compareVersions(QString tagName);
    void showEvent(QShowEvent *e);
};

#endif // ABOUTDIALOG_H
