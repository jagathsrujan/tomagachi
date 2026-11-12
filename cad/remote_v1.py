# Handheld remote v1 shell. Parametric FreeCAD script.
# Run inside the FreeCAD GUI:  exec(open(r"<this file>").read())
# Axes: X across, Y along the body (head at +Y, USB at -Y), Z thickness
# (Z=0 is the back face, Z=T is the front face). Head centre is the origin.
# All part sizes below are nominal guesses until measured. Edit and re-run.

import os, math
import FreeCAD as App, Part
from FreeCAD import Vector as V

BASE = r"E:\tomagachi ai agent product\tomagachi\cad"
DOC = "remote_v1"

# ---------------- parameters: measure these on the real parts ----------------
# Display: GC9A01 1.28" round module
DISP_GLASS_D = 35.6   # glass outer diameter (+0.4 fit is added)
DISP_WIN_D   = 33.0    # visible window in the front wall
DISP_PCB_D   = 38.1   # round PCB diameter
DISP_STACK   = 3.25   # glass face to PCB back
DISP_TAB_Y   = -25.0  # header row position below display centre
# Speaker: round
SPK_D        = 40.0    # outer diameter
SPK_DEPTH    = 20.0    # face to back of magnet (max 21 in this shell)
# ESP32-S3 dev board, header pins soldered, mounted pins-toward-front
BRD_L        = 64.0    # length
BRD_W        = 28.0    # width
BRD_COMP_H   = 3.2     # tallest part on the component side (USB-C, module can)
# Outemu (MX-style) switches
SW_N         = 3
SW_PITCH     = 19.0
SW_CUT       = 14.0    # square plate cutout
SW_PLATE     = 1.5     # plate thickness the clips need
# Small modules
MIC_D        = 14.0    # INMP441 round breakout
MIC_PORT_OFF = 1.4     # sound port offset from PCB centre, toward the display (from the STEP model)
AMP_W, AMP_L = 17.6, 18.5   # MAX98357A breakout (from the STEP model)
# Fasteners
INSERT_HOLE_D = 4.0    # hole for M3 heat-set insert (OD about 4.6-5.0)
SCREW_D       = 3.4    # M3 clearance
HEAD_D        = 6.4    # M3 cap head counterbore

# ---------------- shell geometry ----------------
T        = 34.0   # body thickness
HEAD_R   = 34.0
HANDLE_W = 46.0
BOT      = 128.0  # bottom of handle is y = -BOT
NECK_R   = 14.0
BOT_R    = 8.0
EDGE_R   = 3.0    # outer edge rounding
WALL     = 2.4    # side wall
FLOOR    = 2.0    # back wall
FRONT    = 2.0    # front wall
SPLIT    = 12.0   # shells part at this Z
SPK_Y    = 4.0    # speaker centre above display centre
MIC_Y    = -38.5  # below the display's header tab, which reaches y = -26.5
SW_Y0    = -58.0  # first switch centre
WELL_SINK = 11.0  # plate depth below the front face: cap tops stand 2 mm proud, switches hidden
WELL_HALF = 9.4   # well half width: keycap skirt 8.8 + 0.6 gap
BOSS_R   = 3.75
TOE_BOSS_H = 8.0
BRIDGE_BOSS_TOP = 16.0

# Dial potentiometer (9 mm RV09 / RK09 type, 6 mm D shaft). Measure yours.
#   It sits in the back shell beside the neck, shaft toward the front; a thumbwheel on
#   the shaft pokes out of the right side wall (right thumb in a right-hand grip).
POT_X       = 15.4    # pot axis
POT_Y       = -37.5
WHEEL_R     = 10.6    # wheel radius: stands 3 mm out of the side wall
WHEEL_Z0, WHEEL_Z1 = 16.0, 20.5   # wheel rim, 4.5 mm thick
POT_BODY_W  = 9.8     # body width (across X)
POT_BODY_L  = 11.0    # body length
POT_BODY_H  = 6.8     # body depth behind the mounting face
POT_BUSH_D  = 7.0     # bushing diameter (M7 if threaded)
POT_BUSH_H  = 5.0     # bushing height above the mounting face
POT_SHAFT_D = 6.0
POT_SHAFT_L = 13.0    # shaft tip above the mounting face
POT_D_FLAT  = 4.5     # D-shaft thickness across the flat; 0 for a round shaft
POT_FACE    = 10.0    # pot mounting face (underside of a 2 mm plate in the back shell)

