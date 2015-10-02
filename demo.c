#include "simple_safedata.h"

/* Note:
 *    This file will cause all the runtime errors sfd is meant to handle
 *    
 *    Just comment out the noted faulty lines reported as you move on
 *    
 *    The faulty lines have a line of comment starting with "following ..." above
 */

int main () {
   // declare a sfd variable
   sfd_var_dec(int, k);    // function same as int k, but tied to function suite provided in sfd
   
   // declare a sfd static array
   sfd_arr_dec_sta(int, arr0, 100);

   // test cases for variable
      // sfd blocks uninitialised reads
      // following will thus cause termination
      printf("k : %d\n", sfd_var_read(k));
      
      // set lower and upper bound (both are inclusive)
      sfd_var_set_lo_bnd(k, 10);
      sfd_var_set_up_bnd(k, 20);
      
      // following will cause the program to terminate due to bound breaching
      sfd_var_write(k, 30);
      
      // now we do a normal write
      sfd_var_write(k, 20);
      
      // and of course we can read as it's finally initialised
      printf("Finally can read k, k : %d\n", sfd_var_read(k));
      
      // sfd provides read write permissions
      // by default, both read and write are enabled
      
      // now we disable read
      sfd_flag_disable(k, SFD_FL_READ);
      // following will cause it t throw up as a result
      sfd_var_read(k);
      
      // we can do similar to write
      sfd_flag_disable(k, SFD_FL_WRITE);
      // following will cause it to throw up as well
      sfd_var_write(k, 10);
      
      // then we undo both
      sfd_flag_enable(k, SFD_FL_READ | SFD_FL_WRITE);
      
      // one may add constraint to the variable, which is enforced before and after each sfd write
      sfd_add_con(k, int, x, x % 2 == 0);    // here we require k to always be an even number
      
      // now we write an odd number
      // following will obviously terminate
      sfd_var_write(k, 11);
   
   // test case for array
      // sfd array has all of above, except for bound checking
      // note that sfd array tracks individual initialisation of element using a bitmap
      
      // we try to read an uninitialised int
      // following will terminate the program
      sfd_arr_read(arr0, 1);
      
      // we initialise an int
      sfd_arr_write(arr0, 19, 404);
      
      // and of course we can read from it now
      printf("arr0[19] : %d\n", sfd_arr_read(arr0, 19));
      
      // and of course other unitialised ones will not work
      // following will terminate the program
      sfd_arr_read(arr0, 10);
      
      // you can choose to initialise everything first (with 0)
      sfd_arr_wipe(arr0);
      
      // since all elements are initialised, we can read from anywhere now
      sfd_arr_read(arr0, 99);
   
   return 0;
}