#pragma once

#include <string>
#include <string_view>

namespace wechat {
namespace core {

/// 统一的应用目录管理
///
/// 所有数据收在 {rootDir}/data/ 下：
///   {rootDir}/data/resources/   — 资源文件（图片/视频/音频/文件）
///   {rootDir}/data/cache/       — 临时缓存（缩略图等）
///   {rootDir}/data/config/      — 配置文件
///
/// 当前阶段 rootDir = 项目根目录（PROJECT_ROOT_PATH），
/// 后期有后端后改为 exe 同目录。
class AppPaths {
public:
    /// 设置数据根目录（启动时调用一次）
    static void setDataDir(std::string const& dir);

    /// 获取数据根目录
    static std::string dataDir();

    /// 资源文件路径: {rootDir}/data/resources/{resourceId}{extension}
    /// extension 含点号，如 ".jpg"，通过 toExtension(subtype) 获取
    static std::string resourcePath(std::string const& resourceId,
                                    std::string_view extension);

    /// 缓存目录: {dataDir}/cache/
    static std::string cacheDir();

    /// 配置目录: {dataDir}/config/
    static std::string configDir();

    /// 根据文件内容生成资源 ID（纯 MD5 哈希，不含扩展名）
    /// 同一文件内容始终生成相同 ID（内容寻址）
    static std::string generateResourceId(std::string const& filePath);
};

} // namespace core
} // namespace wechat
