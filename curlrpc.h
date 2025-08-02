#ifndef CURLRPC_H
#define CURLRPC_H

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <wally_bip32.h>

using namespace std;

struct UTXO {
    std::string txid;
    uint32_t vout;
    std::string script_pub_key;
    double amount;
    int height;
    ext_key key;
};

struct Counterpart {
    uint32_t id;
    string pubkey;
    string role;
    string collateral;
};

struct Order {
    uint32_t id;
    string status;
    string way;
    string product;
    string underlying;
    string currency;
    string strike;
    string price;
    Counterpart maker;
    Counterpart taker;
};

class CurlRPC
{
private:
    string rpc_url;
    string rpc_user;
    string rpc_password;

    // Static callback function to capture the cURL response
    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* userp);

public:
    CurlRPC(const string& url, const string& user, const string& password);

    // Function to send a JSON-RPC request
    string send_request(const string& method, const string& params);
    // Function to send a JSON request
    string send_request_json(const string& endpoint, const string& json_data);

    string get_best_block_hash();
    string get_block(const string& block_hash, bool verbose = true);
    string get_blockchain_info();
    string get_block_count();
    string get_block_hash(int height);
    string get_block_header(const string& block_hash, bool verbose = true);
    string get_tx_out(const string& txid, int vout_number);
    string get_rpc_info();
    string get_up_time();
    string sendrawtransaction(const string& tx_hex);
    vector<UTXO> scan_tx_outset(vector<ext_key> keys);

    string new_order(
        const string& pubkey, const string& txid, const string& fund_address, const string& change_address,
        const string& way, const string& product, const string& underlying,
        const string& currency, const string& strike, const string& collateral, const string& price
    );
    string take_order(
        int contract_id, const string& pubkey, const string& collateral,
        const string& txid, const string& fund_address, const string& change_address
    );
    vector<Order> get_pending_orders();
    vector<Order> get_all_orders();
};

#endif // CURLRPC_H

