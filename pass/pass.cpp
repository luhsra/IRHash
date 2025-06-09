#include "pass.hpp"
#include "SlotTracker.hpp"

#ifdef WITH_CLANG_PLUGIN
#include "plugin.hpp"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>
#endif

#include <llvm/Config/llvm-config.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

#include <fstream> // IWYU pragma: keep
#include <unistd.h>
#include <utime.h>

using namespace llvm;

#include "objectcache.hpp"

static enum {
  ATEXIT_NOP,
  ATEXIT_FROM_CACHE,
  ATEXIT_TO_CACHE,
} atexit_mode = ATEXIT_NOP;

static char const *objectfile;
static char const *objectfile_copy;

/// This is the main entry point for the IRHash pass.
PreservedAnalyses IRHashPass::run(Module &M, ModuleAnalysisManager &AM) {
  this->M = &M;
  this->MST = new ModuleSlotTracker(&M, true);
  this->SlotTable = MST->getMachine();

  Hasher &hash = this->ModuleHash;

  // The actual hashing of the module

  hash.update(M.getModuleInlineAsm());

  hash.update(M.getTargetTriple());

  for (const StructType *T : M.getIdentifiedStructTypes()) {
    hash.update(T->isLiteral());
    hash.update(T->isOpaque());
    hash.update(T->isPacked());

    for (Type *Ty : T->elements()) {
      hashType(Ty, hash);
    }
  }

  for (const Function &F : M.functions()) {
    IRHashPass::hashFunction(F, hash);
  }

  for (const GlobalVariable &GV : M.globals()) {
    IRHashPass::hashGlobalVariable(GV, hash);
  }

  Hasher::Digest digest;
  this->ModuleHash.final(digest);

  auto hash_str = digest.digest();

  const std::string out_file = getOutFile();

#ifdef DEBUG
  std::string str;
  raw_string_ostream retsstream(str);
  retsstream << M;
  std::ofstream ll(out_file + '.' + this->pass + ".llvmll");
  ll << retsstream.str() << '\n';
  ll.close();
#endif

  const char *cachedir = getenv("IRHASH_CACHE");
  if (!cachedir) {
    llvm::report_fatal_error("IRHASH_CACHE not set");
  }
  ObjectCache cache(cachedir);

  objectfile = strdup(out_file.c_str());

  std::string copy = cache.find_object_from_hash(out_file, hash_str.c_str());
  atexit(link_object_file);
  if (copy != "") { // hash is known
#ifdef DEBUG_LOGGING
    errs() << '[' << out_file << "] Found in cache: " << hash_str << '\n';
#endif

    atexit_mode = ATEXIT_FROM_CACHE;
    objectfile_copy = strdup(copy.c_str());
#ifdef WITH_CLANG_PLUGIN
    CLANG_CI->getPreprocessor().EndSourceFile();
#endif
    exit(0);
  } else {
#ifdef DEBUG_LOGGING
    errs() << '[' << out_file << "] Not found in cache: " << hash_str << '\n';
#endif

    atexit_mode = ATEXIT_TO_CACHE;
    objectfile_copy = cache.objectcopy_filename(objectfile, hash_str.c_str());
    // continue compilation
  }

  return PreservedAnalyses::all();
}

void IRHashPass::hashGlobalVariable(const GlobalVariable &GV, Hasher &hash) {
  hash.update(GV.getName());

  hash.update(GV.getLinkage());

  if (MaybeAlign A = GV.getAlign()) {
    hash.update(A->value());
  }

  const bool hasInit = GV.hasInitializer();
  hash.update(hasInit);
  if (hasInit) {
    const Constant *CV = static_cast<const Constant *>(GV.getInitializer());
    hashValue(CV, hash);
  }

  hashType(GV.getValueType(), hash);
  hash.update(GV.getThreadLocalMode());
  hash.update((int)GV.getUnnamedAddr());

  for (auto attr : GV.getAttributes()) {
    hash.update(attr.getKindAsEnum());
  }
}

