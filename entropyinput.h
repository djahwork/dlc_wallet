#ifndef ENTROPYINPUT_H
#define ENTROPYINPUT_H

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QKeyEvent>

class EntropyInput : public QDialog
{
    Q_OBJECT
public:
    EntropyInput(QWidget *parent = nullptr);
    void updateCounter();

private:
    int remainingBits;
    QLabel *counterLabel;
    QLineEdit *lineEdit;

protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (object == lineEdit && event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if ((keyEvent->key() == Qt::Key_0 || keyEvent->key() == Qt::Key_1) && remainingBits > 0) {
                updateCounter();
                return false;
            } else {
                return true;
            }
        }
        return QDialog::eventFilter(object, event);
    }

signals:
    void inputAccepted(const QString &bits);

};

#endif // ENTROPYINPUT_H
