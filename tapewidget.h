#ifndef TAPEWIDGET_H
#define TAPEWIDGET_H

#include <QWidget>

class QTimer;
class TuringMachine;

class TapeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TapeWidget(QWidget *parent = nullptr);

    void setMachine(TuringMachine *machine);
    void syncCaretToHead();
    void animateCaretToHead();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onAnimTick();

private:
    TuringMachine *m_machine = nullptr;
    QTimer *m_animTimer = nullptr;

    double m_caretX = 0.0;
    double m_targetCaretX = 0.0;

    int m_cellWidth = 50;
    int m_cellHeight = 50;
    int m_visibleCells = 11;
    int m_firstVisibleIndex = -5;
};

#endif // TAPEWIDGET_H