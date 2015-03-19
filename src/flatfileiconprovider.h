#ifndef FLATFILEICONPROVIDER_H
#define FLATFILEICONPROVIDER_H

#include <QFileIconProvider>
#include <QIcon>

class FlatFileIconProvider : public QFileIconProvider
{
    QIcon m_File;
    QIcon m_Folder;
public:
    explicit FlatFileIconProvider();
    QIcon icon(IconType type) const;
    QIcon icon(const QFileInfo &info) const;
signals:

public slots:

};

#endif // FLATFILEICONPROVIDER_H
