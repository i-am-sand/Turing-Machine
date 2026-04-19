#include "tapewidget.h"
#include "turingmachine.h"

#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QtMath>

TapeWidget::TapeWidget(QWidget *parent)
    : QWidget(parent)
    , m_animTimer(new QTimer(this))
{
    setMinimumHeight(140);
    connect(m_animTimer, &QTimer::timeout, this, &TapeWidget::onAnimTick);
}

void TapeWidget::setMachine(TuringMachine *machine)
{
    m_machine = machine;
    syncCaretToHead();
}

void TapeWidget::syncCaretToHead()
{
    if (!m_machine)
        return;

    int screenCell = m_machine->headPosition() - m_firstVisibleIndex;
    m_caretX = screenCell * m_cellWidth + m_cellWidth / 2.0;
    m_targetCaretX = m_caretX;
    update();
}

void TapeWidget::animateCaretToHead()
{
    if (!m_machine)
        return;

    int screenCell = m_machine->headPosition() - m_firstVisibleIndex;
    m_targetCaretX = screenCell * m_cellWidth + m_cellWidth / 2.0;
    m_animTimer->start(16);
}

void TapeWidget::onAnimTick()
{
    const double speed = 4.0;

    if (m_caretX < m_targetCaretX)
    {
        m_caretX += speed;
        if (m_caretX > m_targetCaretX)
            m_caretX = m_targetCaretX;
    }
    else if (m_caretX > m_targetCaretX)
    {
        m_caretX -= speed;
        if (m_caretX < m_targetCaretX)
            m_caretX = m_targetCaretX;
    }

    update();

    if (qAbs(m_caretX - m_targetCaretX) < 0.001)
        m_animTimer->stop();
}

void TapeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), Qt::white);

    if (!m_machine)
        return;

    for (int i = 0; i < m_visibleCells; ++i)
    {
        int tapeIndex = m_firstVisibleIndex + i;
        int x = i * m_cellWidth;

        QRect cellRect(x, 45, m_cellWidth, m_cellHeight);

        p.setPen(Qt::black);
        p.setBrush(Qt::NoBrush);
        p.drawRect(cellRect);
        p.drawText(cellRect, Qt::AlignCenter, QString(m_machine->symbolAt(tapeIndex)));
    }

    QPoint top(static_cast<int>(m_caretX), 15);
    QPoint left(static_cast<int>(m_caretX) - 8, 30);
    QPoint right(static_cast<int>(m_caretX) + 8, 30);

    QPolygon triangle;
    triangle << top << left << right;

    p.setPen(Qt::black);
    p.setBrush(QColor(100, 220, 255));
    p.drawPolygon(triangle);
}