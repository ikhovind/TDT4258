#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum { dm, fa } cache_map_t;
typedef enum { uc, sc } cache_org_t;
typedef enum { instruction, data } access_t;

typedef struct {
  uint32_t address;
  access_t accesstype;
} mem_access_t;

typedef struct {
  uint64_t accesses;
  uint64_t hits;
  // You can declare additional statistics if
  // you like, however you are now allowed to
  // remove the accesses or hits
} cache_stat_t;

typedef struct {
  unsigned int num_blocks;
  unsigned int block_offset_bits;
  unsigned int index_bits;
  unsigned int num_tag_bits;
  cache_map_t cache_mapping;
  cache_org_t cache_org;
} cache_info_t;

typedef struct {
  uint32_t rep_counter;
  cache_info_t cache_info;
  uint32_t *cache;
} cache_t;

// DECLARE CACHES AND COUNTERS FOR THE STATS HERE

uint32_t cache_size;
uint32_t block_size = 64;
cache_map_t cache_mapping;
cache_org_t cache_org;


static unsigned int mylog2(unsigned int val) {
    unsigned int ret = 0;
    while (val > 1) {
        val >>= 1;
        ret++;
    }
    return ret;
}

// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;

/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */
mem_access_t read_transaction(FILE* ptr_file) {
  char buf[1000];
  char* token;
  char* string = buf;
  mem_access_t access;

  if (fgets(buf, 1000, ptr_file) != NULL) {
    /* Get the access type */
    token = strsep(&string, " \n");
    if (strcmp(token, "I") == 0) {
      access.accesstype = instruction;
    } else if (strcmp(token, "D") == 0) {
      access.accesstype = data;
    } else {
      printf("Unkown access type\n");
      exit(0);
    }

    /* Get the access type */
    token = strsep(&string, " \n");
    access.address = (uint32_t)strtol(token, NULL, 16);

    return access;
  }

  /* If there are no more entries in the file,
   * return an address 0 that will terminate the infinite loop in main
   */
  access.address = 0;
  return access;
}

// gets the index of address with given cache info
uint32_t get_index(cache_info_t cache_info, mem_access_t access) {
  unsigned mask = (1 << cache_info.index_bits) - 1;
  return (access.address >> cache_info.block_offset_bits) & mask;
}

uint32_t get_access_tag(cache_info_t cache_info, mem_access_t mem_access) {
   return mem_access.address >> (31 - cache_info.num_tag_bits);
}

uint32_t get_cache_tag(cache_info_t cache_info, uint32_t line_info) {
  // remove validity bit
  uint32_t test = line_info & ((1 << 31) - 1) ;
  // get tag part
  test = test >> (31 - 1 - cache_info.num_tag_bits);
  return test;
}

bool is_valid(uint32_t line_info) {
  return line_info & 0x80000000;
}

/**
 * Used to insert data into cache, works for both dm and fa mappings
 * @param cache cache you want to insert data into
 * @param access access you want to insert the data for
 */
void replace(cache_t *cache, mem_access_t access) {
  if (cache->cache_info.cache_mapping == dm) {
    // get access tag
    uint32_t shifted_acc_tag = get_access_tag(cache->cache_info, access) << (31 - 1 - cache->cache_info.num_tag_bits);
    // set access tag
    cache->cache[get_index(cache->cache_info, access)] = shifted_acc_tag;
    // set validity bit
    cache->cache[get_index(cache->cache_info, access)] |= 0x80000000;
  }
  else {
    uint32_t shifted_acc_tag = get_access_tag(cache->cache_info, access) << (31 - 1 - cache->cache_info.num_tag_bits);
    // set tag
    cache->cache[cache->rep_counter] = shifted_acc_tag;
    // set validity bits
    cache->cache[cache->rep_counter] |= 0x80000000;
    // iterate rep counter
    cache->rep_counter = (cache->rep_counter + 1) % cache->cache_info.num_blocks;
    printf("Iterating rep counter to: %d\n", cache->rep_counter);
  }
}

/**
 * Checks whether or not a given cache line contains data for the given access
 * @param cache_line cache line you want to check
 * @param access access we are checking for
 * @param cache_info info about cache, num tag bits, block offset bits and index bits
 * @return
 */
bool check_cache_line(uint32_t cache_line, mem_access_t access, cache_info_t cache_info) {
  if (is_valid(cache_line)) {
    if(get_access_tag(cache_info, access) == get_cache_tag(cache_info, cache_line)) {
      return true;
    }
  }
  return false;
}

