#include "wallettab.h"
#include <QLabel>
#include <QClipboard>
#include <QGuiApplication>

WalletTab::WalletTab(Bip32Wallet* wallet, CurlRPC* rpc_client, QWidget* parent)
    : QWidget(parent), wallet(wallet), rpc_client(rpc_client) {

    send_window = nullptr;

    layout = new QVBoxLayout(this);
    layout_address = new QHBoxLayout;
    layout_list_address = new QVBoxLayout;

    layout_list_address->addWidget(new QLabel("External Addresses"));
    list_ext_address = new QListWidget;
    int i = 0;
    for(const auto& key : wallet->get_all_derived_ext_keys()){
        list_ext_address->addItem(
            new QListWidgetItem(QString("Address %1 : %2").arg(QString::number(i), Bip32Wallet::get_bip32_p2pkh_address(key)))
        );
        ++i;
    }
    layout_list_address->addWidget(list_ext_address);

    layout_list_address->addWidget(new QLabel("Change Addresses"));
    list_int_address = new QListWidget;
    i = 0;
    for(const auto& key : wallet->get_all_derived_int_keys()){
        list_int_address->addItem(
            new QListWidgetItem(QString("Address %1 : %2").arg(QString::number(i), Bip32Wallet::get_bip32_p2pkh_address(key)))
        );
        ++i;
    }
    layout_list_address->addWidget(list_int_address);

    layout_address->addLayout(layout_list_address);

    list_info = new QListWidget;
    priv = new QListWidgetItem;
    pub = new QListWidgetItem;
    address = new QListWidgetItem;
    solde = new QListWidgetItem;
    list_info->addItem(priv);
    list_info->addItem(pub);
    list_info->addItem(address);
    list_info->addItem(solde);
    layout_address->addWidget(list_info);

    layout->addLayout(layout_address);

    new_address = new QPushButton("New Address", this);
    layout->addWidget(new_address, 0, Qt::AlignRight);

    list_txid = new QListWidget;
    layout->addWidget(list_txid);

    QPushButton *send = new QPushButton("Send", this);
    layout->addWidget(send, 0, Qt::AlignRight);

    connect(new_address, &QPushButton::clicked, this, &WalletTab::add_new_address);
    connect(list_ext_address, &QListWidget::itemClicked, this, &WalletTab::load_details_address);
    connect(list_int_address, &QListWidget::itemClicked, this, &WalletTab::load_details_address);
    connect(list_info, &QListWidget::itemClicked, this, &WalletTab::copy_to_clipboard);
    connect(send, &QPushButton::clicked, this, &WalletTab::open_send_window);

    fill_txid_list();
    add_new_addresses(10);

    this->setLayout(layout);
}

WalletTab::~WalletTab(){
    delete send_window;
}

void WalletTab::fill_txid_list(){
    list_txid->clear();
    for(const auto& [txid, vect]: wallet->get_all_utxos()){
        for(size_t i = 0; i < vect.size(); ++i){
            list_txid->addItem(new QListWidgetItem(
                QString("%1 #%2 %3 btc").arg(
                    QString::fromStdString(txid), QString::number(vect[i].vout), QString::number(vect[i].amount)
                    )
                ));
        }
    }
}

void WalletTab::add_new_address(){
    add_new_addresses(1);
}

void WalletTab::add_new_addresses(int nb_addresses){
    try {
        vector<ext_key> ext_keys, int_keys;

        for(int i = 0; i < nb_addresses; ++i){
            wallet->derive_next_keys();
            ext_keys.push_back(wallet->get_all_derived_ext_keys().back());
            int_keys.push_back(wallet->get_all_derived_int_keys().back());
        }

        for(auto& key : ext_keys){
            list_ext_address->addItem(new QListWidgetItem(QString("Receive Address %1 : %2").arg(
                QString::number(key.child_num),
                Bip32Wallet::get_bip32_p2pkh_address(key)
            )));
        }

        for(auto& key : int_keys){
            list_int_address->addItem(new QListWidgetItem(QString("Change Address %1 : %2").arg(
                QString::number(key.child_num),
                Bip32Wallet::get_bip32_p2pkh_address(key)
            )));
        }

        vector<ext_key> keys;
        keys.insert(keys.end(), ext_keys.begin(), ext_keys.end());
        keys.insert(keys.end(), int_keys.begin(), int_keys.end());

        const vector<UTXO> new_utxos = this->rpc_client->scan_tx_outset(keys);
        for(UTXO utxo : new_utxos){
            wallet->add_solde(utxo.amount);
            wallet->add_utxos(utxo.txid, {utxo});
        }

        fill_txid_list();

    } catch (const std::exception &e) {
        qDebug() << "Error while adding new addresses:" << e.what();
    }
}

void WalletTab::load_details_address(QListWidgetItem *item){
    if(item->text().contains("Address")){

        ext_key key;
        int nb_address = stoi(item->text().split(u' ').at(2).toStdString());

        if(item->text().contains("Receive")){
            key = wallet->get_all_derived_ext_keys().at(nb_address);
        } else {
            key = wallet->get_all_derived_int_keys().at(nb_address);
        }

        priv->setText(QString(Bip32Wallet::key_to_string(key)));
        pub->setText(QString(Bip32Wallet::key_to_string(key, false)));
        address->setText(QString(Bip32Wallet::get_bip32_p2pkh_address(key)));

        map<string, vector<UTXO>> utxos = wallet->get_all_utxos();
        double tot_solde = 0;
        for(auto& [txid, vect_utxo] : utxos){
            for(auto& utxo: vect_utxo){
                if(Bip32Wallet::key_to_string(utxo.key, false) == Bip32Wallet::key_to_string(key, false)){
                    tot_solde = tot_solde + utxo.amount;
                }
            }
        }
        solde->setText(QString("Solde: %1 btc").arg(QString::number(tot_solde)));
    }
}

void WalletTab::copy_to_clipboard(QListWidgetItem *item){
    if (item) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(item->text());
        qDebug() << "Copied to clipboard:" << item->text();
    }
}

void WalletTab::open_send_window() {
    if (!send_window) {
        QList<QListWidgetItem*> selected_items = list_txid->selectedItems();
        for (QListWidgetItem* item : selected_items) {
            QString item_text = item->text();
            qDebug() << "Selected item text: " << item_text;
        }
        send_window = new SendWindow(selected_items, wallet, rpc_client, nullptr);
        send_window->show();
        connect(send_window, &SendWindow::closed, this, [this]() {
            send_window = nullptr;
        });
    }
}
