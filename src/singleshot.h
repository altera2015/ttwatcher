#ifndef SINGLESHOT_H
#define SINGLESHOT_H

#include <QTimer>
#include <QObject>
#include <functional>

/* This class allows you to create a fire-and-forget QTimer::singleShot that
 * uses std::function. */


class  SingleShot : public QObject
{
    Q_OBJECT
    QTimer m_Timer;
    bool m_AutoDelete;

public:

    typedef std::function< void() > Callback;    
    explicit SingleShot(Callback cb, int ms = 0, bool autoDelete=true, QObject *parent = 0);
    void start( int ms );

    static void go(Callback cb, int ms = 0, bool autoDelete=true, QObject *parent = 0);

private slots:

    void onTimeout();
private:
    Callback m_CB;

    
};


#endif // SINGLESHOT_H
