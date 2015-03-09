#include "ttwatch.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>

#define TT_CREATE_FILE          0x02
#define TT_DELETE_FILE          0x03
#define TT_WRITE_FILE           0x04

#define TT_OPEN_FILE            0x05
#define TT_FILE_EXISTS          0x06
#define TT_READ_FILE            0x07
#define TT_CLOSE_FILE           0x0C

#define TT_READ_FIRST           0x11
#define TT_READ_NEXT            0x12

#define TT_READ_TIME            0x14  // responds with Unix Timestamp 01 16 CB 14 54 DA 15 80 ( time was 54DA15BC )
#define TT_RESET                0x1D // ???
#define TT_GET_VERSION          0x21 // responds with 01 08 07 21 31 2E 38 2E 32 35 radix: ascii: ...!1.8.25

#define TT_GET_BATTERY_LEVEL    0x23

/* File 00010100 contains GPS Quickfix data.
 * This can be downloaded from http://gpsquickfix.services.tomtom.com/fitness/sifgps.f2p3enc.ee
 */

// 02000100 holds year at 02-03 and month at 04 and day at 05... Can't find time :(
// 02000500
// 85000000 Config including heartrate zones around offset 0x60B - this also most likely holds the PC time.
// F2000000 XML Config File
// 01300100 = log?


// 0x0D (0 0), 0x0A (0 0), 0x22 (0 0), 0x22 (0 0), 0x20 (0 0), 0x0a (0 0) during startup.

bool TTWatch::sendCommand(const QByteArray &command, QByteArray &response)
{
    response.clear();
    if ( command.length() > 60 )
    {
        qDebug() << "TTWatch::sendCommand / too long";
        return false;
    }

    quint8 data[64];
    memset(data,0, 64);
    data[0] = 9; // report
    data[1] = command.length();
    data[2] = m_Counter++;
    memcpy(data+3, command.data(), command.length());

    if ( hid_write( m_Device, data, 64) < 64 )
    {
        qDebug() << "TTWatch::sendCommand / write failed.";
        return false;
    }

    response.resize(64);

    if ( hid_read( m_Device, (unsigned char*)response.data(), 64) < 64 )
    {
        qDebug() << "TTWatch::sendCommand / read failed.";
        return false;
    }

    response = response.mid(3, response[1] );

    return true;


}

void TTWatch::appendId(QByteArray &dest, const TTFile &file)
{
    dest.append((char) ((file.id >> 24)&0xff) );
    dest.append((char) ((file.id >> 16)&0xff) );
    dest.append((char) ((file.id >> 8)&0xff) );
    dest.append((char) (file.id & 0xff ) );
}

void TTWatch::buildCommand(QByteArray &dest, quint8 command, quint8 sub, const TTFile &file)
{
    dest.append((char)command);
    dest.append((char)sub);
    appendId(dest, file);
}

void TTWatch::buildShortCommand(QByteArray &dest, quint8 command,const TTFile &file)
{
    dest.append((char)command);
    appendId(dest, file);
}

bool TTWatch::_openFile(const TTFile &file)
{
    QByteArray openFile1, openFile2, response;

    buildCommand(openFile1, TT_FILE_EXISTS, 0, file);
    buildCommand(openFile2, TT_OPEN_FILE, 0, file);

    if (!sendCommand(openFile1, response))
    {
        return false;
    }

    if ( response.at(20) != 0 )
    {
        // file does not exists.
        return false;
    }

    if (!sendCommand(openFile2, response))
    {
        return false;
    }

    if ( response.at(20) != 0 )
    {
        // could not open.
        return false;
    }


    return true;
}

bool TTWatch::_readFile(QByteArray &dest, const TTFile &file, bool processEvents)
{
    dest.clear();
    const quint8 maxReadSize = 0x32;
    QByteArray read, response;
    buildCommand(read, TT_READ_FILE, 0, file);

    for ( quint32 pos = 0 ; pos < file.length; pos+= maxReadSize )
    {
        if ( pos > 0 && processEvents )
        {
            QCoreApplication::processEvents();
        }

        quint8 len = qMin( (quint32)maxReadSize, quint32(file.length - pos) );
        read[7] = (char)len;

        if (!sendCommand(read, response))
        {
            qCritical() << "TTWatch::_readFile / read files. pos = " << pos;
            return false;
        }

        // qDebug() << "READ" << response.toHex();

        if ( response.length() < 9 || (quint8)response[8] != len )
        {
            qCritical() << "TTWatch::_readFile / incorrect length returned . pos = " << pos << " len=" << (quint8)response[8];
            return false;
        }

        dest.append( response.mid(9, len) );
    }

    return true;
}

