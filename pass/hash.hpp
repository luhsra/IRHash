#ifndef IRHASH_HASH_HPP
#define IRHASH_HASH_HPP

#include "xxhash.h"

#include <iomanip>

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/GlobalObject.h>
#include <llvm/Support/raw_ostream.h>

namespace llvm {
struct Hasher {
  XXH3_state_t *state;

  Hasher(Hasher &&other) {
    state = other.state;
    other.state = nullptr;
  }

  Hasher() {
    state = XXH3_createState();
    XXH3_128bits_reset(state);
  }

  Hasher(const Hasher &) = delete;
  Hasher &operator=(const Hasher &) = delete;

  ~Hasher() {
    if (state != nullptr) {
      XXH3_freeState(state);
      state = nullptr;
    }
  }

  struct Digest {
    XXH128_hash_t hash;

    SmallString<32> digest() const {
      std::stringstream retsstream;
      retsstream << std::hex << std::setw(16) << std::setfill('0') << hash.high64 << std::setw(16) << std::setfill('0')
                 << hash.low64;
      return SmallString<32>(retsstream.str());
    }
  };

  /// Add the bytes in the StringRef \p Str to the hash.
  // Note that this isn't a string and so this won't include any trailing NULL
  // bytes.
  void update(StringRef Str) { XXH3_128bits_update(state, Str.data(), Str.size()); }

  void update(uint64_t Data) { XXH3_128bits_update(state, (const void *)&Data, sizeof(Data)); }

  void update(void *Data, size_t Len) { XXH3_128bits_update(state, Data, Len); }

  void update(const GlobalObject &GO) {
    std::string str;
    raw_string_ostream retsstream(str);
    retsstream << GO;
    update(retsstream.str());
  }

  void final(Digest &Ret) const {
    Ret.hash = XXH3_128bits_digest(state);
    // outs() << "final state: " << state << "\n";
  }

  static Digest hash(const GlobalObject &GO) {
    Hasher hash;
    hash.update(GO);

    Digest digest;
    hash.final(digest);
    return digest;
  }

  static Digest hash(StringRef str) {
    Hasher hash;
    hash.update(str);

    Digest digest;
    hash.final(digest);
    return digest;
  }
};

} // namespace llvm

#endif // IRHASH_HASH_HPP