void IRHashPass::hashFunction(const Function &F, Hasher &hash) {
  SlotTable->incorporateFunction(&F);

  hash.update(F.getName());
  hash.update(F.isDeclaration());
  hash.update(F.getLinkage());
  hash.update(F.getVisibility());
  hash.update(F.getCallingConv());
  if (MaybeAlign A = F.getAlign()) {
    hash.update(A->value());
  }

  hashType(F.getReturnType(), hash);
  for (Type *Ty : F.getFunctionType()->params()) {
    hashType(Ty, hash);
  }
  hash.update(F.isVarArg());

  for (const AttributeSet attrSet : F.getAttributes()) {
    for (const Attribute attr : attrSet) {
      hash.update(attr.getKindAsEnum());
    }
  }

  for (const BasicBlock &BB : F) {
    if (BB.hasName()) {
      hash.update(BB.getName());
    }
    for (const Instruction &I : BB) {
      hash.update(I.getOpcode());

      if (I.hasName()) {
        hash.update(I.getName());
      } else if (!I.getType()->isVoidTy()) {
        hash.update(SlotTable->getLocalSlot(&I));
      }

      if (const CallInst *CI = dyn_cast<CallInst>(&I)) {
        hash.update(CI->getTailCallKind());
      } else if (const CmpInst *CI = dyn_cast<CmpInst>(&I)) {
        hash.update(CI->getPredicate());
      } else if (const AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
        hashType(AI->getAllocatedType(), hash);
        hashType(AI->getArraySize()->getType(), hash);
        hash.update(AI->getAlign().value());
      } else {
        hash.update((isa<LoadInst>(I) && cast<LoadInst>(I).isAtomic()) ||
                    (isa<StoreInst>(I) && cast<StoreInst>(I).isAtomic()));

        hash.update(isa<AtomicCmpXchgInst>(I) && cast<AtomicCmpXchgInst>(I).isWeak());

        hash.update((isa<LoadInst>(I) && cast<LoadInst>(I).isVolatile()) ||
                    (isa<StoreInst>(I) && cast<StoreInst>(I).isVolatile()) ||
                    (isa<AtomicCmpXchgInst>(I) && cast<AtomicCmpXchgInst>(I).isVolatile()) ||
                    (isa<AtomicRMWInst>(I) && cast<AtomicRMWInst>(I).isVolatile()));
      }

      for (unsigned i = 0, E = I.getNumOperands(); i != E; ++i) {
        const Value *op = I.getOperand(i);
        if (const Instruction *II = dyn_cast<Instruction>(op)) {
          if (II->hasName()) {
            hash.update(II->getName());
          } else if (!II->getType()->isVoidTy()) {
            // TODO !isVoidTy should be unneccesary - we're using the instruction's result as operand
            hash.update(SlotTable->getLocalSlot(II));
          }
        } else if (const Constant *C = dyn_cast<Constant>(op)) {
          const GlobalVariable *GV = dyn_cast<GlobalVariable>(C);
          if (GV != nullptr && C->hasName()) {
            // for global vars, we only need to hash their reference
            hash.update(GV->getName());
          } else {
            hashValue(C, hash);
          }
        } else if (const BasicBlock *BB = dyn_cast<BasicBlock>(op)) {
          hash.update(SlotTable->getLocalSlot(BB));
        } else if (const Argument *Arg = dyn_cast<Argument>(op)) {
          hashType(Arg->getType(), hash);
          if (Arg->hasName()) {
            hash.update(Arg->getName());
          } else {
            hash.update(SlotTable->getLocalSlot(Arg));
          }
        } else if (const InlineAsm *IA = dyn_cast<InlineAsm>(op)) {
          hash.update(IA->canThrow());
          hash.update(IA->isAlignStack());
          hash.update(IA->getAsmString());
          hash.update(IA->getConstraintString());
        } else if (isa<MetadataAsValue>(op)) {
          // Ignore for now
          break;
        } else {
          errs() << I << '\n';
          errs() << *op << '\n';
          llvm_unreachable("Unhandled Instruction");
        }
      }
    }
  }
}

