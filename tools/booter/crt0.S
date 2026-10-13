# Arranque del cargador: pila propia en MEM2 y salto a main
    .section .text.start
    .globl _start
_start:
    lis     1, 0x9310          # pila en 0x93100000 (por encima del código del cargador)
    ori     1, 1, 0x0000
    subi    1, 1, 64
    bl      booter_main
1:  b       1b
