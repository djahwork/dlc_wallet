#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    if (wally_init(0) != WALLY_OK) {
        qDebug() << "Erreur : Impossible d'initialiser libwally-core.";
        return 1;
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
