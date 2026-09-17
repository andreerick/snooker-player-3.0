#pragma once

class QWidget;

// Force la barre de titre native (Windows) d'une fenetre/dialogue en mode
// sombre : sans ca, elle reste blanche par defaut (dessinee par l'OS, pas
// par le style Qt applique au contenu), ce qui detonne avec le theme
// sombre de l'appli. Sans effet sur les autres plateformes. A appeler une
// fois le widget cree, avant .show()/.exec().
void applyDarkTitleBar(QWidget* window);
