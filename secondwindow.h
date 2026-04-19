#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <QDialog>
#include <QSet>
#include <QTimer>
#include "turingmachine.h"

namespace Ui {
class SecondWindow;
}

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

private:
    Ui::SecondWindow *ui;
    TuringMachine *m_machine;
    QTimer *m_timer;

    int m_stateCount = 3;
    int m_stepIntervalMs = 500;
};



#endif // SECONDWINDOW_H
