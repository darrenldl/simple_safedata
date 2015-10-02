/* simple safe data structure library
 * Author : darrenldl <dldldev@yahoo.com>
 * 
 * Version : 0.01
 * 
 * Note:
 *    The data structures themselves are not threadsafe
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

#ifndef SIMPLE_SAFEDATA_H
#define SIMPLE_SAFEDATA_H

//#define SIMPLE_SAFEDATA_SILENT

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

#include <limits.h>
#include <float.h>

#include <string.h>

#include "simple_bitmap.h"

#define safd_max(a, b) ((a) > (b) ? (a) : (b))

#ifdef SIMPLE_SAFEDATA_SILENT
   #define sfd_printf(...) 0
#else
   #define sfd_printf printf
#endif

/* Flags for sfd_var:
 *    0x1 - initialised
 *    0x2 - read allowed
 *    0x4 - write allowed
 */

#define SFD_FL_INITD 0x1
#define SFD_FL_READ  0x2
#define SFD_FL_WRITE 0x4
#define SFD_FL_CON   0x8

// safe data structure variable declaration
#define sfd_var_dec(type, name) \
   struct {\
      uint_least8_t flags; \
      type val;            \
      type up_bnd;         \
      type lo_bnd;         \
      int (*constraint) (type);\
      char* con_expr;      \
   } name;\
   name.flags = SFD_FL_READ | SFD_FL_WRITE | SFD_FL_CON;\
   name.lo_bnd =\
      _Generic(name.val,\
         signed char    : SCHAR_MIN,\
         unsigned char  : 0,        \
         char           : CHAR_MIN, \
         short          : SHRT_MIN, \
         unsigned short : 0,        \
         int            : INT_MIN,  \
         unsigned int   : 0,        \
         long           : LONG_MIN, \
         unsigned long  : 0,        \
         long long      : LLONG_MIN,\
         unsigned long long : 0,    \
         float          : FLT_MIN,  \
         double         : DBL_MIN,  \
         long double    : LDBL_MIN  \
      )\
   ;\
   name.up_bnd =\
      _Generic(name.val,\
         signed char    : SCHAR_MAX,\
         unsigned char  : UCHAR_MAX,\
         char           : CHAR_MAX, \
         short          : SHRT_MAX, \
         unsigned short : USHRT_MAX,\
         int            : INT_MAX,  \
         unsigned int   : UINT_MAX, \
         long           : LONG_MIN, \
         unsigned long  : ULONG_MAX,\
         long long      : LLONG_MAX,\
         unsigned long long : ULLONG_MAX,\
         float          : FLT_MAX,  \
         double         : DBL_MAX,  \
         long double    : LDBL_MAX  \
      )\
   ;\
   name.constraint = &sfd_dummy;

#define sfd_var_read(name) \
   (name.flags & SFD_FL_INITD? \
      (name.flags & SFD_FL_READ? \
         (name.val)\
      :\
          0* sfd_printf("Read not permitted, file : %s, line : %d\n", __FILE__, __LINE__)\
         +0* sfd_force_exit()\
      )\
   :\
       0* sfd_printf("Uninitialised read : file : %s, line : %d\n", __FILE__, __LINE__)\
      +0* sfd_force_exit()\
   )

