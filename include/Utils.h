#pragma once

#include <string>
#include <ctime>
#include <cstdint>
#include <dpp/dpp.h>
#include <functional>
#include "DataTypes.h"

namespace Utils {
    std::string format_timestamp(time_t timestamp);
    void log_to_file(const std::string& message);
    uint64_t gerar_codigo();
    void BaixarAnexo(dpp::cluster* bot,
        const std::string& url_anexo,
        const std::string& caminho_salvar,
        std::function<void(bool sucesso)> callback_final);
    bool validarFormatoData(const std::string& data);
    bool isDateTomorrow(const std::string& date_str);
    int compararDatas(const std::string& data1, const std::string& data2);
    bool dataPassada(const std::string& data_prazo);
    dpp::embed generatePageEmbed(const PaginationState& state);
}