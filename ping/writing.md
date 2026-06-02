# Packet Processing 1

_This will be my first ever blog written as myself. I plan to write a series of four
blogs, each reimplementing a layer of the typical networking stack, starting from
Ethernet frames, and ending at HTTPS._

## Preface

There's a particular kind of restlessness that comes with a San Mateo summer noon — a cold Baridi Mule from Peet's, the Pedestrian Mall alive with tech bros joyfully walking out for lunch. As for me, I'm blessed with a rare pocket of free time in the gap between two internships, and it just feels criminal not to fill it with something.

<!-- TODO: Add picture from Peet's -->
Figure 1. San Mateo Peet's Coffee view

So, naturally, I decided to rewrite my EC2's networking stack. The goal is to create a minimal stack that serves HTTPS requests. Ideally, it should have much lower latency and higher throughput than the Linux kernel's default networking stack due to being poll-mode driven and less bloated.

## Introduction

The plan is simple. We will rewrite the entire stack in four installations: Ethernet & Ping reponses, IP & TCP, TLS, and HTTPS. This blog will be the first of the four, covering everything up to ping responses.

To fully understand and follow this blog, you will need to know the following
- 7-layer OSI networking model and what the layers entail
- (Recommended) Basic systems knowledge (paging & huge pages, schedulers & multithreading, etc.)
- (Recommended) Basic AWS knowledge (EC2, networking security groups, etc.)

## Development Environment

To set up the recommended environment:

1. Create an Amazon Web Services (AWS) account (free credits granted for new users).
2. Install [AWS CLI](https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html) and log in with `aws login`.
3. Run `ping/spawn.sh` in bash (assuming default region `us-east-1`, adjust for your use case if needed)
4. Run the ssh command printed by `ping/spawn.sh` to connect to the instance.

## Data Plane Development Kit (DPDK)

"DPDK is a set of libraries and drivers for fast packet processing" -- DPDK's README.

DPDK has everything you need to make networking fast and avoid using the kernel as much as kernel-bypassing goes: CPU frequency settings, CPU scheduling bypass (avoiding the Completely Fair Scheduler), memory allocation bypass (avoiding paging), etc.

For the purpose of spamming ping responses though, we only need to care about its Amazon EC2 Elastic Network Adapter drivers.

### Setting up

1. Install related packages with `sudo apt install --update dpdk dpdk-dev dpdk-doc dpdk-kmods-dkms`
2. Load the `igb_uio` kernel module with `sudo modprobe igb_uio`. DPDK uses this to directly access the network card.
3. List your network cards with `dpdk-devbind.py --status` and find the PCI address of the one you want to use (e.g. `0000:00:06.0`). Pick the second network card on the list so you don't accidentally disconnect with your SSH.
4. Run `sudo dpdk-devbind.py -b igb_uio --force <PCI_ADDRESS>` to bind the network card to DPDK.
5. Set up hugepages with `sudo dpdk-hugepages.py --setup 512M`. DPDK uses hugepages to avoid kernel paging, get better performance, and communicate with the network card properly.

## Printing out packets




## Postscript

Of course, https://dl.acm.org/doi/pdf/10.1145/3419394.3423620
