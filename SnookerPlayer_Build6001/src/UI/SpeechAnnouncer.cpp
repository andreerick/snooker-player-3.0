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
    // QTextToSpeech ne liste que les voix de la langue COURANTE : on balaie
    // chaque variante de francais (France d'abord, puis Canada...) pour
    // reunir toutes les voix francaises installees.
    QList<QLocale> frenchLocales;
    for (const QLocale& locale : m_tts->availableLocales())
    {
        if (locale.language() == QLocale::French)
        {
            if (locale.territory() == QLocale::France)
            {
                frenchLocales.prepend(locale);
            }
            else
            {
                frenchLocales.append(locale);
            }
        }
    }
    for (const QLocale& locale : frenchLocales)
    {
        m_tts->setLocale(locale);
        for (const QVoice& voice : m_tts->availableVoices())
        {
            bool known = false;
            for (const QVoice& existing : m_voices)
            {
                known = known || (existing.name() == voice.name());
            }
            if (!known)
            {
                m_voices.append(voice);
            }
        }
    }
    if (!frenchLocales.isEmpty())
    {
        m_tts->setLocale(frenchLocales.first());
    }

    // Liste volontairement courte (choix de l'utilisateur apres essais) :
    // "Amelie" (preferee, voix par defaut) et "Thomas". "Marie" (type Siri) a
    // ete retiree a la demande de l'utilisateur.
    // La voix francaise "Daniel" (type Siri) est EXCLUE : macOS la declare
    // mais elle reste muette (0,2 s pour une phrase de 3 s, mesure avec
    // AVSpeechSynthesizer). Si aucune de ces voix n'existe (autre systeme,
    // ex. Windows), on garde toutes les voix francaises.
    QList<QVoice> kept;
    for (const QString& wanted : { QString::fromUtf8("Am\u00e9lie"), QString("Thomas") })
    {
        for (const QVoice& voice : m_voices)
        {
            if (voice.name() == wanted)
            {
                kept.append(voice);
                break;
            }
        }
    }
    if (!kept.isEmpty())
    {
        m_voices = kept;
    }
}

QList<QVoice> SpeechAnnouncer::availableVoices() const
{
    return m_voices;
}

QString SpeechAnnouncer::recommendedVoiceName() const
{
    // La liste est deja reduite (voir le constructeur) : la recommandee est
    // la premiere, "Amelie" quand elle est installee.
    return m_voices.isEmpty() ? QString() : m_voices.first().name();
}

QString SpeechAnnouncer::displayLabel(const QString& voiceName)
{
    return voiceName;
}

void SpeechAnnouncer::setVoiceByName(const QString& name)
{
    if (!m_tts)
    {
        return;
    }
    const QList<QVoice> voices = availableVoices();
    const QString wanted = name.isEmpty() ? recommendedVoiceName() : name;
    for (const QVoice& voice : voices)
    {
        if (voice.name() == wanted)
        {
            m_tts->setLocale(voice.locale()); // la voix n'est valable que dans sa propre langue
            m_tts->setVoice(voice);
            return;
        }
    }
    if (!name.isEmpty())
    {
        // Voix enregistree mais plus installee : retombe sur la recommandee.
        setVoiceByName(QString());
    }
}

QString SpeechAnnouncer::currentVoiceName() const
{
    return m_tts ? m_tts->voice().name() : QString();
}

void SpeechAnnouncer::preview(const QString& text)
{
    if (!m_tts)
    {
        return;
    }
    m_queue.clear();
    m_speaking = false;
    m_tts->stop();
    m_tts->say(text);
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
