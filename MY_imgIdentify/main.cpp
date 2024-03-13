#include "img_identify.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    img_identify w;
    w.show();

    return a.exec();//处理事件循环
}