HW = HANDLE_W / 2.0
IN_FRONT = T - FRONT                 # inner face of front wall
PLATE_TOP = T - WELL_SINK
PLATE_BOT = PLATE_TOP - SW_PLATE
DISP_WELL = 1.5                      # display sits in a dished well this deep
GLASS_Z = T - DISP_WELL - 1.0        # glass face, behind a 1 mm lip at the well floor
PCB_BACK = GLASS_Z - DISP_STACK
BRD_Y1 = -BOT + WALL + 0.3           # USB end of board
BRD_Y0 = BRD_Y1 + BRD_L              # antenna end
BRD_Z  = FLOOR + 2.8 + BRD_COMP_H    # PCB face on the component side
BRD_ZT = BRD_Z + 1.6                 # PCB face on the pin side
CLOSE_POS = [(22.0, 18.0), (-22.0, 18.0),
             (HW - 4.4, -54.0), (-(HW - 4.4), -54.0),
             (HW - 4.4, -112.0), (-(HW - 4.4), -112.0)]
TOE_POS = [(22.0, -12.0), (-22.0, -12.0)]
BRIDGE_X = 26.5

def box(x0, x1, y0, y1, z0, z1):
    return Part.makeBox(x1 - x0, y1 - y0, z1 - z0, V(x0, y0, z0))

def cyl(x, y, r, z0, z1):
    return Part.makeCylinder(r, z1 - z0, V(x, y, z0))

def fuse_all(shapes):
    s = shapes[0]
    for o in shapes[1:]:
        s = s.fuse(o)
    return s

# ---------------- outer body ----------------
prism = cyl(0, 0, HEAD_R, 0, T).fuse(box(-HW, HW, -BOT, 0, 0, T)).removeSplitter()

def vertical_edges(solid, pred):
    out = []
    for e in solid.Edges:
        if len(e.Vertexes) != 2:
            continue
        a, b = e.Vertexes[0].Point, e.Vertexes[1].Point
        if abs(a.x - b.x) < 1e-6 and abs(a.y - b.y) < 1e-6 and abs(e.Length - T) < 1e-6:
            if pred(a):
                out.append(e)
    return out

neck = vertical_edges(prism, lambda p: abs(abs(p.x) - HW) < 1e-3 and -BOT + 1 < p.y < -1)
prism = prism.makeFillet(NECK_R, neck)
bot = vertical_edges(prism, lambda p: abs(p.y + BOT) < 1e-3)
prism = prism.makeFillet(BOT_R, bot)

wire = prism.slice(V(0, 0, 1), T / 2.0)[0]
outline = Part.Face(wire)
outline.translate(V(0, 0, -T / 2.0))

def off(d):
    """Outline shrunk inward by d, as a face at z=0."""
    return outline.makeOffset2D(-d)

def slab(face, z0, z1):
    f = face.copy()
    f.translate(V(0, 0, z0))
    return f.extrude(V(0, 0, z1 - z0))

rim = [e for e in prism.Edges
       if all(abs(v.Point.z) < 1e-6 for v in e.Vertexes)
       or all(abs(v.Point.z - T) < 1e-6 for v in e.Vertexes)]
try:
    outer = prism.makeFillet(EDGE_R, rim)
    if not outer.isValid() or outer.Volume < 1:
        raise RuntimeError("bad fillet")
    print("outer edge: fillet R%.1f" % EDGE_R)
except Exception as ex:
    outer = prism.makeChamfer(1.5, rim)
    print("outer edge: fillet failed (%s), used 1.5 chamfer" % ex)

cavity = slab(off(WALL), FLOOR, IN_FRONT)
clip = slab(off(WALL - 0.4), FLOOR - 0.3, IN_FRONT + 0.3)
body = outer.cut(cavity)
BIG = 400
back = body.common(box(-BIG, BIG, -BIG, BIG, -1, SPLIT))
front = body.common(box(-BIG, BIG, -BIG, BIG, SPLIT, T + 1))

# ---------------- front shell ----------------
add, cut = [], []

# display: window, glass counterbore, flex tail relief
WR = DISP_WIN_D / 2
add.append(cyl(0, 0, DISP_GLASS_D / 2 + 3.0, GLASS_Z - 1.0, IN_FRONT + 0.1))      # lip and glass locating ring
cut.append(cyl(0, 0, WR, GLASS_Z - 0.1, T + 1))                                  # window
cut.append(Part.makeCone(WR, WR + DISP_WELL, DISP_WELL, V(0, 0, T - DISP_WELL)))  # 45 degree well
cut.append(cyl(0, 0, DISP_GLASS_D / 2 + 0.2, GLASS_Z - 1.1, GLASS_Z))             # glass pocket
cut.append(box(-8.4, 8.4, -21.1, 0, GLASS_Z - 1.1, GLASS_Z))            # flex tail

# toe-clamp bosses for the display
for (x, y) in TOE_POS:
    add.append(cyl(x, y, BOSS_R, IN_FRONT - TOE_BOSS_H, IN_FRONT))
    cut.append(cyl(x, y, INSERT_HOLE_D / 2, IN_FRONT - TOE_BOSS_H - 0.1, IN_FRONT - 1.0))

