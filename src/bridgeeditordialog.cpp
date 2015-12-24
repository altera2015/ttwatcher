#include "bridgeeditordialog.h"
#include "ui_bridgeeditordialog.h"
#include <QStandardPaths>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include "elevationtiledownloaderdialog.h"

QString BridgeEditorDialog::baseDir() const
{
#ifdef TT_DEBUG
    return QApplication::applicationDirPath() + "/../../../src";
#else
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator();
#endif
}


void BridgeEditorDialog::loadBridge(int index)
{
    if ( index >= m_Bridges.count() || index < 0)
    {
        qDebug() << "BAILING!";
        dialogState();
        return;
    }

    const Bridge & bridge = m_Bridges[index];

    ui->mapWidget->clearCircles();
    m_Model.setBridge(&m_Bridges[index]);

    const BridgePointList & points = bridge.constPoints();


    QPolygonF poly;
    for(int i=0;i< points.count();i++)
    {
        const QPointF & coordinate = points.at(i).coordinate();
        poly.append(coordinate);
        ui->mapWidget->addCircle( coordinate );
    }
    QRectF bounds = poly.boundingRect();

    if ( poly.count() == 0 )
    {
        ui->mapWidget->setCenter(m_Settings->lastZoom(), m_Settings->lastLatitude(),m_Settings->lastLongitude());
    }
    else
    {
        int zoom = ui->mapWidget->boundsToZoom( bounds );
        ui->mapWidget->setCenter( zoom, bounds.center() );
        ui->mapWidget->update();;
    }

    ui->captureWidthSpin->setValue( bridge.captureWidth() );
    dialogState();
}

void BridgeEditorDialog::showBridges( const QString & currentBridge )
{
    ui->bridgeSelector->clear();
    m_Model.setBridge(nullptr);

    if ( m_Bridges.count() == 0 )
    {
        return;
    }

    ui->bridgeSelector->blockSignals(true);

    int selectIndex = -1;
    for (int i=0;i<m_Bridges.count();i++)
    {
        ui->bridgeSelector->addItem(m_Bridges[i].name());
        if ( m_Bridges[i].name() == currentBridge )
        {
            selectIndex = i;
        }
    }
    ui->bridgeSelector->blockSignals(false);

    if ( selectIndex >= 0 )
    {
        ui->bridgeSelector->setCurrentIndex(selectIndex);
        loadBridge(selectIndex);
    }
    else
    {
        ui->bridgeSelector->setCurrentIndex(0);
        loadBridge(0);

    }
}


void BridgeEditorDialog::loadBridgeFile(const QString &filename)
{
    QString currentBridge = ui->bridgeSelector->currentText();    
    setChanged(false);
    m_Bridges.clear();
    ui->bridgeSelector->clear();

    if ( !Bridge::loadBridgeFile(filename, m_Bridges))
    {
        QMessageBox::warning(this, tr("Could not load the requested file."), tr("Could not load file"));
        return;
    }

    showBridges(currentBridge);
    dialogState();
}

void BridgeEditorDialog::loadUserBridgeFiles()
{
    QStringList names;
    QString dir = baseDir();

    m_BridgeFiles.clear();
    m_Model.setBridge(nullptr);

    QDirIterator it(dir, QStringList() << "*.bridges", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        m_BridgeFiles.append( it.fileInfo() );
        names.append( it.fileName() );
    }

    QString currentFile = ui->bridgeFile->currentText();
    int selectIndex = -1;
    ui->bridgeFile->clear();

    if ( names.count() == 0 )
    {
        dialogState();
        return;
    }

    ui->bridgeFile->blockSignals(true);
    for (int i=0;i<names.count();i++)
    {
        ui->bridgeFile->addItem( names[i], QVariant::fromValue(m_BridgeFiles[i]));
        if ( names[i] == currentFile )
        {
            selectIndex = i;
        }
    }
    ui->bridgeFile->blockSignals(false);

    if ( selectIndex >= 0 )
    {
        ui->bridgeFile->setCurrentIndex(selectIndex);
        currentBridgeFileChanged(selectIndex);
    }
    else
    {        
        ui->bridgeFile->setCurrentIndex(0);
        currentBridgeFileChanged(0);
    }
    dialogState();
}

void BridgeEditorDialog::setChanged(bool changed)
{
    m_BridgeFileChanged2 = changed;
    ui->applyButton->setEnabled( changed );
}

