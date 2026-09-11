#pragma once

#include <string>


class Player
{

public:

    Player();


    void setName(const std::string& name);

    std::string getName() const;


    void addPoints(int points);

    // Ajoute des points de penalite (faute adverse) : compte pour le
    // score mais ne fait jamais partie d'un break (aucune bille empochee).
    void addPenalty(int points);

    // Ajoute des points de Free Ball : compte pour le score et le break
    // (une bille a bien ete empochee), mais suivi separement pour le
    // detail des points affiche a l'ecran.
    void addFreeBallPoints(int points);


    int getScore() const;


    void resetScore();


    void addBreak(int points);

    int getBreak() const;

    void resetBreak();

    // Detail des points marques dans la frame en cours, pour l'affichage
    // "detail des points" (billes empochees normalement / Free Ball /
    // fautes adverses). La somme des trois vaut toujours getScore().
    int getPottedPoints() const;

    int getFreeBallPoints() const;

    int getFoulPoints() const;


private:

    std::string m_name;

    int m_score;

    int m_break;

    int m_pottedPoints = 0;

    int m_freeBallPoints = 0;

    int m_foulPoints = 0;

};