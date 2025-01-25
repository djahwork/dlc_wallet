#ifndef BIP32WALLET_H
#define BIP32WALLET_H

#include <QString>

#include <wally_core.h>
#include <wally_crypto.h>
#include <wally_address.h>
#include <wally_bip32.h>
#include "curlrpc.h"

using namespace std;

class Bip32Wallet
{
private:
    unsigned char byte_entropy[BIP32_ENTROPY_LEN_128];
    ext_key master_key;
    ext_key key_account;
    ext_key external_account;
    ext_key internal_account;
    QString name;
    uint32_t last_child;
    vector<ext_key> external_keys;
    vector<ext_key> internal_keys;
    map<string, vector<UTXO>> utxos;
    double solde;
    uint32_t path[3];
    char *segwit;

public:
    Bip32Wallet();
    Bip32Wallet(const QString &name);
    Bip32Wallet(const QString &entropy, const QString &name);
    Bip32Wallet(const Bip32Wallet &other);

    Bip32Wallet& operator=(const Bip32Wallet &other);

    ~Bip32Wallet();

    void save_to_file() const;
    void load_from_file();
    QString get_name() const;

    void derive_next_keys();
    const vector<ext_key>& get_all_derived_ext_keys() const;
    const vector<ext_key>& get_all_derived_int_keys() const;
    const map<string, vector<UTXO>>& get_all_utxos() const;

    const ext_key& get_master_key() const;
    const uint32_t get_last_child() const;

    void add_utxos(const string& txid, const vector<UTXO>& utxos);
    void add_solde(const double amount);

    static QString key_to_string(const ext_key& key, bool is_private = true);

    static QString get_bip32_p2pkh_address(const ext_key& key);
};

#endif // BIP32WALLET_H
