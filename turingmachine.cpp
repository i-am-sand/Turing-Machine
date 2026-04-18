#include "turingmachine.h"

TuringMachine::TuringMachine(QObject *parent)
    : QObject(parent)
{
    m_states << "q0";
}

void TuringMachine::setAlphabets(const QSet<QChar> &tapeAlphabet,
                                 const QSet<QChar> &extraAlphabet,
                                 QChar blankSymbol)
{
    m_tapeAlphabet = tapeAlphabet;
    m_extraAlphabet = extraAlphabet;
    m_blank = blankSymbol;

    m_fullAlphabet = m_tapeAlphabet;
    m_fullAlphabet.unite(m_extraAlphabet);
    m_fullAlphabet.insert(m_blank);

    clearProgram();
    reset();
}

bool TuringMachine::setInputWord(const QString &word) {
    for (QChar c : word) {
        if (!m_tapeAlphabet.contains(c))
            return false;
    }

    m_tape.clear();
    m_initialTape.clear();

    for (int i = 0; i < word.size(); ++i) {
        m_tape[i] = word[i];
        m_initialTape[i] = word[i];
    }

    m_headPos = 0;
    m_initialHeadPos = 0;
    m_currentState = m_initialState;

    emit machineChanged();
    return true;
}

void TuringMachine::clearProgram() {
    m_program.clear();
}

void TuringMachine::setStates(const QStringList &states) {
    m_states = states;
    if (m_states.isEmpty())
        m_states << "q0";
    m_initialState = m_states.first();
    m_currentState = m_initialState;
}

void TuringMachine::setCommand(const QString &state, QChar readSymbol, const Command &cmd) {
    m_program[makeKey(state, readSymbol)] = cmd;
}

bool TuringMachine::step() {
    QChar current = symbolAt(m_headPos);
    QString key = makeKey(m_currentState, current);
    if (!m_program.contains(key) || !m_program[key].isValid) {
        emit machineStopped("Нет команды для текущего состояния и символа.");
        return false;
    }
    Command cmd = m_program[key];
    if (cmd.move == 'L')
        --m_headPos;
    else if (cmd.move == 'R')
        ++m_headPos;
    m_currentState = cmd.nextState;
    emit machineChanged();
    if (m_currentState.trimmed().toLower() == "halt") {
        emit machineStopped("Машина остановилась.");
        return false;
    }
    return true;
}

void TuringMachine::reset() {
    m_tape = m_initialTape;
    m_headPos = m_initialHeadPos;
    m_currentState = m_initialState;
    emit machineChanged();
}

QChar TuringMachine::symbolAt(int index) const {
    return m_tape.contains(index) ? m_tape.value(index) : m_blank;
}

int TuringMachine::headPosition() const {
    return m_headPos;
}

QString TuringMachine::currentState() const {
    return m_currentState;
}

QStringList TuringMachine::states() const {
    return m_states;
}

QSet<QChar> TuringMachine::fullAlphabet() const {
    return m_fullAlphabet;
}

QSet<QChar> TuringMachine::tapeAlphabet() const {
    return m_tapeAlphabet;
}

QChar TuringMachine::blankSymbol() const {
    return m_blank;
}

bool TuringMachine::isRunning() const {
    return m_running;
}

void TuringMachine::setRunning(bool value) {
    m_running = value;
}

bool TuringMachine::hasAnyHaltCommand() const {
    for (auto it = m_program.begin(); it != m_program.end(); ++it) {
        if (it.value().isValid && it.value().nextState.trimmed().toLower() == "halt")
            return true;
    }
    return false;
}

QString TuringMachine::makeKey(const QString &state, QChar readSymbol) const {
    return state + "|" + readSymbol;
}