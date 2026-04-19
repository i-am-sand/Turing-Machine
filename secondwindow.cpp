#include "secondwindow.h"
#include "ui_secondwindow.h"
#include <QMessageBox>

SecondWindow::SecondWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SecondWindow)
    , m_machine(new TuringMachine(this))
    ,m_timer(new QTimer(this))
{
    ui->setupUi(this);

    ui->lineEditInputWord->setEnabled(false);
    ui->pushButtonSetWord->setEnabled(false);
    ui->tableWidgetProgram->setEnabled(false);
    ui->pushButtonAddState->setEnabled(false);
    ui->pushButtonRemoveState->setEnabled(false);

    ui->pushButtonStart->setEnabled(false);
    ui->pushButtonStop->setEnabled(false);
    ui->pushButtonStep->setEnabled(false);
    ui->pushButtonReset->setEnabled(false);
    ui->pushButtonSpeedUp->setEnabled(false);
    ui->pushButtonSpeedDown->setEnabled(false);

    connect(ui->pushButtonAddState, &QPushButton::clicked, this, [this]() {
        ++m_stateCount;
        rebuildProgramTable();
    });

    connect(ui->pushButtonRemoveState, &QPushButton::clicked, this, [this]() {
        if (m_stateCount > 1)
        {
            --m_stateCount;
            rebuildProgramTable();
        }
    });

    connect(m_timer, &QTimer::timeout, this, &SecondWindow::onAutoStep);
    connect(m_machine, &TuringMachine::machineChanged, this, &SecondWindow::updateUi);
    connect(m_machine, &TuringMachine::machineStopped, this, &SecondWindow::onMachineStopped);

    updateUi();
}

SecondWindow::~SecondWindow()
{
    delete ui;
}

void SecondWindow::on_pushButton_2_clicked()
{
    this->close();
}

void SecondWindow::setAlphabets(const QSet<QChar>& tapeAlphabet,
                                const QSet<QChar>& extraAlphabet)
{
    m_machine->setAlphabets(tapeAlphabet, extraAlphabet);

    rebuildProgramTable();

    ui->tableWidgetProgram->setEnabled(true);
    ui->lineEditInputWord->setEnabled(true);
    ui->pushButtonSetWord->setEnabled(true);
    ui->pushButtonAddState->setEnabled(true);
    ui->pushButtonRemoveState->setEnabled(true);
    ui->pushButtonReset->setEnabled(true);
    ui->pushButtonSpeedUp->setEnabled(true);
    ui->pushButtonSpeedDown->setEnabled(true);

}



void SecondWindow::on_pushButtonSetWord_clicked()
{
    QString word = ui->lineEditInputWord->text();

    if (!m_machine->setInputWord(word))
    {
        QMessageBox::warning(this, "Ошибка",
                             "Во входной строке есть символы, которых нет в алфавите ленты.");
        return;
    }
    ui->pushButtonStart->setEnabled(true);
    ui->pushButtonStep->setEnabled(true);

    updateUi();
}

void SecondWindow::on_pushButtonStep_clicked()
{
    loadProgramFromTable();
    m_machine->step();
    updateUi();
}

void SecondWindow::on_pushButtonStart_clicked()
{
    loadProgramFromTable();

    if (!m_machine->hasAnyHaltCommand())
    {
        QMessageBox::warning(this, "Ошибка",
                             "Нельзя запускать машину, если нет ни одной остановки.");
        return;
    }

    m_machine->setRunning(true);
    setEditingEnabled(false);
    m_timer->start(m_stepIntervalMs);
}

void SecondWindow::on_pushButtonStop_clicked()
{
    m_timer->stop();
    m_machine->setRunning(false);
    setEditingEnabled(true);
    updateUi();
}

void SecondWindow::on_pushButtonReset_clicked()
{
    m_timer->stop();
    m_machine->setRunning(false);
    m_machine->reset();
    setEditingEnabled(true);
    updateUi();
}

void SecondWindow::onAutoStep()
{
    if (!m_machine->step())
    {
        m_timer->stop();
        m_machine->setRunning(false);
        setEditingEnabled(true);
    }

    updateUi();
}

