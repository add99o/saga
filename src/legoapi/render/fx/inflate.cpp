#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include <vorbis/codec.h>
#include <ogg/ogg.h>
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/implode.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

abi_ulong GetMatchLength(unsigned char *, unsigned char *, abi_ulong);
u32 HashString(unsigned char *);

static i32 *HashTable;
static i32 *LinkArray;

void refpack_init() {
    constexpr usize hash_table_size = 0x40000;
    constexpr usize link_array_size = 0x10000;

    HashTable = NULL;
    if (*theMemoryManager.end_cell - *theMemoryManager.cursor_cell > hash_table_size) {
        usize allocation = ALIGN(*theMemoryManager.cursor_cell, 0x10);
        *theMemoryManager.cursor_cell = allocation + hash_table_size;
        HashTable = reinterpret_cast<i32 *>(allocation);
        memset(HashTable, 0, hash_table_size);
        theMemoryManager.allocated += hash_table_size;
        theMemoryManager.remaining -= hash_table_size;
        theMemoryManager.high_water = *theMemoryManager.cursor_cell;
    }

    LinkArray = NULL;
    if (*theMemoryManager.end_cell - *theMemoryManager.cursor_cell > link_array_size) {
        usize allocation = ALIGN(*theMemoryManager.cursor_cell, 0x10);
        *theMemoryManager.cursor_cell = allocation + link_array_size;
        LinkArray = reinterpret_cast<i32 *>(allocation);
        memset(LinkArray, 0, link_array_size);
        theMemoryManager.allocated += link_array_size;
        theMemoryManager.remaining -= link_array_size;
        theMemoryManager.high_water = *theMemoryManager.cursor_cell;
    }
}

static i32 n;
static i32 heapsize;
static u16 heap[511];
static u16 *freq;
static u16 *sortptr;
static u16 len_cnt[17];
static u8 *len;

static __attribute__((noinline, used, optimize("O0"))) void count_len(i32 node) {
    static i32 depth;
    if (node < n) {
        ++len_cnt[MIN(depth, 16)];
        return;
    }
    ++depth;
    count_len(implode_left[node]);
    count_len(implode_right[node]);
    --depth;
}

static __attribute__((noinline, used, optimize("O0"))) void make_len(i32 root) {
    for (i32 bits = 0; bits <= 16; ++bits)
        len_cnt[bits] = 0;
    count_len(root);

    i32 total = 0;
    for (i32 bits = 16; bits > 0; --bits)
        total += static_cast<i32>(len_cnt[bits]) << (16 - bits);
    while (total != 0x10000) {
        --len_cnt[16];
        i32 bits = 15;
        while (bits > 0 && len_cnt[bits] == 0)
            --bits;
        --len_cnt[bits];
        len_cnt[bits + 1] += 2;
        --total;
    }

    for (i32 bits = 16; bits > 0; --bits) {
        for (i32 count = len_cnt[bits]; count > 0; --count)
            len[*sortptr++] = static_cast<u8>(bits);
    }
}

static __attribute__((noinline, used, optimize("O0"))) void downheap(i32 parent) {
    const i32 node = static_cast<i16>(heap[parent]);
    i32 child = parent * 2;
    while (child <= heapsize) {
        if (child < heapsize && freq[heap[child]] > freq[heap[child + 1]])
            ++child;
        if (freq[node] <= freq[heap[child]])
            break;
        heap[parent] = heap[child];
        parent = child;
        child = parent * 2;
    }
    heap[parent] = static_cast<u16>(node);
}

static __attribute__((noinline, used, optimize("O0"))) void make_code(i32 count, u8 *lengths, u16 *codes) {
    u16 next_code[18];
    next_code[0] = 0;
    for (i32 bits = 1; bits <= 16; ++bits)
        next_code[bits + 1] = static_cast<u16>((next_code[bits] + len_cnt[bits]) * 2);
    for (i32 symbol = 0; symbol < count; ++symbol)
        codes[symbol] = next_code[lengths[symbol]]++;
}

