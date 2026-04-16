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
        if (!m_tapeAlphabet.contains(c)) return false;
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