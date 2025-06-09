#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {

constexpr size_t min(auto... values) {
  size_t result = std::numeric_limits<size_t>::max();
  ((result = (values < result) ? values : result), ...);
  return result;
}

constexpr size_t edit_distance(std::string_view source,
                               std::string_view target) {
  std::vector<size_t> dists(target.size() + 1);
  for (size_t i = 0; i <= target.size(); ++i) {
    dists[i] = i;
  }

  for (size_t i = 1; i <= source.size(); ++i) {
    size_t prev = dists[0];
    dists[0] = i;
    for (size_t j = 1; j <= target.size(); ++j) {
      size_t temp = dists[j];

      bool same = source[i - 1] == target[j - 1];
      dists[j] = min(dists[j] + 1,         // deletion
                     dists[j - 1] + 1,     // insertion
                     prev + (same ? 0 : 2) // substitution
      );
      prev = temp;
    }
  }

  return dists[target.size()];
}

// try to delete this and recompile
static_assert(edit_distance("kitten", "sitting") == 5);

} // namespace

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <source> <target>\n";
    return 1;
  }

  size_t distance = edit_distance(argv[1], argv[2]);
  std::cout << distance << '\n';

  return 0;
}
