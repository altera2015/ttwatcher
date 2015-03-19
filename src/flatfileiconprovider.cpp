#include "flatfileiconprovider.h"


FlatFileIconProvider::FlatFileIconProvider() :
    m_File( ":/icons/watch_small.png"),
    m_Folder( ":/icons/folder250.png")
{

}

QIcon FlatFileIconProvider::icon(QFileIconProvider::IconType type) const
{
    switch ( type )
    {
        case QFileIconProvider::File:
            return m_File;
    default:
        return m_Folder;
    }
}

QIcon FlatFileIconProvider::icon(const QFileInfo &info) const
{
    if ( info.isDir() )
    {
        return m_Folder;
    }

    return m_File;
}
