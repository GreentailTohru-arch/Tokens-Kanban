"""Draw the original, horizontally symmetric default avatar (MIT)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops, ImageOps

ROOT = Path(__file__).resolve().parents[1]
SIZE, SCALE = 256, 4
background, ink = '#eff0f0', '#292b3a'
svg = '''<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256" viewBox="0 0 256 256">
<rect width="256" height="256" fill="#eff0f0"/>
<g fill="#292b3a"><circle cx="128" cy="81" r="43"/>
<path d="M24 256v-12a104 104 0 0 1 208 0v12Z"/></g></svg>
'''
image = Image.new('RGB', (SIZE*SCALE, SIZE*SCALE), background)
draw = ImageDraw.Draw(image)
draw.ellipse((85*SCALE, 38*SCALE, 171*SCALE-1, 124*SCALE-1), fill=ink)
draw.ellipse((24*SCALE, 140*SCALE, 232*SCALE-1, 348*SCALE-1), fill=ink)
draw.rectangle((24*SCALE, 244*SCALE, 232*SCALE-1, 256*SCALE-1), fill=ink)
image = image.resize((SIZE, SIZE), Image.Resampling.LANCZOS)
assert ImageChops.difference(image, ImageOps.mirror(image)).getbbox() is None
(ROOT/'assets/images/postcard-default-avatar.svg').write_text(svg, encoding='utf-8')
for path in ('assets/images/postcard-default-avatar.png', 'companion/postcard/default-avatar.png'):
    image.save(ROOT/path)
print('Original centered avatar / exact horizontal symmetry: PASS')
