# ferment

`ferment` is a high-performance IP address expansion tool designed to efficiently process and expand IP addresses and CIDR notations. It's particularly useful for network administrators, security professionals, and developers working with large-scale IP address management.

[`cidrex`](https://github.com/d3mondev/cidrex) is a fine Go version that this utility is inspired by, but I really don't need to carve out 2.5MB of space for something that can be had in < 50K.

## Features

- Supports both IPv4 and IPv6 addresses
- Expands CIDR notations into individual IP addresses
- High-performance processing for large IP ranges
- Cross-platform compatibility
- Simple command-line interface

## Installation

Binary CLI releases for

- macOS Universal
- Linux x86_64, and
- Linux aarch64

are available in the release artifacts.

To install `ferment` from source, clone the repository and compile the source code:

```
git clone https://codeberg.org/hrbrmstr/ferment.git
cd ferment
clang -O3 -o ferment main.c
```

## Usage

Basic usage:

```
./ferment [options] input_file
```

or

```
./ferment [options] < input_file
```

Options:
- `-4`: Include IPv4 addresses (default: on)
- `-6`: Include IPv6 addresses (default: on)
- `-h`: Display help information

## Examples

1. Expand a single CIDR notation:
   ```
   echo "192.168.1.0/24" | ./ferment
   ```

2. Process a file containing mixed IP addresses and CIDR notations:
   ```
   ./ferment test-ips.txt
   ```

   or

   ```
   ./ferment < ip_list.txt
   ```

3. Expand only IPv6 addresses:
   ```
   ./ferment -6 test-ips.txt
   ```

   or

   ```
   ./ferment -6 < test-ips.txt
   ```

## Benchmark

This also beats the Golang version by a tad:

Benchmark 1: ./ferment test-ips.txt > /dev/null
  Time (mean ± σ):      30.6 ms ±   0.6 ms    [User: 28.2 ms, System: 1.9 ms]
  Range (min … max):    29.9 ms …  32.9 ms    80 runs

Benchmark 2: cidrex test-ips.txt > /dev/null
  Time (mean ± σ):      36.4 ms ±   1.4 ms    [User: 30.7 ms, System: 5.4 ms]
  Range (min … max):    34.1 ms …  38.9 ms    71 runs

Summary
  ./ferment test-ips.txt > /dev/null ran
    1.19 ± 0.05 times faster than cidrex test-ips.txt > /dev/null

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `./ferment test-ips.txt > /dev/null` | 30.6 ± 0.6 | 29.9 | 32.9 | 1.00 |
| `cidrex test-ips.txt > /dev/null` | 36.4 ± 1.4 | 34.1 | 38.9 | 1.19 ± 0.05 |


## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