bool TTWatch::_createFile(const TTFile &file)
{
    QByteArray createFile,response;

    _deleteFile(file);

    buildCommand(createFile, TT_CREATE_FILE, 0, file);
    if (!sendCommand(createFile, response))
    {
        return false;
    }

    // TODO check response for valid file.

    return true;
}

bool TTWatch::_writeFile(const QByteArray &source, const TTFile &file, bool processEvents)
{
    if ( file.length != source.length() )
    {
        qCritical() << "TTWatch::_writeFile / length mismatch.";
        return false;
    }

    const quint8 maxWriteSize = 0x36;
    QByteArray writeCommand, response;

    buildShortCommand(writeCommand, TT_WRITE_FILE, file);

    for ( int pos = 0 ; pos < source.length(); pos+= maxWriteSize )
    {
        if ( pos > 0 && processEvents )
        {
            QCoreApplication::processEvents();
        }

        quint8 len = qMin( (quint32)maxWriteSize, quint32(source.length() - pos) );
        QByteArray cmd = writeCommand;
        cmd += source.mid(pos, len);
        cmd.append((char)0); // hardware sillyness, it appears it needs an extra byte at
        // the end to make this work. Without this byte it will not write the actual last byte in
        // each command.

        if ( !sendCommand(cmd, response))
        {
            return false;
        }
    }

    return true;

}

bool TTWatch::_closeFile(const TTFile &file)
{
    QByteArray closeFile, response;
    buildCommand(closeFile, TT_CLOSE_FILE, 0, file);

    if (!sendCommand(closeFile, response))
    {
        return false;
    }

    return true;
}

bool TTWatch::_deleteFile(const TTFile &file)
{
    QByteArray deleteFile, response;
    buildCommand(deleteFile, TT_DELETE_FILE, 0, file);

    if (!sendCommand(deleteFile, response))
    {
        return false;
    }

    return response.at(20) == 0;
}

TTWatch::TTWatch(const QString &path, const QString &serial, QObject *parent) :
    QObject(parent),
    m_Path(path),
    m_Serial(serial),
    m_Device(0),
    m_Counter(0)
{
}

TTWatch::~TTWatch()
{
    close();
}

QString TTWatch::path() const
{
    return m_Path;
}

bool TTWatch::open()
{
    if ( m_Device )
    {
        return true;
    }

    m_Device = hid_open_path( m_Path.toLocal8Bit().data() );
    m_Counter = 0;
    return ( m_Device != 0 );
}

bool TTWatch::close()
{
    if ( m_Device == 0 )
    {
        return false;
    }

    hid_close(m_Device);
    m_Device = 0;

    return true;
}

bool TTWatch::listFiles(TTFileList &fl)
{
    if ( m_Device == 0 )
    {
        qDebug() << "TTWatch::listFiles / called without opening first.";
        return false;
    }

    QByteArray readFirst;
    readFirst.append((char)TT_READ_FIRST);
    QByteArray readNext;
    readNext.append((char)TT_READ_NEXT);

    QByteArray response;

    if ( !sendCommand(readFirst, response ))
    {
        qWarning() << "TTWatch::listFiles / read first failed.";
        return false;
    }

    for (int i=0;i<0x1000;i++)
    {
        if ( !sendCommand(readNext, response))
        {
            qWarning() << "TTWatch::listFiles / read next failed. Iteration " << i;
            return false;
        }

        if ( response.length()!=22 )
        {
            qWarning() << "TTWatch::listFiles / read next failed. Length unexpected: " << response.length();
            return false;
        }

        if ( response.at(20) == 1 )
        {
            qDebug() << "TTWatch::listFiles / read " << i + 1 << " files.";
            return true;
        }

        TTFile f;

        f.length = ( (quint8)response[13] << 24) |
                   ( (quint8)response[14] << 16) |
                   ( (quint8)response[15] << 8) |
                   ( (quint8)response[16] );

        f.id =  ( (quint8)response[6] << 24) |
                ( (quint8)response[7] << 16) |
                ( (quint8)response[8] << 8) |
                ( (quint8)response[9] );


        qDebug() << f.length << QString::number(f.id, 16) << response.toHex();
        fl.append(f);
    }

    qWarning() << "TTWatch::listFiles / iteration returned more than 1000 files!";
    return false;
}

