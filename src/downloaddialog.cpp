#include "downloaddialog.h"
#include "ui_downloaddialog.h"


#include <QNetworkReply>
#include <QNetworkRequest>
#include <QByteArray>
#include <QUrl>
#include <QDebug>
#include <QNetworkProxy>
#include <QDir>
#include <QStringList>
#include <QMessageBox>
#include "singleshot.h"
#include "ttbinreader.h"
#include "elevationloader.h"

void DownloadDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);

    ui->logWidget->clear();

    // we have to run process a bit after showEvent is done, otherwise
    // we might call accepted while still in showEvent,
    // which apparently fails.
    SingleShot::go([this]{
        process();
    }, 250, true, this);
}

DownloadDialog::DownloadDialog(Settings *settings, TTManager *ttManager, QWidget *parent) :
    QDialog(parent),
    m_Settings(settings),
    m_TTManager(ttManager),
    ui(new Ui::DownloadDialog)
{
    ui->setupUi(this);
    connect(&m_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onFinished(QNetworkReply*)));
    connect(m_TTManager, SIGNAL(allExportingFinished()), this, SLOT(onExportingFinished()));
    connect(m_TTManager, SIGNAL(exportError(QString)), this, SLOT(onExportError(QString)));
#ifdef USE_DEBUG_PROXY
    QNetworkProxy p;
    p.setHostName("localhost");
    p.setPort(8888);
    p.setType(QNetworkProxy::HttpProxy);
    m_Manager.setProxy(p);
#endif
}

DownloadDialog::~DownloadDialog()
{
    delete ui;
}

int DownloadDialog::processWatches()
{
    // if the window is already visible we don't need to process.
    if ( this->isVisible() )
    {
        qDebug() << "DownloadDialog::processWatches / already visible.";
        return QDialog::Rejected;
    }

    return exec();
}

QStringList DownloadDialog::filesDownloaded() const
{
    return m_Files;
}

void DownloadDialog::process()
{
    bool shouldDownloadQuickFix = false;

    if ( m_TTManager->watches().count() == 0 )
    {
        workInfo(tr("Done"), true);
        return;
    }

    foreach ( TTWatch * watch, m_TTManager->watches())
    {

        if ( !m_Settings->autoDownload() )
        {
            continue;
        }

        WatchExportersPtr exporters = m_TTManager->exporters( watch->serial());

        /**********************************************/
        /* 1. LOAD TTBINS */
        /**********************************************/

        workInfo(tr("Downloading .ttbins"), false);

        QStringList files = watch->download(Settings::ttdir() + QDir::separator() + exporters->name(), true);

        if ( files.count() > 0 )
        {
            workInfo(tr("Exporting .ttbins"), false);

            /**********************************************/
            /* 2. EXPORT TTBINS */
            /**********************************************/

            foreach ( const QString & filename, files )
            {
                /**********************************************/
                /* 3. Read TTBIN */
                /**********************************************/

                workInfo(tr("Reading .ttbin %1").arg(filename), false);

                TTBinReader br;
                ActivityPtr a = br.read(filename, true);
                if ( !a )
                {
                    workInfo(tr("failed to parse %1.").arg(filename), false);
                    qDebug() << "DownloadDialog::process / could not parse file " << filename;
                    continue;
                }

                /**********************************************/
                /* 4. Load Elevation Data */
                /**********************************************/

                workInfo(tr("Downloading Elevation Data for %1.").arg(filename), false);
                ElevationLoader el;
                if ( el.load(a, true) != ElevationLoader::SUCCESS )
                {
                    workInfo(tr("failed to download elevation data for %1.").arg(filename), false);
                    continue;
                }

                /**********************************************/
                /* 5. Export TTBIN */
                /**********************************************/

                workInfo(tr("Exporting .ttbin . %1").arg(filename), false);

                if ( !exporters->exportActivity(a) )
                {
                    workInfo(tr("Exporting .ttbin failed. %1").arg(filename), false);
                    continue;
                }
            }

            m_Files.append( files );
        }
        else
        {
            workInfo(tr("No new workouts."), false);
        }

        QDateTime lastFixUpload = m_Settings->lastQuickFix(watch->serial());
        QDateTime now = QDateTime::currentDateTime();
        if ( lastFixUpload.secsTo(now) > 24 * 3600 )
        {
            shouldDownloadQuickFix = true;
        }

    }


    /**********************************************/
    /* 6. APPLY GPS QUICK FIX DATA */
    /**********************************************/

    if ( shouldDownloadQuickFix )
    {
        workInfo(tr("Downloading GPS Quick Fix data"), false);
        QNetworkRequest r;
        r.setUrl(QUrl( "http://gpsquickfix.services.tomtom.com/fitness/sifgps.f2p3enc.ee"));
        m_Manager.get(r);
    }
    else
    {
        onExportingFinished();
    }
}

void DownloadDialog::workInfo(const QString &message, bool done)
{
    qDebug() << "DownloadDialog::workInfo" << message<< done;
    ui->logWidget->addItem(message);
    qApp->processEvents();
    if ( done )
    {
        accept();
    }
}

void DownloadDialog::onFinished(QNetworkReply *reply)
{
    if ( reply->error() != QNetworkReply::NoError )
    {
       workInfo(tr("Download QuickFix Data Failed."),true);
       return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    foreach ( TTWatch * watch, m_TTManager->watches())
    {
        workInfo(tr("Writing GPS Quick Fix data to %1...").arg(watch->serial()), false);
        watch->writeFile( data, FILE_GPSQUICKFIX_DATA, true );
        qDebug() << "PostGPS FIX" << watch->postGPSFix();
        m_Settings->setQuickFixDate( watch->serial(), QDateTime::currentDateTime());
    }

    onExportingFinished();
}

void DownloadDialog::onExportingFinished()
{
    bool stillExporting = false;
    WatchExportersMap::iterator i = m_TTManager->exporters().begin();
    for(;i!=m_TTManager->exporters().end();i++)
    {
        if ( i.value()->isExporting() )
        {
            stillExporting = true;
            break;
        }
    }
    if (!stillExporting )
    {
        workInfo(tr("Done"), true);
    }
}

void DownloadDialog::onExportError(QString message)
{
    QMessageBox::warning(this, tr("Error Exporting"), message);
}
