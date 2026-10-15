import numpy as np

def draw_plot(ax, result, plot_type, title):
    vi = result['vi']
    order = result['order']
    diffs = result['diffs']
    ax.clear()

    if len(vi) == 0 or len(order) == 0:
        ax.set_title("No data available")
        ax.text(0.5, 0.5, "Load valid result directories", ha='center', va='center', transform=ax.transAxes)
        return

    if plot_type == "Scatter":
        ax.scatter(vi, order, alpha=0.7, s=60, c='#006494', edgecolors='#0b3751', linewidth=0.5, label='Instances')
        min_val = min(np.min(vi), np.min(order))
        max_val = max(np.max(vi), np.max(order))
        ax.plot([min_val, max_val], [min_val, max_val], 'r--', linewidth=2, label='y = x')
        ax.set_xlabel("Value Iteration")
        ax.set_ylabel("Order-based")
        ax.legend()

    elif plot_type == "Line":
        ax.plot(range(len(vi)), vi, label='VI', linewidth=2)
        ax.plot(range(len(order)), order, label='Order-based', linewidth=2)
        ax.set_xlabel("Index")
        ax.set_ylabel("Value")
        ax.legend()

    elif plot_type == "Box":
        ax.boxplot([vi, order], labels=['VI', 'Order-based'])
        ax.set_ylabel("Value")

    elif plot_type == "Bar":
        means = [np.mean(vi), np.mean(order)]
        ax.bar(['VI', 'Order-based'], means, color=['#006494', '#a13544'])
        ax.set_ylabel("Average value")

    ax.set_title(title or plot_type)
    ax.grid(True, alpha=0.25)