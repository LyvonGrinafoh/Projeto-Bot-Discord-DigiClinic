#include "ConfigManager.h"
#include "Utils.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

bool ConfigManager::load(const std::string& filename) {
    std::ifstream config_file(filename);
    if (!config_file.is_open()) {
        std::cerr << "AVISO: Arquivo " << filename << " nao encontrado. Criando um novo..." << std::endl;
        Utils::log_to_file("AVISO: Arquivo " + filename + " nao encontrado. Criando um novo...");
        json j;
        j["BOT_TOKEN"] = "SEU_TOKEN_AQUI";
        j["SEU_SERVER_ID"] = 0;
        j["CANAIS"] = {
            {"VISITAS", 0}, {"AVISO_DEMANDAS", 0}, {"FINALIZADAS", 0},
            {"LOGS", 0}, {"AVISO_PEDIDOS", 0}, {"PEDIDOS_CONCLUIDOS", 0},
            {"SOLICITACAO_PLACAS", 0}, {"PLACAS_FINALIZADAS", 0},
            {"SUGESTOES", 0}, {"BUGS", 0}
        };
        j["CARGOS"] = { {"PERMITIDO", 0} };
        j["CANAIS_RESTRITOS"] = {
            {"SO_IMAGENS", json::array()},
            {"SO_LINKS", json::array()},
            {"SO_DOCUMENTOS", json::array()}
        };

        std::ofstream new_config_file(filename);
        new_config_file << j.dump(4);
        new_config_file.close();

        std::cerr << "ERRO FATAL: Um novo " << filename << " foi criado. Por favor, preencha as informacoes e reinicie o bot." << std::endl;
        Utils::log_to_file("ERRO FATAL: " + filename + " criado. Bot precisa de configuracao manual para iniciar.");
        loaded_ = false;
        return false;
    }

    json j;
    try {
        config_file >> j;
        config_.token = j.at("BOT_TOKEN").get<std::string>();
        config_.server_id = j.at("SEU_SERVER_ID").get<dpp::snowflake>();
        config_.canal_visitas = j.at("CANAIS").at("VISITAS").get<dpp::snowflake>();
        config_.canal_aviso_demandas = j.at("CANAIS").at("AVISO_DEMANDAS").get<dpp::snowflake>();
        config_.canal_finalizadas = j.at("CANAIS").at("FINALIZADAS").get<dpp::snowflake>();
        config_.canal_logs = j.at("CANAIS").at("LOGS").get<dpp::snowflake>();
        config_.canal_aviso_pedidos = j.at("CANAIS").at("AVISO_PEDIDOS").get<dpp::snowflake>();
        config_.canal_pedidos_concluidos = j.at("CANAIS").at("PEDIDOS_CONCLUIDOS").get<dpp::snowflake>();
        config_.canal_solicitacao_placas = j.at("CANAIS").at("SOLICITACAO_PLACAS").get<dpp::snowflake>();
        config_.canal_placas_finalizadas = j.at("CANAIS").at("PLACAS_FINALIZADAS").get<dpp::snowflake>();
        config_.canal_sugestoes = j.at("CANAIS").at("SUGESTOES").get<dpp::snowflake>();
        config_.canal_bugs = j.at("CANAIS").at("BUGS").get<dpp::snowflake>();
        config_.cargo_permitido = j.at("CARGOS").at("PERMITIDO").get<dpp::snowflake>();
        config_.canais_so_imagens = j.at("CANAIS_RESTRITOS").at("SO_IMAGENS").get<std::vector<dpp::snowflake>>();
        config_.canais_so_links = j.at("CANAIS_RESTRITOS").at("SO_LINKS").get<std::vector<dpp::snowflake>>();
        config_.canais_so_documentos = j.at("CANAIS_RESTRITOS").at("SO_DOCUMENTOS").get<std::vector<dpp::snowflake>>();
    }
    catch (const json::exception& e) {
        std::cerr << "ERRO FATAL: Erro ao ler " << filename << ": " << e.what() << std::endl;
        std::cerr << "Verifique se o arquivo " << filename << " esta formatado corretamente." << std::endl;
        Utils::log_to_file("ERRO FATAL: Erro ao ler " + filename + ": " + std::string(e.what()));
        loaded_ = false;
        return false;
    }

    if (config_.token == "SEU_TOKEN_AQUI" || config_.token.empty()) {
        std::cerr << "ERRO FATAL: O token do bot nao foi definido no arquivo " << filename << "!" << std::endl;
        Utils::log_to_file("ERRO FATAL: Token do bot nao definido no " + filename + ".");
        loaded_ = false;
        return false;
    }

    loaded_ = true;
    return true;
}

const BotConfig& ConfigManager::getConfig() const {
    if (!loaded_) {
        throw std::runtime_error("ConfigManager: Tentativa de acessar configuracao nao carregada.");
    }
    return config_;
}

bool ConfigManager::isLoaded() const {
    return loaded_;
}