void setAllChildrenState( QWidget * widget, bool state )
{
    widget->setEnabled(state);
    foreach ( QWidget * child, widget->findChildren<QWidget*>() )
    {
        child->setEnabled(state);
        setAllChildrenState(child, state);
    }
}

void BridgeEditorDialog::dialogState()
{
    bool hasBridge = ui->bridgeSelector->currentIndex() >= 0;
    setAllChildrenState( ui->fileMapWidget, hasBridge );
    ui->mapPoints->setEnabled(hasBridge);
    setAllChildrenState( ui->interpolateWidget, hasBridge);

    bool hasFile = ui->bridgeFile->currentIndex() >= 0;
    ui->bridgeSelector->setEnabled(hasFile);
    ui->newBridge->setEnabled(hasFile);
    ui->renameButton->setEnabled(hasFile);
    ui->deleteBridge->setEnabled(hasFile);
}

BridgeEditorDialog::BridgeEditorDialog(Settings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BridgeEditorDialog),
    m_Settings(settings),
    m_BridgeFileChanged2(false),
    m_CurrentBridgeFileIndex(-1)
{
    ui->setupUi(this);

    m_StatusBar = new QStatusBar(this);
    ui->verticalLayout->addWidget(m_StatusBar);

    connect(ui->bridgeFile, SIGNAL(currentIndexChanged(int)), this, SLOT(currentBridgeFileChanged(int)));
    connect(ui->bridgeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(currentBridgeChanged(int)));
    connect(ui->mapWidget, SIGNAL(circleSelected(int,QPoint)), this, SLOT(bridgePointSelected(int,QPoint)));
    connect(ui->mapWidget, SIGNAL(circleUnselected(int)), this, SLOT(bridgePointUnselected(int)));
    connect(ui->mapWidget, SIGNAL(circleMoved(int,QPointF)), this, SLOT(bridgePointMoved(int,QPointF)));
    connect(ui->mapWidget, SIGNAL(mouseUp(qreal,qreal)), this, SLOT(mapClicked(qreal,qreal)));
    connect(ui->mapWidget, SIGNAL(mouseMove(QPointF)), this, SLOT(mouseMoved(QPointF)));


    ui->mapWidget->setCenter(m_Settings->lastZoom(), m_Settings->lastLatitude(), m_Settings->lastLongitude());
    ui->mapPoints->setModel(&m_Model);
    ui->mapWidget->allowCircleDragging(true);
    ui->mapWidget->setMouseTracking(true);


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
                ui->tileCombo->addItem( entry["name"].toString(), entry);

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

        ui->tileCombo->setCurrentIndex(tileSelectIndex);
        onTileChanged();
    }


    connect(ui->tileCombo,SIGNAL(currentIndexChanged(int)), this, SLOT(onTileChanged()));


    m_Elevation.prepare();
    loadUserBridgeFiles();
    setChanged(false);

}

BridgeEditorDialog::~BridgeEditorDialog()
{
    delete ui;
}

void BridgeEditorDialog::currentBridgeFileChanged(int index)
{
    if ( m_CurrentBridgeFileIndex >= 0 && m_BridgeFileChanged2 )
    {
        switch (QMessageBox::warning(this, tr("Bridge File Changed"), tr("You have made changes to the bridge file, do you want to save it"), QMessageBox::Yes | QMessageBox::No ) )
        {
        case QMessageBox::Yes:
            on_applyButton_clicked();
            break;
        case QMessageBox::No:
            break;
        case QMessageBox::Cancel:
            ui->bridgeFile->setCurrentIndex(m_CurrentBridgeFileIndex);
            return;
        }
    }

    qDebug() << "currentBridgeFileChanged" << index;
    if ( index < 0 )
    {
        // disable shit.
        m_CurrentBridgeFileIndex = -1;
    }
    else
    {
        m_CurrentBridgeFileIndex = index;
        loadBridgeFile( m_BridgeFiles[index].absoluteFilePath() );
    }
}

void BridgeEditorDialog::currentBridgeChanged(int index)
{
    qDebug() << "currentBridgeChanged" << index;
    loadBridge( index );
}

void BridgeEditorDialog::bridgePointSelected(int index, QPoint pos)
{
    Q_UNUSED(pos);
    m_Model.selectPoint(index);
    ui->removeBridgePoint->setEnabled(true);
}

