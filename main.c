#include <arpa/inet.h>  // for inet_pton, inet_ntop
#include <ctype.h>      // for isspace
#include <getopt.h>     // for getopt_long
#include <netinet/in.h> // for struct in_addr and struct in6_addr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getopt_long
#include <inttypes.h>


#define BUFFER_SIZE 4096 * 10

#define BATCH_SIZE 1024
#define V4_BUFFER_SIZE (16 * BATCH_SIZE * 1024)
#define V6_BUFFER_SIZE (INET6_ADDRSTRLEN * BATCH_SIZE)
#define V6_MAX_ADDRS 1000000

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
  setvbuf(stdout, NULL, _IONBF, 0);
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
  char line[BUFFER_SIZE];
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
      printf("%s\n", line);
    }
    return 0;
  }

  // Try parsing as IPv6 address
  if (inet_pton(AF_INET6, line, &ipv6addr) == 1) {
    // It's an IPv6 address
    if (includeIPv6) {
      if (includeIPv4) {
        printf("%s\n", line);
      }
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

static inline char *ipv4ToString(uint32_t ip, char *buffer) {
  unsigned char bytes[4];
  bytes[0] = ip & 0xFF;
  bytes[1] = (ip >> 8) & 0xFF;
  bytes[2] = (ip >> 16) & 0xFF;
  bytes[3] = (ip >> 24) & 0xFF;
  sprintf(buffer, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
  return buffer;
}

int expandIPv4CIDR(struct in_addr ip, int prefixLen, int includeIPv4) {
  if (!includeIPv4)
    return 0;

  static char *buffer = NULL;
  if (buffer == NULL) {
    buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL) {
      fprintf(stderr, "Failed to allocate memory\n");
      return -1;
    }
  }

  uint32_t ipaddr = ntohl(ip.s_addr);
  uint32_t netmask = (0xFFFFFFFFU << (32 - prefixLen)) & 0xFFFFFFFFU;
  uint32_t network = ipaddr & netmask;
  uint32_t broadcast = network | (~netmask & 0xFFFFFFFFU);

  uint32_t num_addresses = broadcast - network + 1;
  char ip_buffer[16]; // Temporary buffer for a single IP
  char *buf_ptr = buffer;
  size_t total_written = 0;

  for (uint32_t addr = network; addr <= broadcast; addr++) {
    ipv4ToString(addr, ip_buffer);
    int len = strlen(ip_buffer);

    if (buf_ptr - buffer + len + 1 >= BUFFER_SIZE) {
      // Buffer is full, write it out
      fwrite(buffer, 1, buf_ptr - buffer, stdout);
      total_written += buf_ptr - buffer;
      buf_ptr = buffer;
    }

    memcpy(buf_ptr, ip_buffer, len);
    buf_ptr += len;
    *buf_ptr++ = '\n';
  }

  // Write any remaining data
  if (buf_ptr > buffer) {
    fwrite(buffer, 1, buf_ptr - buffer, stdout);
    total_written += buf_ptr - buffer;
  }

  // For benchmarking, you might want to return the number of bytes written
  // instead of actually writing to stdout
  // return total_written;

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

  // Calculate total addresses in this prefix
  uint64_t total_addresses = 1ULL << (128 - prefixLen);
  if (total_addresses > V6_MAX_ADDRS) {
    total_addresses = V6_MAX_ADDRS;
    fprintf(stderr, "Limiting to %" PRIu64 " addresses\n", total_addresses);
  }

  // Apply netmask to the IP address
  for (int i = prefixLen; i < 128; i++) {
    int byte = i / 8;
    int bit = 7 - (i % 8);
    ip.s6_addr[byte] &= ~(1 << bit);
  }

  char buffer[V6_BUFFER_SIZE];
  char *buf_ptr = buffer;
  uint64_t count = 0;

  while (count < total_addresses) {
    for (int i = 0; i < BATCH_SIZE && count < total_addresses; i++, count++) {
      inet_ntop(AF_INET6, &ip, buf_ptr, INET6_ADDRSTRLEN);
      buf_ptr += strlen(buf_ptr);
      *buf_ptr++ = '\n';

      // Increment IP address
      int j;
      for (j = 15; j >= 0; j--) {
        if (++ip.s6_addr[j] != 0)
          break;
      }
      if (j < 0)
        break; // Overflow, we're done
    }

    // Write batch to stdout
    fwrite(buffer, 1, buf_ptr - buffer, stdout);
    buf_ptr = buffer;
  }

  return 0;
}
