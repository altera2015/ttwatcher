#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include "ttmanager.h"
#include "activity.h"
#include "qcustomplot.h"

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
    bool processTTBin(const QString& filename);
    QString ttdir() const;
    QFileSystemModel * m_FSModel;
    QCPAxis * m_Axis3;
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void onWatchesChanged();

    void onGraphMouseMove(QMouseEvent * event);

    void on_actionProcess_TTBIN_triggered();

    void on_actionExit_triggered();

    void on_actionDownload_Workouts_triggered();

    void on_actionShow_in_explorer_triggered();

    void on_treeView_clicked(const QModelIndex &index);

    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
