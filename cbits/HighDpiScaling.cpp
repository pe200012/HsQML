#include "QtGui/QGuiApplication"

extern "C" void hsqml_enable_high_dpi_scaling() {
#if QT_VERSION < 0x060000
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
}
