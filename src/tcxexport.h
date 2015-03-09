#ifndef TCXEXPORT_H
#define TCXEXPORT_H

#include <QIODevice>
#include <QXmlStreamWriter>


#include "activity.h"

class TCXExport
{    
public:
    TCXExport();

    void save( QIODevice * dev, ActivityPtr activity );
};

#endif // TCXEXPORT_H
