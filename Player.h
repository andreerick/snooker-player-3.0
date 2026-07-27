#pragma once

#include <string>


class Player
{

public:

    Player();


    void setName(const std::string& name);

    std::string getName() const;


    void addPoints(int points);


    int getScore() const;


    void resetScore();


    void addBreak(int points);

    int getBreak() const;


private:

    std::string m_name;

    int m_score;

    int m_break;

};