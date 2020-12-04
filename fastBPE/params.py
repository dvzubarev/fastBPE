#!/usr/bin/env python
# coding: utf-8
import math

from scipy.optimize import curve_fit
import numpy as np

def sigmoid(x, x0, k):
    return 1 / (1+ np.exp(-k*(x-x0)))

    # xbegin = 200
    # xend = 84_000
    # step = (xend - xbegin) // len(ydata)
    # print("step", step)

    # # step = 64_500
    # l = list(range(xbegin, xend, step))

def fit_freqs():
    print("Fit freqs")
    ydata = np.array([0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.95])
    step = 64_500
    l = list(range(2000, len(ydata)*(step + 1), step))
    xdata = np.array([math.log2(v) for v in l])
    # xdata = np.array([math.log10(v) for v in l])
    print("data", l)
    print("ydata len", len(ydata), "ydata", ydata)
    print("xdata len", len(xdata), "xdata ", xdata)


    popt, pcov = curve_fit(sigmoid, xdata, ydata)
    print("x0= ", popt[0], "k= ", popt[1])
    print("pcov", pcov)

def fit_len():
    print("Fit len")
    ydata = np.array([0.0, 0.01, 0.3, 0.5, 0.7, 0.8, 0.9, 0.9, ])
    xdata = np.array([1,   2,   3,     4,  5,   6,   7,   8, ])

    popt, pcov = curve_fit(sigmoid, xdata, ydata)
    print("x0= ", popt[0], "k= ", popt[1])
    print("pcov", pcov)


if __name__ == '__main__':
    fit_freqs()
    fit_len()
