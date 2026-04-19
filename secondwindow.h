#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <QDialog>
#include <QSet>
#include <QTimer>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include "turingmachine.h"

namespace Ui {
class SecondWindow;
}

class TapeWidget;

class SecondWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SecondWindow(QWidget *parent = nullptr);
    ~SecondWindow();
    void setAlphabets(const QSet<QChar>& tapeAlphabet,
                      const QSet<QChar>& extraAlphabet);

private slots:
    void on_pushButtonSetWord_clicked();
    void on_pushButtonStep_clicked();
    void on_pushButtonStart_clicked();
    void on_pushButtonStop_clicked();
    void on_pushButtonReset_clicked();
    void on_pushButtonSpeedUp_clicked();
    void on_pushButtonSpeedDown_clicked();
    void on_pushButton_2_clicked();

    void onAutoStep();
    void updateUi();
    void onMachineStopped(const QString &reason);

private:
    void rebuildProgramTable();
    void loadProgramFromTable();
    bool parseCommandText(const QString &text,
                          const QString &currentState,
                          QChar readSymbol,
                          Command &cmd) const;
    QStringList generateStates(int count) const;
    QList<QChar> sortedAlphabetList() const;
    void highlightCurrentStateRow();
    void updateTapeLabels();
    void setEditingEnabled(bool enabled);
    bool isMoveCommand(const QString &s) const;
    void updateUiTextOnly();
    void animateCaret(int direction);
    void onCaretAnimationFinished();

private:
    Ui::SecondWindow *ui;
    TuringMachine *m_machine;
    QTimer *m_timer;

    int m_stateCount = 3;
    int m_stepIntervalMs = 500;
    TapeWidget *m_tapeWidget;
    QPropertyAnimation *m_caretAnimation = nullptr;
    int m_lastAnimatedMove = 0;

    int m_viewOffset = 0;
    int m_caretScreenIndex = 5;
    int m_pendingDirection = 0;
    bool m_stepInProgress = false;

    void placeCaretOverCell(int index);
    void animateCaretToCell(int newIndex);
    void finishStepAnimation();
};



#endif // SECONDWINDOW_H
