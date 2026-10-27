/* scan.c - byte-pattern search and hex context dump over a file.
 * usage:
 *   scan find   <file> <hexbytes>             list all offsets of pattern
 *   scan near   <file> <hexA> <hexB> <win>    offsets where A and B within win bytes
 *   scan dump   <file> <off> <len>            hex+ascii dump
 *   scan u16all <file>                        (unused)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *buf; static long n;

static long load(const char *p){
    FILE *f=fopen(p,"rb"); if(!f){perror(p);exit(1);}
    fseek(f,0,SEEK_END); n=ftell(f); fseek(f,0,SEEK_SET);
    buf=malloc(n); if(fread(buf,1,n,f)!=(size_t)n){perror("read");exit(1);} fclose(f);
    return n;
}
static int hexparse(const char *s, unsigned char *out){
    int k=0; while(*s){ if(s[0]==' '){s++;continue;}
        unsigned v; sscanf(s,"%2x",&v); out[k++]=v; s+=2; if(*s==' ')s++; }
    return k;
}
static void dumprange(long off,long len){
    for(long i=off;i<off+len && i<n;i+=16){
        printf("%08lx  ",i);
        for(long j=0;j<16;j++){ if(i+j<off+len && i+j<n) printf("%02x ",buf[i+j]); else printf("   ");
            if(j==7)printf(" "); }
        printf(" |");
        for(long j=0;j<16;j++){ if(i+j<off+len && i+j<n){ unsigned char c=buf[i+j]; printf("%c",(c>=32&&c<127)?c:'.'); } }
        printf("|\n");
    }
}
int main(int argc,char**argv){
    if(argc<3){fprintf(stderr,"need args\n");return 1;}
    const char *cmd=argv[1];
    load(argv[2]);
    if(!strcmp(cmd,"find")){
        unsigned char pat[64]; int pl=hexparse(argv[3],pat);
        for(long i=0;i+pl<=n;i++) if(!memcmp(buf+i,pat,pl)) printf("%ld 0x%lx\n",i,i);
    } else if(!strcmp(cmd,"near")){
        unsigned char a[64],b[64]; int al=hexparse(argv[3],a),bl=hexparse(argv[4],b);
        long win=atol(argv[5]);
        for(long i=0;i+al<=n;i++) if(!memcmp(buf+i,a,al)){
            for(long j=(i>win?i-win:0); j+bl<=n && j<=i+win; j++)
                if(!memcmp(buf+j,b,bl)) printf("A@0x%lx B@0x%lx (delta %ld)\n",i,j,j-i);
        }
    } else if(!strcmp(cmd,"dump")){
        dumprange(atol(argv[3]),atol(argv[4]));
    } else if(!strcmp(cmd,"map12")){
        /* decode 12-byte records from given start for count records */
        long off=strtol(argv[3],0,0); long cnt=atol(argv[4]);
        for(long k=0;k<cnt;k++){ long p=off+k*12; if(p+12>n)break;
            unsigned id=*(unsigned*)(buf+p);
            unsigned short a=*(unsigned short*)(buf+p+4);
            unsigned short ofs=*(unsigned short*)(buf+p+6);
            unsigned short sz=*(unsigned short*)(buf+p+8);
            unsigned short at=*(unsigned short*)(buf+p+10);
            printf("0x%06lx id=0x%04x vsel=0x%04x off=0x%04x sz=0x%04x attr=0x%04x\n",p,id,a,ofs,sz,at);
        }
    } else if(!strcmp(cmd,"mapauto")){
        /* find maximal run of 12B records with strictly increasing off field (bytes p+6..7) */
        long best=0,bstart=0,blen=0;
        for(long s=0;s+12<=n;s++){
            long p=s; long len=0; int prev=-1; int ok=1;
            while(p+12<=n){
                unsigned short ofs=*(unsigned short*)(buf+p+6);
                unsigned short sz=*(unsigned short*)(buf+p+8);
                unsigned id=*(unsigned*)(buf+p);
                if(id==0||id>0x4000) break;
                if(sz==0||sz>8) break;
                if((int)ofs<=prev) break;
                prev=ofs; len++; p+=12;
            }
            (void)ok;
            if(len>blen){ blen=len; bstart=s; best=s; }
        }
        (void)best;
        printf("run start=0x%lx len=%ld end=0x%lx\n",bstart,blen,bstart+blen*12);
    }
    return 0;
}
