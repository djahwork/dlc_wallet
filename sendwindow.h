#ifndef SENDWINDOW_H
#define SENDWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QCloseEvent>
#include <QDoubleValidator>
#include <QListWidget>
#include <QListWidgetItem>
#include "bip32wallet.h"
#include "curlrpc.h"
#include <wally_transaction.h>
#include <wally_address.h>

class SendWindow : public QWidget {
    Q_OBJECT
public:
    SendWindow(QList<QListWidgetItem*>& selected_items, Bip32Wallet *wallet, CurlRPC *rpc_client, QWidget *parent = nullptr);
    ~SendWindow();
private:
    vector<UTXO> utxos = {};
    double amount = 0;
    unsigned char change_script[WALLY_ADDRESS_PUBKEY_MAX_LEN];
    unsigned char recipient_script[WALLY_ADDRESS_PUBKEY_MAX_LEN];
    size_t change_script_len;
    size_t recipient_script_len;
    wally_tx *tx;
    char *hex_str;
    QVBoxLayout *layout;
    QHBoxLayout *button_layout;
    Bip32Wallet *wallet;
    CurlRPC *rpc_client;
    QDoubleSpinBox *send_amount, *fee_amount;
    QListWidget *list_txid, *list_tx;
    QLineEdit *send_address;
    vector<UTXO> input_utxos = {};
    QPushButton *build, *sign, *send;

    vector<UTXO> get_selected_utxo(QList<QListWidgetItem*>& selected_items);
    double get_amount_utxos(const vector<UTXO> utxos);
private slots:
    void update_addresses(const QString &text);
    void build_tx();
    void sign_tx();
    void send_tx();
protected:
    void closeEvent(QCloseEvent *event) override {
        emit closed();
        event->accept();
    }
signals:
    void closed();
};

#endif // SENDWINDOW_H
