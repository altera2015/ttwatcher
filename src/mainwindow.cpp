#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QDesktopServices>

#include "ttbinreader.h"
#include "tcxexport.h"

bool MainWindow::processTTBin(const QString& filename)
{
    m_Activity.clear();
    m_Seconds.clear();
    m_HeartBeat.clear();
    m_Speed.clear();

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

    double lat=0, lng=0, lastHeart =0;
    int cnt=0;
    ui->mapWidget->clearLines();
    ui->graph->clearGraphs();

    quint64 firstTime = 0;



    TrackPointPtr prev;
    foreach( LapPtr lap, a->laps())
    {

        foreach ( TrackPointPtr tp, lap->points())
        {
            if ( tp->latitude() ==0 && tp->longitude() == 0 )
            {
                continue;
            }

            lat += tp->latitude();
            lng += tp->longitude();
            cnt++;

            if (!prev && firstTime == 0 )
            {
                prev = tp;                
                firstTime = tp->time().toTime_t();
                continue;
            }


            ui->mapWidget->addLine(prev->latitude(), prev->longitude(),tp->latitude(),tp->longitude());
            prev = tp;

            m_Seconds.append( tp->time().toTime_t() - firstTime );
            if ( tp->heartRate() > 0 )
            {
                m_HeartBeat.append( tp->heartRate() );
                lastHeart = tp->heartRate();
            }
            else
            {
                m_HeartBeat.append( lastHeart );
            }
            m_Speed.append( tp->speed() * 3.6 );



        }
    }
    ui->graph->addGraph();
    ui->graph->addGraph(ui->graph->xAxis, ui->graph->yAxis2);
    ui->graph->graph(0)->setPen( QPen(Qt::red));
    ui->graph->graph(1)->setPen( QPen(Qt::blue));

    ui->graph->graph(0)->setData(m_Seconds, m_HeartBeat);
    ui->graph->graph(1)->setData(m_Seconds, m_Speed);
    ui->graph->rescaleAxes();
    ui->graph->yAxis2->setVisible(true);
    ui->graph->yAxis2->setLabel("km/hr");
    ui->graph->yAxis->setLabel("BPM");
    ui->graph->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->graph->xAxis->setDateTimeSpec(Qt::UTC);
    ui->graph->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->mapWidget->setCenter(lat/cnt, lng/cnt);
    ui->graph->replot();


    QFile of(filename + ".tcx");
    if (!of.open(QIODevice::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Could not save tcx file %1.").arg(filename+".tcx"));
        return false;
    }


    TCXExport e;
    e.save(&of, a);
    of.close();
    return true;
}

QString MainWindow::ttdir() const
{
    return QDir::homePath() + QDir::separator() + "TomTom MySports";
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    ui->actionDownload_Workouts->setEnabled(false);

    connect(ui->graph, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(onGraphMouseMove(QMouseEvent*)));
    connect(&m_TTManager, SIGNAL(ttArrived()), this, SLOT(onWatchesChanged()));
    connect(&m_TTManager, SIGNAL(ttRemoved()), this, SLOT(onWatchesChanged()));


    ui->mapWidget->setLatitude(59.9138204);
    ui->mapWidget->setLongitude(10.7387413);

#ifdef I_UNDERSTAND_THIS_IS_DANGEROUS
    m_TTManager.startSearch();
    ui->actionDownload_Workouts->setVisible(true);
#else
    ui->actionDownload_Workouts->setVisible(false);
#endif


    m_FSModel = new QFileSystemModel(this);
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

void MainWindow::onWatchesChanged()
{
    ui->actionDownload_Workouts->setEnabled( m_TTManager.watches().count() > 0 );
}

void MainWindow::onGraphMouseMove(QMouseEvent *event)
{
    if ( !m_Activity)
    {
        return;
    }

    double pos = ui->graph->xAxis->pixelToCoord(event->pos().x());

    ui->mapWidget->clearCircles();

    TrackPointPtr pt = m_Activity->find( pos );

    if ( pt )
    {
        QString msg = QString("%1 seconds, BPM=%2, Speed=%3, Distance=%4").arg(pos).arg(pt->heartRate()).arg(pt->speed()*3.6).arg( pt->cummulativeDistance() );
        ui->statusBar->showMessage(msg,5000);
        ui->mapWidget->addCircle( pt->latitude(), pt->longitude() );
        ui->mapWidget->repaint();
        return;

    }

    // QString msg = QString("%1 seconds").arg(pos);
    // ui->statusBar->showMessage(msg,5000);
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



void MainWindow::on_actionDownload_Workouts_triggered()
{
    if ( m_TTManager.watches().count()== 0)
    {
        ui->statusBar->showMessage(tr("No TT watch detected."));
        return;
    }

    TTWatch * watch = m_TTManager.watches().first();
    if ( !watch->open() )
    {
        ui->statusBar->showMessage(tr("Could not open watch"));
        return;
    }

    watch->download(ttdir() + QDir::separator() + "ttwatcher", false);
    watch->close();
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
    QMessageBox::information(this, tr("TTWatcher"), tr("TTWatcher 1.0\nttbin tcx exporter, handles even corrupted ttbin files.\nhttps://github.com/altera2015/ttwatcher"));
}
