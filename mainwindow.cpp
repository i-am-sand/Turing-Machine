#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "secondwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_secondWindow(new SecondWindow(this))
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QSet<QChar> MainWindow::parseAlphabet(const QString &text) const
{
    QSet<QChar> result;
    for (QChar ch : text) {
        if (!ch.isSpace())
            result.insert(ch);
    }
    return result;
}


void MainWindow::on_pushButton_clicked()
{
    QString tapeText = ui->lineEdit->text();
    QString extraText = ui->lineEdit_2->text();

    QSet<QChar> tapeAlphabet = parseAlphabet(tapeText);
    QSet<QChar> extraAlphabet = parseAlphabet(extraText);

    if (tapeAlphabet.isEmpty()) {
        QMessageBox::warning(this, "Ошибка",
                             "Алфавит ленты не должен быть пустым.");
        return;
    }

    m_secondWindow->setAlphabets(tapeAlphabet, extraAlphabet);
    m_secondWindow->show();
    m_secondWindow->raise();
    m_secondWindow->activateWindow();

    this->hide();
}


