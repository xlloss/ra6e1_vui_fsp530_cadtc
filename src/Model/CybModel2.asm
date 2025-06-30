.section .rodata
.align 4

.global uCYModel2Begin
.global uCYModel2End

uCYModel2Begin:
.incbin "../src/Model/SmartHome_pack_withTxtAndTri.bin"
uCYModel2End:

