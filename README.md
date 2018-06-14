The Krun Linux Kernel
=====================

This is the landing branch for the Krun Linux Kernel.

The Krun Linux kernel has additional system calls for low-latency access to
several MSRs which Krun uses for benchmarking. Although the existing msr (and
rmsr) modules provide access to MSRs via device nodes, they introduce too much
jitter into measurements.

For more information on Krun, see:
http://soft-dev.org/src/krun/

Binary Releases
---------------

Binary releases of the Krun Linux Kernel can be found
[here](https://archive.org/details/krun_linux_kernel_binaries).

Branches
--------

The Krun Linux Kernel functionality is periodically ported to newer kernel
versions. The following versions are currently available:


Version   | Branch
--------- | ------------------
3.16.36   | linux-3.16.36-krun
4.9.0     | linux-4.9-krun
4.9.30    | linux-4.9.30-krun
4.9.88    | linux-4.9.88-krun


Individual branches contain instructions on how to build from source.
