#ifndef TURINGMACHINE_H
#define TURINGMACHINE_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

struct Command
{
    QChar writeSymbol;
    QChar move;
    QString nextState;
    bool isValid = false;
};

class TuringMachine : public QObject
{
    Q_OBJECT

public:
    explicit TuringMachine(QObject *parent = nullptr);

    void setAlphabets(const QSet<QChar> &tapeAlphabet,
                      const QSet<QChar> &extraAlphabet,
                      QChar blankSymbol = QChar(0x039B));

    bool setInputWord(const QString &word);
    void clearProgram();
    void setStates(const QStringList &states);
    void setCommand(const QString &state, QChar readSymbol, const Command &cmd);

    bool step();
    void reset();

    QChar symbolAt(int index) const;
    int headPosition() const;
    QString currentState() const;
    QStringList states() const;
    QSet<QChar> fullAlphabet() const;
    QSet<QChar> tapeAlphabet() const;
    QChar blankSymbol() const;

    bool isRunning() const;
    void setRunning(bool value);

    bool hasAnyHaltCommand() const;

    QChar lastMove() const;

signals:
    void machineChanged();
    void machineStopped(const QString &reason);

private:
    QString makeKey(const QString &state, QChar readSymbol) const;

private:
    QSet<QChar> m_tapeAlphabet;
    QSet<QChar> m_extraAlphabet;
    QSet<QChar> m_fullAlphabet;

    QMap<int, QChar> m_tape;
    QMap<int, QChar> m_initialTape;

    QMap<QString, Command> m_program;

    QStringList m_states;

    int m_headPos = 0;
    int m_initialHeadPos = 0;

    QString m_currentState = "q0";
    QString m_initialState = "q0";

    QChar m_blank = QChar(0x039B);
    bool m_running = false;

    QChar m_lastMove = 'S';
};

#endif