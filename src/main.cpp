#include <QApplication>
#include <qmainwindow.h>
#include <qwidget.h>
#include <wechat/log/Log.h>
#include <spdlog/spdlog.h>
#include <QMainWindow>
int main(int argc, char *argv[]) {
    wechat::log::init();

    spdlog::trace("This is a trace message");
    spdlog::debug("This is a debug message");
    spdlog::info("This is an info message");
    spdlog::warn("This is a warning message");
    spdlog::error("This is an error message");
    spdlog::critical("This is a critical message");
    QApplication app(argc, argv);
    
    auto w = new QWidget;

    w->show();
    return app.exec();
}
