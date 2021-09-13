import argparse
import re
import math

TILES_X = 16
TILES_Y = 8
CACHE_BLOCK_SIZE = 32
CACHE_X_SHIFT = int(math.ceil(math.log2(CACHE_BLOCK_SIZE)))+2
CACHE_X_MASK = (1<<int(math.ceil(math.log2(TILES_X))))-1
CACHE_SOUTH_NOT_NORTH_SHIFT = CACHE_X_SHIFT+math.ceil(math.log2(TILES_X))

print("CACHE_X_SHIFT = %d" % CACHE_X_SHIFT)

parser = argparse.ArgumentParser()
parser.add_argument("profile_log")

args = parser.parse_args()

tile_info = [[{} for y in range(TILES_Y)] for x in range(TILES_X)]

with open(args.profile_log,'r') as f:
    for line in f:
        if line.startswith('int32'):
            m = re.search('from \((\d+),(\d+)\): (\d+)', line)
            tile_x = int(m.group(1))-TILES_X
            tile_y = int(m.group(2))-TILES_Y
            tile_data = int(m.group(3))            
            try:
                d = tile_info[tile_x][tile_y]
            except Exception as e:
                print('out of range on tile_x=%d,tile_y=%d,tile_data=%d'%
                      (tile_x,tile_y,tile_data))
                exit(1)
            if 'tg-x' not in d:
                d['tg-x'] = tile_data
            elif 'tg-y' not in d:
                d['tg-y'] = tile_data
            elif 'tg-id' not in d:
                d['tg-id'] = tile_data
        elif line.startswith('uint32'):
            m = re.search('from \((\d+),(\d+)\): (0x[0-9a-f]+)', line)
            tile_x = int(m.group(1))-TILES_X
            tile_y = int(m.group(2))-TILES_Y
            tile_data = int(m.group(3),16)
            try:
                d = tile_info[tile_x][tile_y]
            except Exception as e:
                print('out of range on tile_x=%d,tile_y=%d,tile_data=%d'%
                      (tile_x,tile_y,tile_data))
                exit(1)
            if 'vcaches' not in d:
                d['vcaches'] = set()
            s = d['vcaches']
            addr = tile_data
            cache_x = (addr >> CACHE_X_SHIFT) & CACHE_X_MASK
            south_not_north = (addr >> CACHE_SOUTH_NOT_NORTH_SHIFT) & 1
            print("addr = %x, cache_x = %x, south_not_north = %d" %(addr,cache_x,south_not_north))                
            string = '{} vcache {}'.format('south' if south_not_north else 'north', cache_x)
            s.add(string)

output = []
for y in range(TILES_Y):
    row = []
    for x in range(TILES_X):
        d = tile_info[x][y]
        tile_str = "(group: {}, ({},{}))".format(d['tg-id'],d['tg-x'],d['tg-y'])
        row.append(tile_str)
        # print target caches
        if 'vcaches' not in d:
            continue
        
        for vc in d['vcaches']:
            print('({},{}) targets {}'.format(x,y,vc))
            
    output.append('\t'.join(row))
print('\n'.join(output))
