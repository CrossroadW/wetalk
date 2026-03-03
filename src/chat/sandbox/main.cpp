#include <QApplication>
#include <spdlog/spdlog.h>

#include <wechat/core/AppPaths.h>
#include <wechat/core/Log.h>

#include "ChatSandbox.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    wechat::log::init();
    wechat::core::AppPaths::setDataDir(PROJECT_ROOT_PATH);

    wechat::chat::ChatSandbox sandbox;
    sandbox.show();

    spdlog::info("Chat sandbox started");
    return app.exec();
}
