import matplotlib.pyplot as plt
from typing import List, TextIO
from pathlib import Path

def read_latencies(f: TextIO) -> List[float]:
    t = [float(token.split("=")[1]) for token in f.read().split() if token.startswith("time=")]
    assert len(t) == 1000

    return t

base_dir = Path(__file__).parent

with open(base_dir / "dpdk.txt") as f_dpdk, open(base_dir / "kernel.txt") as f_kernel:
    dpdk = sorted(read_latencies(f_dpdk))
    kernel = sorted(read_latencies(f_kernel))

    dpdk_notail = dpdk[:990]
    kernel_notail = kernel[:990]

    fig, (dataset100, dataset99) = plt.subplots(2, 1)

    dataset100.hist(dpdk, bins=30, alpha=0.5, color='red', label='DPDK')
    dataset100.hist(kernel, bins=30, alpha=0.5, color='blue', label='Kernel')
    dataset100.set_xlabel('Latency (ms)')
    dataset100.set_ylabel('Frequency')
    dataset100.legend()
    dataset100.set_title('Latency distribution (1000 samples)')

    dataset99.hist(dpdk_notail, bins=30, alpha=0.5, color='red', label='DPDK')
    dataset99.hist(kernel_notail, bins=30, alpha=0.5, color='blue', label='Kernel')
    dataset99.set_xlabel('Latency (ms)')
    dataset99.set_ylabel('Frequency')
    dataset99.legend()
    dataset99.set_title('Latency distribution (99% tail removed)')

    fig.tight_layout()
    fig.savefig(base_dir / "latencies.png", dpi=300)

    # Print summary statistics
    print("DPDK Latencies:")
    print(f"  Min: {dpdk[0]:.3f} ms")
    print(f"  50th percentile (median): {dpdk[500]:.3f} ms")
    print(f"  99th percentile: {dpdk[989]:.3f} ms")
    print(f"  Max: {dpdk[-1]:.3f} ms")

    print("\nKernel Latencies:")
    print(f"  Min: {kernel[0]:.3f} ms")
    print(f"  50th percentile (median): {kernel[500]:.3f} ms")
    print(f"  99th percentile: {kernel[989]:.3f} ms")
    print(f"  Max: {kernel[-1]:.3f} ms")
