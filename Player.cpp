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

    m_pottedPoints += points;
}



void Player::addPenalty(int points)
{
    m_score += points;

    m_foulPoints += points;
}



void Player::addFreeBallPoints(int points)
{
    m_score += points;

    m_break += points;

    m_freeBallPoints += points;
}



int Player::getScore() const
{
    return m_score;
}



void Player::resetScore()
{
    m_score = 0;

    m_break = 0;

    m_pottedPoints = 0;

    m_freeBallPoints = 0;

    m_foulPoints = 0;
}



void Player::addBreak(int points)
{
    m_break += points;
}



int Player::getBreak() const
{
    return m_break;
}



void Player::resetBreak()
{
    m_break = 0;
}



int Player::getPottedPoints() const
{
    return m_pottedPoints;
}



int Player::getFreeBallPoints() const
{
    return m_freeBallPoints;
}



int Player::getFoulPoints() const
{
    return m_foulPoints;
}