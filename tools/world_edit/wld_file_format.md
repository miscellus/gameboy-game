# FORMAT

World Editor Format (.wld)

Little endian.

'ABCD' means 4 literal bytes 'A', 'B', 'C', 'D' or 0x41, 0x42, 0x43, 0x44

## Header

- '\xffWLD'  | 32 bits
- Version | 16 bits

## Chunks (like RIFF)

### Level chunk
- 'LEVL'             | 32 bits
- chunk length       | u32
- width (in tiles)   | u16
- height (in tiles)  | u16
- tile_set_index     | u16
- tile_indexes       | width \* height \* u16
- tile_solid_data    | width \* height bits

### Tile set chunk
- 'TLST'                     | 32 bits
- chunk length               | u32
- tile_count                 | u16
- tile_data_8x8_2bpp_gameboy | tile_count \* 128 bits // two bit planes
