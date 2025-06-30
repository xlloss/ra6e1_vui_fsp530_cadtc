.section .rodata
.align 4

.global g_uSpeexData1Begin
.global g_uSpeexData1End

g_uSpeexData1Begin:
.incbin "../src/Model/DemoWaves16K_ENG.spxpack"
g_uSpeexData1End:
