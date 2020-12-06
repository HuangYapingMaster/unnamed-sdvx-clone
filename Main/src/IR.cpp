#include "stdafx.h"
#include "IR.hpp"
#include "GameConfig.hpp"

static void PopulateScoreJSON(nlohmann::json& json, ScoreIndex& score, BeatmapSettings& map)
{
    json["token"] = g_gameConfig.GetString(GameConfigKeys::IRToken);

    json["score"] = {
        {"score", score.score},
        {"crit", score.crit},
        {"almost", score.almost},
        {"miss", score.miss},
        {"gauge", score.gauge},
        {"gameflags", score.gameflags},
        {"timestamp", score.timestamp},
        {"chartHash", score.chartHash},
        {"windows", {
            {"perfect", score.hitWindowPerfect},
            {"good", score.hitWindowGood},
            {"hold", score.hitWindowHold},
            {"miss", score.hitWindowMiss}
        }}
    };

    json["chart"] = {
        {"title", map.title},
        {"artist", map.artist},
        {"effector", map.effector},
        {"illustrator", map.illustrator},
        {"difficulty", map.difficulty},
        {"level", map.level},
        {"bpm", map.bpm}
    };
}

namespace IR {
    cpr::AsyncResponse PostScore(ScoreIndex& score, BeatmapSettings& map)
    {
        String host = g_gameConfig.GetString(GameConfigKeys::IRBaseURL) + "/score/submit";

        nlohmann::json json;

        PopulateScoreJSON(json, score, map);

        Log(json.dump(), Logger::Severity::Warning);

        return cpr::PostAsync(cpr::Url{host},
                              cpr::Body{json.dump()},
                              cpr::Header{{"Content-Type", "application/json"}});
    }

    //note: this only confirms that the response is well-formed - responses other than 20 will not have most information, so this needs to be beared in mind too
    //e.g., this function returning true does not mean that body will exist, because status may not be 20.
    //it also does not validate each score's structure at this time - possible todo?
    bool ValidatePostScoreReturn(nlohmann::json& json)
    {
        if(json.find("statusCode") == json.end()) return false;
        if(json["statusCode"] < 20 || json["statusCode"] > 59) return false;

        if(json.find("description") == json.end()) return false;

        if(json["statusCode"] == 20)
        {
            if(json.find("body") == json.end()) return false;

            if(json["body"].find("adjacentAbove") == json["body"].end()) return false;
            if(!json["body"]["adjacentAbove"].is_array()) return false;

            if(json["body"].find("adjacentBelow") == json["body"].end()) return false;
            if(!json["body"]["adjacentAbove"].is_array()) return false;

            if(json["body"].find("adjacentBelow") == json["body"].end()) return false;
            if(!json["body"]["adjacentAbove"].is_array()) return false;

            if(json["body"].find("isPB") == json["body"].end()) return false;
            if(!json["body"]["isPB"].is_boolean()) return false;

            if(json["body"].find("isServerRecord") == json["body"].end()) return false;
            if(!json["body"]["isServerRecord"].is_boolean()) return false;

            if(json["body"].find("score") == json["body"].end()) return false;
            if(!json["body"]["score"].is_object()) return false;

            if(json["body"].find("serverRecord") == json["body"].end()) return false;
            if(!json["body"]["serverRecord"].is_object()) return false;
        }

        return true;

    }

}
