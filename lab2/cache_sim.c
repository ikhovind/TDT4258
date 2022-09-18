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
    // len equal to num cache num_blocks, used to keep track of replacement, which cannot be guaranteed to be in rising order
    uint8_t *rep_order;
    uint32_t *data;
    uint8_t rep_order_counter;
} cache_data_t;

typedef struct {
  cache_info_t cache_info;
  cache_data_t data_cache;
  cache_data_t instruction_cache;
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

// gets the index of address with given data_cache info
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

uint8_t get_index_of_first_in(const uint8_t *replace_order, cache_info_t cache_info) {
  uint8_t max = 0;
  for (int i = 0; i < cache_info.num_blocks; ++i) {
    if (max < replace_order[i]) {
      max = replace_order[i];
    }
  }
  uint8_t min = UINT8_MAX;
  uint8_t index = 65;
  for (uint8_t i = 0; i < cache_info.num_blocks; ++i) {
    if ((max - replace_order[i]) < 64 && min > replace_order[i]) {
      min = replace_order[i];
      index = i;
    }
  }
  if (index > 63) {
    printf("Invalid index value when finding replacement");
    exit(1);
  }
  printf("Found index is: %d\n", index);
  return index;
}

/**
 * Used to get insertion index from fa mapped cache
 * @param data
 * @param cache_info
 * @return
 */
uint8_t get_insertion_index(cache_data_t *data, cache_info_t cache_info) {
  for (uint8_t i = 0; i < cache_info.num_blocks; ++i) {
    if (data->data[i] == 0) {
      return i;
    }
  }
  return get_index_of_first_in(data->rep_order, cache_info);
}

/**
 * Used to insert data into given cache, works for both dm and fa mappings
 * @param cache data_cache you want to insert data into
 * @param access access you want to insert the data for
 */
void replace(cache_data_t *cache, cache_info_t cache_info, mem_access_t access) {
  if (cache_info.cache_mapping == dm) {
    // get access tag
    uint32_t shifted_acc_tag = get_access_tag(cache_info, access) << (31 - 1 - cache_info.num_tag_bits);
    // set access tag
    cache->data[get_index(cache_info, access)] = shifted_acc_tag;
    // set validity bit
    cache->data[get_index(cache_info, access)] |= 0x80000000;
  }
  else {
    uint32_t shifted_acc_tag = get_access_tag(cache_info, access) << (31 - 1 - cache_info.num_tag_bits);
    uint8_t index = get_insertion_index(cache, cache_info);
    // set tag
    cache->data[index] = shifted_acc_tag;
    // set validity bits
    cache->data[index] |= 0x80000000;
    // iterate rep counter
    //cache->rep_counter = (cache->rep_counter + 1) % cache->cache_info.num_blocks;
    cache->rep_order[index] = (++cache->rep_order_counter % UINT8_MAX);
  }
}

void nullify(cache_data_t *cache, uint8_t index) {
  cache->data[index] = 0;
  /**
   * do we need to replace rep counter
   *    - this sets a zero, so counter is replaced on next insertion
   *    - meaning that any access that finds itself in cache does not need rep order
   *    - next access does not need rep order becuase this spot is empty
   *    - next access replaces rep order
   *    - e.g. this rep order is replaced before it is needed.
   *    - todo maybe bug with many data lookups replacing instruction elements in cache fucking up rep order
   */
}



/**
 * Checks whether or not a given data_cache line contains data for the given access
 * @param cache_line data_cache line you want to check
 * @param access access we are checking for
 * @param cache_info info about data_cache, num tag bits, block offset bits and index bits
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
uint8_t hit_or_miss(cache_data_t *cache, cache_info_t cache_info, mem_access_t access) {
  printf("  looking for access tag: %x\n", get_access_tag(cache_info, access));
  if (cache_info.cache_mapping == dm) {
    uint8_t index = get_index(cache_info, access);
    uint32_t cache_line = cache->data[index];
    printf("  checking data_cache line: %x, with access tag %x, at index: %d\n", cache_line, get_cache_tag(cache_info, cache_line), get_index(cache_info, access));
    if (check_cache_line(cache_line, access, cache_info)) {
      return index;
    }
  }
  // fully associative
  else {
    for (uint8_t i = 0; i <  cache_info.num_blocks; ++i) {
      printf("  checking data_cache line: %x, with access tag %x\n", cache->data[i], get_cache_tag(cache_info, cache->data[i]));
      if (check_cache_line(cache->data[i], access, cache_info)) {
        return i;
      }
    }
  }
  return UINT8_MAX;
}

bool check_caches(cache_t *cache, mem_access_t access, access_t access_type) {
  if (access_type == data || cache->cache_info.cache_org == uc) {
    printf("searching data cache\n");
    uint8_t res = hit_or_miss(&cache->data_cache, cache->cache_info, access);
    replace(&cache->data_cache, cache->cache_info, access);
    // if it was a cache hit, we know that this memory address should not exist in the other cache
    if (res == UINT8_MAX && cache->cache_info.cache_org == sc) {
      printf("  checking instruction conflict\n");
      uint8_t other_res = hit_or_miss(&cache->instruction_cache, cache->cache_info, access);
      if (other_res != UINT8_MAX) {
        printf("    found conflict, nullifying\n");
        nullify(&cache->instruction_cache, other_res);
      }
      return false;
    }
    return res != UINT8_MAX;
  }
  else {
    printf("searching instruction cache\n");
    uint8_t res = hit_or_miss(&cache->instruction_cache, cache->cache_info, access);
    replace(&cache->instruction_cache, cache->cache_info, access);
    // if it was a cache hit, we know that this memory address should not exist in the other cache, therefore do not need to check it
    if (res == UINT8_MAX) {
      printf("  searching data conflict\n");
      uint8_t other_res = hit_or_miss(&cache->data_cache, cache->cache_info, access);
      if (other_res != UINT8_MAX) {
        printf("    found conflict, nullifying\n");
        nullify(&cache->data_cache, other_res);
      }
      return false;
    }
    return true;
  }
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
        "Usage: ./cache_sim [data_cache size: 128-4096] [data_cache mapping: dm|fa] "
        "[data_cache organization: uc|sc]\n");
    exit(0);
  } else {
    /* argv[0] is program name, parameters start with argv[1] */

    /* Set data_cache size */
    cache_size = atoi(argv[1]);


    /* Set Cache Mapping */
    if (strcmp(argv[2], "dm") == 0) {
      cache_mapping = dm;
    } else if (strcmp(argv[2], "fa") == 0) {
      cache_mapping = fa;
    } else {
      printf("Unknown data_cache mapping\n");
      exit(0);
    }

    /* Set Cache Organization */
    if (strcmp(argv[3], "uc") == 0) {
      cache_org = uc;
    } else if (strcmp(argv[3], "sc") == 0) {
      cache_org = sc;
    } else {
      printf("Unknown data_cache organization\n");
      exit(0);
    }
  }

  cache_info_t cache_info;

  if (cache_org == sc) {
    cache_size = cache_size / 2;
    cache_info.cache_org = sc;
  }
  else {
    cache_info.cache_org = uc;
  }
  cache_info.num_blocks = cache_size / 64;
  cache_info.block_offset_bits = 6;
  if (cache_mapping == dm) {
    cache_info.cache_mapping = dm;
    cache_info.index_bits = mylog2(cache_info.num_blocks);
  }
  else {
    cache_info.cache_mapping = fa;
    cache_info.index_bits = 0;
  }
  cache_info.num_tag_bits = 32 - cache_info.block_offset_bits - cache_info.index_bits;

  printf("num_blocks %d\n", cache_info.num_blocks);
  printf("block_offset_bits %d\n", cache_info.block_offset_bits);
  printf("index_bits %d\n", cache_info.index_bits);
  printf("num_tag_bits %d\n", cache_info.num_tag_bits);
  cache_t cache_box;
  cache_box.data_cache.data = calloc(cache_info.num_blocks, sizeof(uint32_t));
  cache_box.data_cache.rep_order = calloc(cache_info.num_blocks, sizeof(uint8_t));
  cache_box.data_cache.rep_order_counter = 0;

  cache_box.instruction_cache.data = calloc(cache_info.num_blocks, sizeof(uint32_t));
  cache_box.instruction_cache.rep_order = calloc(cache_info.num_blocks, sizeof(uint8_t));
  cache_box.instruction_cache.rep_order_counter = 0;

  cache_box.cache_info = cache_info;

  /* Open the file mem_trace.txt to read memory accesses */
  FILE* ptr_file;
  ptr_file = fopen("mem_trace.txt", "r");
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
    /* Do a data_cache access */
    // ADD YOUR CODE HERE
    if (check_caches(&cache_box, access, access.accesstype)) {
      printf("Cache hit\n");
      cache_statistics.hits++;
    }
    else {
      printf("Cache miss\n");
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
  free(cache_box.data_cache.data);
  free(cache_box.data_cache.rep_order);
  free(cache_box.instruction_cache.data);
  free(cache_box.instruction_cache.rep_order);
}