void BridgeEditorDialog::bridgePointUnselected(int index)
{
    Q_UNUSED(index);
    m_Model.selectPoint(-1);
    ui->removeBridgePoint->setEnabled(false);
}

void BridgeEditorDialog::bridgePointMoved(int index, QPointF pos)
{
    if ( index >= m_Model.rowCount(QModelIndex()))
    {
        return;
    }

    setChanged(true);
    m_Model.bridge()->points()[index].setCoordinate(pos);
    m_Model.bridgePointUpdated(index);
}

void BridgeEditorDialog::mapClicked(qreal latitude, qreal longitude)
{
    if ( !ui->addBridgePoint->isChecked() )
    {
        return;
    }
    ui->addBridgePoint->setChecked(false);

    QPointF pos(longitude, latitude);
    float elevation = 0.0;
    if ( m_Elevation.elevation(pos, elevation) == Elevation::ER_NO_TILE )
    {
        ElevationSource source = m_Elevation.dataSources(pos);
        if ( source.valid )
        {
            ElevationTileDownloaderDialog d(m_Elevation.basePath());
            d.addSource(source);
            d.go();
            if ( d.exec() == QDialog::Accepted )
            {
                mapClicked(latitude, longitude);
                return;
            }
        }
    }

    setChanged(true);
    m_Model.addPoint(pos, elevation);
    ui->mapWidget->addCircle( pos );
    ui->mapWidget->update();
}

void BridgeEditorDialog::mouseMoved(QPointF pos)
{
    float elevation;
    if ( m_Elevation.elevation(pos, elevation) == Elevation::ER_SUCCESS )
    {
        m_StatusBar->showMessage( tr("Latitude %1, Longitude %2, Elevation %3m").arg(pos.y()).arg(pos.x()).arg(elevation));
    }
    else
    {
        m_StatusBar->showMessage( tr("Latitude %1, Longitude %2").arg(pos.y()).arg(pos.x()));
    }
}

void BridgeEditorDialog::onTileChanged()
{
    QJsonObject o = ui->tileCombo->currentData().toJsonObject();
    ui->mapWidget->setTilePath(o["url"].toString(), o["copyright"].toString());
    m_Settings->setTileUrl(o["url"].toString());
    m_Settings->save();
}

void BridgeEditorDialog::on_newFileButton_clicked()
{
    QString bridges = "*.bridges";
    QString fn = QFileDialog::getSaveFileName(this, tr("Create new Bridges file."), baseDir(), "*.bridges",&bridges);
    if ( fn.length() == 0 )
    {
        return;
    }
    if ( !fn.endsWith(".bridges"))
    {
        fn.append(".bridges");
    }

    QFile f(fn);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate );
    f.write("[]");
    f.close();

    loadUserBridgeFiles();
    for (int i=0;i<m_BridgeFiles.count();i++)
    {
        qDebug() << m_BridgeFiles[i].filePath() << fn;
        if ( m_BridgeFiles[i].filePath() == fn )
        {
            ui->bridgeFile->setCurrentIndex(i);
            currentBridgeFileChanged(i);
            break;
        }
    }
}

void BridgeEditorDialog::on_newBridge_clicked()
{
    bool ok= true;
    QString name = QInputDialog::getText(this, tr("Bridge Name"), tr("Enter bridge name."),QLineEdit::Normal, QString(), &ok);
    if ( name.length() == 0 || ok == false)
    {
        return;
    }

    setChanged(true);
    Bridge b(name);
    m_Bridges.append( b );
    showBridges(name);
}

void BridgeEditorDialog::on_deleteBridge_clicked()
{
    int currentIndex = ui->bridgeSelector->currentIndex();
    if ( currentIndex < 0 )
    {
        return;
    }

    if ( QMessageBox::warning(this, tr("Are you sure you want to delete this?"), tr("Are you sure you want to delete this bridge, you cannot undo it."), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel ) == QMessageBox::Yes )
    {
        m_Bridges.removeAt(currentIndex);
    }
    setChanged(true);
    showBridges("");
}

bool BridgeEditorDialog::on_applyButton_clicked()
{
    QString filename = m_BridgeFiles[m_CurrentBridgeFileIndex].absoluteFilePath();
    if ( !Bridge::saveBridgeFile(filename, m_Bridges) )
    {
        QMessageBox::warning(this, tr("Could not save file."), tr("Could not save file."));
        return false;
    }
    setChanged(false);
    return true;
}

