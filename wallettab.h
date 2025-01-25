#ifndef WALLETTAB_H
#define WALLETTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QDebug>
#include "bip32wallet.h"
#include "curlrpc.h"
#include "sendwindow.h"

class WalletTab : public QWidget {
    Q_OBJECT
public:
    WalletTab(Bip32Wallet* wallet, CurlRPC* rpc_client, QWidget* parent = nullptr);
    ~WalletTab();
private:
    QVBoxLayout *layout, *layout_list_address;
    QHBoxLayout *layout_address;
    QListWidget *list_ext_address, *list_int_address, *list_info, *list_txid;
    QListWidgetItem *priv, *pub, *address, *solde;
    QPushButton *new_address;
    Bip32Wallet* wallet;
    CurlRPC* rpc_client;
    SendWindow *send_window;
    void fill_txid_list();
    void add_new_addresses(int nb_addresses = 1);
private slots:
    void add_new_address();
    void load_details_address(QListWidgetItem *item);
    void copy_to_clipboard(QListWidgetItem *item);
    void open_send_window();
};

#endif // WALLETTAB_H
