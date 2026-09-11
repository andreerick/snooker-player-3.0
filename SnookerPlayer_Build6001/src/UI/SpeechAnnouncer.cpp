#include "SpeechAnnouncer.h"

#include <QLocale>
#include <QVoice>

SpeechAnnouncer::SpeechAnnouncer(QObject* parent)
    : QObject(parent)
{
    m_tts = std::make_unique<QTextToSpeech>(this);
    connect(m_tts.get(), &QTextToSpeech::stateChanged, this, &SpeechAnnouncer::onStateChanged);

    // Choisit une voix francaise si la machine en propose une (le moteur
    // par defaut demarre souvent dans la langue systeme de l'utilisateur,
    // pas forcement le francais). Si aucune voix francaise n'est
    // disponible, on garde la voix par defaut plutot que de ne rien dire
    // du tout.
    const QList<QLocale> locales = m_tts->availableLocales();
    for (const QLocale& locale : locales)
    {
        if (locale.language() == QLocale::French)
        {
            m_tts->setLocale(locale);
            break;
        }
    }
}

void SpeechAnnouncer::setPreferredGender(QVoice::Gender gender)
{
    if (!m_tts)
    {
        return;
    }
    const QList<QVoice> voices = m_tts->availableVoices();
    for (const QVoice& voice : voices)
    {
        if (voice.gender() == gender)
        {
            m_tts->setVoice(voice);
            return;
        }
    }
}

void SpeechAnnouncer::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled && m_tts)
    {
        m_tts->stop();
        m_queue.clear();
        m_speaking = false;
    }
}

void SpeechAnnouncer::announce(const QString& text)
{
    if (!m_enabled || !m_tts)
    {
        return;
    }
    m_queue.append(text);
    if (!m_speaking)
    {
        speakNext();
    }
}

void SpeechAnnouncer::speakNext()
{
    if (m_queue.isEmpty())
    {
        m_speaking = false;
        return;
    }
    m_speaking = true;
    QString text = m_queue.takeFirst();
    m_tts->say(text);
}

void SpeechAnnouncer::onStateChanged(QTextToSpeech::State state)
{
    // Des qu'une annonce se termine (Ready) ou echoue (Error), on
    // enchaine sur la suivante dans la file plutot que de rester
    // bloque : c'est ce mecanisme (et non QTextToSpeech::enqueue(), voir
    // announce() ci-dessus) qui fait office de file d'attente ici.
    if (state == QTextToSpeech::Ready || state == QTextToSpeech::Error)
    {
        speakNext();
    }
}
