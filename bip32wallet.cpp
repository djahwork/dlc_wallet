#include "bip32wallet.h"
#include <stdexcept>
#include <ostream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <QDebug>

using namespace std;

Bip32Wallet::Bip32Wallet() {
    path[0] = BIP32_INITIAL_HARDENED_CHILD+84; // 84'
    path[1] = BIP32_INITIAL_HARDENED_CHILD+1; // 1'
    path[2] = BIP32_INITIAL_HARDENED_CHILD; // 0'
}
Bip32Wallet::Bip32Wallet(const QString &name): name(name), solde(0), last_child(0) {
    path[0] = BIP32_INITIAL_HARDENED_CHILD+84;
    path[1] = BIP32_INITIAL_HARDENED_CHILD+1;
    path[2] = BIP32_INITIAL_HARDENED_CHILD;
}
Bip32Wallet::Bip32Wallet(const QString &entropy, const QString &name) : name(name), solde(0), last_child(0) {
    path[0] = BIP32_INITIAL_HARDENED_CHILD+84;
    path[1] = BIP32_INITIAL_HARDENED_CHILD+1;
    path[2] = BIP32_INITIAL_HARDENED_CHILD;

    for (size_t i = 0; i < BIP32_ENTROPY_LEN_128; ++i) {
        unsigned char byte = 0;
        for (size_t j = 0; j < 8; ++j) {
            byte |= (entropy[i * 8 + j].digitValue() & 1) << (7 - j);
        }
        byte_entropy[i] = byte;
    }

    if(bip32_key_from_seed(byte_entropy, BIP32_ENTROPY_LEN_128, BIP32_VER_TEST_PRIVATE, 0, &master_key) != WALLY_OK) {
        throw runtime_error("Failed to create master key.");
    }

    if(bip32_key_from_parent_path(&master_key, path, 3, BIP32_FLAG_KEY_PRIVATE, &key_account) != WALLY_OK){
        throw runtime_error("Failed to create account key.");
    }

    if(bip32_key_from_parent(&key_account, 0, BIP32_FLAG_KEY_PRIVATE, &external_account) != WALLY_OK){
        throw runtime_error("Failed to create external key.");
    }

    if(bip32_key_from_parent(&key_account, 1, BIP32_FLAG_KEY_PRIVATE, &internal_account) != WALLY_OK){
        throw runtime_error("Failed to create internal key.");
    }
}
Bip32Wallet::Bip32Wallet(const Bip32Wallet &other) : name(other.name), master_key(other.master_key), solde(other.solde),
    last_child(other.last_child), key_account(other.key_account), external_account(other.external_account),
    internal_account(other.internal_account) {
    memcpy(byte_entropy, other.byte_entropy, BIP32_ENTROPY_LEN_128);
    memcpy(path, other.path, 3);
}

Bip32Wallet::~Bip32Wallet() = default;

Bip32Wallet& Bip32Wallet::operator=(const Bip32Wallet &other) {
    if (this != &other) {
        memcpy(byte_entropy, other.byte_entropy, BIP32_ENTROPY_LEN_128);
        name = other.name;
        master_key = other.master_key;
        key_account = other.key_account;
        external_account = other.external_account;
        internal_account = other.internal_account;
        last_child = other.last_child;
        solde = other.solde;
    }
    return *this;
}

QString Bip32Wallet::get_name() const {
    return name;
}

void Bip32Wallet::save_to_file() const {
    string filename = name.toStdString() + ".txt";
    ofstream file(filename);
    if (!file) {
        throw runtime_error("Failed to open file for writing: " + filename);
    }

    file << "Wallet Name: " << name.toStdString() << "\n";
    file << "Entropy: ";
    for (size_t i = 0; i < sizeof(byte_entropy); ++i) {
        file << std::hex << static_cast<int>(byte_entropy[i]) << " ";
    }
    file << "\n";

    file << "Private Key: ";
    for (size_t i = 0; i < sizeof(master_key.priv_key); ++i) {
        file << std::hex << static_cast<int>(master_key.priv_key[i]) << " ";
    }
    file << "\n";

    file << "Public Key: ";
    for (size_t i = 0; i < sizeof(master_key.pub_key); ++i) {
        file << std::hex << static_cast<int>(master_key.pub_key[i]) << " ";
    }
    file << "\n";

    file << "Chain Code: ";
    for (size_t i = 0; i < sizeof(master_key.chain_code); ++i) {
        file << std::hex << static_cast<int>(master_key.chain_code[i]) << " ";
    }
    file << "\n";

    file.close();
}

