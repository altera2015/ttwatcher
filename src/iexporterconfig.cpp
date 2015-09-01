#include "iexporterconfig.h"


QString IExporterConfig::encodeToken(const QByteArray &token) const
{
    QByteArray dest = scrambleToken( token );

    QString result;
    foreach ( char c, dest )
    {
        result.append(QChar(c));
    }

    return result;
}


QByteArray IExporterConfig::decodeToken(const QString &token) const
{
    QByteArray source;

    foreach ( QChar c, token )
    {
        source.append( c.toLatin1() );
    }

    return scrambleToken(source);
}



QByteArray IExporterConfig::scrambleToken(const QByteArray &sourceToken) const
{
    QByteArray key;
    key.append( m_Serial.toLatin1() );
    while ( key.length() < sourceToken.length() )
    {
        key.append( char( 0x56 ) );
        key.append( char( 0x33 ) );
        key.append( char( 0x49 ) );
        key.append( char( 0x37 ) );
        key.append( char( 0x4b ) );
        key.append( char( 0x30 ) );
        key.append( char( 0x49 ) );
        key.append( char( 0x39 ) );
        key.append( char( 0x4e ) );
        key.append( char( 0x34 ) );
        key.append( char( 0x47 ) );
        key.append( char( 0x30 ) );
    }

    QByteArray scrambledToken;

    for ( int i=0;i<sourceToken.length();i++)
    {
        quint8 sc, kc;
        sc = (quint8)sourceToken.at(i);
        kc = (quint8)key.at(i);
        scrambledToken.append((char) ( sc ^ kc ));
    }

    return scrambledToken;
}

void IExporterConfig::parseExportTag(QDomElement & element, const QString &idAttribute)
{
    QDomNodeList nodes = element.elementsByTagName("export");
    for (int i=0;i<nodes.length();i++)
    {
        QDomNode node = nodes.at(i);

        if ( !node.isElement())
        {
            continue;
        }

        QDomElement idElement = node.toElement();

        if ( !idElement.attributes().contains("id") )
        {
            continue;
        }

        QString id = idElement.attribute("id");
        if ( id != idAttribute )
        {
            continue;
        }

        setValid(true);
        setAutoOpen(idElement.attribute("autoOpen") == "1");

        return;
    }
}

void IExporterConfig::writeExportTag(QDomDocument &document, QDomElement &element, const QString &idAttribute)
{
    QDomNodeList nodes = element.elementsByTagName("export");
    for (int i=0;i<nodes.length();i++)
    {
        QDomNode node = nodes.at(i);

        if ( !node.isElement())
        {
            continue;
        }

        QDomElement idElement = node.toElement();

        if ( !idElement.attributes().contains("id") )
        {
            continue;
        }

        if ( idElement.attribute("id") == idAttribute )
        {
            if ( isValid() )
            {
                idElement.setAttribute("autoOpen", isAutoOpen() ? "1" : "0");
            }
            else
            {
                idElement.parentNode().removeChild(idElement);
            }
            return;
        }
    }

    // if we get here the node didn't exist.
    if ( !isValid() )
    {
        // cool nothing to do.
        return;
    }

    QDomElement e = document.createElement("export");
    e.setAttribute("id", idAttribute);
    e.setAttribute("autoOpen", isAutoOpen() ? "1" : "0");
    element.appendChild(e);
}

void IExporterConfig::writeTag(QDomDocument &document, QDomElement &parentElement, const QString &tag, const QString &value)
{
    QDomElement oldAuthToken = parentElement.firstChildElement(tag);
    if ( !oldAuthToken.isNull() )
    {
        parentElement.removeChild(oldAuthToken);
    }

    if ( value.length() > 0 )
    {
        QDomElement e = document.createElement(tag);
        QDomText text = document.createTextNode(value);
        parentElement.appendChild(e);
        e.appendChild(text);
    }
}

QString IExporterConfig::readTag(QDomElement &parentElement, const QString &tag, const QString &def, bool *usedDef)
{
    QDomElement token = parentElement.firstChildElement(tag);
    if ( token.isNull() )
    {
        if ( usedDef != 0 )
        {
            *usedDef = true;
        }
        return def;
    }

    if ( usedDef != 0 )
    {
        *usedDef = false;
    }

    return token.text();
}

void IExporterConfig::writeEncodedTag(QDomDocument &document, QDomElement &parentElement, const QString &tag, const QByteArray &value )
{
    QString text = encodeToken(value);
    writeTag(document, parentElement, tag, text);
}

QByteArray IExporterConfig::readEncodedTag(QDomElement &parentElement, const QString &tag, const QByteArray &def)
{
    bool usedDef = false;
    QString value = readTag(parentElement, tag, "", &usedDef);
    if ( usedDef )
    {
        return def;
    }
    return decodeToken(value);
}

IExporterConfig::IExporterConfig(const QString &serial) :
    m_Changed(false),
    m_IsValid(false),
    m_IsAutoOpen(false),
    m_Serial(serial)
{
}

IExporterConfig::~IExporterConfig()
{
}

bool IExporterConfig::equals(const IExporterConfig *other)
{
    return m_IsValid == other->isValid() && m_IsAutoOpen == other->isAutoOpen();
}

bool IExporterConfig::apply(const IExporterConfig *other)
{
    m_Changed = other->m_Changed;
    m_IsValid = other->m_IsValid;
    m_IsAutoOpen = other->m_IsAutoOpen;
    return true;
}

bool IExporterConfig::changed()
{
    return m_Changed;
}

void IExporterConfig::setChanged(bool changed)
{
    m_Changed = changed;
}

bool IExporterConfig::isValid() const
{
    return m_IsValid;
}

void IExporterConfig::setValid(bool valid)
{
    if ( m_IsValid != valid )
    {
        m_IsValid = valid;
        setChanged(true);
    }
}

bool IExporterConfig::isAutoOpen() const
{
    return m_IsAutoOpen;
}

void IExporterConfig::setAutoOpen(bool autoOpen)
{
    if ( m_IsAutoOpen != autoOpen )
    {
        m_IsAutoOpen = autoOpen;
        setChanged(true);
    }
}

QString IExporterConfig::serial() const
{
    return m_Serial;
}

void IExporterConfig::reset()
{
    m_IsAutoOpen = false;
    m_IsValid = false;
    m_Changed = false;
}
