#include "MatchWebServer.h"

#include "httplib.h"

#include <QRandomGenerator>
#include <QJsonDocument>
#include <QSettings>
#include <QCoreApplication>
#include <QDateTime>

namespace
{
    // Nombre maximum de telephones simultanement "connectes" a une meme
    // table (voir MatchWebServer::registerClient()). Choix du projet :
    // au-dela, la page de controle interfererait plus qu'elle n'aiderait
    // (plusieurs personnes cliquant sur les memes boutons en meme temps).
    constexpr int kMaxClients = 2;

    // Un appareil sans nouvelle requete depuis ce delai est considere
    // deconnecte (onglet ferme, telephone sorti du Wi-Fi...) et sa place
    // est liberee. Doit rester nettement superieur a l'intervalle de
    // rafraichissement de la page (1,5s, voir kPageHtml) pour tolerer un
    // reseau Wi-Fi occasionnellement lent sans deconnecter a tort.
    constexpr qint64 kClientTimeoutMs = 15000;
    // Page servie au telephone : auto-actualisee toutes les 1,5s via
    // /api/state/<token>. Theme sombre coherent avec l'appli de bureau.
    // Le jeton est lu depuis l'URL elle-meme (dernier segment du chemin),
    // pas besoin de le repeter en dur ici.
    const char* kPageHtml = R"HTML(<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Snooker Player - Suivi du match</title>
<style>
  body { background:#000; color:#f5f5f5; font-family: -apple-system, Arial, sans-serif;
         text-align:center; padding: 20px 14px 40px; margin: 0; }
  .frames { color:#7a7f87; font-size: 13px; letter-spacing: 1px; margin-bottom: 16px; }
  .scores { display:flex; gap: 10px; margin-bottom: 10px; }
  .row { flex: 1; min-width: 0; background:#111316; border:1px solid #2a2d31; border-radius: 8px;
         padding: 12px; }
  .row.active { border-color:#1a9000; background: rgba(26,144,0,0.12); }
  .name { font-size: 15px; color:#7a7f87; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
  .row.active .name { color:#1a9000; font-weight:bold; }
  .score { font-size: 30px; font-weight:bold; margin-top: 2px; }
  .breakbox { margin: 12px 0 20px; color:#7a7f87; font-size: 13px; }
  .breakvalue { font-size: 22px; color:#f5f5f5; font-weight:bold; }
  .offline { color:#e74c3c; margin-top: 12px; font-size: 13px; }
  .pendingwrap { position: sticky; top: 0; z-index: 5; }
  .pending { display:none; background: #1c1400; border:1px solid #f5a623;
             color:#f5a623; border-radius: 6px; padding: 10px; margin-bottom: 14px;
             font-weight:bold; font-size: 13px; }
  .pending.show { display:block; }
  .freeball { display:none; background: #1c1400; border:1px solid #f5a623;
              color:#f5a623; border-radius: 6px; padding: 6px; margin-bottom: 14px;
              font-size: 12px; }
  .freeball.show { display:block; }
  h2 { font-size: 11px; color:#7a7f87; letter-spacing: 1px; text-align:left;
       margin: 18px 2px 8px; }
  .ballgrid { display:grid; grid-template-columns: repeat(2, 1fr); gap: 8px; }
  .ballbtn { border: 1px solid #fff; border-radius: 8px; padding: 13px 4px; font-weight:bold;
             font-size: 14px; }
  .ballbtn:disabled { background:#2a2d31 !important; color:#6a6d71 !important; border-color:#2a2d31; }
  .ballbtn.wide { grid-column: 1 / span 2; }
  .actiongrid { display:grid; grid-template-columns: 1fr 1fr; gap: 8px; }
  .actionbtn { background:#111316; color:#f5f5f5; border:1px solid #2a2d31;
               border-radius: 8px; padding: 12px 6px; font-size: 13px; }
  .actionbtn.wide { grid-column: 1 / span 2; }
  .actionbtn.danger { color:#e74c3c; }
  .missmenu { display:none; flex-direction: column; gap: 8px; margin-top: 8px; }
  .missmenu.show { display:flex; }
  .actionbtn:active, .ballbtn:active { opacity: 0.7; }
  .namesform input, .namesform select { width: 100%; box-sizing: border-box; background:#111316; color:#f5f5f5;
                      border:1px solid #2a2d31; border-radius: 6px; padding: 12px; font-size: 16px;
                      margin-bottom: 12px; }
  .namesform button { width: 100%; background:#1a9000; color:#fff; border:none;
                       border-radius: 6px; padding: 14px; font-size: 15px; font-weight:bold; }
  .tablefull { display:none; padding: 30px 10px; }
  .tablefull h2 { text-align:center; color:#f5a623; font-size: 16px; letter-spacing: normal; margin: 0 0 10px; }
  .tablefull p { color:#7a7f87; font-size: 13px; }
</style>
</head>
<body>
  <div class="tablefull" id="tablefull">
    <h2>Table deja utilisee par 2 telephones</h2>
    <p>Cette table de Snooker Player a deja atteint sa limite de 2 appareils connectes. Reessayez quand l'un des deux se sera deconnecte.</p>
  </div>
  <div class="namesform" id="namesform" style="display:none">
    <h2 style="text-align:center">NOUVEAU MATCH</h2>
    <input type="text" id="p1input" placeholder="Nom du joueur 1">
    <input type="text" id="p2input" placeholder="Nom du joueur 2">
    <select id="framesinput">
      <option value="1" selected>1 frame (partie rapide)</option>
      <option value="2">Meilleur des 3 frames</option>
      <option value="3">Meilleur des 5 frames</option>
      <option value="4">Meilleur des 7 frames</option>
      <option value="5">Meilleur des 9 frames</option>
    </select>
    <button onclick="submitNames()">Valider</button>
  </div>
  <div id="mainview">
  <div class="frames" id="frames">Chargement...</div>
  <div class="scores">
    <div class="row" id="row1">
      <div class="name" id="name1">-</div>
      <div class="score" id="score1">-</div>
    </div>
    <div class="row" id="row2">
      <div class="name" id="name2">-</div>
      <div class="score" id="score2">-</div>
    </div>
  </div>
  <div class="breakbox">
    BREAK EN COURS<br>
    <span class="breakvalue" id="breakvalue">0</span>
  </div>
  <div class="offline" id="offline" style="display:none">Connexion perdue - nouvelle tentative...</div>

  <div class="pendingwrap">
    <div class="pending" id="pending"></div>
    <div class="freeball" id="freeball">FREE BALL ARME</div>
    <div class="freeball" id="blackreplay">EGALITE : noire respotee -- la moindre faute perd la frame</div>
    <div class="freeball" id="touchingball">BILLE TOUCHANTE : premier contact deja valide pour le prochain coup</div>
    <div class="missmenu" id="misschoicemenu">
      <button class="actionbtn" onclick="sendAction('missReplay')">Remettre en place</button>
      <button class="actionbtn" onclick="sendAction('missChoiceContinue')">Prendre la table</button>
    </div>
  </div>

  <h2>BILLES</h2>
  <div class="ballgrid" id="ballgrid"></div>

  <h2>ACTIONS</h2>
  <div class="actiongrid">
    <button class="actionbtn" onclick="sendAction('missShot')">Fin de break</button>
    <button class="actionbtn" onclick="sendAction('armFoul')">Faute</button>
    <button class="actionbtn" onclick="sendAction('undo')">Retour</button>
    <button class="actionbtn" onclick="sendAction('finishFrame')">Game</button>
    <button class="actionbtn" onclick="sendAction('armMiss')">Miss</button>
    <button class="actionbtn" onclick="showFreeBallMenu()">Free ball</button>
    <button class="actionbtn wide" onclick="showOtherMenu()">Autre</button>
    <button class="actionbtn" onclick="showNewMatchForm()">Nouveau match</button>
    <button class="actionbtn" onclick="sendAction('goHome')">Esc</button>
  </div>
  <div class="missmenu" id="freeballmenu">
    <button class="actionbtn" onclick="chooseFreeBall('missReplay')">Remettre en place</button>
    <button class="actionbtn" onclick="chooseFreeBall('armFreeBall')">Choisir la bille de depart</button>
  </div>
  <div class="missmenu" id="othermenu">
    <button class="actionbtn" onclick="chooseOther('touchingBall')">Bille touchante</button>
    <button class="actionbtn" onclick="chooseOther('armBallOffTable')">Bille sortie de table</button>
    <button class="actionbtn" onclick="chooseOther('whiteOffTable')">Blanche sortie de table</button>
    <button class="actionbtn" onclick="confirmRestartFrame()">Recommencer la frame</button>
    <button class="actionbtn" onclick="showConcedeMenu()">Conceder la frame</button>
    <button class="actionbtn" onclick="chooseOther('armCorrection')">Correction arbitre</button>
    <button class="actionbtn" onclick="chooseOther('closeRepositionGuide')">Fermer le guide</button>
  </div>
  <div class="missmenu" id="concedemenu">
    <button class="actionbtn" id="concedebtn1" onclick="chooseConcede(1)">-</button>
    <button class="actionbtn" id="concedebtn2" onclick="chooseConcede(2)">-</button>
    <button class="actionbtn" onclick="document.getElementById('concedemenu').classList.remove('show')">Annuler</button>
  </div>
  </div>

<script>
  const token = window.location.pathname.split('/').pop();

  // Identifiant de cet appareil, genere une seule fois puis conserve
  // (localStorage) : permet au serveur de reconnaitre "le meme
  // telephone" d'un rafraichissement a l'autre plutot que de compter
  // chaque requete comme un nouvel appareil (voir MatchWebServer::
  // registerClient(), limite de 2 appareils par table).
  let clientId = localStorage.getItem('spClientId');
  if (!clientId) {
    clientId = 'c' + Date.now().toString(36) + Math.random().toString(36).slice(2, 10);
    localStorage.setItem('spClientId', clientId);
  }

  const ballColors = {
    'Rouge':['#ff3b30','#fff'], 'Jaune':['#ffcc00','#000'], 'Verte':['#00c853','#000'],
    'Marron':['#8b4513','#fff'], 'Bleue':['#1565ff','#fff'], 'Rose':['#ff4fa3','#000'],
    'Noire':['#161616','#fff']
  };
  const ballValues = {'Rouge':1,'Jaune':2,'Verte':3,'Marron':4,'Bleue':5,'Rose':6,'Noire':7};

  const grid = document.getElementById('ballgrid');
  for (const name in ballColors) {
    const btn = document.createElement('button');
    // La rouge occupe seule toute la largeur (une seule bille rouge peut
    // etre jouee a la fois), comme sur la telecommande de bureau 2.0.
    btn.id = 'ball-' + name;
    btn.className = name === 'Rouge' ? 'ballbtn wide' : 'ballbtn';
    btn.style.background = ballColors[name][0];
    btn.style.color = ballColors[name][1];
    btn.textContent = name + ' (' + ballValues[name] + ')';
    btn.onclick = () => sendAction('ball', {name});
    grid.appendChild(btn);
  }

  async function sendAction(action, params) {
    try {
      await fetch('/api/control/' + token + '?cid=' + clientId, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({ action, params: params || {} })
      });
    } catch (e) {}
    refresh();
  }

  // Menu "Free ball" : choix entre faire rejouer le fautif ou choisir sa
  // propre bille de depart, meme menu que sur les telecommandes de
  // bureau 1.0 et 2.0 (voir freeBallMenu).
  function showFreeBallMenu() {
    document.getElementById('freeballmenu').classList.add('show');
  }
  function chooseFreeBall(action) {
    document.getElementById('freeballmenu').classList.remove('show');
    sendAction(action);
  }

  // Menu "Autre" : regroupe 6 actions d'arbitrage peu frequentes, meme
  // liste et memes noms que sur les telecommandes de bureau 1.0/2.0
  // (voir otherActionsMenu/simpleOtherActionsMenu dans MainWindow.cpp).
  function showOtherMenu() {
    document.getElementById('othermenu').classList.add('show');
  }
  function chooseOther(action) {
    document.getElementById('othermenu').classList.remove('show');
    sendAction(action);
  }
  // "Recommencer la frame" : confirmation cote telephone (aucune boite
  // de dialogue PC n'est possible depuis cette page), meme texte que la
  // QMessageBox du bureau.
  function confirmRestartFrame() {
    document.getElementById('othermenu').classList.remove('show');
    if (confirm('Annuler tous les points de cette frame et remettre les billes en place (regle du Pat, meme joueur rouvre) ?')) {
      sendAction('restartFrame');
    }
  }
  // "Conceder la frame" : sous-menu a 2 boutons nommes d'apres les
  // joueurs actuels (mis a jour par refresh(), voir plus bas), au lieu
  // de la QMessageBox du bureau.
  function showConcedeMenu() {
    document.getElementById('othermenu').classList.remove('show');
    document.getElementById('concedemenu').classList.add('show');
  }
  function chooseConcede(player) {
    document.getElementById('concedemenu').classList.remove('show');
    sendAction('concedeFrame', { player });
  }

  let startingNewMatch = false;
  let namesFormShown = false;

  async function submitNames() {
    const p1 = document.getElementById('p1input').value.trim();
    const p2 = document.getElementById('p2input').value.trim();
    const frames = parseInt(document.getElementById('framesinput').value, 10);
    if (startingNewMatch) {
      // Ne depend pas de namesFormShown/refresh() (etat pilote par le
      // serveur, voir plus bas) : on gere nous-memes la transition
      // puisque l'ouverture du formulaire etait volontaire, pas imposee
      // par needsNames -- sinon le prochain rafraichissement automatique
      // (1,5s) referme le formulaire avant meme que l'utilisateur ait pu
      // saisir les noms.
      startingNewMatch = false;
      await sendAction('newMatch', { p1, p2, frames });
      document.getElementById('namesform').style.display = 'none';
      document.getElementById('mainview').style.display = 'block';
    } else {
      sendAction('submitNames', { p1, p2, frames });
    }
  }

  // Bouton "Nouveau match" (redemande les noms) : contrairement au tout
  // premier match, l'affichage du formulaire est ici declenche par
  // l'utilisateur, pas par l'etat du serveur.
  function showNewMatchForm() {
    startingNewMatch = true;
    document.getElementById('p1input').value = '';
    document.getElementById('p2input').value = '';
    document.getElementById('framesinput').value = '1';
    document.getElementById('namesform').style.display = 'block';
    document.getElementById('mainview').style.display = 'none';
  }

  async function refresh() {
    try {
      const res = await fetch('/api/state/' + token + '?cid=' + clientId, { cache: 'no-store' });
      if (!res.ok) throw new Error('bad status');
      const d = await res.json();
      document.getElementById('offline').style.display = 'none';

      if (d.full) {
        document.getElementById('tablefull').style.display = 'block';
        document.getElementById('namesform').style.display = 'none';
        document.getElementById('mainview').style.display = 'none';
        return;
      }
      document.getElementById('tablefull').style.display = 'none';

      if (d.waiting && d.needsNames) {
        document.getElementById('namesform').style.display = 'block';
        document.getElementById('mainview').style.display = 'none';
        namesFormShown = true;
        return;
      }
      if (namesFormShown) {
        // Le PC a valide les noms (localement ou via un autre appareil) :
        // revient a la vue normale.
        document.getElementById('namesform').style.display = 'none';
        document.getElementById('mainview').style.display = 'block';
        namesFormShown = false;
      }

      if (d.waiting) {
        document.getElementById('frames').textContent = 'En attente de la configuration du match...';
        document.getElementById('row1').className = 'row';
        document.getElementById('row2').className = 'row';
        document.getElementById('name1').textContent = '-';
        document.getElementById('score1').textContent = '-';
        document.getElementById('name2').textContent = '-';
        document.getElementById('score2').textContent = '-';
        document.getElementById('breakvalue').textContent = '-';
        document.getElementById('pending').className = 'pending';
        document.getElementById('freeball').className = 'freeball';
        document.getElementById('blackreplay').className = 'freeball';
        document.getElementById('touchingball').className = 'freeball';
        document.getElementById('misschoicemenu').className = 'missmenu';
        return;
      }
      document.getElementById('frames').textContent =
        'Meilleur des ' + d.totalFrames + ' frames - ' + d.framesPlayer1 + ' | ' + d.framesPlayer2;
      document.getElementById('name1').textContent = d.player1Name;
      document.getElementById('score1').textContent = d.player1Score;
      document.getElementById('name2').textContent = d.player2Name;
      document.getElementById('score2').textContent = d.player2Score;
      document.getElementById('concedebtn1').textContent = d.player1Name + ' concede';
      document.getElementById('concedebtn2').textContent = d.player2Name + ' concede';
      document.getElementById('row1').className = 'row' + (d.p1Active ? ' active' : '');
      document.getElementById('row2').className = 'row' + (!d.p1Active ? ' active' : '');
      document.getElementById('breakvalue').textContent = d.breakValue;
      if (d.ballsOnTable) {
        for (const name in d.ballsOnTable) {
          const ballBtn = document.getElementById('ball-' + name);
          if (ballBtn) {
            ballBtn.disabled = !d.ballsOnTable[name];
          }
        }
      }
      const pendingEl = document.getElementById('pending');
      if (d.pendingActionText) {
        pendingEl.textContent = d.pendingActionText;
        pendingEl.className = 'pending show';
      } else {
        pendingEl.className = 'pending';
      }
      document.getElementById('freeball').className = 'freeball' + (d.isFreeBall ? ' show' : '');
      document.getElementById('blackreplay').className = 'freeball' + (d.isBlackReplay ? ' show' : '');
      document.getElementById('touchingball').className = 'freeball' + (d.isTouchingBall ? ' show' : '');
      document.getElementById('misschoicemenu').className = 'missmenu' + (d.isMissChoicePending ? ' show' : '');
    } catch (e) {
      document.getElementById('offline').style.display = 'block';
    }
  }
  refresh();
  setInterval(refresh, 1500);
</script>
</body>
</html>
)HTML";
}

MatchWebServer::MatchWebServer(QObject* parent)
    : QObject(parent)
{
}

MatchWebServer::~MatchWebServer()
{
    stop();
}

bool MatchWebServer::start(int preferredPort)
{
    stop();

    // Jeton + port persistes dans un fichier local (table.ini, a cote de
    // l'executable) : contrairement a l'ancien comportement (nouveau
    // jeton alea toire a chaque demarrage), le lien reste IDENTIQUE d'un
    // lancement de l'appli a l'autre. Necessaire pour qu'un QR code
    // imprime une seule fois et colle sur la table reste valable
    // indefiniment (voir ShareSessionDialog) -- un jeton different a
    // chaque fois casserait un QR code papier des le prochain lancement.
    QSettings settings(QCoreApplication::applicationDirPath() + "/table.ini", QSettings::IniFormat);
    QString token = settings.value("server/token").toString();
    if (token.length() != 8)
    {
        const QString alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // sans 0/O/1/I ambigus
        token.clear();
        for (int i = 0; i < 8; ++i)
        {
            token += alphabet.at(QRandomGenerator::global()->bounded(alphabet.length()));
        }
        settings.setValue("server/token", token);
    }
    m_token = token;

    // Meme logique pour le port : on retente d'abord celui utilise la
    // derniere fois (presque toujours libre), pour que l'adresse dans le
    // QR code papier reste, elle aussi, stable en pratique.
    preferredPort = settings.value("server/port", preferredPort).toInt();

    m_server = std::make_unique<httplib::Server>();
    setupRoutes();

    // Etat par defaut tant que MainWindow n'a pas encore publie un vrai
    // etat de match (ex. pendant la boite de dialogue des noms de
    // joueurs) : evite d'afficher des scores/noms vides ou "undefined"
    // sur le telephone pendant cette courte fenetre.
    {
        QMutexLocker locker(&m_stateMutex);
        // needsNames : le telephone doit proposer un formulaire de saisie
        // des noms (voir promptWifiSharingBeforeMatch()/promptPlayerNames()
        // dans MainWindow) plutot que le simple message d'attente, tant
        // qu'aucun vrai etat de match n'a ete publie via updateState().
        m_state = QJsonObject{ {"waiting", true}, {"needsNames", true} };
    }

    for (int attempt = 0; attempt < 10; ++attempt)
    {
        int candidatePort = preferredPort + attempt;
        // bind_to_port() renvoie un bool (succes/echec), PAS le numero de
        // port -- le port reellement ouvert est celui qu'on lui a demande.
        if (m_server->bind_to_port("0.0.0.0", candidatePort))
        {
            m_port = candidatePort;
            break;
        }
    }

    if (m_port == 0)
    {
        m_server.reset();
        return false;
    }
    settings.setValue("server/port", m_port);

    m_serverThread = std::thread([this]()
        {
            m_server->listen_after_bind();
        });
    m_running = true;
    return true;
}

void MatchWebServer::stop()
{
    if (!m_running)
    {
        return;
    }
    if (m_server)
    {
        m_server->stop();
    }
    if (m_serverThread.joinable())
    {
        m_serverThread.join();
    }
    m_server.reset();
    m_running = false;
    m_port = 0;
}

void MatchWebServer::updateState(const QJsonObject& state)
{
    QMutexLocker locker(&m_stateMutex);
    m_state = state;
}

bool MatchWebServer::hasActiveClient() const
{
    QMutexLocker locker(&m_stateMutex);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = m_activeClients.begin(); it != m_activeClients.end(); )
    {
        if (now - it.value() > kClientTimeoutMs)
        {
            it = m_activeClients.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return !m_activeClients.isEmpty();
}

bool MatchWebServer::registerClient(const QString& clientId)
{
    QMutexLocker locker(&m_stateMutex);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Purge les appareils qui n'ont pas redonne signe de vie depuis
    // kClientTimeoutMs : libere leur place pour un nouvel appareil.
    for (auto it = m_activeClients.begin(); it != m_activeClients.end(); )
    {
        if (now - it.value() > kClientTimeoutMs)
        {
            it = m_activeClients.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (m_activeClients.contains(clientId))
    {
        m_activeClients[clientId] = now;
        return true;
    }
    if (m_activeClients.size() >= kMaxClients)
    {
        return false;
    }
    m_activeClients.insert(clientId, now);
    return true;
}

void MatchWebServer::setupRoutes()
{
    // Page HTML : verifie le jeton dans le chemin, sinon 404 (pas
    // d'indice donne a un appareil qui devinerait une mauvaise URL).
    m_server->Get(R"(/m/([A-Z0-9]+))", [this](const httplib::Request& req, httplib::Response& res)
        {
            if (req.matches[1] != m_token.toStdString())
            {
                res.status = 404;
                return;
            }
            res.set_content(kPageHtml, "text/html; charset=utf-8");
        });

    // Etat courant en JSON, protege par le meme jeton. Sert aussi de
    // "battement de coeur" pour la limite de kMaxClients appareils par
    // table (voir registerClient()) : c'est la requete la plus frequente
    // (toutes les 1,5s), donc le point le plus fiable pour detecter
    // qu'un appareil est toujours la.
    m_server->Get(R"(/api/state/([A-Z0-9]+))", [this](const httplib::Request& req, httplib::Response& res)
        {
            if (req.matches[1] != m_token.toStdString())
            {
                res.status = 404;
                return;
            }
            QString clientId = QString::fromStdString(req.get_param_value("cid"));
            if (!clientId.isEmpty() && !registerClient(clientId))
            {
                res.set_content(R"({"full":true})", "application/json; charset=utf-8");
                return;
            }
            QJsonObject stateCopy;
            {
                QMutexLocker locker(&m_stateMutex);
                stateCopy = m_state;
            }
            QJsonDocument doc(stateCopy);
            res.set_content(doc.toJson(QJsonDocument::Compact).toStdString(), "application/json; charset=utf-8");
        });

    // Action de controle a distance (bille cliquee, faute armee, etc.) :
    // ce handler tourne sur le thread du serveur, PAS le thread Qt/GUI --
    // il ne doit donc jamais toucher Frame/Match directement. Il se
    // contente de valider le jeton et d'emettre le signal, dont la
    // livraison est mise en file sur le thread GUI par Qt (voir
    // controlActionRequested() dans le .h).
    m_server->Post(R"(/api/control/([A-Z0-9]+))", [this](const httplib::Request& req, httplib::Response& res)
        {
            if (req.matches[1] != m_token.toStdString())
            {
                res.status = 404;
                return;
            }
            QString clientId = QString::fromStdString(req.get_param_value("cid"));
            if (!clientId.isEmpty() && !registerClient(clientId))
            {
                res.set_content(R"({"ok":false,"full":true})", "application/json; charset=utf-8");
                return;
            }
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(req.body));
            QJsonObject body = doc.object();
            QString action = body.value("action").toString();
            QJsonObject params = body.value("params").toObject();
            if (action.isEmpty())
            {
                res.status = 400;
                return;
            }
            emit controlActionRequested(action, params);
            res.set_content(R"({"ok":true})", "application/json; charset=utf-8");
        });
}
