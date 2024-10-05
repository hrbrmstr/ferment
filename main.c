#include <arpa/inet.h>  // for inet_pton, inet_ntop
#include <ctype.h>      // for isspace
#include <getopt.h>     // for getopt_long
#include <netinet/in.h> // for struct in_addr and struct in6_addr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getopt_long

void printUsage();
int processInput(FILE *inputFile, int includeIPv4, int includeIPv6);
int printIPsFromLine(char *line, int includeIPv4, int includeIPv6);
int expandIPv4CIDR(struct in_addr ip, int prefixLen, int includeIPv4);
int expandIPv6CIDR(struct in6_addr ip, int prefixLen, int includeIPv6);
void applyIPv6Netmask(struct in6_addr *ip, struct in6_addr *network,
                      int prefixLen);
int incrementIPv6Address(struct in6_addr *addr);
int isIPv6AddressBeyondPrefix(struct in6_addr *current_ip,
                              struct in6_addr *network, int prefixLen);

int main(int argc, char *argv[]) {
  int opt;
  int includeIPv4 = 0, includeIPv6 = 0;
  int help = 0;
  static struct option long_options[] = {{"ipv4", no_argument, 0, '4'},
                                         {"ipv6", no_argument, 0, '6'},
                                         {"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};
  while ((opt = getopt_long(argc, argv, "46h", long_options, NULL)) != -1) {
    switch (opt) {
    case '4':
      includeIPv4 = 1;
      break;
    case '6':
      includeIPv6 = 1;
      break;
    case 'h':
      help = 1;
      break;
    default:
      help = 1;
      break;
    }
  }

  if (help) {
    printUsage();
    return 0;
  }

  // If neither or both flags are set, include both IPv4 and IPv6
  if (!includeIPv4 && !includeIPv6) {
    includeIPv4 = 1;
    includeIPv6 = 1;
  }

  // Determine input source: file if provided, otherwise stdin
  FILE *inputFile = NULL;
  if (optind < argc) {
    inputFile = fopen(argv[optind], "r");
    if (!inputFile) {
      perror("Error opening input file");
      exit(EXIT_FAILURE);
    }
  } else {
    inputFile = stdin;
  }

  // Process input
  if (processInput(inputFile, includeIPv4, includeIPv6) != 0) {
    fprintf(stderr, "Error processing input\n");
    exit(EXIT_FAILURE);
  }

  if (inputFile != stdin) {
    fclose(inputFile);
  }

  return 0;
}

void printUsage() {
  printf("cidrex - Expand CIDR ranges\n\n");
  printf("Usage:\n");
  printf("  cidrex [OPTIONS] [filename]\n\n");
  printf("Options:\n");
  printf("  -4, --ipv4       Print only IPv4 addresses\n");
  printf("  -6, --ipv6       Print only IPv6 addresses\n");
  printf("  -h, --help       Display this help message\n\n");
  printf("Examples:\n");
  printf("  cidrex input.txt\n");
  printf("  cidrex -4 input.txt\n");
  printf("  cat input.txt | cidrex -6\n");
}

int processInput(FILE *inputFile, int includeIPv4, int includeIPv6) {
  char line[256];
  while (fgets(line, sizeof(line), inputFile)) {
    // Remove trailing newline
    line[strcspn(line, "\n")] = '\0';

    // Remove leading and trailing whitespace
    char *start = line;
    while (isspace((unsigned char)*start))
      start++;
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
      *end-- = '\0';

    if (*start == '\0') {
      continue; // Skip empty lines
    }

    if (printIPsFromLine(start, includeIPv4, includeIPv6) != 0) {
      // Continue processing
      continue;
    }
  }
  return 0;
}

int printIPsFromLine(char *line, int includeIPv4, int includeIPv6) {
  struct in_addr ipv4addr;
  struct in6_addr ipv6addr;

  // Try parsing as IPv4 address
  if (inet_pton(AF_INET, line, &ipv4addr) == 1) {
    // It's an IPv4 address
    if (includeIPv4) {
      char ipStr[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &ipv4addr, ipStr, sizeof(ipStr));
      printf("%s\n", ipStr);
    }
    return 0;
  }

  // Try parsing as IPv6 address
  if (inet_pton(AF_INET6, line, &ipv6addr) == 1) {
    // It's an IPv6 address
    if (includeIPv6) {
      char ipStr[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &ipv6addr, ipStr, sizeof(ipStr));
      printf("%s\n", ipStr);
    }
    return 0;
  }

  // Try parsing as CIDR notation
  char *slash = strchr(line, '/');
  if (slash) {
    *slash = '\0';
    char *prefixLenStr = slash + 1;
    int prefixLen = atoi(prefixLenStr);
    if (prefixLen < 0) {
      fprintf(stderr, "invalid prefix length: %s\n", line);
      return 0;
    }

    // Try parsing as IPv4 CIDR
    if (inet_pton(AF_INET, line, &ipv4addr) == 1) {
      if (prefixLen > 32) {
        fprintf(stderr, "invalid prefix length for IPv4: %d\n", prefixLen);
        return 0;
      }
      return expandIPv4CIDR(ipv4addr, prefixLen, includeIPv4);
    }

    // Try parsing as IPv6 CIDR
    if (inet_pton(AF_INET6, line, &ipv6addr) == 1) {
      if (prefixLen > 128) {
        fprintf(stderr, "invalid prefix length for IPv6: %d\n", prefixLen);
        return 0;
      }
      return expandIPv6CIDR(ipv6addr, prefixLen, includeIPv6);
    }
  }

  fprintf(stderr, "invalid IP or CIDR: %s\n", line);
  return 0;
}

int expandIPv4CIDR(struct in_addr ip, int prefixLen, int includeIPv4) {
  if (!includeIPv4)
    return 0;

  uint32_t ipaddr = ntohl(ip.s_addr);
  uint32_t netmask = (0xFFFFFFFF << (32 - prefixLen)) & 0xFFFFFFFF;
  uint32_t network = ipaddr & netmask;
  uint32_t broadcast = network | (~netmask & 0xFFFFFFFF);

  char ipStr[INET_ADDRSTRLEN];

  uint32_t num_addresses = broadcast - network + 1;

  for (uint32_t i = 0; i < num_addresses; i++) {
    uint32_t addr = network + i;
    struct in_addr current_ip;
    current_ip.s_addr = htonl(addr);
    inet_ntop(AF_INET, &current_ip, ipStr, sizeof(ipStr));
    printf("%s\n", ipStr);
  }
  return 0;
}

void applyIPv6Netmask(struct in6_addr *ip, struct in6_addr *network,
                      int prefixLen) {
  int full_bytes = prefixLen / 8;
  int remaining_bits = prefixLen % 8;

  memset(network, 0, sizeof(struct in6_addr));

  // Copy the full bytes
  for (int i = 0; i < full_bytes; i++) {
    network->s6_addr[i] = ip->s6_addr[i];
  }

  if (remaining_bits > 0 && full_bytes < 16) {
    // Apply the mask to the next byte
    uint8_t mask = (uint8_t)(0xFF << (8 - remaining_bits));
    network->s6_addr[full_bytes] = ip->s6_addr[full_bytes] & mask;
  }
  // Remaining bytes are already zero due to memset
}

int incrementIPv6Address(struct in6_addr *addr) {
  // Increment the address by 1
  for (int i = 15; i >= 0; i--) {
    if (++(addr->s6_addr[i]) != 0) {
      // No overflow, done
      return 0;
    }
    // Overflow, continue to next byte
  }
  // If we reach here, the address wrapped around to zero
  return 1; // Indicate that we have reached the end of address space
}

int isIPv6AddressBeyondPrefix(struct in6_addr *current_ip,
                              struct in6_addr *network, int prefixLen) {
  int byte_index = prefixLen / 8;
  int bit_index = prefixLen % 8;

  // Compare the bytes up to the byte containing the last prefix bit
  if (memcmp(current_ip->s6_addr, network->s6_addr, byte_index) != 0) {
    return 1; // current_ip is beyond the prefix
  }

  if (bit_index == 0) {
    // Prefix ends at a byte boundary, addresses are equal in the prefix
    return 0;
  }

  // Check the bits in the byte containing the last prefix bit
  uint8_t mask = (uint8_t)(0xFF << (8 - bit_index));
  if ((current_ip->s6_addr[byte_index] & mask) !=
      (network->s6_addr[byte_index] & mask)) {
    return 1;
  }

  // Addresses are equal in the prefix
  return 0;
}

int expandIPv6CIDR(struct in6_addr ip, int prefixLen, int includeIPv6) {
  if (!includeIPv6)
    return 0;

  // Apply netmask to the IP address
  struct in6_addr network;
  applyIPv6Netmask(&ip, &network, prefixLen);

  // Define the maximum number of addresses to prevent infinite loops
  uint64_t max_addresses = 1000000;
  uint64_t count = 0;

  struct in6_addr current_ip;
  memcpy(&current_ip, &network, sizeof(struct in6_addr));

  char ipStr[INET6_ADDRSTRLEN];
  do {
    inet_ntop(AF_INET6, &current_ip, ipStr, sizeof(ipStr));
    printf("%s\n", ipStr);
    count++;
    if (count >= max_addresses) {
      fprintf(stderr, "Reached maximum number of addresses (%llu), stopping\n",
              max_addresses);
      break;
    }
    if (incrementIPv6Address(&current_ip) != 0) {
      // Reached the end of address space
      break;
    }
  } while (!isIPv6AddressBeyondPrefix(&current_ip, &network, prefixLen));

  return 0;
}
