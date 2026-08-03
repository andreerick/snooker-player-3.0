# =============================================================
# Makefile - Snooker Player 3.0
# Compilation et exécution des scénarios de test sous Linux/macOS
#
# Utilisation :
#   make            -> compile le programme principal ET les tests
#   make run        -> lance le programme principal
#   make test       -> compile et exécute les scénarios de test
#   make clean      -> supprime les binaires
# =============================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra

# Sources communes (logique de jeu, sans point d'entrée)
COMMON_SRCS := \
	Ball.cpp \
	BallSet.cpp \
	Player.cpp \
	Shot.cpp \
	ShotHistory.cpp \
	Referee.cpp \
	Frame.cpp \
	GameManager.cpp \
	Match.cpp \
	Tournament.cpp

# Cible principale (programme de démo original)
MAIN_BIN  := snooker_main
MAIN_SRCS := $(COMMON_SRCS) SnookerPlayer_01.cpp

# Cible tests
TEST_BIN  := test_scenarios
TEST_SRCS := $(COMMON_SRCS) TestScenarios.cpp

.PHONY: all run test clean

all: $(MAIN_BIN) $(TEST_BIN)

$(MAIN_BIN): $(MAIN_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_BIN): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

run: $(MAIN_BIN)
	./$(MAIN_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(MAIN_BIN) $(TEST_BIN)
