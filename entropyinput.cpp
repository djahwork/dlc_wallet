#include "entropyinput.h"


EntropyInput::EntropyInput(QWidget *parent) : QDialog(parent), remainingBits(128) {
    setWindowTitle("Enter 128 Bits");

    QLabel *label = new QLabel("Enter a sequence of 128 bits (0s and 1s):");
    counterLabel = new QLabel(QString("Remaining: %1 bits").arg(remainingBits));
    lineEdit = new QLineEdit;
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    lineEdit->installEventFilter(this);

    connect(buttonBox, &QDialogButtonBox::accepted, [this] {
        QString input = lineEdit->text();
        if (input.length() != 128 || !input.contains('0') || !input.contains('1')) {
            QMessageBox::warning(this, "Invalid Input", "Please enter exactly 128 bits (0s and 1s).");
        } else {
            emit inputAccepted(input);
            accept();
        }
    });

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(label);
    mainLayout->addWidget(counterLabel);
    mainLayout->addWidget(lineEdit);
    mainLayout->addWidget(buttonBox);

    setFixedSize(400, 200);  // Set fixed size for the dialog
}

void EntropyInput::updateCounter() {
    remainingBits -= 1;
    counterLabel->setText(QString("Remaining: %1 bits").arg(remainingBits));
}
