#include "Utils.h"
#include "DataTypes.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <random>
#include <chrono>

namespace Utils {

    std::string format_timestamp(time_t timestamp) {
        std::stringstream ss;
        ss << std::put_time(std::localtime(&timestamp), "%d/%m/%Y %H:%M:%S");
        return ss.str();
    }

    void log_to_file(const std::string& message) {
        std::ofstream log_file(LOG_FILE, std::ios_base::app);
        if (log_file.is_open()) {
            log_file << "[" << format_timestamp(std::time(nullptr)) << "] " << message << std::endl;
        }
    }

    uint64_t gerar_codigo() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> distrib(1000000000, 9999999999);
        return distrib(gen);
    }

    void BaixarAnexo(dpp::cluster* bot,
        const std::string& url_anexo,
        const std::string& caminho_salvar,
        std::function<void(bool sucesso)> callback_final)
    {
        bot->request(url_anexo, dpp::m_get, [caminho_salvar, callback_final](const dpp::http_request_completion_t& http) {
            if (http.status == 200) {
                try {
                    std::ofstream arquivo_saida(caminho_salvar, std::ios::binary);
                    if (!arquivo_saida.is_open()) {
                        callback_final(false);
                        return;
                    }
                    arquivo_saida.write(http.body.c_str(), http.body.length());
                    arquivo_saida.close();
                    callback_final(true);
                }
                catch (const std::exception& e) {
                    callback_final(false);
                }
            }
            else {
                callback_final(false);
            }
            });
    }

    bool validarFormatoData(const std::string& data) {
        std::tm tm = {};
        std::stringstream ss(data);
        ss >> std::get_time(&tm, "%d/%m/%Y");

        if (ss.fail() || !ss.eof()) {
            return false;
        }

        if (tm.tm_mday < 1 || tm.tm_mday > 31) return false;
        if (tm.tm_mon < 0 || tm.tm_mon > 11) return false;
        if (tm.tm_year < 100) return false; // Requer ano com 4 dígitos (ex: 2024)

        return true;
    }

}