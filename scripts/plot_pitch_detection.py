#!/usr/bin/python3

import sys
import os
import argparse
import logging

import numpy as np
import matplotlib.pyplot as plt


def main():
    parser = argparse.ArgumentParser(description="")
    parser.add_argument("file", type=str, help="Input file containing values")
    parser.add_argument(
        "--buffer_size", type=int, default=1024, help="Jack buffer size"
    )
    parser.add_argument(
        "--sample_rate",
        type=int,
        default=48000,
        help="Sample rate of audio. With buffer size, it's possible to compute the time step of the graph",
    )
    parser.add_argument(
        "--channel_count",
        type=int,
        default=2,
        help="Number of channels. Values are interleaved in the file.",
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Set logging level to INFO"
    )

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.INFO)

    read_period = True  # Whether the file contains the period or the frequency
    vals = [[] for i in range(args.channel_count)]

    with open(args.file) as pitch_file:
        i = 0

        for line in pitch_file:
            channel = i % args.channel_count
            val = float(line)
            if val < 0:
                # no detection
                vals[channel].append(float("nan"))
            else:
                if read_period:
                    # convert to frequency
                    vals[channel].append(args.sample_rate / val)
                else:
                    vals[channel].append(val)
            i += 1

    sample_per_second = args.sample_rate / args.buffer_size
    logging.info("%0.3f sample per second" % sample_per_second)
    n_samples = min([len(v) for v in vals])
    t = np.arange(0, n_samples) / sample_per_second
    vals = np.array(vals).transpose()

    plt.title("Pitch detection")
    plt.xlabel("Time (s)")
    plt.ylabel("Frequency (Hz)")
    plt.grid(True)
    plt.plot(t, vals, label="Measured frequency")
    plt.legend()
    plt.show()


# ====


def stuff(args):
    pass


if __name__ == "__main__":
    main()
