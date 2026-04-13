from math import sin
import random
import matplotlib.pyplot as plt

# simulate time series
n = 100
true_value = 50  # stable water level distance cm
measurements = []
kalman = []

# kalman params (typical)
q = 0.2
r = 5

x = 50
p = 1

for i in range(n):
    # simulate noisy ultrasonic reading
    noise = random.uniform(-8, 8)
    z = true_value + noise

    # occasional spikes (like real sensor)
    if random.random() < 0.05:
        z += random.uniform(-20, 20)

    measurements.append(z)

    # kalman predict/update
    p = p + q
    k = p / (p + r)
    x = x + k * (z - x)
    p = (1 - k) * p

    kalman.append(x)

plt.figure()
plt.plot(measurements, label="Raw Sensor")
plt.plot(kalman, label="Kalman Filtered")
plt.axhline(true_value)
plt.title("Ultrasonic Sensor vs Kalman Filter Output")
plt.xlabel("Sample")
plt.ylabel("Distance (cm)")
plt.legend()
plt.show()