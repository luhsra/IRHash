#ifndef IRHASH_PLUGIN_HPP
#define IRHASH_PLUGIN_HPP

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendPluginRegistry.h>

using namespace clang;

static const CompilerInstance *CLANG_CI = nullptr;

// The clang plugin.
// It's only used to get to the CompilerInstance which is static.
class PassCIAction : public PluginASTAction {

protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
    return std::make_unique<ASTConsumer>();
  }

  bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &args) override {
    CLANG_CI = &CI;
    return true;
  }

  PluginASTAction::ActionType getActionType() override { return AddBeforeMainAction; }
};

static FrontendPluginRegistry::Add<PassCIAction> X("irhash-clang",
                                                   "Clang plugin to get the CompilerInstance in the IRHash pass.");

#endif // IRHASH_PLUGIN_HPP
