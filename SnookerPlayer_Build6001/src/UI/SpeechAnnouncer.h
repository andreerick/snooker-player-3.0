#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextToSpeech>
#include <QVoice>
#include <memory>

// QTextToSpeech est inclus ici (pas juste avant-declare) : le slot
// onStateChanged() ci-dessous doit connaitre QTextToSpeech::State pour
// que MOC genere la connexion au signal stateChanged().

// =====================================================================
// SpeechAnnouncer
// ---------------------------------------------------------------------
// Annonce a voix haute les evenements de la partie (points marques,
// fautes, changement de joueur) via Qt6::TextToSpeech, deliberement
// choisi plutot qu'une API specifique a un OS (ex. SAPI Windows) : ce
// module Qt est deja multi-plateforme (SAPI sous Windows, synthese
// native sous macOS, speech-dispatcher sous Linux), ce qui compte pour
// ce projet (version PC finalisee en premier, mais portage Mac prevu).
// =====================================================================
class SpeechAnnouncer : public QObject
{
    Q_OBJECT

public:
    explicit SpeechAnnouncer(QObject* parent = nullptr);

    // Coupe/reactive les annonces sans détruire le moteur vocal (evite
    // de le recreer a chaque bascule). Desactive par defaut au demarrage
    // serait surprenant pour l'utilisateur qui vient d'ajouter la
    // fonctionnalite ; actif par defaut, avec un bouton pour couper.
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // Voix francaises installees sur la machine (voir availableVoices() de
    // QTextToSpeech, restreint a la langue choisie dans le constructeur).
    QList<QVoice> availableVoices() const;

    // Nom de la voix recommandee par defaut : la premiere de la liste
    // ("Amelie"). Vide si aucune voix n'est disponible.
    QString recommendedVoiceName() const;

    // Libelle affiche pour une voix dans les Parametres (son nom).
    static QString displayLabel(const QString& voiceName);

    // Choisit la voix par son nom (QVoice::name()). Nom vide ou inconnu :
    // voix recommandee (recommendedVoiceName()).
    void setVoiceByName(const QString& name);

    // Nom de la voix actuellement utilisee.
    QString currentVoiceName() const;

    // Prononce `text` tout de suite (interrompt une annonce en cours),
    // meme si les annonces sont desactivees : sert a "Ecouter" dans les
    // Parametres. N'utilise pas la file d'attente de announce().
    void preview(const QString& text);

    // Ajoute `text` a la file d'attente vocale (voir m_queue) : ne parle
    // JAMAIS par-dessus une annonce en cours, contrairement a
    // QTextToSpeech::say() seul (qui interrompt/remplace l'utterance en
    // cours). QTextToSpeech::enqueue() existe pour ca mais s'est avere
    // peu fiable avec le plugin SAPI (annonces perdues en silence) : la
    // file est donc geree ici a la main, avec say() + le signal
    // stateChanged() pour enchainer nous-memes. Ne fait rien si l'audio
    // est desactive ou si aucun moteur vocal n'a pu etre initialise.
    void announce(const QString& text);

private slots:
    void onStateChanged(QTextToSpeech::State state);

private:
    void speakNext();

    std::unique_ptr<QTextToSpeech> m_tts;
    // Toutes les voix francaises (France, Canada, ...) : QTextToSpeech ne
    // liste que les voix de la langue COURANTE, il faut donc balayer chaque
    // variante de francais disponible au demarrage.
    QList<QVoice> m_voices;
    bool m_enabled = true;
    QStringList m_queue;
    bool m_speaking = false;
};
