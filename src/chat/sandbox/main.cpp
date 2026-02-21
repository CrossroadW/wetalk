#include <QApplication>
#include <spdlog/spdlog.h>

#include <wechat/log/Log.h>

#include "ChatSandbox.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    wechat::log::init();

    wechat::chat::ChatSandbox sandbox;
    sandbox.show();

    spdlog::info("Chat sandbox started");
    return app.exec();
}