void IRHashPass::hashValue(const Constant *CV, Hasher &hash) {
  if (CV->hasName()) {
    hash.update(CV->getName());
  }

  if (const ConstantInt *CI = dyn_cast<ConstantInt>(CV)) {
    const APInt I = CI->getValue();
    hash.update((void *)I.getRawData(), sizeof(uint64_t) * I.getNumWords());
    return;
  }

  if (const ConstantFP *CFP = dyn_cast<ConstantFP>(CV)) {
    const APInt I = CFP->getValueAPF().bitcastToAPInt();
    hash.update((void *)I.getRawData(), sizeof(uint64_t) * I.getNumWords());
    return;
  }

  // Array, Struct, Vector
  if (const ConstantAggregate *CA = dyn_cast<ConstantAggregate>(CV)) {
    const unsigned N = CA->getNumOperands();
    for (unsigned i = 0; i < N; i++) {
      hashValue(CA->getOperand(i), hash);
    }
    return;
  }

  // constant data arrays/vectors
  if (const ConstantDataSequential *CA = dyn_cast<ConstantDataSequential>(CV)) {
    for (unsigned i = 0, e = CA->getNumElements(); i != e; ++i) {
      hashValue(CA->getElementAsConstant(i), hash);
    }
    return;
  }

  // something Null-ish
  if (isa<ConstantAggregateZero>(CV) || isa<ConstantTokenNone>(CV) || isa<ConstantAggregateZero>(CV) ||
      isa<ConstantTokenNone>(CV) || isa<ConstantPointerNull>(CV) || isa<ConstantTokenNone>(CV) ||
      isa<PoisonValue>(CV) || isa<UndefValue>(CV)) {
    hash.update(isa<ConstantAggregateZero>(CV));
    hash.update(isa<ConstantTargetNone>(CV));
    hash.update(isa<ConstantPointerNull>(CV));
    hash.update(isa<ConstantTokenNone>(CV));
    hash.update(isa<PoisonValue>(CV));
    hash.update(isa<UndefValue>(CV));
    hash.update(isa<GlobalVariable>(CV));
    return;
  }

  if (const GlobalVariable *GV = dyn_cast<GlobalVariable>(CV)) {
    hash.update(GV->hasInitializer());
    if (GV->hasInitializer()) {
      // hashValue(GV->getInitializer(), hash);
      hash.update(GV->getInitializer()->getName());
    }
    return;
  }

  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(CV)) {
    hash.update(CE->getOpcode());

    for (User::const_op_iterator OI = CE->op_begin(); OI != CE->op_end(); ++OI) {
      hash.update((*OI)->getName());
    }
    return;
  }

  if (const Function *F = dyn_cast<Function>(CV)) {
    hash.update(F->getName());
    return;
  }

  if (const BlockAddress *BA = dyn_cast<BlockAddress>(CV)) {
    hash.update(BA->getFunction()->getName());
    return;
  }

  if (const GlobalValue *GV = dyn_cast<GlobalValue>(CV)) {
    hash.update(GV->getName());
    return;
  }

  errs() << CV << '\n';
  errs() << *CV << '\n';
  errs() << isa<ConstantAggregate>(CV) << '\n';
  errs() << isa<ConstantExpr>(CV) << '\n';
  errs() << isa<DSOLocalEquivalent>(CV) << '\n';
  errs() << isa<GlobalValue>(CV) << '\n';
  errs() << isa<GlobalAlias>(CV) << '\n';
  errs() << isa<GlobalObject>(CV) << '\n';
  errs() << isa<Function>(CV) << '\n';
  errs() << isa<GlobalIFunc>(CV) << '\n';
  errs() << isa<GlobalVariable>(CV) << '\n';
  errs() << isa<NoCFIValue>(CV) << '\n';
  errs() << (isa<ConstantAggregateZero>(CV) || isa<ConstantTargetNone>(CV)) << '\n';

  llvm_unreachable("Unhandled Constant");
}

void IRHashPass::hashType(const Type *Ty, Hasher &hash) {
  hash.update(Ty->getTypeID());

  switch (Ty->getTypeID()) {
  case Type::VoidTyID:
    return;
  case Type::HalfTyID:
    return;
  case Type::BFloatTyID:
    return;
  case Type::FloatTyID:
    return;
  case Type::DoubleTyID:
    return;
  case Type::X86_FP80TyID:
    return;
  case Type::FP128TyID:
    return;
  case Type::PPC_FP128TyID:
    return;
  case Type::LabelTyID:
    return;
  case Type::MetadataTyID:
    return;
  case Type::X86_MMXTyID:
    return;
  case Type::X86_AMXTyID:
    return;
  case Type::TokenTyID:
    return;
  case Type::IntegerTyID:
    hash.update(cast<IntegerType>(Ty)->getBitWidth());
    return;

  case Type::FunctionTyID: {
    const FunctionType *FTy = cast<FunctionType>(Ty);
    hashType(FTy->getReturnType(), hash);
    for (Type *Ty : FTy->params()) {
      hashType(Ty, hash);
    }
    hash.update(FTy->isVarArg());
    return;
  }
  case Type::StructTyID: {
    const StructType *STy = cast<StructType>(Ty);

    const StringRef name = STy->getName();
    if (!name.empty()) {
      // hash globaly once and just hash the name afterwards
      hash.update(name);
      return;
    }

    hash.update(STy->isLiteral());
    hash.update(STy->isOpaque());
    hash.update(STy->isPacked());

    for (Type *Ty : STy->elements()) {
      hashType(Ty, hash);
    }

    return;
  }
  case Type::PointerTyID: {
    const PointerType *PTy = cast<PointerType>(Ty);
#if LLVM_VERSION_MAJOR >= 17
    hash.update(PTy->getAddressSpace());
#else
    hash.update(PTy->isOpaque());
    if (PTy->isOpaque()) {
      return;
    }
    hashType(PTy->getNonOpaquePointerElementType(), hash);
#endif
    return;
  }
  case Type::ArrayTyID: {
    const ArrayType *ATy = cast<ArrayType>(Ty);
    hash.update(ATy->getNumElements());
    hashType(ATy->getElementType(), hash);
    return;
  }
  case Type::FixedVectorTyID:
  case Type::ScalableVectorTyID: {
    const VectorType *PTy = cast<VectorType>(Ty);
    ElementCount EC = PTy->getElementCount();
    hash.update(EC.isScalable());
    hash.update(EC.getKnownMinValue());
    hashType(PTy->getElementType(), hash);
    return;
  }
  case Type::TypedPointerTyID: {
    llvm_unreachable("Invalid TypeID??");
    // TypedPointerType *TPTy = cast<TypedPointerType>(Ty);
    // hashType(*TPTy->getElementType(), hash);
    // return;
  }
  case Type::TargetExtTyID:
    const TargetExtType *TETy = cast<TargetExtType>(Ty);
    hash.update(Ty->getTargetExtName());
    for (Type *Inner : TETy->type_params())
      hashType(Inner, hash);
    for (unsigned IntParam : TETy->int_params())
      hash.update(IntParam);
    return;
  }

  llvm_unreachable("Invalid TypeID");
}