// guess they never miss, huh...
bool hit_or_miss(cache_t *cache, mem_access_t access) {
  printf("  looking for access tag: %x\n", get_access_tag(cache->cache_info, access));
  if (cache->cache_info.cache_mapping == dm) {
    uint32_t cache_line = cache->cache[get_index(cache->cache_info, access)];
    printf("  checking cache line: %x, with access tag %x, at index: %d\n", cache_line, get_cache_tag(cache->cache_info, cache_line),
           get_index(cache->cache_info, access));
    if (check_cache_line(cache_line, access, cache->cache_info)) {
      return true;
    }
  }
  // fully associative
  else {
    for (int i = 0; i <  cache->cache_info.num_blocks; ++i) {
      printf("  checking cache line: %x, with access tag %x\n", cache->cache[i], get_cache_tag(cache->cache_info, cache->cache[i]));
      if (check_cache_line(cache->cache[i], access, cache->cache_info)) {
        return true;
      }
    }
  }
  replace(cache, access);
  return false;
}

void main(int argc, char** argv) {
  // Reset statistics:
  memset(&cache_statistics, 0, sizeof(cache_stat_t));

  /* Read command-line parameters and initialize:
   * cache_size, cache_mapping and cache_org variables
   */
  /* IMPORTANT: *IF* YOU ADD COMMAND LINE PARAMETERS (you really don't need to),
   * MAKE SURE TO ADD THEM IN THE END AND CHOOSE SENSIBLE DEFAULTS SUCH THAT WE
   * CAN RUN THE RESULTING BINARY WITHOUT HAVING TO SUPPLY MORE PARAMETERS THAN
   * SPECIFIED IN THE UNMODIFIED FILE (cache_size, cache_mapping and cache_org)
   */
  if (argc != 4) { /* argc should be 2 for correct execution */
    printf(
        "Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] "
        "[cache organization: uc|sc]\n");
    exit(0);
  } else {
    /* argv[0] is program name, parameters start with argv[1] */

    /* Set cache size */
    cache_size = atoi(argv[1]);


    /* Set Cache Mapping */
    if (strcmp(argv[2], "dm") == 0) {
      cache_mapping = dm;
    } else if (strcmp(argv[2], "fa") == 0) {
      cache_mapping = fa;
    } else {
      printf("Unknown cache mapping\n");
      exit(0);
    }

    /* Set Cache Organization */
    if (strcmp(argv[3], "uc") == 0) {
      cache_org = uc;
    } else if (strcmp(argv[3], "sc") == 0) {
      cache_org = sc;
    } else {
      printf("Unknown cache organization\n");
      exit(0);
    }
  }

  cache_info_t cache_info;
  cache_info.num_blocks = cache_size / 64;
  cache_info.block_offset_bits = 6;
  if (cache_mapping == dm) {
    cache_info.cache_mapping = dm;
    if (cache_org == uc) {
      cache_info.index_bits = mylog2(cache_info.num_blocks);
      cache_info.num_tag_bits = 32 - cache_info.block_offset_bits - cache_info.index_bits;
      cache_info.cache_org = uc;
    }
  }
  else {
    cache_info.cache_mapping = fa;
    if (cache_org == uc) {
      cache_info.index_bits = 0;
      cache_info.num_tag_bits = 32 - cache_info.block_offset_bits - cache_info.index_bits;
      cache_info.cache_org = uc;
    }
  }

  printf("num_blocks %d\n", cache_info.num_blocks);
  printf("block_offset_bits %d\n", cache_info.block_offset_bits);
  printf("index_bits %d\n", cache_info.index_bits);
  printf("num_tag_bits %d\n", cache_info.num_tag_bits);
  cache_t cache_box;
  cache_box.cache = calloc(cache_info.num_blocks, sizeof(uint32_t));
  cache_box.cache_info = cache_info;
  cache_box.rep_counter = 0;

  /* Open the file mem_trace.txt to read memory accesses */
  FILE* ptr_file;
  ptr_file = fopen("mem_trace1.txt", "r");
  if (!ptr_file) {
    printf("Unable to open the trace file\n");
    exit(1);
  }

  /* Loop until whole trace file has been read */
  mem_access_t access;
  while (1) {
    access = read_transaction(ptr_file);
    // If no transactions left, break out of loop
    if (access.address == 0) break;
    cache_statistics.accesses++;
    printf("%d %x\n", access.accesstype, access.address);
    /* Do a cache access */
    // ADD YOUR CODE HERE
    if (cache_org == uc) {
      if (hit_or_miss(&cache_box, access)) {
	      printf("Cache hit");
        cache_statistics.hits++;
      }
    }
    else {
      exit(1);
    }
  }

  /* Print the statistics */
  // DO NOT CHANGE THE FOLLOWING LINES!
  printf("\nCache Statistics\n");
  printf("-----------------\n\n");
  printf("Accesses: %ld\n", cache_statistics.accesses);
  printf("Hits:     %ld\n", cache_statistics.hits);
  printf("Hit Rate: %.4f\n",
         (double)cache_statistics.hits / cache_statistics.accesses);
  // DO NOT CHANGE UNTIL HERE
  // You can extend the memory statistic printing if you like!

  /* Close the trace file */
  fclose(ptr_file);
  free(cache_box.cache);
}
