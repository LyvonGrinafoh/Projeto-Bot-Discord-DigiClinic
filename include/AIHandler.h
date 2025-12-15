#pragma once
#include <string>
#include <functional>
#include <dpp/dpp.h>
#include <nlohmann/json.hpp>

class AIHandler {
private:
    dpp::cluster& bot_;
    std::string modelName_ = "llama3";

public:
    AIHandler(dpp::cluster& bot);

    void Ask(const std::string& prompt, std::function<void(std::string)> callback);

    void AskWithContext(const std::string& system_context, const std::string& user_prompt, std::function<void(std::string)> callback, float temp = 0.3f);
};