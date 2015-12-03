#include <QDebug>
#include "elevationtiledownloaderdialog.h"
#include "ui_elevationtiledownloaderdialog.h"


#include <quazip.h>
#include <quazipfile.h>
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QFileInfo>

bool ElevationTileDownloaderDialog::unpack(ElevationTileDownloaderItem *item)
{
    item->setStatus(ElevationTileDownloaderItem::UNPACKING);
    item->setProgress(0);

    QuaZip zip(item->f->fileName());
    if (!zip.open(QuaZip::mdUnzip))
    {
        qDebug()<< "DatasetDownloaderDialog::unpack / unable to open zip file.";
        return false;
    }

    bool success = true;
    QStringList filesUnpacked;

    QuaZipFile file(&zip);
    for(bool more=zip.goToFirstFile(); more; more=zip.goToNextFile()) {

        QString name = file.getActualFileName();
        if ( name.endsWith(".flt") || name.endsWith(".hdr") || name.endsWith(".hgt") )
        {
            file.open(QIODevice::ReadOnly);

            QString destDir = m_BaseDir + QDir::separator() + item->destDir + QDir::separator();
            QString destName = destDir + name;
            QDir d;

            if (!d.exists(destDir))
            {
                d.mkpath(destDir);
            }
            if ( d.exists(destName))
            {
                d.remove(destName);
            }

            QFile of( destName );
            if ( !of.open(QIODevice::WriteOnly))
            {
                file.close();
                success = false;
                break;
            }

            filesUnpacked.append(destName);

            quint64 pos = 0;
            QByteArray data;
            while ( !file.atEnd() )
            {
                data = file.read(1024*1024);

                if ( of.write(data) != data.size() )
                {
                    success = false;
                    file.close();
                    break;
                }

                pos = pos + data.size();
                item->setProgress( ( pos * 100 ) / file.size() );
                QApplication::processEvents();
            }

            of.close();
            file.close();
        }
    }
    zip.close();

    if ( !success )
    {
        QDir d;
        foreach ( const QString &fn, filesUnpacked)
        {
            d.remove(fn);
        }
    }

    return success;
}

void ElevationTileDownloaderDialog::download(ElevationTileDownloaderItem * dataset)
{
    if ( dataset->urls.count() <= 0 )
    {
        dataset->setStatus( ElevationTileDownloaderItem::FAILED);
        checkDone();
        return;
    }

    dataset->setStatus( ElevationTileDownloaderItem::DOWNLOADING);
    QNetworkRequest r( dataset->urls.first() );

    QNetworkReply * reply  = m_Manager.get( r );

    if ( dataset->f )
    {
        dataset->f->deleteLater();
    }
    dataset->f = new QTemporaryFile();
    dataset->reply = reply;

    dataset->f->open();

    connect(reply, &QNetworkReply::downloadProgress, [this,dataset](qint64 read, qint64 total){
        dataset->setProgress( ( read * 100.0) / total);
    });

    connect(reply, &QNetworkReply::readyRead, [this, dataset]() {
        dataset->f->write( dataset->reply->readAll() );
    });

    connect(reply, &QNetworkReply::finished, [this,dataset, reply](){

        QNetworkReply::NetworkError e = reply->error();

        switch( reply->error() )
        {
        case QNetworkReply::NoError:
        {
            dataset->f->write( dataset->reply->readAll() );
            dataset->f->close();

            reply->deleteLater();
            dataset->reply = nullptr;

            if ( unpack(dataset) )
            {
                dataset->setStatus(ElevationTileDownloaderItem::SUCCESS);
                checkDone();
            }
            else
            {
                dataset->f->deleteLater();
                dataset->f = nullptr;

                dataset->setRetries( dataset->retries() + 1 );
                if ( dataset->retries() > 3 )
                {
                    dataset->urls.pop_front();
                    dataset->setRetries(0);
                }
                download(dataset);
            }
            break;
        }
        case QNetworkReply::OperationCanceledError:
            dataset->f->close();
            dataset->f->deleteLater();
            dataset->f = nullptr;
            reply->deleteLater();
            dataset->reply = nullptr;
            dataset->setStatus( ElevationTileDownloaderItem::FAILED);
            checkDone();
            break;
        default:
        {
            dataset->f->close();
            dataset->f->deleteLater();
            dataset->f = nullptr;

            reply->deleteLater();
            dataset->reply = nullptr;

            dataset->setRetries( dataset->retries() + 1 );
            if ( dataset->retries() > 3 )
            {
                dataset->urls.pop_front();
                dataset->setRetries(0);
            }
            download(dataset);
        }
        }

    });
}




void ElevationTileDownloaderDialog::checkDone()
{
    const ElevationDownloaderItemList & list = m_ListModel.list();
    for (int i=0;i<list.count();i++)
    {
        if (( list[i]->status() == ElevationTileDownloaderItem::DOWNLOADING ||
              list[i]->status() == ElevationTileDownloaderItem::UNPACKING ) )
        {
            return;
        }
    }

    // automatically close dialog after download.
    accept();
}

void ElevationTileDownloaderDialog::closeEvent(QCloseEvent *e)
{
    on_cancelButton_clicked();
    QDialog::closeEvent(e);
}

ElevationTileDownloaderDialog::ElevationTileDownloaderDialog(const QString &baseDir, QWidget *parent) :
    QDialog(parent),
    m_BaseDir(baseDir),
    ui(new Ui::ElevationTileDownloaderDialog)
{    
    ui->setupUi(this);
    ui->treeView->setModel(&m_ListModel);
}

void ElevationTileDownloaderDialog::addSource(ElevationSource &source)
{
    m_ListModel.addSource(source);
    ui->treeView->resizeColumnToContents(0);
    ui->treeView->resizeColumnToContents(1);
}


void ElevationTileDownloaderDialog::go()
{
    const ElevationDownloaderItemList & list = m_ListModel.list();
    for (int i=0;i<list.count();i++)
    {
        ElevationTileDownloaderItem * ds = list[i];
        download(ds);
    }
}


ElevationTileDownloaderDialog::~ElevationTileDownloaderDialog()
{
    delete ui;
}



void ElevationTileDownloaderDialog::on_cancelButton_clicked()
{
    const ElevationDownloaderItemList & list = m_ListModel.list();
    for (int i=0;i<list.count();i++)
    {
        ElevationTileDownloaderItem * ds = list[i];
        if ( ds->reply )
        {
            ds->reply->abort();
        }
        if ( ds->status() != ElevationTileDownloaderItem::SUCCESS )
        {
            ds->setStatus(ElevationTileDownloaderItem::FAILED);
        }
        checkDone();
    }
    reject();
}
