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

#include "simple_bitmap.h"

#ifdef SIMPLE_BITMAP_SILENT
   #define printf(...)
#endif

#define s_b_min(a, b) ((a) < (b) ? (a) : (b))
#define s_b_max(a, b) ((a) > (b) ? (a) : (b))

#define s_b_encrypt(start, size, counter, key) do {\
               for (counter = 0; counter < size; counter++) {\
                  *((unsigned char*) start + counter) ^= (unsigned char) (key >> ((counter % 4) * CHAR_BIT));\
               }\
            } while (0)
#define s_b_decrypt(start, size, counter, key) s_b_encrypt(start, size, counter, key)

#define s_b_rand_fill(start, size, counter, rand_ctx, temp) do {\
               for (counter = 0; counter < size; counter++) {\
                  if (counter % 4 == 0) {temp = rand(&rand_ctx);}\
                  *((unsigned char*) start + counter) = (unsigned char) (temp >> ((counter % 4) * CHAR_BIT));\
               }\
            } while (0)

#define s_b_val_fill(start, size, counter, val) do {\
               for (counter = 0; counter < size; counter++) {\
                  *((unsigned char*) start + counter) = (unsigned char) (val);\
               }\
            } while (0)

int bitmap_init (simple_bitmap* map, map_block* base, map_block* end, uint_fast32_t size_in_bits, map_block default_value) {
   int ret_temp;
   
   #ifdef SIMPLE_BITMAP_META_DATA_SECURITY
   uint_least8_t offsets;
   
   unsigned char count = 0;
   
   uint_fast32_t temp;
   
   time_t timer;
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_init : map is NULL\n");
      return WRONG_INPUT;
   }
   #endif
   
   // setup global random things
   if (!rand_things_init) {
      pthread_mutex_lock(&rand_things_init_lock);
      if (!rand_things_init) {
         // initialise randomness
         time(&timer);
         
         for (count = 0; count < sizeof(timer); count++) {
            s_b_rand_ctx.randrsl[count] = (timer >> (count*8))&((unsigned char) -1);
         }
         
         randinit(&s_b_rand_ctx);
         
         rand_encrypt_xor_meta = rand(&s_b_rand_ctx);
         rand_encrypt_add_meta = rand(&s_b_rand_ctx);
         rand_encrypt_xor_meta2 = rand(&s_b_rand_ctx);
         rand_encrypt_add_meta2 = rand(&s_b_rand_ctx);
         rand_cookie_correct_meta = rand(&s_b_rand_ctx);
      }
      rand_things_init = 1;
      pthread_mutex_unlock(&rand_things_init_lock);
   }
   
   map->obj_rand_encrypt_xor_meta = rand(&s_b_rand_ctx);
   map->obj_rand_encrypt_add_meta = rand(&s_b_rand_ctx);
   map->obj_rand_encrypt_xor_meta2 = rand(&s_b_rand_ctx);
   map->obj_rand_encrypt_add_meta2 = rand(&s_b_rand_ctx);
   map->offsets                   = rand(&s_b_rand_ctx);
   map->obj_cookie_meta           = rand(&s_b_rand_ctx);
   
   offsets = map->offsets;
   #endif
   
   // input check
   if (end == NULL) {
      #ifndef SIMPLE_BITMAP_SKIP_CHECK
      if (size_in_bits == 0) {
         printf("bitmap_init : end is NULL but size is 0 as well\n");
         return WRONG_INPUT;
      }
      #endif
      
      map->end = base + get_bitmap_map_block_index(size_in_bits-1);
      
      map->length = size_in_bits;
   }
   else {
      map->end = end;
      map->length = (end - base + 1) * MAP_BLOCK_BIT;
   }
   
   map->base = base;
   
   bitmap_meta_encrypt(map);
   
   if (default_value > 1) {
      // ;  // do nothing
      return bitmap_count_zeros_and_ones(map);
   }
   else if (default_value) {
      return bitmap_one(map);
   }
   else {
      return bitmap_zero(map);
   }
   
   return 0;
}

#ifdef SIMPLE_BITMAP_META_DATA_SECURITY
int bitmap_meta_encrypt (simple_bitmap* map) {
   uint32_t key;
   uint32_t key2;
   
   unsigned char count;
   unsigned char count2;
   
   uint_least8_t offsets;
   
   uint_least8_t rand_indicators;
   
   uint_fast32_t temp;
   
   unsigned char* cur1,* cur2;
   
   unsigned char rand_degrees[5][2];
   //unsigned char rand_revert_to[5];
   
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_meta_encrypt : map is NULL\n");
      return WRONG_INPUT;
   }
   #endif
   
   key = (map->obj_rand_encrypt_xor_meta + rand_encrypt_add_meta)
         ^ rand_encrypt_xor_meta + (map->obj_rand_encrypt_add_meta ^ rand_encrypt_xor_meta);
         
   key2 = (map->obj_rand_encrypt_xor_meta2 + rand_encrypt_add_meta2)
         ^ rand_encrypt_xor_meta2 + (map->obj_rand_encrypt_add_meta2 ^ rand_encrypt_xor_meta2);
   
   count = 0;
   
   offsets = map->offsets;
   
   s_b_encrypt(&map->offsets, sizeof(map->offsets), count, key);
   
   // copy values over to secure data fields
   map->base_a[((offsets >> OFF_BA)&0x1)]    = map->base;
   map->end_a[((offsets >> OFF_EN)&0x1)]     = map->end;
   map->length_a[((offsets >> OFF_LE)&0x1)]  = map->length;
   map->number_of_zeros_a[((offsets >> OFF_NZ)&0x1)]  = map->number_of_zeros;
   map->number_of_ones_a[((offsets >> OFF_NO)&0x1)]   = map->number_of_ones;
   
   // remove pattern that can cause major weakness
   rand_indicators = 0;
   // handle base and end addresses
   rand_degrees[OFF_BA][0] = 0xFF;
   rand_degrees[OFF_BA][1] = 0;
   rand_degrees[OFF_EN][0] = 0xFF;
   rand_degrees[OFF_EN][1] = 0;
   for (count = 0, cur1 = (unsigned char*) &map->base_a[((offsets >> OFF_BA)&0x1)], cur2 = (unsigned char*) &map->end_a[((offsets >> OFF_EN)&0x1)];
         count < sizeof(map->base_a[0]);
         count++, cur1++, cur2++) {
      if (*cur1 == 0x00) {
         if (rand_degrees[OFF_BA][0] == 0xFF) {
            rand_degrees[OFF_BA][0] = count;
         }
         rand_degrees[OFF_BA][1] = count;
      }
      if (*cur2 == 0x00) {
         if (rand_degrees[OFF_EN][0] == 0xFF) {
            rand_degrees[OFF_EN][0] = count;
         }
         rand_degrees[OFF_EN][1] = count;
      }
   }
   if (rand_degrees[OFF_BA][1] - rand_degrees[OFF_BA][0] + 1 > 0) {
      rand_indicators |= 0x1 << OFF_BA;
   }
   if (rand_degrees[OFF_EN][1] - rand_degrees[OFF_EN][0] + 1 > 0) {
      rand_indicators |= 0x1 << OFF_EN;
   }
   rand_degrees[OFF_LE][0] = 0xFF;
   rand_degrees[OFF_LE][1] = 0;
   for (count = 0, cur1 = (unsigned char*) &map->length_a[((offsets >> OFF_LE)&0x1)];
         count < sizeof(map->length_a[0]);
         count++, cur1++) {
      if (*cur1 == 0x00) {
         if (rand_degrees[OFF_LE][0] == 0xFF) {
            rand_degrees[OFF_LE][0] = count;
         }
         rand_degrees[OFF_LE][1] = count;
      }
   }
   if (rand_degrees[OFF_LE][1] - rand_degrees[OFF_LE][0] + 1 > 0) {
      rand_indicators |= 0x1 << OFF_LE;
   }
   rand_degrees[OFF_NZ][0] = 0xFF;
   rand_degrees[OFF_NZ][1] = 0;
   rand_degrees[OFF_NO][0] = 0xFF;
   rand_degrees[OFF_NO][1] = 0;
   for (count = 0, cur1 = (unsigned char*) &map->number_of_zeros_a[((offsets >> OFF_NZ)&0x1)], cur2 = (unsigned char*) &map->number_of_ones_a[((offsets >> OFF_NO)&0x1)];
         count < sizeof(map->number_of_zeros_a[0]);
         count++, cur1++, cur2++) {
      if (*cur1 == 0x00) {
         if (rand_degrees[OFF_NZ][0] == 0xFF) {
            rand_degrees[OFF_NZ][0] = count;
         }
         rand_degrees[OFF_NZ][1] = count;
      }
      if (*cur2 == 0x00) {
         if (rand_degrees[OFF_NO][0] == 0xFF) {
            rand_degrees[OFF_NO][0] = count;
         }
         rand_degrees[OFF_NO][1] = count;
      }
   }
   if (rand_degrees[OFF_NZ][1] - rand_degrees[OFF_NZ][0] + 1 > 0) {
      rand_indicators |= 0x1 << OFF_NZ;
   }
   if (rand_degrees[OFF_NO][1] - rand_degrees[OFF_NO][0] + 1 > 0) {
      rand_indicators |= 0x1 << OFF_NO;
   }
   
   s_b_rand_fill((unsigned char*) (&map->base_a[((offsets >> OFF_BA)&0x1)]) + rand_degrees[OFF_BA][0],
                  rand_degrees[OFF_BA][1] - rand_degrees[OFF_BA][0] + 1, count, s_b_rand_ctx, temp);
   s_b_rand_fill((unsigned char*) (&map->end_a[((offsets >> OFF_EN)&0x1)]) + rand_degrees[OFF_EN][0],
                  rand_degrees[OFF_EN][1] - rand_degrees[OFF_EN][0] + 1, count, s_b_rand_ctx, temp);
   s_b_rand_fill((unsigned char*) (&map->number_of_zeros_a[((offsets >> OFF_NZ)&0x1)]) + rand_degrees[OFF_NZ][0],
                  rand_degrees[OFF_NZ][1] - rand_degrees[OFF_NZ][0] + 1, count, s_b_rand_ctx, temp);
   s_b_rand_fill((unsigned char*) (&map->number_of_ones_a[((offsets >> OFF_NO)&0x1)]) + rand_degrees[OFF_NO][0],
                  rand_degrees[OFF_NO][1] - rand_degrees[OFF_NO][0] + 1, count, s_b_rand_ctx, temp);
   
   map->rand_indicators = rand_indicators;
   for (count = 0; count < 5; count++) {
      if ((rand_indicators >> count)&0x1) {
         map->rand_degrees[count][0] = rand_degrees[count][0];
         map->rand_degrees[count][1] = rand_degrees[count][1];
      }
      else {
         s_b_rand_fill(&map->rand_degrees[count][0], sizeof(map->rand_degrees[0][0]), count2, s_b_rand_ctx, temp);
         s_b_rand_fill(&map->rand_degrees[count][1], sizeof(map->rand_degrees[0][0]), count2, s_b_rand_ctx, temp);
      }
      rand_degrees[count][0] = 0;
      rand_degrees[count][1] = 0;
   }
   
   // encrypt all secure data fields
   s_b_encrypt(&map->base_a[((offsets >> OFF_BA)&0x1)], sizeof(map->base_a[0]), count, key);
   s_b_encrypt(&map->end_a[((offsets >> OFF_EN)&0x1)], sizeof(map->end_a[0]), count, key);
   s_b_encrypt(&map->length_a[((offsets >> OFF_LE)&0x1)], sizeof(map->length_a[0]), count, key);
   s_b_encrypt(&map->number_of_zeros_a[((offsets >> OFF_NZ)&0x1)], sizeof(map->number_of_zeros_a[0]), count, key);
   s_b_encrypt(&map->number_of_ones_a[((offsets >> OFF_NO)&0x1)], sizeof(map->number_of_ones_a[0]), count, key);
   
   s_b_encrypt(&map->rand_indicators, sizeof(map->rand_indicators), count, key2);
   s_b_encrypt(&map->rand_degrees, sizeof(map->rand_degrees), count, key2);
   
   // fill dummy slots
   s_b_rand_fill(&map->base_a[!((offsets >> OFF_BA)&0x1)], sizeof(map->base_a[0]), count, s_b_rand_ctx, temp);
   s_b_rand_fill(&map->end_a[!((offsets >> OFF_EN)&0x1)], sizeof(map->end_a[0]), count, s_b_rand_ctx, temp);
   s_b_rand_fill(&map->length_a[!((offsets >> OFF_LE)&0x1)], sizeof(map->length_a[0]), count, s_b_rand_ctx, temp);
   s_b_rand_fill(&map->number_of_zeros_a[!((offsets >> OFF_NZ)&0x1)], sizeof(map->number_of_zeros_a[0]), count, s_b_rand_ctx, temp);
   s_b_rand_fill(&map->number_of_ones_a[!((offsets >> OFF_NO)&0x1)], sizeof(map->number_of_ones_a[0]), count, s_b_rand_ctx, temp);
   
   // fill unused data slots
   s_b_rand_fill(&map->base, sizeof(map->base), count, s_b_rand_ctx, temp);
   s_b_rand_fill(&map->end, sizeof(map->end), count, s_b_rand_ctx, temp);
   s_b_rand_fill(&map->length, sizeof(map->length), count, s_b_rand_ctx, temp);
   s_b_rand_fill(&map->number_of_zeros, sizeof(map->number_of_zeros), count, s_b_rand_ctx, temp);
   s_b_rand_fill(&map->number_of_ones, sizeof(map->number_of_ones), count, s_b_rand_ctx, temp);
   
   // clear stack
   key = 0;
   key2 = 0;
   offsets = 0;
   
   return 0;
}

