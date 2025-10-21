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

}