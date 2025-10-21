#pragma once

#include <string>
#include <ctime>
#include <cstdint>

namespace Utils {

    std::string format_timestamp(time_t timestamp);
    void log_to_file(const std::string& message);
    uint64_t gerar_codigo();

}