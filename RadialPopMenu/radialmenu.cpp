#include "radialmenu.h"
#include <QEvent>
#include <QPainter>
#include <QStyleOptionToolButton>
#include <QStylePainter>
#include <QToolButton>
#include <cmath>
#include <QPropertyAnimation>
#include <QAction>
#include <QDebug>
#include <QMouseEvent>
#include <QApplication>

static constexpr qreal _2PI = 3.1415926 * 2;

inline QPoint pointInterpolate(const QPoint & from, const QPoint & to, qreal progress)
{
    return from + (to - from) * progress;
}

struct RadialMenu::ActioItem
{
    CircleToolButton * button;
    QPoint startPos;
    QPoint endPos;
};

// 跟菜单，处理拖拽移动窗口
class RadialMenuRoot : public CircleToolButton
{
public:
    using CircleToolButton::CircleToolButton;

    // 是否移动menu的顶层窗口，方便拖拽
    void setMoveTopWindow(bool top){
        moveTop = top;
    }
protected:
    bool event(QEvent *e)
    {
        switch (e->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        {
            QMouseEvent * me = static_cast<QMouseEvent *>(e);
            if(me->button() == Qt::LeftButton)
            {
                oldMousePos = me->globalPos();
                mousePressed = true;
            }
        }
            break;
        case QEvent::MouseMove:
            if(mousePressed)
            {
                QMouseEvent * me = static_cast<QMouseEvent *>(e);
                QPoint delta = QPoint(me->globalPos() - oldMousePos);
                // 触发拖拽距离判定
                if(!startDrag)
                    startDrag = std::max(std::abs(delta.x()), std::abs(delta.y())) >= QApplication::startDragDistance();
                if(startDrag)
                {
                    QWidget * w = moveTop ? this->window() : this;
                    w->move(w->pos() + delta);
                    oldMousePos = me->globalPos();
                }
            }
            break;
        case QEvent::MouseButtonRelease:
        {
            if(static_cast<QMouseEvent *>(e)->button() == Qt::LeftButton)
                startDrag = mousePressed = false;
        }
            break;

        default:
            break;
        }
        return CircleToolButton::event(e);
    }
    bool hitButton(const QPoint &pos) const
    {
        // 如果处于拖拽中，这不处理按下逻辑
        // 这种处理方式可能有问题
        if(startDrag)
            return false;
        return CircleToolButton::hitButton(pos);
    }
private:
    QPoint oldMousePos;
    bool mousePressed = false;
    bool moveTop = true;
    bool startDrag = false;
};


RadialMenu::RadialMenu(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    RadialMenuRoot * root = new RadialMenuRoot(this);
    root->setMoveTopWindow(true);
    rootButton = root;
    rootButton->setIconSize(QSize(24, 24));
    rootButton->setCheckable(true);
    rootButton->setFocusPolicy(Qt::NoFocus);

    connect(rootButton, &CircleToolButton::clicked, this, &RadialMenu::onMenuClicked);

    // 使用QVariantAnimation触发完整动画进度
    // 因为可能设置延迟，所以有时间差
    // 使用QTimeLine计算当前进度的时间，不参与动画
    progressAnimation = new QVariantAnimation(this);
    progressAnimation->setStartValue(0.0);
    progressAnimation->setEndValue(1.0);
    baseTimeLine = new QTimeLine(animationDuration, this);
    connect(progressAnimation, &QVariantAnimation::valueChanged, this, &RadialMenu::onAnimationValueChanged);
}

RadialMenu::~RadialMenu()
{
    // 不关注QAction的所有权
    qDeleteAll(allActionItem);
    allActionItem.clear();
}

void RadialMenu::setMenuIcon(const QIcon &icon, QSize iconSize)
{
    rootButton->setIcon(icon);
    rootButton->setIconSize(iconSize);
}

void RadialMenu::setAnimationDuration(int duration)
{
    animationDuration = duration;
    if(duration > 0)
        baseTimeLine->setDuration(duration);
    updateTimeLine();
}

