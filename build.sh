#!/usr/bin/env bash
# Compila o sistema de monitoramento de sensores (Linux / macOS)
# Uso: ./build.sh
set -e
g++ -std=c++17 -O2 -Wall -o sensores main.cpp
echo "Compilado com sucesso: ./sensores"
echo "Execute com: ./sensores"
