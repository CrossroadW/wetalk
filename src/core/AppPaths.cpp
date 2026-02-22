#include <wechat/core/AppPaths.h>

#include <boost/uuid/detail/md5.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

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

std::string AppPaths::generateResourceId(std::string const& filePath) {
    namespace fs = std::filesystem;

    std::ifstream file(filePath, std::ios::binary);
    if (!file) return {};

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    // MD5 哈希
    boost::uuids::detail::md5 hash;
    hash.process_bytes(content.data(), content.size());
    boost::uuids::detail::md5::digest_type digest;
    hash.get_digest(digest);

    // 转为 32 位十六进制字符串
    auto const* bytes = reinterpret_cast<unsigned char const*>(&digest);
    std::ostringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<int>(bytes[i]);
    }

    // 拼接原扩展名: {md5}.{ext}
    auto ext = fs::path(filePath).extension().string();
    return ss.str() + ext;
}

} // namespace core
} // namespace wechat
