#include "iactivityexporter.h"

void IActivityExporter::parseExportTag(QDomElement element, const QString &idAttribute, bool &enabled, bool &autoOpen)
{
    enabled = false;

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

        enabled = true;
        autoOpen = idElement.attribute("autoOpen") == "1";

        return;
    }

}

void IActivityExporter::writeExportTag(QDomDocument &document, QDomElement &element, const QString &idAttribute, bool enabled, bool autoOpen)
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
            if ( enabled )
            {
                idElement.setAttribute("autoOpen", autoOpen ? "1" : "0");
            }
            else
            {
                idElement.parentNode().removeChild(idElement);
            }
            return;
        }
    }

    // if we get here the node didn't exist.
    if ( !enabled )
    {
        // cool nothing to do.
        return;
    }

    QDomElement e = document.createElement("export");
    e.setAttribute("id", idAttribute);
    e.setAttribute("autoOpen", autoOpen ? "1" : "0");
    element.appendChild(e);
}

IActivityExporter::IActivityExporter(QObject *parent) :
    QObject(parent)
{
}