i32 __attribute__((optimize("O0"))) ImplodeMakeTree(i32 symbol_count, u16 *frequencies, unsigned char *lengths,
                                                      u16 *codes) {
    n = symbol_count;
    freq = frequencies;
    len = lengths;
    i32 root = n;
    heapsize = 0;
    heap[1] = 0;

    for (i32 symbol = 0; symbol < n; ++symbol) {
        len[symbol] = 0;
        if (freq[symbol] != 0)
            heap[++heapsize] = static_cast<u16>(symbol);
    }
    if (heapsize <= 1) {
        codes[heap[1]] = 0;
        return static_cast<i16>(heap[1]);
    }

    for (i32 parent = heapsize / 2; parent > 0; --parent)
        downheap(parent);

    sortptr = codes;
    do {
        const i32 first = static_cast<i16>(heap[1]);
        if (first < n)
            *sortptr++ = static_cast<u16>(first);
        heap[1] = heap[heapsize--];
        downheap(1);

        const i32 second = static_cast<i16>(heap[1]);
        if (second < n)
            *sortptr++ = static_cast<u16>(second);

        freq[root] = static_cast<u16>(freq[first] + freq[second]);
        heap[1] = static_cast<u16>(root);
        downheap(1);
        implode_left[root] = static_cast<u16>(first);
        implode_right[root] = static_cast<u16>(second);
        ++root;
    } while (heapsize > 1);

    sortptr = codes;
    make_len(root - 1);
    make_code(n, len, codes);
    return root - 1;
}

i32 refpack(unsigned char *source, abi_long source_size, unsigned char *destination) {
    memset(HashTable, 0xff, 0x40000);

    unsigned char *output = destination;
    if (source_size <= 0) {
        *output = 0xfc;
        return 1;
    }

    unsigned char *input_start = source;
    unsigned char *literal_start = source;
    abi_long remaining = source_size;
    u32 literal_count = 0;

    while (remaining > 0) {
        const i32 input_index = source - input_start;
        const u32 hash = HashString(source);
        i32 candidate_index = HashTable[hash];
        const i32 chain_limit = MAX(0, input_index - 0x3fff);
        u32 match_length = 2;
        u32 match_offset = 0;
        u32 command_size = 2;

        while (candidate_index >= chain_limit) {
            unsigned char *candidate = input_start + candidate_index;
            if (candidate[match_length] == source[match_length]) {
                const u32 length = GetMatchLength(source, candidate, MIN(remaining, 0x404));
                if (length > match_length) {
                    const u32 offset = input_index - 1 - candidate_index;
                    u32 size;
                    if (offset <= 0x3ff && length <= 0xa) {
                        size = 2;
                    } else if (offset <= 0x3fff && length <= 0x43) {
                        size = 3;
                    } else {
                        size = 4;
                    }
                    if (length + 4 - size > match_length + 4 - command_size) {
                        match_length = length;
                        match_offset = offset;
                        command_size = size;
                        if (length > 0x403) {
                            break;
                        }
                    }
                }
            }
            candidate_index = LinkArray[candidate_index & 0x3fff];
        }

        LinkArray[input_index & 0x3fff] = HashTable[hash];
        HashTable[hash] = input_index;

        if (command_size >= match_length) {
            ++source;
            --remaining;
            ++literal_count;
            continue;
        }

        while (literal_count > 3) {
            const u32 count = MIN(literal_count & ~3u, 0x70u);
            *output++ = (count >> 2) - 0x21;
            memmove(output, literal_start, count);
            output += count;
            literal_start += count;
            literal_count -= count;
        }

        if (command_size == 2) {
            *output++ = ((match_length - 3) << 2) + ((match_offset >> 8) << 5) + literal_count;
            *output++ = match_offset;
        } else if (command_size == 3) {
            *output++ = match_length + 0x7c;
            *output++ = (literal_count << 6) + (match_offset >> 8);
            *output++ = match_offset;
        } else {
            *output++ = 0xc0 + ((match_offset >> 16) << 4) + (((match_length - 5) >> 8) << 2) + literal_count;
            *output++ = match_offset >> 8;
            *output++ = match_offset;
            *output++ = match_length - 5;
        }

        memmove(output, literal_start, literal_count);
        output += literal_count;
        source += match_length;
        remaining -= match_length;
        literal_start = source;
        literal_count = 0;
    }

    while (literal_count > 3) {
        const u32 count = MIN(literal_count & ~3u, 0x70u);
        *output++ = (count >> 2) - 0x21;
        memmove(output, literal_start, count);
        output += count;
        literal_start += count;
        literal_count -= count;
    }
    *output++ = literal_count - 4;
    memmove(output, literal_start, literal_count);
    output += literal_count;
    return output - destination;
}
