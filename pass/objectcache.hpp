#ifndef IRHASH_OBJECTCACHE_HPP
#define IRHASH_OBJECTCACHE_HPP

#include <llvm/Passes/PassPlugin.h>

#include <string>
#include <sys/stat.h>

using namespace llvm;

struct ObjectCache {
  std::string m_cachedir;

  ObjectCache(std::string cachedir) : m_cachedir(cachedir) {}

  char *objectcopy_filename(std::string objectfile, std::string hash) {
    std::string dir(m_cachedir + "/" + hash.substr(0, 2));
    mkdir(dir.c_str(), 0755);
    std::string path(dir + "/" + hash.substr(2) + ".o");
    return strdup(path.c_str());
  }

  std::string find_object_from_hash(std::string objectfile, std::string hash) {
    std::string ObjectPath(m_cachedir + "/" + hash.substr(0, 2) + "/" + hash.substr(2) + ".o");
    struct stat dummy;
    if (stat(ObjectPath.c_str(), &dummy) == 0) {
      // Found!
      return ObjectPath;
    }
    return "";
  }
};

#endif // IRHASH_OBJECTCACHE_HPP
