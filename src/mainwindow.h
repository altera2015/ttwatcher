#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include "ttmanager.h"
#include "activity.h"
#include "qcustomplot.h"
#include "elevationloader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    TTManager m_TTManager;
    ActivityPtr m_Activity;
    QVector<double> m_Seconds;
    QVector<double> m_HeartBeat;
    QVector<double> m_Cadence;
    QVector<double> m_Speed;
    QVector<double> m_Elevation;
    bool processTTBin(const QString& filename);
    QString ttdir() const;
    QFileSystemModel * m_FSModel;
    QCPAxis * m_Axis3;
    QCPAxis * m_Axis4;
    ElevationLoader m_ElevationLoader;
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onElevationLoaded(bool success, ActivityPtr activity);

    void onWatchesChanged();

    void onGraphMouseMove(QMouseEvent * event);

    void on_actionProcess_TTBIN_triggered();

    void on_actionExit_triggered();

    void on_actionDownload_Workouts_triggered();

    void on_actionShow_in_explorer_triggered();

    void on_treeView_clicked(const QModelIndex &index);

    void on_actionAbout_triggered();

    void on_actionShow_Speed_toggled(bool arg1);

    void on_actionShow_Heart_Rate_toggled(bool arg1);

    void on_actionShow_Cadence_toggled(bool arg1);

    void on_actionShow_Elevation_toggled(bool arg1);

    void on_actionGo_to_website_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
