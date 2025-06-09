#ifndef IRHASH_PASS_H
#define IRHASH_PASS_H

#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/IR/PassManager.h>

#include "hash.hpp"

namespace llvm {

struct Symbol {
  StringRef kind;
  StringRef name;
};

inline bool operator<(const Symbol &lhs, const Symbol &rhs) { return lhs.name < rhs.name || lhs.kind < rhs.kind; }

/// The main IRHash pass.
class IRHashPass : public PassInfoMixin<IRHashPass> {
private:
  Module *M = nullptr;
  ModuleSlotTracker *MST = nullptr;
  SlotTracker *SlotTable = nullptr;
  Hasher ModuleHash;
  const char *pass; // pass name

  static bool isStatic(const GlobalValue *GV);
  static void hashType(const Type *T, Hasher &hash);
  static void hashValue(const Constant *CV, Hasher &hash);
  static void hashGlobalVariable(const GlobalVariable &GV, Hasher &hash);
  void hashFunction(const Function &F, Hasher &hash);

  static std::string getOutFile();
  static void link_object_file();

public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  IRHashPass(const char *pass) : pass(pass) {}
  IRHashPass(IRHashPass &&other) : ModuleHash(std::move(other.ModuleHash)) {
    MST = other.MST;
    other.MST = nullptr;

    M = other.M;
    other.M = nullptr;

    SlotTable = other.SlotTable;
    other.SlotTable = nullptr;

    pass = other.pass;
  }
  ~IRHashPass() {
    if (MST) {
      free(MST);
    }
  }

  static bool isRequired() { return true; }
  void setPass(const char *pass) { this->pass = pass; }
};
} // namespace llvm

#endif // IRHASH_PASS_H
