#include "centeredexpmovavg.h"

CenteredExpMovAvg::CenteredExpMovAvg(int windowSize, double base) :
    m_WindowSize(windowSize)
{
    if ( m_WindowSize < 0 )
    {
        m_WindowSize = 61;
    }

    if ( m_WindowSize % 2 == 0 )
    {
        m_WindowSize++;
    }

    int center = m_WindowSize / 2;
    m_Weights.resize(m_WindowSize);
    double * weights = m_Weights.data();

    weights[center] = 1;
    double normalize = weights[center];

    // we are allowed to do <= here and access element center + center since
    // we are enforcing a odd number of elements
    for (int i=1; i <= center; i++ )
    {
        double weight = pow(base,i);
        weights[ center + i ] = weight;
        weights[ center - i ] = weight;
        normalize += weight * 2;
    }

    // normalize.
    for (int i=0;i<m_WindowSize;i++)
    {
        weights[i] = weights[i] / normalize;
    }
}

void CenteredExpMovAvg::add(double data)
{
    m_Data.append(data);
}

double CenteredExpMovAvg::cea(int pos) const
{
    if ( pos >= m_Data.length() )
    {
        pos = m_Data.length() - 1;
    }
    if ( pos < 0 )
    {
        pos = 0;
    }

    int center = m_WindowSize / 2;
    int minPos = qMax(0, pos-center);
    int offset = abs(qMin(0, pos-center));
    int maxPos = qMin(m_Data.length()-1, pos+center);


    const double * values = m_Data.data();
    const double * weights = m_Weights.data();

    double v = 0;

    if ( maxPos - minPos < m_WindowSize - 1)
    {
        // if we don't use the full window, we
        // have to recalculate the normalization factor.
        double normalize = 0;
        for (int i=minPos;i<maxPos;i++)
        {
            normalize += weights[offset + i - minPos];
        }


        for (int i=minPos;i<maxPos;i++)
        {
            v += values[i] * weights[offset + i - minPos] / normalize;
        }

    }
    else
    {
        // full window used, no re-normalization needed.
        for (int i=minPos;i<maxPos;i++)
        {
            v += values[i] * weights[offset + i - minPos];
        }
    }

    return v;
}



int CenteredExpMovAvg::size() const
{
    return m_Data.size();
}
