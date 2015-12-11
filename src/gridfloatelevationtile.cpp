#include "gridfloatelevationtile.h"
#include <QDebug>
#include <QStringList>
#include <QRegExp>
#include <QFileInfo>
#include <QtEndian>
#include "settings.h"




GridFloatElevationTile::GridFloatElevationTile(const QString &filename, QObject *parent) :
    ElevationTile(parent),
    m_Filename(filename),
    m_Data(nullptr),
    m_NoData(999),
    m_LSBFirst(true)
{
}

GridFloatElevationTile::~GridFloatElevationTile()
{
    close();
}



bool GridFloatElevationTile::readHeader()
{
    QFile f(m_Filename + ".hdr");
    if ( !f.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("GridFloatElevationTile::readHeader / %1.hdr file not found.").arg(m_Filename);
        return false;
    }

    #define NCOL_PROP       (1<<0)
    #define NROW_PROP       (1<<1)
    #define XCORNER_PROP    (1<<2)
    #define YCORNER_PROP    (1<<3)
    #define CELLSIZE_PROP   (1<<4)
    #define ALL_PROP        ((1<<5)-1)


    int props = 0;

    while ( !f.atEnd() )
    {
        QString s = f.readLine();
        s = s.toLower();
        QStringList sl = s.split(QRegExp("\\s"), QString::SkipEmptyParts );
        if ( sl.count() != 2 )
        {
            continue;
        }

        if ( sl[0] == "ncols" )
        {
            m_Cols = sl[1].toInt();
            props |= NCOL_PROP;
            continue;
        }
        if ( sl[0] == "nrows" )
        {
            m_Rows = sl[1].toInt();
            props |= NROW_PROP;
            continue;
        }
        if ( sl[0] == "xllcorner")
        {
            m_BottomLeft.setX( sl[1].toDouble());
            // m_Bounds.setLeft( sl[1].toDouble());
            props |= XCORNER_PROP;
            continue;
        }
        if ( sl[0] == "yllcorner")
        {
            m_BottomLeft.setY( sl[1].toDouble());
            // m_Bounds.setBottom( sl[1].toDouble());
            props |= YCORNER_PROP;
            continue;
        }
        if ( sl[0] == "cellsize")
        {
            m_CellSize = sl[1].toDouble();
            props |= CELLSIZE_PROP;
            continue;
        }

        if ( sl[0] == "nodata_value")
        {
            m_NoData = sl[1].toFloat();
            continue;
        }

        if ( sl[0] == "byteorder")
        {
            m_LSBFirst = sl[1] == "lsbfirst";
            continue;
        }
    }

    if ( props != ALL_PROP )
    {
        qDebug() << QString("GridFloatElevationTile::readHeader / Not all properties read from %1.hdr file ").arg(m_Filename) << props;
        return false;
    }

    m_TopRight.setX( m_CellSize * (m_Cols - 1) + m_BottomLeft.x() );
    m_TopRight.setY( m_CellSize * (m_Rows - 1) + m_BottomLeft.y() );


    QFileInfo fi(m_Filename + ".flt");
    if (!fi.exists())
    {
        qDebug() << QString("GridFloatElevationTile::readHeader / %1.flt does not exists").arg(m_Filename);
        return false;
    }

    m_Filesize = fi.size();

    if ( m_Filesize != m_Cols * m_Rows * 4 )
    {
        qDebug() << QString("GridFloatElevationTile::readHeader / %1.flt incorrect file size.").arg(m_Filename);
        return false;
    }

    return true;
}

bool GridFloatElevationTile::open()
{
    if ( m_File.isOpen() )
    {
        return true;
    }

    m_File.setFileName(m_Filename + ".flt");

    if (!m_File.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("GridFloatElevationTile::open / Could not open %1.flt").arg(m_Filename);
        return false;
    }

    m_Data = (float*) m_File.map(0, m_File.size() );
    return true;
}

void GridFloatElevationTile::close()
{
    m_File.close();
    m_Data = nullptr;
}

bool GridFloatElevationTile::isOpen() const
{
    return m_File.isOpen();
}

QPointF GridFloatElevationTile::bottomLeft() const
{
    return m_BottomLeft;
}

QPointF GridFloatElevationTile::topRight() const
{
    return m_TopRight;
}

bool GridFloatElevationTile::contains(const QPointF &p) const
{
    bool ok = p.x() >= m_BottomLeft.x();
    ok &= p.x() <= m_TopRight.x();
    ok &= p.y() >= m_BottomLeft.y();
    ok &= p.y() <= m_TopRight.y();
    return ok;
}

float GridFloatElevationTile::arcSeconds() const
{
    return m_CellSize * 3600;
}

