#include "curlrpc.h"
#include <iostream>
#include <QDebug>
#include <regex>
#include "bip32wallet.h"

CurlRPC::CurlRPC(const std::string& url, const std::string& user, const std::string& password)
    : rpc_url(url), rpc_user(user), rpc_password(password) {}

size_t CurlRPC::write_callback(void* contents, size_t size, size_t nmemb, string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

string CurlRPC::send_request(const string& method, const string& params) {
    CURL* curl = curl_easy_init();
    string response_data;

    if (curl) {
        string request_body = "{\"jsonrpc\":\"1.0\",\"id\":\"curlrpc\",\"method\":\"" + method + "\",\"params\":" + params + "}";

        curl_easy_setopt(curl, CURLOPT_URL, rpc_url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, (rpc_user + ":" + rpc_password).c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "cURL error: " << curl_easy_strerror(res) << endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
    } else {
        cerr << "Error initializing cURL." << endl;
    }

    return response_data;
}

string CurlRPC::send_request_json(const string& endpoint, const string& json_data) {
    CURL* curl = curl_easy_init();
    struct curl_slist* headers = nullptr;
    string response_data;

    rpc_url = rpc_url + "/" + endpoint;

    if (curl) {
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, rpc_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        if(json_data != ""){
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "cURL error: " << curl_easy_strerror(res) << endl;
        }

        // Clean up
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        cerr << "Error initializing cURL." << endl;
    }

    return response_data;
}

string CurlRPC::get_best_block_hash() {
    return send_request("getbestblockhash", "[]");
}

string CurlRPC::get_block(const string& block_hash, bool verbose) {
    string params = "[\"" + block_hash + "\", " + (verbose ? "true" : "false") + "]";
    return send_request("getblock", params);
}

string CurlRPC::get_blockchain_info() {
    return send_request("getblockchaininfo", "[]");
}

string CurlRPC::get_block_count() {
    return send_request("getblockcount", "[]");
}

string CurlRPC::get_block_hash(int height) {
    string params = "[" + to_string(height) + "]";
    return send_request("getblockhash", params);
}

string CurlRPC::get_block_header(const string& block_hash, bool verbose) {
    string params = "[\"" + block_hash + "\", " + (verbose ? "true" : "false") + "]";
    return send_request("getblockheader", params);
}

string CurlRPC::get_tx_out(const string& txid, int vout_number) {
    string params = "[\"" + txid + "\", " + to_string(vout_number) + "]";
    return send_request("gettxout", params);
}

string CurlRPC::get_rpc_info() {
    return send_request("getrpcinfo", "[]");
}

string CurlRPC::get_up_time() {
    return send_request("uptime", "[]");
}

vector<UTXO> CurlRPC::scan_tx_outset(vector<ext_key> keys) {
    string str_keys = "";
    map<string, ext_key> map_key;
    for(int i = 0; i < keys.size(); ++i){
        string str_key = Bip32Wallet::key_to_string(keys[i], false).toStdString();
        str_keys = str_keys + "{\"desc\":\"wpkh(" + str_key + ")\",\"range\":[0,100]}";
        if(i < keys.size()-1){
            str_keys = str_keys + ",";
        }
        map_key[str_key] = keys[i];
    }
    string json_response = send_request("scantxoutset", "[\"start\", [" + str_keys + "]]");
    vector<UTXO> unspents;
    string desc;
    try {
        auto json = nlohmann::json::parse(json_response);

        if (json.contains("result")) {
            auto result = json["result"];

            if (result.contains("unspents")) {
                for (const auto& utxoJson : result["unspents"]) {
                    desc = utxoJson["desc"].get<string>();
                    regex pubkey_regex("\\[.*\\]([0-9a-fA-F]+)");
                    smatch match;
                    if (!regex_search(desc, match, pubkey_regex) || match.size() <= 1) {
                        cerr << "Public key not found in the descriptor!" << endl;
                    }
                    UTXO utxo = {
                        utxoJson["txid"].get<string>(),
                        utxoJson["vout"].get<uint32_t>(),
                        utxoJson["scriptPubKey"].get<string>(),
                        utxoJson["amount"].get<double>(),
                        utxoJson["height"].get<int>(),
                        map_key[match[1].str()]
                    };
                    unspents.push_back(utxo);
                }
            }
        }
    } catch (const exception& e) {
        cerr << "Erreur lors du parsing JSON: " << e.what() << endl;
    }

    return unspents;
}

string CurlRPC::sendrawtransaction(const string& tx_hex){
    string json_response = send_request("sendrawtransaction", "[\"" + tx_hex + "\"]");

    return json_response;
}

string CurlRPC::new_order(
    const string& pubkey, const string& txid, const string& fund_address, const string& change_address,
    const string& way, const string& product, const string& underlying,
    const string& currency, const string& strike, const string& collateral, const string& price
){
    nlohmann::json payload = {
        {"pubkey", pubkey},
        {"txid", txid},
        {"fund_address", fund_address},
        {"change_address", change_address},
        {"status", "pending"},
        {"way", way},
        {"product", product},
        {"underlying", underlying},
        {"currency", currency},
        {"strike", strike},
        {"collateral", collateral},
        {"price", price}
    };

    string json_data = payload.dump();
    string response = send_request_json("api/contract/new", json_data);

    return response;
}

string CurlRPC::take_order(
    int contract_id, const string& pubkey, const string& collateral,
    const string& txid, const string& fund_address, const string& change_address
){
    nlohmann::json payload = {
        {"contract_id", contract_id},
        {"pubkey", pubkey},
        {"collateral", collateral},
        {"txid", txid},
        {"fund_address", fund_address},
        {"change_address", change_address}
    };

    string json_data = payload.dump();
    string response = send_request_json("api/contract/take", json_data);

    return response;
}

vector<Order> CurlRPC::get_pending_orders(){
    string json_response = send_request_json("api/contract/pending", "");
    vector<Order> orders;
    try {
        auto json = nlohmann::json::parse(json_response);

        if (json.contains("results")) {
            for (const auto& orderJson : json["results"]) {
                Order order = {
                    orderJson["id"].get<uint32_t>(),
                    orderJson["status"].get<string>(),
                    orderJson["way"].get<string>(),
                    orderJson["product"].get<string>(),
                    orderJson["underlying"].get<string>(),
                    orderJson["currency"].get<string>(),
                    orderJson["strike"].get<string>(),
                    orderJson["price"].get<string>()
                };
                Counterpart maker = {};
                Counterpart taker = {};
                order.maker = maker;
                order.taker = taker;
                orders.push_back(order);
            }
        }
    } catch (const exception& e) {
        cerr << "Erreur lors du parsing JSON: " << e.what() << endl;
    }

    return orders;
}

vector<Order> CurlRPC::get_all_orders(){
    string json_response = send_request_json("api/contract", "");
    vector<Order> orders;
    try {
        auto json = nlohmann::json::parse(json_response);

        if (json.contains("results")) {
            for (const auto& orderJson : json["results"]) {
                Order order = {
                    orderJson["id"].get<uint32_t>(),
                    orderJson["status"].get<string>(),
                    orderJson["way"].get<string>(),
                    orderJson["product"].get<string>(),
                    orderJson["underlying"].get<string>(),
                    orderJson["currency"].get<string>(),
                    orderJson["strike"].get<string>(),
                    orderJson["price"].get<string>()
                };
                Counterpart maker = {};
                Counterpart taker = {};
                if(orderJson["role"].get<string>() == "maker"){
                    maker.id = orderJson["counterpart_id"].get<uint32_t>();
                    maker.role = orderJson["role"].get<string>();
                    maker.collateral = orderJson["collateral"].get<string>();
                    maker.pubkey = orderJson["pubkey"].get<string>();
                } else {
                    taker.id = orderJson["counterpart_id"].get<uint32_t>();
                    taker.role = orderJson["role"].get<string>();
                    taker.collateral = orderJson["collateral"].get<string>();
                    taker.pubkey = orderJson["pubkey"].get<string>();
                }
                order.maker = maker;
                order.taker = taker;
                orders.push_back(order);
            }
        }
    } catch (const exception& e) {
        cerr << "Erreur lors du parsing JSON: " << e.what() << endl;
    }

    return orders;
}
