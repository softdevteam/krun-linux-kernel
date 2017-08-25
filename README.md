# The Krun Linux Kernel

This repo houses the customised Linux-3.16.3 kernel for use with the
[Krun Benchmarking System](https://github.com/softdevteam/krun).

This kernel has additional system calls for low-latency access to several MSRs
which Krun uses for benchmarking. Although the existing `msr' (and `rmsr')
modules provide access to MSRs via device nodes, they introduce too much jitter
into measurements.

## Setup and Installation

This section describes the setup of the Krun kernel, including putting the
kernel into `full tickless mode', which Krun insists upon.

### Step 1: Make Sure You Are on the Right Branch

This branch is for kernel version 3.16.3.

If this is not the version you want, then switch branch. To see which Kernel
versions we have patched, run:

```
$ git branch -r | grep krun
```

Our branches are named `linux-*-krun`.

### Step 2: Configure and Build the Kernel

Krun insists the kernel is running in "full tickless mode", thus minimising
regular tick interrupts where possible (for all but the boot CPU core).

The kernel installation process varys depending on the Linux distribution. We
assume you will be using Debian, since this is the Linux distribution Krun is
designed to run on.

On a Debian machine, the easiest way to build the kernel, and to have the
custom headers installed in the right place, is to build and installi deb
packages.

 * Whilst running the stock Debian kernel (the configuration from the new
   kernel is inherited from the running kernel), run `make menuconfig` in the
   top-level directory of this repo.

 * Go into `General setup->Timers subsystem->Timer tick handling` and choose
   `Full dynticks system (tickless)` (internally known as `CONFIG_NO_HZ_FULL`).

 * Then go up one level and select `Full dynticks system on all CPUs by default
   (except CPU 0)` (internally `NO_HZ_FULL_ALL`).

 * Go back to the top-level menu and find `General setup->Local Version` and
   type in a meaningful name. As tempting as it is, *do not* use symbols in
   this name, as it will cause the Debian package build to bomb out. This name
   will help you identify if the system is running your kernel.

 * Save and exit.

 * Run `make`

 * Run `make deb-pkg`

## Step 3: Install and Boot the New Kernel

The previous step should have created deb packages in the parent directory. Next:

 * Install them with `dpkg -i <files>`, where `<files>` are the deb packages
   you want to install. You will need to install the: `linux-image`, `linux-firmware-image`,
   `linux-headers` and `linux-libc-dev` packages (whose exact names will vary).

 * Reboot the system and check the kernel is running with `uname -r`. Debian
   should have set the kernel as the default, but sometimes (for unknown
   reasons) it doesn't. If it has not selected your kernel, you need to edit
   `/etc/default/grub` and run `update-grub` before rebooting.

## Step 3: Check the Kernel Over

Now you are running the Krun kernel. Let's check it all looks OK:

 * Check the kernel is fully tickless with `grep NO_HZ_FULL_ALL
   /boot/config-\`uname -r\``. You should see `CONFIG_NO_HZ_FULL_ALL=y` in the
   output.

 * Use `grep -r krun /usr/include`. There should be some system call numbers
   defined, starting `__NR_krun_` .

 * Check the syscalls work with `cd krun && make && ./test_prog` (in the
   top-level of this repo). This runs a program which resets the MSRs and
   prints them every so often. The counters should increase monotonically
   (unless you run long enough to see an overflow).

## Further Reading

For more information on tickless mode, see
[the kernel docs](https://www.kernel.org/doc/Documentation/timers/NO_HZ.txt).

For more information about `make deb-pkg`, see
[the debian handbook](https://debian-handbook.info/browse/stable/sect.kernel-compilation.html).

If you are having trouble setting your default kernel, [this may help](

## License

See the COPYING file for licensing information.
