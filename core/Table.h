#pragma once

namespace Snooker {

// Dimensions officielles de la table de snooker (surface de jeu)
struct TableDimensions
{
    static constexpr float LENGTH_MM  = 3570.0f; // longueur (mm)
    static constexpr float WIDTH_MM   = 1770.0f; // largeur (mm)
    static constexpr float POCKET_MM  =   90.0f; // largeur d'une poche (mm)

    // Ligne de baulk : 737 mm du coussin du bas
    static constexpr float BAULK_LINE_MM = 737.0f;

    // Rayon du demi-cercle "D"
    static constexpr float D_RADIUS_MM = 292.0f;
};

// Position 2D sur la table (origine = coin bas-gauche)
// X : axe largeur (0-1770 mm)
// Y : axe longueur (0-3570 mm), 0 = cote baulk
struct Position
{
    float x; // mm sur la largeur
    float y; // mm sur la longueur
};

// Positions officielles des spots de billes
struct BallSpots
{
    // Centre de la table en largeur
    static constexpr float CENTER_X = TableDimensions::WIDTH_MM / 2.0f; // 885 mm

    static constexpr Position YELLOW = { 1209.0f, TableDimensions::BAULK_LINE_MM }; // droite baulk
    static constexpr Position GREEN  = {  561.0f, TableDimensions::BAULK_LINE_MM }; // gauche baulk
    static constexpr Position BROWN  = { CENTER_X, TableDimensions::BAULK_LINE_MM }; // centre baulk
    static constexpr Position BLUE   = { CENTER_X, 1785.0f }; // centre table
    static constexpr Position PINK   = { CENTER_X, 2844.0f }; // devant triangle rouges
    static constexpr Position BLACK  = { CENTER_X, 3246.0f }; // spot noir (324 mm du coussin haut)
};

// Positions des 6 poches
struct PocketPositions
{
    static constexpr Position TOP_LEFT     = {   0.0f, TableDimensions::LENGTH_MM };
    static constexpr Position TOP_RIGHT    = { TableDimensions::WIDTH_MM, TableDimensions::LENGTH_MM };
    static constexpr Position MID_LEFT     = {   0.0f, TableDimensions::LENGTH_MM / 2.0f };
    static constexpr Position MID_RIGHT    = { TableDimensions::WIDTH_MM, TableDimensions::LENGTH_MM / 2.0f };
    static constexpr Position BOTTOM_LEFT  = {   0.0f,   0.0f };
    static constexpr Position BOTTOM_RIGHT = { TableDimensions::WIDTH_MM,   0.0f };
};

} // namespace Snooker
