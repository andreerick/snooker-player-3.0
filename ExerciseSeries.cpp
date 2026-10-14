#include "ExerciseSeries.h"

namespace
{
    struct Progress
    {
        int reds = 0;
        bool lastWasRed = false;
        int finalIndex = 0;
    };

    Progress replay(const std::vector<int>& sequence)
    {
        Progress p;
        for (int value : sequence)
        {
            if (value == 1)
            {
                ++p.reds;
                p.lastWasRed = true;
            }
            else if (p.lastWasRed)
            {
                p.lastWasRed = false;
            }
            else
            {
                ++p.finalIndex;
            }
        }
        return p;
    }
}

ExerciseSeries::ExerciseSeries(int redCount, bool redsOnly)
    : m_redCount(redCount)
    , m_redsOnly(redsOnly)
{
}

ExerciseSeries::Next ExerciseSeries::next() const
{
    Progress p = replay(m_sequence);
    if (m_redsOnly)
    {
        return p.reds < m_redCount ? Next::Red : Next::Done;
    }
    if (p.lastWasRed)
    {
        return Next::AnyColor;
    }
    if (p.reds < m_redCount)
    {
        return Next::Red;
    }
    if (p.finalIndex < 6)
    {
        return Next::SpecificColor;
    }
    return Next::Done;
}

int ExerciseSeries::nextColorValue() const
{
    Progress p = replay(m_sequence);
    return 2 + p.finalIndex;
}

bool ExerciseSeries::pot(int ballValue)
{
    bool allowed = false;
    switch (next())
    {
    case Next::Red:
        allowed = (ballValue == 1);
        break;
    case Next::AnyColor:
        allowed = (ballValue >= 2 && ballValue <= 7);
        break;
    case Next::SpecificColor:
        allowed = (ballValue == nextColorValue());
        break;
    case Next::Done:
        break;
    }
    if (allowed)
    {
        m_sequence.push_back(ballValue);
    }
    return allowed;
}

bool ExerciseSeries::undo()
{
    if (m_sequence.empty())
    {
        return false;
    }
    m_sequence.pop_back();
    return true;
}

int ExerciseSeries::points() const
{
    int total = 0;
    for (int value : m_sequence)
    {
        total += value;
    }
    return total;
}

int ExerciseSeries::redsPotted() const
{
    return replay(m_sequence).reds;
}
