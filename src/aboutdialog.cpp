#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "version.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>
#include <QDebug>
#include <QDesktopServices>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    connect(&m_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onFinished(QNetworkReply*)));
    ui->setupUi(this);
    ui->downloadLatestButton->setVisible(false);
    ui->newVersionLabel->setVisible(false);
    ui->versionLabel->setText( tr("TTWatcher Version %1").arg(VER_FILEVERSION_STR));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_pushButton_clicked()
{
    close();
}

void AboutDialog::onFinished(QNetworkReply *reply)
{
    if ( reply->error() != QNetworkReply::NoError )
    {
        qDebug() << "AboutDialog::onFinished / cannot load latest release." << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson(data, &pe);
    if ( pe.error != QJsonParseError::NoError )
    {
        qDebug() << "AboutDialog::onFinished / can't pase json " << pe.errorString();
        return;
    }

    QJsonObject o = d.object();

    if ( o.contains("tag_name"))
    {
        QString version = o["tag_name"].toString();

        if ( compareVersions(version) )
        {
            m_LatestVersionUrl = QUrl( o["html_url"].toString() );
            ui->newVersionLabel->setVisible(true);
            ui->downloadLatestButton->setVisible(true);
        }
    }
}

bool AboutDialog::compareVersions(QString tagName)
{
    if ( tagName.startsWith("v"))
    {
        tagName = tagName.mid(1);
    }
    QStringList tagNameEntries = tagName.split(".");
    QVector<int> latest;
    foreach ( const QString & entry, tagNameEntries)
    {
        latest.append( entry.toInt());
    }

    int current[] = { VER_FILEVERSION };

    int cnt = qMin( latest.count(), 3);
    for (int i=0;i<cnt;i++)
    {
        if ( latest[i] > current[i] )
        {
            return true;
        }
        if ( latest[i] < current[i] )
        {
            return false;
        }
    }

    return false;
}

void AboutDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);

    QNetworkRequest r;
    r.setUrl(QUrl("https://api.github.com/repos/altera2015/ttwatcher/releases/latest"));
    r.setRawHeader("Accept", "application/vnd.github.v3+json");
    m_Manager.get(r);
}

void AboutDialog::on_downloadLatestButton_clicked()
{
    QDesktopServices::openUrl(m_LatestVersionUrl);
}