bool IRHashPass::isStatic(const GlobalValue *GV) {
  // Check if the linkage type of the function is internal or private.
  const GlobalValue::LinkageTypes linkage = GV->getLinkage();
  return linkage == GlobalValue::InternalLinkage || linkage == GlobalValue::PrivateLinkage;
}

std::string IRHashPass::getOutFile() {
#ifdef WITH_CLANG_PLUGIN
  return CLANG_CI->getFrontendOpts().OutputFile;
#else
  // get Outfile
  std::string OutputFile;
  std::ifstream CommandLine{"/proc/self/cmdline"};
  if (CommandLine.good()) {
    std::string Arg;
    do {
      getline(CommandLine, Arg, '\0');
      if ("-o" == Arg) {
        getline(CommandLine, OutputFile, '\0');
        break;
      }
    } while (Arg.size());
  }
  return OutputFile;
#endif
}

void IRHashPass::link_object_file() {
  assert(atexit_mode != ATEXIT_NOP);

  const char *src, *dst;

  if (atexit_mode == ATEXIT_FROM_CACHE) {
    src = objectfile_copy;
    dst = objectfile;
  } else {
    src = objectfile;
    dst = objectfile_copy;
  }

  /* If destination exists, we have to unlink it. */
  struct stat dummy;
  if (stat(dst, &dummy) == 0) { // exists
    if (unlink(dst) != 0) {     // unlink failed
      errs() << "dst=" << dst << '\n';
      perror("irhash: unlink objectfile/objectfile copy");
      return;
    }
  }
  if (stat(src, &dummy) != 0) { // src exists
    errs() << "src=" << src << '\n';
    perror("irhash: source objectfile/objectfile copy does not exist");
    return;
  }

  // Copy by hardlink
  if (link(src, dst) != 0) {
    errs() << "src=" << src << " dst=" << dst << '\n';
    perror("irhash: objectfile update failed");
    return;
  }

  // Write Hash to hashfile
  if (atexit_mode == ATEXIT_FROM_CACHE) {
    // Update Timestamp
    utime(dst, NULL);
  }
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize IRHash when added to the pass pipeline on the
// command line, i.e. via '-passes=irhash'
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "IRHash", LLVM_VERSION_STRING, [](PassBuilder &PB) {
  // errs() << "Pipeline: " << Pipeline << '\n';
  // https://github.com/Jakob-Koschel/llvm-passes#new-pass-manager

#if PIPELINE == 0
            PB.registerPipelineStartEPCallback( // adding optimization once at the start of the pipeline
                [&](ModulePassManager &MPM, OptimizationLevel Level) { MPM.addPass(IRHashPass("0")); });
#elif PIPELINE == 1
            PB.registerOptimizerLastEPCallback(
                [&](ModulePassManager &MPM, OptimizationLevel Level) { MPM.addPass(IRHashPass("1")); });
#else
#error "PIPELINE not defined"
#endif

            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "irhash") {
                    MPM.addPass(IRHashPass("0"));
                    return true;
                  }
                  return false;
                });
          }};
}
