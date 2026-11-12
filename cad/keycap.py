# MX-stem keycap for the handheld remote (Outemu Blue). Parametric FreeCAD script.
# Datum: origin on the switch axis at the plate top, +Z toward the user, cap at rest.
# Use:  ns = {"__name__": "keycap"}; exec(open(r"<this file>").read(), ns)
#       solid = ns["make_keycap"]("talk" | "approve" | "deny")
# Run directly in the FreeCAD GUI to show the three variants side by side.
#
# Print upright: skirt and post bottoms on the bed (both at Z_BASE), top face up.
# The thumb surface is then the last, cleanest layers; the only bridge is the
# cavity ceiling (about 3.4 mm spans between skirt and post). No supports.

import Part
import FreeCAD as App
from FreeCAD import Vector as V

# ---------- switch stem, measured from parts/mx_switch.stp (heights above plate top) ----------
STEM_ARM_L   = 4.00   # cross arm tip to tip (both arms)
STEM_ARM_T   = 1.10   # arm thickness (X arm has 1.30 crush nubs near its tips)
STEM_TOP     = 10.01  # cross top at rest
SLIDER_TOP   = 6.42   # slider shoulder at rest (cross starts here)
TRAVEL       = 4.0
HOUSING_TOP  = 6.6    # conservative (real model 6.0, stand-in 6.6)

# ---------- fit ----------
SOCKET_CLR   = 0.08   # per side, FDM clearance on the cross
LEAD_IN      = 0.3    # socket mouth chamfer
ROOF_GAP     = 0.24   # socket roof above stem top
PRESS_CLR    = 0.3    # cap ceiling above housing top at full press
POST_HX      = 3.45   # post half sizes: must fit the housing's stem opening
POST_HY      = 2.70   #   (real opening about +-3.9 x +-3.05, stand-in +-3.6 x +-2.9)

# ---------- shape ----------
Z_BASE       = 6.65   # skirt and post bottom (just above the slider shoulder)
SKIRT_HALF   = 8.8    # 17.6 mm square at the bottom
TOP_HALF     = 7.8    # 15.6 mm square at the top
CORNER_R     = 2.0    # outer vertical corner radius
TOP_EDGE_R   = 1.0    # outer top edge round
WALL         = 1.2
ROOF_CHAMFER = 0.8    # 45 deg chamfer where the cavity meets its ceiling
TOP_SKIN     = 1.2    # minimum top thickness over the cavity ceiling
DISH         = 0.9    # dish depth at the edge midpoints of the flat top
SYM_H        = 0.7    # raised symbol height above the dish
SYM_W        = 1.5    # symbol stroke width
DOME         = 0.6    # (unused) rise of the old Approve dome
BAR_R        = 0.9    # Talk bar: rounded rib, radius = height above dish bottom
BAR_L        = 9.0    # Talk bar length (across X, tip to tip)

CEIL   = HOUSING_TOP + TRAVEL + PRESS_CLR     # cavity ceiling at rest
TOP_Z  = CEIL + TOP_SKIN + DISH               # top rim height
ROOF_Z = STEM_TOP + ROOF_GAP                  # socket roof
FLAT_R = TOP_HALF - TOP_EDGE_R                # where the top fillet starts


def _rrect(half, r, z):
    """Rounded square wire, outer half size `half`, corner radius r, at height z."""
    f = Part.makePlane(2 * (half - r), 2 * (half - r), V(-(half - r), -(half - r), z))
    return f.makeOffset2D(r).OuterWire


def _cross(arm, th, z):
    a, t = arm / 2.0, th / 2.0
    pts = [(a, t), (t, t), (t, a), (-t, a), (-t, t), (-a, t),
           (-a, -t), (-t, -t), (-t, -a), (t, -a), (t, -t), (a, -t), (a, t)]
    return Part.makePolygon([V(x, y, z) for x, y in pts])


def _half_at(z):
    return SKIRT_HALF + (TOP_HALF - SKIRT_HALF) * (z - Z_BASE) / (TOP_Z - Z_BASE)


