#pragma once

#include <QDialog>
#include <QVoice>

class QCheckBox;
class QComboBox;

// =====================================================================
// SettingsDialog
// ---------------------------------------------------------------------
// Ecran "Parametres" (tuile de HomeScreen) : uniquement ce qui est
// reellement un reglage GLOBAL et persistant (annonces vocales, statut
// camera + calibration, info logicielle) -- le nombre de frames d'un
// match n'y figure PAS (deplace dans "Nouveau match", voir
// MainWindow::promptPlayerNames() : ca varie a chaque match plutot que
// d'etre une preference fixe) et les actions d'effacement de donnees
// (historique des matchs, liste des joueurs, tournoi en cours) non plus
// -- deplacees dans l'ecran concerne (panneau de match, PlayersDialog,
// TournamentDialog respectivement), sur demande de l'utilisateur.
//
// La preference "annonces vocales" est lue/ecrite dans settings.ini
// (cle "speech/enabled") -- necessaire car cet ecran est accessible
// AVANT qu'un match n'existe (donc avant que SpeechAnnouncer ne soit
// construit, voir MainWindow::beginMatch()) : ne peut pas se contenter
// de basculer un objet vivant comme le fait le bouton "Son" du panneau
// de match, doit persister pour etre relu au prochain beginMatch().
// =====================================================================
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    // Lit la preference persistee (settings.ini, "speech/enabled"),
    // vrai par defaut. Utilise par MainWindow::beginMatch() pour
    // initialiser SpeechAnnouncer dans le bon etat des sa creation.
    static bool loadSpeechEnabled();

    // Lit le genre de voix preferre (settings.ini, "speech/gender"),
    // feminin par defaut. Voir SpeechAnnouncer::setPreferredGender().
    static QVoice::Gender loadSpeechGender();

    // Lit le modele de telecommande de bureau choisi (settings.ini,
    // "ui/remoteVersion") : "1.0" (complete, scenarios/tests) ou "2.0"
    // (simplifiee, pour l'usage courant -- valeur par defaut). Voir
    // MainWindow::applyRemotePanelVisibility().
    static QString loadRemoteVersion();

signals:
    // Emis quand la case "annonces vocales" change PENDANT que la boite
    // de dialogue est ouverte : permet a MainWindow d'appliquer tout de
    // suite le changement si un match est deja en cours (SpeechAnnouncer
    // existe), plutot que d'attendre le prochain match pour en tenir compte.
    void speechEnabledChanged(bool enabled);

    // Meme logique que speechEnabledChanged, pour le choix de genre de voix.
    void speechGenderChanged(QVoice::Gender gender);

    // Emis quand le choix de modele de telecommande change PENDANT que la
    // boite de dialogue est ouverte, pour que MainWindow bascule tout de
    // suite si un match est deja en cours (voir applyRemotePanelVisibility()).
    void remoteVersionChanged(const QString& version);

private:
    QCheckBox* m_speechCheckBox = nullptr;
    QComboBox* m_genderCombo = nullptr;
    QComboBox* m_remoteVersionCombo = nullptr;
};
