#include <iostream>
#include <exception>
#include <memory>
#include "BotCore.h"
#include "Utils.h"

int main() {
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