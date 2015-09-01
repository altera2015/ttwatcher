#ifndef TTWATCH_H
#define TTWATCH_H

#include <QObject>
#include <QList>
#include <QUrl>
#include <QStringList>

#include "hidapi.h"



#define FILE_SYSTEM_FIRMWARE        (0x000000f0)
#define FILE_GPSQUICKFIX_DATA       (0x00010100)
#define FILE_GPS_FIRMWARE           (0x00010200)
#define FILE_FIRMWARE_UPDATE_LOG    (0x00013001)
#define FILE_MANIFEST1              (0x00850000)
#define FILE_MANIFEST2              (0x00850001)
#define FILE_PREFERENCES_XML        (0x00f20000)
#define FILE_TYPE_MASK              (0xffff0000)
#define FILE_RACE_DATA              (0x00710000)
#define FILE_RACE_HISTORY_DATA      (0x00720000)
#define FILE_HISTORY_DATA           (0x00730000)
#define FILE_HISTORY_SUMMARY        (0x00830000)
#define FILE_TTBIN_DATA             (0x00910000)

typedef struct ttfile {
    quint32 id;
    quint32 length;
} TTFile;
typedef QList<TTFile> TTFileList;

class TTWatch : public QObject
{
    Q_OBJECT
    QString m_Path;
    QString m_Serial;
    hid_device * m_Device;
    quint8 m_Counter;

    quint32 readquint32( const QByteArray &data, int offset ) const;
    bool sendCommand( const QByteArray & command, QByteArray & response );
    static void appendId( QByteArray & dest, const TTFile & file);

    static void buildCommand(QByteArray &dest, quint8 command, const TTFile & file);

    bool _openFile( TTFile & file );
    bool _readFile( QByteArray &dest, const TTFile & file, bool processEvents = false );
    bool _createFile(const TTFile &file );
    bool _writeFile( const QByteArray & source, const TTFile & file, bool processEvents = false);
    bool _closeFile( const TTFile & file );
    bool _deleteFile( const TTFile & file );

    class WatchOpener {
        TTWatch * m_Watch;
        bool m_ShouldClose;
    public:
        WatchOpener( TTWatch * watch ) : m_Watch(watch), m_ShouldClose(false) {

        }
        ~WatchOpener() {
            if ( m_ShouldClose )
            {
                m_Watch->close();
            }
        }

        bool open( ) {
            if ( m_Watch->isOpen() )
            {
                return true;
            }
            if ( m_Watch->open() )
            {
                m_ShouldClose = true;
                return true;
            }
            return false;
        }

        void close() {
            m_Watch->close();
        }
    };

public:
    explicit TTWatch(const QString & path, const QString & serial, QObject *parent = 0);
    virtual ~TTWatch();

    QString path() const;
    QString serial() const;

    bool open();
    bool isOpen() const;
    bool close();
    bool listFiles( TTFileList & fl );
    bool deleteFile( quint32 fileId );
    bool readFile(QByteArray & data , quint32 fileId, bool processEvents = false);
    bool writeFile(const QByteArray & source, quint32 fileId, bool processEvents = false);
    int batteryLevel();    

    bool downloadPreferences( QByteArray & data );
    bool uploadPreferences( const QByteArray & data );
    bool postGPSFix();

    // convenience functions
    QStringList download( const QString & basePath, bool deleteWhenDone );

private slots:


};

#endif // TTWATCH_H
