#include <wechat/core/AppPaths.h>

#include <filesystem>

namespace wechat {
namespace core {

namespace {
std::filesystem::path dataDir_;
} // namespace

void AppPaths::setDataDir(std::string const& dir) {
    dataDir_ = std::filesystem::path(dir).lexically_normal();
}

std::string AppPaths::dataDir() {
    return dataDir_.string();
}

std::string AppPaths::resourcePath(std::string const& resourceId) {
    return (dataDir_ / "data" / "resources" / resourceId).string();
}

std::string AppPaths::cacheDir() {
    return (dataDir_ / "data" / "cache").string();
}

std::string AppPaths::configDir() {
    return (dataDir_ / "data" / "config").string();
}

} // namespace core
} // namespace wechat