void Bip32Wallet::load_from_file() {
    string filename = name.toStdString() + ".txt";
    ifstream file(filename);
    if (!file) {
        throw runtime_error("Failed to open file: " + filename);
    }

    string line;
    bool has_private_key = false, has_public_key = false, has_chain_code = false;

    while (getline(file, line)) {
        istringstream iss(line);
        string key, value;

        if (line.find("Entropy:") != string::npos) {
            size_t pos = line.find("Entropy:") + 9;
            istringstream keyStream(line.substr(pos));
            for (size_t i = 0; i < 16; ++i) {
                int byte;
                keyStream >> hex >> byte;
                byte_entropy[i] = static_cast<unsigned char>(byte);
            }

            if(bip32_key_from_seed(byte_entropy, BIP32_ENTROPY_LEN_128, BIP32_VER_TEST_PRIVATE, 0, &master_key) != WALLY_OK) {
                throw runtime_error("Failed to create master key.");
            }

            if(bip32_key_from_parent_path(&master_key, path, 3, BIP32_FLAG_KEY_PRIVATE, &key_account) != WALLY_OK){
                throw runtime_error("Failed to create account key.");
            }

            if(bip32_key_from_parent(&key_account, 0, BIP32_FLAG_KEY_PRIVATE, &external_account) != WALLY_OK){
                throw runtime_error("Failed to create external key.");
            }

            if(bip32_key_from_parent(&key_account, 1, BIP32_FLAG_KEY_PRIVATE, &internal_account) != WALLY_OK){
                throw runtime_error("Failed to create internal key.");
            }

            qDebug() << "master key: " << this->key_to_string(master_key);
            qDebug() << "account key: " << this->key_to_string(key_account);
            qDebug() << "ext key: " << this->key_to_string(external_account, false);
            qDebug() << "int key: " << this->key_to_string(internal_account);

        }
    }

    file.close();
}

void Bip32Wallet::derive_next_keys() {
    ext_key child_ext_key, child_int_key;
    if (bip32_key_from_parent(&external_account, last_child, BIP32_FLAG_KEY_PRIVATE, &child_ext_key) != WALLY_OK) {
        throw runtime_error("Failed to derive external child key.");
    }
    if (bip32_key_from_parent(&internal_account, last_child, BIP32_FLAG_KEY_PRIVATE, &child_int_key) != WALLY_OK) {
        throw runtime_error("Failed to derive internal child key.");
    }
    external_keys.push_back(child_ext_key);
    internal_keys.push_back(child_int_key);
    ++last_child;
}

void Bip32Wallet::add_utxos(const string& txid, const vector<UTXO>& utxos) {
    if (this->utxos.find(txid) != this->utxos.end()) {
        this->utxos[txid].insert(this->utxos[txid].end(), utxos.begin(), utxos.end());
    } else {
        this->utxos[txid] = utxos;
    }
}

const uint32_t Bip32Wallet::get_last_child() const {
    return last_child;
}

const ext_key& Bip32Wallet::get_master_key() const {
    return master_key;
}

const vector<ext_key>& Bip32Wallet::get_all_derived_ext_keys() const {
    return external_keys;
}

const vector<ext_key>& Bip32Wallet::get_all_derived_int_keys() const {
    return internal_keys;
}

const map<string, vector<UTXO>>& Bip32Wallet::get_all_utxos() const {
    return utxos;
}

void Bip32Wallet::add_solde(const double amount){
    solde = solde + amount;
}

QString Bip32Wallet::key_to_string(const ext_key& key, bool is_private) {
    ostringstream oss;
    oss << hex << setfill('0');
    if (is_private) {
        for (size_t i = 0; i < sizeof(key.priv_key); ++i) {
            oss << setw(2) << static_cast<int>(key.priv_key[i]);
        }
    } else {
        for (size_t i = 0; i < sizeof(key.pub_key); ++i) {
            oss << setw(2) << static_cast<int>(key.pub_key[i]);
        }
    }
    return QString::fromStdString(oss.str());
}

QString Bip32Wallet::get_bip32_p2pkh_address(const ext_key& key) {
    char *address = nullptr;
    if (wally_bip32_key_to_addr_segwit(&key, "tb", 0, &address) != WALLY_OK) {
        throw runtime_error("Erreur: Impossible de generer l'adresse Bitcoin Testnet.");
    }
    return QString::fromStdString(address);
}
