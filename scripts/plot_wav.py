#!/usr/bin/python3

import sys
import os
import argparse
import logging

import numpy as np
import scipy.io.wavfile as wav
import matplotlib.pyplot as plt

def main():
    parser = argparse.ArgumentParser(description="")
    parser.add_argument("files", type=str, nargs='*', help="Input files")
    parser.add_argument("--start", type=float, help="Start time in seconds")
    parser.add_argument("--length", type=float, help="Length in seconds, if start is specified")
    parser.add_argument('-v', "--verbose", action='store_true', help="Set logging level to INFO")

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.INFO)

    for fname in args.files:
        logging.info("Processing file: %s" % fname)
        rate, data = wav.read(fname)
        logging.info("rate: %d" % rate)
        logging.info("length: %f seconds (%d samples)" % (data.shape[0] / rate, data.shape[0]))
        logging.info("channels: %d" % data.shape[1])
        logging.info("data type: %s" % str(data.dtype))

        if args.start:
            start = int(args.start * rate)
            if args.length:
                length = int(args.length * rate)
            else:
                # by default length is 1024 samples
                length = 1024
            logging.info("plot_start: %d" % start)
            logging.info("plot_length: %d" % length)
        else:
            start = 0
            length = data.shape[0]

        # plot two channels one on top of the other
        plt.subplot(2, 1, 1)
        plt.xlim(start, start + length)
        plt.plot(data[:,0])
        plt.subplot(2, 1, 2)
        plt.xlim(start, start + length)
        plt.plot(data[:,1])

        plt.show()

# ====

def stuff(args):
    pass

if __name__ == "__main__":
    main()
