#ifndef BRIDGEEDITORDIALOG_H
#define BRIDGEEDITORDIALOG_H

#include <QDialog>
#include <QList>
#include <QFileInfo>
#include <QStatusBar>

#include "bridge.h"
#include "settings.h"
#include "bridgepointitemmodel.h"
#include "elevation.h"

namespace Ui {
class BridgeEditorDialog;
}

class BridgeEditorDialog : public QDialog
{
    Q_OBJECT

    Elevation m_Elevation;
    Settings * m_Settings;
    QList<QFileInfo> m_BridgeFiles;
    QString m_Filename;
    BridgeList m_Bridges;
    bool m_BridgeFileChanged2;
    int m_CurrentBridgeFileIndex;
    BridgePointItemModel m_Model;
    QStatusBar * m_StatusBar;

    QString baseDir() const;
    void showBridges(const QString &currentBridge);
    void loadBridge( int index );
    void loadBridgeFile( const QString & filename );
    void loadUserBridgeFiles();
    void setChanged( bool changed );
    void dialogState();
public:
    explicit BridgeEditorDialog(Settings * settings, QWidget *parent = 0);
    ~BridgeEditorDialog();

private:
    Ui::BridgeEditorDialog *ui;
private slots:
    void currentBridgeFileChanged( int index );
    void currentBridgeChanged( int index );
    void bridgePointSelected( int index, QPoint pos );
    void bridgePointUnselected( int index );
    void bridgePointMoved( int index, QPointF pos );
    void mapClicked( qreal latitude, qreal longitude );
    void mouseMoved(QPointF pos);
    void onTileChanged();
    void on_newFileButton_clicked();
    void on_newBridge_clicked();
    void on_deleteBridge_clicked();
    bool on_applyButton_clicked();
    void on_okButton_clicked();
    void on_cancelButton_clicked();

    void on_removeBridgePoint_clicked();
    void on_renameButton_clicked();
    void on_generateButton_clicked();
    void on_captureWidthSpin_valueChanged(int arg1);
};

#endif // BRIDGEEDITORDIALOG_H
