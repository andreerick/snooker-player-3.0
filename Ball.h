#pragma once
#include <string>

class Ball
{
public:

    Ball(std::string name, int value);

    std::string getName() const;

    int getValue() const;


private:

    std::string m_name;

    int m_value;
};