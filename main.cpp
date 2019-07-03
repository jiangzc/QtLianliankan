#include "main_game_window.h"
#include <QApplication>
#include "startdialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainGameWindow w;

    StartDialog startdlg;
    startdlg.setStyleSheet("QDialog {background-image: url(:/res/image/background/7.jpg)}");

    if (startdlg.exec() == QDialog::Accepted) {
        w.show();
        return a.exec();
    }
    return 0;

}