void RadialMenu::setActionAnimationDelay(int delay)
{
    actionDelay = delay;
    updateTimeLine();
}

void RadialMenu::addAction(QAction *action)
{
    for(ActioItem * item : allActionItem)
    {
        if(item->button->defaultAction() == action)
            return;
    }

    ActioItem * item = new ActioItem();
    item->button = new CircleToolButton(this);
    item->button->setIconSize(rootButton->iconSize());
    item->button->setFocusPolicy(Qt::NoFocus);
    item->button->setDefaultAction(action);
    item->button->setVisible(true);
    allActionItem << item;

    updateTimeLine();
    doLayout();
}

void RadialMenu::removeAction(QAction *action)
{
    auto it = allActionItem.begin();
    while(it != allActionItem.end())
    {
        ActioItem * item = (*it);
        if(item->button->defaultAction() == action)
        {
            item->button->deleteLater();
            item->button = nullptr;
            delete item;
            allActionItem.erase(it);
            return;
        }
    }
    updateTimeLine();
    doLayout();
}

void RadialMenu::setRadialAngle(qreal startAngle, qreal spanAngle)
{
    _startAngle= startAngle;
    _spanAngle = spanAngle;
    doLayout();
}

void RadialMenu::setRadialDistance(int distance)
{
    if(distance > 0)
    {
        radialDistance = distance;
        doLayout();
    }
}

QSize RadialMenu::idealSize()
{
    return doLayout(true);
}

void RadialMenu::setAlignment(Qt::Alignment alignment)
{
    _align = alignment;
    doLayout();
}

bool RadialMenu::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Resize:
    case QEvent::LayoutRequest:
        doLayout();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void RadialMenu::paintEvent(QPaintEvent *)
{
}

void RadialMenu::onMenuClicked()
{
    // 动画时长小，不处理
    if(animationDuration <= 0)
    {
        doLayout();
        return;
    }
    if(rootButton->isChecked())
    {
        // 展开
        if(progressAnimation->state() == QAbstractAnimation::Running)
        {
            progressAnimation->setPaused(true);
            progressAnimation->setDirection(QAbstractAnimation::Forward);
            progressAnimation->resume();
        }
        else
        {
            progressAnimation->setDirection(QAbstractAnimation::Forward);
            progressAnimation->start();
        }
    }
    else
    {
        // 折叠
        if(progressAnimation->state() == QAbstractAnimation::Running)
        {
            progressAnimation->setPaused(true);
            progressAnimation->setDirection(QAbstractAnimation::Backward);
            progressAnimation->resume();
        }
        else
        {
            progressAnimation->setDirection(QAbstractAnimation::Backward);
            progressAnimation->start();
        }
    }
}

void RadialMenu::updateTimeLine()
{
    if(animationDuration > 0)
    {
        if(actionDelay <= 0 || allActionItem.isEmpty())
            progressAnimation->setDuration(animationDuration);
        else
            progressAnimation->setDuration(animationDuration + (allActionItem.count() - 1) * actionDelay);
    }
}

