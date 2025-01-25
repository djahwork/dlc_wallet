#ifndef NEWORDER_H
#define NEWORDER_H

#include <QWidget>
#include <QCloseEvent>
#include "curlrpc.h"
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include "bip32wallet.h"

using namespace std;

class NewOrder : public QWidget
{
    Q_OBJECT
public:
    NewOrder(const string& product, const vector<Bip32Wallet*> wallets, QWidget *parent = nullptr);
    NewOrder(const Order& order, const vector<Bip32Wallet*> wallets, QWidget *parent = nullptr);
    ~NewOrder();
private:
    string product;
    CurlRPC *json_curl_client;
    QVBoxLayout *layout;
    QHBoxLayout *button_layout;
    QComboBox *select_pubkey, *select_way, *select_underlying, *select_currency;
    QLineEdit *edit_strike, *edit_collateral, *edit_price;
    QListWidget *list_details ;
    QPushButton *cancel, *send;
    QStringList pubkeys = {};
protected:
    void closeEvent(QCloseEvent *event) override {
        emit closed();
        event->accept();
    }
signals:
    void closed();
};

#endif // NEWORDER_H
