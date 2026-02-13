#include <wechat/log/Log.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>

namespace wechat::log {

void init() {
    auto logDir = std::filesystem::current_path() / "logs";
    std::filesystem::create_directories(logDir);

    auto logPath = (logDir / "wetalk.log").string();

    auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        logPath, 0, 0, false, 7);

    std::vector<spdlog::sink_ptr> sinks{dailySink};

#ifndef NDEBUG
    sinks.push_back(
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
#endif

    auto logger =
        std::make_shared<spdlog::logger>("wetalk", sinks.begin(), sinks.end());

#ifdef NDEBUG
    logger->set_level(spdlog::level::info);
#else
    logger->set_level(spdlog::level::trace);
#endif

    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(3));
}

} // namespace wechat::log
