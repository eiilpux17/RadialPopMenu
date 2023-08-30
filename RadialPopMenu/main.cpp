#include "radialmenu.h"

#include <QAction>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet("file:///:/stylesheet.qss");
    RadialMenu * menu= new RadialMenu();
    menu->setMenuIcon(QIcon(":/res/geren.svg"), QSize(24, 24));
    menu->resize(400, 400);
    menu->show();
    menu->setAnimationDuration(300);
    menu->setActionAnimationDelay(50);
    menu->setRadialAngle( -3.1415926 / 2,  -1 * 3.1415926);
    menu->setRadialDistance(100);
    menu->setAlignment(Qt::AlignCenter);
    menu->setContentsMargins(10, 10, 10, 10);


    QVector<QString> icons = {":/res/daochu.svg",":/res/shezhi2.svg", ":/res/sousuo.svg", ":/res/tuichu.svg", ":/res/xiangqing.svg"};
    for(const QString & path : icons)
    {
        QAction * action = new QAction(menu);
        action->setIcon(QIcon(path));
        menu->addAction(action);
    }
    menu->resize(menu->idealSize());
    return a.exec();
}