# mic: sound hole and locating ring
cut.append(cyl(0, MIC_Y + MIC_PORT_OFF, 1.25, IN_FRONT - 1, T + 1))
mic_ring = cyl(0, MIC_Y, MIC_D / 2 + 1.5, IN_FRONT - 2.5, IN_FRONT).cut(
    cyl(0, MIC_Y, MIC_D / 2 + 0.3, IN_FRONT - 3, IN_FRONT + 1))
mic_ring = mic_ring.cut(box(-2.5, 2.5, MIC_Y - 12, MIC_Y, IN_FRONT - 3, IN_FRONT))  # wire gap
add.append(mic_ring)

# switch well: sunken plate with square cutouts
sw_ys = [SW_Y0 - i * SW_PITCH for i in range(SW_N)]
wy0, wy1 = sw_ys[-1] - WELL_HALF, sw_ys[0] + WELL_HALF
add.append(box(-11.25, 11.25, wy0 - 1.5, wy1 + 1.5, PLATE_BOT, IN_FRONT + 0.1))
well_cut = [box(-WELL_HALF, WELL_HALF, wy0, wy1, PLATE_TOP, T + 1)]
for y in sw_ys:
    well_cut.append(box(-SW_CUT / 2, SW_CUT / 2, y - SW_CUT / 2, y + SW_CUT / 2,
                        PLATE_BOT - 1, PLATE_TOP + 1))

# board hold-down ribs (press on the bare pin-side face of the board)
rib_ys = [(sw_ys[0] + sw_ys[1]) / 2.0, wy0 - 7.0]
add.append(box(-6, 6, rib_ys[0] - 1.5, rib_ys[0] + 1.5, BRD_ZT + 0.2, PLATE_BOT + 0.1))
add.append(box(-6, 6, rib_ys[1] - 1.5, rib_ys[1] + 1.5, BRD_ZT + 0.2, IN_FRONT + 0.1))
# thumbwheel notch in the side wall, open toward the split so the shell drops over the wheel
cut.append(cyl(POT_X, POT_Y, WHEEL_R + 0.6, SPLIT - 0.1, WHEEL_Z1 + 0.4))

# closing bosses with heat-set inserts (inserted from the split face)
for (x, y) in CLOSE_POS:
    add.append(cyl(x, y, BOSS_R, SPLIT, IN_FRONT + 0.1).common(clip))
    cut.append(cyl(x, y, INSERT_HOLE_D / 2, SPLIT - 0.1, SPLIT + 7.0))
    cut.append(cyl(x, y, 1.6, SPLIT, SPLIT + 12.0))

# alignment lip that drops into the back shell
lip = slab(off(WALL + 0.2).cut(off(WALL + 1.4)), SPLIT - 2.0, SPLIT)
lip = lip.fuse(slab(off(WALL - 0.1).cut(off(WALL + 1.4)), SPLIT, SPLIT + 3.0))
lip_keepout = [cyl(x, y, BOSS_R + 0.4, 0, SPLIT) for (x, y) in CLOSE_POS]
lip_keepout += [cyl(sx * BRIDGE_X, SPK_Y, BOSS_R + 0.4, 0, BRIDGE_BOSS_TOP + 0.5) for sx in (1, -1)]
lip_keepout.append(box(-14, 14, -BOT - 1, -BOT + 8, 0, SPLIT))  # USB bay
lip_keepout.append(box(POT_X - 8, HW + 2, POT_Y - 8.5, POT_Y + 8.5, 0, SPLIT + 0.1))  # pot plate
lip = lip.cut(fuse_all(lip_keepout))
add.append(lip)

front = front.fuse(fuse_all(add))
front = front.cut(fuse_all(cut + well_cut)).removeSplitter()

# round the rim and the floor edge of the display well
def _well_edges(z, r):
    return [e for e in front.Edges
            if hasattr(e.Curve, "Radius") and abs(e.Curve.Radius - r) < 1e-3
            and abs(e.Curve.Center.z - z) < 1e-3
            and abs(e.Curve.Center.x) < 1e-3 and abs(e.Curve.Center.y) < 1e-3]
for (z, r, rad) in ((T, WR + DISP_WELL, 1.2), (T - DISP_WELL, WR, 0.4)):
    try:
        f = front.makeFillet(rad, _well_edges(z, r))
        if f.isValid() and len(f.Solids) == 1:
            front = f
            print("display well: fillet R%.1f at z=%.1f" % (rad, z))
        else:
            print("display well: fillet at z=%.1f gave a bad solid, skipped" % z)
    except Exception as ex:
        print("display well: fillet at z=%.1f failed (%s)" % (z, ex))

