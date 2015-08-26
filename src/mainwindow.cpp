#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QDesktopServices>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QAbstractEventDispatcher>
#include <QMenu>


#include "exportworkingdialog.h"
#include "flatfileiconprovider.h"
#include "ttbinreader.h"
#include "tcxexport.h"
#include "version.h"
#include "singleshot.h"
#include "datasmoothing.h"
#include "settingsdialog.h"
#include "aboutdialog.h"
#include "downloaddialog.h"
#include "centeredexpmovavg.h"

#ifdef _WIN32
#include <dbt.h>
#endif

bool MainWindow::processTTBin(const QString& filename)
{
    m_Activity.clear();
    m_Seconds.clear();
    m_HeartBeat.clear();
    m_Speed.clear();
    m_Cadence.clear();

    ui->mapWidget->clearLines();

    TTBinReader br;

    ActivityPtr a = br.read(filename, true);
    m_Activity = a;

    if ( !a )
    {
        ui->statusBar->showMessage(tr("Could not parse file %1").arg(filename));
        return false;
    }

    ui->statusBar->showMessage(tr("Loading Elevation Data..."));
    m_ElevationLoader.load(a);
    return true;
}

void MainWindow::onElevationLoaded(bool success, ActivityPtr activity)
{
    if ( activity != m_Activity )
    {
        return;
    }

    if ( !success )
    {
        qDebug() << "MainWindow::onElevationLoaded / elevation data load failed.";
        ui->statusBar->showMessage(tr("Import done, failed to load elevation data, retry later."));
    }
    else
    {
        ui->statusBar->showMessage(tr("Import done."));
    }

    QRectF bounds;
    bool firstBounds = true;

    double lastHeart =0;

    ui->mapWidget->clearLines();
    ui->graph->clearGraphs();

    quint64 firstTime = 0;

    CenteredExpMovAvg cadence(61, 0.95);
    CenteredExpMovAvg speed(21, 0.95);
    CenteredExpMovAvg heartBeat(31, 0.95);
    CenteredExpMovAvg altitude(61, 0.95);

    TrackPointPtr prev;

    foreach( LapPtr lap, m_Activity->laps())
    {

        foreach ( TrackPointPtr tp, lap->points())
        {
            if ( tp->latitude() ==0 && tp->longitude() == 0 )
            {
                continue;
            }

            if ( firstBounds )
            {
                firstBounds = false;
                bounds.setTop(tp->latitude());
                bounds.setLeft(tp->longitude());
                bounds.setBottom(tp->latitude());
                bounds.setRight(tp->longitude());
            }

            if ( tp->latitude() < bounds.bottom() )
            {
                bounds.setBottom(tp->latitude());
            }
            else if ( tp->latitude() > bounds.top() )
            {
                bounds.setTop( tp->latitude());
            }

            if ( tp->longitude() < bounds.left() )
            {
                bounds.setLeft(tp->longitude());
            }
            else if ( tp->longitude() > bounds.right() )
            {
                bounds.setRight( tp->longitude() );
            }




            if (!prev && firstTime == 0 )
            {
                prev = tp;
                firstTime = tp->time().toTime_t();
                continue;
            }


            ui->mapWidget->addLine(prev->latitude(), prev->longitude(),tp->latitude(),tp->longitude());
            prev = tp;

            m_Seconds.append( tp->time().toTime_t() - firstTime );

            if ( success )
            {
                if ( m_Settings->useMetric() )
                {
                    altitude.add(tp->altitude());
                }
                else
                {
                    altitude.add(tp->altitude()*3.28084);
                }
            }

            if ( tp->heartRate() > 0 )
            {
                heartBeat.add(tp->heartRate());
                lastHeart = tp->heartRate();
            }
            else
            {
                heartBeat.add(lastHeart);
            }





            if ( m_Activity->sport() == Activity::RUNNING )
            {                
                int c = qMin(4, tp->cadence());
                cadence.add( 60 * c );

            }
            if ( m_Activity->sport() == Activity::BIKING )
            {                
                cadence.add( tp->cadence() );


                // use SPEED
                if ( m_Settings->useMetric() )
                {
                    speed.add(tp->speed() * 3.6); // kmh
                }
                else
                {
                    speed.add(tp->speed() * 3.6 / 1.60934 ); // mph
                }
            }
            else
            {
                // use pace
                if ( m_Settings->useMetric() )
                {
                    if ( tp->speed() > 0 )
                    {
                        speed.add(60.0 / ( tp->speed() * 3.6) ); // kmh
                    }
                    else
                    {
                        speed.add(10);
                    }
                }
                else
                {
                    if ( tp->speed() > 0 )
                    {
                        speed.add(60.0 / ( tp->speed() * 3.6 / 1.60934 ) ); // kmh
                    }
                    else
                    {
                        speed.add(16);
                    }
                }
            }

        }
    }

    m_Cadence.clear();
    m_HeartBeat.clear();
    m_Elevation.clear();
    m_Speed.clear();
    for(int i =0; i< cadence.size();i++)
    {
        m_Cadence.append( cadence.cea(i));
        m_HeartBeat.append( heartBeat.cea(i));
        if ( success )
        {
            m_Elevation.append( altitude.cea(i));
        }
        m_Speed.append( speed.cea(i));
    }

    if ( m_Axis3 == 0 )
    {
        m_Axis3 = ui->graph->axisRect(0)->addAxis(QCPAxis::atLeft);
    }
    if ( m_Axis4 == 0 )
    {
        m_Axis4 = ui->graph->axisRect(0)->addAxis(QCPAxis::atRight);
    }

    ui->graph->addGraph();
    ui->graph->addGraph(ui->graph->xAxis, ui->graph->yAxis2);
    ui->graph->addGraph(ui->graph->xAxis, m_Axis3);
    ui->graph->addGraph(ui->graph->xAxis, m_Axis4);


    ui->graph->graph(0)->setPen( QPen(Qt::darkRed));
    ui->graph->graph(1)->setPen( QPen(Qt::blue));
    ui->graph->graph(2)->setPen( QPen(Qt::darkMagenta));
    ui->graph->graph(3)->setPen( QPen(Qt::darkGreen));

    ui->graph->graph(0)->setData(m_Seconds, m_HeartBeat);
    ui->graph->graph(1)->setData(m_Seconds, m_Speed);
    ui->graph->graph(2)->setData(m_Seconds, m_Cadence);
    ui->graph->graph(3)->setData(m_Seconds, m_Elevation);

    ui->graph->yAxis->setLabel("HeartRate [BPM]");
    ui->graph->yAxis->setTickPen( ui->graph->graph(0)->pen() );
    ui->graph->yAxis->setBasePen( ui->graph->graph(0)->pen() );
    ui->graph->yAxis->setLabelColor( ui->graph->graph(0)->pen().color() );

    if ( m_Activity->sport() == Activity::BIKING )
    {
        if ( m_Settings->useMetric())
        {
            ui->graph->yAxis2->setLabel("Speed [km/hr]");
        }
        else
        {
            ui->graph->yAxis2->setLabel("Speed [Mph]");
        }
        ui->graph->yAxis2->setRangeReversed(false);
    }
    else
    {
        if ( m_Settings->useMetric())
        {
            ui->graph->yAxis2->setLabel("Pace [min/km]");
        }
        else
        {
            ui->graph->yAxis2->setLabel("Pace [min/M]");
        }
        ui->graph->yAxis2->setRangeReversed(true);
    }
    ui->graph->yAxis2->setVisible(true);    
    ui->graph->yAxis2->setTickPen( ui->graph->graph(1)->pen() );
    ui->graph->yAxis2->setBasePen( ui->graph->graph(1)->pen() );
    ui->graph->yAxis2->setLabelColor( ui->graph->graph(1)->pen().color() );

    m_Axis3->setLabel("Cadence [rpm]");
    m_Axis3->setTickPen( ui->graph->graph(2)->pen() );
    m_Axis3->setBasePen( ui->graph->graph(2)->pen() );
    m_Axis3->setLabelColor( ui->graph->graph(2)->pen().color() );

    if ( m_Settings->useMetric())
    {
        m_Axis4->setLabel("Elevation [m]");
    }
    else
    {
        m_Axis4->setLabel("Elevation [ft]");
    }
    m_Axis4->setTickPen( ui->graph->graph(3)->pen() );
    m_Axis4->setBasePen( ui->graph->graph(3)->pen() );
    m_Axis4->setLabelColor( ui->graph->graph(3)->pen().color() );


    ui->graph->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->graph->xAxis->setDateTimeSpec(Qt::UTC);
    ui->graph->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->graph->setInteractions(QCP::iRangeZoom | QCP::iRangeDrag);
    ui->graph->axisRect(0)->setRangeDrag(Qt::Horizontal);
    ui->graph->axisRect(0)->setRangeZoom(Qt::Horizontal);


    ui->graph->graph(0)->setVisible( ui->actionShow_Heart_Rate->isChecked() );
    ui->graph->graph(1)->setVisible( ui->actionShow_Speed->isChecked() );
    ui->graph->graph(2)->setVisible( ui->actionShow_Cadence->isChecked() );
    ui->graph->graph(3)->setVisible( ui->actionShow_Elevation->isChecked() );

    ui->graph->rescaleAxes();
    //ui->graph->graph(1)->valueAxis()->setRangeLower(0.0);
    ui->graph->graph(2)->valueAxis()->setRangeLower(0.0);
    ui->graph->graph(2)->valueAxis()->setRangeUpper(200);


    ui->graph->replot();

    QPointF center = bounds.center();
    int zoom = ui->mapWidget->boundsToZoom( bounds );
    ui->mapWidget->setCenter(zoom, center.y(), center.x());
    ui->mapWidget->repaint();
}



