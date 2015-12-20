#include <QDebug>
#include "elevation.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QDir>
#include "gridfloatelevationtile.h"
#include "srtmelevationtile.h"
#include <QStandardPaths>

Elevation::Elevation(const QString &basePath) :
    m_BasePath(basePath),
    m_LastTile(0)
{
    if ( m_BasePath.length() == 0 )
    {

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
        m_BasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "data/";
#else
        m_BasePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator() + "data/";
#endif

        qDebug() << "Elevation using " << m_BasePath;
    }

    if (!( m_BasePath.endsWith("/") || m_BasePath.endsWith("\\")))
    {
        m_BasePath += QDir::separator();
    }

    QDir d(m_BasePath);

    if ( !d.exists() )
    {
        d.mkpath(basePath);
    }
}

Elevation::~Elevation()
{
    clear();
}

void Elevation::clear()
{
    foreach( ElevationTile * et, m_Tiles)
    {
        et->deleteLater();
    }
    m_Tiles.clear();
}

void Elevation::prepare()
{
    m_LastTile = nullptr;
    clear();

    QStringList filters;

    filters.append("*.hgt"); // SRTM
    filters.append("*.hdr"); // NED
    filters.append("*.flt"); // NED

    QDirIterator it(m_BasePath, filters, QDir::AllEntries, QDirIterator::Subdirectories);
    while ( it.hasNext())
    {
        it.next();

        QFileInfo fi = it.fileInfo();
        if ( fi.completeSuffix() == "hdr" )
        {
            // found a GridFloat entry!
            QString fn = fi.path() + QDir::separator() + fi.baseName();
            GridFloatElevationTile * gf = new GridFloatElevationTile( fn, this );
            if ( !gf->readHeader() )
            {
                qDebug() << "Elevation::prepare / failed to load " << fn;
            }
            else
            {
                qDebug() << "Elevation::prepare /loaded " <<qSetRealNumberPrecision( 20 )<< fn << gf->bottomLeft() << gf->topRight();
                m_Tiles.append( gf );
            }
        }
        if ( fi.completeSuffix() == "hgt" )
        {
            QString fn = fi.path() + QDir::separator() + fi.fileName();
            SRTMElevationTile * srtm = new SRTMElevationTile( fn, this );
            if ( !srtm->readHeader() )
            {
                qDebug() << "Elevation::prepare / failed to load " << fn;
            }
            else
            {
                qDebug() << "Elevation::prepare /loaded " <<qSetRealNumberPrecision( 20 )<< fn << srtm->bottomLeft() << srtm->topRight();
                m_Tiles.append( srtm );
            }
        }
    }
}

QString Elevation::basePath() const
{
    return m_BasePath;
}

bool Elevation::hasElevation(const QPointF &p)
{
    if ( m_LastTile && m_LastTile->contains(p))
    {
        return true;
    }

    foreach( ElevationTile * et, m_Tiles)
    {
        if (et->contains(p))
        {
            return true;
        }
    }

    return false;
}

Elevation::ElevationResult Elevation::elevation(const QPointF &p, float &elevation)
{
    if ( m_LastTile && m_LastTile->contains(p))
    {
        return m_LastTile->elevation(p, elevation) ? ER_SUCCESS : ER_NO_DATA;
    }

    ElevationTile * bestTile = 0;
    foreach( ElevationTile * et, m_Tiles)
    {
        if ( et->contains(p))
        {
            if ( bestTile == 0 )
            {
                bestTile = et;
            }
            else
            {
                if ( et->arcSeconds() < bestTile->arcSeconds() )
                {
                    bestTile = et;
                }
            }
        }
    }

    if ( bestTile != 0 )
    {
        m_LastTile = bestTile;
        if ( !m_LastTile->open() )
        {
            m_Tiles.removeOne(m_LastTile);
            m_LastTile->removeFiles();
            m_LastTile->deleteLater();
            m_LastTile = nullptr;
            return this->elevation(p, elevation);
        }

        return m_LastTile->elevation(p, elevation) ? ER_SUCCESS : ER_NO_DATA;
    }
    else
    {
        qDebug() << "Elevation::elevation / no tile for point " << p;
        elevation = 0.0;
        return ER_NO_TILE;
    }
}

ElevationSource Elevation::dataSources(const QPointF &p)
{
    ElevationSourceList list;

    list.append( GridFloatElevationTile::dataSetURLs(p) );

    list.append( SRTMElevationTile::dataSetURLs(p) );


    qSort(list.begin(), list.end(), [](ElevationSource &a, ElevationSource &b) {
        return a.arcSeconds < b.arcSeconds;
    });

    if ( list.length() > 0 )
    {
        return list.at(0);
    }


    return ElevationSource();
}
