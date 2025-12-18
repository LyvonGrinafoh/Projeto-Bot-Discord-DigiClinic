#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <ctime>
#include <cstring>

std::time_t stringToTime(const std::string& date) {
    std::tm tm = {};
    std::stringstream ss(date);
    ss >> std::get_time(&tm, "%d/%m/%Y");
    if (ss.fail()) return -1;
    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return std::mktime(&tm);
}

std::string Utils::format_timestamp(time_t timestamp) {
    std::tm* tm_info = std::localtime(&timestamp);
    char buffer[80];
    std::strftime(buffer, 80, "%d/%m/%Y %H:%M:%S", tm_info);
    return std::string(buffer);
}

uint64_t Utils::gerar_codigo() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis(100000, 999999);
    return dis(gen);
}

bool Utils::validarFormatoData(const std::string& data) {
    if (data.length() != 10) return false;
    if (data[2] != '/' || data[5] != '/') return false;
    try {
        int dia = std::stoi(data.substr(0, 2));
        int mes = std::stoi(data.substr(3, 2));
        int ano = std::stoi(data.substr(6, 4));
        if (dia < 1 || dia > 31) return false;
        if (mes < 1 || mes > 12) return false;
        if (ano < 2020 || ano > 2100) return false;
        return true;
    }
    catch (...) { return false; }
}

bool Utils::isDateTomorrow(const std::string& dataStr) {
    time_t target = stringToTime(dataStr);
    if (target == -1) return false;
    time_t now = time(nullptr);
    tm* tm_now = localtime(&now);
    tm_now->tm_mday += 1;
    mktime(tm_now);
    tm* tm_target = localtime(&target);
    return (tm_now->tm_year == tm_target->tm_year &&
        tm_now->tm_mon == tm_target->tm_mon &&
        tm_now->tm_mday == tm_target->tm_mday);
}

int Utils::compararDatas(const std::string& data1, const std::string& data2) {
    time_t t1 = stringToTime(data1);
    time_t t2 = stringToTime(data2);
    if (t1 == -1 || t2 == -1) return 0;
    if (t1 < t2) return -1;
    if (t1 > t2) return 1;
    return 0;
}

bool Utils::dataPassada(const std::string& data) {
    time_t target = stringToTime(data);
    if (target == -1) return false;
    time_t now = time(nullptr);
    tm* tm_now = localtime(&now);
    tm_now->tm_hour = 0; tm_now->tm_min = 0; tm_now->tm_sec = 0;
    time_t midnight_now = mktime(tm_now);
    return target < midnight_now;
}

void Utils::log_to_file(const std::string& message) {
    std::ofstream log_file(LOG_FILE, std::ios_base::app);
    if (log_file.is_open()) {
        log_file << "[" << format_timestamp(std::time(nullptr)) << "] " << message << std::endl;
    }
}

dpp::embed Utils::criarEmbedPadrao(const std::string& titulo, const std::string& descricao, uint32_t cor) {
    dpp::embed embed = dpp::embed().set_title(titulo).set_description(descricao).set_color(cor);
    embed.set_footer(dpp::embed_footer().set_text("Digital Clinic Bot").set_icon("https://cdn-icons-png.flaticon.com/512/4712/4712035.png"));
    embed.set_timestamp(time(nullptr));
    return embed;
}

void Utils::BaixarAnexo(dpp::cluster* bot, const std::string& url, const std::string& path, std::function<void(bool success, std::string content)> callback) {
    bot->request(url, dpp::m_get, [path, callback](const dpp::http_request_completion_t& cc) {
        if (cc.status == 200) {
            std::ofstream file(path, std::ios::binary);
            file.write(cc.body.c_str(), cc.body.size());
            file.close();
            callback(true, cc.body);
        }
        else { callback(false, ""); }
        });
}

dpp::embed Utils::generatePageEmbed(const PaginationState& state) {
    dpp::embed embed = dpp::embed().set_color(dpp::colors::blue);
    std::string title = "Lista de Itens";
    if (state.listType == "demandas") title = "📋 Demandas Pendentes";
    else if (state.listType == "leads") title = "📇 Lista de Leads";
    else if (state.listType == "visitas") title = "📅 Visitas Agendadas";
    else if (state.listType == "placas") title = "🚪 Placas Pendentes";
    else if (state.listType == "estoque") title = "📦 Estoque";
    embed.set_title(title);
    int start = (state.currentPage - 1) * state.itemsPerPage;
    int end = std::min((int)state.items.size(), start + state.itemsPerPage);
    int totalPages = (state.items.size() + state.itemsPerPage - 1) / state.itemsPerPage;
    if (totalPages < 1) totalPages = 1;
    std::stringstream desc;
    if (state.items.empty()) { desc << "Nenhum item encontrado."; }
    else {
        for (int i = start; i < end; ++i) {
            const auto& item_variant = state.items[i];
            if (std::holds_alternative<Solicitacao>(item_variant)) {
                const auto& s = std::get<Solicitacao>(item_variant);
                desc << "**#" << s.id << "** | " << s.texto << " (Resp: " << s.nome_usuario_responsavel << ")\n";
            }
            else if (std::holds_alternative<Lead>(item_variant)) {
                const auto& l = std::get<Lead>(item_variant);
                desc << "**" << l.nome << "** | 🗓️ " << l.data_contato << " (" << l.origem << ") - Status: " << l.status_conversa << "\n";
            }
            else if (std::holds_alternative<Visita>(item_variant)) {
                const auto& v = std::get<Visita>(item_variant);
                desc << "**" << v.data << " " << v.horario << "** | Dr(a) " << v.doutor << " (" << v.unidade << ")\n";
            }
            else if (std::holds_alternative<Placa>(item_variant)) {
                const auto& p = std::get<Placa>(item_variant);
                desc << "**#" << p.id << "** Dr(a) " << p.doutor << " (" << p.tipo_placa << ")\n";
            }
            else if (std::holds_alternative<EstoqueItem>(item_variant)) {
                const auto& e = std::get<EstoqueItem>(item_variant);
                desc << "**" << e.nome_item << "** | Qtd: " << e.quantidade << " " << e.unidade << " (" << e.local_estoque << ")\n";
            }
            else if (std::holds_alternative<RelatorioDiario>(item_variant)) {
                const auto& r = std::get<RelatorioDiario>(item_variant);
                std::string resumo = r.conteudo.length() > 50 ? r.conteudo.substr(0, 50) + "..." : r.conteudo;
                desc << "**" << r.username << "** (" << r.data_hora << "): " << resumo << "\n";
            }
        }
    }
    embed.set_description(desc.str());
    embed.set_footer(dpp::embed_footer().set_text("Página " + std::to_string(state.currentPage) + " de " + std::to_string(totalPages)));
    return embed;
}