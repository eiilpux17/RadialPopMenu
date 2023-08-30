#ifndef RADIALMENU_H
#define RADIALMENU_H

#include <QWidget>
#include<QToolButton>
#include <QTimeLine>

class QVariantAnimation;
class CircleToolButton;

class RadialMenu : public QWidget
{
    Q_OBJECT

public:
    RadialMenu(QWidget *parent = nullptr);
    ~RadialMenu();

    // 设置菜单图标
    void setMenuIcon(const QIcon & icon, QSize iconSize = QSize(24,24));

    // 设置基准动画时常
    void setAnimationDuration(int duration);
    // 设置每个action的动画延迟，duration不包含delay
    void setActionAnimationDelay(int delay);
    // 增加/移除指定action
    void addAction(QAction * action);
    void removeAction(QAction *action);
    // 设置action展开起始角度、角度范围，以控件中心为准
    void setRadialAngle(qreal startAngle, qreal spanAngle);
    // 设置展开距离，以控件中心为准
    void setRadialDistance(int distance);
    // 获取展开时容纳所有action的尺寸
    QSize idealSize();
    // 设置对齐，当窗口尺寸与实际显示不一致时，action区域的对齐方向
    void setAlignment(Qt::Alignment alignment);

protected:
    bool event(QEvent *e);
    void paintEvent(QPaintEvent *event);

private:
    void onMenuClicked();

private:
    // 更新动画duration
    void updateTimeLine();
    // 布局
    QSize doLayout(bool calcOnly = false);
    // 动画
    void onAnimationValueChanged(QVariant value);

public:
    CircleToolButton * rootButton = nullptr;
    struct ActioItem;
    QVector<ActioItem*> allActionItem;
    int radialDistance = 100;

    QVariantAnimation * progressAnimation = nullptr;
    QTimeLine * baseTimeLine = nullptr;
    int actionDelay= 0;
    int animationDuration = 100;

    qreal _startAngle = 0;
    qreal _spanAngle = 2 * 3.1415926;
    Qt::Alignment _align = Qt::AlignLeft | Qt::AlignTop;
};


class CircleToolButton : public QToolButton
{
public:
    using QToolButton::QToolButton;
    // 设置绘制的不透明度
    void setOpacity(qreal opacity);
protected:
    // 点击区域，因为窗口是矩形，限定圆形范围
    bool hitButton(const QPoint &pos) const;
    void paintEvent(QPaintEvent *);
    bool event(QEvent *e);
    // 坐标是否在圆形范围内
    bool posInCircle(const QPoint &pos) const;

private:
    qreal paintOpacaty = 1.0;
};



#endif // RADIALMENU_H
