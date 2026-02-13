#include <QApplication>
#include <spdlog/spdlog.h>
int main(int argc, char *argv[]) {
    spdlog::info("WeChat Clone starting...");

    QApplication app(argc, argv);

    spdlog::info("Application initialized");
    return app.exec();
}
