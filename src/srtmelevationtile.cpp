#include "srtmelevationtile.h"
#include <QDebug>
#include <QFileInfo>
#include <QtEndian>

// http://dds.cr.usgs.gov/srtm/version2_1/Documentation/SRTM_Topo.pdf
// http://gis.stackexchange.com/questions/43743/how-to-extract-elevation-from-hgt-file
// https://dds.cr.usgs.gov/srtm/version2_1/SRTM3/

SRTMElevationTile::SRTMElevationTile(const QString & filename, QObject *parent) :
    ElevationTile(parent),
    m_Filename(filename),
    m_Data(nullptr),
    m_Set(UNKNOWN)
{
}

SRTMElevationTile::~SRTMElevationTile()
{
    close();
}


bool SRTMElevationTile::readHeader()
{
    // Just use filesize to determine if it's a SRTM3 or SRTM1 tile
    QFileInfo f(m_Filename);
    m_Filesize = f.size();

    QString filename = f.fileName();
    filename = filename.toLower();

    QChar latIn = filename.at(0);
    QChar lngIn = filename.at(3);
    QString latStr = filename.mid(1,2);
    QString lngStr = filename.mid(4,3);
    int lat = latStr.toInt();
    int lng = lngStr.toInt();
    if ( latIn == QChar('s') )
    {
        lat = -lat;
    }
    if ( lngIn == QChar('w') )
    {
        lng = -lng;
    }

    m_TopRight = QPointF(lng + 1.0, lat + 1.0);
    m_BottomLeft = QPointF(lng, lat );


    if ( m_Filesize == 3601 * 3601 * 2 ) // STRM1
    {
        m_ArcSeconds = 1.0;
        m_CellSize = m_ArcSeconds / 3600;
        m_Set = SRTM1;
        m_Rows = 3601;
        m_Cols = 3601;
        return true;
    }
    if ( m_Filesize == 1201 * 1201 * 2 ) // STRM3
    {
        m_ArcSeconds = 3.0;
        m_CellSize = m_ArcSeconds / 3600;
        m_Set = SRTM3;
        m_Rows = 1201;
        m_Cols = 1201;
        return true;
    }

    m_Set = UNKNOWN ;
    return false;
}

bool SRTMElevationTile::open()
{
    if ( m_File.isOpen() )
    {
        return true;
    }

    m_File.setFileName(m_Filename);

    if (!m_File.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("SRTMElevationTile::open / Could not open %1").arg(m_Filename);
        return false;
    }

    m_Data = (qint16*) m_File.map(0, m_File.size() );
    return true;
}

void SRTMElevationTile::close()
{
    m_File.close();
    m_Data = nullptr;
}

bool SRTMElevationTile::isOpen() const
{
    return m_File.isOpen();
}

QPointF SRTMElevationTile::bottomLeft() const
{
    return m_BottomLeft;
}

QPointF SRTMElevationTile::topRight() const
{
    return m_TopRight;
}

bool SRTMElevationTile::contains(const QPointF &p) const
{
    bool ok = p.x() >= m_BottomLeft.x();
    ok &= p.x() <= m_TopRight.x();
    ok &= p.y() >= m_BottomLeft.y();
    ok &= p.y() <= m_TopRight.y();
    return ok;
}

float SRTMElevationTile::arcSeconds() const
{
    return m_ArcSeconds;
}

bool SRTMElevationTile::elevation(const QPointF &p, float &elevation) const
{
    elevation = 0.0;
    if ( !m_File.isOpen() )
    {
        qDebug() << "SRTMElevationTile::elevation / Tile not open " << p;
        return false;
    }

    QPointF refFrame = p - m_BottomLeft;
    QPointF refFrameScaled = refFrame / m_CellSize;
    QPoint  c = refFrameScaled.toPoint();

    if ( c.y() >= m_Rows || c.y() < 0 || c.x() >= m_Cols || c.x() < 0 )
    {
        qDebug() << "SRTMElevationTile::elevation / invalid coordinate for this tile." << p;
        return false;
    }

    int offset = ( m_Rows - c.y() - 1 ) * m_Cols + c.x();
    if ( offset >= 0 && offset < ( m_Filesize / sizeof(qint16)) )
    {
        qint16 e;
        e = qFromBigEndian( m_Data[offset]);

        if ( e == -32768 )
        {
            qDebug() << "SRTMElevationTile::elevation / no data." << p;
            return false;
        }

        elevation = e;
        return true;
    }

    qDebug() << "SRTMElevationTile::elevation / Invalid Index requested." << p << offset << m_Set;
    return false;
}

bool SRTMElevationTile::removeFiles()
{
    close();
    QFile f;
    f.setFileName(m_Filename);
    f.remove();
    return true;
}

//http://gis.stackexchange.com/questions/43743/how-to-extract-elevation-from-hgt-file
//http://dds.cr.usgs.gov/srtm/version2_1/Documentation/SRTM_Topo.pdf


ElevationSourceList SRTMElevationTile::dataSetURLs(const QPointF &p)
{

    QString baseUrl = "https://dds.cr.usgs.gov";


    // DTED convention SOUTHWEST / LOWER LEFT CORNER
    int lat = floor(p.y());
    int lng = floor(p.x());

    char ns = p.y() > 0 ? 'N' : 'S';
    char ew = p.x() > 0 ? 'E' : 'W';

    int slat = lat;
    int slng = lng;

    lat = abs(lat);
    lng = abs(lng);

    ElevationSourceList sourceList;

    QString s1 = QString("%1%2%3%4.hgt.zip").arg(ns).arg(lat, 2, 10, QChar('0')).arg(ew).arg(lng, 3, 10, QChar('0'));
    QFile lookup(":/srtm.txt");
    if ( !lookup.open(QIODevice::ReadOnly))
    {
        qDebug() << "SRTMElevationTile::dataSetURLs / none found.";
        return sourceList;
    }

    QString midUrl = "";
    while ( !lookup.atEnd())
    {
        QString l = lookup.readLine();
        l = l.trimmed();
        if ( l.startsWith("/"))
        {
            midUrl = l;
        }
        else
        {
            if ( l == s1 )
            {
                QUrl u(baseUrl + midUrl + "/" + s1 );
                sourceList.append( ElevationSource( u, midUrl, 3, "SRTM / USGS / NASA", QPointF(slng,slat)) );
                break;
            }
        }
    }

    return sourceList;
}
