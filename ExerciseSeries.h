#pragma once

#include <vector>

// =====================================================================
// ExerciseSeries
// ---------------------------------------------------------------------
// Logique de comptage d'une serie d'entrainement a 15 rouges (Line-Up,
// The T) : alternance rouge / couleur tant qu'il reste des rouges, puis
// les six couleurs dans l'ordre (jaune -> noire). Maximum 147.
// Independant de Qt : testable en ligne de commande (voir
// tests/EngineTests.cpp).
// Valeurs : 1 = rouge, 2 = jaune, 3 = verte, 4 = marron, 5 = bleue,
// 6 = rose, 7 = noire.
// =====================================================================
class ExerciseSeries
{
public:
    enum class Next
    {
        Red,            // une rouge est attendue
        AnyColor,       // n'importe quelle couleur (apres une rouge)
        SpecificColor,  // couleur imposee par l'ordre final (nextColorValue())
        Done            // toutes les billes sont empochees
    };

    // redsOnly : variante "rouges seulement" (aucune couleur), score max = redCount.
    explicit ExerciseSeries(int redCount = 15, bool redsOnly = false);

    Next next() const;
    int nextColorValue() const;

    // Enregistre une bille empochee ; false si elle n'est pas permise a
    // ce moment (la serie n'est alors pas modifiee).
    bool pot(int ballValue);
    bool undo();

    int points() const;
    int redsPotted() const;
    int redCount() const { return m_redCount; }
    bool redsOnly() const { return m_redsOnly; }
    bool isDone() const { return next() == Next::Done; }
    const std::vector<int>& sequence() const { return m_sequence; }

private:
    int m_redCount;
    bool m_redsOnly;
    std::vector<int> m_sequence;
};
