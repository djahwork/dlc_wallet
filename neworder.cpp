#include "neworder.h"
#include "curlrpc.h"
#include <QLabel>

NewOrder::NewOrder(const string& product, const vector<Bip32Wallet*> wallets, QWidget *parent) : QWidget(parent), product(product) {

    string rpc_url = "http://localhost:8000";
    json_curl_client = new CurlRPC(rpc_url, "", "");

    this->resize(400, 500);

    layout = new QVBoxLayout(this);
    for(const auto& wallet : wallets){
        for(const auto& [txid, vect_utxo] : wallet->get_all_utxos()){
            for(const auto& utxo : vect_utxo){
                pubkeys.push_back(
                    QString("%1 (%2 btc)").arg(
                        Bip32Wallet::key_to_string(utxo.key, false),
                        QString::number(utxo.amount)
                    )
                );
            }
        }
    }
    select_pubkey = new QComboBox(this);
    select_pubkey->addItems(pubkeys);
    select_way = new QComboBox(this);
    select_way->addItems({"Buy", "Sell"});
    select_underlying = new QComboBox(this);
    select_underlying->addItems({"bitcoin"});
    select_currency = new QComboBox(this);
    select_currency->addItems({"usd"});
    edit_strike = new QLineEdit(this);
    edit_strike->setPlaceholderText("Enter Stike");
    edit_collateral = new QLineEdit(this);
    edit_collateral->setPlaceholderText("Enter Collateral");
    edit_price = new QLineEdit(this);
    edit_price->setPlaceholderText("Enter Price");
    button_layout = new QHBoxLayout;
    cancel = new QPushButton("Cancel", this);
    send = new QPushButton("Send", this);
    button_layout->addWidget(cancel, 0, Qt::AlignRight);
    button_layout->addWidget(send, 0, Qt::AlignRight);

    connect(cancel, &QPushButton::clicked, this, &QWidget::close);

    layout->addWidget(new QLabel("Pubkey", this));
    layout->addWidget(select_pubkey);
    layout->addWidget(new QLabel("Order Type", this));
    layout->addWidget(select_way);
    layout->addWidget(new QLabel("Underlying", this));
    layout->addWidget(select_underlying);
    layout->addWidget(new QLabel("Currency", this));
    layout->addWidget(select_currency);
    layout->addWidget(new QLabel("Strike", this));
    layout->addWidget(edit_strike);
    layout->addWidget(new QLabel("Collateral", this));
    layout->addWidget(edit_collateral);

    if(product == "forward"){
        setWindowTitle("New future contract");
    } else if(product == "vanillacall"){
        setWindowTitle("New call contract");
        layout->addWidget(new QLabel("Price", this));
        layout->addWidget(edit_price);
    } else {
        setWindowTitle("New put contract");
        layout->addWidget(new QLabel("Price", this));
        layout->addWidget(edit_price);
    }

    layout->addLayout(button_layout);

    connect(send, &QPushButton::clicked, this, [this, product, wallets](){
        string tx_txid, fund_address, change_address;
        string pubkey = select_pubkey->currentText().split(u' ').at(0).toStdString();
        for(const auto& wallet : wallets){
            for(const auto& [txid, vect_utxo] : wallet->get_all_utxos()){
                for(const auto& utxo : vect_utxo){
                    if(pubkey == Bip32Wallet::key_to_string(utxo.key, false)){
                        tx_txid = utxo.txid;
                        fund_address = Bip32Wallet::get_bip32_p2pkh_address(wallet->get_all_derived_ext_keys().back()).toStdString();
                        change_address = Bip32Wallet::get_bip32_p2pkh_address(wallet->get_all_derived_int_keys().back()).toStdString();
                    }
                }
            }
        }
        qDebug() << json_curl_client->new_order(
            pubkey,
            tx_txid,
            fund_address,
            change_address,
            select_way->currentText().toStdString(),
            product,
            select_underlying->currentText().toStdString(),
            select_currency->currentText().toStdString(),
            edit_strike->text().toStdString(),
            edit_collateral->text().toStdString(),
            edit_price->text().toStdString()
            );
        emit close();
    });
}

