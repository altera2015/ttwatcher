#ifndef TTWATCH_H
#define TTWATCH_H

#include <QObject>
#include <QList>
#include <QUrl>
#include <QStringList>

#include "hidapi.h"

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

        bool close() {
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
    bool downloadPreferences( QIODevice & dest );

    // convenience functions
    QStringList download( const QString & basePath, bool deleteWhenDone );

private slots:


};

#endif // TTWATCH_H
