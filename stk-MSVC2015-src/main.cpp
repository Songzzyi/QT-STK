#include "QT_STK.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    // 开启Qt对高DPI显示的支持
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    // 开启Qt对高分辨率版本pixmap的支持
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    // 设置对高DPI值的四舍五入规则
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication a(argc, argv);
    QT_STK w;
    w.show();

    return a.exec();
}
