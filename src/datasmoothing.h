#ifndef DATASMOOTHING_H
#define DATASMOOTHING_H

#include <QVector>

template<class T>
class DataSmoothing
{    
    QVector<T> m_Data;
    int m_Interval;
    double m_RunningTotal;
public:
    DataSmoothing(int interval=60) :
        m_Interval(interval),
        m_RunningTotal(0.0) {
    }

    double add( T v ) {
        m_Data.push_back(v);
        m_RunningTotal+=v;
        while ( m_Data.length() > m_Interval )
        {
            m_RunningTotal -= m_Data.front();
            m_Data.pop_front();
        }
        return m_RunningTotal / m_Data.length();
    }


    double value () const {
        if ( m_Data.length() > 0 )
        {
            return m_RunningTotal / m_Data.length();
        }
        else
        {
            return 0;
        }
    }

    void clear() {
        m_RunningTotal = 0;
        m_Data.clear();
    }

    int count() const {
        return m_Data.count();
    }
};

#endif // DATASMOOTHING_H