void BridgeEditorDialog::on_okButton_clicked()
{
    if ( on_applyButton_clicked())
    {
        accept();
    }
}

void BridgeEditorDialog::on_cancelButton_clicked()
{
    if ( m_BridgeFileChanged2 )
    {
        if ( QMessageBox::warning(this, tr("Data changed!"), tr("Data was changed, are you sure you want to close this dialog and loose the changes?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel ) != QMessageBox::Yes )
        {
            return;
        }
    }
    reject();
}

void BridgeEditorDialog::on_removeBridgePoint_clicked()
{
    int index = m_Model.selectedPoint();
    if ( index < 0 || index > m_Model.bridge()->points().count() )
    {
        return;
    }

    setChanged(true);
    ui->mapWidget->removeCircle( index );
    m_Model.removePoint( index );
}

void BridgeEditorDialog::on_renameButton_clicked()
{
    bool ok;

    QString name = "";
    QString newName = QInputDialog::getText(this, tr("Bridge Name"), tr("Enter bridge name"), QLineEdit::Normal, name, &ok);
    if ( ok && newName.length() > 0 )
    {
        m_Model.bridge()->setName(newName);
        ui->bridgeSelector->setItemText(ui->bridgeSelector->currentIndex(), newName);
        setChanged(true);
    }
}

// -1 < x < 1
// cubic polynomal bridge simulator.
double bridgeSim ( double base, double center, double x)
{
    double xsq = x*x;
    return center + 2 * ( base - center ) * xsq + ( center - base ) * xsq * xsq;
}

void BridgeEditorDialog::on_generateButton_clicked()
{
    // just estimate bridge profile to be a normal distribution.
    if ( m_Model.bridge() == nullptr )
    {
        QMessageBox::warning(this, tr("No Bridge"), tr("No bridge selected, add one first."));
        return;
    }

    BridgePointList & bpl = m_Model.bridge()->points();
    if ( bpl.count() < 2 )
    {
        QMessageBox::warning(this, tr("Not enough points."), tr("Please mark both ends of the bridge and set the center height. Then press generate again."));
        return;
    }

    if ( bpl.count() > 2 )
    {
        if ( QMessageBox::warning(this, tr("Generate requires 2 points."), tr("The interpolation function only uses the two outer points to calculate the bridge points. You have more than two, pressing yes will remove all intermediate points."), QMessageBox::Yes | QMessageBox::No ) == QMessageBox::No )
        {
            return;
        }

        while ( bpl.count() > 2 )
        {
            bpl.removeAt(1);
        }
    }

    QPointF a = bpl.at(0).coordinate();
    float ea = bpl.at(0).elevation();
    QPointF b = bpl.at(1).coordinate();
    float eb = bpl.at(1).elevation();
    float ce = ui->heightBox->value();

    QLineF l(a,b);

    int steps = ui->stepBox->value();
    steps &= 0xfe; // make it even.
    int halfway = steps / 2;

    BridgePointList newList;
    newList.append( bpl.at(0));

    for (int i=1;i<=halfway;i++)
    {
        double pos = (double)i / (double) steps ;
        double bridgePos = pos * 2.0 - 1.0;
        QPointF p = l.pointAt( pos );
        float e = bridgeSim( ea, ce, bridgePos );
        BridgePoint bp(p, e);
        newList.append(bp);
    }

    for (int i=halfway+1;i<steps;i++)
    {
        double pos = (double)i / (double) steps ;
        double bridgePos = pos * 2.0 - 1.0;
        QPointF p = l.pointAt( pos );
        float e = bridgeSim( eb, ce, bridgePos );
        BridgePoint bp(p, e);
        newList.append(bp);
    }

    newList.append( bpl.at(1));

    m_Model.replacePoints( newList );

    ui->mapWidget->clearCircles();
    for (int i=0;i<bpl.count();i++)
    {
        ui->mapWidget->addCircle( bpl.at(i).coordinate() );
    }
    ui->mapWidget->update();
    setChanged(true);
}

void BridgeEditorDialog::on_captureWidthSpin_valueChanged(int arg1)
{
    if ( m_Model.bridge() == nullptr )
    {
        return;
    }
    m_Model.bridge()->setCaptureWidth(arg1);
}
