import matplotlib.pyplot as plt
import numpy as np

# Дані
threads = ['1', 'Фіз/2', 'Фіз', 'Фіз*2', 'Фіз*4', 'Фіз*8', 'Фіз*16']
data = {
    1000: [0, 2.5, 2.5, 5, 7, 31, 47],
    10000: [51, 17, 13, 12.5, 15.5, 32.5, 52.5],
    50000: [1340, 465, 307.5, 227.5, 234, 252.5, 281]
}

# Побудова графіків
fig, axes = plt.subplots(1, 3, figsize=(15, 5))
for ax, (size, times) in zip(axes, data.items()):
    ax.plot(threads, times, marker='o', linestyle='-', label=f'{size} елементів')
    ax.set_title(f'Час виконання для {size} елементів')
    ax.set_xlabel('Кількість потоків')
    ax.set_ylabel('Час (мс)')
    ax.grid(True)
    ax.legend()

plt.tight_layout()
plt.show()
