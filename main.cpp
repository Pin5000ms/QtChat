#include "mainwindow.h"

#include <QApplication>


void applyStyleSheet(QApplication &app) {
    QFile file(":/style/style.qss"); // 可以使用資源文件，或直接用 "style.qss"
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        app.setStyleSheet(stream.readAll());
        file.close();
    }
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    applyStyleSheet(app);
    MainWindow w;
    w.show();
    return app.exec();
}
