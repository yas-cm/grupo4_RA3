#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def visualize_csv(filename):
    if not os.path.exists(filename):
        print(f"Arquivo {filename} não encontrado!")
        return
    
    df = pd.read_csv(filename)
    
    # Criar gráficos
    fig, axes = plt.subplots(2, 2, figsize=(12, 10))
    
    # CPU Usage
    for pid in df['PID'].unique():
        pid_data = df[df['PID'] == pid]
        axes[0,0].plot(pid_data.index, pid_data['CPU%'], label=f'PID {pid}')
    axes[0,0].set_title('Uso de CPU (%)')
    axes[0,0].legend()
    
    # Memory Usage
    for pid in df['PID'].unique():
        pid_data = df[df['PID'] == pid]
        axes[0,1].plot(pid_data.index, pid_data['Memoria_MB'], label=f'PID {pid}')
    axes[0,1].set_title('Uso de Memória (MB)')
    axes[0,1].legend()
    
    # I/O Rates
    for pid in df['PID'].unique():
        pid_data = df[df['PID'] == pid]
        axes[1,0].plot(pid_data.index, pid_data['IO_Read_Rate'], label=f'PID {pid} Read', linestyle='--')
        axes[1,0].plot(pid_data.index, pid_data['IO_Write_Rate'], label=f'PID {pid} Write', linestyle='-')
    axes[1,0].set_title('Taxas de I/O (B/s)')
    axes[1,0].legend()
    
    # Network Rates
    for pid in df['PID'].unique():
        pid_data = df[df['PID'] == pid]
        axes[1,1].plot(pid_data.index, pid_data['Net_RX_Rate'], label=f'PID {pid} RX', linestyle='--')
        axes[1,1].plot(pid_data.index, pid_data['Net_TX_Rate'], label=f'PID {pid} TX', linestyle='-')
    axes[1,1].set_title('Taxas de Rede (B/s)')
    axes[1,1].legend()
    
    plt.tight_layout()
    plt.savefig('monitoring_plot.png')
    print("Gráfico salvo como: monitoring_plot.png")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Uso: python3 visualize.py <arquivo_csv>")
        sys.exit(1)
    
    visualize_csv(sys.argv[1])