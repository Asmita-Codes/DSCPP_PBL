#include "leaderboard.h"
#include <string.h>
#include <stdio.h>
void leaderboard_sort(DnScoreEntry*a,size_t n){if(!a)return;for(size_t i=1;i<n;i++){DnScoreEntry x=a[i];size_t j=i;while(j&&a[j-1].score<x.score){a[j]=a[j-1];j--;}a[j]=x;}}
int leaderboard_find(const DnScoreEntry*a,size_t n,const char*name){if(!a||!name)return -1;for(size_t i=0;i<n;i++)if(strcmp(a[i].name,name)==0)return (int)i;return -1;}
int leaderboard_load(DnScoreEntry*a,size_t cap,const char*f){if(!a||!f)return 0;FILE*p=fopen(f,"r");if(!p)return 0;int n=0;while((size_t)n<cap&&fscanf(p,"%31s %d %d",a[n].name,&a[n].score,&a[n].seconds)==3)n++;fclose(p);leaderboard_sort(a,(size_t)n);return n;}
int leaderboard_save(const DnScoreEntry*a,size_t n,const char*f){if(!a||!f)return 0;FILE*p=fopen(f,"w");if(!p)return 0;for(size_t i=0;i<n;i++)fprintf(p,"%31s %d %d\n",a[i].name,a[i].score,a[i].seconds);fclose(p);return 1;}
