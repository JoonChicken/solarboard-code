import numpy as np
import matplotlib.pyplot as plt


def pressure_to_altitude(pressure):
    pres_mb = pressure / 100;
    h_alt = 145366.45 * (1 - pow(pres_mb / 1013.25, 0.190284));
    return h_alt * 0.3048;


filename = "SUNDAT99.csv" #input("Enter a SUNDATAx.csv to graph: ")
arr = np.genfromtxt(filename, delimiter=", ", dtype=None, names=True)

# plt.plot(arr["current1"])  #[8000:22000]
# plt.plot(arr["current2"])  #[8000:22000]
# plt.plot(arr["current3"])  #[8000:22000]
# plt.plot(arr["current4"])  #[8000:22000]
plt.plot(arr["accelx"])  #[8000:22000]
plt.plot(arr["accely"])  #[8000:22000]
plt.plot(arr["accelz"])  #[8000:22000]
# plt.legend(["sensor 1", "sensor 2", "sensor 3", "sensor 4"], loc="upper left")
plt.legend(["accel x", "accel y", "accel z"], loc="upper right")
plt.xlabel("Time (ms)")
# plt.ylabel("Current (A)")
plt.ylabel("Acceleration (m/s^2)")
plt.show()

# plt.plot(arr["timestamp"], arr["degrees x"])
# plt.plot(arr["timestamp"], arr["degrees y"])
# plt.plot(arr["timestamp"], arr["degrees z"])
# plt.legend(["degrees x", "degrees y", "degrees z"], loc="upper right");
# plt.xlabel("Time (ms)")
# plt.ylabel("Degrees")
# plt.show()


# plt.plot(arr[""], arr["current1"])
# plt.plot(arr[""], arr["current2"])
# plt.plot(arr[""], arr["current3"])
# plt.plot(arr[""], arr["current4"])
# plt.legend(["sensor 1", "sensor 2", "sensor 3", "sensor 4"], loc="upper right");
# plt.xlabel("Time (ms)")
# plt.ylabel("Current (A)")
# plt.show()