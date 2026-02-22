#include <wechat/core/AppPaths.h>
#include <wechat/core/Message.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using wechat::core::AppPaths;

int main() {
    AppPaths::setDataDir(PROJECT_ROOT_PATH);
    auto resourceDir =
        fs::path(AppPaths::dataDir()) / "data" / "resources";

    if (!fs::exists(resourceDir)) {
        std::cerr << "Resource directory does not exist: "
                  << resourceDir << "\n";
        return 1;
    }

    int renamed = 0;
    int skipped = 0;

    for (auto const& entry : fs::directory_iterator(resourceDir)) {
        if (!entry.is_regular_file()) continue;

        auto oldPath = entry.path();

        // 生成纯 MD5 哈希
        auto hash = AppPaths::generateResourceId(oldPath.string());
        if (hash.empty()) {
            std::cerr << "  SKIP (cannot read): " << oldPath.filename() << "\n";
            ++skipped;
            continue;
        }

        // 通过 subtypeFromExtension → toExtension 规范化扩展名
        // 例如 .jpeg → .jpg
        auto subtype = wechat::core::subtypeFromExtension(
            oldPath.extension().string());
        auto canonicalExt = wechat::core::toExtension(subtype);
        auto expectedName = hash + std::string(canonicalExt);

        if (oldPath.filename().string() == expectedName) {
            std::cout << "  OK     " << oldPath.filename() << "\n";
            ++skipped;
            continue;
        }

        auto newPath = oldPath.parent_path() / expectedName;
        if (fs::exists(newPath)) {
            std::cout << "  DUP    " << oldPath.filename()
                      << " -> " << expectedName
                      << " (already exists, removing)\n";
            fs::remove(oldPath);
            ++renamed;
            continue;
        }

        std::cout << "  RENAME " << oldPath.filename()
                  << " -> " << expectedName << "\n";
        fs::rename(oldPath, newPath);
        ++renamed;
    }

    std::cout << "\nDone: " << renamed << " renamed, "
              << skipped << " unchanged.\n";
    return 0;
}
