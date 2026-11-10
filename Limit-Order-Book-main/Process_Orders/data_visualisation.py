import os
import sys
from datetime import datetime
import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

def create_bar_chart_from_csv(csv_filename, figures_dir):
    print(f"\n==================== Data Visualiser ====================")
    abs_path = os.path.abspath(csv_filename)
    mtime = datetime.fromtimestamp(os.path.getmtime(csv_filename)).strftime('%Y-%m-%d %H:%M:%S')
    size_mb = os.path.getsize(csv_filename) / (1024 * 1024)
    print(f"Loading CSV:   {abs_path}")
    print(f"File Size:     {size_mb:.2f} MB")
    print(f"Last Modified: {mtime}")
    print("Reading CSV data, please wait...")

    # Read the CSV file into a DataFrame
    df = pd.read_csv(csv_filename, header=None, names=['Order Type', 'Times', 'Executed Orders', 'AVL Tree Balances'])
    
    # Ensure the CSV has the necessary columns
    if 'Times' not in df.columns or 'Order Type' not in df.columns:
        raise ValueError("CSV file must contain 'Times' and 'Order Type' columns")
    
    times = df['Times']
    orderTypes = df['Order Type']
    average_time = times.mean()
    trades_count = df['Executed Orders'].sum()
    balances_count = df['AVL Tree Balances'].sum()

    print(f"\n--- Benchmark Summary Statistics ---")
    print(f"Total Orders:          {len(df):,}")
    print(f"Average Latency:       {average_time:.2f} ns ({1000000000/average_time:,.0f} orders/sec)")
    print(f"Total Executed Trades: {trades_count:,}")
    print(f"Total AVL Balances:    {balances_count:,}")
    print(f"-------------------------------------\n")

    os.makedirs(figures_dir, exist_ok=True)

    # 1. Distribution of Order Types (Pie Chart)
    order_type_counts = orderTypes.value_counts()[::-1]
    plt.figure(figsize=(8, 8))
    plt.pie(order_type_counts, labels=order_type_counts.index, autopct='%1.1f%%', startangle=0, colors=plt.cm.tab20.colors)
    plt.title('Distribution of Order Types')
    fig1 = os.path.join(figures_dir, 'OrderTypes.png')
    plt.savefig(fig1, bbox_inches='tight')
    plt.close()
    print(f"Saved: {fig1}")

    # 2. Orders by Latency Histogram
    filtered_times = times[times <= 4000]
    plt.figure(figsize=(10, 6))
    plt.hist(filtered_times, bins=40, color='skyblue', edgecolor='black')
    plt.title(f'Orders by Latency Histogram - mean={average_time:.1f}ns ({1000000000/average_time:,.0f} orders/s)')
    plt.xlabel('Latency (ns)')
    plt.ylabel('Number of Orders')
    fig2 = os.path.join(figures_dir, 'LatencyHistogram.png')
    plt.savefig(fig2, bbox_inches='tight')
    plt.close()
    print(f"Saved: {fig2}")
    
    # 3. Latency by Order Type (Bar chart with percentiles)
    excluded_order_types = ['Market', 'AddMarketLimit']
    filtered_df = df[~df['Order Type'].isin(excluded_order_types)]
    stats = filtered_df.groupby('Order Type')['Times'].agg(['mean', lambda x: x.quantile(0.15), lambda x: x.quantile(0.85)]).reset_index()
    stats.columns = ['Order Type', 'mean', '25th', '75th']
    stats['error_lower'] = stats['mean'] - stats['25th']
    stats['error_upper'] = stats['75th'] - stats['mean']
    stats = stats.sort_values(by='mean')

    plt.figure(figsize=(12, 6))
    plt.bar(stats['Order Type'], stats['mean'], yerr=[stats['error_lower'], stats['error_upper']], capsize=5, color='skyblue', edgecolor='black')
    plt.title('Latency by Order Type')
    plt.xlabel('Order Type')
    plt.ylabel('Latency (ns)')
    plt.xticks(rotation=45)
    fig3 = os.path.join(figures_dir, 'OrderTypeLatencies.png')
    plt.savefig(fig3, bbox_inches='tight')
    plt.close()
    print(f"Saved: {fig3}")

    # 4. Latency by Number of Trades
    market_df = df[df['Order Type'].isin(['Market', 'AddMarketLimit'])]
    stats = market_df.groupby('Executed Orders')['Times'].agg(['mean', 'count']).reset_index()
    stats.columns = ['Executed Orders', 'mean', 'count']
    stats_filtered = stats[stats['count'] >= 5]

    plt.figure(figsize=(12, 6))
    plt.bar(stats_filtered['Executed Orders'], stats_filtered['mean'], capsize=5, color='skyblue', edgecolor='black')
    plt.title('Latency by Number of Trades')
    plt.xlabel('Number of Trades per Order')
    plt.ylabel('Latency (ns)')
    fig4 = os.path.join(figures_dir, 'ExecutedOrders.png')
    plt.savefig(fig4, bbox_inches='tight')
    plt.close()
    print(f"Saved: {fig4}")

    # 5. Latency by AVL Tree Balances
    balance_df = df[df['AVL Tree Balances'] != 0]
    stats = balance_df.groupby('AVL Tree Balances')['Times'].agg(['mean', 'count']).reset_index()
    stats.columns = ['AVL Tree Balances', 'mean', 'count']
    stats_filtered = stats[stats['count'] >= 5]

    plt.figure(figsize=(12, 6))
    plt.bar(stats_filtered['AVL Tree Balances'], stats_filtered['mean'], capsize=5, color='skyblue', edgecolor='black')
    plt.title('Latency by Number of AVL Tree Balances')
    plt.xlabel('Number of AVL Tree Balances')
    plt.ylabel('Latency (ns)')
    fig5 = os.path.join(figures_dir, 'AVLTreeBalances.png')
    plt.savefig(fig5, bbox_inches='tight')
    plt.close()
    print(f"Saved: {fig5}")

    # 6. 3D Plot: Latency by Number of Trades and AVL Tree Balances
    fig = plt.figure(figsize=(10, 6))
    ax = fig.add_subplot(111, projection='3d')
    stats = market_df.groupby(['Executed Orders', 'AVL Tree Balances'])['Times'].agg(['mean', 'count']).reset_index()
    stats.columns = ['Executed Orders', 'AVL Tree Balances', 'mean', 'count']
    stats_filtered = stats[stats['count'] >= 5]
    
    x = stats_filtered['Executed Orders']
    y = stats_filtered['AVL Tree Balances']
    z = stats_filtered['mean']
    x_flipped = x.max() - x

    ax.bar3d(x_flipped, y, 0, 1, 1, z, color='skyblue')
    ax.set_xlabel('Number of Trades per Order')
    ax.set_ylabel('AVL Tree Balances')
    ax.set_zlabel('Time (ns)')
    ax.set_xticks([3, 13, 23, 33, 43, 53, 63, 73])
    ax.set_xticklabels([70, 60, 50, 40, 30, 20, 10, 0])
    plt.title('Latency by Number of Trades and AVL Tree Balances')
    fig6 = os.path.join(figures_dir, '3D.png')
    plt.savefig(fig6, bbox_inches='tight')
    plt.close()
    print(f"Saved: {fig6}")

    print("\nAll visualizations created and saved successfully!")

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir)

    # Determine CSV file path
    if len(sys.argv) > 1:
        csv_filename = sys.argv[1]
    else:
        # Check root first (newly generated benchmark file), then Process_Orders
        root_csv = os.path.join(root_dir, 'order_processing_times.csv')
        local_csv = os.path.join(script_dir, 'order_processing_times.csv')
        if os.path.exists(root_csv):
            csv_filename = root_csv
        elif os.path.exists(local_csv):
            csv_filename = local_csv
        else:
            csv_filename = 'order_processing_times.csv'

    figures_dir = os.path.join(root_dir, 'figures')
    create_bar_chart_from_csv(csv_filename, figures_dir)
