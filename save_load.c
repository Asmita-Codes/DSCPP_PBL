#include "save_load.h"
#include <stdio.h>
int save_load_save(const char*p,int s,int h,int k,int t,int r,int pi,int m){if(!p)return 0;FILE*f=fopen(p,"w");if(!f)return 0;int ok=fprintf(f,"DNEMS_SAVE 1\n%d %d %d %d %d %d %d\n",s,h,k,t,r,pi,m)>0;fclose(f);return ok;}
int save_load_load(const char*p,int*s,int*h,int*k,int*t,int*r,int*pi,int*m){if(!p)return 0;FILE*f=fopen(p,"r");if(!f)return 0;char magic[32];int ver,ok=fscanf(f,"%31s %d",magic,&ver)==2&&magic[0]=='D'&&ver==1;if(ok)ok=fscanf(f,"%d %d %d %d %d %d %d",s,h,k,t,r,pi,m)==7;fclose(f);return ok;}