NewOrder::NewOrder(const Order& order, const vector<Bip32Wallet*> wallets, QWidget *parent) : QWidget(parent), product(order.product) {

    string rpc_url = "http://localhost:8000";
    json_curl_client = new CurlRPC(rpc_url, "", "");

    for(const auto& wallet : wallets){
        for(const auto& [txid, vect_utxo] : wallet->get_all_utxos()){
            for(const auto& utxo : vect_utxo){
                pubkeys.push_back(
                    QString("%1 (%2 btc)").arg(
                        Bip32Wallet::key_to_string(utxo.key, false),
                        QString::number(utxo.amount)
                        )
                    );
            }
        }
    }

    setWindowTitle(QString("Take %1 contract").arg(QString::fromStdString(product)));

    layout = new QVBoxLayout(this);
    list_details = new QListWidget;
    select_pubkey = new QComboBox(this);
    select_pubkey->addItems(pubkeys);
    edit_collateral = new QLineEdit(this);
    edit_collateral->setPlaceholderText("Enter Collateral");
    button_layout = new QHBoxLayout;
    cancel = new QPushButton("Cancel", this);
    send = new QPushButton("Send", this);
    button_layout->addWidget(cancel, 0, Qt::AlignRight);
    button_layout->addWidget(send, 0, Qt::AlignRight);

    layout->addWidget(new QLabel("Contract details", this));
    layout->addWidget(list_details);
    list_details->addItem(new QListWidgetItem("Product: " + QString::fromStdString(order.product)));
    list_details->addItem(new QListWidgetItem("Counterparty side: " + QString::fromStdString(order.way)));
    if(order.way == "Buy"){
        list_details->addItem(new QListWidgetItem("Your side: Sell"));
    } else {
        list_details->addItem(new QListWidgetItem("Your side: Buy"));
    }
    list_details->addItem(new QListWidgetItem("Underlying: " + QString::fromStdString(order.underlying)));
    list_details->addItem(new QListWidgetItem("Currency: " + QString::fromStdString(order.currency)));
    list_details->addItem(new QListWidgetItem("Strike: " + QString::fromStdString(order.strike)));
    list_details->addItem(new QListWidgetItem("Price: " + QString::fromStdString(order.price)));
    layout->addWidget(new QLabel("Pubkey", this));
    layout->addWidget(select_pubkey);
    layout->addWidget(new QLabel("Collateral", this));
    layout->addWidget(edit_collateral);
    layout->addLayout(button_layout);

    connect(cancel, &QPushButton::clicked, this, &QWidget::close);
    connect(send, &QPushButton::clicked, this, [this, order, wallets](){
        string tx_txid, fund_address, change_address;
        string pubkey = select_pubkey->currentText().split(u' ').at(0).toStdString();
        for(const auto& wallet : wallets){
            for(const auto& [txid, vect_utxo] : wallet->get_all_utxos()){
                for(const auto& utxo : vect_utxo){
                    if(pubkey == Bip32Wallet::key_to_string(utxo.key, false)){
                        tx_txid = utxo.txid;
                        fund_address = Bip32Wallet::get_bip32_p2pkh_address(wallet->get_all_derived_ext_keys().back()).toStdString();
                        change_address = Bip32Wallet::get_bip32_p2pkh_address(wallet->get_all_derived_int_keys().back()).toStdString();
                    }
                }
            }
        }
        qDebug() << json_curl_client->take_order(
            order.id,
            select_pubkey->currentText().toStdString(),
            edit_collateral->text().toStdString(),
            tx_txid,
            fund_address,
            change_address
        );
        emit close();
    });
}

NewOrder::~NewOrder(){
    delete json_curl_client;
}
