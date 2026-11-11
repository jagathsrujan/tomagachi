"""Converts the host preview's PPM frames to 240x240 PNGs with a circular mask, and builds
contact sheets (2x) so a whole set of screens can be inspected at once."""
import glob, os, sys
from PIL import Image, ImageDraw, ImageFont

src, dst = sys.argv[1], sys.argv[2]
mask = Image.new("L", (240, 240), 0)
ImageDraw.Draw(mask).ellipse((0, 0, 239, 239), fill=255)
names = []
for path in sorted(glob.glob(os.path.join(src, "*.ppm"))):
    name = os.path.splitext(os.path.basename(path))[0]
    img = Image.open(path).convert("RGB")
    out = Image.new("RGB", (240, 240), (28, 28, 28))
    out.paste(img, (0, 0), mask)
    out.save(os.path.join(dst, name + ".png"))
    names.append(name)

def sheet(group, title, cols=4):
    items = [n for n in names if n[:2] in group]
    if not items:
        return
    cell = 250
    rows = (len(items) + cols - 1) // cols
    canvas = Image.new("RGB", (cols * cell, rows * (cell + 14)), (28, 28, 28))
    draw = ImageDraw.Draw(canvas)
    for i, n in enumerate(items):
        im = Image.open(os.path.join(dst, n + ".png"))
        x, y = (i % cols) * cell + 5, (i // cols) * (cell + 14) + 2
        canvas.paste(im, (x, y + 12))
        draw.text((x, y), n, fill=(200, 200, 200))
    canvas = canvas.resize((canvas.width * 2 // 1, canvas.height * 2 // 1), Image.NEAREST) if False else canvas
    canvas.save(os.path.join(dst, title))

sheet({"01", "02", "03", "04", "05", "06"}, "sheet_1_status.png")
sheet({"07", "08", "09"}, "sheet_2_approvals.png")
sheet({"10", "11", "12", "13", "14", "15", "16", "17"}, "sheet_3_notices_results.png")
print(len(names), "screens written to", dst)
