#include <gtest/gtest.h>
#include <wechat/core/AppPaths.h>
#include <wechat/core/Message.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using wechat::core::AppPaths;

// ── 测试夹具：使用系统临时目录 ──

class AppPathsTest : public ::testing::Test {
protected:
    fs::path tempDir;

    void SetUp() override {
        tempDir = fs::temp_directory_path() / "wetalk_test_app_paths";
        fs::create_directories(tempDir);
        AppPaths::setDataDir(tempDir.string());
    }

    void TearDown() override {
        fs::remove_all(tempDir);
    }

    // 辅助：在临时目录下创建文件，写入指定内容
    fs::path createTempFile(std::string const& relativePath,
                            std::string const& content) {
        auto filePath = tempDir / relativePath;
        fs::create_directories(filePath.parent_path());
        std::ofstream out(filePath, std::ios::binary);
        out << content;
        out.close();
        return filePath;
    }
};

// ── setDataDir / dataDir ──

TEST_F(AppPathsTest, DataDirRoundTrip) {
    EXPECT_TRUE(fs::equivalent(fs::path(AppPaths::dataDir()), tempDir));
}

TEST_F(AppPathsTest, DataDirNormalizesTrailingSeparator) {
    auto dirWithSlash = tempDir.string() + "/";
    AppPaths::setDataDir(dirWithSlash);
    // 带尾部分隔符和不带的应该指向同一目录
    EXPECT_TRUE(fs::equivalent(fs::path(AppPaths::dataDir()), tempDir));
}

TEST_F(AppPathsTest, DataDirNormalizesDotSegments) {
    auto subdir = tempDir / "subdir";
    fs::create_directories(subdir);
    auto dirWithDots = (subdir / "..").string();
    AppPaths::setDataDir(dirWithDots);
    EXPECT_TRUE(fs::equivalent(fs::path(AppPaths::dataDir()), tempDir));
}

// ── resourcePath ──

TEST_F(AppPathsTest, ResourcePathConstruction) {
    auto expected =
        (tempDir / "data" / "resources" / "abc123.jpg").lexically_normal();
    auto actual = fs::path(AppPaths::resourcePath("abc123", ".jpg"));
    EXPECT_EQ(actual, expected);
}

TEST_F(AppPathsTest, ResourcePathNoExtension) {
    auto expected =
        (tempDir / "data" / "resources" / "abc123").lexically_normal();
    auto actual = fs::path(AppPaths::resourcePath("abc123", ""));
    EXPECT_EQ(actual, expected);
}

TEST_F(AppPathsTest, ResourcePathWithToExtension) {
    using wechat::core::ResourceSubtype;
    using wechat::core::toExtension;

    auto ext = toExtension(ResourceSubtype::Png);
    auto actual = fs::path(AppPaths::resourcePath("hash123", ext));
    auto expected =
        (tempDir / "data" / "resources" / "hash123.png").lexically_normal();
    EXPECT_EQ(actual, expected);
}

// ── cacheDir / configDir ──

TEST_F(AppPathsTest, CacheDirConstruction) {
    auto expected = (tempDir / "data" / "cache").lexically_normal();
    auto actual = fs::path(AppPaths::cacheDir());
    EXPECT_EQ(actual, expected);
}

TEST_F(AppPathsTest, ConfigDirConstruction) {
    auto expected = (tempDir / "data" / "config").lexically_normal();
    auto actual = fs::path(AppPaths::configDir());
    EXPECT_EQ(actual, expected);
}

// ── generateResourceId ──

TEST_F(AppPathsTest, GenerateResourceIdReturns32HexChars) {
    auto filePath = createTempFile("test_file.txt", "hello world");
    auto id = AppPaths::generateResourceId(filePath.string());
    EXPECT_EQ(id.size(), 32u);
    // 仅包含十六进制字符
    for (char c : id) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex character: " << c;
    }
}

