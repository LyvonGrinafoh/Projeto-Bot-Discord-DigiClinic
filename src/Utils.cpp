#include "Utils.h"
#include "DataTypes.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <random>
#include <chrono>
#include <ctime>
#include <variant>

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
        if (ss.fail() || !ss.eof()) return false;
        if (tm.tm_mday < 1 || tm.tm_mday > 31) return false;
        if (tm.tm_mon < 0 || tm.tm_mon > 11) return false;
        if (tm.tm_year < 100) return false;
        return true;
    }

    bool isDateTomorrow(const std::string& date_str) {
        if (date_str.empty() || date_str == "N/A") return false;

        std::time_t now = std::time(nullptr);
        std::tm* now_tm = std::localtime(&now);

        std::time_t tomorrow = now + (24 * 60 * 60);
        std::tm* tomorrow_tm = std::localtime(&tomorrow);

        std::stringstream ss;
        ss << std::put_time(tomorrow_tm, "%d/%m/%Y");
        std::string tomorrow_str = ss.str();

        return date_str == tomorrow_str;
    }

    // --- Função de Paginação Centralizada ---
    dpp::embed generatePageEmbed(const PaginationState& state) {
        dpp::embed embed;

        if (state.listType == "leads") {
            embed.set_title("📋 Lista de Leads").set_color(dpp::colors::blue);
        }
        else if (state.listType == "demandas") {
            embed.set_title("📝 Lista de Demandas/Pedidos").set_color(dpp::colors::orange);
        }
        else if (state.listType == "visitas") {
            embed.set_title("📅 Lista de Visitas").set_color(dpp::colors::green);
        }
        else if (state.listType == "placas") {
            embed.set_title("🪧 Lista de Placas").set_color(dpp::colors::yellow);
        }
        else if (state.listType == "estoque") {
            embed.set_title("📦 Lista de Estoque").set_color(dpp::colors::purple);
        }

        int totalItems = state.items.size();
        int totalPages = (totalItems + state.itemsPerPage - 1) / state.itemsPerPage;

        if (totalPages == 0) totalPages = 1;
        if (state.currentPage > totalPages) const_cast<PaginationState&>(state).currentPage = totalPages;

        int start = (state.currentPage - 1) * state.itemsPerPage;
        int end = std::min(start + state.itemsPerPage, totalItems);

        std::string description = "";

        for (int i = start; i < end; ++i) {
            const auto& item_variant = state.items[i];

            if (state.listType == "leads") {
                if (const Lead* l = std::get_if<Lead>(&item_variant)) {
                    description += "**" + std::to_string(l->id) + "** - " + l->nome +
                        " (" + l->status_conversa + ")\n";
                }
            }
            else if (state.listType == "demandas") {
                if (const Solicitacao* s = std::get_if<Solicitacao>(&item_variant)) {
                    std::string tipo_icon = (s->tipo == PEDIDO) ? "🛒" : (s->tipo == DEMANDA ? "❗" : "🔔");
                    std::string prio_icon = (s->prioridade == 2) ? "🔴" : ((s->prioridade == 0) ? "🟢" : "🟡");

                    std::string texto_curto = s->texto;
                    if (texto_curto.length() > 50) texto_curto = texto_curto.substr(0, 50) + "...";

                    description += prio_icon + " " + tipo_icon + " **" + std::to_string(s->id) + "** - " +
                        texto_curto + " (" + s->status + ")\n";
                }
            }
            else if (state.listType == "visitas") {
                if (const Visita* v = std::get_if<Visita>(&item_variant)) {
                    description += "**" + std::to_string(v->id) + "** - " + v->data + " " + v->horario +
                        " - Dr(a). " + v->doutor + " (" + v->status + ")\n";
                }
            }
            else if (state.listType == "placas") {
                if (const Placa* p = std::get_if<Placa>(&item_variant)) {
                    description += "**" + std::to_string(p->id) + "** - " + p->doutor +
                        " (" + p->tipo_placa + ") - " + p->status + "\n";
                }
            }
            else if (state.listType == "estoque") {
                if (const EstoqueItem* e = std::get_if<EstoqueItem>(&item_variant)) {
                    description += "**" + e->nome_item + "** (" + std::to_string(e->quantidade) + " " + e->unidade + ")\n";
                }
            }
        }

        if (description.empty()) {
            description = "Nenhum item encontrado nesta página.";
        }

        embed.set_description(description);
        embed.set_footer(dpp::embed_footer().set_text("Página " + std::to_string(state.currentPage) + " de " + std::to_string(totalPages) + " | Total: " + std::to_string(totalItems)));

        return embed;
    }

}