#include "secondwindow.h"
#include "ui_secondwindow.h"
#include <QMessageBox>

SecondWindow::SecondWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SecondWindow)
    , m_machine(new TuringMachine(this))
    , m_timer(new QTimer(this))
    , m_caretAnimation(nullptr)
{
    // Создание элементов интерфейса
    ui->setupUi(this);

    // Установка флагов для сворачивания окна и открытия на весь экран
    setWindowFlags(Qt::Window |
                   Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint |
                   Qt::WindowCloseButtonHint);

    // Анимация головки
    m_caretAnimation = new QPropertyAnimation(ui->labelHeadArrow, "pos", this);
    m_caretAnimation->setDuration(180);
    m_caretAnimation->setEasingCurve(QEasingCurve::Linear);

    // После окончания анимации вызов метода finishStepAnimation
    connect(m_caretAnimation, &QPropertyAnimation::finished,
            this, &SecondWindow::finishStepAnimation);

    // Ставит головку на центральную ячейку ленты
    QPoint p = ui->labelHeadArrow->pos();
    p.setX(ui->labelCell5->x() + ui->labelCell5->width() / 2 - ui->labelHeadArrow->width() / 2);
    ui->labelHeadArrow->move(p);

    // Отключение кнопок до ввода алфавитов и строки
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

    // При нажатии кнопки увеличивается/уменьшается число состояний и таблица строится заново
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
    // Присрабатывании таймера вызывется автоматический шаг
    connect(m_timer, &QTimer::timeout, this, &SecondWindow::onAutoStep);
    connect(m_machine, &TuringMachine::machineChanged, this, &SecondWindow::updateUiTextOnly);
    connect(m_machine, &TuringMachine::machineStopped, this, &SecondWindow::onMachineStopped);

    updateUi();
}

SecondWindow::~SecondWindow()
{
    delete ui;
}

void SecondWindow::on_pushButton_2_clicked()
{
    this->parentWidget()->show();
    this->hide();
}

void SecondWindow::setAlphabets(const QSet<QChar>& tapeAlphabet,
                                const QSet<QChar>& extraAlphabet)
{
    // Передача алфавита машине
    m_machine->setAlphabets(tapeAlphabet, extraAlphabet);

    // Перестройка таблицы
    rebuildProgramTable();

    // Разблокировка элементов интерфейса
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
    // Считывание строки
    QString word = ui->lineEditInputWord->text();

    // Проверка корректности строки и её запись на ленту
    if (!m_machine->setInputWord(word))
    {
        QMessageBox::warning(this, "Ошибка",
                             "Во входной строке есть символы, которых нет в алфавите ленты.");
        return;
    }

    m_viewOffset = m_machine->headPosition() - 5;
    m_caretScreenIndex = 5;
    updateTapeLabels();
    placeCaretOverCell(m_caretScreenIndex);

    // Разрешение запуска и шага
    ui->pushButtonStart->setEnabled(true);
    ui->pushButtonStep->setEnabled(true);

    updateUiTextOnly();
}

void SecondWindow::on_pushButtonStep_clicked()
{
    // Если предыдущий шаг не завершен (анимационно), нельзя начать след.
    if (m_stepInProgress)
        return;

    // Текст из таблицы переводится во внутренние команды
    loadProgramFromTable();

    bool ok = m_machine->step();
    if (!ok)
    {
        updateUiTextOnly();
        updateTapeLabels();
        return;
    }

    updateUiTextOnly();

    // Для анимации головки храним последний шаг
    QChar move = m_machine->lastMove();

    if (move == 'R')
    {
        m_pendingDirection = +1;
        m_stepInProgress = true;
        animateCaretToCell(m_caretScreenIndex + 1);
    }
    else if (move == 'L')
    {
        m_pendingDirection = -1;
        m_stepInProgress = true;
        animateCaretToCell(m_caretScreenIndex - 1);
    }
    else
    {
        updateTapeLabels();
    }
}

void SecondWindow::on_pushButtonStart_clicked()
{
    loadProgramFromTable();

    // Есть ли остановки
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
    m_stepInProgress = false;
    m_pendingDirection = 0;

    m_caretAnimation->stop();

    m_machine->reset();

    m_viewOffset = m_machine->headPosition() - 5;
    m_caretScreenIndex = 5;

    updateTapeLabels();
    placeCaretOverCell(m_caretScreenIndex);

    setEditingEnabled(true);
    updateUiTextOnly();
}

// Циклический запуск шагов по таймеру
void SecondWindow::onAutoStep()
{
    if (m_stepInProgress)
        return;

    bool ok = m_machine->step();

    if (!ok)
    {
        m_timer->stop();
        m_machine->setRunning(false);
        setEditingEnabled(true);
        updateUiTextOnly();
        updateTapeLabels();
        return;
    }

    updateUiTextOnly();

    QChar move = m_machine->lastMove();

    if (move == 'R')
    {
        m_pendingDirection = +1;
        m_stepInProgress = true;
        animateCaretToCell(m_caretScreenIndex + 1);
    }
    else if (move == 'L')
    {
        m_pendingDirection = -1;
        m_stepInProgress = true;
        animateCaretToCell(m_caretScreenIndex - 1);
    }
    else
    {
        updateTapeLabels();
    }
}