# ---------------- back shell ----------------
add, cut = [], []

# speaker grille and locating ring
p = 4.2
for i in range(-6, 7):
    for j in range(-6, 7):
        gx = i * p + (p / 2 if j % 2 else 0)
        gy = j * p * 0.866
        if math.hypot(gx, gy) <= SPK_D / 2 - 4.0:
            cut.append(cyl(gx, SPK_Y + gy, 1.25, -1, FLOOR + 1))
ring = cyl(0, SPK_Y, SPK_D / 2 + 0.3 + 1.6, FLOOR - 0.1, FLOOR + 5).cut(
    cyl(0, SPK_Y, SPK_D / 2 + 0.3, FLOOR - 1, FLOOR + 6))
ring = ring.cut(box(-4, 4, SPK_Y - SPK_D / 2 - 3, SPK_Y - SPK_D / 2 + 2, FLOOR, FLOOR + 6))
add.append(ring)

# speaker bridge bosses
for sx in (1, -1):
    add.append(cyl(sx * BRIDGE_X, SPK_Y, BOSS_R, FLOOR - 0.1, BRIDGE_BOSS_TOP).common(clip))
    cut.append(cyl(sx * BRIDGE_X, SPK_Y, INSERT_HOLE_D / 2, BRIDGE_BOSS_TOP - 7.0, BRIDGE_BOSS_TOP + 1))

# amp frame
AMP_Y = -46.0
fr = box(-AMP_W / 2 - 1.5, AMP_W / 2 + 1.5, AMP_Y - AMP_L / 2 - 1.5, AMP_Y + AMP_L / 2 + 1.5,
         FLOOR - 0.1, FLOOR + 3.0)
fr = fr.cut(box(-AMP_W / 2 - 0.3, AMP_W / 2 + 0.3, AMP_Y - AMP_L / 2 - 0.3, AMP_Y + AMP_L / 2 + 0.3,
                FLOOR - 1, FLOOR + 4))
fr = fr.cut(box(-5, 5, AMP_Y, AMP_Y + AMP_L, FLOOR, FLOOR + 4))   # wire gap, head side
add.append(fr)

# pot mount: 2 mm plate with a U slot (pot slides in from the head side), a rail and an
# end stop so the body cannot turn, and a wall tab that fills the notch under the wheel
px0 = POT_X - POT_BODY_W / 2 - 0.25
plate = box(px0 - 1.2, HW, POT_Y - 6.5, POT_Y + 6.5, POT_FACE, POT_FACE + 2.0).common(clip)
plate = plate.cut(cyl(POT_X, POT_Y, POT_BUSH_D / 2 + 0.15, POT_FACE - 1, POT_FACE + 3))
plate = plate.cut(box(POT_X - POT_BUSH_D / 2 - 0.15, POT_X + POT_BUSH_D / 2 + 0.15, POT_Y, POT_Y + 10,
                      POT_FACE - 1, POT_FACE + 3))
add.append(plate)
add.append(box(px0 - 1.2, px0, POT_Y - 6.5, POT_Y + 6.5, FLOOR - 0.1, POT_FACE + 0.1))
add.append(box(px0 - 1.2, HW, POT_Y - 7.0, POT_Y - POT_BODY_L / 2 - 0.3, FLOOR - 0.1, POT_FACE + 0.1).common(clip))
add.append(body.common(cyl(POT_X, POT_Y, WHEEL_R + 0.3, SPLIT - 0.1, WHEEL_Z0 - 0.4)))

# board cradle: pads under the module can and USB shells, side guides, end stop
pad_top = BRD_Z - BRD_COMP_H
add.append(box(-7, 7, BRD_Y0 - 17, BRD_Y0 - 3, FLOOR - 0.1, pad_top))
add.append(box(-12, 12, BRD_Y1 + 0.5, BRD_Y1 + 7, FLOOR - 0.1, pad_top))
for sx in (1, -1):
    x0 = sx * (BRD_W / 2 + 0.3)
    x1 = sx * (BRD_W / 2 + 1.8)
    add.append(box(min(x0, x1), max(x0, x1), BRD_Y1 + 3, BRD_Y0 - 1, FLOOR - 0.1, BRD_ZT + 1.4))
add.append(box(-10, 10, BRD_Y0 + 0.3, BRD_Y0 + 1.8, FLOOR - 0.1, BRD_ZT + 1.4))

# closing bosses: clearance hole and head counterbore from the back
for (x, y) in CLOSE_POS:
    add.append(cyl(x, y, BOSS_R, FLOOR - 0.1, SPLIT).common(clip))
    cut.append(cyl(x, y, SCREW_D / 2, -1, SPLIT + 1))
    cut.append(cyl(x, y, HEAD_D / 2, -1, 3.0))

