#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "bip32wallet.h"
#include "curlrpc.h"
#include <QMainWindow>
#include <QMessageBox>
#include <QDialog>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QInputDialog>
#include <QTextBrowser>
#include <QFileDialog>
#include <QListWidget>
#include <QPushButton>
#include <QClipboard>
#include "marketwindow.h"
#include "neworder.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void add_wallet_tab(Bip32Wallet *wallet);
    void empty_window();

private:
    Ui::MainWindow *ui;
    QTextBrowser *text_widget;
    QTabWidget *tab_widget;
    vector<Bip32Wallet*> wallets;
    CurlRPC *rpc_client;
    MarketWindow *market_window;
    vector<NewOrder*> new_order_windows;
    map<QWidget*, Bip32Wallet*> map_table_wallet;

public slots:
    void new_wallet();
    void open_wallet();
    void close_tab(int tab);
    void open_market_window();
    void open_new_order_future(const vector<Bip32Wallet*> wallets);
    void open_new_order_call(const vector<Bip32Wallet*> wallets);
    void open_new_order_put(const vector<Bip32Wallet*> wallets);
};
#endif // MAINWINDOW_H
