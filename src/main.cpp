#include <iostream>
#include <exception>
#include <memory>
#include <filesystem>
#include "BotCore.h"
#include "Utils.h"


namespace fs = std::filesystem;

int main() {

    std::cout << "\n=== INICIANDO DIAGNOSTICO DE DIRETORIO ===" << std::endl;

    fs::path diretorioAtual = fs::current_path();
    std::cout << "Diretorio de Execucao (CWD): " << diretorioAtual << std::endl;

    std::string arquivoTeste = "estoque.json";

    if (fs::exists(arquivoTeste)) {
        std::cout << "[SUCESSO] O arquivo '" << arquivoTeste << "' foi ENCONTRADO!" << std::endl;
        std::cout << "Tamanho do arquivo: " << fs::file_size(arquivoTeste) << " bytes." << std::endl;
    }
    else {
        std::cout << "[ERRO CRITICO] O arquivo '" << arquivoTeste << "' NAO FOI ENCONTRADO neste diretorio." << std::endl;
        std::cout << "O bot vai procurar em: " << diretorioAtual / arquivoTeste << std::endl;
        std::cout << "DICA: Verifique se o arquivo nao esta dentro de uma subpasta 'database' ou 'Debug'." << std::endl;
    }
    std::cout << "==========================================\n" << std::endl;
    // -------------------------------------

    try {
        auto botInstance = std::make_unique<BotCore>();

        botInstance->run();

        Utils::log_to_file("--- Bot encerrado normally ---");
        return 0;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "ERRO FATAL NA INICIALIZACAO: " << e.what() << std::endl;
        Utils::log_to_file("ERRO FATAL NA INICIALIZACAO: " + std::string(e.what()));
        std::cout << "Pressione Enter para sair." << std::endl;
        std::cin.get();
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "ERRO INESPERADO NA INICIALIZACAO: " << e.what() << std::endl;
        Utils::log_to_file("ERRO INESPERADO NA INICIALIZACAO: " + std::string(e.what()));
        std::cout << "Pressione Enter para sair." << std::endl;
        std::cin.get();
        return 1;
    }
    catch (...) {
        std::cerr << "ERRO DESCONHECIDO E GRAVE NA INICIALIZACAO!" << std::endl;
        Utils::log_to_file("ERRO DESCONHECIDO E GRAVE NA INICIALIZACAO!");
        std::cout << "Pressione Enter para sair." << std::endl;
        std::cin.get();
        return 1;
    }
}