# USB bay: room for two USB-C plug overmoulds
cut.append(box(-13, 13, -BOT - 1, -BOT + WALL + 0.5, 2.5, 10.5))

back = back.fuse(fuse_all(add))
back = back.cut(fuse_all(cut)).removeSplitter()

# ---------------- loose printed parts ----------------
def toe_clamp(bx, by):
    zb = IN_FRONT - TOE_BOSS_H
    s = box(-BOSS_R, 8.5, -4, 4, zb - 2.5, zb)
    s = s.fuse(box(6.0, 8.5, -4, 4, zb - 0.1, PCB_BACK))
    s = s.cut(cyl(0, 0, SCREW_D / 2, zb - 3, zb + 1))
    s.rotate(V(0, 0, 0), V(0, 0, 1), math.degrees(math.atan2(-by, -bx)))
    s.translate(V(bx, by, 0))
    return s

toes = [toe_clamp(x, y) for (x, y) in TOE_POS]

br_z = FLOOR + SPK_DEPTH
bridge = box(-BRIDGE_X - BOSS_R, BRIDGE_X + BOSS_R, SPK_Y - 4, SPK_Y + 4, br_z, br_z + 2.5)
for sx in (1, -1):
    bridge = bridge.fuse(box(sx * BRIDGE_X - BOSS_R, sx * BRIDGE_X + BOSS_R, SPK_Y - 4, SPK_Y + 4,
                             BRIDGE_BOSS_TOP, br_z + 0.1))
for sx in (1, -1):
    bridge = bridge.cut(cyl(sx * BRIDGE_X, SPK_Y, SCREW_D / 2, BRIDGE_BOSS_TOP - 1, br_z + 4))
bridge = bridge.removeSplitter()

# ---------------- stand-in components (for fit checks only) ----------------
mock = {}
d = cyl(0, 0, DISP_PCB_D / 2, PCB_BACK, PCB_BACK + 1.6)
d = d.fuse(cyl(0, 0, DISP_GLASS_D / 2, PCB_BACK + 1.6, GLASS_Z))
d = d.fuse(box(-11.5, 11.5, DISP_TAB_Y - 1.5, -10, PCB_BACK, PCB_BACK + 1.6))
mock["display"] = d
mock["display_jumpers"] = box(-9, 9, DISP_TAB_Y - 1.3, DISP_TAB_Y + 1.3, PCB_BACK - 21.5, PCB_BACK)
s = cyl(0, SPK_Y, SPK_D / 2, FLOOR, FLOOR + 2)
s = s.fuse(Part.makeCone(SPK_D / 2, 12, 10, V(0, SPK_Y, FLOOR + 2)))
s = s.fuse(cyl(0, SPK_Y, 12, FLOOR + 12, FLOOR + SPK_DEPTH))
mock["speaker"] = s
b = box(-BRD_W / 2, BRD_W / 2, BRD_Y1, BRD_Y0, BRD_Z, BRD_ZT)
b = b.fuse(box(-9, 9, BRD_Y0 - 20, BRD_Y0, BRD_Z - 3.1, BRD_Z))
for ux in (-6.5, 6.5):
    b = b.fuse(box(ux - 4.5, ux + 4.5, BRD_Y1 - 0.2, BRD_Y1 + 7, BRD_Z - BRD_COMP_H, BRD_Z))
mock["board"] = b
j = None
for sx in (1, -1):
    r = box(min(sx * 11.4, sx * 14.0), max(sx * 11.4, sx * 14.0), BRD_Y1 + 5, BRD_Y0 - 2,
            BRD_ZT, BRD_ZT + 16.5)
    j = r if j is None else j.fuse(r)
mock["board_jumpers"] = j
def frustum(cx, cy, a0, b0, z0, a1, b1, z1):
    """Tapered block: half-sizes a0 x b0 at z0 to a1 x b1 at z1."""
    def rect(a, b, z):
        return Part.makePolygon([V(cx - a, cy - b, z), V(cx + a, cy - b, z), V(cx + a, cy + b, z),
                                 V(cx - a, cy + b, z), V(cx - a, cy - b, z)])
    return Part.makeLoft([rect(a0, b0, z0), rect(a1, b1, z1)], True, True)

