#ifndef CENTERED_EXPONENTIONAL_MOVING_AVERAGE_H
#define CENTERED_EXPONENTIONAL_MOVING_AVERAGE_H

#include <QVector>

class CenteredExpMovAvg
{
    typedef QVector<double> Data;
    Data m_Data;
    Data m_Weights;
    int m_WindowSize;

public:

    CenteredExpMovAvg(int windowSize = 61, double base = 0.95);
    void add(double data);
    double cea(int pos) const;
    int size() const;

};

#endif // CENTERED_EXPONENTIONAL_MOVING_AVERAGE_H
