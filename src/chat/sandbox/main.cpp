#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Chat Sandbox");
    window.setCentralWidget(new QLabel("Chat module sandbox - TODO"));
    window.resize(800, 600);
    window.show();

    spdlog::info("Chat sandbox started");
    return app.exec();
}