bool TTWatch::deleteFile(const TTFile &file)
{
    if ( m_Device == 0 )
    {
        qWarning() << "TTWatch::deleteFile / called without opening first.";
        return false;
    }

    return _deleteFile(file);
}

bool TTWatch::readFile(QByteArray &data, const TTFile &file, bool processEvents)
{
    if ( m_Device == 0 )
    {
        qWarning() << "TTWatch::listFiles / called without opening first.";
        return false;
    }

    if ( !_openFile(file))
    {
        return false;
    }

    bool result = _readFile(data, file, processEvents);

    _closeFile(file);

    return result;
}

bool TTWatch::writeFile(const QByteArray &source, const TTFile &file, bool processEvents)
{
    if ( m_Device == 0 )
    {
        qWarning() << "TTWatch::listFiles / called without opening first.";
        return false;
    }

    if ( !_createFile(file))
    {
        return false;
    }

    bool result = _writeFile(source, file, processEvents);

    _closeFile(file);

    return result;
}

int TTWatch::batteryLevel()
{
    if ( m_Device == 0 )
    {
        qWarning() << "TTWatch::batteryLevel / called without opening first.";
        return -1;
    }

    QByteArray command, response;
    command.append((char)TT_GET_BATTERY_LEVEL);
    command.append((char)0);

    if (!sendCommand(command, response))
    {
        qWarning() << "TTWatch::batteryLevel / failed.";
        return -1;
    }


    return (quint8)response.at(1);
}

int TTWatch::download(const QString &basePath, bool deleteWhenDone)
{
    TTFileList fl;

    if (!listFiles(fl))
    {
        return -1;
    }

    int count = 0;

    foreach ( const TTFile & file, fl)
    {
        if (! (( ( file.id & 0xFF000000) == 0x91000000 ) && file.length > 100 ) )
        {
            continue;
        }

        QByteArray fileData;

        if ( !readFile( fileData, file, true ) )
        {
            qWarning() << "TTWatch::download / failed to download " << file.id;
            continue;
        }

        if ( fileData.length() < 100 )
        {
            qWarning() << "TTWatch::download / no data in file " << file.id;
            continue;
        }

        const quint8 * data = (const quint8*) fileData.data();

        if (!( data[0] == 0x20 && data[1] >= 0x05 ))
        {
            qWarning() << "TTWatch::download / not a ttbin file " << file.id;
            continue;
        }

        QDateTime t = QDateTime::fromTime_t( (data[11] << 24) | (data[10] << 16) | (data[9] << 8 ) | data[8] );

        QString exportPath = basePath + QDir::separator() + t.date().toString("yyyy-MM-dd");
        QDir d;
        if (!d.mkpath(exportPath))
        {
            qWarning() << "TTWatch::download / could not save in path " << exportPath;
            continue;
        }

        QString filename = exportPath + QDir::separator() + "workout-" + t.time().toString("hh_mm") + ".ttbin";

        QFileInfo fi(filename);

        if ( fi.exists() && fi.size() == file.length )
        {
            qWarning() << "TTWatch::download / file already exists. " << filename << file.id;
            continue;
        }

        QFile f(filename);

        if ( !f.open( QIODevice::WriteOnly ))
        {
            qWarning() << "TTWatch::download / could not open file " << filename;
            continue;
        }

        f.write(fileData);
        f.close();
        count++;

        if ( deleteWhenDone )
        {
            if (!deleteFile( file ))
            {
                qWarning() << "TTWatch::download / could not delete file on TT " << file.id;
            }
        }
    }

    return count;
}