def make_keycap(variant="deny"):
    variant = variant.lower()
    # outer body with rounded corners and rounded top edge
    outer = Part.makeLoft([_rrect(SKIRT_HALF, CORNER_R, Z_BASE),
                           _rrect(TOP_HALF, CORNER_R, TOP_Z)], True, True)
    top_edges = [e for e in outer.Edges if all(abs(v.Point.z - TOP_Z) < 1e-6 for v in e.Vertexes)
                 and abs(e.BoundBox.ZMax - TOP_Z) < 1e-6 and abs(e.BoundBox.ZMin - TOP_Z) < 1e-6]
    try:
        outer = outer.makeFillet(TOP_EDGE_R, top_edges)
    except Exception:
        outer = outer.makeChamfer(TOP_EDGE_R, top_edges)

    # cavity: wall parallel to the outer skirt, 45 deg chamfer into a flat ceiling
    ri = CORNER_R - WALL
    hb = SKIRT_HALF - WALL
    hc = _half_at(CEIL - ROOF_CHAMFER) - WALL
    slope = (hb - hc) / (CEIL - ROOF_CHAMFER - Z_BASE)
    cav = Part.makeLoft([_rrect(hb + slope, ri, Z_BASE - 1.0),
                         _rrect(hc, ri, CEIL - ROOF_CHAMFER),
                         _rrect(hc - ROOF_CHAMFER, ri, CEIL)], True, True)
    cap = outer.cut(cav)

    # stem post with cross socket and lead-in
    post = Part.makeBox(2 * POST_HX, 2 * POST_HY, CEIL - Z_BASE + 0.5, V(-POST_HX, -POST_HY, Z_BASE))
    cap = cap.fuse(post)
    arm, th = STEM_ARM_L + 2 * SOCKET_CLR, STEM_ARM_T + 2 * SOCKET_CLR
    sock = Part.Face(_cross(arm, th, Z_BASE - 0.5)).extrude(V(0, 0, ROOF_Z - Z_BASE + 0.5))
    lead = Part.makeLoft([_cross(arm + 2 * (LEAD_IN + 0.5), th + 2 * (LEAD_IN + 0.5), Z_BASE - 0.5),
                          _cross(arm, th, Z_BASE + LEAD_IN)], True, True)
    cap = cap.cut(sock.fuse(lead))

    # top surface: the same dish on every key, with a raised symbol to feel
    # (single colour print). +Y is toward the display. talk = microphone,
    # approve = tick, deny = cross.
    if variant not in ("deny", "talk", "approve"):
        raise ValueError("variant must be talk, approve or deny")
    r = (FLAT_R ** 2 + DISH ** 2) / (2 * DISH)
    zc = TOP_Z - DISH + r
    cap = cap.cut(Part.makeSphere(r, V(0, 0, zc)))
    z0, h = TOP_Z - DISH - 0.4, 3.0

    def bar(p1, p2, w):
        """Round-ended stroke from p1 to p2, width w, as a tall prism."""
        (x1, y1), (x2, y2) = p1, p2
        L = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
        nx, ny = -(y2 - y1) / L * w / 2, (x2 - x1) / L * w / 2
        poly = Part.makePolygon([V(x1 + nx, y1 + ny, z0), V(x2 + nx, y2 + ny, z0),
                                 V(x2 - nx, y2 - ny, z0), V(x1 - nx, y1 - ny, z0),
                                 V(x1 + nx, y1 + ny, z0)])
        s = Part.Face(poly).extrude(V(0, 0, h))
        for (x, y) in (p1, p2):
            s = s.fuse(Part.makeCylinder(w / 2, h, V(x, y, z0)))
        return s

    if variant == "approve":
        sym = bar((-4.2, -0.4), (-1.4, -3.2), SYM_W).fuse(bar((-1.4, -3.2), (4.2, 3.4), SYM_W))
    elif variant == "deny":
        sym = bar((-3.6, -3.6), (3.6, 3.6), SYM_W).fuse(bar((-3.6, 3.6), (3.6, -3.6), SYM_W))
    else:
        sym = bar((0, 1.6), (0, 4.0), 3.0)                                   # capsule
        ring = Part.makeCylinder(3.5, h, V(0, 1.6, z0)).cut(Part.makeCylinder(2.4, h + 2, V(0, 1.6, z0 - 1)))
        ring = ring.common(Part.makeBox(10, 5, h, V(-5, 1.6 - 5, z0)))        # lower half: cradle
        sym = sym.fuse(ring).fuse(bar((0, -1.9), (0, -4.2), 1.1)).fuse(bar((-2.0, -4.6), (2.0, -4.6), 1.1))
    # keep only SYM_H above the dish surface, following its curve
    sym = sym.cut(Part.makeSphere(r - SYM_H, V(0, 0, zc)))
    cap = cap.fuse(sym)
    cap = cap.removeSplitter()
    return Part.Solid(cap.Solids[0]) if len(cap.Solids) == 1 else cap


VARIANTS = ("talk", "approve", "deny")

if __name__ == "__main__":
    d = App.newDocument("keycap")
    for i, n in enumerate(VARIANTS):
        s = make_keycap(n)
        s.translate(V(20 * (i - 1), 0, 0))
        o = d.addObject("Part::Feature", "keycap_" + n)
        o.Shape = s
        bb = s.BoundBox
        print("%-8s valid=%s solids=%d vol=%.0f bbox %.2f x %.2f, z %.2f..%.2f"
              % (n, s.isValid(), len(s.Solids), s.Volume, bb.XLength, bb.YLength, bb.ZMin, bb.ZMax))
    d.recompute()
