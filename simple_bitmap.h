/* simple bitmap library
 * Author : darrenldl <dldldev@yahoo.com>
 * 
 * Version : 0.09
 * 
 * Note:
 *    simple bitmap is NOT thread safe
 * 
 * License:
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <http://unlicense.org/>
 */

#ifndef SIMPLE_BITMAP_H
#define SIMPLE_BITMAP_H

//#define SIMPLE_BITMAP_SILENT

//#define SIMPLE_BITMAP_SKIP_CHECK

//#define SIMPLE_BITMAP_META_DATA_SECURITY

#ifndef SIMPLE_BITMAP_SILENT
   #include <stdio.h>
#endif

#include <stdint.h>

#include <stddef.h>

#include <limits.h>

#ifdef SIMPLE_BITMAP_META_DATA_SECURITY
   #include "rand.h"
   #include <time.h>
   #include <string.h>
   #include <pthread.h>
#else
   #define bitmap_meta_encrypt(...)
   #define bitmap_meta_decrypt(...)
#endif

#include "simple_something_error.h"

#define get_bitmap_map_block_number(size_in_bits)   ((size_in_bits) / MAP_BLOCK_BIT + (((size_in_bits) % MAP_BLOCK_BIT) == 0 ? 0 : 1))
#define get_bitmap_map_block_index(bit_index)       ((bit_index) / (MAP_BLOCK_BIT))
#define get_bitmap_map_block_bit_index(bit_index)   ((bit_index) % (MAP_BLOCK_BIT))
#define get_bitmap_excess_bits(bit_index)           ((bit_index) % (MAP_BLOCK_BIT))

#define MAP_BLOCK_BIT   CHAR_BIT

typedef unsigned char map_block;    // map block must be unsigned
typedef struct simple_bitmap simple_bitmap;
typedef uint_fast32_t bit_index;
typedef struct bitmap_cont_group bitmap_cont_group;

struct simple_bitmap {
   map_block* base;
   map_block* end;
   bit_index length;
   
   bit_index number_of_zeros;
   bit_index number_of_ones;
   #ifdef SIMPLE_BITMAP_META_DATA_SECURITY
   uint32_t obj_rand_encrypt_xor_meta;
   uint32_t obj_rand_encrypt_add_meta;
   uint32_t obj_rand_encrypt_xor_meta2;
   uint32_t obj_rand_encrypt_add_meta2;
   
   #define CRT_OF 1
   uint_least8_t offsets;     // crypt order #1
   
   #define OFF_BA 0
   map_block* base_a[2];        // #2,   offset order #0
   #define OFF_EN 1
   map_block* end_a[2];         // #3,   #1
   #define OFF_LE 2
   bit_index length_a[2];       // #4,   #2
   
   uint32_t obj_cookie_meta;  // #5
   
   #define OFF_NZ 3
   bit_index number_of_zeros_a[2]; // #6,   #3
   #define OFF_NO 4
   bit_index number_of_ones_a[2];  // #7,   #4
   
   uint_least8_t rand_indicators;  // # 8, N/A
   uint_least8_t rand_degrees[5][2];
   //unsigned char rand_revert_to[5];
   
   #endif
};

struct bitmap_cont_group {
   map_block bit_type;
   bit_index start;
   bit_index length;
};

/* Scheme:
 *    randomise all keys, if unrandomised
 *    (obj_rand_encrypt_xor_meta + rand_encrypt_add_meta)
 *    XOR rand_encrypt_xor_meta + (obj_rand_encrypt_add_meta XOR rand_encrypt_xor_meta)
 *       is used to xor encrypt/decrypt the data
 */
#ifdef SIMPLE_BITMAP_META_DATA_SECURITY
uint_fast32_t  rand_encrypt_xor_meta;
uint_fast32_t  rand_encrypt_add_meta;
uint_fast32_t  rand_encrypt_xor_meta2;
uint_fast32_t  rand_encrypt_add_meta2;
uint_fast32_t  rand_cookie_correct_meta;
randctx s_b_rand_ctx;
unsigned char rand_things_init;
pthread_mutex_t rand_things_init_lock;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* default value :
 *    0 - overwrite space with 0s
 *    1 - overwrite space with 1s
 *   >1 - leave the space as it is
 */
int bitmap_init   (simple_bitmap* map, map_block* base, map_block* end, uint_fast32_t size_in_bits, map_block default_value);

#ifdef SIMPLE_BITMAP_META_DATA_SECURITY
int bitmap_meta_encrypt (simple_bitmap* map);
int bitmap_meta_decrypt (simple_bitmap* map);
#endif

