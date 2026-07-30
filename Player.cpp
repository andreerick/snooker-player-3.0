#include "Player.h"
#include <iostream>




Player::Player()
{
    m_name = "Joueur";

    m_score = 0;

    m_break = 0;
}



void Player::setName(const std::string& name)
{
    m_name = name;
}



std::string Player::getName() const
{
    
         

    return m_name;
}



void Player::addPoints(int points)
{
    m_score += points;

    m_break += points;
}



int Player::getScore() const
{
    return m_score;
}



void Player::resetScore()
{
    m_score = 0;

    m_break = 0;
}



void Player::addBreak(int points)
{
    m_break += points;
}



int Player::getBreak() const
{
    return m_break;
}