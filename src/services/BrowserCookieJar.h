#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// A conservative host-only cookie subset. Never send host A's cookies to
// host B, even when an untrusted Set-Cookie names a parent/public suffix.
// Does not attempt to provide a browser-grade Public Suffix List or full RFC6265.
class BrowserCookieJar {
public:
  static constexpr int CAP=12;
  struct Entry {
    char host[96], path[64], name[32], value[96];
    bool secure;
  };
  int size() const {return count;}
  void clear() {count=0;memset(items,0,sizeof(items));}
  static bool urlParts(const char *url,char *host,size_t hc,
                       char *path,size_t pc,bool &https) {
    https=false;
    const char *p=nullptr;
    if(!strncmp(url,"https://",8)){p=url+8;https=true;}
    else if(!strncmp(url,"http://",7))p=url+7;
    else return false;
    const char *end=p;
    while(*end&&*end!='/'&&*end!='?'&&*end!='#')++end;
    size_t n=(size_t)(end-p);
    if(!n||n>=hc)return false;
    for(size_t i=0;i<n;++i){
      unsigned char c=(unsigned char)p[i];
      if(!(isalnum(c)||c=='.'||c=='-'||c==':'))return false;
      host[i]=(char)tolower(c);
    }
    host[n]=0;
    char *colon=strchr(host,':');if(colon)*colon=0;
    if(!host[0]||!strchr(host,'.'))return false;
    const char *at=*end=='/'?end:"/";
    const char *q=at;
    while(*q&&*q!='?'&&*q!='#')++q;
    n=(size_t)(q-at);if(!n||n>=pc)return false;
    memcpy(path,at,n);path[n]=0;
    return true;
  }
  bool ingest(const char *url,const char *header) {
    if(!url||!header)return false;
    char host[96],uri[192];bool https=false;
    if(!urlParts(url,host,sizeof(host),uri,sizeof(uri),https))return false;
    const char *eq=strchr(header,'=');
    if(!eq||eq==header)return false;
    const char *semicolon=strchr(header,';');
    if(semicolon&&eq>semicolon)return false;
    size_t nl=(size_t)(eq-header);
    if(nl>=sizeof(Entry::name)||!nl)return false;
    char name[32];memcpy(name,header,nl);name[nl]=0;
    for(size_t i=0;i<nl;++i)
      if(!(isalnum((unsigned char)name[i])||name[i]=='_'||name[i]=='-'||name[i]=='.'))
        return false;
    const char *v=eq+1,*ve=semicolon?semicolon:header+strlen(header);
    while(v<ve&&(*v==' '||*v=='"'))++v;
    while(ve>v&&(ve[-1]==' '||ve[-1]=='"'))--ve;
    size_t vl=(size_t)(ve-v);
    if(vl>=sizeof(Entry::value))return false;
    char value[96];memcpy(value,v,vl);value[vl]=0;
    for(size_t i=0;i<vl;++i)
      if((unsigned char)value[i]<33||(unsigned char)value[i]>126||value[i]==';'||value[i]=='\\')
        return false;
    bool secure=false,remove=false,domainSpecified=false;
    char path[64]="/";
    const char *slash=strrchr(uri,'/');
    if(slash&&slash>uri){
      size_t n=(size_t)(slash-uri);
      if(n<sizeof(path)){memcpy(path,uri,n);path[n]=0;}
    }
    const char *p=semicolon;
    while(p&&*p){
      ++p;while(*p==' '||*p=='\t')++p;
      const char *next=strchr(p,';');
      const char *end=next?next:p+strlen(p);
      if((size_t)(end-p)>=6&&!strncasecmpPortable(p,"Secure",6)
         &&(end==p+6||p[6]==' '))secure=true;
      if((size_t)(end-p)>=7&&!strncasecmpPortable(p,"Domain=",7)){
        domainSpecified=true;
        const char *d=p+7;
        while(d<end&&*d=='.')++d;
        size_t n=(size_t)(end-d);
        if(n!=strlen(host)||strncasecmpPortable(d,host,n))return false;
      }
      if((size_t)(end-p)>=5&&!strncasecmpPortable(p,"Path=",5)){
        const char *q=p+5;size_t n=(size_t)(end-q);
        if(n>=sizeof(path)||!n||*q!='/')return false;
        for(size_t k=0;k<n;++k)if((unsigned char)q[k]<33)return false;
        memcpy(path,q,n);path[n]=0;
      }
      if((size_t)(end-p)>=9&&!strncasecmpPortable(p,"Max-Age=",8)){
        const char *age=p+8;if(*age=='0'||*age=='-')remove=true;
      }
      p=next;
    }
    if(secure&&!https)return false;
    if(!strncmp(name,"__Secure-",9)&&(!secure||!https))return false;
    if(!strncmp(name,"__Host-",7)&&(!secure||!https||strcmp(path,"/")||domainSpecified))
      return false;
    int slot=-1;
    for(int i=0;i<count;++i)
      if(!strcmp(items[i].host,host)&&!strcmp(items[i].path,path)
         &&!strcmp(items[i].name,name)){slot=i;break;}
    if(remove){
      if(slot>=0){for(int i=slot;i<count-1;++i)items[i]=items[i+1];--count;}
      return true;
    }
    if(slot<0){if(count==CAP){for(int i=1;i<count;++i)items[i-1]=items[i];--count;}slot=count++;}
    Entry &e=items[slot];copy(e.host,sizeof(e.host),host);
    copy(e.path,sizeof(e.path),path);copy(e.name,sizeof(e.name),name);
    copy(e.value,sizeof(e.value),value);e.secure=secure;
    return true;
  }
  bool requestHeader(const char *url,char *out,size_t cap) const {
    if(!out||!cap)return false;
    out[0]=0;
    char host[96],path[192];bool https=false;
    if(!url||!urlParts(url,host,sizeof(host),path,sizeof(path),https))return false;
    size_t pos=0;
    for(int i=0;i<count;++i) {
      const Entry &e=items[i];
      if(strcmp(e.host,host)||(e.secure&&!https))continue;
      size_t n=strlen(e.path);
      if(strncmp(path,e.path,n) || (n>1&&path[n]&&path[n]!='/'))continue;
      int wrote=snprintf(out+pos,cap-pos,"%s%s=%s",pos?"; ":"",e.name,e.value);
      if(wrote<0||(size_t)wrote>=cap-pos){out[0]=0;return false;}
      pos+=(size_t)wrote;
    }
    return pos>0;
  }
  // Line format C1\nS/TAB/host/TAB/path/TAB/name/TAB/value. Domain is
  // validated against a synthetic same-host URL on re-import.
  size_t serialize(char *out,size_t cap) const {
    if(!out||cap<4)return 0;
    size_t used=0;int n=snprintf(out,cap,"C1\n");if(n<0||(size_t)n>=cap)return 0;used=n;
    for(int i=0;i<count;++i) {
      const Entry &e=items[i];
      n=snprintf(out+used,cap-used,"%c\t%s\t%s\t%s\t%s\n",
                 e.secure?'S':'P',e.host,e.path,e.name,e.value);
      if(n<0||(size_t)n>=cap-used)return 0;
      used+=(size_t)n;
    }
    return used;
  }
  bool deserialize(char *data) {
    clear();if(!data||strncmp(data,"C1\n",3))return false;
    char *p=data+3;
    while(*p) {
      char *nl=strchr(p,'\n');if(!nl)return false;*nl=0;
      if(*p) {
        char *parts[5]={p};int n=1;
        for(char *q=p;*q;++q)if(*q=='\t'&&n<5){*q=0;parts[n++]=q+1;}
        if(n!=5||(strcmp(parts[0],"S")&&strcmp(parts[0],"P")))return false;
        char url[300],header[280];
        int un=snprintf(url,sizeof(url),"%s://%s%s",*parts[0]=='S'?"https":"http",
                        parts[1],parts[2]);
        int hn=snprintf(header,sizeof(header),"%s=%s; Path=%s%s",
                        parts[3],parts[4],parts[2],*parts[0]=='S'?"; Secure":"");
        if(un<=0||hn<=0||(size_t)un>=sizeof(url)||(size_t)hn>=sizeof(header)
           ||!ingest(url,header))return false;
      }
      p=nl+1;
    }
    return true;
  }
private:
  Entry items[CAP] = {};
  int count=0;
  static void copy(char *o,size_t cap,const char *v){snprintf(o,cap,"%s",v);}
  static int strncasecmpPortable(const char *a,const char *b,size_t n) {
    for(size_t i=0;i<n;++i) {
      unsigned char x=(unsigned char)tolower((unsigned char)a[i]);
      unsigned char y=(unsigned char)tolower((unsigned char)b[i]);
      if(x!=y)return x-y;
      if(!x)return 0;
    }return 0;
  }
};