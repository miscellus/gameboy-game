

# 11111111
# 10000001

# 11111111
# 10011111

# 0 is darkest
# 3 is brightest

from random import randint

def generate_bitplanes(tile):
	result = []
	for y in range(8):
		plane0 = 0
		plane1 = 0
		for x in range(8):
			color = tile[y][x] & 3

			bit_index = 7-x
			plane0 |= (color & 1) << bit_index
			plane1 |= ((color >> 1) & 1) << bit_index

		result.append((plane0 | (plane1 << 8)) & 0xffff)
	return result

tiles = [
	("Background", [
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0],
		[0,0,0,0,0,0,0,0]]),
	("Wall_01", [
		[3,3,3,3,3,3,3,2],
		[3,1,2,2,2,2,3,2],
		[3,2,3,3,3,3,3,2],
		[3,2,3,3,3,3,3,2],
		[3,2,3,3,3,3,3,2],
		[3,2,3,3,3,3,3,2],
		[3,3,3,3,3,3,3,2],
		[2,2,2,2,2,2,2,3]]),
	("Wall_02", [
		[3,3,3,3,3,3,3,3],
		[3,1,2,2,2,2,3,3],
		[3,2,3,3,3,3,3,3],
		[3,2,3,3,3,3,3,3],
		[3,2,3,3,3,3,3,3],
		[3,2,3,3,3,3,3,3],
		[3,3,3,3,3,3,3,3],
		[3,3,3,3,3,3,3,3]]),
	("Wall_03", [
		[3,3,3,3,3,3,3,3],
		[3,3,3,3,3,3,3,3],
		[3,3,2,2,3,3,3,3],
		[3,3,2,2,3,3,3,3],
		[3,3,3,3,3,3,3,3],
		[3,3,3,3,3,3,3,3],
		[3,3,3,3,3,3,3,3],
		[3,3,3,3,3,3,3,3]]),
	("CircleCorner", [
		[0,0,0,0,0,3,3,3],
		[0,0,0,3,3,2,2,2],
		[0,0,3,2,2,2,2,2],
		[0,3,2,2,2,2,2,2],
		[0,3,2,2,2,2,2,2],
		[3,2,2,2,2,2,2,2],
		[3,2,2,2,2,2,2,2],
		[3,2,2,2,2,2,2,2]]),
	("Player2", [
		[0,0,3,3,3,3,0,0],
		[3,3,3,3,3,3,3,3],
		[3,3,1,3,3,1,3,3],
		[0,3,1,3,3,1,3,0],
		[0,3,3,3,3,3,3,0],
		[3,3,3,3,3,3,3,3],
		[3,3,3,3,3,3,3,3],
		[0,0,3,3,3,3,0,0]]),
	("unused145", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused815", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused104", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused113", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused805", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused463", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused439", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused273", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused904", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused752", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused928", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused848", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused611", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused276", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused242", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused043", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused039", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused686", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused196", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused893", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused619", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused812", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused471", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused044", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("unused007", [[randint(0,3) for i in range(8)] for j in range(8)]),
	("Player_2", [
		[0,3,3,3,3,3,3,0],
		[3,3,3,3,3,3,3,3],
		[3,2,2,3,2,2,2,3],
		[3,1,3,1,1,3,1,3],
		[3,1,3,1,1,3,1,3],
		[3,1,1,1,1,1,1,3],
		[3,1,1,1,1,1,1,3],
		[0,3,3,3,3,3,3,0]]),
]

print("""\
SECTION "Tiles",ROM0

TileData:""")

for tile_index, tile_entry in enumerate(tiles):
	print("Tile_{} equ {}".format(tile_entry[0], tile_index))
	print("dw", ','.join(map(lambda x: "${:04x}".format(x), generate_bitplanes(tile_entry[1]))))

print("TileDataEnd:")