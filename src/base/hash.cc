#include <base/hash.h>

#include <histogram/counter.h>

#include <third_party/smhasher/exported/MurmurHash3.h>

namespace dist_clang {
namespace base {

String MakeHash(const String& input) {
  histogram::Counter counter("hash", input.size());

  char buf[16];
  MurmurHash3_x64_128(input.data(), input.size(), 0, buf);
  return String(buf, 16);
}

}  // namespace base
}  // namespace dist_clang