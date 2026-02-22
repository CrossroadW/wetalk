#include <wechat/core/AppPaths.h>

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
        auto expectedName = AppPaths::generateResourceId(oldPath.string());
        if (expectedName.empty()) {
            std::cerr << "  SKIP (cannot read): " << oldPath.filename() << "\n";
            ++skipped;
            continue;
        }

        if (oldPath.filename().string() == expectedName) {
            std::cout << "  OK     " << oldPath.filename() << "\n";
            ++skipped;
            continue;
        }

        auto newPath = oldPath.parent_path() / expectedName;
        if (fs::exists(newPath)) {
            std::cout << "  DUP    " << oldPath.filename()
                      << " -> " << expectedName << " (already exists, removing)\n";
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