void SecondWindow::on_pushButtonSpeedUp_clicked()
{
    m_stepIntervalMs -= 50;
    if (m_stepIntervalMs < 50)
        m_stepIntervalMs = 50;

    // Если таймер активен, запускает его с новым интервалом
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
    // Сохраняем старый алфавит
    QMap<QString, QString> oldData;
    QList<QChar> oldAlphabet;

    if (ui->tableWidgetProgram->columnCount() > 0)
    {
        // Запоминаем символы из заголовков столбцов
        for (int col = 0; col < ui->tableWidgetProgram->columnCount(); ++col)
        {
            QTableWidgetItem *headerItem = ui->tableWidgetProgram->horizontalHeaderItem(col);
            if (headerItem && !headerItem->text().isEmpty())
                oldAlphabet.append(headerItem->text()[0]);
        }
        // Запоминание строк
        for (int row = 0; row < ui->tableWidgetProgram->rowCount(); ++row)
        {
            QString state = "q" + QString::number(row);

            for (int col = 0; col < ui->tableWidgetProgram->columnCount(); ++col)
            {
                QTableWidgetItem *item = ui->tableWidgetProgram->item(row, col);
                if (!item || item->text().trimmed().isEmpty())
                    continue;

                QString key = state + "|" + QString(oldAlphabet[col]);
                oldData[key] = item->text();
            }
        }
    }

    // Новый список состояний и новый алфавит
    QStringList states = generateStates(m_stateCount);
    m_machine->setStates(states);

    QList<QChar> newAlphabet = sortedAlphabetList();

    // Проверка
    bool onlyExpansion = true;
    for (QChar ch : oldAlphabet)
    {
        if (!newAlphabet.contains(ch))
        {
            onlyExpansion = false;
            break;
        }
    }

    // Перестраиваем таблицу
    ui->tableWidgetProgram->clear();
    ui->tableWidgetProgram->setRowCount(states.size());
    ui->tableWidgetProgram->setColumnCount(newAlphabet.size());

    QStringList horizontalHeaders;
    for (QChar ch : newAlphabet)
        horizontalHeaders << QString(ch);

    ui->tableWidgetProgram->setHorizontalHeaderLabels(horizontalHeaders);
    ui->tableWidgetProgram->setVerticalHeaderLabels(states);
    ui->tableWidgetProgram->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Если было только расширение
    if (onlyExpansion)
    {
        for (int row = 0; row < states.size(); ++row)
        {
            QString state = states[row];

            for (int col = 0; col < newAlphabet.size(); ++col)
            {
                QString key = state + "|" + QString(newAlphabet[col]);
                if (!oldData.contains(key))
                    continue;

                ui->tableWidgetProgram->setItem(row, col, new QTableWidgetItem(oldData[key]));
            }
        }
    }
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
    // Значения по умолчанию
    cmd.writeSymbol = readSymbol;
    cmd.move = 'S';
    cmd.nextState = currentState;
    cmd.isValid = false;

    if (parts.size() == 1)
    {
        QString one = parts[0].trimmed();

        // Движение
        if (isMoveCommand(one))
        {
            cmd.move = one.toUpper()[0];
            cmd.isValid = true;
            return true;
        }

        // Остановка
        if (one == "!")
        {
            cmd.nextState = "!";
            cmd.move = 'S';
            cmd.writeSymbol = readSymbol;
            cmd.isValid = true;
            return true;
        }

        // Просто переписать символ
        if (one.size() == 1)
        {
            cmd.writeSymbol = one[0];
            cmd.move = 'S';
            cmd.nextState = currentState;
            cmd.isValid = true;
            return true;
        }

        // Просто состояние
        cmd.nextState = one;
        cmd.isValid = !cmd.nextState.isEmpty();
        return cmd.isValid;
    }

    if (parts[0].trimmed() == "!")
    {
        cmd.nextState = "!";
        cmd.move = 'S';
        cmd.writeSymbol = readSymbol;
        cmd.isValid = true;
        return true;
    }

    if (parts.size() == 2)
    {
        if (isMoveCommand(parts[0])) // Сдвиг, состояние
        {
            cmd.move = parts[0].trimmed().toUpper()[0];
            cmd.nextState = parts[1].isEmpty() ? currentState : parts[1];
            cmd.isValid = true;
            return true;
        }

        if (parts[0].size() == 1 && isMoveCommand(parts[1])) // Символ, сдвиг
        {
            cmd.writeSymbol = parts[0][0];
            cmd.move = parts[1].trimmed().toUpper()[0];
            cmd.nextState = currentState;
            cmd.isValid = true;
            return true;
        }

        if (parts[0].size() == 1) // Символ, состояние
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
    m_machine->clearProgram(); // Очищает старую программу

    QStringList states = generateStates(m_stateCount); // Создание списка состояний
    m_machine->setStates(states);

    QList<QChar> alphabet = sortedAlphabetList(); // Список символов

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
    // Массив указателей
    QLabel* labels[11] = {
        ui->labelCell0, ui->labelCell1, ui->labelCell2,
        ui->labelCell3, ui->labelCell4, ui->labelCell5,
        ui->labelCell6, ui->labelCell7, ui->labelCell8,
        ui->labelCell9, ui->labelCell10
    };

    // Индексы ячеек
    for (int i = 0; i < 11; ++i)
    {
        int tapeIndex = m_viewOffset + i;
        // Запись символа в ячейку
        labels[i]->setText(QString(m_machine->symbolAt(tapeIndex)));

        if (i == m_caretScreenIndex)
        {
            labels[i]->setStyleSheet("background-color: rgb(180, 220, 255); color: black; border: 2px solid blue;");
        }
        else
        {
            labels[i]->setStyleSheet("background-color: white; color: black; border: 1px solid black;");
        }
    }
}

// Обновляет только надписи состояния, скорости и подсветку строки
void SecondWindow::updateUiTextOnly()
{
    ui->labelCurrentState->setText("Состояние: " + m_machine->currentState());
    ui->labelSpeed->setText("Скорость: " + QString::number(m_stepIntervalMs) + " мс");
    highlightCurrentStateRow();
}

// Полностью обновляет интерфейс
void SecondWindow::updateUi()
{
    updateUiTextOnly();
    updateTapeLabels();
}

// Подсвет текущего состояния
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

// Блокировка интерфейса
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

// Когда машина остановилась
void SecondWindow::onMachineStopped(const QString &reason)
{
    m_timer->stop();
    m_machine->setRunning(false);
    m_stepInProgress = false;
    m_pendingDirection = 0;

    m_caretAnimation->stop();

    setEditingEnabled(true);
    updateUiTextOnly();
    updateTapeLabels();

    QMessageBox::information(this, "Остановка", reason);
}
// Анимация головки
void SecondWindow::placeCaretOverCell(int index)
{
    QLabel* labels[11] = {
        ui->labelCell0, ui->labelCell1, ui->labelCell2,
        ui->labelCell3, ui->labelCell4, ui->labelCell5,
        ui->labelCell6, ui->labelCell7, ui->labelCell8,
        ui->labelCell9, ui->labelCell10
    };

    QLabel *cell = labels[index];

    QPoint cellTopLeft = cell->mapTo(ui->frameTape, QPoint(0, 0));

    QPoint p = ui->labelHeadArrow->pos();
    p.setX(cellTopLeft.x() + cell->width() / 2 - ui->labelHeadArrow->width() / 2);
    ui->labelHeadArrow->move(p);
}
// Анимация головки до нужной ячейки
void SecondWindow::animateCaretToCell(int newIndex)
{
    QLabel* labels[11] = {
        ui->labelCell0, ui->labelCell1, ui->labelCell2,
        ui->labelCell3, ui->labelCell4, ui->labelCell5,
        ui->labelCell6, ui->labelCell7, ui->labelCell8,
        ui->labelCell9, ui->labelCell10
    };

    QLabel *targetCell = labels[newIndex];

    QPoint targetTopLeft = targetCell->mapTo(ui->frameTape, QPoint(0, 0));

    QPoint startPos = ui->labelHeadArrow->pos();
    QPoint endPos = startPos;
    endPos.setX(targetTopLeft.x() + targetCell->width() / 2 - ui->labelHeadArrow->width() / 2);

    m_caretAnimation->stop();
    m_caretAnimation->setDuration(m_stepIntervalMs);
    m_caretAnimation->setStartValue(startPos);
    m_caretAnimation->setEndValue(endPos);
    m_caretAnimation->start();
}

// Сдвиг ленты
void SecondWindow::finishStepAnimation()
{
    m_caretScreenIndex += m_pendingDirection;

    int shift = (11 * 2) / 3;
    if (m_caretScreenIndex >= 10)
    {
        m_viewOffset += shift;
        m_caretScreenIndex -= shift;
        updateTapeLabels();
        placeCaretOverCell(m_caretScreenIndex);
    }
    else if (m_caretScreenIndex <= 0)
    {
        m_viewOffset -= shift;
        m_caretScreenIndex += shift;
        updateTapeLabels();
        placeCaretOverCell(m_caretScreenIndex);
    }
    else
    {
        placeCaretOverCell(m_caretScreenIndex);
    }

    m_pendingDirection = 0;
    m_stepInProgress = false;

    updateUiTextOnly();
    updateTapeLabels();
}

