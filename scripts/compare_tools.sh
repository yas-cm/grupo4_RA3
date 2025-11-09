#!/bin/bash

echo "=== VALIDAÇÃO E COMPARAÇÃO DO MONITOR ==="
echo "Processo atual: PID $$"
echo ""

# Criar um processo de teste
echo "Criando processo de teste..."
sleep 300 &
TEST_PID=$!
echo "Processo de teste: PID $TEST_PID"

echo -e "\n"=============================""
echo "1. NOSSO MONITOR (DETALHADO):"
echo "============================="
./resource-monitor --pid $TEST_PID --detalhes

echo -e "\n"=============================""
echo "2. COMPARAÇÃO CPU:"
echo "============================="
echo "Nosso monitor vs Ferramentas Oficiais:"
echo "---"
./resource-monitor --pid $TEST_PID --detalhes 2>/dev/null | grep -E "(Uso de CPU|CPU%)" | head -2
echo "---"
echo "PS:    $(ps -p $TEST_PID -o pcpu --no-headers | xargs)%"
echo "Top:   $(top -p $TEST_PID -n 1 -b | grep $TEST_PID | awk '{print $9}')%"

echo -e "\n"=============================""
echo "3. COMPARAÇÃO MEMÓRIA:"
echo "============================="
echo "Nosso monitor:"
./resource-monitor --pid $TEST_PID --detalhes 2>/dev/null | grep -E "(MEMORIA|RSS|VSZ)" | head -3
echo "---"
echo "PS:"
ps -p $TEST_PID -o pid,rss,vsz,pmem --no-headers | awk '{
    printf "  RSS: %.1f MB, VSZ: %.1f MB, PMEM: %s%%\n", $2/1024, $3/1024, $4
}'
echo "---"
echo "/proc diretamente:"
read rss vsz < /proc/$TEST_PID/statm
page_size=$(getconf PAGESIZE)
printf "  RSS: %.1f MB, VSZ: %.1f MB\n" \
    $((rss * page_size / 1024 / 1024)) \
    $((vsz * page_size / 1024 / 1024))

echo -e "\n"=============================""
echo "4. COMPARAÇÃO I/O:"
echo "============================="
echo "Nosso monitor:"
./resource-monitor --pid $TEST_PID --detalhes 2>/dev/null | grep -A4 "INFORMACOES DE I/O"
echo "---"
echo "/proc/$TEST_PID/io:"
cat /proc/$TEST_PID/io 2>/dev/null | grep -E "(read_bytes|write_bytes|rchar|wchar)"

echo -e "\n"=============================""
echo "5. COMPARAÇÃO REDE:"
echo "============================="
echo "Nosso monitor:"
./resource-monitor --pid $TEST_PID --detalhes 2>/dev/null | grep -A4 "INFORMACOES DE REDE"
echo "---"
echo "Estatísticas de rede (/proc/$TEST_PID/net/dev):"
cat /proc/$TEST_PID/net/dev 2>/dev/null | head -3

echo -e "\n"=============================""
echo "6. MONITORAMENTO EM TEMPO REAL (5s):"
echo "============================="
./resource-monitor --pids $TEST_PID,$$ --intervalo 2 &
MONITOR_PID=$!
sleep 5
kill $MONITOR_PID 2>/dev/null

echo -e "\n"=============================""
echo "7. TESTE COM PROCESSOS CONHECIDOS:"
echo "=============================""
echo "Systemd (PID 1):"
./resource-monitor --pid 1 --detalhes 2>/dev/null | grep -E "(CPU%|MEMORIA)" | head -2
echo "---"
echo "Shell atual (PID $$):"  
./resource-monitor --pid $$ --detalhes 2>/dev/null | grep -E "(CPU%|MEMORIA)" | head -2

# Limpeza
kill $TEST_PID 2>/dev/null
echo -e "\nValidação concluída! ✅"