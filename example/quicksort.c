#include <stdio.h>
#include <string.h>

/// Swap two elements in an array.
///
/// \param a The first element.
/// \param b The second element.
static void swap(char **a, char **b) {
  char *temp = *a;
  *a = *b;
  *b = temp;
}

/// Partition the array into two parts, one with elements less than a pivot
/// element and the other with elements greater than the pivot.
///
/// \param arr The array to partition.
/// \param low The lower bound of @c arr.
/// \param high The upper bound of @c arr.
/// \return The index of the pivot element.
static int partition(char **arr, int low, int high) {
  char *pivot = arr[high];

  int i = low - 1;
  for (int j = low; j < high; j++) {
    if (strcmp(arr[j], pivot) < 0) {
      i++;
      swap(&arr[i], &arr[j]);
    }
  }
  swap(&arr[i + 1], &arr[high]);
  return i + 1;
}

/// Sort an array of strings using the quicksort algorithm.
///
/// \param arr The array to sort.
/// \param low The lower bound of the array.
/// \param high The upper bound of the array.
static void quicksort(char **arr, int low, int high) {
  if (low < high) {
    int idx = partition(arr, low, high);
    quicksort(arr, low, idx - 1);
    quicksort(arr, idx + 1, high);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <strings to sort>\n", argv[0]);
    return 1;
  }

  quicksort(argv, 1, argc - 1);

  for (int i = 1; i < argc; i++) {
    printf("%s\n", argv[i]);
  }

  return 0;
}
