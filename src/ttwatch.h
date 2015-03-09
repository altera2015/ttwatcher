#ifndef TTWATCH_H
#define TTWATCH_H

#include <QObject>
#include <QList>
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

    bool sendCommand( const QByteArray & command, QByteArray & response );
    static void appendId( QByteArray & dest, const TTFile & file);
    static void buildCommand( QByteArray &dest, quint8 command, quint8 sub, const TTFile & file);
    static void buildShortCommand(QByteArray &dest, quint8 command, const TTFile & file);

    bool _openFile( const TTFile & file );
    bool _readFile( QByteArray &dest, const TTFile & file, bool processEvents = false );
    bool _createFile(const TTFile &file );
    bool _writeFile( const QByteArray & source, const TTFile & file, bool processEvents = false);
    bool _closeFile( const TTFile & file );
    bool _deleteFile( const TTFile & file );

public:
    explicit TTWatch(const QString & path, const QString & serial, QObject *parent = 0);
    virtual ~TTWatch();

    QString path() const;

    bool open();
    bool close();
    bool listFiles( TTFileList & fl );
    bool deleteFile( const TTFile & file );
    bool readFile(QByteArray & data , const TTFile & file, bool processEvents = false);
    bool writeFile(const QByteArray & source, const TTFile &file, bool processEvents = false);
    int batteryLevel();

    // convenience functions
    int download( const QString & basePath, bool deleteWhenDone );

};

#endif // TTWATCH_H
