#include <cassert>
#include <vector>
#include "../boards/21/MotoNav/PhotoProtocol.h"
int main() {
 assert(PhotoProtocol::obsolete(12,11));
 assert(!PhotoProtocol::obsolete(12,12));
 assert(!PhotoProtocol::obsolete(255,0));
 assert(PhotoProtocol::obsolete(0,255));
 std::vector<uint8_t> f(PhotoProtocol::Bytes,0);
 const uint8_t header[]={80,72,79,1,232,0,232,0};
 for(unsigned i=0;i<8;i++) f[i]=header[i];
 assert(PhotoProtocol::valid(f.data(),f.size()));
 assert(!PhotoProtocol::valid(f.data(),f.size()-1));
 f[5]=1; assert(!PhotoProtocol::valid(f.data(),f.size())); f[5]=0;
 f[3]=2; assert(!PhotoProtocol::valid(f.data(),f.size()));
 size_t stride=485; unsigned parts=(PhotoProtocol::Bytes+stride-1)/stride;
 for(unsigned seq=0;seq<parts;seq++) {
  size_t off=seq*stride, len=std::min(stride,PhotoProtocol::Bytes-off);
  assert(PhotoProtocol::chunk(PhotoProtocol::Bytes,parts,seq,off,len,stride));
  assert(!PhotoProtocol::chunk(PhotoProtocol::Bytes,parts,seq,off+1,len,stride));
 }
 assert(!PhotoProtocol::chunk(PhotoProtocol::Bytes,parts,parts,0,10,stride));
 assert(!PhotoProtocol::chunk(PhotoProtocol::Bytes,parts,0,0,485,0));
}
