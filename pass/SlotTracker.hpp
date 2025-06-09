#ifndef IRHASH_SLOTTRACKER_HPP
#define IRHASH_SLOTTRACKER_HPP

// Excerpt from the LLVM source code (llvm/include/llvm/Transforms/Utils/SlotTracker.hpp)

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ModuleSlotTracker.h>

using namespace llvm;

namespace llvm {

//===----------------------------------------------------------------------===//
// SlotTracker Class: Enumerate slot numbers for unnamed values
//===----------------------------------------------------------------------===//
/// This class provides computation of slot numbers for LLVM Assembly writing.
///
class SlotTracker : public AbstractSlotTrackerStorage {
public:
  /// ValueMap - A mapping of Values to slot numbers.
  using ValueMap = DenseMap<const Value *, unsigned>;

private:
  /// TheModule - The module for which we are holding slot numbers.
  const Module *TheModule;

  /// TheFunction - The function for which we are holding slot numbers.
  const Function *TheFunction = nullptr;
  bool FunctionProcessed = false;
  bool ShouldInitializeAllMetadata;

  std::function<void(AbstractSlotTrackerStorage *, const Module *, bool)> ProcessModuleHookFn;
  std::function<void(AbstractSlotTrackerStorage *, const Function *, bool)> ProcessFunctionHookFn;

  /// The summary index for which we are holding slot numbers.
  const ModuleSummaryIndex *TheIndex = nullptr;

  /// mMap - The slot map for the module level data.
  ValueMap mMap;
  unsigned mNext = 0;

  /// fMap - The slot map for the function level data.
  ValueMap fMap;
  unsigned fNext = 0;

  /// mdnMap - Map for MDNodes.
  DenseMap<const MDNode *, unsigned> mdnMap;
  unsigned mdnNext = 0;

  /// asMap - The slot map for attribute sets.
  DenseMap<AttributeSet, unsigned> asMap;
  unsigned asNext = 0;

  /// ModulePathMap - The slot map for Module paths used in the summary index.
  StringMap<unsigned> ModulePathMap;
  unsigned ModulePathNext = 0;

  /// GUIDMap - The slot map for GUIDs used in the summary index.
  DenseMap<GlobalValue::GUID, unsigned> GUIDMap;
  unsigned GUIDNext = 0;

  /// TypeIdMap - The slot map for type ids used in the summary index.
  StringMap<unsigned> TypeIdMap;
  unsigned TypeIdNext = 0;

public:
  /// Construct from a module.
  ///
  /// If \c ShouldInitializeAllMetadata, initializes all metadata in all
  /// functions, giving correct numbering for metadata referenced only from
  /// within a function (even if no functions have been initialized).
  explicit SlotTracker(const Module *M, bool ShouldInitializeAllMetadata = false);

  /// Construct from a function, starting out in incorp state.
  ///
  /// If \c ShouldInitializeAllMetadata, initializes all metadata in all
  /// functions, giving correct numbering for metadata referenced only from
  /// within a function (even if no functions have been initialized).
  explicit SlotTracker(const Function *F, bool ShouldInitializeAllMetadata = false);

  /// Construct from a module summary index.
  explicit SlotTracker(const ModuleSummaryIndex *Index);

  SlotTracker(const SlotTracker &) = delete;
  SlotTracker &operator=(const SlotTracker &) = delete;

  ~SlotTracker() = default;

  void setProcessHook(std::function<void(AbstractSlotTrackerStorage *, const Module *, bool)>);
  void setProcessHook(std::function<void(AbstractSlotTrackerStorage *, const Function *, bool)>);

  unsigned getNextMetadataSlot() override { return mdnNext; }

  void createMetadataSlot(const MDNode *N) override;

  /// Return the slot number of the specified value in it's type
  /// plane.  If something is not in the SlotTracker, return -1.
  int getLocalSlot(const Value *V);
  int getGlobalSlot(const GlobalValue *V);
  int getMetadataSlot(const MDNode *N) override;
  int getAttributeGroupSlot(AttributeSet AS);
  int getModulePathSlot(StringRef Path);
  int getGUIDSlot(GlobalValue::GUID GUID);
  int getTypeIdSlot(StringRef Id);

  /// If you'd like to deal with a function instead of just a module, use
  /// this method to get its data into the SlotTracker.
  void incorporateFunction(const Function *F) {
    TheFunction = F;
    FunctionProcessed = false;
  }

  const Function *getFunction() const { return TheFunction; }

  /// After calling incorporateFunction, use this method to remove the
  /// most recently incorporated function from the SlotTracker. This
  /// will reset the state of the machine back to just the module contents.
  void purgeFunction();

  /// MDNode map iterators.
  using mdn_iterator = DenseMap<const MDNode *, unsigned>::iterator;

  mdn_iterator mdn_begin() { return mdnMap.begin(); }
  mdn_iterator mdn_end() { return mdnMap.end(); }
  unsigned mdn_size() const { return mdnMap.size(); }
  bool mdn_empty() const { return mdnMap.empty(); }

  /// AttributeSet map iterators.
  using as_iterator = DenseMap<AttributeSet, unsigned>::iterator;

  as_iterator as_begin() { return asMap.begin(); }
  as_iterator as_end() { return asMap.end(); }
  unsigned as_size() const { return asMap.size(); }
  bool as_empty() const { return asMap.empty(); }

  /// GUID map iterators.
  using guid_iterator = DenseMap<GlobalValue::GUID, unsigned>::iterator;

  /// These functions do the actual initialization.
  inline void initializeIfNeeded();
  int initializeIndexIfNeeded();

  // Implementation Details
private:
  /// CreateModuleSlot - Insert the specified GlobalValue* into the slot table.
  void CreateModuleSlot(const GlobalValue *V);

  /// CreateMetadataSlot - Insert the specified MDNode* into the slot table.
  void CreateMetadataSlot(const MDNode *N);

  /// CreateFunctionSlot - Insert the specified Value* into the slot table.
  void CreateFunctionSlot(const Value *V);

  /// Insert the specified AttributeSet into the slot table.
  void CreateAttributeSetSlot(AttributeSet AS);

  inline void CreateModulePathSlot(StringRef Path);
  void CreateGUIDSlot(GlobalValue::GUID GUID);
  void CreateTypeIdSlot(StringRef Id);

  /// Add all of the module level global variables (and their initializers)
  /// and function declarations, but not the contents of those functions.
  void processModule();
  // Returns number of allocated slots
  int processIndex();

  /// Add all of the functions arguments, basic blocks, and instructions.
  void processFunction();

  /// Add the metadata directly attached to a GlobalObject.
  void processGlobalObjectMetadata(const GlobalObject &GO);

  /// Add all of the metadata from a function.
  void processFunctionMetadata(const Function &F);

  /// Add all of the metadata from an instruction.
  void processInstructionMetadata(const Instruction &I);
};

} // end namespace llvm

#endif // IRHASH_SLOTTRACKER_HPP
