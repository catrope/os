/* This file takes care of those system calls that deal with time.
 *
 * The entry points into this file are
 *   do_time:		perform the TIME system call
 *   do_stime:		perform the STIME system call
 *   do_times:		perform the TIMES system call
 */

#include "pm.h"
#include <minix/callnr.h>
#include <minix/com.h>
#include <signal.h>
#include "mproc.h"
#include "param.h"

/*===========================================================================*
 *				do_time					     *
 *===========================================================================*/
PUBLIC int do_time()
{
/* Perform the time(tp) system call. This returns the time in seconds since 
 * 1.1.1970.  MINIX is an astrophysically naive system that assumes the earth 
 * rotates at a constant rate and that such things as leap seconds do not 
 * exist.
 */
  clock_t uptime, boottime;
  int s;

  if ( (s=getuptime2(&uptime, &boottime)) != OK) 
  	panic(__FILE__,"do_time couldn't get uptime", s);

  mp->mp_reply.reply_time = (time_t) (boottime + (uptime/system_hz));
  mp->mp_reply.reply_utime = (uptime%system_hz)*1000000/system_hz;
  return(OK);
}

/* macro to save some space */
#define LEAP(x) if(rt>=x) { rt--; }

PUBLIC int do_utctime()
{
  clock_t uptime, boottime;
  time_t rt;
  int s;

  if ( (s=getuptime2(&uptime, &boottime)) != OK)
        panic(__FILE__, "do_utctime couldn't get uptime", s);

  rt = (time_t) (boottime + (uptime/system_hz));
  mp->mp_reply.reply_utime = (uptime%system_hz)*1000000/system_hz;
  
/* Correct for leap seconds.
 */
  if ( rt >= 1230768023 )
  {
  /* This will be used in practically all cases.
   */
  	rt -= 24;
  }
  else
  {
  /* If for some reason the clock is behind, do the correction right.
   */
    LEAP(78796800); LEAP(94694400); LEAP(126230400); LEAP(157766400); LEAP(189302400);
    LEAP(220924800); LEAP(252460800); LEAP(283996800); LEAP(315532800); LEAP(362793600); 
    LEAP(394329600); LEAP(425865600); LEAP(489024000); LEAP(567993600); LEAP(631152000); 
    LEAP(662688000); LEAP(709948800); LEAP(741484800); LEAP(773020800); LEAP(820454400); 
    LEAP(867715200); LEAP(915148800); LEAP(1136073600); LEAP(1230768000);
  }
  
  mp->mp_reply.reply_time = rt;
  
  return(OK);
}

/*===========================================================================*
 *				do_stime				     *
 *===========================================================================*/
PUBLIC int do_stime()
{
/* Perform the stime(tp) system call. Retrieve the system's uptime (ticks 
 * since boot) and pass the new time in seconds at system boot to the kernel.
 */
  clock_t uptime, boottime;
  int s;

  if (mp->mp_effuid != SUPER_USER) { 
      return(EPERM);
  }
  if ( (s=getuptime(&uptime)) != OK) 
      panic(__FILE__,"do_stime couldn't get uptime", s);
  boottime = (long) m_in.stime - (uptime/system_hz);

  s= sys_stime(boottime);		/* Tell kernel about boottime */
  if (s != OK)
	panic(__FILE__, "pm: sys_stime failed", s);

  return(OK);
}

/*===========================================================================*
 *				do_times				     *
 *===========================================================================*/
PUBLIC int do_times()
{
/* Perform the times(buffer) system call. */
  register struct mproc *rmp = mp;
  clock_t user_time, sys_time, uptime;
  int s;

  if (OK != (s=sys_times(who_e, &user_time, &sys_time, &uptime, NULL)))
      panic(__FILE__,"do_times couldn't get times", s);
  rmp->mp_reply.reply_t1 = user_time;		/* user time */
  rmp->mp_reply.reply_t2 = sys_time;		/* system time */
  rmp->mp_reply.reply_t3 = rmp->mp_child_utime;	/* child user time */
  rmp->mp_reply.reply_t4 = rmp->mp_child_stime;	/* child system time */
  rmp->mp_reply.reply_t5 = uptime;		/* uptime since boot */

  return(OK);
}