QSize RadialMenu::doLayout(bool calcOnly)
{
    if(!calcOnly && !this->size().isValid())
        return QSize();

    // 布局
    // 先将所有控件以(0,0)为中心布局，计算展开后所有控件的区域
    // 再将显示区域对齐到窗口区域，计算偏移
    // 将所有控件移动偏移到最终位置
    // item_rects前面是action的区域信息，最后一个是中心菜单区域
    QVector<QPair<QRect, QRect>> item_rects;
    item_rects.resize(allActionItem.count() + 1);

    QRect root_rect = rootButton->rect();
    root_rect.moveCenter(QPoint(0, 0));
    item_rects.last().first = root_rect;

    QRect bounding = root_rect;
    const int count = allActionItem.count();
    qreal angle_step = _spanAngle / (count - 1);
    qreal offset = _startAngle;
    for(int i = 0 ; i < count; i++)
    {
        // 起始区域
        QPair<QRect, QRect> & button_rects = item_rects[i];
        button_rects.first = allActionItem[i]->button->rect();
        button_rects.first.moveCenter(QPoint(0, 0));

        // 展开区域
        button_rects.second = button_rects.first;
        qreal angle = angle_step * i + offset;
        qreal dx = std::cos(angle) * radialDistance;
        qreal dy = std::sin(angle) * radialDistance;
        button_rects.second.moveCenter(QPointF(dx, dy).toPoint());

        // 所有控件区域
        bounding = bounding.united(button_rects.second);
    }

    if(calcOnly)
    {
        // 直接返回容纳所有控件的size
        int left = 0, top = 0, right = 0, bottom = 0;
        getContentsMargins(&left, &top, &right, &bottom);
        return bounding.size() + QSize(left + right, top + bottom);
    }
    else
    {
        bool expanded = rootButton->isChecked();
        QRect cr = this->contentsRect();
        QRect layout_bounding = QStyle::alignedRect(Qt::LeftToRight, _align, bounding.size(), cr);
        QPoint delta = layout_bounding.topLeft() - bounding.topLeft();
        for(int i = 0 ; i < allActionItem.count(); i++)
        {
            ActioItem * item = allActionItem[i];
            const QPair<QRect, QRect> & button_rects = item_rects[i];
            item->startPos = button_rects.first.topLeft() + delta;
            item->endPos = button_rects.second.topLeft() + delta;
            if(expanded)
            {
                item->button->setGeometry(QRect(item->endPos, button_rects.first.size()));
                item->button->setOpacity(1.0);
            }
            else
            {
                item->button->setGeometry(QRect(item->startPos, button_rects.second.size()));
                item->button->setOpacity(0.0);
            }

        }
        rootButton->move(item_rects.last().first.topLeft() + delta);
        rootButton->raise();
        return QSize(layout_bounding.right(), layout_bounding.bottom());
    }
}
void RadialMenu::onAnimationValueChanged(QVariant)
{
    // 动画时间
    int time = progressAnimation->currentTime();
    const int count = allActionItem.count();
    for(int i = 0 ; i< count; i++)
    {
        ActioItem * item = allActionItem[i];
        // 对应action的实际时间
        int real_time = (time - actionDelay * i);
        if(time <= 0)
        {
            // 此时对应action延时还未结束
            item->button->move(item->startPos);
            item->button->setOpacity(0.0);
        }
        else
        {
            // action实际进度
            qreal progress = baseTimeLine->valueForTime(real_time);
            QPoint curr_pos = pointInterpolate(item->startPos, item->endPos, progress);
            item->button->move(curr_pos);
            item->button->setOpacity(1.0);
        }
    }
}

void CircleToolButton::setOpacity(qreal opacity)
{
    paintOpacaty = opacity;
    update();
}

bool CircleToolButton::hitButton(const QPoint &pos) const
{
    return posInCircle(pos);
}

void CircleToolButton::paintEvent(QPaintEvent *)
{
    QRect rect = this->contentsRect();
    int r = std::min(rect.width(), rect.height()) / 2;
    QPainterPath path;
    path.addEllipse(rect.center(), r, r);

    QStylePainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(paintOpacaty);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    // 悬停状态，但没有在圆形范围内，去掉悬停状态
    if(opt.state & QStyle::State_MouseOver && !posInCircle(this->mapFromGlobal(QCursor::pos())))
        opt.state &= ~QStyle::State_MouseOver;
    p.setClipPath(path);
    p.drawComplexControl(QStyle::CC_ToolButton, opt);
}

bool CircleToolButton::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::HoverMove:
        // 刷新，要动态计算是否悬停
        update();
        break;
    default:
        break;
    }
    return QToolButton::event(e);
}

bool CircleToolButton::posInCircle(const QPoint &pos) const
{
    QRect rect = this->contentsRect();
    int r = std::min(rect.width(), rect.height()) / 2;
    QPoint dc = rect.center() - pos;
    return r * r > std::pow(dc.x(), 2) + std::pow(dc.y(), 2);
}
