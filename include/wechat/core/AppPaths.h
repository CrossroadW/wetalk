#pragma once

#include <string>

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

    /// 资源文件路径: {dataDir}/resources/{resourceId}
    static std::string resourcePath(std::string const& resourceId);

    /// 缓存目录: {dataDir}/cache/
    static std::string cacheDir();

    /// 配置目录: {dataDir}/config/
    static std::string configDir();
};

} // namespace core
} // namespace wechat
