#include "sendwindow.h"
#include <QLabel>
#include <QListWidgetItem>
#include <wally_core.h>
#include <wally_crypto.h>
#include <wally_address.h>
#include <wally_bip32.h>
#include <wally_core.h>
#include <wally_transaction.h>
#include <wally_script.h>
#include <wally_address.h>
#include "wally_psbt.h"
#include <vector>
#include <QDebug>
#include "curlrpc.h"

bool hex_to_bytes(const string &hex, unsigned char *bytes, size_t len) {
    if (hex.size() != len * 2) return false;
    for (size_t i = 0; i < len; ++i) {
        sscanf(hex.substr(i * 2, 2).c_str(), "%2hhx", &bytes[i]);
    }
    return true;
}

string reverse_txid(const string &txid) {
    string reversed;
    for (size_t i = 0; i < txid.size(); i += 2) {
        reversed.insert(0, txid.substr(i, 2));
    }
    return reversed;
}

SendWindow::SendWindow(QList<QListWidgetItem*>& selected_items, Bip32Wallet *wallet, CurlRPC *rpc_client, QWidget *parent)
    : QWidget(parent), wallet(wallet), rpc_client(rpc_client) {

    utxos = get_selected_utxo(selected_items);
    amount = get_amount_utxos(utxos);

    setWindowTitle("Send Window");
    layout = new QVBoxLayout(this);
    list_txid = new QListWidget;
    list_tx = new QListWidget;
    send_amount = new QDoubleSpinBox(this);
    send_amount->setRange(0.0, amount);
    send_amount->setDecimals(8);
    fee_amount = new QDoubleSpinBox(this);
    fee_amount->setRange(0.0, amount);
    fee_amount->setDecimals(8);
    send_address = new QLineEdit(this);
    send_address->setPlaceholderText("Enter send address here");
    button_layout = new QHBoxLayout;
    build = new QPushButton("Build Tx", this);
    sign = new QPushButton("Sign Tx", this);
    send = new QPushButton("Send", this);
    button_layout->addWidget(build, 0, Qt::AlignRight);
    button_layout->addWidget(sign, 0, Qt::AlignRight);
    button_layout->addWidget(send, 0, Qt::AlignRight);

    layout->addWidget(new QLabel("Send new transaction", this));

    if(!selected_items.empty()){
        layout->addWidget(new QLabel("UTXOs to use", this));
        layout->addWidget(list_txid);
        for(const UTXO& utxo : utxos){
            list_txid->addItem(new QListWidgetItem(
                QString("%1 #%2").arg(QString::fromStdString(utxo.txid), QString::number(utxo.vout))
            ));
        }
    }

    layout->addWidget(new QLabel(QString("Amount to send (max %1 btc)").arg(amount), this));
    layout->addWidget(send_amount);
    layout->addWidget(send_address);
    layout->addWidget(new QLabel(QString("Fee to pay (max %1 btc)").arg(amount), this));
    layout->addWidget(fee_amount);
    layout->addWidget(list_tx);
    layout->addLayout(button_layout);

    connect(send_address, &QLineEdit::textChanged, this, &SendWindow::update_addresses);
    connect(build, &QPushButton::clicked, this, &SendWindow::build_tx);
    connect(sign, &QPushButton::clicked, this, &SendWindow::sign_tx);
    connect(send, &QPushButton::clicked, this, &SendWindow::send_tx);

    setLayout(layout);
}

SendWindow::~SendWindow() {
    wally_free_string(hex_str);
    wally_tx_free(tx);
}

vector<UTXO> SendWindow::get_selected_utxo(QList<QListWidgetItem*>& selected_items){
    map<string, vector<UTXO>> map_utxos = wallet->get_all_utxos();
    vector<UTXO> utxos;
    if(!selected_items.empty()){
        for(QListWidgetItem *item : selected_items){
            uint32_t vout = item->text().split(u' ').at(1).split(u'#').at(1).toUInt();
            string item_txid = item->text().split(u' ').at(0).toStdString();
            for(const UTXO& utxo : map_utxos[item_txid]){
                if(utxo.vout == vout){
                    utxos.push_back(utxo);
                    input_utxos.push_back(utxo);
                }
            }
        }
    } else {
        for(const auto& [txid, vect_utxos] : map_utxos){
            utxos.insert(utxos.end(), vect_utxos.begin(), vect_utxos.end());
        }
    }
    sort(utxos.begin(), utxos.end(), [](const UTXO &a, const UTXO &b) {
        return a.amount > b.amount;
    });
    return utxos;
}

