#include "marketwindow.h"
using namespace std;

MarketWindow::MarketWindow(vector<Bip32Wallet*> wallets, QWidget *parent) : QWidget(parent), wallets(wallets) {

    string rpc_url = "http://localhost:8000";
    json_curl_client = new CurlRPC(rpc_url, "", "");
    vector<Order> pending_orders = json_curl_client->get_pending_orders();
    vector<Order> my_orders;
    for(const Order& order : pending_orders){
        for(const auto& wallet : wallets){
            if(order.maker.pubkey == Bip32Wallet::key_to_string(wallet->get_master_key(), false).toStdString()){
                my_orders.push_back(order);
                break;
            }
        }
    }

    setWindowTitle("Order Book");

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Pending Orders", this));

    QHBoxLayout *order_layout = new QHBoxLayout;
    QTableWidget *table_order = new QTableWidget(pending_orders.size(), 8, this);
    table_order->setHorizontalHeaderLabels({"ID", "Way", "Product", "Underlying", "Currency", "Strike", "Price", "Status"});
    table_order->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_order->setSelectionMode(QAbstractItemView::SingleSelection);
    order_layout->addWidget(table_order);
    layout->addLayout(order_layout);

    QHBoxLayout *button_layout = new QHBoxLayout;
    QPushButton *take = new QPushButton("Take", this);
    button_layout->addWidget(take, 0, Qt::AlignRight);
    layout->addLayout(button_layout);

    layout->addWidget(new QLabel("My orders", this));

    QHBoxLayout *my_order_layout = new QHBoxLayout;
    QTableWidget *table_my_order = new QTableWidget(my_orders.size(), 8, this);
    table_my_order->setHorizontalHeaderLabels({"ID", "Way", "Product", "Underlying", "Currency", "Strike", "Price", "Status"});
    table_my_order->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_my_order->setSelectionMode(QAbstractItemView::SingleSelection);
    my_order_layout->addWidget(table_my_order);
    layout->addLayout(my_order_layout);

    int row = 0;
    for(const auto& order : pending_orders){
        table_order->setItem(row, 0, new QTableWidgetItem(QString::number(order.id)));
        table_order->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(order.way)));
        table_order->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(order.product)));
        table_order->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(order.underlying)));
        table_order->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(order.currency)));
        table_order->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(order.strike)));
        table_order->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(order.price)));
        table_order->setItem(row, 7, new QTableWidgetItem(QString::fromStdString(order.status)));
        ++row;
    }

    row = 0;
    for(const auto& order : my_orders){
        table_my_order->setItem(row, 0, new QTableWidgetItem(QString::number(order.id)));
        table_my_order->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(order.way)));
        table_my_order->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(order.product)));
        table_my_order->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(order.underlying)));
        table_my_order->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(order.currency)));
        table_my_order->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(order.strike)));
        table_my_order->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(order.price)));
        table_my_order->setItem(row, 7, new QTableWidgetItem(QString::fromStdString(order.status)));
        ++row;
    }

    connect(take, &QPushButton::clicked, this, [this, table_order](){
        QList<QTableWidgetItem*> selected_row = table_order->selectedItems();
        open_take_order(selected_row);
    });
}

void MarketWindow::open_take_order(QList<QTableWidgetItem*> selected_row){
    Order order;
    order.id = static_cast<uint32_t>(selected_row[0]->text().toInt());
    order.way = selected_row[1]->text().toStdString();
    order.product = selected_row[2]->text().toStdString();
    order.underlying = selected_row[3]->text().toStdString();
    order.currency = selected_row[4]->text().toStdString();
    order.strike = selected_row[5]->text().toStdString();
    order.price = selected_row[6]->text().toStdString();
    order.status = selected_row[7]->text().toStdString();
    take_order_window = new NewOrder(order, wallets);
    take_order_window->show();
    connect(take_order_window, &NewOrder::closed, this, [this](){
        take_order_window = nullptr;
    });
}

MarketWindow::~MarketWindow(){
    delete json_curl_client;
}
