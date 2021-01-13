import itertools as it
import numpy as np
from collections import namedtuple

RUCHE_FACTOR = 3
Coordinate = namedtuple('Coordinate', ['X', 'Y'])
Dimension = namedtuple('Dimension', ['X', 'Y'])
Hops = namedtuple('Hops', ['X', 'Y', 'Ruche'])

pod_origin = Coordinate(X=16, Y=8)
pod_dim = Dimension(X=16, Y=8)

tile_xs = range(pod_origin.X, pod_origin.X + pod_dim.X)
tile_ys = range(pod_origin.Y, pod_origin.Y + pod_dim.Y)

cache_xs = tile_xs
cache_ys = [pod_origin.Y - 1, pod_origin.Y + pod_dim.Y]

tiles = [Coordinate(X=x, Y=y) for (x,y) in it.product(tile_xs, tile_ys)]
caches = [Coordinate(X=x, Y=y) for (x,y) in it.product(cache_xs, cache_ys)]

def calc_hops(src, dest):
    x_dist = abs(src.X - dest.X)
    y_dist = abs(src.Y - dest.Y)
    # Packets travel RUCHE_FACTOR on each hop but must use local network before turning (subtract -1)
    # Use // for floor division, but (-1 // N == -1), so use max() to filter out negatives (when x_dist == 0)
    ruche_hops = max(0, x_dist - 1) // RUCHE_FACTOR
    x_hops = x_dist - ruche_hops * RUCHE_FACTOR
    y_hops = y_dist
    return Hops(X=x_hops, Y=y_hops, Ruche=ruche_hops)

def linear1(src, dest):
    h = calc_hops(src, dest)
    return 2*(h.Ruche + h.Y + h.X)

def linear2(src, dest):
    h = calc_hops(src, dest)
    return (h.Ruche + h.Y + h.X)

def ysquare(src, dest):
    h = calc_hops(src, dest)
    return h.Ruche + h.Y**2 + h.X

default = {tuple(t): 32 for t in tiles}
lin1 = {tuple(t): max([linear1(t, c) for c in caches]) for t in tiles}
lin2 = {tuple(t): (12*max([linear2(t, c) for c in caches]))//15 for t in tiles}
ysq = {tuple(t): max([ysquare(t, c) for c in caches]) for t in tiles}

def format(ds):
    tiles = ds.keys()
    xs = sorted(list({t[0] for t in tiles}))
    ys = sorted(list({t[1] for t in tiles}))
    s = "{"
    for y in ys:
        l = "{"
        for x in xs:
            l += f"{ds[(x,y)]:2d}, "
        l = (l[:-2] +  "},\n")
        s += l
    s = s[:-2] + "};"
    return s

print("Default")
print(format(default))
print("Linear1: 2*(X + Ruche + Y)")
print(format(lin1))
print("Linear2: 2*(X + Ruche + Y)")
print(format(lin2))
print("Y-Squared (X + Ruche + Y**2)")
print(format(ysq))


    