# Outemu (MX footprint) switch: housing, stem, pins
sw_h, sw_s, sw_p = [], [], []
PT = PLATE_TOP
for y in sw_ys:
    h = box(-6.95, 6.95, y - 6.95, y + 6.95, PT - 5.0, PT)              # lower housing in the cutout
    h = h.fuse(box(-7.8, 7.8, y - 7.8, y + 7.8, PT, PT + 1.0))           # flange on the plate
    h = h.fuse(frustum(0, y, 7.0, 7.0, PT + 1.0, 5.5, 5.25, PT + 6.6))   # upper housing
    h = h.fuse(cyl(0, y, 2.0, PT - 8.3, PT - 5.0))                       # centre post
    h = h.cut(box(-3.6, 3.6, y - 2.9, y + 2.9, PT + 2.0, PT + 7))        # stem opening (room for 4 mm travel)
    sw_h.append(h)
    s = box(-3.5, 3.5, y - 2.8, y + 2.8, PT + 3.0, PT + 6.6)             # slider
    s = s.fuse(box(-2.05, 2.05, y - 0.6, y + 0.6, PT + 6.6, PT + 10.0))  # cross stem (top 10.0, measured)
    s = s.fuse(box(-0.6, 0.6, y - 2.05, y + 2.05, PT + 6.6, PT + 10.0))
    sw_s.append(s)
    sw_p.append(box(-3.81 - 0.5, -3.81 + 0.5, y + 2.54 - 0.15, y + 2.54 + 0.15, PT - 8.3, PT - 5.0))
    sw_p.append(box(2.54 - 0.5, 2.54 + 0.5, y + 5.08 - 0.15, y + 5.08 + 0.15, PT - 8.3, PT - 5.0))
mock["switches"] = Part.makeCompound(sw_h)
mock["switch_stems"] = Part.makeCompound(sw_s)
mock["switch_pins"] = Part.makeCompound(sw_p)

# Keycaps from keycap.py (datum: switch axis at plate top). Top to bottom: Talk, Approve, Deny.
_kc = {"__name__": "keycap"}
exec(open(os.path.join(BASE, "keycap.py")).read(), _kc)
KEY_TRAVEL = _kc["TRAVEL"]
KEY_NAMES = ("talk", "approve", "deny")
keycap_local = {n: _kc["make_keycap"](n) for n in KEY_NAMES}
keycaps = {}
for n, y in zip(KEY_NAMES, sw_ys):
    c = keycap_local[n].copy()
    c.translate(V(0, y, PLATE_TOP))
    keycaps["keycap_" + n] = c
mock["amp"] = box(-AMP_W / 2, AMP_W / 2, AMP_Y - AMP_L / 2, AMP_Y + AMP_L / 2, FLOOR, FLOOR + 3.0)
mock["mic"] = cyl(0, MIC_Y, MIC_D / 2, IN_FRONT - 2.6, IN_FRONT)

# potentiometer stand-in: body, pins, bushing, D shaft
def _dshape(r, z0, z1, clr=0.0):
    s = cyl(POT_X, POT_Y, r + clr, z0, z1)
    if POT_D_FLAT > 0:
        s = s.common(box(POT_X - 10, POT_X + POT_D_FLAT - POT_SHAFT_D / 2 + clr, POT_Y - 10, POT_Y + 10, z0, z1))
    return s
pm = box(POT_X - POT_BODY_W / 2, POT_X + POT_BODY_W / 2, POT_Y - POT_BODY_L / 2, POT_Y + POT_BODY_L / 2,
         POT_FACE - POT_BODY_H, POT_FACE)
pm = pm.fuse(box(POT_X - 3, POT_X + 3, POT_Y + POT_BODY_L / 2, POT_Y + POT_BODY_L / 2 + 2,   # legs bent flat
                 POT_FACE - POT_BODY_H - 1.0, POT_FACE - POT_BODY_H + 1))
pm = pm.fuse(cyl(POT_X, POT_Y, POT_BUSH_D / 2, POT_FACE - 0.1, POT_FACE + POT_BUSH_H))
pm = pm.fuse(_dshape(POT_SHAFT_D / 2, POT_FACE + POT_BUSH_H - 0.1, POT_FACE + POT_SHAFT_L))
mock["pot"] = pm

# thumbwheel (printed, rim face on the bed, hub up: no supports)
HUB_Z1 = POT_FACE + POT_SHAFT_L + 1.5
knob = cyl(POT_X, POT_Y, WHEEL_R, WHEEL_Z0, WHEEL_Z1).fuse(cyl(POT_X, POT_Y, 4.6, WHEEL_Z0, HUB_Z1))
knob = knob.cut(_dshape(POT_SHAFT_D / 2, WHEEL_Z0 - 1, POT_FACE + POT_SHAFT_L + 0.3, 0.08))
flutes = []
for i in range(28):
    a = math.radians(i * 360.0 / 28)
    flutes.append(cyl(POT_X + (WHEEL_R + 0.25) * math.cos(a), POT_Y + (WHEEL_R + 0.25) * math.sin(a), 0.75,
                      WHEEL_Z0 - 1, WHEEL_Z1 + 1))
knob = knob.cut(fuse_all(flutes)).removeSplitter()

# ---------------- document ----------------
if DOC in App.listDocuments():
    App.closeDocument(DOC)
