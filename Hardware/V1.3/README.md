Errors:

FP connector reversed
FP RX/TX swapped
3.3V DC missing feeback forward capacitor

Enable isolation not working. Actual pullup current ~300nA
--> Keep opto always on. Move R34 -> R52. R56 100k, Q16 remove. Q17 nmos. R1 0
Next version: hard pulldown (330?), opto pulls up to 3.3v