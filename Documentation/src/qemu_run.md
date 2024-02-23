# Running the kernel in QEMU

You can run the kernel in QEMU by running:
```bash
cd ${MUREPO}/Root/
qemu-system-x86_64 \
  -cpu host \
  -enable-kvm \
  -smp 4 \
  -boot d \
  -cdrom MuOS.iso \
  -d cpu_reset,unimp,guest_errors,int \
  -m 128 \
  -s \
  -serial stdio \
  -vga std \
  -no-reboot -no-shutdown
```