bool GridFloatElevationTile::elevation(const QPointF &p, float &elevation) const
{
    elevation = 0.0;
    if ( !m_File.isOpen() )
    {
        qDebug() << "GridFloatElevationTile::elevation / Tile not open " << p;
        return false;
    }

    QPointF refFrame = p - m_BottomLeft;
    QPointF refFrameScaled = refFrame / m_CellSize;
    QPoint  c = refFrameScaled.toPoint();

    if ( c.y() >= m_Rows || c.y() < 0 || c.x() >= m_Cols || c.x() < 0 )
    {
        qDebug() << "GridFloatElevationTile::elevation / invalid coordinate for this tile." << p;
        return false;
    }

    int offset = ( m_Rows - c.y() - 1 ) * m_Cols + c.x();
    if ( offset >= 0 && offset < (m_Filesize/sizeof(float)) )
    {

        typedef union {
            quint32 i;
            float f;
        } FloatUInt32Union;

        FloatUInt32Union * dataFq = (FloatUInt32Union*) m_Data;

        if ( m_LSBFirst)
        {
            FloatUInt32Union v;
            v.i = qFromLittleEndian( dataFq[offset].i );
            elevation = v.f;
        }
        else
        {
            FloatUInt32Union v;
            v.i = qFromBigEndian( dataFq[offset].i );
            elevation = v.f;
        }

        if ( qFuzzyCompare(elevation, m_NoData))
        {
            qDebug() << "GridFloatElevationTile::elevation / no data." << p;
            return false;
        }

        return true;
    }

    qDebug() << "GridFloatElevationTile::elevation / Invalid Index requested." << p << offset << m_CellSize << m_Cols;
    return false;
}

bool GridFloatElevationTile::removeFiles()
{
    close();
    QFile f;
    f.setFileName(m_Filename + ".hdr");
    f.remove();
    f.setFileName(m_Filename + ".flt");
    f.remove();
    return true;
}

class DataSetSource {
public:

    DataSetSource( QString baseUrl, QRectF bounds, QString baseDir, double arcSeconds ) : m_BaseUrl(baseUrl), m_Bounds(bounds), m_BaseDir(baseDir), m_ArcSeconds(arcSeconds) {}
    QString m_BaseUrl;
    QRectF m_Bounds;
    QString m_BaseDir;
    double m_ArcSeconds;
};

bool GridFloatElevationTile::lookup(const QString &list, const QString &fn)
{
    QFile f(list);
    if ( !f.open(QIODevice::ReadOnly))
    {
        return false;
    }


    while(!f.atEnd())
    {
        QString l = f.readLine();
        l = l.trimmed();
        if ( l == fn )
        {
            f.close();
            return true;
        }
    }

    f.close();
    return false;
}

ElevationSourceList GridFloatElevationTile::dataSetURLs(const QPointF &p)
{

    QString baseUrl = "ftp://rockyftp.cr.usgs.gov/";


    // DTED convention NORTHWEST / UPPER LEFT CORNER
    int lat = ceil(p.y());
    int lng = floor(p.x());

    char ns = p.y() > 0 ? 'n' : 's';
    char ew = p.x() > 0 ? 'e' : 'w';

    int slat = lat;
    int slng = lng;

    lat = abs(lat);
    lng = abs(lng);

    ElevationSourceList sourceList;

    QString s1 = QString("%1%2%3%4.zip").arg(ns).arg(lat, 2, 10, QChar('0')).arg(ew).arg(lng, 3, 10, QChar('0'));

    //we have NED 0.3333 but these files are huge.

    Settings * settings = Settings::get();
    if ( settings->useHighResolutionElevation() )
    {
        if ( lookup(":/ned13.txt", s1) )
        {
            QUrl u(baseUrl + "/vdelivery/Datasets/Staged/NED/13/GridFloat/" + s1 );
            sourceList.append( ElevationSource( u, "/NED/13/", 0.3333333, "3DEP / USGD", QPointF(slng, slat-1)) );
            return sourceList;
        }
    }

    if ( lookup(":/ned1.txt", s1) )
    {
        QUrl u(baseUrl + "/vdelivery/Datasets/Staged/NED/1/GridFloat/" + s1 );
        sourceList.append( ElevationSource( u, "/NED/1/", 1, "3DEP / USGD NED 1", QPointF(slng, slat-1)) );
        return sourceList;
    }


    if ( lookup(":/ned2.txt", s1) )
    {
        QUrl u(baseUrl + "/vdelivery/Datasets/Staged/NED/2/GridFloat/" + s1 );
        sourceList.append( ElevationSource( u, "/NED/2/", 2, "3DEP / USGD NED 2", QPointF(slng, slat-1)) );
        return sourceList;
    }

    return sourceList;
}



