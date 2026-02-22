#include <wechat/core/AppPaths.h>

#include <filesystem>

namespace wechat {
namespace core {

namespace {
std::string dataDir_;
} // namespace

void AppPaths::setDataDir(std::string const& dir) {
    dataDir_ = dir;
    // 去除末尾分隔符
    while (!dataDir_.empty() &&
           (dataDir_.back() == '/' || dataDir_.back() == '\\')) {
        dataDir_.pop_back();
    }
}

std::string AppPaths::dataDir() {
    return dataDir_;
}

std::string AppPaths::resourcePath(std::string const& resourceId) {
    namespace fs = std::filesystem;
    return (fs::path(dataDir_) / "resources" / resourceId).string();
}

std::string AppPaths::cacheDir() {
    namespace fs = std::filesystem;
    return (fs::path(dataDir_) / "cache").string();
}

std::string AppPaths::configDir() {
    namespace fs = std::filesystem;
    return (fs::path(dataDir_) / "config").string();
}

} // namespace core
} // namespace wechat
