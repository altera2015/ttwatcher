#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QDesktopServices>

#include "flatfileiconprovider.h"
#include "ttbinreader.h"
#include "tcxexport.h"
#include "version.h"
#include "singleshot.h"
#include "datasmoothing.h"

bool MainWindow::processTTBin(const QString& filename)
{
    m_Activity.clear();
    m_Seconds.clear();
    m_HeartBeat.clear();
    m_Speed.clear();
    m_Cadence.clear();

    ui->mapWidget->clearLines();


    QFile f(filename);
    if ( !f.open(QIODevice::ReadOnly ))
    {
        ui->statusBar->showMessage(tr("Could not open file %1").arg(filename));
        return false;
    }


    TTBinReader br;

    ActivityPtr a = br.read(f, true);
    m_Activity = a;

    if ( !a )
    {
        ui->statusBar->showMessage(tr("Could not parse file %1").arg(filename));
        return false;
    }

    a->setFilename( filename );

    ui->statusBar->showMessage(tr("Loading Elevation Data..."));
    m_ElevationLoader.load(a);

    int distance = 0;

    QTime t(0,0,0);
    foreach ( LapPtr lap, a->laps())
    {
        distance += lap->length();
        t = t.addSecs( lap->totalSeconds() );
    }


    QString stats = tr("Date: %1\nDistance: %2 m\nDuration: %3").arg(a->date().toString()).arg( distance ).arg(  t.toString( ));
    ui->statLabel->setText(stats);

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


    // QVector<int> cadence;
    DataSmoothing<int> cadence;
    DataSmoothing<int> speed;
    DataSmoothing<int> heartBeat;
    DataSmoothing<int> altitude;

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
                m_Elevation.append( altitude.add(tp->altitude()) );
            }

            if ( tp->heartRate() > 0 )
            {
                m_HeartBeat.append( heartBeat.add(tp->heartRate()) );
                lastHeart = tp->heartRate();
            }
            else
            {
                m_HeartBeat.append( heartBeat.add(lastHeart) );
            }

            m_Speed.append( speed.add(tp->speed() * 3.6) );

            if ( m_Activity->sport() == Activity::RUNNING )
            {
                if ( tp->cadence() > 0 )
                {
                    cadence.add( tp->cadence() );
                }

                m_Cadence.append( 60.0 * cadence.value() );

            }
            if ( m_Activity->sport() == Activity::BIKING )
            {
                m_Cadence.append( cadence.add(tp->cadence()) );
            }

        }
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

    ui->graph->yAxis2->setVisible(true);
    ui->graph->yAxis2->setLabel("Speed [km/hr]");
    ui->graph->yAxis2->setTickPen( ui->graph->graph(1)->pen() );
    ui->graph->yAxis2->setBasePen( ui->graph->graph(1)->pen() );
    ui->graph->yAxis2->setLabelColor( ui->graph->graph(1)->pen().color() );

    m_Axis3->setLabel("Cadence [rpm]");
    m_Axis3->setTickPen( ui->graph->graph(2)->pen() );
    m_Axis3->setBasePen( ui->graph->graph(2)->pen() );
    m_Axis3->setLabelColor( ui->graph->graph(2)->pen().color() );

    m_Axis4->setLabel("Elevation [m]");
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
    ui->graph->graph(1)->valueAxis()->setRangeLower(0.0);
    ui->graph->replot();

    QPointF center = bounds.center();
    int zoom = ui->mapWidget->boundsToZoom( bounds );
    ui->mapWidget->setCenter(zoom, center.y(), center.x());



    QFile of(m_Activity->filename() + ".tcx");
    if (!of.open(QIODevice::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Could not save tcx file %1.").arg(m_Activity->filename()+".tcx"));
        return;
    }


    TCXExport e;
    e.save(&of, m_Activity);
    of.close();
}



