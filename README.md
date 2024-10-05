# ferment

`ferment` is a high-performance IP address expansion tool designed to efficiently process and expand IP addresses and CIDR notations. It's particularly useful for network administrators, security professionals, and developers working with large-scale IP address management.

## Features

- Supports both IPv4 and IPv6 addresses
- Expands CIDR notations into individual IP addresses
- High-performance processing for large IP ranges
- Cross-platform compatibility
- Simple command-line interface

## Installation

To install Ferment, clone the repository and compile the source code:

```
git clone https://codeberg.org/hrbrmstr/ferment.git
cd ferment
just build
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
   ./ferment ip_list.txt
   ```

   or

   ```
   ./ferment < ip_list.txt
   ```

3. Expand only IPv6 addresses:
   ```
   ./ferment -4 < ipv6_list.txt
   ```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
