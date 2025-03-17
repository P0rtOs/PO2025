import matplotlib.pyplot as plt
import numpy as np

# Дані
sizes = [10_000_000, 1_000_000_000, 2_000_000_000]
straight = [389, 778, 1669]
mutex = [1888, 264, 690]
cas = [5, 206, 517]

# Графік 1: Лінійний
plt.figure(figsize=(10, 5))
plt.plot(sizes[1:], straight[1:], marker='o', label='Straight')
plt.plot(sizes[1:], mutex[1:], marker='s', label='Mutex')
plt.plot(sizes[1:], cas[1:], marker='^', label='CAS')
plt.xlabel("Array Size")
plt.ylabel("Time (ms)")
plt.title("Execution Time Comparison (Line Plot)")
plt.legend()
plt.grid()
plt.show()

# Графік 2: Стовпчастий
bar_width = 0.3
x = np.arange(len(sizes[1:]))

plt.figure(figsize=(10, 5))
plt.bar(x - bar_width, straight[1:], bar_width, label='Straight')
plt.bar(x, mutex[1:], bar_width, label='Mutex')
plt.bar(x + bar_width, cas[1:], bar_width, label='CAS')
plt.xticks(x, sizes[1:])
plt.xlabel("Array Size")
plt.ylabel("Time (ms)")
plt.title("Execution Time Comparison (Bar Chart)")
plt.legend()
plt.show()
