#include "AIHandler.h"
#include "Utils.h"
#include <iostream>
#include <thread>
#include "httplib.h" 

using json = nlohmann::json;

AIHandler::AIHandler(dpp::cluster& bot) : bot_(bot) {}

void AIHandler::Ask(const std::string& prompt, std::function<void(std::string)> callback) {
    AskWithContext("", prompt, callback);
}

void AIHandler::AskWithContext(const std::string& system_context, const std::string& user_prompt, std::function<void(std::string)> callback, float temp) {
    json body;
    body["model"] = modelName_;
    body["stream"] = false;
    body["options"] = { {"temperature", temp} };

    if (!system_context.empty()) {
        body["prompt"] = "System: " + system_context + "\nUser: " + user_prompt;
    }
    else {
        body["prompt"] = user_prompt;
    }

    std::cout << "[IA] Iniciando thread para: " << modelName_ << "..." << std::endl;

    std::thread([this, body, callback]() {

        httplib::Client cli("127.0.0.1", 11434);

        cli.set_read_timeout(180, 0);
        cli.set_connection_timeout(5, 0);

        auto res = cli.Post("/api/generate", body.dump(), "application/json");

        if (res && res->status == 200) {
            try {
                json response = json::parse(res->body);
                if (response.contains("response")) {
                    std::string text = response["response"];
                    callback(text);
                }
                else {
                    callback("Erro: A IA respondeu, mas o campo 'response' estava vazio.");
                }
            }
            catch (const std::exception& e) {
                callback("Erro ao ler JSON da IA: " + std::string(e.what()));
            }
        }
        else {
            std::string erro = res ? std::to_string(res->status) : "Timeout ou Conexão Recusada";
            std::cout << "[ERRO OLLAMA] Status: " << erro << std::endl;
            callback("❌ Ocorreu um erro ao contatar o cérebro da IA. (Status: " + erro + ")");
        }

        }).detach();
}