double SendWindow::get_amount_utxos(const vector<UTXO> utxos){
    double value = 0;
    for(const UTXO& utxo : utxos){
        value = value + utxo.amount;
    }
    return value;
}

void SendWindow::update_addresses(const QString &text){
    string std_addr = send_address->text().toStdString();

    wally_addr_segwit_n_to_bytes(
        std_addr.c_str(), std_addr.c_str() ? strlen(std_addr.c_str()) : 0, "tb", "tb" ? strlen("tb") : 0,
        0, recipient_script, sizeof(recipient_script), &recipient_script_len
    );

    std_addr = Bip32Wallet::get_bip32_p2pkh_address(wallet->get_all_derived_int_keys().back()).toStdString();

    wally_addr_segwit_n_to_bytes(
        std_addr.c_str(), std_addr.c_str() ? strlen(std_addr.c_str()) : 0, "tb", "tb" ? strlen("tb") : 0,
        0, change_script, sizeof(change_script), &change_script_len
    );

    list_tx->clear();
}

void SendWindow::build_tx(){
    uint64_t to_send = static_cast<uint64_t>(send_amount->value() * 100000000.0);
    uint64_t fee = static_cast<uint64_t>(fee_amount->value() * 100000000.0);

    double input_amount = 0;
    if(input_utxos.empty()){
        for(size_t i = 0; i < utxos.size(); ++i){
            if(input_amount < send_amount->value() + fee_amount->value()){
                input_utxos.push_back(utxos[i]);
                input_amount += utxos[i].amount;
            } else {
                break;
            }
        }
    } else {
        input_amount = amount;
    }

    uint64_t available_amount = static_cast<uint64_t>(input_amount * 100000000.0);
    uint64_t change_amount = available_amount - to_send - fee;

    size_t num_inputs = input_utxos.size();
    size_t num_outputs = 2;
    if(wally_tx_init_alloc(WALLY_TX_VERSION_2, 0, num_inputs, num_outputs, &tx) != WALLY_OK){
        qDebug() << "Error while init tx.";
    }

    for(uint32_t i = 0; i < input_utxos.size(); ++i){
        struct wally_tx_input *input = nullptr;
        unsigned char txid[WALLY_TXHASH_LEN];
        if (!hex_to_bytes(reverse_txid(input_utxos[i].txid), txid, WALLY_TXHASH_LEN)) {
            qDebug() << "Invalid txid format: " << input_utxos[i].txid;
        }
        if(wally_tx_input_init_alloc(txid, WALLY_TXHASH_LEN, input_utxos[i].vout, WALLY_TX_SEQUENCE_FINAL, NULL, 0, NULL, &input) != WALLY_OK){
            qDebug() << "Error while allocating input.";
        }
        if(wally_tx_add_input(tx, input) != WALLY_OK){
            qDebug() << "Error adding input.";
        }
    }

    struct wally_tx_output *recipient_output = nullptr;
    if(wally_tx_output_init_alloc(to_send, recipient_script, recipient_script_len, &recipient_output)!= WALLY_OK){
        qDebug() << "Error while allocating recipient output.";
    }
    if(wally_tx_add_output(tx, recipient_output) != WALLY_OK){
        qDebug() << "Error while adding recipient output.";
    }

    struct wally_tx_output *change_output = nullptr;
    if(wally_tx_output_init_alloc(change_amount, change_script, change_script_len, &change_output) != WALLY_OK){
        qDebug() << "Error while allocating change output.";
    }
    if(wally_tx_add_output(tx, change_output) != WALLY_OK){
        qDebug() << "Error while adding change output.";
    }

    if (wally_tx_to_hex(tx, 1, &hex_str) != WALLY_OK) {
        qDebug() << "Failed to convert transaction to hex";
    }

    string result(hex_str);

    list_tx->addItem(new QListWidgetItem(QString("Transaction created successfully (unsigned):")));
    list_tx->addItem(new QListWidgetItem(QString("Version: %1").arg(QString::number(tx->version))));
    list_tx->addItem(new QListWidgetItem(QString("Locktime: %1").arg(QString::number(tx->locktime))));
    list_tx->addItem(new QListWidgetItem(QString("Inputs: %1").arg(QString::number(tx->num_inputs))));
    list_tx->addItem(new QListWidgetItem(QString("Outputs: %1").arg(QString::number(tx->num_outputs))));

    list_tx->addItem(new QListWidgetItem(QString("Transaction Hex: %1").arg(QString::fromStdString(result))));
    qDebug() << result;
}