void SecondWindow::on_pushButtonSpeedUp_clicked()
{
    m_stepIntervalMs -= 50;
    if (m_stepIntervalMs < 50)
        m_stepIntervalMs = 50;

    if (m_timer->isActive())
        m_timer->start(m_stepIntervalMs);

    updateUi();
}

void SecondWindow::on_pushButtonSpeedDown_clicked()
{
    m_stepIntervalMs += 50;

    if (m_timer->isActive())
        m_timer->start(m_stepIntervalMs);

    updateUi();
}

bool SecondWindow::isMoveCommand(const QString &s) const
{
    QString t = s.trimmed().toUpper();
    return t == "L" || t == "R" || t == "S";
}

QStringList SecondWindow::generateStates(int count) const
{
    QStringList result;
    for (int i = 0; i < count; ++i)
        result << ("q" + QString::number(i));
    return result;
}

QList<QChar> SecondWindow::sortedAlphabetList() const
{
    QList<QChar> result;

    QList<QChar> tape = m_machine->tapeAlphabet().values();
    QList<QChar> extra = m_machine->fullAlphabet().values();

    std::sort(tape.begin(), tape.end(), [](QChar a, QChar b) {
        return a.unicode() < b.unicode();
    });

    result = tape;

    result.append(m_machine->blankSymbol());

    QSet<QChar> tapeSet = m_machine->tapeAlphabet();
    QChar blank = m_machine->blankSymbol();

    QList<QChar> extraOnly;
    for (QChar ch : m_machine->fullAlphabet())
    {
        if (!tapeSet.contains(ch) && ch != blank)
            extraOnly.append(ch);
    }

    std::sort(extraOnly.begin(), extraOnly.end(), [](QChar a, QChar b) {
        return a.unicode() < b.unicode();
    });

    for (QChar ch : extraOnly)
        result.append(ch);

    return result;
}

