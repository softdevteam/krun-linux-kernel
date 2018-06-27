# The Krun Linux Kernel

This branch houses the customised Linux-4.9.88 Kernel for use with the
[Krun Benchmarking System](https://github.com/softdevteam/krun).

This kernel has additional system calls for low-latency access to several MSRs
which Krun uses for benchmarking. Although the existing `msr` (and `rmsr`)
modules provide access to MSRs via device nodes, they introduce too much jitter
into measurements.

## Installation

These instructions assume a Debian Linux system.

Binary packages of the Krun Linux kernel can be found
[here](https://archive.org/download/krun_linux_kernel_binaries).

### Step 1: Make Sure You Are on the Right Branch

Skip this step if you are installing a binary package.

This branch is for kernel version 4.9.88.

If this is not the version you want, then switch branch. To see which Kernel
versions we have patched, run:

```
$ git branch -r | grep krun
```

Our branches are named `linux-\*-krun`.

### Step 2: Configure and Build the Kernel

Skip this step if you are installing a binary package.

Krun requires that the kernel is running in "full tickless mode", thus minimising
regular tick interrupts where possible (for all but the boot CPU core).

 * Whilst running the stock Debian kernel (the configuration from the new
   kernel is inherited from the running kernel), run `make menuconfig` in the
   top-level directory of this repo.

 * Go into `General setup->Timers subsystem->Timer tick handling` and choose
   `Full dynticks system (tickless)` (internally known as `CONFIG_NO_HZ_FULL`).

 * Then go up one level and select `Full dynticks system on all CPUs by default
   (except CPU 0)` (internally `NO_HZ_FULL_ALL`).

 * Go back to the top-level menu and find `General setup->Local Version` and
   type in a meaningful name. This name will help you identify if the system is
   running your kernel (via `uname -a`). Be careful not to use exotic symbols
   in this field. Dots work, but underscores and hyphens confuse Debian's
   package creation tools. When creating our binary packages, we use
   `krun.<first 6 chars of git hash>`.

 * Save and exit.

 * Run `make bindeb-pkg`

Now you will have `.deb` packages in the parent directory.

## Step 3: Install and Boot the New Kernel

Next:

 * Install the `linux-image` deb with `dpkg -i` (the exact filename will vary).

 * Copy `krun-syscall.h` (found in the `krun` directory if you are building
   from source) into `/usr/include/x86_64-linux-gnu/sys/krun-syscall.h` (Sorry
   about this, we tried to have the header included in a libc-dev package, but
   sadly doing so upsets `apt-get` after such a package is installed).

 * Reboot the system and check for `krun` in the kernel identity with `uname
   -r`. Debian should have set the kernel as the default. If not, you need to
   edit `/etc/default/grub` and run `update-grub` before rebooting again.

## Step 4: Check the Kernel Over

Now you are running the Krun kernel. Let's check it all looks OK:

 * Check the kernel is fully tickless with ``grep NO_HZ_FULL_ALL
   /boot/config-`uname -r` ``. You should see `CONFIG_NO_HZ_FULL_ALL=y` in the
   output.

 * You can check the syscalls work with `cd krun && make && ./test_prog` (in the
   top-level of the source this repo). This runs a program which resets the MSRs and
   prints them every so often. The counters should increase monotonically
   (unless you run long enough to see an overflow).

## Further Reading

For more information on tickless mode, see
[the kernel docs](https://www.kernel.org/doc/Documentation/timers/NO_HZ.txt).

For more information about `make deb-pkg`, see
[the Debian handbook](https://debian-handbook.info/browse/stable/sect.kernel-compilation.html).

## License

See the COPYING file for licensing information.
