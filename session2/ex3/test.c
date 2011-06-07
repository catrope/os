#include<time.h>
#include<stdio.h>

int main()
{
  printf("time    = %u\n", time(0));
  printf("utctime = %u\n", utctime(0));
  return 0;
}