int bitmap_meta_decrypt (simple_bitmap* map) {
   uint32_t key;
   uint32_t key2;
   
   unsigned char count;
   
   uint_least8_t offsets;
   
   uint_least8_t rand_indicators;
   
   unsigned char rand_degrees[5][2];
   //unsigned char rand_revert_to[5];
   
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_meta_encrypt : map is NULL\n");
      return WRONG_INPUT;
   }
   #endif
   
   key = (map->obj_rand_encrypt_xor_meta + rand_encrypt_add_meta)
         ^ rand_encrypt_xor_meta + (map->obj_rand_encrypt_add_meta ^ rand_encrypt_xor_meta);
         
   key2 = (map->obj_rand_encrypt_xor_meta2 + rand_encrypt_add_meta2)
         ^ rand_encrypt_xor_meta2 + (map->obj_rand_encrypt_add_meta2 ^ rand_encrypt_xor_meta2);
   
   count = 0;
   
   s_b_decrypt(&map->offsets, sizeof(map->offsets), count, key);
   
   offsets = map->offsets;
   
   s_b_decrypt(&map->rand_indicators, sizeof(map->rand_indicators), count, key2);
   s_b_decrypt(&map->rand_degrees, sizeof(map->rand_degrees), count, key2);
   
   rand_indicators = map->rand_indicators;
   for (count = 0; count < 5; count++) {
      rand_degrees[count][0] = map->rand_degrees[count][0];
      rand_degrees[count][1] = map->rand_degrees[count][1];
   }
   
   // decrypt all data fields
   s_b_decrypt(&map->base_a[((offsets >> OFF_BA)&0x1)], sizeof(map->base_a[0]), count, key);
   s_b_decrypt(&map->end_a[((offsets >> OFF_EN)&0x1)], sizeof(map->end_a[0]), count, key);
   s_b_decrypt(&map->length_a[((offsets >> OFF_LE)&0x1)], sizeof(map->length_a[0]), count, key);
   s_b_decrypt(&map->number_of_zeros_a[((offsets >> OFF_NZ)&0x1)], sizeof(map->number_of_zeros_a[0]), count, key);
   s_b_decrypt(&map->number_of_ones_a[((offsets >> OFF_NO)&0x1)], sizeof(map->number_of_ones_a[0]), count, key);
   
   // revert patterns
   if ((map->rand_indicators >> OFF_BA)&0x1) {
      s_b_val_fill((unsigned char*) (&map->base_a[((offsets >> OFF_BA)&0x1)]) + rand_degrees[OFF_BA][0],
                     rand_degrees[OFF_BA][1] - rand_degrees[OFF_BA][0] + 1, count, 0x00);
   }
   if ((map->rand_indicators >> OFF_EN)&0x1) {
      s_b_val_fill((unsigned char*) (&map->end_a[((offsets >> OFF_EN)&0x1)]) + rand_degrees[OFF_EN][0], 
                     rand_degrees[OFF_EN][1] - rand_degrees[OFF_EN][0] + 1, count, 0x00);
   }
   if ((map->rand_indicators >> OFF_LE)&0x1) {
      s_b_val_fill((unsigned char*) (&map->length_a[((offsets >> OFF_LE)&0x1)]) + rand_degrees[OFF_LE][0], 
                     rand_degrees[OFF_LE][1] - rand_degrees[OFF_LE][0] + 1, count, 0x00);
   }
   if ((map->rand_indicators >> OFF_NZ)&0x1) {
      s_b_val_fill((unsigned char*) (&map->number_of_zeros_a[((offsets >> OFF_NZ)&0x1)]) + rand_degrees[OFF_NZ][0], 
                     rand_degrees[OFF_NZ][1] - rand_degrees[OFF_NZ][0] + 1, count, 0x00);
   }
   if ((map->rand_indicators >> OFF_NO)&0x1) {
      s_b_val_fill((unsigned char*) (&map->number_of_ones_a[((offsets >> OFF_NO)&0x1)]) + rand_degrees[OFF_NO][0], 
                     rand_degrees[OFF_NO][1] - rand_degrees[OFF_NO][0] + 1, count, 0x00);
   }
   
   // copy from secure data fields
   map->base      = map->base_a[((offsets >> OFF_BA)&0x1)];
   map->end       = map->end_a[((offsets >> OFF_EN)&0x1)];
   map->length    = map->length_a[((offsets >> OFF_LE)&0x1)];
   map->number_of_zeros = map->number_of_zeros_a[((offsets >> OFF_NZ)&0x1)];
   map->number_of_ones  = map->number_of_ones_a[((offsets >> OFF_NO)&0x1)];
   
   // clear stack
   key = 0;
   key2 = 0;
   offsets = 0;
   
   return 0;
}
#endif

