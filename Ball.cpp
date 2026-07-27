#include "Ball.h"

Ball::Ball(std::string name, int value)
{
    m_name = name;
    m_value = value;
}


std::string Ball::getName() const
{
    return m_name;
}


int Ball::getValue() const
{
    return m_value;
}