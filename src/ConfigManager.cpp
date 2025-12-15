#include "ConfigManager.h"
#include "Utils.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp> 

using json = nlohmann::json;

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
        j["CARGOS"] = {
            {"PERMITIDO", 0},
            {"TATUAPE", 0},
            {"CAMPO_BELO", 0},
            {"ADM", 0}
        };
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
        if (j.contains("BOT_TOKEN")) config_.token = j.at("BOT_TOKEN").get<std::string>();
        if (j.contains("SEU_SERVER_ID")) config_.server_id = j.at("SEU_SERVER_ID").get<uint64_t>();

        if (j.contains("CANAIS")) {
            auto& c = j["CANAIS"];
            if (c.contains("VISITAS")) config_.canal_visitas = c["VISITAS"].get<uint64_t>();
            if (c.contains("AVISO_DEMANDAS")) config_.canal_aviso_demandas = c["AVISO_DEMANDAS"].get<uint64_t>();
            if (c.contains("FINALIZADAS")) config_.canal_finalizadas = c["FINALIZADAS"].get<uint64_t>();
            if (c.contains("LOGS")) config_.canal_logs = c["LOGS"].get<uint64_t>();
            if (c.contains("AVISO_PEDIDOS")) config_.canal_aviso_pedidos = c["AVISO_PEDIDOS"].get<uint64_t>();
            if (c.contains("PEDIDOS_CONCLUIDOS")) config_.canal_pedidos_concluidos = c["PEDIDOS_CONCLUIDOS"].get<uint64_t>();
            if (c.contains("SOLICITACAO_PLACAS")) config_.canal_solicitacao_placas = c["SOLICITACAO_PLACAS"].get<uint64_t>();
            if (c.contains("PLACAS_FINALIZADAS")) config_.canal_placas_finalizadas = c["PLACAS_FINALIZADAS"].get<uint64_t>();
            if (c.contains("SUGESTOES")) config_.canal_sugestoes = c["SUGESTOES"].get<uint64_t>();
            if (c.contains("BUGS")) config_.canal_bugs = c["BUGS"].get<uint64_t>();
        }

        if (j.contains("CARGOS")) {
            auto& c = j["CARGOS"];
            if (c.contains("PERMITIDO")) config_.cargo_permitido = c["PERMITIDO"].get<uint64_t>();
            if (c.contains("TATUAPE")) config_.cargo_tatuape = c["TATUAPE"].get<uint64_t>();
            if (c.contains("CAMPO_BELO")) config_.cargo_campo_belo = c["CAMPO_BELO"].get<uint64_t>();
            if (c.contains("ADM")) config_.cargo_adm = c["ADM"].get<uint64_t>();
        }

        if (j.contains("CANAIS_RESTRITOS")) {
            auto& c = j["CANAIS_RESTRITOS"];
            if (c.contains("SO_IMAGENS") && c["SO_IMAGENS"].is_array()) {
                config_.canais_so_imagens.clear();
                for (const auto& val : c["SO_IMAGENS"]) config_.canais_so_imagens.push_back(val.get<uint64_t>());
            }
            if (c.contains("SO_LINKS") && c["SO_LINKS"].is_array()) {
                config_.canais_so_links.clear();
                for (const auto& val : c["SO_LINKS"]) config_.canais_so_links.push_back(val.get<uint64_t>());
            }
            if (c.contains("SO_DOCUMENTOS") && c["SO_DOCUMENTOS"].is_array()) {
                config_.canais_so_documentos.clear();
                for (const auto& val : c["SO_DOCUMENTOS"]) config_.canais_so_documentos.push_back(val.get<uint64_t>());
            }
        }
    }
    catch (const json::exception& e) {
        std::cerr << "ERRO FATAL: Erro ao ler " << filename << ": " << e.what() << std::endl;
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
    if (!loaded_) throw std::runtime_error("ConfigManager: Tentativa de acessar configuracao nao carregada.");
    return config_;
}

bool ConfigManager::isLoaded() const { return loaded_; }