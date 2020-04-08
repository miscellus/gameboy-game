j = 0
for i in range (1024):
	if i % 32 == 0:
		if i == 32*18:
			out("")
		if i > 0:
			out("")
		out("DB ", end="")
	else:
		out(",", end="")
		if i%32 == 20:
			out(" ", end="")
	if i&31 < 20 and i < 32*18:
		out("${:02X}".format(j&255), end="")
		j += 1
	else:
		out("${:02X}".format(floor(random()*256)), end="")