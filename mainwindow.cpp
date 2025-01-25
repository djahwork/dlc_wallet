#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "entropyinput.h"
#include <cstring>
#include "wallettab.h"
#include <QThread>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow), tab_widget(new QTabWidget)
{
    ui->setupUi(this);
    empty_window();
    tab_widget->setTabsClosable(true);
    connect(tab_widget, &QTabWidget::tabCloseRequested, this, &MainWindow::close_tab);
    string rpc_user = "tuco";
    string rpc_password = "ragroumes";
    string rpc_url = "http://localhost:18332/";

    rpc_client = new CurlRPC(rpc_url, rpc_user, rpc_password);

    connect(ui->actionFuture, &QAction::triggered, this, [this](){
        open_new_order_future(wallets);
    });
    connect(ui->actionVanillaCall, &QAction::triggered, this, [this](){
        open_new_order_call(wallets);
    });
    connect(ui->actionVanillaPut, &QAction::triggered, this, [this](){
        open_new_order_put(wallets);
    });

    market_window = nullptr;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete rpc_client;

    for (Bip32Wallet* wallet : wallets) {
        delete wallet;
    }
    wallets.clear();

    for(NewOrder* new_order_window : new_order_windows){
        delete new_order_window;
    }
    new_order_windows.clear();

    delete market_window;

    wally_cleanup(0);
}

void MainWindow::empty_window() {
    text_widget = new QTextBrowser(this);
    text_widget->setText(QString("\n\nFile menu -> New Wallet\nor \nFile menu -> Open Wallet"));
    text_widget->setGeometry(220, 230, 500, 240);
    text_widget->setStyleSheet(QString("border: 1px dashed #3498db; border-radius: 10px; padding: 5px;font-size: 28px;text-align: center;"));
    text_widget->setAlignment(Qt::AlignCenter);
    text_widget->show();
}

void MainWindow::add_wallet_tab(Bip32Wallet *wallet) {

    QWidget *tab = new WalletTab(wallet, rpc_client, this);
    tab_widget->addTab(tab, wallet->get_name());

    tab_widget->setCurrentIndex(tab_widget->count() - 1);

    map_table_wallet[tab] = wallet;

    if (tab_widget->count() > 0) {
        text_widget->hide();
        setCentralWidget(tab_widget);
        tab_widget->show();
    }
}

void MainWindow::new_wallet(){
    bool ok = false;

    QString wallet_name = QInputDialog::getText(
        this, "Wallet Name", "Enter a name for this wallet: ", QLineEdit::Normal, QString(), &ok
    );

    if (ok && !wallet_name.isEmpty()) {
        EntropyInput *dialog = new EntropyInput(this);
        QObject::connect(dialog, &EntropyInput::inputAccepted, [this, wallet_name](const QString &bits) {
            qDebug() << "User input bits:" << bits;
            Bip32Wallet *wallet = new Bip32Wallet(bits, wallet_name);
            wallets.push_back(wallet);
            wallet->save_to_file();
            qDebug() << "New Bip32 wallet " << wallet_name << " created.";
            add_wallet_tab(wallet);
        });

        dialog->exec();
    }
}

void MainWindow::open_wallet(){
    QString fichier = QFileDialog::getOpenFileName(this, "Ouvrir un fichier", QString(), "Text (*.txt)");
    auto parts = fichier.split(u'/');
    Bip32Wallet *wallet = new Bip32Wallet(parts.at(parts.size()-1).split(u'.').at(0));
    wallet->load_from_file();
    wallets.push_back(wallet);
    add_wallet_tab(wallet);
}

void MainWindow::close_tab(int index){
    qDebug() << "Remove tab: " << index;

    QWidget *widget = tab_widget->widget(index);
    if (widget == market_window) {
        tab_widget->removeTab(index);
        delete market_window;
        market_window = nullptr;
    } else {
        Bip32Wallet* wallet_to_remove = map_table_wallet[widget];
        auto it = std::find(wallets.begin(), wallets.end(), wallet_to_remove);
        if (it != wallets.end()) {
            wallets.erase(it);
            delete wallet_to_remove;
        }
        tab_widget->removeTab(index);
    }

    if(tab_widget->count() == 0){
        tab_widget->setVisible(false);
        empty_window();
    }
}

void MainWindow::open_market_window(){
    if (!market_window) {
        market_window = new MarketWindow(wallets, this);

        int index = tab_widget->addTab(market_window, "Market");
        tab_widget->setCurrentIndex(index);
        if (tab_widget->count() > 0) {
            text_widget->hide();
            setCentralWidget(tab_widget);
            tab_widget->show();
        }
    } else {
        int index = tab_widget->indexOf(market_window);
        if (index != -1) {
            tab_widget->setCurrentIndex(index);
        }
    }
}

void MainWindow::open_new_order_future(const vector<Bip32Wallet*> wallets){
    NewOrder *new_order_window = new NewOrder("forward", wallets);
    new_order_window->show();
    new_order_windows.push_back(new_order_window);
    connect(new_order_window, &NewOrder::closed, this, [&new_order_window](){
        new_order_window = nullptr;
    });
}

void MainWindow::open_new_order_call(const vector<Bip32Wallet*> wallets){
    NewOrder *new_order_window = new NewOrder("vanillacall", wallets);
    new_order_window->show();
    new_order_windows.push_back(new_order_window);
    connect(new_order_window, &NewOrder::closed, this, [&new_order_window](){
        new_order_window = nullptr;
    });
}

void MainWindow::open_new_order_put(const vector<Bip32Wallet*> wallets){
    NewOrder *new_order_window = new NewOrder("vanillaput", wallets);
    new_order_window->show();
    new_order_windows.push_back(new_order_window);
    connect(new_order_window, &NewOrder::closed, this, [&new_order_window](){
        new_order_window = nullptr;
    });
}
