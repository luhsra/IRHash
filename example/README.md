# Example

This folder contains two programs (C and C++) that demonstrate IRHash.
To get started, use the provided `Dockerfile`:

```terminal
> docker build -t irhash-example -f Dockerfile ..
> docker run -it --rm irhash-example
root@4ab9c85f20c6:/example# make -s
[/tmp/edit-distance-9275c5.o] Not found in cache: 15fd2518b60c38357d44773ec4fbad2a
[/tmp/quicksort-ec240f.o] Not found in cache: b17cfa0fadd7cc5d16d906e309e3f688
root@4ab9c85f20c6:/example# vim edit-distance.cpp # try to remove the static_assert()
root@04a69b18db80:/example# make -s
[/tmp/edit-distance-572b16.o] Found in cache: 15fd2518b60c38357d44773ec4fbad2a
```

Both `vim` and `nano` are installed in the container.
