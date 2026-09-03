import os
from PIL import Image, ImageDraw, ImageFont

WORKSPACE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ICON_PATH = os.path.join(WORKSPACE_DIR, "resources", "images", "icon.png")

def create_wizard_large(output_path):
    # High-DPI 3x resolution (164 * 3 = 492, 314 * 3 = 942)
    # Menghilangkan efek buram/pecah pada layar resolusi tinggi (1080p, 2K, 4K)
    width, height = 492, 942
    img = Image.new("RGBA", (width, height), (17, 22, 31, 255))
    draw = ImageDraw.Draw(img)

    # 1. Subtle smooth dark gradient
    for y in range(height):
        factor = y / height
        r = int(14 + factor * 8)
        g = int(18 + factor * 10)
        b = int(26 + factor * 12)
        draw.line([(0, y), (width, y)], fill=(r, g, b, 255))

    # 2. Sleek left teal accent stripe (#00e0a8)
    for x in range(6):
        draw.line([(x, 0), (x, height)], fill=(0, 224, 168, 255))

    # 3. Render REAL Deskmon App Icon in High-Resolution
    if os.path.exists(ICON_PATH):
        try:
            icon_img = Image.open(ICON_PATH).convert("RGBA")
            # Resize icon with high quality Lanczos filter (220x213 px)
            icon_size = (220, 213)
            icon_resized = icon_img.resize(icon_size, Image.Resampling.LANCZOS)

            # Circular glow plate behind icon
            cx = width // 2
            cy = 280
            r = 135

            # Glowing rings
            draw.ellipse([cx - r - 6, cy - r - 6, cx + r + 6, cy + r + 6],
                         outline=(0, 224, 168, 60), width=3)
            draw.ellipse([cx - r, cy - r, cx + r, cy + r],
                         fill=(24, 32, 44, 255), outline=(0, 224, 168, 255), width=4)

            # Paste crisp icon with alpha blending
            paste_x = (width - icon_size[0]) // 2
            paste_y = cy - (icon_size[1] // 2)
            img.paste(icon_resized, (paste_x, paste_y), icon_resized)
        except Exception as e:
            print(f"Warning loading icon: {e}")

    # 4. Clean, Sharp Typography: Hanya "DESKMON" (Tanpa Activity Monitoring, Tanpa Perusahaan)
    try:
        font_title = ImageFont.truetype("arialbd.ttf", 52)
    except IOError:
        font_title = ImageFont.load_default()

    title_text = "DESKMON"
    bbox = draw.textbbox((0, 0), title_text, font=font_title)
    tw = bbox[2] - bbox[0]
    draw.text(((width - tw) // 2, 465), title_text, font=font_title, fill=(255, 255, 255, 255))

    # Glowing teal accent line under title
    cx = width // 2
    draw.line([(cx - 60, 545), (cx + 60, 545)], fill=(0, 224, 168, 255), width=5)

    # Simpan sebagai 24-bit RGB BMP (format standar Inno Setup)
    rgb_img = img.convert("RGB")
    rgb_img.save(output_path, "BMP")
    print(f"Created crisp high-res wizard large image: {output_path} ({width}x{height})")


def create_wizard_small(output_path):
    # High-DPI 3x resolution (55 * 3 = 165, 165x165 px)
    width, height = 165, 165
    img = Image.new("RGBA", (width, height), (20, 26, 36, 255))
    draw = ImageDraw.Draw(img)

    # Center circle badge with teal border
    cx, cy = width // 2, height // 2
    r = 72
    draw.ellipse([cx - r - 3, cy - r - 3, cx + r + 3, cy + r + 3],
                 outline=(0, 224, 168, 80), width=2)
    draw.ellipse([cx - r, cy - r, cx + r, cy + r],
                 fill=(26, 36, 50, 255), outline=(0, 224, 168, 255), width=4)

    # Embed real high-res app icon
    if os.path.exists(ICON_PATH):
        try:
            icon_img = Image.open(ICON_PATH).convert("RGBA")
            icon_size = (112, 108)
            icon_resized = icon_img.resize(icon_size, Image.Resampling.LANCZOS)
            paste_x = (width - icon_size[0]) // 2
            paste_y = (height - icon_size[1]) // 2
            img.paste(icon_resized, (paste_x, paste_y), icon_resized)
        except Exception as e:
            print(f"Warning loading small icon: {e}")

    rgb_img = img.convert("RGB")
    rgb_img.save(output_path, "BMP")
    print(f"Created crisp high-res wizard small image: {output_path} ({width}x{height})")


if __name__ == "__main__":
    assets_dir = os.path.join(os.path.dirname(__file__), "assets")
    os.makedirs(assets_dir, exist_ok=True)

    large_path = os.path.join(assets_dir, "wizard_large.bmp")
    small_path = os.path.join(assets_dir, "wizard_small.bmp")

    create_wizard_large(large_path)
    create_wizard_small(small_path)