int bitmap_zero (simple_bitmap* map) {
   map_block* cur;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_zero : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_zero : base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->end == NULL) {
      printf("bitmap_zero : end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->length == 0) {
      printf("bitmap_zero : map has no length\n");
      return CORRUPTED_DATA;
   }
   #endif
   
   // write 0s
   for (cur = map->base; cur <= map->end; cur++) {
      *cur = 0;
   }
   
   map->number_of_zeros = map->length;
   map->number_of_ones = 0;
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_one (simple_bitmap* map) {
   map_block mask;
   
   map_block buf;
   
   map_block* cur;
   
   unsigned char count;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_one : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_one : base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->end == NULL) {
      printf("bitmap_one : end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->length == 0) {
      printf("bitmap_one : map has no length\n");
      return CORRUPTED_DATA;
   }
   #endif
   
   // write 1s
   for (cur = map->base; cur <= map->end; cur++) {
      *cur = (map_block) -1;
   }
   
   cur = map->end;
   
   // set the extra bits back to 0
   mask = 0;
   for (count = 0; count <= get_bitmap_map_block_bit_index(map->length-1); count++) {
      mask |= 0x1 << (MAP_BLOCK_BIT - count - 1);
   }
   *cur &= mask;
   
   map->number_of_zeros = 0;
   map->number_of_ones = map->length;
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

static int map_blocks_ferris_wheel_flip (map_block* start, map_block* end, uint_fast32_t count) {
   map_block* start1 = start;
   map_block* start2 = end - count + 1;
   map_block temp;
   
   for (; start2 <= end; start1++, start2++) {
      temp = *start2;
      *start2 = *start1;
      *start1 = temp;
   }
   
   return 0;
}

int bitmap_shift (simple_bitmap* map, bit_index offset, char direction, map_block default_val, unsigned char wrap_around) {
   map_block* start;
   map_block* end;
   
   map_block* end2;
   
   map_block mask_clear;
   
   map_block temp;
   
   uint_fast16_t blocks_to_shift;
   
   unsigned char bits_to_shift;
   
   unsigned char count;
   
   char shrink_direction;
   
   uint_fast32_t move_count;
   
   map_block* start1;
   
   map_block* cur;
   
   map_block* temp_end;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_shift : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_shift : base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->end == NULL) {
      printf("bitmap_shift : end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->length == 0) {
      printf("bitmap_shift : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_shift : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_shift : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   #endif
   
   blocks_to_shift = offset / MAP_BLOCK_BIT;
   
   bits_to_shift = offset % MAP_BLOCK_BIT;
   
   if (blocks_to_shift == 0) {
      goto bitshift;
   }
   
   if (!wrap_around) {  // no need to rotate
      if (blocks_to_shift >= get_bitmap_map_block_number(map->length)) {
         if (default_val > 1) {
            ; // do nothing
         }
         else if (default_val) {
            bitmap_one(map);
         }
         else {
            bitmap_zero(map);
         }
         return 0;
      }
      
      if (direction >= 0) {
         start = map->end - blocks_to_shift;
         start1 = map->end;
         
         for (; start1 >= map->base; start--, start1--) {
            *start1 = *start;
         }
         
         start = map->base + blocks_to_shift - 1;
         
         // overwrite remaining blocks with default value
         if (default_val > 1) {
            ; // do nothing
         }
         else {
            if (default_val == 1) {
               temp = (map_block) -1;
            }
            else {
               temp = 0;
            }
            for (; start >= map->base; start--) {
               *start = temp;
            }
         }
      }
      else {
         start = map->base;
         start1 = map->base + blocks_to_shift;
         
         for (; start1 <= map->end; start++, start1++) {
            *start = *start1;
         }
         
         start = map->end - blocks_to_shift + 1;
         
         // overwrite remaining blocks with default value
         if (default_val > 1) {
            ; // do nothing
         }
         else {
            if (default_val == 1) {
               temp = (map_block) -1;
            }
            else {
               temp = 0;
            }
            for (; start <= map->end; start++) {
               *start = temp;
            }
         }
      }
   }
   else {      // need to rotate
      if (blocks_to_shift >= map->length) {
         blocks_to_shift = blocks_to_shift % get_bitmap_map_block_number(map->length);
      }
      
      start = map->base;
      end = map->end;
      
      move_count = s_b_min(blocks_to_shift, get_bitmap_map_block_number(map->length) - blocks_to_shift);
      
      // main rotation algorithm begins
      if (direction >= 0) {
         if (move_count == blocks_to_shift) {
            shrink_direction = 1;
         }
         else {
            shrink_direction = -1;
         }
      }
      else {
         if (move_count == blocks_to_shift) {
            shrink_direction = -1;
         }
         else {
            shrink_direction = 1;
         }
      }
      
      while (start < end) {
         map_blocks_ferris_wheel_flip(start, end, move_count);
         
         if (shrink_direction == 1) {
            start += move_count;
         }
         else {
            end -= move_count;
         }
         
         if (start - end < 2 * move_count) { // need to flip direction and reduce unit
            shrink_direction *= -1;
            // move_count = min(move_count, start - end + 1 - move_count);
            move_count = start - end + 1 - move_count;
         }
      }
      // main rotation algorithm ends
      
      // handle the bitwise mismatching
      if (get_bitmap_excess_bits(map->length)) {
         if (direction >= 0) {   // shift to right
            // handle the remaining unshifted blocks
            for (cur = map->base+blocks_to_shift-1;
                  cur > map->base; cur--) {
               temp = *(cur-1) << get_bitmap_excess_bits(map->length);
               
               *cur >>= MAP_BLOCK_BIT - get_bitmap_excess_bits(map->length);
               *cur |= temp;
            }
            
            // handle the last block
            temp = (*(map->end) << get_bitmap_excess_bits(map->length));
            
            *(map->base) >>= MAP_BLOCK_BIT - get_bitmap_excess_bits(map->length);
            *(map->base) |= temp;
         }
         else {      // shift to left
            // handle the previously last block
            temp = *(map->end-blocks_to_shift+1) >> get_bitmap_excess_bits(map->length);
            
            *(map->end-blocks_to_shift) |= temp;
            
            // shift remaining blocks to left including the currently last block
            for (cur = map->end-blocks_to_shift+1;
                  cur < map->end; cur++) {
               temp = *(cur+1) >> get_bitmap_excess_bits(map->length);
               
               *cur <<= MAP_BLOCK_BIT - get_bitmap_excess_bits(map->length);
               *cur |= temp;
            }
            
            // handle the last block
            *(map->end) <<= MAP_BLOCK_BIT - get_bitmap_excess_bits(map->length);
         }
      }
   }
   
   bitmap_count_zeros_and_ones(map);
   
   bitshift:
   if (bits_to_shift == 0) {
      return 0;
   }
   
   if (!wrap_around) {  // no need to recycle
      if (direction >= 0) {   // shift to right
         for (cur = map->end; cur > map->base; cur--) {
            *cur >>= bits_to_shift;
            
            *cur |= (*(cur-1) << (MAP_BLOCK_BIT - bits_to_shift));
         }
         
         *(map->base) >>= bits_to_shift;
      }
      else {      // shift to left
         for (cur = map->base; cur < map->end; cur++) {
            *cur <<= bits_to_shift;
            
            *cur |= (*(cur+1) >> (MAP_BLOCK_BIT - bits_to_shift));
         }
         
         *(map->end) <<= bits_to_shift;
      }
   }
   else {   // need to rotate
      if (direction >= 0) {   // shift to right
         temp = *(map->end);
         
         for (cur = map->end; cur > map->base; cur--) {
            *cur >>= bits_to_shift;
            
            *cur |= (*(cur-1) << (MAP_BLOCK_BIT - bits_to_shift));
         }
         
         *(map->base) >>= bits_to_shift;
         
         // piece the previous last block into the start first
         if (bits_to_shift < get_bitmap_map_block_bit_index(map->length-1)+1) {
            temp <<= get_bitmap_excess_bits(map->length) - bits_to_shift;
         }
         else {
            temp >>= bits_to_shift - get_bitmap_excess_bits(map->length);
            temp |= *(map->end) << get_bitmap_excess_bits(map->length);
         }
         
         *(map->base) |= temp;
      }
      else {      // shift to left
         temp = *(map->base);
         
         for (cur = map->base; cur < map->end; cur++) {
            *cur <<= bits_to_shift;
            
            *cur |= (*(cur+1) >> (MAP_BLOCK_BIT - bits_to_shift));
         }
         
         *(map->end) <<= bits_to_shift;
         
         // piece the previous first block into the end
         if (bits_to_shift < get_bitmap_excess_bits(map->length)) {
            temp >>= get_bitmap_excess_bits(map->length) - bits_to_shift;
            
            *(map->end) |= temp;
         }
         else {
            *(map->end-1) |= temp >> (MAP_BLOCK_BIT - (bits_to_shift - get_bitmap_excess_bits(map->length)));
            *(map->end) |= temp << (bits_to_shift - get_bitmap_excess_bits(map->length));
         }
      }
   }
   
   bitmap_count_zeros_and_ones(map);
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_not (simple_bitmap* map) {
   map_block* cur;
   
   unsigned char count;
   
   map_block mask;
   
   bit_index temp;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_not : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_not : base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->end == NULL) {
      printf("bitmap_not : end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->length == 0) {
      printf("bitmap_not : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_not : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_not : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   #endif
   
   // flip bits
   for (cur = map->base; cur <= map->end; cur++) {
      *cur = ~(*cur);
   }
   
   // clean up the edge
   mask = 0;
   for (count = 0; count <= get_bitmap_map_block_bit_index(map->length-1); count++) {
      mask |= 0x1 << (MAP_BLOCK_BIT - count - 1);
   }
   *(map->end) &= mask;
   
   // switch numbers
   temp = map->number_of_ones;
   map->number_of_ones = map->number_of_zeros;
   map->number_of_zeros = temp;
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_and (simple_bitmap* map1, simple_bitmap* map2, simple_bitmap* ret_map, unsigned char enforce_same_size) {
   map_block* cur1;
   map_block* cur2;
   map_block* cur_ret;
   
   unsigned char count;
   
   map_block mask;
   
   bit_index temp;
   
   bitmap_meta_decrypt(map1);
   bitmap_meta_decrypt(map2);
   bitmap_meta_decrypt(ret_map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map1 == NULL) {
      printf("bitmap_and : map1 is NULL\n");
      return WRONG_INPUT;
   }
   if (map1->base == NULL) {
      printf("bitmap_and : map1->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map1->end == NULL) {
      printf("bitmap_and : map1->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map1->length == 0) {
      printf("bitmap_and : map1 has no length\n");
      return CORRUPTED_DATA;
   }
   if (map1->base + get_bitmap_map_block_index(map1->length-1) != map1->end) {
      printf("bitmap_and : map1 : is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map1->number_of_zeros + map1->number_of_ones != map1->length) {
      printf("bitmap_and : map1 : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (map2 == NULL) {
      printf("bitmap_and : map2 is NULL\n");
      return WRONG_INPUT;
   }
   if (map2->base == NULL) {
      printf("bitmap_and : map2->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map2->end == NULL) {
      printf("bitmap_and : map2->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map2->length == 0) {
      printf("bitmap_and : map2 has no length\n");
      return CORRUPTED_DATA;
   }
   if (map2->base + get_bitmap_map_block_index(map2->length-1) != map2->end) {
      printf("bitmap_and : map2 : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map2->number_of_zeros + map2->number_of_ones != map2->length) {
      printf("bitmap_and : map2 : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (ret_map == NULL) {
      printf("bitmap_and : ret_map is NULL\n");
      return WRONG_INPUT;
   }
   if (ret_map->base == NULL) {
      printf("bitmap_and : ret_map->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->end == NULL) {
      printf("bitmap_and : ret_map->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->length == 0) {
      printf("bitmap_and : ret_map has no length\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->base + get_bitmap_map_block_index(ret_map->length-1) != ret_map->end) {
      printf("bitmap_and : ret_map : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->number_of_zeros + ret_map->number_of_ones != ret_map->length) {
      printf("bitmap_and : ret_map : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   
   if (enforce_same_size) {
      if (!(map1->length == map2->length == ret_map->length)) {
         printf("bitmap_and : map1 and map2 have different sizes\n");
         return WRONG_INPUT;
      }
   }
   #endif
   
   // do AND bitwise operation
   for (cur1 = map1->base, cur2 = map2->base, cur_ret = ret_map->base;
         cur1 <= map1->end || cur2 <= map2->end || cur_ret <= ret_map->end;
         cur1++, cur2++, cur_ret++) {
      *cur_ret = *cur1 & *cur2;
   }
   
   bitmap_count_zeros_and_ones(ret_map);
   
   bitmap_meta_encrypt(map1);
   bitmap_meta_encrypt(map2);
   bitmap_meta_encrypt(ret_map);
   
   return 0;
}

int bitmap_or (simple_bitmap* map1, simple_bitmap* map2, simple_bitmap* ret_map, unsigned char enforce_same_size) {
   map_block* cur1;
   map_block* cur2;
   map_block* cur_ret;
   
   unsigned char count;
   
   map_block mask;
   
   bit_index temp;
   
   bitmap_meta_decrypt(map1);
   bitmap_meta_decrypt(map2);
   bitmap_meta_decrypt(ret_map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map1 == NULL) {
      printf("bitmap_or : map1 is NULL\n");
      return WRONG_INPUT;
   }
   if (map1->base == NULL) {
      printf("bitmap_or : map1->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map1->end == NULL) {
      printf("bitmap_or : map1->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map1->length == 0) {
      printf("bitmap_or : map1 has no length\n");
      return CORRUPTED_DATA;
   }
   if (map1->base + get_bitmap_map_block_index(map1->length-1) != map1->end) {
      printf("bitmap_or : map1 : is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map1->number_of_zeros + map1->number_of_ones != map1->length) {
      printf("bitmap_or : map1 : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (map2 == NULL) {
      printf("bitmap_or : map2 is NULL\n");
      return WRONG_INPUT;
   }
   if (map2->base == NULL) {
      printf("bitmap_or : map2->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map2->end == NULL) {
      printf("bitmap_or : map2->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map2->length == 0) {
      printf("bitmap_or : map2 has no length\n");
      return CORRUPTED_DATA;
   }
   if (map2->base + get_bitmap_map_block_index(map2->length-1) != map2->end) {
      printf("bitmap_or : map2 : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map2->number_of_zeros + map2->number_of_ones != map2->length) {
      printf("bitmap_or : map2 : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (ret_map == NULL) {
      printf("bitmap_or : ret_map is NULL\n");
      return WRONG_INPUT;
   }
   if (ret_map->base == NULL) {
      printf("bitmap_or : ret_map->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->end == NULL) {
      printf("bitmap_or : ret_map->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->length == 0) {
      printf("bitmap_or : ret_map has no length\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->base + get_bitmap_map_block_index(ret_map->length-1) != ret_map->end) {
      printf("bitmap_or : ret_map : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->number_of_zeros + ret_map->number_of_ones != ret_map->length) {
      printf("bitmap_or : ret_map : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   
   if (enforce_same_size) {
      if (!(map1->length == map2->length == ret_map->length)) {
         printf("bitmap_or : map1 and map2 have different sizes\n");
         return WRONG_INPUT;
      }
   }
   #endif
   
   // do OR bitwise operation
   for (cur1 = map1->base, cur2 = map2->base, cur_ret = ret_map->base;
         cur1 <= map1->end || cur2 <= map2->end || cur_ret <= ret_map->end;
         cur1++, cur2++, cur_ret++) {
      *cur_ret = *cur1 | *cur2;
   }
   
   bitmap_count_zeros_and_ones(ret_map);
   
   bitmap_meta_encrypt(map1);
   bitmap_meta_encrypt(map2);
   bitmap_meta_encrypt(ret_map);
   
   return 0;
}

int bitmap_xor (simple_bitmap* map1, simple_bitmap* map2, simple_bitmap* ret_map, unsigned char enforce_same_size) {
   map_block* cur1;
   map_block* cur2;
   map_block* cur_ret;
   
   unsigned char count;
   
   map_block mask;
   
   bit_index temp;
   
   bitmap_meta_decrypt(map1);
   bitmap_meta_decrypt(map2);
   bitmap_meta_decrypt(ret_map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map1 == NULL) {
      printf("bitmap_xor : map1 is NULL\n");
      return WRONG_INPUT;
   }
   if (map1->base == NULL) {
      printf("bitmap_xor : map1->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map1->end == NULL) {
      printf("bitmap_xor : map1->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map1->length == 0) {
      printf("bitmap_xor : map1 has no length\n");
      return CORRUPTED_DATA;
   }
   if (map1->base + get_bitmap_map_block_index(map1->length-1) != map1->end) {
      printf("bitmap_xor : map1 : is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map1->number_of_zeros + map1->number_of_ones != map1->length) {
      printf("bitmap_xor : map1 : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (map2 == NULL) {
      printf("bitmap_xor : map2 is NULL\n");
      return WRONG_INPUT;
   }
   if (map2->base == NULL) {
      printf("bitmap_xor : map2->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map2->end == NULL) {
      printf("bitmap_xor : map2->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map2->length == 0) {
      printf("bitmap_xor : map2 has no length\n");
      return CORRUPTED_DATA;
   }
   if (map2->base + get_bitmap_map_block_index(map2->length-1) != map2->end) {
      printf("bitmap_xor : map2 : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map2->number_of_zeros + map2->number_of_ones != map2->length) {
      printf("bitmap_xor : map2 : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (ret_map == NULL) {
      printf("bitmap_xor : ret_map is NULL\n");
      return WRONG_INPUT;
   }
   if (ret_map->base == NULL) {
      printf("bitmap_xor : ret_map->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->end == NULL) {
      printf("bitmap_xor : ret_map->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->length == 0) {
      printf("bitmap_xor : ret_map has no length\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->base + get_bitmap_map_block_index(ret_map->length-1) != ret_map->end) {
      printf("bitmap_xor : ret_map : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (ret_map->number_of_zeros + ret_map->number_of_ones != ret_map->length) {
      printf("bitmap_xor : ret_map : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   
   if (enforce_same_size) {
      if (!(map1->length == map2->length == ret_map->length)) {
         printf("bitmap_xor : map1 and map2 have different sizes\n");
         return WRONG_INPUT;
      }
   }
   #endif
   
   // do XOR bitwise operation
   for (cur1 = map1->base, cur2 = map2->base, cur_ret = ret_map->base;
         cur1 <= map1->end || cur2 <= map2->end || cur_ret <= ret_map->end;
         cur1++, cur2++, cur_ret++) {
      *cur_ret = *cur1 ^ *cur2;
   }
   
   bitmap_count_zeros_and_ones(ret_map);
   
   bitmap_meta_encrypt(map1);
   bitmap_meta_encrypt(map2);
   bitmap_meta_encrypt(ret_map);
   
   return 0;
}

int bitmap_read (simple_bitmap* map, bit_index index, map_block* result, unsigned char no_auto_crypt) {
   map_block* cur;
   
   uint_fast32_t block_index;
   unsigned char bit_indx;
   
   map_block mask;
   
   map_block buf;
   
   if (!no_auto_crypt) {
      bitmap_meta_decrypt(map);
   }
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_read : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_read : base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->end == NULL) {
      printf("bitmap_read : end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->length == 0) {
      printf("bitmap_read : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_read : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_read : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   #endif
   block_index = get_bitmap_map_block_index(index);
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map->base + block_index > map->end || index >= map->length) {
      printf("bitmap_read : index exceeds range\n");
      return WRONG_INPUT;
   }
   #endif
   bit_indx = get_bitmap_map_block_bit_index(index);
   
   mask = 0x1 << ((MAP_BLOCK_BIT - 1) - bit_indx);
   
   buf = *(map->base + block_index) & mask;
   
   buf = buf >> ((MAP_BLOCK_BIT - 1) - bit_indx);
   
   *result = buf;
   
   if (!no_auto_crypt) {
      bitmap_meta_encrypt(map);
   }
   
   return 0;
}

int bitmap_write (simple_bitmap* map, bit_index index, map_block input_value, unsigned char no_auto_crypt) {
   map_block* cur;
   
   uint_fast32_t block_index;
   unsigned char bit_indx;
   
   map_block mask;
   
   map_block buf;
   
   map_block original;
   
   if (!no_auto_crypt) {
      bitmap_meta_decrypt(map);
   }
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_write : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_write : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_write : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_write : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_write : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_write : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   #endif
   block_index = get_bitmap_map_block_index(index);
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map->base + block_index > map->end || index >= map->length) {
      printf("bitmap_write : index exceeds range\n");
      return WRONG_INPUT;
   }
   #endif
   bit_indx = get_bitmap_map_block_bit_index(index);
   
   mask = 0x1 << ((MAP_BLOCK_BIT - 1) - bit_indx);
   
   buf = (input_value & 0x1) << ((MAP_BLOCK_BIT - 1) - bit_indx);
   buf &= mask;
   
   original = *(map->base + block_index) & mask;
   
   *(map->base + block_index) &= ~mask;
   *(map->base + block_index) |= buf;
   
   if (buf == 0 && original != 0) {
      map->number_of_zeros    ++;
      map->number_of_ones     --;
   }
   else if (buf != 0 && original == 0) {
      map->number_of_zeros    --;
      map->number_of_ones     ++;
   }
   
   if (!no_auto_crypt) {
      bitmap_meta_encrypt(map);
   }

   return 0;
}

int bitmap_count_zeros_and_ones (simple_bitmap* map) {
   map_block mask;
   
   map_block buf;
   
   map_block* cur;
   
   unsigned char count;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_count_zeros_and_ones : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_count_zeros_and_ones : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_count_zeros_and_ones : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_count_zeros_and_ones : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_count_zeros_and_ones : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   #endif
   
   // setup mask so left most bit is 1
   mask = 0x1 << (MAP_BLOCK_BIT - 1);
   
   // reset
   map->number_of_zeros = 0;
   map->number_of_ones = 0;
   
   // count all bits before last map_block
   for (cur = map->base; cur < map->end; cur++) {
      buf = *cur;
      for (count = 0; count < MAP_BLOCK_BIT; count++) {
         if ((buf & mask) == 0) {
            map->number_of_zeros++;
         }
         else {
            map->number_of_ones++;
         }
         buf = buf << 1;
      }
   }
   
   // setting once more to be sure
   cur = map->end;
   buf = *cur;
   
   // setup mask so left most bit is 1
   mask = 0x1 << (MAP_BLOCK_BIT - 1);
   
   // count bits in last map_block
   for (count = 0; count <= get_bitmap_map_block_bit_index(map->length-1); count++) {
      if ((buf & mask) == 0) {
         map->number_of_zeros++;
      }
      else {
         map->number_of_ones++;
      }
      buf = buf << 1;
   }
   
   // clean up the edge
   mask = 0;
   for (count = 0; count <= get_bitmap_map_block_bit_index(map->length-1); count++) {
      mask |= 0x1 << (MAP_BLOCK_BIT - count - 1);
   }
   *cur &= mask;
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_first_one_bit_index (simple_bitmap* map, bit_index* result, bit_index skip_to_bit) {
   map_block buf;
   
   map_block mask;
   
   map_block* cur;
   
   uint_fast8_t count = 0;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_first_one_bit_index : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_first_one_bit_index : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_first_one_bit_index : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_first_one_bit_index : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_first_one_bit_index : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_first_one_bit_index : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (result == NULL) {
      printf("bitmap_first_one_bit_index : result is null\n");
      return WRONG_INPUT;
   }
   if (skip_to_bit >= map->length) {
      printf("bitmap_first_one_bit_index : skip_to_bit is out of range\n");
      return WRONG_INPUT;
   }
   #endif
   
   // skip
   cur = map->base + get_bitmap_map_block_index(skip_to_bit);
   
   buf = *cur;
   
   // mask skipped bits
   mask = 0;
   for (count = 0; count < MAP_BLOCK_BIT - get_bitmap_map_block_bit_index(skip_to_bit); count++) {
      mask |= 0x1 << count;
   }
   buf = buf & mask;
   
   // setup mask so left most bit is 1
   mask = 0x1 << (MAP_BLOCK_BIT - 1);
   
   // find first non zero map_block
   for (; cur <= map->end; cur++) {
      if (count == 0) {
         buf = *cur;
      }
      if (buf != 0) {
         break;
      }
      count = 0;
   }
   
   // find first one bit
   for (count = 0; count < MAP_BLOCK_BIT; count++) {
      if (buf & mask) {
         break;
      }
      buf = buf << 1;
   }
   
   *result = (cur - map->base) * MAP_BLOCK_BIT + count;
   
   if (*result >= map->length) {
      return SEARCH_FAIL;
   }
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_first_zero_bit_index (simple_bitmap* map, bit_index* result, bit_index skip_to_bit) {
   map_block buf;
   
   map_block mask;
   
   map_block* cur;
   
   uint_fast8_t count = 0;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_first_zero_bit_index : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_first_zero_bit_index : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_first_zero_bit_index : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_first_zero_bit_index : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_first_zero_bit_index : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_first_zero_bit_index : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (skip_to_bit >= map->length) {
      printf("bitmap_first_zero_bit_index : skip_to_bit is out of range\n");
      return WRONG_INPUT;
   }
   #endif
   
   // skip
   cur = map->base + get_bitmap_map_block_index(skip_to_bit);
   
   buf = *cur;
   
   // mask skipped bits
   mask = 0;
   for (count = 0; count < get_bitmap_map_block_bit_index(skip_to_bit); count++) {
      mask |= 0x1 << (MAP_BLOCK_BIT - count - 1);
   }
   buf = buf | mask;
   
   // setup mask so left most bit is 1
   mask = 0x1 << (MAP_BLOCK_BIT - 1);
   
   // find first non all one map_block
   for (; cur <= map->end; cur++) {
      if (count == 0) {
         buf = *cur;
      }
      if (buf != (map_block) -1) {
         break;
      }
      count = 0;
   }
   
   // find first zero bit
   for (count = 0; count < MAP_BLOCK_BIT; count++) {
      if ((~buf) & mask) {
         break;
      }
      buf = buf << 1;
   }
   
   *result = (cur - map->base) * MAP_BLOCK_BIT + count;
   
   if (*result >= map->length) {
      return SEARCH_FAIL;
   }
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_first_one_cont_group (simple_bitmap* map, bitmap_cont_group* ret_grp, bit_index skip_to_bit) {
   map_block buf;
  
   map_block mask;
   
   map_block* cur;
   
   uint_fast8_t count = 0;
  
   bit_index zero_count;
   bit_index one_count;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_first_one_cont_group : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_first_one_cont_group : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_first_one_cont_group : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_first_one_cont_group : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_first_one_cont_group : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_first_one_cont_group : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (ret_grp == NULL) {
      printf("bitmap_first_one_cont_group : ret_grp is NULL\n");
      return WRONG_INPUT;
   }
   if (skip_to_bit >= map->length) {
      printf("bitmap_first_one_cont_group : skip_to_bit is out of range\n");
      return WRONG_INPUT;
   }
   #endif
   
   // setup return group
   ret_grp->bit_type = 0x1;
   ret_grp->start = 0;
   ret_grp->length = 0;
   
   // skip
   cur = map->base + get_bitmap_map_block_index(skip_to_bit);
   
   buf = *cur;
   
   // mask skipped bits
   mask = 0;
   for (count = 0; count < MAP_BLOCK_BIT - get_bitmap_map_block_bit_index(skip_to_bit); count++) {
      mask |= 0x1 << count;
   }
   buf = buf & mask;
   
   // setup mask so left most bit is 1
   mask = 0x1 << (MAP_BLOCK_BIT - 1);
   
   // find first non zero map block
   for (; cur <= map->end; cur++) {
      if (count == 0) {
         buf = *cur;
      }
      if (buf != 0) {
         break;
      }
      count = 0;
   }
   
   // find first one bit
   for (zero_count = 0; zero_count < MAP_BLOCK_BIT; zero_count++) {
      if (buf & mask) {
         break;
      }
      buf = buf << 1;
   }
   
   // set start
   ret_grp->start = (cur - map->base) * MAP_BLOCK_BIT + zero_count;
   
   if (ret_grp->start >= map->length) {
      return SEARCH_FAIL;
   }
   
   // cur initialised already
   for (; cur <= map->end; cur++) {
      if (zero_count == 0) {
         buf = *cur;
      }
      
      // count ones
      for (one_count = 0; one_count < MAP_BLOCK_BIT - zero_count; one_count++) {
         if ((~buf) & mask) {
            break;
         }
         buf = buf << 1;
      }
      
      // update count
      ret_grp->length += one_count;
      
      if ((one_count + zero_count) != MAP_BLOCK_BIT) {
         // the one bits end in this map_block, no need to continue
         break;
      }
      
      zero_count = 0;
   }
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_first_zero_cont_group (simple_bitmap* map, bitmap_cont_group* ret_grp, bit_index skip_to_bit) {
   map_block buf;
  
   map_block mask;
  
   map_block* cur;
  
   uint_fast8_t count = 0;
  
   bit_index zero_count;
   bit_index one_count;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_first_zero_cont_group : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_first_zero_cont_group : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_first_zero_cont_group : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_first_zero_cont_group : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_first_zero_cont_group : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_first_zero_cont_group : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (ret_grp == NULL) {
      printf("bitmap_first_zero_cont_group : ret_grp is NULL\n");
      return WRONG_INPUT;
   }
   if (skip_to_bit >= map->length) {
      printf("bitmap_first_zero_cont_group : skip_to_bit is out of range\n");
      return WRONG_INPUT;
   }
   #endif
   
   // setup return group
   ret_grp->bit_type = 0x0;
   ret_grp->start = 0;
   ret_grp->length = 0;
   
   // skip
   cur = map->base + get_bitmap_map_block_index(skip_to_bit);
   
   buf = *cur;
   
   // mask skipped bits
   mask = 0;
   for (count = 0; count < get_bitmap_map_block_bit_index(skip_to_bit); count++) {
      mask |= 0x1 << (MAP_BLOCK_BIT - count - 1);
   }
   buf = buf | mask;
   
   // setup mask so left most bit is 1
   mask = 0x1 << (MAP_BLOCK_BIT - 1);
   
   // find first non all one map block
   for (; cur <= map->end; cur++) {
      if (count == 0) {
         buf = *cur;
      }
      if (buf != (map_block) -1) {
         break;
      }
      count = 0;
   }
   
   // find first zero bit
   for (one_count = 0; one_count < MAP_BLOCK_BIT; one_count++) {
      if ((~buf) & mask) {
         break;
      }
      buf = buf << 1;
   }

   // set start
   ret_grp->start = (cur - map->base) * MAP_BLOCK_BIT + one_count;
   
   if (ret_grp->start >= map->length) {
      return SEARCH_FAIL;
   }
   
   // cur initialised already
   for (; cur <= map->end; cur++) {
      if (one_count == 0) {
         buf = *cur;
      }
      
      // count zeros
      for (zero_count = 0; zero_count < MAP_BLOCK_BIT - one_count; zero_count++) {
         if (buf & mask) {
            break;
         }
         buf = buf << 1;
      }
      
      // update count
      ret_grp->length += zero_count;
      
      if ((zero_count + one_count) != MAP_BLOCK_BIT) {
         // the zero bits end in this map_block, no need to continue
         break;
      }
      
      one_count = 0;
   }
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_first_one_bit_index_back (simple_bitmap* map, bit_index* result, bit_index skip_to_bit) {
   map_block buf;
   
   map_block mask;
   
   map_block* cur;
   
   uint_fast8_t count = 0;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_first_one_bit_index_back : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_first_one_bit_index_back : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_first_one_bit_index_back : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_first_one_bit_index_back : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_first_one_bit_index_back : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_first_one_bit_index_back : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (result == NULL) {
      printf("bitmap_first_one_bit_index_back : result is null\n");
      return WRONG_INPUT;
   }
   if (skip_to_bit >= map->length) {
      printf("bitmap_first_one_bit_index_back : skip_to_bit is out of range\n");
      return WRONG_INPUT;
   }
   #endif
   
   // skip
   cur = map->base + get_bitmap_map_block_index(skip_to_bit);
   
   buf = *cur;
   
   // mask skipped bits
   mask = 0;
   for (count = 0; count <= get_bitmap_map_block_bit_index(skip_to_bit); count++) {
      mask |= (0x1 << (MAP_BLOCK_BIT - 1)) >> count;
   }
   buf = buf & mask;
   
   // setup mask so right most bit is 1
   mask = 0x1;
   
   // find first non zero map_block
   for (; cur >= map->base; cur--) {
      if (count == MAP_BLOCK_BIT) {
         buf = *cur;
      }
      if (buf != 0) {
         break;
      }
      count = MAP_BLOCK_BIT;
   }
   
   // find first one bit
   for (count = 0; count < MAP_BLOCK_BIT; count++) {
      if (buf & mask) {
         break;
      }
      buf = buf >> 1;
   }
   
   if (count == MAP_BLOCK_BIT) { // failed to find the one bit
      *result = map->length;
      return SEARCH_FAIL;
   }
   else {
      *result = (cur - map->base) * MAP_BLOCK_BIT + (MAP_BLOCK_BIT-1 - count);
      return 0;
   }
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_first_zero_bit_index_back (simple_bitmap* map, bit_index* result, bit_index skip_to_bit) {
   map_block buf;
   
   map_block mask;
   
   map_block* cur;
   
   uint_fast8_t count = 0;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_first_zero_bit_index_back : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_first_zero_bit_index_back : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_first_zero_bit_index_back : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_first_zero_bit_index_back : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_first_zero_bit_index_back : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_first_zero_bit_index_back : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (skip_to_bit >= map->length) {
      printf("bitmap_first_zero_bit_index_back : skip_to_bit is out of range\n");
      return WRONG_INPUT;
   }
   #endif
   
   // skip
   cur = map->base + get_bitmap_map_block_index(skip_to_bit);
   
   buf = *cur;
   
   // mask skipped bits
   mask = 0;
   for (count = 0; count < MAP_BLOCK_BIT-1 - get_bitmap_map_block_bit_index(skip_to_bit); count++) {
      mask |= 0x1 << count;
   }
   buf = buf | mask;
   
   // setup mask so right most bit is 1
   mask = 0x1;
   
   // find first non all one map_block
   for (; cur >= map->base; cur--) {
      if (count == MAP_BLOCK_BIT) {
         buf = *cur;
      }
      if (buf != (map_block) -1) {
         break;
      }
      count = MAP_BLOCK_BIT;
   }
   
   // find first zero bit
   for (count = 0; count < MAP_BLOCK_BIT; count++) {
      if ((~buf) & mask) {
         break;
      }
      buf = buf >> 1;
   }
   
   if (count == MAP_BLOCK_BIT) {
      *result = map->length;
      return SEARCH_FAIL;
   }
   else {
      *result = (cur - map->base) * MAP_BLOCK_BIT + (MAP_BLOCK_BIT-1 - count);
      return 0;
   }
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_first_one_cont_group_back (simple_bitmap* map, bitmap_cont_group* ret_grp, bit_index skip_to_bit) {
   map_block buf;
  
   map_block mask;
   
   map_block* cur;
   
   uint_fast8_t count = 0;
  
   bit_index zero_count;
   bit_index one_count;
   
   bit_index end;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_first_one_cont_group_back : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_first_one_cont_group_back : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_first_one_cont_group_back : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_first_one_cont_group_back : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_first_one_cont_group_back : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_first_one_cont_group_back : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (ret_grp == NULL) {
      printf("bitmap_first_one_cont_group_back : ret_grp is NULL\n");
      return WRONG_INPUT;
   }
   if (skip_to_bit >= map->length) {
      printf("bitmap_first_one_cont_group_back : skip_to_bit is out of range\n");
      return WRONG_INPUT;
   }
   #endif
   
   // setup return group
   ret_grp->bit_type = 0x1;
   ret_grp->start = 0;
   ret_grp->length = 0;
   
   // skip
   cur = map->base + get_bitmap_map_block_index(skip_to_bit);
   
   buf = *cur;
   
   // mask skipped bits
   mask = 0;
   for (count = 0; count <= get_bitmap_map_block_bit_index(skip_to_bit); count++) {
      mask |= (0x1 << (MAP_BLOCK_BIT - 1)) >> count;
   }
   buf = buf & mask;
   
   // setup mask so right most bit is 1
   mask = 0x1;
   
   // find first non zero map block
   for (; cur >= map->base; cur--) {
      if (count == MAP_BLOCK_BIT) {
         buf = *cur;
      }
      if (buf != 0) {
         break;
      }
      count = MAP_BLOCK_BIT;
   }
   
   // find first one bit
   for (zero_count = 0; zero_count < MAP_BLOCK_BIT; zero_count++) {
      if (buf & mask) {
         break;
      }
      buf = buf >> 1;
   }
   
   if (zero_count == MAP_BLOCK_BIT) {
      return SEARCH_FAIL;
   }
   else {
      end = (cur - map->base) * MAP_BLOCK_BIT + (MAP_BLOCK_BIT-1 - zero_count);
   }
   
   // cur initialised already
   for (; cur >= map->base; cur--) {
      if (zero_count == 0) {
         buf = *cur;
      }
      
      // count ones
      for (one_count = 0; one_count < MAP_BLOCK_BIT - zero_count; one_count++) {
         if ((~buf) & mask) {
            break;
         }
         buf = buf >> 1;
      }
      
      // update count
      ret_grp->length += one_count;
      
      if ((one_count + zero_count) != MAP_BLOCK_BIT) {
         // the one bits end in this map_block, no need to continue
         break;
      }
      
      zero_count = 0;
   }
   
   // set start
   ret_grp->start = end - ret_grp->length + 1;
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_first_zero_cont_group_back (simple_bitmap* map, bitmap_cont_group* ret_grp, bit_index skip_to_bit) {
   map_block buf;
  
   map_block mask;
  
   map_block* cur;
  
   uint_fast8_t count = 0;
  
   bit_index zero_count;
   bit_index one_count;
   
   bit_index end;
   
   bitmap_meta_decrypt(map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_first_zero_cont_group_back : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_first_zero_cont_group_back : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_first_zero_cont_group_back : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_first_zero_cont_group_back : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_first_zero_cont_group_back : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_first_zero_cont_group_back : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (ret_grp == NULL) {
      printf("bitmap_first_zero_cont_group_back : ret_grp is NULL\n");
      return WRONG_INPUT;
   }
   if (skip_to_bit >= map->length) {
      printf("bitmap_first_zero_cont_group_back : skip_to_bit is out of range\n");
      return WRONG_INPUT;
   }
   #endif
   
   // setup return group
   ret_grp->bit_type = 0x0;
   ret_grp->start = 0;
   ret_grp->length = 0;
   
   // skip
   cur = map->base + get_bitmap_map_block_index(skip_to_bit);
   
   buf = *cur;
   
   // mask skipped bits
   mask = 0;
   for (count = 0; count < MAP_BLOCK_BIT-1 - get_bitmap_map_block_bit_index(skip_to_bit); count++) {
      mask |= 0x1 << count;
   }
   buf = buf | mask;
   
   // setup mask so right most bit is 1
   mask = 0x1;
   
   // find first non all one map block
   for (; cur >= map->base; cur--) {
      if (count == MAP_BLOCK_BIT) {
         buf = *cur;
      }
      if (buf != (map_block) -1) {
         break;
      }
      count = MAP_BLOCK_BIT;
   }
   
   // find first zero bit
   for (one_count = 0; one_count < MAP_BLOCK_BIT; one_count++) {
      if ((~buf) & mask) {
         break;
      }
      buf = buf >> 1;
   }
   
   if (one_count == MAP_BLOCK_BIT) {
      return SEARCH_FAIL;
   }
   else {
      end = (cur - map->base) * MAP_BLOCK_BIT + (MAP_BLOCK_BIT-1 - one_count);
   }
   
   // cur initialised already
   for (; cur >= map->base; cur--) {
      if (one_count == 0) {
         buf = *cur;
      }
      
      // count zeros
      for (zero_count = 0; zero_count < MAP_BLOCK_BIT - one_count; zero_count++) {
         if (buf & mask) {
            break;
         }
         buf = buf >> 1;
      }
      
      // update count
      ret_grp->length += zero_count;
      
      if ((zero_count + one_count) != MAP_BLOCK_BIT) {
         // the zero bits end in this map_block, no need to continue
         break;
      }
      
      one_count = 0;
   }
   
   // set start
   ret_grp->start = end - ret_grp->length + 1;
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

// both maps must be initialised
int bitmap_copy (simple_bitmap* src_map, simple_bitmap* dst_map, unsigned char allow_truncate, map_block default_value) {
   map_block* src_cur;
   map_block* dst_cur;
   
   map_block mask;
   
   unsigned char count;
   
   bitmap_meta_decrypt(src_map);
   bitmap_meta_decrypt(dst_map);
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (src_map == NULL) {
      printf("bitmap_copy : src_map is NULL\n");
      return WRONG_INPUT;
   }
   if (src_map->base == NULL) {
      printf("bitmap_copy : src_map->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (src_map->end == NULL) {
      printf("bitmap_copy : src_map->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (src_map->length == 0) {
      printf("bitmap_copy : src_map has no length\n");
      return CORRUPTED_DATA;
   }
   if (src_map->base + get_bitmap_map_block_index(src_map->length-1) != src_map->end) {
      printf("bitmap_copy : src_map : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (src_map->number_of_zeros + src_map->number_of_ones != src_map->length) {
      printf("bitmap_copy : src_map : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   if (dst_map == NULL) {
      printf("bitmap_copy : dst_map is NULL\n");
      return WRONG_INPUT;
   }
   if (dst_map->base == NULL) {
      printf("bitmap_copy : dst_map->base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (dst_map->end == NULL) {
      printf("bitmap_copy : dst_map->end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (dst_map->length == 0) {
      printf("bitmap_copy : dst_map has no length\n");
      return CORRUPTED_DATA;
   }
   if (dst_map->base + get_bitmap_map_block_index(dst_map->length-1) != dst_map->end) {
      printf("bitmap_copy : dst_map : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (dst_map->number_of_zeros + dst_map->number_of_ones != dst_map->length) {
      printf("bitmap_copy : dst_map : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   #endif
   
   if (src_map->length <= dst_map->length) { // no need for truncation at all
   
      // clean up a bit
      if (default_value & 0x1) {
         bitmap_one(dst_map);
      }
      else {
         bitmap_zero(dst_map);
      }
      
      for (src_cur = src_map->base, dst_cur = dst_map->base;
            src_cur <= src_map->end;
            src_cur ++, dst_cur++)
      {
         *dst_cur = *src_cur;
      }
      
      // clean off the edge
      dst_cur = dst_map->base + (src_map->end - src_map->base);
      mask = 0;
      for (count = 0; count <= get_bitmap_map_block_bit_index(src_map->length-1); count++) {
         mask |= 0x1 << (MAP_BLOCK_BIT - count - 1);
      }
      if (default_value & 0x1) {
         *dst_cur |= ~mask;
      }
      else {
         *dst_cur &= mask;
      }
   }
   else {   // need to truncate
      if (allow_truncate == 0) {
         printf("bitmap_copy : truncation needed but not allowed, both maps are untouched\n");
         return GENERAL_FAIL;
      }
   
      // clean up a bit
      if (default_value & 0x1) {
         bitmap_one(dst_map);
      }
      else {
         bitmap_zero(dst_map);
      }
      
      for (src_cur = src_map->base, dst_cur = dst_map->base;
            dst_cur <= dst_map->end;
            src_cur ++, dst_cur++)
      {
         *dst_cur = *src_cur;
      }
      
      // clean off the edge
      dst_cur = dst_map->end;
      mask = 0;
      for (count = 0; count <= get_bitmap_map_block_bit_index(dst_map->length-1); count++) {
         mask |= 0x1 << (MAP_BLOCK_BIT - count - 1);
      }
      if (default_value & 0x1) {
         *dst_cur |= ~mask;
      }
      else {
         *dst_cur &= mask;
      }
   }
   
   bitmap_count_zeros_and_ones(dst_map);
   
   bitmap_meta_encrypt(src_map);
   bitmap_meta_encrypt(dst_map);
   
   return 0;
}

int bitmap_meta_copy (simple_bitmap* src_map, simple_bitmap* dst_map) {
   dst_map->base     =  src_map->base;
   dst_map->end      =  src_map->end;
   dst_map->length   =  src_map->length;
   
   dst_map->number_of_zeros   =  src_map->number_of_zeros;
   dst_map->number_of_ones    =  src_map->number_of_ones;
   return 0;
}

// memory management is not handled
// this function only handles meta data and initialise uninitialised map blocks
int bitmap_grow (simple_bitmap* map, map_block* end, uint_fast32_t size_in_bits, map_block default_value) {
   map_block* cur;
   
   map_block mask;
   
   map_block* old_end;
   
   bit_index old_length;
   
   unsigned char count;
   
   bitmap_meta_decrypt(map);
   
   //input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_grow : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_grow : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_grow : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_grow : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_grow : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_grow : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   #endif
   
   if (end == NULL) {
      if (size_in_bits == 0) {
         printf("bitmap_grow : end is NULL but size is 0 as well\n");
         return WRONG_INPUT;
      }
      if (size_in_bits < map->length) {
         printf("bitmap_grow : request length is smaller than old length\n");
         return WRONG_INPUT;
      }
      if (size_in_bits == map->length) {
         printf("bitmap_grow : request length is same as old length\n");
         return WRONG_INPUT;
      }
      old_end = map->end;
      old_length = map->length;
      map->end = map->base + get_bitmap_map_block_index(size_in_bits-1);
      map->length = size_in_bits;
   }
   else {
      if (end < map->end) {
         printf("bitmap_grow : request end is lower than old end\n");
         return WRONG_INPUT;
      }
      if (end == map->end) {
         printf("bitmap_grow : request end is same as old end\n");
         return WRONG_INPUT;
      }
      old_end = map->end;
      old_length = map->length;
      map->end = end;
      map->length = (end - map->base + 1) * MAP_BLOCK_BIT;
   }
   
   // clean off the edge and remaining map blocks
   mask = 0;
   for (count = 0; count <= get_bitmap_map_block_bit_index(old_length-1); count++) {
      mask |= 0x1 << (MAP_BLOCK_BIT - count - 1);
   }
   if (default_value > 1) {
      ;     // do nothing
   }
   else if (default_value & 0x1) {
      *old_end |= ~mask;
      for (cur = old_end + 1; cur <= map->end; cur++) {
         *cur = (map_block) -1;
      }
   }
   else {
      *old_end &= mask;
      for (cur = old_end + 1; cur <= map->end; cur++) {
         *cur = 0;
      }
   }
   
   bitmap_count_zeros_and_ones(map);
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

// cuts the bitmap and modifies the meta data
int bitmap_shrink (simple_bitmap* map, map_block* end, uint_fast32_t size_in_bits) {
   map_block* cur;
   
   map_block mask;
   
   map_block* old_end;
   
   bit_index old_length;
   
   unsigned char count;
   
   bitmap_meta_decrypt(map);
   
   //input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_shrink : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_shrink : base is NULL\n");
      return WRONG_INPUT;
   }
   if (map->end == NULL) {
      printf("bitmap_shrink : end is NULL\n");
      return WRONG_INPUT;
   }
   if (map->length == 0) {
      printf("bitmap_shrink : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_shrink : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_shrink : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   #endif
   
   if (end == NULL) {
      #ifndef SIMPLE_BITMAP_SKIP_CHECK
      if (size_in_bits == 0) {
         printf("bitmap_shrink : end is NULL but size is 0 as well\n");
         return WRONG_INPUT;
      }
      if (size_in_bits > map->length) {
         printf("bitmap_shrink : request length is larger than old length\n");
         return WRONG_INPUT;
      }
      if (size_in_bits == map->length) {
         printf("bitmap_shrink : request length is same as old length\n");
         return WRONG_INPUT;
      }
      #endif
      old_end = map->end;
      //old_length = map->length;
      map->end = map->base + get_bitmap_map_block_index(size_in_bits-1);
      map->length = size_in_bits;
   }
   else {
      #ifndef SIMPLE_BITMAP_SKIP_CHECK
      if (end > map->end) {
         printf("bitmap_shrink : request end is higher than old end\n");
         return WRONG_INPUT;
      }
      if (end == map->end) {
         printf("bitmap_shrink : request end is same as old end\n");
         return WRONG_INPUT;
      }
      #endif
      old_end = map->end;
      //old_length = map->length;
      map->end = end;
      map->length = (end - map->base + 1) * MAP_BLOCK_BIT;
   }
   
   // wipe off the edge and old map blocks
   mask = 0;
   for (count = 0; count <= get_bitmap_map_block_bit_index(map->length-1); count++) {
      mask |= 0x1 << (MAP_BLOCK_BIT - count - 1);
   }
   *(map->end) &= mask;
   for (cur = map->end + 1; cur <= old_end; cur++) {
      *cur = 0;
   }
   
   bitmap_count_zeros_and_ones(map);
   
   bitmap_meta_encrypt(map);
   
   return 0;
}

int bitmap_show (simple_bitmap* map) {
   #ifdef SIMPLE_BITMAP_META_DATA_SECURITY
   uint_least8_t offsets;
   uint_least8_t rand_indicators;
   
   unsigned char count;
   #endif
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_show : map is NULL\n");
      return WRONG_INPUT;
   }
   #endif
   printf("####################\n");
   
   printf("map->base : %p\n", map->base);
   printf("map->end  : %p\n", map->end);
   printf("map->length : %d\n", (int) map->length);
   printf("map->number_of_zeros : %d\n", (int) map->number_of_zeros);
   printf("map->number_of_ones  : %d\n", (int) map->number_of_ones);
   
   #ifdef SIMPLE_BITMAP_META_DATA_SECURITY
   offsets = map->offsets;
   printf("obj_rand_encrypt_xor_meta  : %x\n", map->obj_rand_encrypt_xor_meta);
   printf("obj_rand_encrypt_add_meta  : %x\n", map->obj_rand_encrypt_add_meta);
   printf("obj_rand_encrypt_xor_meta2 : %x\n", map->obj_rand_encrypt_xor_meta2);
   printf("obj_rand_encrypt_add_meta2 : %x\n", map->obj_rand_encrypt_add_meta2);
   printf("* - actually used\n");
   printf("offsets : %x\n", offsets);
   if (!((offsets >> OFF_BA)&0x1)) {
      printf("map->base[0]* : %p\n", map->base_a[0]);
      printf("map->base[1]  : %p\n", map->base_a[1]);
   }
   else {
      printf("map->base[0]  : %p\n", map->base_a[0]);
      printf("map->base[1]* : %p\n", map->base_a[1]);
   }
   if (!((offsets >> OFF_EN)&0x1)) {
      printf("map->end[0]*  : %p\n", map->end_a[0]);
      printf("map->end[1]   : %p\n", map->end_a[1]);
   }
   else {
      printf("map->end[0]   : %p\n", map->end_a[0]);
      printf("map->end[1]*  : %p\n", map->end_a[1]);
   }
   if (!((offsets >> OFF_LE)&0x1)) {
      printf("map->length[0]* : %u\n", map->length_a[0]);
      printf("map->length[1]  : %u\n", map->length_a[1]);
   }
   else {
      printf("map->length[0]  : %u\n", map->length_a[0]);
      printf("map->length[1]* : %u\n", map->length_a[1]);
   }
   if (!((offsets >> OFF_NZ)&0x1)) {
      printf("map->number_of_zeros[0]* : %u\n", map->number_of_zeros_a[0]);
      printf("map->number_of_zeros[1]  : %u\n", map->number_of_zeros_a[1]);
   }
   else {
      printf("map->number_of_zeros[0]  : %u\n", map->number_of_zeros_a[0]);
      printf("map->number_of_zeros[1]* : %u\n", map->number_of_zeros_a[1]);
   }
   if (!((offsets >> OFF_NO)&0x1)) {
      printf("map->number_of_ones[0]*  : %u\n", map->number_of_ones_a[0]);
      printf("map->number_of_ones[1]   : %u\n", map->number_of_ones_a[1]);
   }
   else {
      printf("map->number_of_ones[0]   : %u\n", map->number_of_ones_a[0]);
      printf("map->number_of_ones[1]*  : %u\n", map->number_of_ones_a[1]);
   }
   
   rand_indicators = map->rand_indicators;
   
   printf("map->rand_indicators : %x\n", rand_indicators);
   printf("map->rand_degrees :\n");
   for (count = 0; count < 5; count++) {
      printf("   [%d][%d] : %d, ", count, 0, map->rand_degrees[count][0]);
      printf("   [%d][%d] : %d\n", count, 1, map->rand_degrees[count][1]);
   }
   
   /*printf("map->rand_revert_to : ");
   for (count = 0; count < sizeof(map->rand_revert_to)-1; count++) {
      printf("%x, ", map->rand_revert_to[count]);
   }
   printf("%x\n", map->rand_revert_to[count]);*/
   #endif
   
   printf("####################\n");
   
   return 0;
}

int bitmap_cont_group_show (bitmap_cont_group* grp) {
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (grp == NULL) {
      printf("bitmap_cont_group_show : grp is NULL\n");
      return WRONG_INPUT;
   }
   #endif
   printf("####################\n");
   
   printf("grp->bit_type : %x\n", grp->bit_type);
   printf("grp->start  : %d\n", (int) grp->start);
   printf("grp->length : %d\n", (int) grp->length);
   
   printf("####################\n");
   
   return 0;
}

int bitmap_raw_show (simple_bitmap* map) {
   uint_fast16_t count;
   
   map_block* cur;
   
   // input check
   #ifndef SIMPLE_BITMAP_SKIP_CHECK
   if (map == NULL) {
      printf("bitmap_raw_show : map is NULL\n");
      return WRONG_INPUT;
   }
   if (map->base == NULL) {
      printf("bitmap_raw_show : base is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->end == NULL) {
      printf("bitmap_raw_show : end is NULL\n");
      return CORRUPTED_DATA;
   }
   if (map->length == 0) {
      printf("bitmap_raw_show : map has no length\n");
      return CORRUPTED_DATA;
   }
   if (map->base + get_bitmap_map_block_index(map->length-1) != map->end) {
      printf("bitmap_raw_show : length is inconsistent with base and end\n");
      return CORRUPTED_DATA;
   }
   if (map->number_of_zeros + map->number_of_ones != map->length) {
      printf("bitmap_raw_show : inconsistent statistics of number of ones and zeros\n");
      return CORRUPTED_DATA;
   }
   #endif
   printf("####################\n");
   
   printf("bitmap_raw :\n");
   
   count = 0;
   for (cur = map->base; cur <= map->end; cur++) {
      printf("%02X ", *cur);
      if (count == 15) {
         count = 0;
         printf("\n");
      }
      else {
         count++;
      }
   }
   printf("\n");
   
   printf("####################\n");
   
   return 0;
}