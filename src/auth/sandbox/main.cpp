#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Auth Sandbox");
    window.setCentralWidget(new QLabel("Auth module sandbox - TODO"));
    window.resize(800, 600);
    window.show();

    spdlog::info("Auth sandbox started");
    return app.exec();
}
