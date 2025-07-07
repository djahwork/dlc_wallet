# DLC Wallet

This project implements a C++ Bitcoin Wallet. It includes integration with the `libwally-core` library and can be connected to a DLC contract oracle.

## 🔧 Linux Manual Setup Guide

Follow the steps below to build and run this project manually on a Linux system.

---

### ✅ Prerequisites

⚒️ Install required packages:

```bash
sudo apt update
sudo apt install -y \
  git cmake autoconf libtool pkg-config build-essential \
  python3 python3-pip python3-setuptools \
  libcurl4-openssl-dev
```

⚒️ Update python path:

```bash
sudo ln -s /usr/bin/python3 /usr/bin/python
```

⚒️ Install nlohmann JSON

```bash
sudo apt install nlohmann-json3-dev
```

⚒️ Install libwally-core

```bash
git clone https://github.com/ElementsProject/libwally-core.git
cd libwally-core
git submodule init
git submodule sync --recursive
git submodule update --init --recursive
./tools/autogen.sh
./configure --enable-shared --enable-static
make
make check
sudo make install
```

🧪 Verify install

```bash
ls /usr/local/include/wally_core.h
ls /usr/local/lib/libwallycore.a
```

---

### 📥 Clone the Project

```bash
git clone --recurse-submodules https://github.com/djahwork/dlc_oracle.git
cd dlc_oracle
```

---

### ⚒️ Build DLC Wallet


```bash
mkdir build
cd build
cmake ..
make install
```

---

### 🚀 Run DLC Wallet

```bash
./dlc_wallet
```

---
