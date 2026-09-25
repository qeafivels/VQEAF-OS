#include "QeappFormat.h"
#include "QeappSignature.h"
#include <fstream>
#include <cstring>
#include <vector>
#include <iostream>
static bool check(const char *path, bool expectValid, const char *expectType=nullptr) {
 std::ifstream in(path,std::ios::binary);
 if(!in) return false;
 std::vector<uint8_t> b{std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
 if(b.size()<192) return false;
 const char *reason="";
 Qeapp::Header h;
 if(!Qeapp::parseHeader(b.data(),b.size(),h,reason)) return !expectValid;
 Qeapp::Meta meta;
 if(!Qeapp::parseManifest((const char*)b.data()+116,h.manifestLen,meta,reason))return !expectValid;
 if(expectType && strcmp(meta.type,expectType))return false;
 size_t offsets[]={116,116+h.manifestLen,116+h.manifestLen+h.iconLen};
 size_t lengths[]={h.manifestLen,h.iconLen,h.payloadLen};
 const uint8_t *hashes[]={h.manifestHash,h.iconHash,h.payloadHash};
 for(int i=0;i<3;i++){
  Qeapp::Sha256 hash;uint8_t out[32];hash.update(b.data()+offsets[i],lengths[i]);hash.finish(out);
  if(!Qeapp::equalHash(out,hashes[i]))return !expectValid;
 }
 Qeapp::Sha256 full;uint8_t digest[32];full.update(b.data(),b.size()-76);full.finish(digest);
 const bool valid=Qeapp::verifySignature(digest,b.data()+b.size()-76,reason,meta.type);
 if(valid!=expectValid){std::cerr<<"verify mismatch for "<<path<<" reason="<<reason<<"\n";return false;}
 if(valid && h.iconLen==Qeapp::ICON_BYTES) {
   const uint8_t *icon=b.data()+offsets[1];
   // Exactly 2048 raw RGB565-LE pixels, not PNG or arbitrary blob.
   if(icon[0]==0x89 && icon[1]=='P')return false;
 }
 return true;
}
int main(int argc,char**argv){
 if(argc!=6){std::cerr<<"need luaOK textOK luaProd textBeta corruptIcon\n";return 2;}
 if(!check(argv[1],true,"lua"))return 11;
 if(!check(argv[2],true,"text"))return 12;
 if(!check(argv[3],false,"lua"))return 13;
 if(!check(argv[4],false,"text"))return 14;
 if(!check(argv[5],false,"lua"))return 15;
 std::puts("PASS: dual publisher: production text, beta Lua, cross-type rejection, signed RGB565 icon and corrupted icon reject");
 return 0;
}
