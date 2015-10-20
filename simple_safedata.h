/* simple safe data structure library
 * Author : darrenldl <dldldev@yahoo.com>
 * 
 * Version : 0.04
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

//define SIMPLE_SAFEDATA_REPORTONLY

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
   #define sfd_ptr_dec(type, name)  type name
   #define sfd_ptr_link(...)
   #define sfd_ptr_nullify(name)    (name = 0)
   #define sfd_ptr_read(name)       name
   #define sfd_ptr_write(name, in_val)    (*name = in_val)
   #define sfd_ptr_incre(name, in_val)    (*name += in_val)
   #define sfd_ptr_point_nv(name_ptr, name_var)    (name_ptr = &name_var)
   #define sfd_ptr_point_sv(name_ptr, name_var)    (name_ptr = &name_var)
   #define sfd_ptr_def_con_addr(...)
   #define sfd_ptr_add_con_addr(...)   0
   #define sfd_ptr_enforce_con_addr(...)  1
   #define sfd_ptr_deref_read(name)    (*name)
   #define sfd_ptr_deref_write(name, in_val)    (*name = in_val)
   #define sfd_ptr_deref_incre(name, in_val)    (*name += in_val)
#else

#ifdef SIMPLE_SAFEDATA_SILENT
   #define sfd_printf(...) 0
#else
   #define sfd_printf 0*printf
#endif

#ifdef __cplusplus
extern "C" {
#endif

// INTERNAL USE
static int sfd_force_exit() {
   #ifndef SIMPLE_SAFEDATA_REPORTONLY
   exit(SFD_ERR_RET_CODE);
   #endif
   return 0;
}

// INTERNAL USE
static int sfd_memset(void *str, int c, size_t n) {
   memset(str, c, n);
   return 0;
}

#ifdef __cplusplus
}
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
#define SFD_FL_SFD_VAR  0x40  // for sfd ptr
#define SFD_FL_BOUNDED  0x60  // for sfd ptr
#define SFD_FL_CON_ADDR 0x200 // for sfd ptr
#define SFD_FL_CON_VAL  0x400 // for sfd ptr

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
         (name.flags & SFD_FL_INITD ?\
            (name.val += in_val)\
         :\
             sfd_printf("sfd : Uninitialised incre : file : %s, line : %d\n", __FILE__, __LINE__)\
            +sfd_force_exit()\
         )\
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
          0* bitmap_read(&name.init_map, indx, &name.temp, 0)\
         +  (name.temp? \
               (name.start[indx] += in_val)\
            :\
                sfd_printf("sfd : Uninitialised incre : file : %s, line : %d\n", __FILE__, __LINE__)\
               +sfd_force_exit()\
            )\
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
      va_end(args);\
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
      va_end(args);\
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

typedef struct sfd_ptr_meta_data sfd_ptr_meta_data;
struct sfd_ptr_meta_data {
   void* val_ptr;
   union {
      int (*signed_char_con_addr)         (signed char*);
      int (*unsigned_char_con_addr)       (unsigned char*);
      int (*char_con_addr)                (char*);
      int (*short_con_addr)               (short*);
      int (*unsigned_short_con_addr)      (unsigned short*);
      int (*int_con_addr)                 (int*);
      int (*unsigned_int_con_addr)        (unsigned int*);
      int (*long_con_addr)                (long*);
      int (*unsigned_long_con_addr)       (unsigned long*);
      int (*long_long_con_addr)           (long long*);
      int (*unsigned_long_long_con_addr)  (unsigned long long*);
      int (*float_con_addr)               (float*);
      int (*double_con_addr)              (double*);
      int (*long_double_con_addr)         (long double*);
   } con_addr;
   union {
      int (**signed_char_con_val)          (signed char);
      int (**unsigned_char_con_val)        (unsigned char);
      int (**char_con_val)                 (char);
      int (**short_con_val)                (short);
      int (**unsigned_short_con_val)       (unsigned short);
      int (**int_con_val)                  (int);
      int (**unsigned_int_con_val)         (unsigned int);
      int (**long_con_val)                 (long);
      int (**unsigned_long_con_val)        (unsigned long);
      int (**long_long_con_val)            (long long);
      int (**unsigned_long_long_con_val)   (unsigned long long);
      int (**float_con_val)                (float);
      int (**double_con_val)               (double);
      int (**long_double_con_val)          (long double);
   } con_val;
   uint_least8_t ptr_type;
   uint_least16_t flags;
   uint_least16_t* var_flags;
   char* con_addr_in_effect;
   char* con_addr_expr;
   char** con_val_in_effect;
   char** con_val_expr;
   sfd_ptr_meta_data* prev;
   sfd_ptr_meta_data* next;
};

// ONLY USED IN ARGUMENT LIST IN DECLARATION OF FUNCTION
#define sfd_ptr_as_args(type, name) \
   sfd_ptr_meta_data* name, type* name##_sfd_ptr

// ONLY USED IN CALLING OF FUNCTION
#define sfd_ptr_pass(name) \
   &name, name##_sfd_ptr

// ONLY USED TO SET UP LOCAL SFD_PTR WITHIN A FUNCTION
#define sfd_ptr_return_meta(name) \
   *name

#define SFD_PTR_SIGNED_CHAR        0x1
#define SFD_PTR_UNSIGNED_CHAR      0x2
#define SFD_PTR_CHAR               0x3
#define SFD_PTR_SHORT              0x4
#define SFD_PTR_UNSIGNED_SHORT     0x5
#define SFD_PTR_INT                0x6
#define SFD_PTR_UNSIGNED_INT       0x7
#define SFD_PTR_LONG               0x8
#define SFD_PTR_UNSIGNED_LONG      0x9
#define SFD_PTR_LONG_LONG          0x10
#define SFD_PTR_UNSIGNED_LONG_LONG 0x11
#define SFD_PTR_FLOAT              0x12
#define SFD_PTR_DOUBLE             0x13
#define SFD_PTR_LONG_DOUBLE        0x14

// can NOT be used as expression
#define sfd_ptr_dec(type, name) \
   sfd_ptr_meta_data name;\
   type name##_sfd_ptr;\
   name.ptr_type =\
      _Generic((type) 0,\
         signed char    * : SFD_PTR_SIGNED_CHAR,   \
         unsigned char  * : SFD_PTR_UNSIGNED_CHAR, \
         char           * : SFD_PTR_CHAR,          \
         short          * : SFD_PTR_SHORT,         \
         unsigned short * : SFD_PTR_UNSIGNED_SHORT,\
         int            * : SFD_PTR_INT,           \
         unsigned int   * : SFD_PTR_UNSIGNED_INT,  \
         long           * : SFD_PTR_LONG,          \
         unsigned long  * : SFD_PTR_UNSIGNED_LONG, \
         long long      * : SFD_PTR_LONG_LONG,     \
         unsigned long long * : SFD_PTR_UNSIGNED_LONG_LONG,\
         float          * : SFD_PTR_FLOAT,         \
         double         * : SFD_PTR_DOUBLE,        \
         long double    * : SFD_PTR_LONG_DOUBLE,   \
         default :\
             sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
            +sfd_force_exit()\
      )\
   ;\
   name.flags = SFD_FL_READ | SFD_FL_WRITE | SFD_FL_CON_ADDR | SFD_FL_CON_VAL;\
   name.prev = 0;\
   name.next = 0;\
   name.var_flags = 0;\
   _Generic(name##_sfd_ptr,\
      signed char    * : name.con_addr.signed_char_con_addr        = 0,\
      unsigned char  * : name.con_addr.unsigned_char_con_addr      = 0,\
      char           * : name.con_addr.char_con_addr               = 0,\
      short          * : name.con_addr.short_con_addr              = 0,\
      unsigned short * : name.con_addr.unsigned_short_con_addr     = 0,\
      int            * : name.con_addr.int_con_addr                = 0,\
      unsigned int   * : name.con_addr.unsigned_int_con_addr       = 0,\
      long           * : name.con_addr.long_con_addr               = 0,\
      unsigned long  * : name.con_addr.unsigned_long_con_addr      = 0,\
      long long      * : name.con_addr.long_long_con_addr          = 0,\
      unsigned long long * : name.con_addr.unsigned_long_long_con_addr = 0,\
      float          * : name.con_addr.float_con_addr              = 0,\
      double         * : name.con_addr.double_con_addr             = 0,\
      long double    * : name.con_addr.long_double_con_addr        = 0,\
      default :\
          sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
   );\
   _Generic(name##_sfd_ptr,\
      signed char    * : name.con_val.signed_char_con_val          = 0,\
      unsigned char  * : name.con_val.unsigned_char_con_val        = 0,\
      char           * : name.con_val.char_con_val                 = 0,\
      short          * : name.con_val.short_con_val                = 0,\
      unsigned short * : name.con_val.unsigned_short_con_val       = 0,\
      int            * : name.con_val.int_con_val                  = 0,\
      unsigned int   * : name.con_val.unsigned_int_con_val         = 0,\
      long           * : name.con_val.long_con_val                 = 0,\
      unsigned long  * : name.con_val.unsigned_long_con_val        = 0,\
      long long      * : name.con_val.long_long_con_val            = 0,\
      unsigned long long * : name.con_val.unsigned_long_long_con_val   = 0,\
      float          * : name.con_val.float_con_val                = 0,\
      double         * : name.con_val.double_con_val               = 0,\
      long double    * : name.con_val.long_double_con_val          = 0,\
      default :\
          sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
   );

static int sfd_ptr_link_func (sfd_ptr_meta_data* old_node, sfd_ptr_meta_data* new_node, char* file, int line) {
   if (old_node == 0) {
      sfd_printf("Old node pointer is null : file : %s, line : %d\n", file, line);
      sfd_force_exit();
   }
   if (new_node == 0) {
      sfd_printf("New node pointer is null : file : %s, line : %d\n", file, line);
      sfd_force_exit();
   }
   
   if (old_node->next == 0) {    // at the end
      old_node->next = new_node;
      new_node->prev = old_node;
   }
   else if (old_node->prev == 0) {  // at the front
      old_node->prev = new_node;
      new_node->next = old_node;
   }
   else {                           // in the middle
      (old_node->next)->prev = new_node;
      new_node->next = old_node->next;
      old_node->next = new_node;
      new_node->prev = old_node;
   }
   
   return 0;
}

// CAN be used as expression
#define sfd_ptr_link(name1, name2) \
   sfd_ptr_link_func(&name1, &name2, __FILE__, __LINE__)

static int sfd_ptr_nullify_func (sfd_ptr_meta_data* node, signed char direction) {
   node->val_ptr = 0;
   node->var_flags = 0;
   if (direction == 0) {
      if (node->prev != 0) {
         sfd_ptr_nullify_func(node->prev, -1);
         node->prev = 0;
      }
      if (node->next != 0) {
         sfd_ptr_nullify_func(node->next, 1);
         node->next = 0;
      }
   }
   else if (direction < 0) {
      if (node->prev != 0) {
         sfd_ptr_nullify_func(node->prev, -1);
         node->prev = 0;
      }
   }
   else if (direction > 0) {
      if (node->next != 0) {
         sfd_ptr_nullify_func(node->next, 1);
         node->next = 0;
      }
   }
   return 0;
}

// CAN be used as expression
#define sfd_ptr_nullify(name) \
   (name.flags & SFD_FL_WRITE ?\
      sfd_ptr_nullify_func(&name, 0)\
   :\
       sfd_printf("Write to pointer not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )

// CAN be used as expression
#define sfd_ptr_read(name) \
   (\
   (name.flags & SFD_FL_READ ?\
      (name.flags & SFD_FL_INITD? \
         _Generic(name##_sfd_ptr,\
            signed char    * : (signed char*)    name.val_ptr,\
            unsigned char  * : (unsigned char*)  name.val_ptr,\
            char           * : (char*)           name.val_ptr,\
            short          * : (short*)          name.val_ptr,\
            unsigned short * : (unsigned short*) name.val_ptr,\
            int            * : (int*)            name.val_ptr,\
            unsigned int   * : (unsigned int*)   name.val_ptr,\
            long           * : (long*)           name.val_ptr,\
            unsigned long  * : (unsigned long*)  name.val_ptr,\
            long long      * : (long*)           name.val_ptr,\
            unsigned long long * : (unsigned long long*) name.val_ptr,\
            float          * : (float*)          name.val_ptr,\
            double         * : (double*)         name.val_ptr,\
            long double    * : (long double*)    name.val_ptr,\
            default :\
                sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
               +sfd_force_exit()\
         )\
      :\
          (void*)0\
         +sfd_printf("Uninitialised read from pointer : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   :\
       (void*)0\
      +sfd_printf("Read from pointer not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

// CAN be used as expression
#define sfd_ptr_write(name, in_val) \
   (\
   (name.flags & SFD_FL_WRITE ?\
      _Generic(name##_sfd_ptr,\
         signed char    * : name.val_ptr = (signed char*) in_val     + 0*(name.flags |= SFD_FL_INITD),\
         unsigned char  * : name.val_ptr = (unsigned char*) in_val   + 0*(name.flags |= SFD_FL_INITD),\
         char           * : name.val_ptr = (char*) in_val            + 0*(name.flags |= SFD_FL_INITD),\
         short          * : name.val_ptr = (short*) in_val           + 0*(name.flags |= SFD_FL_INITD),\
         unsigned short * : name.val_ptr = (unsigned short*) in_val  + 0*(name.flags |= SFD_FL_INITD),\
         int            * : name.val_ptr = (int*) in_val             + 0*(name.flags |= SFD_FL_INITD),\
         unsigned int   * : name.val_ptr = (unsigned int*) in_val    + 0*(name.flags |= SFD_FL_INITD),\
         long           * : name.val_ptr = (long*) in_val            + 0*(name.flags |= SFD_FL_INITD),\
         unsigned long  * : name.val_ptr = (unsigned long*) in_val   + 0*(name.flags |= SFD_FL_INITD),\
         long long      * : name.val_ptr = (long*) in_val            + 0*(name.flags |= SFD_FL_INITD),\
         unsigned long long * : name.val_ptr = (unsigned long long*) in_val + 0*(name.flags |= SFD_FL_INITD),\
         float          * : name.val_ptr = (float*) in_val           + 0*(name.flags |= SFD_FL_INITD),\
         double         * : name.val_ptr = (double*) in_val          + 0*(name.flags |= SFD_FL_INITD),\
         long double    * : name.val_ptr = (long double*) in_val     + 0*(name.flags |= SFD_FL_INITD),\
         default :\
             sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
            +sfd_force_exit()\
      )\
   :\
       (void*)0\
      +sfd_printf("Write to pointer not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   +\
   (name.flags & SFD_FL_CON_ADDR ?\
      sfd_ptr_enforce_con_addr(name)\
   :\
      0\
   )\
   )

// CAN be used as expression
#define sfd_ptr_incre(name, in_val) \
   (\
   (name.flags & SFD_FL_WRITE ?\
      (name.flags & SFD_FL_INITD ?\
         _Generic(name##_sfd_ptr,\
            signed char    * : name.val_ptr += (signed char*) in_val     + 0*(name.flags |= SFD_FL_INITD),\
            unsigned char  * : name.val_ptr += (unsigned char*) in_val   + 0*(name.flags |= SFD_FL_INITD),\
            char           * : name.val_ptr += (char*) in_val            + 0*(name.flags |= SFD_FL_INITD),\
            short          * : name.val_ptr += (short*) in_val           + 0*(name.flags |= SFD_FL_INITD),\
            unsigned short * : name.val_ptr += (unsigned short*) in_val  + 0*(name.flags |= SFD_FL_INITD),\
            int            * : name.val_ptr += (int*) in_val             + 0*(name.flags |= SFD_FL_INITD),\
            unsigned int   * : name.val_ptr += (unsigned int*) in_val    + 0*(name.flags |= SFD_FL_INITD),\
            long           * : name.val_ptr += (long*) in_val            + 0*(name.flags |= SFD_FL_INITD),\
            unsigned long  * : name.val_ptr += (unsigned long*) in_val   + 0*(name.flags |= SFD_FL_INITD),\
            long long      * : name.val_ptr += (long*) in_val            + 0*(name.flags |= SFD_FL_INITD),\
            unsigned long long * : name.val_ptr += (unsigned long long*) in_val + 0*(name.flags |= SFD_FL_INITD),\
            float          * : name.val_ptr += (float*) in_val           + 0*(name.flags |= SFD_FL_INITD),\
            double         * : name.val_ptr += (double*) in_val          + 0*(name.flags |= SFD_FL_INITD),\
            long double    * : name.val_ptr += (long double*) in_val     + 0*(name.flags |= SFD_FL_INITD),\
            default :\
                sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
               +sfd_force_exit()\
         )\
      :\
          sfd_printf("sfd : Uninitialised incre : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   :\
       sfd_printf("Write to pointer not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

// can NOT be used as expression
#define sfd_ptr_point_nv(name_ptr, name_var) \
   if (name_ptr.flags & SFD_FL_WRITE) {\
      (name_ptr.flags &= ~SFD_FL_SFD_VAR);\
      sfd_ptr_link_var_flags_func(&name_ptr, 0);\
      sfd_ptr_write(name_ptr, &name_var);\
      switch (name_ptr.ptr_type) {\
         case SFD_PTR_SIGNED_CHAR :\
            name_ptr.con_val.signed_char_con_val    = (void *) 0;\
            break;\
         case SFD_PTR_UNSIGNED_CHAR :\
            name_ptr.con_val.unsigned_char_con_val  = (void *) 0;\
            break;\
         case SFD_PTR_CHAR :\
            name_ptr.con_val.char_con_val           = (void *) 0;\
            break;\
         case SFD_PTR_SHORT :\
            name_ptr.con_val.short_con_val          = (void *) 0;\
            break;\
         case SFD_PTR_UNSIGNED_SHORT :\
            name_ptr.con_val.unsigned_short_con_val = (void *) 0;\
            break;\
         case SFD_PTR_INT :\
            name_ptr.con_val.int_con_val            = (void *) 0;\
            break;\
         case SFD_PTR_UNSIGNED_INT :\
            name_ptr.con_val.unsigned_int_con_val   = (void *) 0;\
            break;\
         case SFD_PTR_LONG :\
            name_ptr.con_val.long_con_val           = (void *) 0;\
            break;\
         case SFD_PTR_UNSIGNED_LONG :\
            name_ptr.con_val.unsigned_long_con_val  = (void *) 0;\
            break;\
         case SFD_PTR_LONG_LONG :\
            name_ptr.con_val.long_long_con_val      = (void *) 0;\
            break;\
         case SFD_PTR_UNSIGNED_LONG_LONG :\
            name_ptr.con_val.unsigned_long_long_con_val = (void *) 0;\
            break;\
         case SFD_PTR_FLOAT :\
            name_ptr.con_val.float_con_val          = (void *) 0;\
            break;\
         case SFD_PTR_DOUBLE :\
            name_ptr.con_val.double_con_val         = (void *) 0;\
            break;\
         case SFD_PTR_LONG_DOUBLE :\
            name_ptr.con_val.long_double_con_val    = (void *) 0;\
            break;\
         default :\
            sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__);\
            sfd_force_exit();\
      }\
   }\
   else {\
      sfd_printf("Write to pointer not permitted : file : %s, line : %d\n", __FILE__, __LINE__);\
      sfd_force_exit();\
   }

static int sfd_ptr_link_var_flags_func (sfd_ptr_meta_data* name, uint_least16_t* p_flags) {
   name->var_flags = p_flags;
   
   return 0;
}

// can NOT be used as expression
#define sfd_ptr_point_sv(name_ptr, name_var) \
   if (name_ptr.flags & SFD_FL_WRITE) {\
      (name_ptr.flags |= SFD_FL_SFD_VAR);\
      sfd_ptr_link_var_flags_func(&name_ptr, &name_var.flags);\
      sfd_ptr_write(name_ptr, &name_var.val);\
      switch (name_ptr.ptr_type) {\
         case SFD_PTR_SIGNED_CHAR :\
            name_ptr.con_val.signed_char_con_val    = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_UNSIGNED_CHAR :\
            name_ptr.con_val.unsigned_char_con_val  = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_CHAR :\
            name_ptr.con_val.char_con_val           = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_SHORT :\
            name_ptr.con_val.short_con_val          = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_UNSIGNED_SHORT :\
            name_ptr.con_val.unsigned_short_con_val = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_INT :\
            name_ptr.con_val.int_con_val            = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_UNSIGNED_INT :\
            name_ptr.con_val.unsigned_int_con_val   = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_LONG :\
            name_ptr.con_val.long_con_val           = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_UNSIGNED_LONG :\
            name_ptr.con_val.unsigned_long_con_val  = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_LONG_LONG :\
            name_ptr.con_val.long_long_con_val      = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_UNSIGNED_LONG_LONG :\
            name_ptr.con_val.unsigned_long_long_con_val = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_FLOAT :\
            name_ptr.con_val.float_con_val          = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_DOUBLE :\
            name_ptr.con_val.double_con_val         = (void *) &name_var.constraint;\
            break;\
         case SFD_PTR_LONG_DOUBLE :\
            name_ptr.con_val.long_double_con_val    = (void *) &name_var.constraint;\
            break;\
         default :\
            sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__);\
            sfd_force_exit();\
      }\
      name_ptr.con_val_in_effect = &name_var.con_in_effect;\
      name_ptr.con_val_expr = &name_var.con_expr;\
   }\
   else {\
      sfd_printf("Write to pointer not permitted : file : %s, line : %d\n", __FILE__, __LINE__);\
      sfd_force_exit();\
   }

// can NOT be used as expression
#define sfd_ptr_def_con_addr(con_name, type, arg_name, expr) \
   int sfd_con_##con_name##_addr (type arg_name) {\
      return (expr);\
   }\
   char* sfd_con_##con_name##_addr_expr = #expr;

// can NOT be used as expression
#define sfd_ptr_add_con_addr(name, con_name) \
   switch (name.ptr_type) {\
      case SFD_PTR_SIGNED_CHAR :\
         name.con_addr.signed_char_con_addr      = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_UNSIGNED_CHAR :\
         name.con_addr.unsigned_char_con_addr    = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_CHAR :\
         name.con_addr.char_con_addr             = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_SHORT :\
         name.con_addr.short_con_addr            = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_UNSIGNED_SHORT :\
         name.con_addr.unsigned_short_con_addr   = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_INT :\
         name.con_addr.int_con_addr              = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_UNSIGNED_INT :\
         name.con_addr.unsigned_int_con_addr     = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_LONG :\
         name.con_addr.long_con_addr             = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_UNSIGNED_LONG :\
         name.con_addr.unsigned_long_con_addr    = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_LONG_LONG :\
         name.con_addr.long_long_con_addr        = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_UNSIGNED_LONG_LONG :\
         name.con_addr.unsigned_long_con_addr    = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_FLOAT :\
         name.con_addr.float_con_addr            = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_DOUBLE :\
         name.con_addr.double_con_addr           = (void *) &sfd_con_##con_name##_addr;\
         break;\
      case SFD_PTR_LONG_DOUBLE :\
         name.con_addr.long_double_con_addr      = (void *) &sfd_con_##con_name##_addr;\
         break;\
      default :\
         sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__);\
         sfd_force_exit();\
   }\
   name.con_addr_in_effect = #con_name;\
   name.con_addr_expr = sfd_con_##con_name##_addr_expr;

// CAN be used as expression
#define sfd_ptr_enforce_con_addr(name) \
   (_Generic(name##_sfd_ptr,\
      signed char    * : \
         (name.con_addr.signed_char_con_addr ?\
            name.con_addr.signed_char_con_addr((signed char*) name.val_ptr)\
         :\
            1\
         ),\
      unsigned char  * : \
         (name.con_addr.unsigned_char_con_addr ?\
            name.con_addr.unsigned_char_con_addr((unsigned char*) name.val_ptr)\
         :\
            1\
         ),\
      char           * : \
         (name.con_addr.char_con_addr ?\
            name.con_addr.char_con_addr((char*) name.val_ptr)\
         :\
            1\
         ),\
      short          * : \
         (name.con_addr.short_con_addr ?\
            name.con_addr.short_con_addr((short*) name.val_ptr)\
         :\
            1\
         ),\
      unsigned short * : \
         (name.con_addr.unsigned_short_con_addr ?\
            name.con_addr.unsigned_short_con_addr((unsigned short*) name.val_ptr)\
         :\
            1\
         ),\
      int            * : \
         (name.con_addr.int_con_addr ?\
            name.con_addr.int_con_addr((int*) name.val_ptr)\
         :\
            1\
         ),\
      unsigned int   * : \
         (name.con_addr.unsigned_int_con_addr ?\
            name.con_addr.unsigned_int_con_addr((unsigned int*) name.val_ptr)\
         :\
            1\
         ),\
      long           * : \
         (name.con_addr.long_con_addr ?\
            name.con_addr.long_con_addr((long*) name.val_ptr)\
         :\
            1\
         ),\
      unsigned long  * : \
         (name.con_addr.unsigned_long_con_addr ?\
            name.con_addr.unsigned_long_con_addr((unsigned long*) name.val_ptr)\
         :\
            1\
         ),\
      long long      * : \
         (name.con_addr.long_long_con_addr ?\
            name.con_addr.long_long_con_addr((long long*) name.val_ptr)\
         :\
            1\
         ),\
      unsigned long long * : \
         (name.con_addr.unsigned_long_long_con_addr ?\
            name.con_addr.unsigned_long_long_con_addr((unsigned long long*) name.val_ptr)\
         :\
            1\
         ),\
      float          * : \
         (name.con_addr.float_con_addr ?\
            name.con_addr.float_con_addr((float*) name.val_ptr)\
         :\
            1\
         ),\
      double         * : \
         (name.con_addr.double_con_addr ?\
            name.con_addr.double_con_addr((double*) name.val_ptr)\
         :\
            1\
         ),\
      long double    * : \
         (name.con_addr.long_double_con_addr ?\
            name.con_addr.long_double_con_addr((long double*) name.val_ptr)\
         :\
            1\
         ),\
      default :\
          sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   ?\
      0\
   :\
       sfd_printf("Constraint on pointer failed : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_printf("  Constraint in effect  : %s\n", name.con_addr_in_effect)\
      +sfd_printf("  Constraint expression : %s\n", name.con_addr_expr)\
      +sfd_force_exit()\
   )

// CAN be used as expression
#define sfd_ptr_deref_read(name) \
   (\
   (name.flags & SFD_FL_READ ?\
      (name.val_ptr ?\
         (name.flags & SFD_FL_SFD_VAR ?\
            (*name.var_flags & SFD_FL_READ ?\
               (*name.var_flags & SFD_FL_INITD ?\
                     _Generic(name##_sfd_ptr,\
                        signed char    * : *((signed char*)    name.val_ptr),\
                        unsigned char  * : *((unsigned char*)  name.val_ptr),\
                        char           * : *((char*)           name.val_ptr),\
                        short          * : *((short*)          name.val_ptr),\
                        unsigned short * : *((unsigned short*) name.val_ptr),\
                        int            * : *((int*)            name.val_ptr),\
                        unsigned int   * : *((unsigned int*)   name.val_ptr),\
                        long           * : *((long*)           name.val_ptr),\
                        unsigned long  * : *((unsigned long*)  name.val_ptr),\
                        long long      * : *((long long*)      name.val_ptr),\
                        unsigned long long * : *((unsigned long long*) name.val_ptr),\
                        float          * : *((float*)          name.val_ptr),\
                        double         * : *((double*)         name.val_ptr),\
                        long double    * : *((long double*)    name.val_ptr),\
                        default :\
                            sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
                           +sfd_force_exit()\
                     )\
               :\
                   sfd_printf("Uninitialised read : file : %s, line : %d\n", __FILE__, __LINE__)\
                  +sfd_force_exit()\
               )\
            :\
                sfd_printf("Read from variable pointed to not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
               +sfd_force_exit()\
            )\
         :\
            _Generic(name##_sfd_ptr,\
               signed char    * : *((signed char*)    name.val_ptr),\
               unsigned char  * : *((unsigned char*)  name.val_ptr),\
               char           * : *((char*)           name.val_ptr),\
               short          * : *((short*)          name.val_ptr),\
               unsigned short * : *((unsigned short*) name.val_ptr),\
               int            * : *((int*)            name.val_ptr),\
               unsigned int   * : *((unsigned int*)   name.val_ptr),\
               long           * : *((long*)           name.val_ptr),\
               unsigned long  * : *((unsigned long*)  name.val_ptr),\
               long long      * : *((long long*)      name.val_ptr),\
               unsigned long long * : *((unsigned long long*) name.val_ptr),\
               float          * : *((float*)          name.val_ptr),\
               double         * : *((double*)         name.val_ptr),\
               long double    * : *((long double*)    name.val_ptr),\
               default :\
                   sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
                  +sfd_force_exit()\
            )\
         )\
      :\
          sfd_printf("Null pointer deref read : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   :\
       sfd_printf("Read from pointer not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

// CAN be used as expression
#define sfd_ptr_deref_write(name, in_val) \
   (\
   (name.flags & SFD_FL_READ ?\
      (name.val_ptr ?\
         (name.flags & SFD_FL_SFD_VAR ?\
            (*name.var_flags & SFD_FL_WRITE ?\
                  _Generic(name##_sfd_ptr,\
                        signed char    * : *((signed char*)    name.val_ptr) = (signed char)    in_val,\
                        unsigned char  * : *((unsigned char*)  name.val_ptr) = (unsigned char)  in_val,\
                        char           * : *((char*)           name.val_ptr) = (char)           in_val,\
                        short          * : *((short*)          name.val_ptr) = (short)          in_val,\
                        unsigned short * : *((unsigned short*) name.val_ptr) = (unsigned short) in_val,\
                        int            * : *((int*)            name.val_ptr) = (int)            in_val,\
                        unsigned int   * : *((unsigned int*)   name.val_ptr) = (unsigned int)   in_val,\
                        long           * : *((long*)           name.val_ptr) = (long)           in_val,\
                        unsigned long  * : *((unsigned long*)  name.val_ptr) = (unsigned long)  in_val,\
                        long long      * : *((long long*)      name.val_ptr) = (long long)      in_val,\
                        unsigned long long * : *((unsigned long long*) name.val_ptr) = (unsigned long long) in_val,\
                        float          * : *((float*)          name.val_ptr) = (float)          in_val,\
                        double         * : *((double*)         name.val_ptr) = (double)         in_val,\
                        long double    * : *((long double*)    name.val_ptr) = (long double)    in_val,\
                     default :\
                         sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
                        +sfd_force_exit()\
                  )\
                  +\
                  (*name.var_flags & SFD_FL_CON ?\
                     (_Generic(name##_sfd_ptr,\
                        signed char    * : \
                           (name.con_val.signed_char_con_val ?\
                              (*name.con_val.signed_char_con_val ?\
                                 (*name.con_val.signed_char_con_val)     (*((signed char*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        unsigned char  * : \
                           (name.con_val.unsigned_char_con_val ?\
                              (*name.con_val.unsigned_char_con_val ?\
                                 (*name.con_val.unsigned_char_con_val)     (*((unsigned char*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        char           * : \
                           (name.con_val.char_con_val ?\
                              (*name.con_val.char_con_val ?\
                                 (*name.con_val.char_con_val)     (*((char*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        short          * : \
                           (name.con_val.short_con_val ?\
                              (*name.con_val.short_con_val ?\
                                 (*name.con_val.short_con_val)     (*((short*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        unsigned short * : \
                           (name.con_val.unsigned_short_con_val ?\
                              (*name.con_val.unsigned_short_con_val ?\
                                 (*name.con_val.unsigned_short_con_val)     (*((unsigned short*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        int            * : \
                           (name.con_val.int_con_val ?\
                              (*name.con_val.int_con_val ?\
                                 (*name.con_val.int_con_val)     (*((int*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        unsigned int   * : \
                           (name.con_val.unsigned_int_con_val ?\
                              (*name.con_val.unsigned_int_con_val ?\
                                 (*name.con_val.unsigned_int_con_val)     (*((unsigned int*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        long           * : \
                           (name.con_val.long_con_val ?\
                              (*name.con_val.long_con_val ?\
                                 (*name.con_val.long_con_val)     (*((long*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        unsigned long  * : \
                           (name.con_val.unsigned_long_con_val ?\
                              (*name.con_val.unsigned_long_con_val ?\
                                 (*name.con_val.unsigned_long_con_val)     (*((unsigned long*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        long long      * : \
                           (name.con_val.long_long_con_val ?\
                              (*name.con_val.long_long_con_val ?\
                                 (*name.con_val.long_long_con_val)     (*((long long*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        unsigned long long * : \
                           (name.con_val.unsigned_long_long_con_val ?\
                              (*name.con_val.unsigned_long_long_con_val ?\
                                 (*name.con_val.unsigned_long_long_con_val)     (*((unsigned long long*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        float          * : \
                           (name.con_val.float_con_val ?\
                              (*name.con_val.float_con_val ?\
                                 (*name.con_val.float_con_val)     (*((float*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        double         * : \
                           (name.con_val.double_con_val ?\
                              (*name.con_val.double_con_val ?\
                                 (*name.con_val.double_con_val)     (*((double*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        long double    * : \
                           (name.con_val.long_double_con_val ?\
                              (*name.con_val.long_double_con_val ?\
                                 (*name.con_val.long_double_con_val)     (*((long double*) name.val_ptr))\
                              :\
                                 1\
                              )\
                           :\
                              1\
                           ),\
                        default :\
                            sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
                           +sfd_force_exit()\
                        )\
                     ?\
                        0\
                     :\
                         sfd_printf("Constraint on variable pointed to failed : file : %s, line : %d\n", __FILE__, __LINE__)\
                        +sfd_printf("  Constraint in effect  : %s\n", *name.con_val_in_effect)\
                        +sfd_printf("  Constraint expression : %s\n", *name.con_val_expr)\
                        +sfd_force_exit()\
                     )\
                     +\
                     (*name.var_flags |= SFD_FL_INITD)\
                  :\
                     0\
                  )\
            :\
                sfd_printf("Write to variable pointed to not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
               +sfd_force_exit()\
            )\
         :\
            _Generic(name##_sfd_ptr,\
               signed char    * : *((signed char*)    name.val_ptr) = (signed char)    in_val,\
               unsigned char  * : *((unsigned char*)  name.val_ptr) = (unsigned char)  in_val,\
               char           * : *((char*)           name.val_ptr) = (char)           in_val,\
               short          * : *((short*)          name.val_ptr) = (short)          in_val,\
               unsigned short * : *((unsigned short*) name.val_ptr) = (unsigned short) in_val,\
               int            * : *((int*)            name.val_ptr) = (int)            in_val,\
               unsigned int   * : *((unsigned int*)   name.val_ptr) = (unsigned int)   in_val,\
               long           * : *((long*)           name.val_ptr) = (long)           in_val,\
               unsigned long  * : *((unsigned long*)  name.val_ptr) = (unsigned long)  in_val,\
               long long      * : *((long long*)      name.val_ptr) = (long long)      in_val,\
               unsigned long long * : *((unsigned long long*) name.val_ptr) = (unsigned long long) in_val,\
               float          * : *((float*)          name.val_ptr) = (float)          in_val,\
               double         * : *((double*)         name.val_ptr) = (double)         in_val,\
               long double    * : *((long double*)    name.val_ptr) = (long double)    in_val,\
               default :\
                   sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
                  +sfd_force_exit()\
            )\
         )\
      :\
          sfd_printf("Null pointer deref write : file : %s, line : %d\n", __FILE__, __LINE__)\
         +sfd_force_exit()\
      )\
   :\
       sfd_printf("Read from pointer not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

// CAN be used as expression
#define sfd_ptr_deref_incre(name, in_val) \
   (\
   (name.flags & SFD_FL_READ ?\
      (name.flags & SFD_FL_SFD_VAR ?\
         (*name.var_flags & SFD_FL_WRITE ?\
            (*name.var_flags & SFD_FL_INITD ?\
               _Generic(name##_sfd_ptr,\
                  signed char    * : *((signed char*)    name.val_ptr) += (signed char)    in_val,\
                  unsigned char  * : *((unsigned char*)  name.val_ptr) += (unsigned char)  in_val,\
                  char           * : *((char*)           name.val_ptr) += (char)           in_val,\
                  short          * : *((short*)          name.val_ptr) += (short)          in_val,\
                  unsigned short * : *((unsigned short*) name.val_ptr) += (unsigned short) in_val,\
                  int            * : *((int*)            name.val_ptr) += (int)            in_val,\
                  unsigned int   * : *((unsigned int*)   name.val_ptr) += (unsigned int)   in_val,\
                  long           * : *((long*)           name.val_ptr) += (long)           in_val,\
                  unsigned long  * : *((unsigned long*)  name.val_ptr) += (unsigned long)  in_val,\
                  long long      * : *((long long*)      name.val_ptr) += (long long)      in_val,\
                  unsigned long long * : *((unsigned long long*) name.val_ptr) += (unsigned long long) in_val,\
                  float          * : *((float*)          name.val_ptr) += (float)          in_val,\
                  double         * : *((double*)         name.val_ptr) += (double)         in_val,\
                  long double    * : *((long double*)    name.val_ptr) += (long double)    in_val,\
                  default :\
                      sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
                     +sfd_force_exit()\
               )\
               +\
               (*name.var_flags & SFD_FL_CON ?\
                  (_Generic(name##_sfd_ptr,\
                     signed char    * : \
                        (name.con_val.signed_char_con_val ?\
                           (*name.con_val.signed_char_con_val ?\
                              (*name.con_val.signed_char_con_val)     (*((signed char*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     unsigned char  * : \
                        (name.con_val.unsigned_char_con_val ?\
                           (*name.con_val.unsigned_char_con_val ?\
                              (*name.con_val.unsigned_char_con_val)     (*((unsigned char*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     char           * : \
                        (name.con_val.char_con_val ?\
                           (*name.con_val.char_con_val ?\
                              (*name.con_val.char_con_val)     (*((char*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     short          * : \
                        (name.con_val.short_con_val ?\
                           (*name.con_val.short_con_val ?\
                              (*name.con_val.short_con_val)     (*((short*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     unsigned short * : \
                        (name.con_val.unsigned_short_con_val ?\
                           (*name.con_val.unsigned_short_con_val ?\
                              (*name.con_val.unsigned_short_con_val)     (*((unsigned short*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     int            * : \
                        (name.con_val.int_con_val ?\
                           (*name.con_val.int_con_val ?\
                              (*name.con_val.int_con_val)     (*((int*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     unsigned int   * : \
                        (name.con_val.unsigned_int_con_val ?\
                           (*name.con_val.unsigned_int_con_val ?\
                              (*name.con_val.unsigned_int_con_val)     (*((unsigned int*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     long           * : \
                        (name.con_val.long_con_val ?\
                           (*name.con_val.long_con_val ?\
                              (*name.con_val.long_con_val)     (*((long*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     unsigned long  * : \
                        (name.con_val.unsigned_long_con_val ?\
                           (*name.con_val.unsigned_long_con_val ?\
                              (*name.con_val.unsigned_long_con_val)     (*((unsigned long*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     long long      * : \
                        (name.con_val.long_long_con_val ?\
                           (*name.con_val.long_long_con_val ?\
                              (*name.con_val.long_long_con_val)     (*((long long*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     unsigned long long * : \
                        (name.con_val.unsigned_long_long_con_val ?\
                           (*name.con_val.unsigned_long_long_con_val ?\
                              (*name.con_val.unsigned_long_long_con_val)     (*((unsigned long long*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     float          * : \
                        (name.con_val.float_con_val ?\
                           (*name.con_val.float_con_val ?\
                              (*name.con_val.float_con_val)     (*((float*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     double         * : \
                        (name.con_val.double_con_val ?\
                           (*name.con_val.double_con_val ?\
                              (*name.con_val.double_con_val)     (*((double*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     long double    * : \
                        (name.con_val.long_double_con_val ?\
                           (*name.con_val.long_double_con_val ?\
                              (*name.con_val.long_double_con_val)     (*((long double*) name.val_ptr))\
                           :\
                              1\
                           )\
                        :\
                           1\
                        ),\
                     default :\
                         sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
                        +sfd_force_exit()\
                     )\
                  ?\
                     0\
                  :\
                      sfd_printf("Constraint on variable pointed to failed : file : %s, line : %d\n", __FILE__, __LINE__)\
                     +sfd_printf("  Constraint in effect  : %s\n", *name.con_val_in_effect)\
                     +sfd_printf("  Constraint expression : %s\n", *name.con_val_expr)\
                     +sfd_force_exit()\
                  )\
                  +\
                  (*name.var_flags |= SFD_FL_INITD)\
               :\
                  0\
               )\
            :\
                sfd_printf("sfd : Uninitialised incre : file : %s, line : %d\n", __FILE__, __LINE__)\
               +sfd_force_exit()\
            )\
         :\
             sfd_printf("Write to variable pointed to not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
            +sfd_force_exit()\
         )\
      :\
         _Generic(name##_sfd_ptr,\
            signed char    * : *((signed char*)    name.val_ptr) += (signed char)    in_val,\
            unsigned char  * : *((unsigned char*)  name.val_ptr) += (unsigned char)  in_val,\
            char           * : *((char*)           name.val_ptr) += (char)           in_val,\
            short          * : *((short*)          name.val_ptr) += (short)          in_val,\
            unsigned short * : *((unsigned short*) name.val_ptr) += (unsigned short) in_val,\
            int            * : *((int*)            name.val_ptr) += (int)            in_val,\
            unsigned int   * : *((unsigned int*)   name.val_ptr) += (unsigned int)   in_val,\
            long           * : *((long*)           name.val_ptr) += (long)           in_val,\
            unsigned long  * : *((unsigned long*)  name.val_ptr) += (unsigned long)  in_val,\
            long long      * : *((long long*)      name.val_ptr) += (long long)      in_val,\
            unsigned long long * : *((unsigned long long*) name.val_ptr) += (unsigned long long) in_val,\
            float          * : *((float*)          name.val_ptr) += (float)          in_val,\
            double         * : *((double*)         name.val_ptr) += (double)         in_val,\
            long double    * : *((long double*)    name.val_ptr) += (long double)    in_val,\
            default :\
                sfd_printf("Unexpected type : file : %s, line : %d\n", __FILE__, __LINE__)\
               +sfd_force_exit()\
         )\
      )\
   :\
       sfd_printf("Read from pointer not permitted : file : %s, line : %d\n", __FILE__, __LINE__)\
      +sfd_force_exit()\
   )\
   )

#endif

#endif