TEST_F(AppPathsTest, GenerateResourceIdDeterministic) {
    auto file1 = createTempFile("file_a.txt", "same content");
    auto file2 = createTempFile("file_b.txt", "same content");
    auto id1 = AppPaths::generateResourceId(file1.string());
    auto id2 = AppPaths::generateResourceId(file2.string());
    // 相同内容 → 相同 ID（内容寻址）
    EXPECT_EQ(id1, id2);
}

TEST_F(AppPathsTest, GenerateResourceIdDifferentForDifferentContent) {
    auto file1 = createTempFile("file_x.txt", "content A");
    auto file2 = createTempFile("file_y.txt", "content B");
    auto id1 = AppPaths::generateResourceId(file1.string());
    auto id2 = AppPaths::generateResourceId(file2.string());
    EXPECT_NE(id1, id2);
}

TEST_F(AppPathsTest, GenerateResourceIdEmptyForNonexistentFile) {
    auto id = AppPaths::generateResourceId("/nonexistent/path/file.bin");
    EXPECT_TRUE(id.empty());
}

TEST_F(AppPathsTest, GenerateResourceIdHandlesEmptyFile) {
    auto filePath = createTempFile("empty.txt", "");
    auto id = AppPaths::generateResourceId(filePath.string());
    // 空文件也应该有有效的 MD5
    EXPECT_EQ(id.size(), 32u);
}

TEST_F(AppPathsTest, GenerateResourceIdHandlesBinaryContent) {
    std::string binaryContent;
    for (int i = 0; i < 256; ++i) {
        binaryContent += static_cast<char>(i);
    }
    auto filePath = createTempFile("binary.bin", binaryContent);
    auto id = AppPaths::generateResourceId(filePath.string());
    EXPECT_EQ(id.size(), 32u);
}

// ── Message.h constexpr 映射函数 ──

TEST(ResourceSubtypeMapping, ToExtensionRoundTrip) {
    using wechat::core::ResourceSubtype;
    using wechat::core::subtypeFromExtension;
    using wechat::core::toExtension;

    // 所有已知类型应该能往返转换
    ResourceSubtype types[] = {
        ResourceSubtype::Png,  ResourceSubtype::Jpeg, ResourceSubtype::Gif,
        ResourceSubtype::Webp, ResourceSubtype::Bmp,  ResourceSubtype::Mp4,
        ResourceSubtype::Avi,  ResourceSubtype::Mkv,  ResourceSubtype::Webm,
        ResourceSubtype::Mp3,  ResourceSubtype::Wav,  ResourceSubtype::Ogg,
        ResourceSubtype::Flac, ResourceSubtype::Aac,  ResourceSubtype::Pdf,
        ResourceSubtype::Doc,  ResourceSubtype::Xls,  ResourceSubtype::Zip,
    };

    for (auto subtype : types) {
        auto ext = toExtension(subtype);
        EXPECT_FALSE(ext.empty()) << "Extension should not be empty for known type";
        auto roundTripped = subtypeFromExtension(ext);
        EXPECT_EQ(roundTripped, subtype)
            << "Round-trip failed for extension: " << ext;
    }
}

TEST(ResourceSubtypeMapping, JpegAlias) {
    using wechat::core::ResourceSubtype;
    using wechat::core::subtypeFromExtension;

    // .jpeg 和 .jpg 都映射到 Jpeg
    EXPECT_EQ(subtypeFromExtension(".jpg"), ResourceSubtype::Jpeg);
    EXPECT_EQ(subtypeFromExtension(".jpeg"), ResourceSubtype::Jpeg);
}

TEST(ResourceSubtypeMapping, UnknownExtension) {
    using wechat::core::ResourceSubtype;
    using wechat::core::subtypeFromExtension;
    using wechat::core::toExtension;

    EXPECT_EQ(subtypeFromExtension(".xyz"), ResourceSubtype::Unknown);
    EXPECT_EQ(subtypeFromExtension(""), ResourceSubtype::Unknown);
    EXPECT_EQ(toExtension(ResourceSubtype::Unknown), "");
}