#define sfd_var_write(name, in_val) \
   (name.flags&SFD_FL_CON? sfd_enforce_con(name) : 0)\
   +0*   (name.flags & SFD_FL_WRITE? \
            (name.val = in_val)\
            +0* (name.flags |= SFD_FL_INITD)\
         :\
             0* sfd_printf("Write not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
            +0* sfd_force_exit()\
         )\
   +0* (name.flags&SFD_FL_CON? sfd_enforce_con(name) : 0)

#define sfd_flag_get(name) \
   (name.flags)

#define sfd_flag_set(name, in_val) \
   (name.flags = (in_val))

#define sfd_flag_enable(name, in_val) \
   (name.flags |= (in_val))

#define sfd_flag_disable(name, in_val) \
   (name.flags &= ~(in_val))

#define sfd_var_get_lo_bnd(name) \
   (name.lo_bnd)

#define sfd_var_set_lo_bnd(name, in_val) \
   (name.lo_bnd = in_val)

#define sfd_var_get_up_bnd(name) \
   (name.up_bnd)

#define sfd_var_set_up_bnd(name, in_val) \
   (name.up_bnd = in_val)

#define sfd_add_con(name, type, arg_name, expr) \
   int constraint##_sfd_var_##name (type arg_name) \
   {\
      return (expr);\
   }\
   name.constraint = &constraint##_sfd_var_##name;\
   name.con_expr = #expr;

#define sfd_enforce_con(name) \
   (name.constraint(name.val)? \
      0\
   :\
       0* sfd_printf("Constraint failed : file : %s, line : %d\n", __FILE__, __LINE__)\
      +0* sfd_printf("Constraint in effect : %s\n", name.con_expr)\
      +0* sfd_force_exit()\
   )

#define sfd_arr_dec_sta(type, name, in_size)\
   struct {\
      uint_least8_t flags; \
      type* start;         \
      uint_fast32_t size;  \
      simple_bitmap init_map;\
      map_block temp;\
   } name;\
   map_block name##_sfd_raw_init_map [get_bitmap_map_block_number(in_size)];\
   type name##_sfd_arr [in_size];\
   name.flags = SFD_FL_READ | SFD_FL_WRITE;\
   name.start = name##_sfd_arr;\
   name.size = in_size;\
   bitmap_init(&name.init_map, name##_sfd_raw_init_map, NULL, in_size, 0);

#define sfd_arr_dec_dyn(type, name, in_size)\
   struct {\
      uint_least8_t flags; \
      type* start;         \
      uint_fast32_t size;  \
      simple_bitmap init_map;\
      map_block temp;\
   } name;\
   map_block* name##_sfd_raw_init_map = (map_block*) malloc(sizeof(map_block) * get_bitmap_map_block_number(in_size));\
   name.start = (type*) malloc(sizeof(type) * in_size);\
   if (name##_sfd_raw_init_map == NULL || name.start == NULL) {\
      sfd_printf("sfd_arr_dec_dyn : malloc failed : file : %s, line : %d\n", __FILE__, __LINE__);\
      name.flags = 0;\
      name.size = 0;\
   }\
   else {\
      name.flags = SFD_FL_READ | SFD_FL_WRITE;\
      name.size = in_size;\
      bitmap_init(&name.init_map, name##_sfd_raw_init_map, NULL, in_size, 0);\
   }

#define sfd_arr_dec_man(type, name, in_size, bmp_start, arr_start)\
   struct {\
      uint_least8_t flags; \
      type* start;         \
      uint_fast32_t size;  \
      simple_bitmap init_map;\
      map_block temp;\
   } name;\
   name.flags = SFD_FL_READ | SFD_FL_WRITE;\
   name.start = arr_start;\
   name.size = in_size;\
   bitmap_init(&name.init_map, bmp_start, NULL, in_size, 0);

#define sfd_arr_read(name, indx) \
   (name.flags&SFD_FL_INITD? \
      (indx < name.size? \
         (name.start[indx])\
      :\
          0* sfd_printf("Index out of bound : file : %s, line : %d\n", __FILE__, __LINE__)\
         +0* sfd_force_exit()\
      )\
   :\
       0* bitmap_read(&name.init_map, indx, &name.temp, 0)\
      +  (name.temp? \
            (name.start[indx])\
         :\
             0* sfd_printf("Uninitialised read : file : %s, line : %d\n", __FILE__, __LINE__)\
            +0* sfd_force_exit()\
         )\
   )

#define sfd_arr_write(name, indx, in_val) \
   (name.flags&SFD_FL_WRITE? \
      (indx < name.size? \
             (name.start[indx] = in_val)\
         +0* bitmap_write(&name.init_map, indx, 1, 0)\
      :\
          0* sfd_printf("Index out of bound : file : %s, line : %d\n", __FILE__, __LINE__)\
         +0* sfd_force_exit()\
      )\
   :\
       0* sfd_printf("Write not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +0* sfd_force_exit()\
   )

#define sfd_arr_wipe(name)\
       (memset(name.start, name.size, sizeof(*name.start)))\
   +0* (name.flags |= SFD_FL_INITD)



static int sfd_force_exit() {
   exit(0);
   return 0;
}

static int sfd_dummy() {
   return 1;
}

#endif