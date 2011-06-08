#include <lib.h>
#define utctime _utctime
#include <time.h>

PUBLIC time_t utctime(tp)
time_t *tp;
{
  message m;
  if (_syscall(MM, UTCTIME, &m) < 0) return( (time_t) -1);
  if (tp != (time_t *)0 ) *tp = m.m2_l1;
  return(m.m2_l1);
}
