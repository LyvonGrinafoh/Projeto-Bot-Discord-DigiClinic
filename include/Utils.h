#pragma once
#include <string>
#include <dpp/dpp.h>
#include <functional>
#include "DataTypes.h"


inline const std::string DATABASE_FILE = "database.json";
inline const std::string LEADS_DATABASE_FILE = "leads_database.json";
inline const std::string COMPRAS_DATABASE_FILE = "compras.json";
inline const std::string VISITAS_DATABASE_FILE = "visitas_database.json";
inline const std::string TICKETS_DATABASE_FILE = "tickets.json";
inline const std::string PLACAS_DATABASE_FILE = "placas.json";
inline const std::string ESTOQUE_DATABASE_FILE = "estoque.json";
inline const std::string RELATORIOS_DATABASE_FILE = "relatorios.json";
inline const std::string LOG_FILE = "bot_log.txt";

class Utils {
public:
    static std::string format_timestamp(time_t timestamp);
    static uint64_t gerar_codigo();
    static bool validarFormatoData(const std::string& data);

    static bool isDateTomorrow(const std::string& dataStr);
    static int compararDatas(const std::string& data1, const std::string& data2);
    static bool dataPassada(const std::string& data);

    static void log_to_file(const std::string& message);
    static dpp::embed criarEmbedPadrao(const std::string& titulo, const std::string& descricao, uint32_t cor);
    static void BaixarAnexo(dpp::cluster* bot, const std::string& url, const std::string& path, std::function<void(bool success, std::string content)> callback);
    static dpp::embed generatePageEmbed(const PaginationState& state);
};