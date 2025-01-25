#ifndef MARKETWINDOW_H
#define MARKETWINDOW_H
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QCloseEvent>
#include <QDoubleValidator>
#include <QListWidgetItem>
#include "curlrpc.h"
#include <QTableWidget>
#include "neworder.h"

class MarketWindow : public QWidget
{
    Q_OBJECT
public:
    MarketWindow(vector<Bip32Wallet*> wallets, QWidget *parent = nullptr);
    void open_take_order(QList<QTableWidgetItem*> selected_row);
    ~MarketWindow();
private:
    CurlRPC *json_curl_client;
    NewOrder *take_order_window;
    vector<Bip32Wallet*> wallets;
protected:
    void closeEvent(QCloseEvent *event) override {
        emit closed();
        event->accept();
    }
signals:
    void closed();
};

#endif // MARKETWINDOW_H
