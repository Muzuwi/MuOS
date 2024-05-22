.text
.globl platform_idle
platform_idle:
    wfi
    j platform_idle
