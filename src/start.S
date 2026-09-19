/*
 * The loader jumps to the first byte of the raw payload, so put _start at
 * the very beginning of .text (link start.o first; .text.startup sorts
 * before .text in the default script).
 */
.section .text.startup,"ax",@progbits
.global _start
.extern main
_start:
    jmp main
