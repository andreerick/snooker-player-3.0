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

    // Liste volontairement courte : Thomas et Amelie (les plus claires des
    // voix de base de macOS, choix de l'utilisateur) + toute voix
    // "Premium"/"Amelioree"/"Enhanced"/"Siri" installee (bien plus naturelle).
    // Si aucune de celles-ci n'existe (autre systeme, ex. Windows), on garde
    // toutes les voix francaises plutot que de n'en proposer aucune.
    QList<QVoice> kept;
    for (const QVoice& voice : m_voices)
    {
        const QString name = voice.name();
        const bool natural = name.contains("Premium", Qt::CaseInsensitive)
            || name.contains("Enhanced", Qt::CaseInsensitive)
            || name.contains(QString::fromUtf8("Am\u00e9lior"), Qt::CaseInsensitive)
            || name.contains("Siri", Qt::CaseInsensitive);
        const bool chosen = name.startsWith("Thomas", Qt::CaseInsensitive)
            || name.startsWith(QString::fromUtf8("Am\u00e9lie"), Qt::CaseInsensitive);
        if (natural || chosen)
        {
            kept.append(voice);
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
    // Plus le score est petit, meilleure est la voix. Les voix "Premium" et
    // "Ameliore(e)" (a telecharger dans les reglages du systeme) sonnent
    // beaucoup moins robotiques que les voix de base ; a defaut, "Thomas"
    // (voix francaise standard la plus naturelle de macOS), puis "Amelie".
    auto score = [](const QVoice& voice) -> int
    {
        const QString name = voice.name();
        int s = 50;
        if (name.contains("Premium", Qt::CaseInsensitive)) s = 0;
        else if (name.contains("Enhanced", Qt::CaseInsensitive)
              || name.contains(QString::fromUtf8("Am\u00e9lior"), Qt::CaseInsensitive)
              || name.contains("Siri", Qt::CaseInsensitive)) s = 10;
        else if (name.startsWith("Thomas", Qt::CaseInsensitive)) s = 20;
        else if (name.startsWith(QString::fromUtf8("Am\u00e9lie"), Qt::CaseInsensitive)) s = 30;
        if (voice.locale().territory() == QLocale::France) s -= 1; // France avant Canada
        return s;
    };
    QString best;
    int bestScore = 1000;
    for (const QVoice& voice : availableVoices())
    {
        const int sc = score(voice);
        if (sc < bestScore)
        {
            bestScore = sc;
            best = voice.name();
        }
    }
    return best;
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
