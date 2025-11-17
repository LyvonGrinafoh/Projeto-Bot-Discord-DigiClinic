#include "ReportGenerator.h"
#include "DatabaseManager.h"
#include "DataTypes.h"
#include "Utils.h"
#include <xlsxwriter.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdio>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <optional>

// Helper para converter enum em string
static std::string tipoSolicitacaoToString(TipoSolicitacao tipo) {
    switch (tipo) {
    case DEMANDA: return "Demanda";
    case PEDIDO: return "Pedido";
    case LEMBRETE: return "Lembrete";
    default: return "Desconhecido";
    }
}

// Helper para checar filtro de data no formato DD/MM/AAAA
static bool checkDateFilter(const std::string& date_str, std::optional<int64_t> mes, std::optional<int64_t> ano) {
    if (!mes.has_value() && !ano.has_value()) return true;

    std::tm tm = {};
    std::stringstream ss(date_str);
    ss >> std::get_time(&tm, "%d/%m/%Y");
    if (ss.fail()) return false;

    bool mes_match = !mes.has_value() || (tm.tm_mon + 1 == mes.value());
    bool ano_match = !ano.has_value() || (tm.tm_year + 1900 == ano.value());
    return mes_match && ano_match;
}

// Helper para checar filtro de data no formato Utils::format_timestamp
static bool checkTimestampFilter(const std::string& timestamp_str, std::optional<int64_t> mes, std::optional<int64_t> ano) {
    if (!mes.has_value() && !ano.has_value()) return true;

    std::tm tm = {};
    std::stringstream ss(timestamp_str);
    ss >> std::get_time(&tm, "%d/%m/%Y %H:%M:%S"); // Formato do Utils::format_timestamp
    if (ss.fail()) return false;

    bool mes_match = !mes.has_value() || (tm.tm_mon + 1 == mes.value());
    bool ano_match = !ano.has_value() || (tm.tm_year + 1900 == ano.value());
    return mes_match && ano_match;
}

static std::string getFilenameSufixo(std::optional<int64_t> mes, std::optional<int64_t> ano) {
    std::string sufixo = "";
    if (mes.has_value()) { sufixo += "_Mes-" + std::to_string(mes.value()); }
    if (ano.has_value()) { sufixo += "_Ano-" + std::to_string(ano.value()); }
    return sufixo;
}

ReportGenerator::ReportGenerator(DatabaseManager& db, dpp::cluster& bot) :
    db_(db), bot_(bot) {
}