int bitmap_data_check   (simple_bitmap* map);   // potentially expensive as it goes through entire bitmap

int bitmap_zero   (simple_bitmap* map);
int bitmap_one    (simple_bitmap* map);

// this function does not invoke malloc or any other means to obtain dynamic memory
// the rotatation is done in place with fixed static memory consumption
// this increases code complexity but avoids messing up programs' memory allocation scheme
/* Note:
 *    offset is in bits
 * 
 *    direction:
 *      >= 0 - shift to right
 *       < 0 - shift to left
 * 
 *    wrap_around means the same as rotate, if that makes more sense
 *    
 *    default value is only used when wrap around is not enabled
 * 
 *    If wrap around is not enabled,
 *    then the new space is replaced by 1 or 0 or nothing,
 *    depending on the default value(see below)
 *    
 *    default value:
 *       0 - overwrite space with 0s
 *       1 - overwrite space with 1s
 *      >1 - leave the space as it is
 * 
 *    The rotation algorithm used within has been verified formally(mathematically),
 *    the actual proof is provided in s_b_bitshift_proof.txt file(unix newline encoding)
 * 
 *    The rotation algorithm was devised independently, thus the style and proof
 *    may not match any of the implemention done by others
 */
int bitmap_shift  (simple_bitmap* map, bit_index offset, char direction, map_block default_val, unsigned char wrap_around);

int bitmap_not    (simple_bitmap* map);
int bitmap_and    (simple_bitmap* map1, simple_bitmap* map2, simple_bitmap* ret_map, unsigned char enforce_same_size);
int bitmap_or     (simple_bitmap* map1, simple_bitmap* map2, simple_bitmap* ret_map, unsigned char enforce_same_size);
int bitmap_xor    (simple_bitmap* map1, simple_bitmap* map2, simple_bitmap* ret_map, unsigned char enforce_same_size);

int bitmap_read   (simple_bitmap* map, uint_fast32_t index, map_block* result,     unsigned char no_auto_crypt);
int bitmap_write  (simple_bitmap* map, uint_fast32_t index, map_block input_value, unsigned char no_auto_crypt);

int bitmap_first_one_bit_index   (simple_bitmap* map, uint_fast32_t* result, bit_index skip_to_bit);
int bitmap_first_zero_bit_index  (simple_bitmap* map, uint_fast32_t* result, bit_index skip_to_bit);

int bitmap_first_one_cont_group     (simple_bitmap* map, bitmap_cont_group* ret_grp, bit_index skip_to_bit);
int bitmap_first_zero_cont_group    (simple_bitmap* map, bitmap_cont_group* ret_grp, bit_index skip_to_bit);

// backward version of the searching functions
/* Note:
 *    for index searching functions, the indexing follow as above ones(from left to right)
 *    for continuous group searching functions,
 *    the indexing and direction follow as above ones(from left to right)
 */
int bitmap_first_one_bit_index_back   (simple_bitmap* map, uint_fast32_t* result, bit_index skip_to_bit);
int bitmap_first_zero_bit_index_back  (simple_bitmap* map, uint_fast32_t* result, bit_index skip_to_bit);

int bitmap_first_one_cont_group_back     (simple_bitmap* map, bitmap_cont_group* ret_grp, bit_index skip_to_bit);
int bitmap_first_zero_cont_group_back    (simple_bitmap* map, bitmap_cont_group* ret_grp, bit_index skip_to_bit);

int bitmap_count_zeros_and_ones (simple_bitmap* map);

// both maps must be initialised
int bitmap_copy (simple_bitmap* src_map, simple_bitmap* dst_map, unsigned char allow_truncate, map_block default_value);

// no data checks, only copying
int bitmap_meta_copy (simple_bitmap* src_map, simple_bitmap* dst_map);

// map must be initialised
// this function does not handle memory management issues
/* default value :
 *    0 - overwrite space with 0s
 *    1 - overwrite space with 1s
 *   >1 - leave the space as it is
 */
int bitmap_grow (simple_bitmap* map, map_block* end, uint_fast32_t size_in_bits, map_block default_value);
int bitmap_shrink (simple_bitmap* map, map_block* end, uint_fast32_t size_in_bits);

int bitmap_show (simple_bitmap* map);
int bitmap_cont_group_show (bitmap_cont_group* grp);
int bitmap_raw_show (simple_bitmap* map);

#ifdef __cplusplus
}
#endif

#endif