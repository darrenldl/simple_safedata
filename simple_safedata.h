/* simple safe data structure library
 * Author : darrenldl <dldldev@yahoo.com>
 * 
 * Version : 0.02
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

//#define SIMPLE_SAFEDATA_DISABLE

// customise your application return code upon SFD error here
#define SFD_ERR_RET_CODE   1404

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

#include <limits.h>
#include <float.h>

#include <string.h>

#include <stdarg.h>

#include "simple_bitmap.h"

#define safd_max(a, b) ((a) > (b) ? (a) : (b))

#ifdef SIMPLE_SAFEDATA_DISABLE
   #define sfd_flag_get(...)        0
   #define sfd_flag_set(...)        0
   #define sfd_flag_enable(...)     0
   #define sfd_flag_disable(...)    0
   #define sfd_var_dec(type, name)     type name
   #define sfd_var_read(name)          name
   #define sfd_var_write(name, in_val) (name = in_val)
   #define sfd_var_incre(name, in_val) (name += in_val)
   #define sfd_var_get_lo_bnd(...)  0
   #define sfd_var_get_up_bnd(...)  0
   #define sfd_var_set_lo_bnd(...)  0
   #define sfd_var_set_up_bnd(...)  0
   #define sfd_var_def_con(...)
   #define sfd_var_add_con(...)     0
   #define sfd_var_enforce_con(...) 1
   #define sfd_arr_dec_sta(type, name, size)                      type name[size]
   #define sfd_arr_dec_dyn(type, name, size)                      type* name = (type*) malloc(sizeof(type) * size)
   #define sfd_arr_dec_man(type, name, size, start_bmap, start)   type* name = start
   #define sfd_arr_read(name, indx)                               name[indx]
   #define sfd_arr_write(name, indx, in_val)                      (name[indx] = in_val)
   #define sfd_arr_incre(name, indx, in_val)                      (name[indx] += in_val)
   #define sfd_arr_wipe(...)        0
   #define sfd_arr_def_con_ele(...)
   #define sfd_arr_def_con_arr(...)
   #define sfd_arr_enforce_con(...) 1
   #define sfd_arr_add_con_ele(...) 0
   #define sfd_arr_add_con_arr(...) 0
   #define sfd_arr_get_size(...)    0
#else

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

#define SFD_FL_INITD    0x1   // for all sfd data
#define SFD_FL_READ     0x2   // for all sfd data
#define SFD_FL_WRITE    0x4   // for all sfd data
#define SFD_FL_CON      0x8   // for all sfd data
#define SFD_FL_CON_ELE  0x10  // for sfd arr
#define SFD_FL_CON_ARR  0x20  // for sfd arr

// can NOT be used as expression
#define sfd_var_dec(type, name) \
   struct {\
      uint_least16_t flags; \
      type val;            \
      type up_bnd;         \
      type lo_bnd;         \
      int (*constraint) (type);\
      char* con_in_effect; \
      char* con_expr;      \
      type ret_temp;       \
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
         long double    : LDBL_MIN, \
         default :\
             sfd_printf("sfd : Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
            +sfd_force_exit()\
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
         long double    : LDBL_MAX, \
         default :\
             sfd_printf("sfd : Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
            +sfd_force_exit()\
      )\
   ;\
   name.constraint = 0;

static int sfd_force_exit() {
   exit(SFD_ERR_RET_CODE);
   return 0;
}

// CAN be used as expression
#define sfd_var_read(name) \
   (\
   name.ret_temp =\
   (name.flags & SFD_FL_READ? \
      (name.flags & SFD_FL_INITD? \
         (name.val)\
      :\
          sfd_printf("sfd : Uninitialised read : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   :\
       sfd_printf("sfd : Read not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

// CAN be used as expression
#define sfd_var_write(name, in_val) \
   (\
   name.ret_temp =\
    (in_val < name.lo_bnd? \
      sfd_printf("sfd : Lower bound breached : file : %s, line : %d\n", __FILE__, __LINE__) + sfd_force_exit(): 0)\
   +(in_val > name.up_bnd? \
      sfd_printf("sfd : Upper bound breached : file : %s, line : %d\n", __FILE__, __LINE__) + sfd_force_exit(): 0)\
   +  (name.flags & SFD_FL_WRITE? \
          (name.val = in_val)\
         +0* (name.flags |= SFD_FL_INITD)\
      :\
          sfd_printf("sfd : Write not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   +\
   (name.flags & SFD_FL_CON?\
      (name.constraint ?\
         sfd_var_enforce_con(name)\
      :\
         0\
      )\
   :\
      0\
   )\
   )

// CAN be used as expression
#define sfd_var_incre(name, in_val) \
   (\
   name.ret_temp =\
    (name.val + (in_val) < name.lo_bnd? \
      sfd_printf("sfd : Lower bound breached : file : %s, line : %d\n", __FILE__, __LINE__) + sfd_force_exit(): 0)\
   +(name.val + (in_val) > name.up_bnd? \
      sfd_printf("sfd : Upper bound breached : file : %s, line : %d\n", __FILE__, __LINE__) + sfd_force_exit(): 0)\
   +  (name.flags & SFD_FL_WRITE? \
          (name.val += in_val)\
         +0* (name.flags |= SFD_FL_INITD)\
      :\
          sfd_printf("sfd : Write not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   +\
   (name.flags & SFD_FL_CON?\
      (name.constraint ?\
         sfd_var_enforce_con(name)\
      :\
         0\
      )\
   :\
      0\
   )\
   )

// CAN be used as expression
#define sfd_flag_get(name) \
   (name.flags)

// CAN be used as expression
#define sfd_flag_set(name, in_val) \
   (name.flags = (in_val))

// CAN be used as expression
#define sfd_flag_enable(name, in_val) \
   (name.flags |= (in_val))

// CAN be used as expression
#define sfd_flag_disable(name, in_val) \
   (name.flags &= ~(in_val))

// CAN be used as expression
#define sfd_var_get_lo_bnd(name) \
   (name.lo_bnd)

// CAN be used as expression
#define sfd_var_set_lo_bnd(name, in_val) \
   (name.lo_bnd = in_val)

// CAN be used as expression
#define sfd_var_get_up_bnd(name) \
   (name.up_bnd)

// CAN be used as expression
#define sfd_var_set_up_bnd(name, in_val) \
   (name.up_bnd = in_val)

// can NOT be used as expression
#define sfd_var_def_con(con_name, type, arg_name, expr) \
   int sfd_con_##con_name##_var (type arg_name) {\
      return (expr);\
   }\
   char* sfd_con_##con_name##_var_expr = #expr;

// can NOT be used as expression
#define sfd_var_add_con(name, con_name) \
   name.constraint = &sfd_con_##con_name##_var;\
   name.con_in_effect = #con_name;\
   name.con_expr = sfd_con_##con_name##_var_expr;

// CAN be used as expression
#define sfd_var_enforce_con(name) \
   (name.constraint(name.val)? \
      0\
   :\
       sfd_printf("sfd : Constraint failed : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_printf("        Constraint in effect  : %s\n", name.con_in_effect)\
      +sfd_printf("        Constraint expression : %s\n", name.con_expr)\
      +sfd_force_exit()\
   )

// can NOT be used as expression
#define sfd_arr_dec_sta(type, name, in_size)\
   struct {\
      uint_least16_t flags; \
      type* start;         \
      uint_fast32_t size;  \
      simple_bitmap init_map;\
      map_block temp;      \
      type ret_temp;       \
      int (*constraint_ele) (type);    \
      char* con_in_effect_ele;         \
      char* con_expr_ele;              \
      int (*constraint_arr) (int, ...);\
      char* con_in_effect_arr;         \
      char* con_expr_arr;              \
   } name;\
   map_block name##_sfd_raw_init_map [get_bitmap_map_block_number(in_size)];\
   type name##_sfd_arr [in_size];\
   name.flags = SFD_FL_READ | SFD_FL_WRITE | SFD_FL_CON_ELE | SFD_FL_CON_ARR;\
   name.start = name##_sfd_arr;\
   name.size = in_size;\
   bitmap_init(&name.init_map, name##_sfd_raw_init_map, NULL, in_size, 0);\
   name.constraint_ele = 0;\
   name.constraint_arr = 0;

// can NOT be used as expression
#define sfd_arr_dec_dyn(type, name, in_size)\
   struct {\
      uint_least16_t flags; \
      type* start;         \
      uint_fast32_t size;  \
      simple_bitmap init_map;\
      map_block temp;      \
      type ret_temp;       \
      int (*constraint_ele) (type);    \
      char* con_in_effect_ele;         \
      char* con_expr_ele;              \
      int (*constraint_arr) (int, ...);\
      char* con_in_effect_arr;         \
      char* con_expr_arr;              \
   } name;\
   map_block* name##_sfd_raw_init_map = (map_block*) malloc(sizeof(map_block) * get_bitmap_map_block_number(in_size));\
   name.start = (type*) malloc(sizeof(type) * in_size);\
   if (name##_sfd_raw_init_map == NULL || name.start == NULL) {\
      sfd_printf("sfd : sfd_arr_dec_dyn : malloc failed : file : %s, line : %d\n", __FILE__, __LINE__);\
      name.flags = 0;\
      name.size = 0;\
      sfd_force_exit();\
   }\
   else {\
      name.flags = SFD_FL_READ | SFD_FL_WRITE | SFD_FL_CON_ELE | SFD_FL_CON_ARR;\
      name.size = in_size;\
      bitmap_init(&name.init_map, name##_sfd_raw_init_map, NULL, in_size, 0);\
   }\
   name.constraint_ele = 0;\
   name.constraint_arr = 0;

// can NOT be used as expression
#define sfd_arr_dec_man(type, name, in_size, bmp_start, arr_start)\
   struct {\
      uint_least16_t flags; \
      type* start;         \
      uint_fast32_t size;  \
      simple_bitmap init_map;\
      map_block temp;      \
      type ret_temp;       \
      int (*constraint_ele) (type);    \
      char* con_in_effect_ele;         \
      char* con_expr_ele;              \
      int (*constraint_arr) (int, ...);\
      char* con_in_effect_arr;         \
      char* con_expr_arr;              \
   } name;\
   name.flags = SFD_FL_READ | SFD_FL_WRITE | SFD_FL_CON_ELE | SFD_FL_CON_ARR;\
   name.start = arr_start;\
   name.size = in_size;\
   bitmap_init(&name.init_map, bmp_start, NULL, in_size, 0);\
   name.constraint_ele = 0;\
   name.constraint_arr = 0;

// CAN be used as expression
#define sfd_arr_read(name, indx) \
   (\
   name.ret_temp =\
   (name.flags & SFD_FL_READ? \
      (indx < name.size? \
         (name.flags & SFD_FL_INITD? \
            (name.start[indx])\
         :\
             0* bitmap_read(&name.init_map, indx, &name.temp, 0)\
            +  (name.temp? \
                  (name.start[indx])\
               :\
                   sfd_printf("sfd : Uninitialised read : file : %s, line : %d\n", __FILE__, __LINE__)\
                  +sfd_force_exit()\
               )\
         )\
      :\
          sfd_printf("sfd : Index out of bound : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   :\
       sfd_printf("sfd : Read not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

// CAN be used as expression
#define sfd_arr_write(name, indx, in_val) \
   (\
   name.ret_temp =\
   (name.flags & SFD_FL_WRITE? \
      (indx < name.size? \
          (name.start[indx] = in_val)\
         +0* bitmap_write(&name.init_map, indx, 1, 0)\
         +\
         (name.flags & SFD_FL_CON_ELE?\
            (name.constraint_ele ?\
               sfd_arr_enforce_con_ele(name, name.start[indx])\
            :\
               0\
            )\
         :\
            0\
         )\
         +\
         (name.flags & SFD_FL_CON_ARR?\
            (name.constraint_arr ?\
               sfd_arr_enforce_con_arr(name)\
            :\
               0\
            )\
         :\
            0\
         )\
      :\
          sfd_printf("sfd : Index out of bound : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   :\
       sfd_printf("sfd : Write not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

// CAN be used as expression
#define sfd_arr_incre(name, indx, in_val) \
   (\
   name.ret_temp =\
   (name.flags & SFD_FL_WRITE? \
      (indx < name.size? \
          (name.start[indx] += in_val)\
         +0* bitmap_write(&name.init_map, indx, 1, 0)\
         +\
         (name.flags & SFD_FL_CON_ELE?\
            (name.constraint_ele ?\
               sfd_arr_enforce_con_ele(name, name.start[indx])\
            :\
               0\
            )\
         :\
            0\
         )\
         +\
         (name.flags & SFD_FL_CON_ARR?\
            (name.constraint_arr ?\
               sfd_arr_enforce_con_arr(name)\
            :\
               0\
            )\
         :\
            0\
         )\
      :\
          sfd_printf("sfd : Index out of bound : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   :\
       sfd_printf("sfd : Write not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

// CAN be used as expression
#define sfd_arr_wipe(name) \
   (\
   name.ret_temp =\
   (name.flags & SFD_FL_WRITE? \
       (sfd_memset(name.start, 0, sizeof(name.start[0]) * name.size))\
      +0* (name.flags |= SFD_FL_INITD)\
   :\
       sfd_printf("sfd : Write not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

// CAN be used as expression
#define sfd_arr_enforce_con_ele(name, val) \
   (name.constraint_ele(val)? \
      0\
   :\
       sfd_printf("sfd : Constraint failed : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_printf("        Constraint in effect  : %s\n", name.con_in_effect_ele)\
      +sfd_printf("        Constraint expression : %s\n", name.con_expr_ele)\
      +sfd_force_exit()\
   )

// CAN be used as expression
#define sfd_arr_enforce_con_arr(name) \
   (name.constraint_arr(0, name.start, name.size)? \
      0\
   :\
       sfd_printf("sfd : Constraint failed : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_printf("        Constraint in effect  : %s\n", name.con_in_effect_arr)\
      +sfd_printf("        Constraint expression : %s\n", name.con_expr_arr)\
      +sfd_force_exit()\
   )

// can NOT be used as expression
#define sfd_arr_def_con_ele(con_name, type, arg_name, expr) \
   int sfd_con_##con_name##_per_element (type arg_name) {\
      return (expr);\
   }\
   int sfd_con_##con_name##_arr_loop (int N, ...) {\
      va_list args;\
      va_start(args, N);\
      int i = 0;\
      type* start = va_arg(args, type*);\
      int size = va_arg(args, uint_fast32_t);\
      for (i = 0; i < size; i++) {\
         if ( ! sfd_con_##con_name##_per_element(start[i])) {\
            return 0;\
         }\
      }\
      return 1;\
   }\
   char* sfd_con_##con_name##_arr_expr = #expr;

// can NOT be used as expression
#define sfd_arr_def_con_arr(con_name, type, func_name) \
   int sfd_con_##con_name##_array_wise (int N, ...) {\
      va_list args;\
      va_start(args, N);\
      type* start = va_arg(args, type*);\
      int size = va_arg(args, uint_fast32_t);\
      return func_name(start, size);\
   }\
   char* sfd_con_##con_name##_arr_expr = #func_name;

// can NOT be used as expression
#define sfd_arr_add_con_ele(name, con_name, arr_wise) \
   name.constraint_ele     = &sfd_con_##con_name##_per_element;\
   name.con_in_effect_ele  = #con_name;                        \
   name.con_expr_ele       = sfd_con_##con_name##_arr_expr;    \
   if (arr_wise) {\
      name.constraint_arr     = &sfd_con_##con_name##_arr_loop;\
      name.con_in_effect_arr  = #con_name;                     \
      name.con_expr_arr       = sfd_con_##con_name##_arr_expr; \
   }

// can NOT be used as expression
#define sfd_arr_add_con_arr(name, con_name) \
   name.constraint_arr     = &sfd_con_##con_name##_array_wise;\
   name.con_in_effect_arr  = #con_name;\
   name.con_expr_arr       = sfd_con_##con_name##_arr_expr;

// CAN be used as expression
#define sfd_arr_get_size(name) \
   (name.size)

// INTERNAL USE
static int sfd_memset(void *str, int c, size_t n) {
   memset(str, c, n);
   return 0;
}

#endif

#endif