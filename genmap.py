from random import randint

j = 0
for i in range (1024):
	if i % 32 == 0:
		if i == 32*18:
			print("")
		if i > 0:
			print("")
		print("DB ", end="")
	else:
		print(",", end="")
		if i%32 == 20:
			print(" ", end="")
	if i&31 < 20 and i < 32*18:
		print("${:02X}".format(0x1d if randint(0, 100) < 10 else 0), end="")
		j += 1
	else:
		print("${:02X}".format(0x1d), end="")