void ReportGenerator::gerarPlanilhaLeads(const dpp::slashcommand_t& event, std::optional<int64_t> mes, std::optional<int64_t> ano) {
    std::string filename_sufixo = getFilenameSufixo(mes, ano);
    std::time_t tt = std::time(nullptr); std::tm* agora = std::localtime(&tt); std::stringstream ss; ss << std::put_time(agora, "%d-%m-%Y");
    std::string filename = "Planilha_Leads" + filename_sufixo + "_" + ss.str() + ".xlsx";
    std::remove(filename.c_str());

    lxw_workbook* workbook = workbook_new(filename.c_str()); if (!workbook) { event.edit_original_response(dpp::message("Erro ao criar o arquivo Excel para Leads.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO FATAL: workbook_new falhou para " + filename); return; }
    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, "Leads"); if (!worksheet) { event.edit_original_response(dpp::message("Erro ao criar a planilha Excel para Leads.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO FATAL: workbook_add_worksheet falhou para Leads"); workbook_close(workbook); return; }
    worksheet_set_column(worksheet, 0, 0, 15, NULL); worksheet_set_column(worksheet, 1, 1, 12, NULL); worksheet_set_column(worksheet, 2, 3, 12, NULL); worksheet_set_column(worksheet, 4, 4, 30, NULL); worksheet_set_column(worksheet, 5, 5, 25, NULL); worksheet_set_column(worksheet, 6, 6, 20, NULL); worksheet_set_column(worksheet, 7, 9, 18, NULL); worksheet_set_column(worksheet, 10, 10, 22, NULL); worksheet_set_column(worksheet, 11, 11, 18, NULL); worksheet_set_column(worksheet, 12, 12, 50, NULL); worksheet_set_column(worksheet, 13, 14, 20, NULL); worksheet_set_column(worksheet, 15, 17, 18, NULL); worksheet_set_column(worksheet, 18, 18, 15, NULL); worksheet_set_column(worksheet, 19, 19, 15, NULL); worksheet_set_column(worksheet, 20, 20, 18, NULL); worksheet_set_column(worksheet, 21, 21, 30, NULL); worksheet_set_column(worksheet, 22, 22, 20, NULL);
    lxw_format* header_format = workbook_add_format(workbook); format_set_bold(header_format); lxw_format* red_format = workbook_add_format(workbook); format_set_bg_color(red_format, LXW_COLOR_RED); format_set_font_color(red_format, LXW_COLOR_WHITE); lxw_format* green_format = workbook_add_format(workbook); format_set_bg_color(green_format, LXW_COLOR_GREEN); format_set_font_color(green_format, LXW_COLOR_WHITE); lxw_format* yellow_format = workbook_add_format(workbook); format_set_bg_color(yellow_format, LXW_COLOR_YELLOW); lxw_format* wrap_format = workbook_add_format(workbook); format_set_text_wrap(wrap_format); lxw_format* money_format = workbook_add_format(workbook); format_set_num_format(money_format, "R$ #,##0.00");
    const char* headers[] = { "ID", "Origem", "Data Contato", "Hora Contato", "Nome", "Contato", "Area", "Conhece Formato?", "Veio de Campanha?", "Respondido?", "Data e Hora da Resposta", "Problema Contato?", "Observacoes", "Status Conversa", "Status FollowUp", "Unidade", "Convidou Visita?", "Agendou Visita?", "Tipo Fechamento", "Teve Adicionais?", "Valor Fechamento", "Print Final", "Criado Por" };
    for (int i = 0; i < 23; ++i) { worksheet_write_string(worksheet, 0, i, headers[i], header_format); }

    const auto& leads_map = db_.getLeads(); std::vector<Lead> sorted_leads;
    for (const auto& pair : leads_map) {
        if (checkDateFilter(pair.second.data_contato, mes, ano)) {
            sorted_leads.push_back(pair.second);
        }
    }

    std::sort(sorted_leads.begin(), sorted_leads.end(), [](const Lead& a, const Lead& b) { try { std::tm tm_a = {}, tm_b = {}; std::stringstream ss_a(a.data_contato); std::stringstream ss_b(b.data_contato); ss_a >> std::get_time(&tm_a, "%d/%m/%Y"); ss_b >> std::get_time(&tm_b, "%d/%m/%Y"); if (ss_a.fail() || ss_b.fail()) return false; std::time_t time_a = mktime(&tm_a); std::time_t time_b = mktime(&tm_b); return time_a < time_b; } catch (...) { return false; } });

    int row = 1;
    for (const auto& lead : sorted_leads) {
        bool has_image = false;
        worksheet_write_number(worksheet, row, 0, lead.id, NULL);
        worksheet_write_string(worksheet, row, 1, lead.origem.c_str(), NULL);
        worksheet_write_string(worksheet, row, 2, lead.data_contato.c_str(), NULL);
        worksheet_write_string(worksheet, row, 3, lead.hora_contato.c_str(), NULL);
        worksheet_write_string(worksheet, row, 4, lead.nome.c_str(), NULL);
        worksheet_write_string(worksheet, row, 5, lead.contato.c_str(), NULL);
        worksheet_write_string(worksheet, row, 6, lead.area_atuacao.c_str(), NULL);
        worksheet_write_string(worksheet, row, 7, lead.conhece_formato ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 8, lead.veio_campanha ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 9, lead.respondido ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 10, lead.data_hora_resposta.c_str(), NULL);
        worksheet_write_string(worksheet, row, 11, lead.problema_contato ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 12, lead.observacoes.c_str(), wrap_format);
        lxw_format* status_format = NULL; if (lead.problema_contato) { status_format = yellow_format; }
        else if (lead.status_conversa == "Sem Resposta") { status_format = green_format; }
        else if (lead.status_conversa == "Nao Contatado") { status_format = red_format; }
        worksheet_write_string(worksheet, row, 13, lead.status_conversa.c_str(), status_format);
        worksheet_write_string(worksheet, row, 14, lead.status_followup.c_str(), NULL);
        worksheet_write_string(worksheet, row, 15, lead.unidade.c_str(), NULL);
        worksheet_write_string(worksheet, row, 16, lead.convidou_visita ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 17, lead.agendou_visita ? "Sim" : "Nao", NULL);
        worksheet_write_string(worksheet, row, 18, lead.tipo_fechamento.c_str(), NULL);
        worksheet_write_string(worksheet, row, 19, lead.teve_adicionais ? "Sim" : "Nao", NULL);
        worksheet_write_number(worksheet, row, 20, lead.valor_fechamento, money_format);

        if (!lead.print_final_conversa.empty()) {
            if (lead.print_final_conversa.rfind("lead_prints", 0) == 0 && std::filesystem::exists(lead.print_final_conversa)) {
                worksheet_set_row(worksheet, row, 150, NULL);
                lxw_image_options options = { .x_scale = 0.2, .y_scale = 0.2, .object_position = LXW_OBJECT_MOVE_DONT_SIZE };
                worksheet_insert_image_opt(worksheet, row, 21, lead.print_final_conversa.c_str(), &options);
                has_image = true;
            }
            else if (lead.print_final_conversa.rfind("http", 0) == 0) {
                worksheet_write_url(worksheet, row, 21, lead.print_final_conversa.c_str(), NULL);
            }
            else {
                worksheet_write_string(worksheet, row, 21, lead.print_final_conversa.c_str(), NULL);
            }
        }
        else {
            worksheet_write_string(worksheet, row, 21, "", NULL);
        }

        if (!has_image) {
            worksheet_set_row(worksheet, row, LXW_DEF_ROW_HEIGHT, NULL);
        }

        worksheet_write_string(worksheet, row, 22, lead.criado_por.empty() ? "N/A" : lead.criado_por.c_str(), NULL);
        row++;
    }

    if (row == 1) {
        worksheet_write_string(worksheet, 1, 0, "Nenhum lead encontrado para o filtro selecionado.", NULL);
    }

    if (workbook_close(workbook) != LXW_NO_ERROR) { event.edit_original_response(dpp::message("Erro ao finalizar e salvar o arquivo Excel para Leads.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO: workbook_close falhou para " + filename); return; }

    dpp::message msg; msg.set_content("Aqui está a planilha de leads atualizada!");
    try {
        msg.add_file(filename, dpp::utility::read_file(filename));
        dpp::slashcommand_t event_copy = event;
        std::string captured_filename = filename;
        event.edit_original_response(msg, [this, event_copy, captured_filename](const dpp::confirmation_callback_t& cb) {
            if (!cb.is_error()) {
                bot_.start_timer([this, event_copy](dpp::timer timer_handle) mutable {
                    event_copy.delete_original_response([](const dpp::confirmation_callback_t& delete_cb) {
                        if (delete_cb.is_error() && delete_cb.get_error().code != 10062) {
                            Utils::log_to_file("Falha ao deletar resposta (planilha leads): " + delete_cb.get_error().message);
                        }
                        });
                    bot_.stop_timer(timer_handle);
                    }, 10);
                bot_.start_timer([this, captured_filename](dpp::timer timer_handle) {
                    if (std::remove(captured_filename.c_str()) == 0) {
                        Utils::log_to_file("Arquivo de planilha removido com sucesso: " + captured_filename);
                    }
                    else {
                        Utils::log_to_file("AVISO: Falha ao remover arquivo de planilha: " + captured_filename);
                    }
                    bot_.stop_timer(timer_handle);
                    }, 900);
            }
            else { Utils::log_to_file("Falha ao editar resposta original com planilha leads: " + cb.get_error().message); }
            });
    }
    catch (const dpp::exception& e) { event.edit_original_response(dpp::message("Erro ao ler ou anexar o arquivo da planilha de leads: " + std::string(e.what()))); Utils::log_to_file("ERRO: Falha ao enviar planilha de leads: " + std::string(e.what())); }
}

void ReportGenerator::gerarPlanilhaCompras(const dpp::slashcommand_t& event, std::optional<int64_t> mes, std::optional<int64_t> ano) {
    std::string filename_sufixo = getFilenameSufixo(mes, ano);
    std::time_t tt = std::time(nullptr); std::tm* agora = std::localtime(&tt); std::stringstream ss; ss << std::put_time(agora, "%d-%m-%Y");
    std::string filename = "Planilha_Gastos" + filename_sufixo + "_" + ss.str() + ".xlsx";
    std::remove(filename.c_str());

    lxw_workbook* workbook = workbook_new(filename.c_str()); if (!workbook) { event.edit_original_response(dpp::message("Erro ao criar o arquivo Excel para Gastos.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO FATAL: workbook_new falhou para " + filename); return; } lxw_worksheet* worksheet = workbook_add_worksheet(workbook, "Gastos"); if (!worksheet) { event.edit_original_response(dpp::message("Erro ao criar a planilha Excel para Gastos.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO FATAL: workbook_add_worksheet falhou para Gastos"); workbook_close(workbook); return; }
    worksheet_set_column(worksheet, 0, 0, 15, NULL);
    worksheet_set_column(worksheet, 1, 1, 40, NULL);
    worksheet_set_column(worksheet, 2, 2, 20, NULL);
    worksheet_set_column(worksheet, 3, 3, 15, NULL);
    worksheet_set_column(worksheet, 4, 4, 50, NULL);
    worksheet_set_column(worksheet, 5, 5, 25, NULL);
    worksheet_set_column(worksheet, 6, 6, 25, NULL);
    lxw_format* header_format = workbook_add_format(workbook); format_set_bold(header_format);
    lxw_format* wrap_format = workbook_add_format(workbook); format_set_text_wrap(wrap_format);
    lxw_format* money_format = workbook_add_format(workbook); format_set_num_format(money_format, "R$ #,##0.00");

    const char* headers[] = { "ID", "Descricao", "Local Compra", "Valor", "Observacao", "Registrado Por", "Data do Registro" };
    for (int i = 0; i < 7; ++i) { worksheet_write_string(worksheet, 0, i, headers[i], header_format); }

    const auto& compras_map = db_.getCompras(); int row = 1;
    for (const auto& [id, compra] : compras_map) {
        if (checkTimestampFilter(compra.data_registro, mes, ano)) {
            worksheet_write_number(worksheet, row, 0, compra.id, NULL);
            worksheet_write_string(worksheet, row, 1, compra.descricao.c_str(), NULL);
            worksheet_write_string(worksheet, row, 2, compra.local_compra.c_str(), NULL);
            worksheet_write_number(worksheet, row, 3, compra.valor, money_format);
            worksheet_write_string(worksheet, row, 4, compra.observacao.c_str(), wrap_format);
            worksheet_write_string(worksheet, row, 5, compra.registrado_por.c_str(), NULL);
            worksheet_write_string(worksheet, row, 6, compra.data_registro.c_str(), NULL);
            row++;
        }
    }

    if (row == 1) {
        worksheet_write_string(worksheet, 1, 0, "Nenhum gasto encontrado para o filtro selecionado.", NULL);
    }

    if (workbook_close(workbook) != LXW_NO_ERROR) { event.edit_original_response(dpp::message("Erro ao finalizar e salvar o arquivo Excel para Gastos.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO: workbook_close falhou para " + filename); return; }

    dpp::message msg; msg.set_content("Aqui está a planilha de registros de gastos!");
    try {
        msg.add_file(filename, dpp::utility::read_file(filename));
        dpp::slashcommand_t event_copy = event;
        std::string captured_filename = filename;
        event.edit_original_response(msg, [this, event_copy, captured_filename](const dpp::confirmation_callback_t& cb) {
            if (!cb.is_error()) {
                bot_.start_timer([this, event_copy](dpp::timer th) mutable { event_copy.delete_original_response([](const auto& dcb) { if (dcb.is_error() && dcb.get_error().code != 10062) { Utils::log_to_file("Falha ao deletar resposta (planilha gastos): " + dcb.get_error().message); }}); bot_.stop_timer(th); }, 10);
                bot_.start_timer([this, captured_filename](dpp::timer timer_handle) {
                    if (std::remove(captured_filename.c_str()) == 0) {
                        Utils::log_to_file("Arquivo de planilha removido com sucesso: " + captured_filename);
                    }
                    else {
                        Utils::log_to_file("AVISO: Falha ao remover arquivo de planilha: " + captured_filename);
                    }
                    bot_.stop_timer(timer_handle);
                    }, 900);
            }
            else { Utils::log_to_file("Falha ao editar resposta original com planilha gastos: " + cb.get_error().message); }
            });
    }
    catch (const dpp::exception& e) { event.edit_original_response(dpp::message("Erro ao ler ou anexar o arquivo da planilha de gastos: " + std::string(e.what()))); Utils::log_to_file("ERRO: Falha ao enviar planilha de gastos: " + std::string(e.what())); }
}

void ReportGenerator::gerarPlanilhaVisitas(const dpp::slashcommand_t& event, std::optional<int64_t> mes, std::optional<int64_t> ano) {
    std::string filename_sufixo = getFilenameSufixo(mes, ano);
    std::time_t tt = std::time(nullptr); std::tm* agora = std::localtime(&tt); std::stringstream ss; ss << std::put_time(agora, "%d-%m-%Y");
    std::string filename = "Planilha_Visitas" + filename_sufixo + "_" + ss.str() + ".xlsx";
    std::remove(filename.c_str());

    lxw_workbook* workbook = workbook_new(filename.c_str()); if (!workbook) { event.edit_original_response(dpp::message("Erro ao criar o arquivo Excel para Visitas.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO FATAL: workbook_new falhou para " + filename); return; } lxw_worksheet* worksheet = workbook_add_worksheet(workbook, "Visitas"); if (!worksheet) { event.edit_original_response(dpp::message("Erro ao criar a planilha Excel para Visitas.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO FATAL: workbook_add_worksheet falhou para Visitas"); workbook_close(workbook); return; }
    worksheet_set_column(worksheet, 0, 0, 15, NULL); worksheet_set_column(worksheet, 1, 1, 15, NULL); worksheet_set_column(worksheet, 2, 2, 30, NULL); worksheet_set_column(worksheet, 3, 3, 25, NULL); worksheet_set_column(worksheet, 4, 5, 15, NULL); worksheet_set_column(worksheet, 6, 6, 20, NULL); worksheet_set_column(worksheet, 7, 7, 20, NULL); worksheet_set_column(worksheet, 8, 8, 25, NULL); worksheet_set_column(worksheet, 9, 9, 70, NULL);
    lxw_format* header_format = workbook_add_format(workbook); format_set_bold(header_format); lxw_format* wrap_format = workbook_add_format(workbook); format_set_text_wrap(wrap_format); lxw_format* cancelled_format = workbook_add_format(workbook); format_set_font_strikeout(cancelled_format); lxw_format* cancelled_wrap_format = workbook_add_format(workbook); format_set_font_strikeout(cancelled_wrap_format); format_set_text_wrap(cancelled_wrap_format);
    const char* headers[] = { "ID", "Status", "Dr(a).", "Área", "Data", "Horário", "Unidade", "Telefone", "Agendado Por", "Observações / Histórico" }; for (int i = 0; i < 10; ++i) { worksheet_write_string(worksheet, 0, i, headers[i], header_format); }

    const auto& visitas_map = db_.getVisitas(); std::vector<Visita> sorted_visitas;
    for (const auto& pair : visitas_map) {
        if (checkDateFilter(pair.second.data, mes, ano)) {
            sorted_visitas.push_back(pair.second);
        }
    }

    int row = 1;
    for (const auto& visita : sorted_visitas) {
        lxw_format* row_format = (visita.status == "cancelada") ? cancelled_format : NULL;
        lxw_format* obs_format = (visita.status == "cancelada") ? cancelled_wrap_format : wrap_format;
        worksheet_write_number(worksheet, row, 0, visita.id, row_format);
        worksheet_write_string(worksheet, row, 1, visita.status.c_str(), row_format);
        worksheet_write_string(worksheet, row, 2, visita.doutor.c_str(), row_format);
        worksheet_write_string(worksheet, row, 3, visita.area.c_str(), row_format);
        worksheet_write_string(worksheet, row, 4, visita.data.c_str(), row_format);
        worksheet_write_string(worksheet, row, 5, visita.horario.c_str(), row_format);
        worksheet_write_string(worksheet, row, 6, visita.unidade.c_str(), row_format);
        worksheet_write_string(worksheet, row, 7, visita.telefone.c_str(), row_format);
        worksheet_write_string(worksheet, row, 8, visita.quem_marcou_nome.c_str(), row_format);
        worksheet_write_string(worksheet, row, 9, visita.observacoes.c_str(), obs_format);
        row++;
    }

    if (row == 1) {
        worksheet_write_string(worksheet, 1, 0, "Nenhuma visita encontrada para o filtro selecionado.", NULL);
    }

    if (workbook_close(workbook) != LXW_NO_ERROR) { event.edit_original_response(dpp::message("Erro ao finalizar e salvar o arquivo Excel para Visitas.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO: workbook_close falhou para " + filename); return; }
    dpp::message msg; msg.set_content("Aqui está a planilha de visitas agendadas!");
    try {
        msg.add_file(filename, dpp::utility::read_file(filename));
        dpp::slashcommand_t event_copy = event;
        std::string captured_filename = filename;
        event.edit_original_response(msg, [this, event_copy, captured_filename](const dpp::confirmation_callback_t& cb) {
            if (!cb.is_error()) {
                bot_.start_timer([this, event_copy](dpp::timer th) mutable { event_copy.delete_original_response([](const auto& dcb) { if (dcb.is_error() && dcb.get_error().code != 10062) { Utils::log_to_file("Falha ao deletar resposta (planilha visitas): " + dcb.get_error().message); }}); bot_.stop_timer(th); }, 10);
                bot_.start_timer([this, captured_filename](dpp::timer timer_handle) {
                    if (std::remove(captured_filename.c_str()) == 0) {
                        Utils::log_to_file("Arquivo de planilha removido com sucesso: " + captured_filename);
                    }
                    else {
                        Utils::log_to_file("AVISO: Falha ao remover arquivo de planilha: " + captured_filename);
                    }
                    bot_.stop_timer(timer_handle);
                    }, 900);
            }
            else { Utils::log_to_file("Falha ao editar resposta original com planilha visitas: " + cb.get_error().message); }
            });
    }
    catch (const dpp::exception& e) { event.edit_original_response(dpp::message("Erro ao ler ou anexar o arquivo da planilha de visitas: " + std::string(e.what()))); Utils::log_to_file("ERRO: Falha ao enviar planilha de visitas: " + std::string(e.what())); }
}

void ReportGenerator::gerarPlanilhaDemandas(const dpp::slashcommand_t& event, std::optional<int64_t> mes, std::optional<int64_t> ano) {
    std::string filename_sufixo = getFilenameSufixo(mes, ano);
    if (mes.has_value() || ano.has_value()) { filename_sufixo += "_(Filtro_NA)"; } // Informa que o filtro não se aplica

    std::time_t tt = std::time(nullptr); std::tm* agora = std::localtime(&tt); std::stringstream ss; ss << std::put_time(agora, "%d-%m-%Y");
    std::string filename = "Planilha_Demandas" + filename_sufixo + "_" + ss.str() + ".xlsx";
    std::remove(filename.c_str());

    lxw_workbook* workbook = workbook_new(filename.c_str()); if (!workbook) { event.edit_original_response(dpp::message("Erro ao criar o arquivo Excel para Demandas.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO FATAL: workbook_new falhou para " + filename); return; }
    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, "Demandas"); if (!worksheet) { event.edit_original_response(dpp::message("Erro ao criar a planilha Excel para Demandas.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO FATAL: workbook_add_worksheet falhou para Demandas"); workbook_close(workbook); return; }

    worksheet_set_column(worksheet, 0, 0, 15, NULL); // ID
    worksheet_set_column(worksheet, 1, 1, 12, NULL); // Tipo
    worksheet_set_column(worksheet, 2, 2, 12, NULL); // Status
    worksheet_set_column(worksheet, 3, 3, 25, NULL); // Responsavel
    worksheet_set_column(worksheet, 4, 4, 70, NULL); // Texto
    worksheet_set_column(worksheet, 5, 5, 20, NULL); // Prazo
    worksheet_set_column(worksheet, 6, 6, 40, NULL); // Anexo

    lxw_format* header_format = workbook_add_format(workbook); format_set_bold(header_format);
    lxw_format* wrap_format = workbook_add_format(workbook); format_set_text_wrap(wrap_format);
    lxw_format* finalizada_format = workbook_add_format(workbook); format_set_bg_color(finalizada_format, LXW_COLOR_GREEN); format_set_font_color(finalizada_format, LXW_COLOR_WHITE);
    lxw_format* cancelada_format = workbook_add_format(workbook); format_set_bg_color(cancelada_format, LXW_COLOR_RED); format_set_font_color(cancelada_format, LXW_COLOR_WHITE);

    const char* headers[] = { "ID", "Tipo", "Status", "Responsavel", "Descricao", "Prazo", "Anexo" };
    for (int i = 0; i < 7; ++i) { worksheet_write_string(worksheet, 0, i, headers[i], header_format); }

    const auto& solicitacoes_map = db_.getSolicitacoes();
    int row = 1;

    // NOTA: Filtro de data não aplicado aqui pois 'Solicitacao' não tem data de criação.
    if (mes.has_value() || ano.has_value()) {
        worksheet_write_string(worksheet, 1, 0, "Filtro de data N/A para este tipo de planilha.", NULL);
        row++;
    }

    for (const auto& [id, sol] : solicitacoes_map) {
        lxw_format* status_format = NULL;
        if (sol.status == "finalizada") { status_format = finalizada_format; }
        else if (sol.status == "cancelada") { status_format = cancelada_format; }

        worksheet_write_number(worksheet, row, 0, sol.id, NULL);
        worksheet_write_string(worksheet, row, 1, tipoSolicitacaoToString(sol.tipo).c_str(), NULL);
        worksheet_write_string(worksheet, row, 2, sol.status.c_str(), status_format);
        worksheet_write_string(worksheet, row, 3, sol.nome_usuario_responsavel.c_str(), NULL);
        worksheet_write_string(worksheet, row, 4, sol.texto.c_str(), wrap_format);
        worksheet_write_string(worksheet, row, 5, sol.prazo.c_str(), NULL);
        worksheet_write_string(worksheet, row, 6, sol.anexo_path.c_str(), NULL);
        row++;
    }

    if (workbook_close(workbook) != LXW_NO_ERROR) { event.edit_original_response(dpp::message("Erro ao finalizar e salvar o arquivo Excel para Demandas.").set_flags(dpp::m_ephemeral)); Utils::log_to_file("ERRO: workbook_close falhou para " + filename); return; }

    dpp::message msg; msg.set_content("Aqui está a planilha de demandas atualizada!");
    try {
        msg.add_file(filename, dpp::utility::read_file(filename));
        dpp::slashcommand_t event_copy = event;
        std::string captured_filename = filename;
        event.edit_original_response(msg, [this, event_copy, captured_filename](const dpp::confirmation_callback_t& cb) {
            if (!cb.is_error()) {
                bot_.start_timer([this, event_copy](dpp::timer th) mutable { event_copy.delete_original_response([](const auto& dcb) { if (dcb.is_error() && dcb.get_error().code != 10062) { Utils::log_to_file("Falha ao deletar resposta (planilha demandas): " + dcb.get_error().message); }}); bot_.stop_timer(th); }, 10);
                bot_.start_timer([this, captured_filename](dpp::timer timer_handle) {
                    if (std::remove(captured_filename.c_str()) == 0) {
                        Utils::log_to_file("Arquivo de planilha removido com sucesso: " + captured_filename);
                    }
                    else {
                        Utils::log_to_file("AVISO: Falha ao remover arquivo de planilha: " + captured_filename);
                    }
                    bot_.stop_timer(timer_handle);
                    }, 900);
            }
            else { Utils::log_to_file("Falha ao editar resposta original com planilha demandas: " + cb.get_error().message); }
            });
    }
    catch (const dpp::exception& e) { event.edit_original_response(dpp::message("Erro ao ler ou anexar o arquivo da planilha de demandas: " + std::string(e.what()))); Utils::log_to_file("ERRO: Falha ao enviar planilha de demandas: " + std::string(e.what())); }
}