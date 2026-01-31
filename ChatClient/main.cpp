#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // 处理高分屏缩放
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