void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if ( !e->mimeData()->hasUrls())
    {
        return;
    }

    foreach (const QUrl &url, e->mimeData()->urls())
    {
        if ( !url.isLocalFile() )
        {
            continue;
        }
        QString filename = url.toLocalFile();
        if ( filename.endsWith("ttbin", Qt::CaseInsensitive))
        {
            e->acceptProposedAction();
            return;
        }
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    if ( !e->mimeData()->hasUrls())
    {
        return;
    }


    foreach (const QUrl &url, e->mimeData()->urls())
    {
        if ( !url.isLocalFile() )
        {
            continue;
        }
        QString filename = url.toLocalFile();
        if ( filename.endsWith("ttbin", Qt::CaseInsensitive))
        {
            processTTBin(filename);
            return;
        }
    }
}

void MainWindow::download(bool manualDownload)
{
    DownloadDialog * dd = findChild<DownloadDialog*>();
    if ( !dd )
    {
        dd = new DownloadDialog(m_Settings, &m_TTManager, this);
    }

    if ( dd->processWatches(manualDownload) == QDialog::Accepted )
    {
        m_WorkoutTreeModel.rescan(false);
        QStringList files = dd->filesDownloaded();
        if ( files.count() > 0 )
        {
            QModelIndex index = m_WorkoutTreeModel.findWorkoutItem( files.first());
            if ( index.isValid() )
            {
                QModelIndex proxyIndex = m_WorkoutSortingFilter.mapFromSource( index ) ;
                ui->treeView->scrollTo( proxyIndex );
                ui->treeView->setCurrentIndex(proxyIndex);
                on_treeView_clicked(proxyIndex);
            }
        }
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_Axis3(0),
    m_Axis4(0),
    m_Settings( Settings::get() ),
    m_WorkoutTreeModel(m_Settings->ttdir()),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setAcceptDrops(true);

    ui->actionShow_Cadence->setChecked(true);
    ui->actionShow_Elevation->setChecked(true);
    ui->actionShow_Heart_Rate->setChecked(true);
    ui->actionShow_Speed->setChecked(true);

    connect(ui->graph, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(onGraphMouseMove(QMouseEvent*)));
    connect(&m_TTManager, SIGNAL(ttArrived(QString)), this, SLOT(onWatchArrived()));
    connect(&m_ElevationLoader, SIGNAL(loaded(bool,ActivityPtr)), this, SLOT(onElevationLoaded(bool,ActivityPtr)));
    connect(&m_WatchTimer, SIGNAL(timeout()), this, SLOT(onWatchArrivedDelay()));
    m_WatchTimer.setSingleShot(true);
    m_WatchTimer.setInterval(500);


    m_TTManager.startSearch();

    m_WorkoutSortingFilter.setSortRole(Qt::UserRole);
    m_WorkoutSortingFilter.setSourceModel(&m_WorkoutTreeModel);
    ui->treeView->setModel( & m_WorkoutSortingFilter );
    ui->treeView->sortByColumn(0,Qt::DescendingOrder);
    ui->treeView->expandAll();
    ui->treeView->resizeColumnToContents(0);
    ui->treeView->resizeColumnToContents(1);
    ui->treeView->resizeColumnToContents(2);
    ui->treeView->resizeColumnToContents(3);

    m_TileCombo = new QComboBox;
    ui->toolBar->addSeparator();
    ui->toolBar->addWidget( m_TileCombo );


    int tileIndex = 0;
    int tileSelectIndex = -1;
    QFile f(":/cfg/tiles.json");
    if ( f.open(QIODevice::ReadOnly))
    {
        QByteArray json = f.readAll();
        f.close();

        QJsonParseError pe;
        QJsonDocument d = QJsonDocument::fromJson( json, &pe );
        if ( pe.error == QJsonParseError::NoError )
        {
            foreach ( QJsonValue v, d.array())
            {
                QJsonObject entry = v.toObject();
                m_TileCombo->addItem( entry["name"].toString(), entry);

                if ( entry["url"].toString() == m_Settings->tileUrl() )
                {
                    tileSelectIndex = tileIndex;
                }
                tileIndex++;
            }
        }
        else
        {
            qWarning() << "Could not parse JSON tile file." << pe.errorString();
        }

        m_TileCombo->setCurrentIndex(tileSelectIndex);
        onTileChanged();
    }


    connect(m_TileCombo,SIGNAL(currentIndexChanged(int)), this, SLOT(onTileChanged()));


    ui->mapWidget->setCenter(m_Settings->lastZoom(), m_Settings->lastLatitude(), m_Settings->lastLongitude());


#ifdef WIN32

    connect(&m_DeviceArriveDebounce, SIGNAL(timeout()), &m_TTManager, SLOT(checkForTTs()));
    m_DeviceArriveDebounce.setInterval(500);
    m_DeviceArriveDebounce.setSingleShot(true);

    QAbstractEventDispatcher::instance()->installNativeEventFilter(this);

    GUID hidGUID = { 0x4d1e55b2, 0xf16f, 0x11cf,
                          0x88,0xcb,0x00,0x11,0x11,0x00,0x00,0x30 };

    DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
    ZeroMemory( &notificationFilter, sizeof(notificationFilter) );
    notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notificationFilter.dbcc_classguid = hidGUID;
    HDEVNOTIFY status = RegisterDeviceNotification((HWND)this->winId(),
            &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
    if( !status )
    {
        qDebug() << "MainWindow::MainWindow / Could not RegisterDeviceNotification.";
    }
#endif


    ui->treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    WatchExportersPtr watchExporters = m_TTManager.defaultExporters();
    IActivityExporterList exporters = watchExporters->exporters();
    foreach ( IActivityExporterPtr ae, exporters )
    {
        QAction * action = new QAction(ae->icon(), ae->name(), ui->treeView);
        connect(action, &QAction::triggered, [this, ae]() {
            exportActivity( ae->name());
        });
        ui->treeView->addAction(action);
    }
    QAction * separator = new QAction(this);
    separator->setSeparator(true);
    ui->treeView->addAction( separator );
    ui->treeView->addAction( ui->actionShow_in_explorer);
    ui->treeView->addAction( ui->actionChange_Activity_Type);

}

void MainWindow::onTileChanged()
{
    QJsonObject o = m_TileCombo->currentData().toJsonObject();
    ui->mapWidget->setTilePath(o["url"].toString(), o["copyright"].toString());
    m_Settings->setTileUrl(o["url"].toString());
    m_Settings->save();
}

MainWindow::~MainWindow()
{
    m_Settings->setLastLatitude( ui->mapWidget->latitude() );
    m_Settings->setLastLongitude( ui->mapWidget->longitude() );
    m_Settings->setLastZoom( ui->mapWidget->zoom() );
    m_Settings->save();
    delete ui;
}

bool MainWindow::nativeEventFilter(const QByteArray &eventType, void *message, long *l)
{
    Q_UNUSED(l);
#ifdef WIN32

    MSG * msg = ( MSG * ) message;

    if ( msg == 0 )
    {
        return false;
    }

    #define DBT_DEVNODES_CHANGED 0x0007

    if ( msg->message == WM_DEVICECHANGE && msg->wParam == DBT_DEVNODES_CHANGED )
    {
        qDebug() << "MainWindow::nativeEventFilter / got device change.";
        m_DeviceArriveDebounce.start();
    }

    return false;
#else
    return false;
#endif
}


void MainWindow::exportActivity(const QString &exporterName)
{
    if ( !m_Activity)
    {
        return;
    }

    QString filename = m_Activity->filename();

    QFileInfo fi(filename);
    QDir fileDir = fi.absoluteDir();
    QString path = QDir::cleanPath(fileDir.path());
    path = QDir::toNativeSeparators(path);
    QStringList parts = path.split( QDir::separator() );

    WatchExportersPtr prefs;

    for (int i=parts.count()-1;i>=0;i--)
    {
        prefs = m_TTManager.exportersForName( parts[i] );
        if ( prefs )
        {
            break;
        }
    }

    if ( !prefs )
    {
        prefs = m_TTManager.defaultExporters();
    }

    if ( prefs )
    {
        ExportWorkingDialog ewd(m_Activity, prefs, exporterName, this);
        ewd.exec();

        /* if ( !prefs->exportActivity(m_Activity, exporterName) )
        {
            QMessageBox::warning(this, tr("Warning"), tr("Exporting to %1 has not yet been setup, please go to the settings and configure it first.").arg(exporterName));
        }*/
    }
    else
    {
        ui->statusBar->showMessage(tr("Could not find an exporter object."));
    }
}


/*
 * When a Watch is connected
 *
 * 1. Load the preferences file.
 * 2. Load the .ttbin's
 * 3. Export .ttbin's as specified in preferences file
 * 4. Upload GPS Quick fix Data.
 * 5. Store preferences files on drive. This file may only be modified if Watch is present.
 *
 */
void MainWindow::onWatchArrived()
{        
    m_WatchTimer.start();
}

void MainWindow::onWatchArrivedDelay()
{
    if ( !m_Settings->autoDownload() )
    {
        return;
    }
    download(false);
}

void MainWindow::onGraphMouseMove(QMouseEvent *event)
{
    if ( !m_Activity)
    {
        return;
    }

    double pos = ui->graph->xAxis->pixelToCoord(event->pos().x());

    ui->mapWidget->clearCircles();

    if ( pos < 0 )
    {
        return;
    }

    TrackPointPtr pt = m_Activity->find( pos );

    if ( pt )
    {
        int cadence = 0;
        int idx = m_Seconds.indexOf( round(pos) );
        if ( idx > 0 && idx < m_Cadence.count() )
        {
            cadence = m_Cadence.at(idx);
        }

        QTime t(0,0,0);
        t = t.addSecs((int)pos);

        QString msg = QString("Time=%1, HeartRate=%2 bpm, Speed=%3 kph, Distance=%4 m, Cadence=%5 pm, Elevation=%6").arg(t.toString()).arg(pt->heartRate()).arg(pt->speed()*3.6).arg( pt->cummulativeDistance() ).arg(cadence).arg(pt->altitude());
        ui->statusBar->showMessage(msg,5000);
        ui->mapWidget->addCircle( pt->latitude(), pt->longitude() );
        ui->mapWidget->repaint();
        return;
    }
}



void MainWindow::on_actionProcess_TTBIN_triggered()
{
    static QString dir;
    if ( dir.length() == 0 )
    {
        dir = Settings::ttdir();
    }

    QString fn = QFileDialog::getOpenFileName(this, tr("Find .ttbin file"), dir, "ttbin files (*.ttbin)");
    if ( fn.length() == 0 )
    {
        return;
    }

    if ( processTTBin(fn) )
    {
        QFileInfo fi(fn);
        dir = fi.canonicalPath();
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}
void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    if ( !index.isValid() )
    {
        return;
    }

    QModelIndex sourceIndex = m_WorkoutSortingFilter.mapToSource(index);

    TTWorkoutItem * item = m_WorkoutTreeModel.indexToWorkoutItem(sourceIndex);
    if ( item == 0 )
    {
        return;
    }

    processTTBin(item->filename());
}

// from http://stackoverflow.com/questions/9137692/qdesktopservicesopenurl-with-selecting-specified-file-in-explorer
void selectFileInExplorer(const QString& path){
#if defined(Q_OS_WIN)
    const QString explorer = "explorer";
    QStringList param;
    if (!QFileInfo(path).isDir())
    {
        param << QLatin1String("/select,");
    }
    param << QDir::toNativeSeparators(path);
    QProcess::startDetached(explorer, param);
#elif defined(Q_OS_MAC)
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
               << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                  .arg(path);
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
               << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute("/usr/bin/osascript", scriptArgs);
#elif defined(Q_OS_LINUX)
    QFileInfo f(filename);
    QDesktopServices::openUrl( QUrl::fromLocalFile(f.absolutePath()));
#endif
}

void MainWindow::on_actionShow_in_explorer_triggered()
{    
    QString filename = Settings::ttdir();
    QModelIndex index = ui->treeView->currentIndex();
    if ( !index.isValid() )
    {
        return;
    }
    QModelIndex sourceIndex = m_WorkoutSortingFilter.mapToSource(index);

    // if user selected a workout point to that otherwise just open TT directory.
    TTWorkoutItem * item = m_WorkoutTreeModel.indexToWorkoutItem(sourceIndex);
    if ( item != 0 )
    {
        filename = item->filename();
    }
    selectFileInExplorer(filename);
}


void MainWindow::on_actionAbout_triggered()
{    
    AboutDialog * ad = findChild<AboutDialog*>();
    if ( !ad)
    {
        ad = new AboutDialog(this);
    }
    ad->show();
}

void MainWindow::on_actionShow_Speed_toggled(bool arg1)
{
    if ( ui->graph->graphCount() >= 2 )
    {
        ui->graph->graph(1)->setVisible(arg1);
        ui->graph->replot();
    }
}



void MainWindow::on_actionShow_Heart_Rate_toggled(bool arg1)
{
    if ( ui->graph->graphCount() >= 1 )
    {
        ui->graph->graph(0)->setVisible(arg1);
        ui->graph->replot();
    }
}

void MainWindow::on_actionShow_Cadence_toggled(bool arg1)
{
    if ( ui->graph->graphCount() >= 3 )
    {
        ui->graph->graph(2)->setVisible(arg1);
        ui->graph->replot();
    }
}

void MainWindow::on_actionShow_Elevation_toggled(bool arg1)
{
    if ( ui->graph->graphCount() >= 4 )
    {
        ui->graph->graph(3)->setVisible(arg1);
        ui->graph->replot();
    }
}

void MainWindow::on_actionGo_to_website_triggered()
{
    QDesktopServices::openUrl( QUrl("https://github.com/altera2015/ttwatcher") );
}

/*
 * 1. Try to find the Watch Preferences file that goes with the .ttbin
 * 2. If not watch use default preferences object.
 */
void MainWindow::on_actionExport_Activity_triggered()
{    
    exportActivity("");
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog * s = findChild<SettingsDialog*>();
    if ( !s )
    {
        s = new SettingsDialog (m_Settings, &m_TTManager, this);
    }

    s->show();

}

void MainWindow::on_actionDownload_from_watch_triggered()
{
    download(true);
}

void MainWindow::on_actionRescan_workout_directory_triggered()
{
    m_WorkoutTreeModel.rescan(false);
}

void MainWindow::on_actionChange_Activity_Type_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();
    if ( !index.isValid())
    {
        return;
    }

    QModelIndex sourceIndex = m_WorkoutSortingFilter.mapToSource(index);
    TTWorkoutItem * item = m_WorkoutTreeModel.indexToWorkoutItem(sourceIndex);
    if ( !item )
    {
        return;
    }

    QStringList types;
    QMap<QString, Activity::Sport> tx;
    int currentIndex = -1;
    for (int i=(int)Activity::RUNNING;i<(int)Activity::OTHER;i++)
    {
        Activity::Sport sport = (Activity::Sport)i;
        QString label = Activity::sportToString(sport);
        types.append(label);
        tx[label] = sport;
        if ( item->sport() == sport )
        {
            currentIndex = types.count() - 1;
        }
    }

    bool ok = false;
    QString selected = QInputDialog::getItem(this, tr("Change activity type"), tr("Activity Type"), types, currentIndex, false, &ok);

    if ( !tx.contains(selected) || !ok )
    {
        return;
    }

    Activity::Sport sport = tx[selected];

    if ( item->sport() == sport )
    {
        ui->statusBar->showMessage(tr("Activity already set to that type."));
        return;
    }

    TTBinReader br;

    QFile input(item->filename());
    if ( !input.open(QIODevice::ReadOnly) )
    {
        ui->statusBar->showMessage(tr("Failed to change activity type (input open failed)."));
        return;
    }

    QFile output(item->filename() + ".2.ttbin");
    if ( !output.open(QIODevice::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Failed to change activity type (output open failed)."));
        return;
    }

    if ( !br.updateActivityType(input, true, output, sport) )
    {
        ui->statusBar->showMessage(tr("Failed to change activity type"));
        return;
    }

    input.close();
    output.close();
    input.rename( item->filename() + ".backup");
    output.rename( item->filename() );
    if ( !m_WorkoutTreeModel.reloadIndex(sourceIndex) )
    {
        output.remove();
        input.rename( item->filename() );
    }

    ui->statusBar->showMessage(tr("Activity Type changed."));
}
