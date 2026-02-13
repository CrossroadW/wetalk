#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Contacts Sandbox");
    window.setCentralWidget(new QLabel("Contacts module sandbox - TODO"));
    window.resize(800, 600);
    window.show();

    spdlog::info("Contacts sandbox started");
    return app.exec();
}