QString MainWindow::ttdir() const
{
    return QDir::homePath() + QDir::separator() + "TomTom MySports";
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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_Axis3(0),
    m_Axis4(0)
{
    ui->setupUi(this);

    setAcceptDrops(true);

    ui->actionShow_Cadence->setChecked(true);
    ui->actionShow_Elevation->setChecked(true);
    ui->actionShow_Heart_Rate->setChecked(true);
    ui->actionShow_Speed->setChecked(true);


    ui->actionDownload_Workouts->setEnabled(false);

    connect(ui->graph, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(onGraphMouseMove(QMouseEvent*)));
    connect(&m_TTManager, SIGNAL(ttArrived()), this, SLOT(onWatchesChanged()));
    connect(&m_TTManager, SIGNAL(ttRemoved()), this, SLOT(onWatchesChanged()));
    connect(&m_ElevationLoader, SIGNAL(loaded(bool,ActivityPtr)), this, SLOT(onElevationLoaded(bool,ActivityPtr)));

    ui->mapWidget->setLatitude(59.9138204);
    ui->mapWidget->setLongitude(10.7387413);

#ifdef I_UNDERSTAND_THIS_IS_DANGEROUS
    m_TTManager.startSearch();
    ui->actionDownload_Workouts->setVisible(true);
    ui->actionExport_Activity->setVisible(true);
#else
    ui->actionDownload_Workouts->setVisible(false);
    ui->actionExport_Activity->setVisible(false);
#endif


    m_FSModel = new QFileSystemModel(this);
    FlatFileIconProvider * icons = new FlatFileIconProvider();
    m_FSModel->setIconProvider(icons);
    m_FSModel->setRootPath( ttdir() );
    m_FSModel->setFilter(QDir::AllDirs|QDir::Files|QDir::NoDotAndDotDot);
    QStringList fl;
    fl.append("*.ttbin");
    m_FSModel->setNameFilterDisables(false);
    m_FSModel->setNameFilters(fl);
    ui->treeView->setModel(m_FSModel);

    QModelIndex idx = m_FSModel->index( ttdir() );
    ui->treeView->setRootIndex(idx);
    ui->treeView->hideColumn(3);
    ui->treeView->hideColumn(2);
    ui->treeView->hideColumn(1);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*
 * When a Watch is connected
 *
 * 1. Load the preferences file.
 * 2. Load the .ttbin's
 * 3. Export .ttbin's as specified in preferences file
 * 4. Upload GPS Quick Data.
 * 5. Store preferences files on drive. This file may only be modified if Watch is present.
 *
 */
void MainWindow::onWatchesChanged()
{
    ui->actionDownload_Workouts->setEnabled( m_TTManager.watches().count() > 0 );

    foreach ( TTWatch * watch, m_TTManager.watches())
    {
        QByteArray prefData;
        QBuffer buffer(&prefData);
        buffer.open(QIODevice::WriteOnly);

        /**********************************************/
        /* 1. LOADING PREFERENCES */
        /**********************************************/

        emit workInfo(tr("Loading Preferences"), false);

        if ( !watch->downloadPreferences(buffer) )
        {
            qCritical() << "MainWindow::onWatchesChanged / unable to load preferences file.";
            emit workInfo(tr("Failed to load preferences"), true);
            continue;
        }

        buffer.close();

        /*QFile tempf("tempf.xml");
        tempf.open(QIODevice::WriteOnly);
        tempf.write(prefData);
        tempf.close();*/

        WatchPreferencesPtr preferences = m_TTManager.preferences( watch->serial() );
        if ( !preferences )
        {
            qCritical() << "MainWindow::onWatchesChanged / did not get a preferences ptr.";
            continue;
        }

        if ( !preferences->parsePreferences( prefData ) )
        {
            qCritical() << "MainWindow::onWatchesChanged / failed to parse preferences.";
            emit workInfo(tr("Failed to parse preferences"), true);
            continue;
        }

        /**********************************************/
        /* 2. LOAD TTBINS */
        /**********************************************/

        emit workInfo(tr("Downloading .ttbins"), false);

        QStringList files = watch->download(ttdir() + QDir::separator() + preferences->name(), true);

        emit workInfo(tr("Exporting .ttbins"), false);

        /**********************************************/
        /* 3. EXPORT TTBINS */
        /**********************************************/

        foreach ( const QString & filename, files )
        {
            if ( !preferences->exportFile(filename) )
            {
                emit workInfo(tr("Exporting .ttbin failed. %1").arg(filename), false);
            }
        }

        // place the UI interaction on a slight delay to give the m_FSModel a chance
        // to pick up the new files.
        SingleShot::go([this, files](){

            if ( files.count() > 0 )
            {
                QModelIndex index = m_FSModel->index( files.first() );
                if ( index.isValid() )
                {
                    ui->treeView->setCurrentIndex(index);
                    ui->treeView->scrollTo(index);
                }
            }

        }, 1000, true, this);

        /**********************************************/
        /* 4. APPLY GPS QUICK DATA */
        /**********************************************/

        // soon.
        emit workInfo(tr("Uploading GPS Quick data"), false);

    }

    emit workInfo(tr("Done"), true);

    /**********************************************/
    /* 5. Save preferences data. */
    /**********************************************/
    m_TTManager.savePreferences();
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
        if ( idx > 0 )
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
        dir = ttdir();
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

    if ( m_FSModel->isDir(index) )
    {
        return;
    }

    QFileInfo fi = m_FSModel->fileInfo(index);

    processTTBin(fi.filePath());
}



void MainWindow::on_actionShow_in_explorer_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();

    QString filename = ttdir();
    if ( index.isValid() )
    {
        QFileInfo f = m_FSModel->fileInfo( index );
        filename = f.absolutePath();
    }
    QDesktopServices::openUrl( QUrl::fromLocalFile(filename ) );
}


void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, tr("TTWatcher"), tr("TTWatcher %1\n\nttbin tcx exporter, handles even corrupted ttbin files.\nhttps://github.com/altera2015/ttwatcher").arg(VER_FILEVERSION_STR));
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

void MainWindow::on_actionExport_Activity_triggered()
{
    if ( !m_Activity)
    {
        return;
    }

    if ( m_TTManager.watches().count()== 0)
    {
        ui->statusBar->showMessage(tr("No TT watch detected."));
        return;
    }

    QString filename = m_Activity->filename();

    QFileInfo fi(filename);
    QDir fileDir = fi.absoluteDir();
    QStringList parts = fileDir.path().split( QDir::separator() );

    WatchPreferencesPtr prefs;

    for (int i=parts.count()-1;i>=0;i--)
    {
        prefs = m_TTManager.preferencesForName( parts[i] );
        if ( prefs )
        {
            break;
        }
    }

    if ( !prefs )
    {
        prefs = m_TTManager.defaultPreferences();
    }

    if ( prefs )
    {
        prefs->exportActivity(m_Activity);
    }
    else
    {
        ui->statusBar->showMessage(tr("Could not find an exporter object."));
    }
}