doc = App.newDocument(DOC)

def show(name, shape, color, transp=0):
    o = doc.addObject("Part::Feature", name)
    o.Shape = shape
    try:
        o.ViewObject.ShapeColor = color
        o.ViewObject.Transparency = transp
    except Exception:
        pass
    return o

printed = {
    "front_shell": front,
    "back_shell": back,
    "toe_clamp_a": toes[0],
    "toe_clamp_b": toes[1],
    "speaker_bridge": bridge,
}
show("front_shell", front, (0.20, 0.21, 0.24))
show("back_shell", back, (0.30, 0.31, 0.34))
show("toe_clamp_a", toes[0], (0.95, 0.45, 0.10))
show("toe_clamp_b", toes[1], (0.95, 0.45, 0.10))
show("speaker_bridge", bridge, (0.95, 0.45, 0.10))
printed["dial_wheel"] = knob
show("dial_wheel", knob, (0.95, 0.45, 0.10))
key_cols = {"keycap_talk": (0.07, 0.07, 0.08), "keycap_approve": (0.07, 0.07, 0.08),
            "keycap_deny": (0.07, 0.07, 0.08)}   # one black filament for everything
for n, s in keycaps.items():
    printed[n] = s
    show(n, s, key_cols[n])
mock_cols = {"display": (0.1, 0.3, 0.8), "display_jumpers": (0.8, 0.8, 0.2), "speaker": (0.6, 0.6, 0.6),
             "board": (0.1, 0.5, 0.2), "board_jumpers": (0.8, 0.8, 0.2), "switches": (0.92, 0.92, 0.95),
             "switch_stems": (0.15, 0.55, 0.95), "switch_pins": (0.8, 0.6, 0.2),
             "amp": (0.5, 0.1, 0.6), "mic": (0.7, 0.5, 0.1), "pot": (0.2, 0.6, 0.6)}
# Real switch model (downloaded STEP). In that file the plate top is at z = -0.9
# and the pins are on the +Y side. The simple stand-ins stay for the clash check
# only, because the real model's clips overlap the cutout until they flex.
SW_STEP = os.path.join(BASE, "parts", "mx_switch.stp")
SW_STEP_PLATE_Z = -0.9
real_sw = []
if os.path.isfile(SW_STEP):
    src = Part.read(SW_STEP)
    for y in sw_ys:
        c = src.copy()
        c.translate(V(0, y, PLATE_TOP - SW_STEP_PLATE_Z))
        real_sw.append(c)
hidden = ("switches", "switch_stems", "switch_pins") if real_sw else ()

# Real mic and amp models (GrabCAD STEP files). Stand-ins stay for the clash check.
real_parts = {}
p = os.path.join(BASE, "parts", "inmp441.step")
if os.path.isfile(p):
    # File: PCB normal along Y, port face at y=0, can on +Y, header pins on -Y.
    # Shown without the header (wires are soldered direct), port face on the front wall.
    m = Part.makeCompound([s for s in Part.read(p).Solids if s.BoundBox.YMin > -0.1])
    m.rotate(V(0, 0, 0), V(1, 0, 0), -90)          # model -Y -> +Z, model +Z -> +Y
    m.translate(V(0, MIC_Y, IN_FRONT - 0.02))
    real_parts["inmp441_mic"] = (m, (0.1, 0.35, 0.6))
    hidden += ("mic",)
p = os.path.join(BASE, "parts", "gc9a01_display.step")
if os.path.isfile(p):
    # File: PCB normal along Y, glass face at y=3.21, header tab toward +Z, pins on -Y.
    m = Part.read(p)
    m.rotate(V(0, 0, 0), V(1, 0, 0), 90)           # model +Y -> +Z, model +Z -> -Y
    # the glass is 0.34 mm off the PCB centre; centre the glass on the window
    m.translate(V(0, -0.34, GLASS_Z - 3.21 - 0.02))
    real_parts["gc9a01_display"] = (m, (0.1, 0.3, 0.8))
    hidden += ("display",)
p = os.path.join(BASE, "parts", "max98357a.step")
if os.path.isfile(p):
    m = Part.read(p)
    bb = m.BoundBox
    m.translate(V(-bb.Center.x, AMP_Y - bb.Center.y, FLOOR - bb.ZMin))   # lowest point on the floor
    real_parts["max98357a_amp"] = (m, (0.5, 0.1, 0.6))
    hidden += ("amp",)
for n, s in mock.items():
    o = show("mock_" + n, s, mock_cols[n])
    if n in hidden:
        o.ViewObject.Visibility = False
for i, c in enumerate(real_sw):
    show("outemu_switch_%d" % (i + 1), c, (0.92, 0.92, 0.95))
for n, (s, col) in real_parts.items():
    show(n, s, col)
