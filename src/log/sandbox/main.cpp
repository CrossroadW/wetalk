#include <spdlog/spdlog.h>

int main() {
    spdlog::set_level(spdlog::level::trace);

    spdlog::trace("This is a trace message");
    spdlog::debug("This is a debug message");
    spdlog::info("This is an info message");
    spdlog::warn("This is a warning message");
    spdlog::error("This is an error message");
    spdlog::critical("This is a critical message");

    return 0;
}
