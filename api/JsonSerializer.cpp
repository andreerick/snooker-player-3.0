#include "JsonSerializer.h"
#include "../Match.h"

#include <sstream>
#include <string>
#include <stdexcept>

namespace Api {

// --- Helpers JSON minimalistes (sans dependance externe) ---

static std::string jsonStr(const std::string& s)
{
    return "\"" + s + "\"";
}

static std::string jsonKV(const std::string& key, const std::string& value)
{
    return jsonStr(key) + ": " + value;
}

static std::string jsonKVStr(const std::string& key, const std::string& value)
{
    return jsonStr(key) + ": " + jsonStr(value);
}

// ---

std::string gameStateToJson(GameManager& manager)
{
    Match& match = manager.getMatch();
    Frame& frame = match.getCurrentFrame();

    Player& p1  = frame.getPlayer1();
    Player& p2  = frame.getPlayer2();
    Player& cur = frame.currentPlayer();

    std::ostringstream oss;
    oss << "{\n";
    oss << "  " << jsonKVStr("status", "ok") << ",\n";
    oss << "  \"match\": {\n";
    oss << "    " << jsonKV("frames_player1", std::to_string(match.getFramesPlayer1())) << ",\n";
    oss << "    " << jsonKV("frames_player2", std::to_string(match.getFramesPlayer2())) << ",\n";
    oss << "    " << jsonKV("match_finished", match.isMatchFinished() ? "true" : "false") << "\n";
    oss << "  },\n";
    oss << "  \"frame\": {\n";

    // Phase
    std::string phase;
    switch (frame.getPhase())
    {
    case FramePhase::Reds:         phase = "reds";         break;
    case FramePhase::LastRedColor: phase = "last_red_color"; break;
    case FramePhase::FinalColors:  phase = "final_colors"; break;
    case FramePhase::Finished:     phase = "finished";     break;
    }

    oss << "    " << jsonKVStr("phase", phase) << ",\n";
    oss << "    " << jsonKV("reds_remaining", std::to_string(frame.redsRemaining())) << ",\n";
    oss << "    " << jsonKVStr("current_player", cur.getName()) << ",\n";
    oss << "    \"required_ball\": {\n";
    Ball req = frame.getRequiredBall();
    oss << "      " << jsonKVStr("name", req.getName()) << ",\n";
    oss << "      " << jsonKV("value", std::to_string(req.getValue())) << "\n";
    oss << "    },\n";
    oss << "    \"players\": [\n";
    oss << "      {\n";
    oss << "        " << jsonKVStr("name", p1.getName()) << ",\n";
    oss << "        " << jsonKV("score", std::to_string(p1.getScore())) << "\n";
    oss << "      },\n";
    oss << "      {\n";
    oss << "        " << jsonKVStr("name", p2.getName()) << ",\n";
    oss << "        " << jsonKV("score", std::to_string(p2.getScore())) << "\n";
    oss << "      }\n";
    oss << "    ]\n";
    oss << "  }\n";
    oss << "}";
    return oss.str();
}

std::string shotHistoryToJson(const ShotHistory& history)
{
    std::ostringstream oss;
    oss << "{\n";
    oss << "  " << jsonKV("total_shots", std::to_string(history.getShotCount())) << "\n";
    oss << "}";
    return oss.str();
}

std::string fusedStateToJson(const Camera::FusedGameState& state)
{
    std::ostringstream oss;
    oss << "{\n";
    oss << "  " << jsonKV("valid", state.valid ? "true" : "false") << ",\n";
    oss << "  " << jsonKV("timestamp_ms", std::to_string(state.timestamp_ms)) << ",\n";
    oss << "  \"balls\": [\n";

    for (size_t i = 0; i < state.balls.size(); ++i)
    {
        const auto& b = state.balls[i];
        oss << "    {\n";
        oss << "      " << jsonKVStr("color", b.color) << ",\n";
        oss << "      " << jsonKV("x_mm", std::to_string(b.x_mm)) << ",\n";
        oss << "      " << jsonKV("y_mm", std::to_string(b.y_mm)) << ",\n";
        oss << "      " << jsonKV("confidence", std::to_string(b.confidence)) << ",\n";
        oss << "      " << jsonKV("camera_id", std::to_string(b.camera_id)) << "\n";
        oss << "    }";
        if (i + 1 < state.balls.size()) oss << ",";
        oss << "\n";
    }

    oss << "  ]\n";
    oss << "}";
    return oss.str();
}

// Parseur JSON minimal pour extraire ball_name et ball_value
Ball parseShotRequest(const std::string& json)
{
    // Recherche "ball_name":"..."
    auto findValue = [&](const std::string& key) -> std::string
    {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos = json.find('"', pos);
        if (pos == std::string::npos) return "";
        size_t start = pos + 1;
        size_t end = json.find('"', start);
        if (end == std::string::npos) return "";
        return json.substr(start, end - start);
    };

    auto findInt = [&](const std::string& key) -> int
    {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return 0;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return 0;
        ++pos;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
        std::string num;
        while (pos < json.size() && (std::isdigit(json[pos]) || json[pos] == '-'))
        {
            num += json[pos++];
        }
        return num.empty() ? 0 : std::stoi(num);
    };

    std::string name  = findValue("ball_name");
    int         value = findInt("ball_value");

    if (name.empty())
    {
        throw std::invalid_argument("Champ 'ball_name' manquant dans la requete JSON");
    }

    return Ball(name, value);
}

std::string errorJson(const std::string& message)
{
    return "{ " + jsonKVStr("status", "error") + ", " +
           jsonKVStr("message", message) + " }";
}

std::string successJson(const std::string& message)
{
    return "{ " + jsonKVStr("status", "ok") + ", " +
           jsonKVStr("message", message) + " }";
}

} // namespace Api