void SendWindow::sign_tx(){
    for (size_t i = 0; i < tx->num_inputs; ++i) {

        unsigned char priv_key[EC_PRIVATE_KEY_LEN];
        std::memcpy(priv_key, input_utxos[i].key.priv_key + 1, EC_PRIVATE_KEY_LEN);

        unsigned char script_pubkey[25];
        size_t script_pubkey_len = sizeof(script_pubkey);

        script_pubkey[0] = 0x76;
        script_pubkey[1] = 0xa9;
        script_pubkey[2] = 0x14;
        wally_hash160(input_utxos[i].key.pub_key, sizeof(input_utxos[i].key.pub_key), script_pubkey + 3, 20);
        script_pubkey[23] = 0x88;
        script_pubkey[24] = 0xac;

        unsigned char sighash[WALLY_TXHASH_LEN];
        if (wally_tx_get_btc_signature_hash(
                tx, i, script_pubkey, script_pubkey_len, static_cast<uint64_t>(input_utxos[i].amount * 100000000.0),
                WALLY_SIGHASH_ALL, WALLY_TX_FLAG_USE_WITNESS, sighash, WALLY_TXHASH_LEN) != WALLY_OK
            ) {
            qDebug() << "Failed to get signature hash for input " << i;
        }

        // Create the signature
        unsigned char signature[EC_SIGNATURE_LEN];
        size_t sig_len = EC_SIGNATURE_LEN;
        if (wally_ec_sig_from_bytes(priv_key, EC_PRIVATE_KEY_LEN, sighash, WALLY_TXHASH_LEN, EC_FLAG_ECDSA, signature, sig_len) != WALLY_OK) {
            qDebug() << "Failed to generate signature for input " << i;
        }

        // Normalize the signature to enforce low-S value
        unsigned char normalized_sig[EC_SIGNATURE_LEN];
        if (wally_ec_sig_normalize(signature, sig_len, normalized_sig, EC_SIGNATURE_LEN) != WALLY_OK) {
            qDebug() << "Failed to normalize the ECDSA signature";
        }

        // Convert to DER format
        unsigned char der_sig[EC_SIGNATURE_DER_MAX_LEN];
        size_t der_sig_len;
        if (wally_ec_sig_to_der(normalized_sig, EC_SIGNATURE_LEN, der_sig, sizeof(der_sig), &der_sig_len) != WALLY_OK) {
            qDebug() << "Failed to convert ECDSA signature to DER format";
        }

        // Add SIGHASH flag to the signature
        der_sig[der_sig_len++] = WALLY_SIGHASH_ALL;

        // Add the signature to the witness stack
        struct wally_tx_witness_stack *witness = nullptr;
        if (wally_tx_witness_stack_init_alloc(2, &witness) != WALLY_OK) {
            qDebug() << "Failed to allocate witness stack for input " << i;
        }

        if (wally_tx_witness_stack_add(witness, der_sig, der_sig_len) != WALLY_OK) {
            qDebug() << "Failed to add witness signature for input " << i;
        }
        if (wally_tx_witness_stack_add(witness, input_utxos[i].key.pub_key, EC_PUBLIC_KEY_LEN) != WALLY_OK) {
            qDebug() << "Failed to add witness script pub for input " << i;
        }

        // Attach the witness stack to the input
        if (wally_tx_set_input_witness(tx, i, witness) != WALLY_OK) {
            qDebug() << "Failed to set witness for input " << i;
        }
    }

    if (wally_tx_to_hex(tx, 1, &hex_str) != WALLY_OK) {
        qDebug() << "Failed to convert signed transaction to hex";
    }
    string result(hex_str);
    list_tx->addItem(new QListWidgetItem(QString("Signed Tx Hex: %1").arg(QString::fromStdString(result))));
    qDebug() << result;
}

void SendWindow::send_tx(){
    string response = rpc_client->sendrawtransaction(hex_str);
    list_tx->addItem(new QListWidgetItem(QString::fromStdString(response)));
    qDebug() << response;
}