doc.recompute()

# ---------------- checks ----------------
print("--- validity ---")
for n, s in printed.items():
    bb = s.BoundBox
    print("%-15s valid=%s solids=%d vol=%.0f mm3  bbox %.1f x %.1f x %.1f" % (
        n, s.isValid(), len(s.Solids), s.Volume, bb.XLength, bb.YLength, bb.ZLength))
print("--- interference (mm3, should be 0) ---")
items = list(printed.items()) + [("mock_" + n, s) for n, s in mock.items()]
clash = 0
for i in range(len(items)):
    for k in range(i + 1, len(items)):
        v = items[i][1].common(items[k][1]).Volume
        if v > 0.01:
            clash += 1
            print("CLASH %-18s %-18s %.2f" % (items[i][0], items[k][0], v))
print("clashes: %d" % clash)

# keycaps at rest (above, in the pairwise check) and at full press: caps and
# stand-in stems move TRAVEL toward the plate, housings and shell stay put.
print("--- keycaps at full press (mm3, should be 0) ---")
down = V(0, 0, -KEY_TRAVEL)
stems_dn = mock["switch_stems"].copy()
stems_dn.translate(down)
for n, c in keycaps.items():
    cd = c.copy()
    cd.translate(down)
    for tn, t in (("front_shell", front), ("mock_switches", mock["switches"]),
                  ("mock_switch_stems", stems_dn)):
        v = cd.common(t).Volume
        print("%-15s pressed vs %-18s %.3f %s" % (n, tn, v, "CLASH" if v > 0.01 else "ok"))
    others = Part.makeCompound([o for k, o in keycaps.items() if k != n])
    v = cd.common(others).Volume
    print("%-15s pressed vs %-18s %.3f %s" % (n, "resting neighbours", v, "CLASH" if v > 0.01 else "ok"))
# clearance at rest and pressed (minimum distance to housing, flange, plate and well)
for n, c in keycaps.items():
    cd = c.copy()
    cd.translate(down)
    print("%-15s min gap: rest shell %.2f, housing %.2f | pressed shell %.2f, housing %.2f" % (
        n, c.distToShape(front)[0], c.distToShape(mock["switches"])[0],
        cd.distToShape(front)[0], cd.distToShape(mock["switches"])[0]))
# one real-STEP check: Talk cap on switch 1, at rest and pressed (STEP stem moves with it)
if real_sw:
    sols = real_sw[0].Solids
    c = keycaps["keycap_talk"]
    for tag, dz in (("rest", 0.0), ("pressed", -KEY_TRAVEL)):
        cc = c.copy()
        cc.translate(V(0, 0, dz))
        # solids 3 (spring), 4 (stem) and 5 (click jacket) travel with the cap;
        # 0, 1, 2, 6 (housings, pins) stay put
        hv = sum(cc.common(s).Volume for i, s in enumerate(sols) if i not in (3, 4, 5))
        mv = 0.0
        for i in (3, 5):
            m = sols[i].copy()
            m.translate(V(0, 0, dz))
            mv += cc.common(m).Volume
        st = sols[4].copy()
        st.translate(V(0, 0, dz))
        sv = cc.common(st).Volume
        print("real STEP %-7s cap vs housing %.3f, vs spring/jacket %.3f, vs stem %.3f mm3 (stem nubs: press-fit)"
              % (tag, hv, mv, sv))
        print("real STEP %-7s min gap cap to housing %.2f mm" % (
            tag, min(cc.distToShape(s)[0] for i, s in enumerate(sols) if i not in (3, 4, 5))))
print("overall: %.1f x %.1f x %.1f mm" % (outer.BoundBox.XLength, outer.BoundBox.YLength, outer.BoundBox.ZLength))

# ---------------- export ----------------
out = os.path.join(BASE, "out")
if not os.path.isdir(out):
    os.makedirs(out)
import MeshPart
for n, s in printed.items():
    lin = 0.05
    if n.startswith("keycap_"):   # export in print pose: skirt on z=0, top up
        s = keycap_local[n[7:]].copy()
        s.translate(V(0, 0, -s.BoundBox.ZMin))
        lin = 0.02
    m = MeshPart.meshFromShape(Shape=s, LinearDeflection=lin, AngularDeflection=0.3)
    m.write(os.path.join(out, n + ".stl"))
Part.makeCompound(list(printed.values())).exportStep(os.path.join(out, "remote_v1_printed.step"))
asm = list(printed.values()) + [s for n, s in mock.items() if n not in hidden] + real_sw + [s for (s, c) in real_parts.values()]
Part.makeCompound(asm).exportStep(
    os.path.join(out, "remote_v1_assembly.step"))
doc.saveAs(os.path.join(BASE, DOC + ".FCStd"))
print("exported to " + out)