void SecondWindow::rebuildProgramTable()
{
    QStringList states = generateStates(m_stateCount);
    m_machine->setStates(states);

    QList<QChar> alphabet = sortedAlphabetList();

    ui->tableWidgetProgram->clear();
    ui->tableWidgetProgram->setRowCount(states.size());
    ui->tableWidgetProgram->setColumnCount(alphabet.size());

    QStringList horizontalHeaders;
    for (QChar ch : alphabet)
        horizontalHeaders << QString(ch);

    ui->tableWidgetProgram->setHorizontalHeaderLabels(horizontalHeaders);
    ui->tableWidgetProgram->setVerticalHeaderLabels(states);

    ui->tableWidgetProgram->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

bool SecondWindow::parseCommandText(const QString &text,
                                    const QString &currentState,
                                    QChar readSymbol,
                                    Command &cmd) const
{
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return false;

    QStringList parts = trimmed.split(',', Qt::KeepEmptyParts);
    for (QString &part : parts)
        part = part.trimmed();

    cmd.writeSymbol = readSymbol;
    cmd.move = 'S';
    cmd.nextState = currentState;
    cmd.isValid = false;

    if (parts.size() == 1)
    {
        if (isMoveCommand(parts[0]))
        {
            cmd.move = parts[0].trimmed().toUpper()[0];
            cmd.isValid = true;
            return true;
        }
        cmd.nextState = parts[0];
        cmd.isValid = !cmd.nextState.isEmpty();
        return cmd.isValid;
    }

    if (parts.size() == 2)
    {
        if (isMoveCommand(parts[0]))
        {
            cmd.move = parts[0].trimmed().toUpper()[0];
            cmd.nextState = parts[1].isEmpty() ? currentState : parts[1];
            cmd.isValid = true;
            return true;
        }

        if (parts[0].size() == 1 && isMoveCommand(parts[1]))
        {
            cmd.writeSymbol = parts[0][0];
            cmd.move = parts[1].trimmed().toUpper()[0];
            cmd.nextState = currentState;
            cmd.isValid = true;
            return true;
        }

        if (parts[0].size() == 1)
        {
            cmd.writeSymbol = parts[0][0];
            cmd.move = 'S';
            cmd.nextState = parts[1].isEmpty() ? currentState : parts[1];
            cmd.isValid = true;
            return true;
        }

        return false;
    }

    if (parts.size() == 3)
    {
        QString writePart = parts[0];
        QString movePart  = parts[1].toUpper();
        QString nextPart  = parts[2];

        if (writePart.size() != 1)
            return false;

        if (!isMoveCommand(movePart))
            return false;

        cmd.writeSymbol = writePart[0];
        cmd.move = movePart[0];
        cmd.nextState = nextPart.isEmpty() ? currentState : nextPart;
        cmd.isValid = true;
        return true;
    }

    return false;
}
void SecondWindow::loadProgramFromTable()
{
    m_machine->clearProgram();

    QStringList states = generateStates(m_stateCount);
    m_machine->setStates(states);

    QList<QChar> alphabet = sortedAlphabetList();

    for (int row = 0; row < ui->tableWidgetProgram->rowCount(); ++row)
    {
        QString state = states[row];

        for (int col = 0; col < ui->tableWidgetProgram->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetProgram->item(row, col);
            if (!item)
                continue;

            QString text = item->text().trimmed();
            if (text.isEmpty())
                continue;

            Command cmd;
            if (!parseCommandText(text, state, alphabet[col], cmd))
            {
                QMessageBox::warning(this, "Ошибка",
                                     QString("Некорректная команда в строке %1, столбце %2")
                                         .arg(row + 1).arg(col + 1));
                return;
            }

            if (!m_machine->fullAlphabet().contains(cmd.writeSymbol))
            {
                QMessageBox::warning(this, "Ошибка",
                                     QString("Символ '%1' не входит в алфавит.")
                                         .arg(cmd.writeSymbol));
                return;
            }

            m_machine->setCommand(state, alphabet[col], cmd);
        }
    }
}

void SecondWindow::updateTapeLabels()
{
    QLabel* labels[11] = {
        ui->labelCell0, ui->labelCell1, ui->labelCell2,
        ui->labelCell3, ui->labelCell4, ui->labelCell5,
        ui->labelCell6, ui->labelCell7, ui->labelCell8,
        ui->labelCell9, ui->labelCell10
    };

    int head = m_machine->headPosition();

    for (int i = 0; i < 11; ++i)
    {
        int tapeIndex = head + (i - 5);
        labels[i]->setText(QString(m_machine->symbolAt(tapeIndex)));
    }
}

void SecondWindow::updateUi()
{
    ui->labelCurrentState->setText("Состояние: " + m_machine->currentState());
    ui->labelSpeed->setText("Скорость: " + QString::number(m_stepIntervalMs) + " мс");

    updateTapeLabels();
    highlightCurrentStateRow();
}

void SecondWindow::highlightCurrentStateRow()
{
    QString current = m_machine->currentState();

    for (int row = 0; row < ui->tableWidgetProgram->rowCount(); ++row)
    {
        bool active = (ui->tableWidgetProgram->verticalHeaderItem(row) &&
                       ui->tableWidgetProgram->verticalHeaderItem(row)->text() == current);

        for (int col = 0; col < ui->tableWidgetProgram->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetProgram->item(row, col);
            if (!item)
            {
                item = new QTableWidgetItem();
                ui->tableWidgetProgram->setItem(row, col, item);
            }

            item->setBackground(active ? QColor(255, 255, 180) : Qt::white);
        }
    }
}

void SecondWindow::setEditingEnabled(bool enabled)
{
    ui->lineEditInputWord->setEnabled(enabled);
    ui->pushButtonSetWord->setEnabled(enabled);
    ui->tableWidgetProgram->setEnabled(enabled);
    ui->pushButtonAddState->setEnabled(enabled);
    ui->pushButtonRemoveState->setEnabled(enabled);

    ui->pushButtonStart->setEnabled(enabled);
    ui->pushButtonStep->setEnabled(enabled);

    ui->pushButtonStop->setEnabled(!enabled);
    ui->pushButtonReset->setEnabled(true);
    ui->pushButtonSpeedUp->setEnabled(true);
    ui->pushButtonSpeedDown->setEnabled(true);
}

void SecondWindow::onMachineStopped(const QString &reason)
{
    m_timer->stop();
    m_machine->setRunning(false);
    setEditingEnabled(true);
    updateUi();

    QMessageBox::information(this, "Остановка", reason);
}


