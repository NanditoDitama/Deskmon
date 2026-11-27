import math
import os
from PIL import Image, ImageDraw, ImageFilter, ImageFont

WORKSPACE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ICON_PATH = os.path.join(WORKSPACE_DIR, "resources", "images", "icon.png")

EMERALD = (0, 224, 168)
EMERALD_DEEP = (4, 120, 87)
BLUE_ACCENT = (59, 130, 246)
BG_TOP = (10, 16, 29)
BG_BOTTOM = (16, 27, 46)


def load_font(preferred_names, size):
    font_dirs = [
        os.path.join(os.environ.get("WINDIR", r"C:\Windows"), "Fonts"),
        "/usr/share/fonts",
        "/System/Library/Fonts",
    ]
    for name in preferred_names:
        for base in font_dirs:
            candidate = os.path.join(base, name)
            if os.path.exists(candidate):
                try:
                    return ImageFont.truetype(candidate, size)
                except OSError:
                    pass
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def add_radial_glow(base_img, center, radius, rgb, peak_alpha):
    layer = Image.new("RGBA", base_img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    cx, cy = center
    draw.ellipse([cx - radius, cy - radius, cx + radius, cy + radius], fill=(*rgb, peak_alpha))
    layer = layer.filter(ImageFilter.GaussianBlur(radius=max(8, radius // 2)))
    return Image.alpha_composite(base_img, layer)


def paste_deskmon_icon(canvas, center, target_size):
    if not os.path.exists(ICON_PATH):
        return
    try:
        icon_img = Image.open(ICON_PATH).convert("RGBA")
        # Crop transparent margin if any so the diamond fills evenly
        bbox = icon_img.getbbox()
        if bbox:
            icon_img = icon_img.crop(bbox)
        ratio = min(target_size / icon_img.width, target_size / icon_img.height)
        new_size = (max(1, int(round(icon_img.width * ratio))), max(1, int(round(icon_img.height * ratio))))
        icon_resized = icon_img.resize(new_size, Image.Resampling.LANCZOS)
        paste_x = center[0] - new_size[0] // 2
        paste_y = center[1] - new_size[1] // 2
        canvas.alpha_composite(icon_resized, (paste_x, paste_y))
    except Exception as exc:
        print(f"Warning loading icon: {exc}")


def create_wizard_large(output_path):
    # Render 2x supersampled (984x1884) then downsample to high-DPI Inno Setup size (492x942)
    scale = 2
    out_w, out_h = 492, 942
    w, h = out_w * scale, out_h * scale

    img = Image.new("RGBA", (w, h), (*BG_TOP, 255))
    draw = ImageDraw.Draw(img)

    # 1. Deep slate vertical gradient backdrop
    for y in range(h):
        t = y / float(h - 1)
        r = int(BG_TOP[0] + (BG_BOTTOM[0] - BG_TOP[0]) * t)
        g = int(BG_TOP[1] + (BG_BOTTOM[1] - BG_TOP[1]) * t)
        b = int(BG_TOP[2] + (BG_BOTTOM[2] - BG_TOP[2]) * t)
        draw.line([(0, y), (w, y)], fill=(r, g, b, 255))

    # 2. Subtle ambient aurora glows matching Deskmon palette
    img = add_radial_glow(img, (w // 2, int(h * 0.25)), 240 * scale, EMERALD, 62)
    img = add_radial_glow(img, (int(w * 0.85), int(h * 0.78)), 220 * scale, BLUE_ACCENT, 44)
    img = add_radial_glow(img, (int(w * 0.15), int(h * 0.62)), 160 * scale, EMERALD_DEEP, 36)

    # 3. Subtle architectural dot grid fading softly toward edges
    grid = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    grid_draw = ImageDraw.Draw(grid)
    step = 28 * scale
    for gx in range(step, w, step):
        for gy in range(step, h, step):
            dx = (gx - w * 0.5) / (w * 0.55)
            dy = (gy - h * 0.42) / (h * 0.55)
            fade = max(0.0, 1.0 - math.hypot(dx, dy))
            alpha = int(28 * (fade ** 1.4))
            if alpha > 2:
                r = 1.5 * scale
                grid_draw.ellipse([gx - r, gy - r, gx + r, gy + r], fill=(203, 213, 225, alpha))
    img = Image.alpha_composite(img, grid)

    # 4. Crisp emerald top edge accent bar & hairline right divider
    overlay = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    od = ImageDraw.Draw(overlay)
    od.rectangle([0, 0, w, 5 * scale], fill=(*EMERALD, 230))
    od.rectangle([w - 2 * scale, 0, w, h], fill=(*EMERALD, 55))

    # 5. Glassmorphic squircle plate behind the Deskmon brand mark
    cx = w // 2
    cy = int(h * 0.27)
    card_half = 86 * scale
    plate_box = [cx - card_half, cy - card_half, cx + card_half, cy + card_half]
    od.rounded_rectangle(
        [plate_box[0] - 8 * scale, plate_box[1] - 8 * scale, plate_box[2] + 8 * scale, plate_box[3] + 8 * scale],
        radius=36 * scale,
        fill=(0, 224, 168, 16),
        outline=(0, 224, 168, 55),
        width=2 * scale,
    )
    od.rounded_rectangle(
        plate_box,
        radius=30 * scale,
        fill=(22, 34, 54, 225),
        outline=(255, 255, 255, 38),
        width=2 * scale,
    )
    # Subtle inner highlight line on top of squircle
    od.line(
        [(plate_box[0] + 24 * scale, plate_box[1] + 2 * scale), (plate_box[2] - 24 * scale, plate_box[1] + 2 * scale)],
        fill=(0, 224, 168, 150),
        width=2 * scale,
    )
    img = Image.alpha_composite(img, overlay)

    paste_deskmon_icon(img, (cx, cy), 108 * scale)

    # 6. Typography: "Deskmon" + clean pill subtitle
    font_title = load_font(["segoeuib.ttf", "SegoeUI-Bold.ttf", "arialbd.ttf", "DejaVuSans-Bold.ttf"], 46 * scale)
    font_sub = load_font(["seguisb.ttf", "segoeui.ttf", "arial.ttf", "DejaVuSans.ttf"], 15 * scale)
    font_mini = load_font(["seguisb.ttf", "segoeui.ttf", "arial.ttf", "DejaVuSans.ttf"], 12 * scale)

    td = ImageDraw.Draw(img)
    title_text = "Deskmon"
    tb = td.textbbox((0, 0), title_text, font=font_title)
    tw = tb[2] - tb[0]
    th = tb[3] - tb[1]
    title_y = cy + card_half + 34 * scale
    td.text((cx - tw // 2, title_y), title_text, font=font_title, fill=(248, 250, 252, 255))

    # Pill tag under title
    pill_text = "SETUP WIZARD"
    pb = td.textbbox((0, 0), pill_text, font=font_sub)
    pw = pb[2] - pb[0]
    ph = pb[3] - pb[1]
    pill_pad_x = 18 * scale
    pill_pad_y = 9 * scale
    pill_y = title_y + th + 28 * scale
    pill_h = ph + pill_pad_y * 2
    pill_box = [
        cx - pw // 2 - pill_pad_x,
        pill_y,
        cx + pw // 2 + pill_pad_x,
        pill_y + pill_h,
    ]
    pill_layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    pd = ImageDraw.Draw(pill_layer)
    pd.rounded_rectangle(
        pill_box,
        radius=pill_h // 2,
        fill=(0, 224, 168, 28),
        outline=(0, 224, 168, 120),
        width=int(1.5 * scale),
    )
    img = Image.alpha_composite(img, pill_layer)
    td = ImageDraw.Draw(img)
    td.text(
        (cx, pill_y + pill_h // 2),
        pill_text,
        font=font_sub,
        fill=(110, 247, 212, 255),
        anchor="mm",
    )

    # 7. Modern minimalist translucent dashboard card motif at bottom
    card_layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    cd = ImageDraw.Draw(card_layer)
    m_x = 42 * scale
    m_w = w - m_x * 2
    m_y = int(h * 0.69)
    m_h = 165 * scale
    cd.rounded_rectangle(
        [m_x, m_y, m_x + m_w, m_y + m_h],
        radius=18 * scale,
        fill=(255, 255, 255, 13),
        outline=(255, 255, 255, 28),
        width=int(1.5 * scale),
    )

    # Card header row + live emerald status indicator
    dot_x = m_x + 22 * scale
    dot_y = m_y + 24 * scale
    cd.ellipse([dot_x - 5 * scale, dot_y - 5 * scale, dot_x + 5 * scale, dot_y + 5 * scale], fill=(*EMERALD, 255))
    cd.rounded_rectangle(
        [dot_x + 14 * scale, dot_y - 5 * scale, dot_x + 128 * scale, dot_y + 5 * scale],
        radius=5 * scale,
        fill=(226, 232, 240, 140),
    )

    # Three clean metric progress bars inside motif card
    bars = [
        (0.84, (*EMERALD, 220)),
        (0.62, (*BLUE_ACCENT, 200)),
        (0.45, (148, 163, 184, 150)),
    ]
    bar_x = m_x + 20 * scale
    bar_full_w = m_w - 40 * scale
    for idx, (ratio, col) in enumerate(bars):
        by = m_y + (56 + idx * 30) * scale
        bh = 10 * scale
        cd.rounded_rectangle([bar_x, by, bar_x + bar_full_w, by + bh], radius=5 * scale, fill=(255, 255, 255, 18))
        cd.rounded_rectangle([bar_x, by, bar_x + int(bar_full_w * ratio), by + bh], radius=5 * scale, fill=col)

    img = Image.alpha_composite(img, card_layer)

    # Bottom footer label
    fd = ImageDraw.Draw(img)
    footer_text = "Fast, Lightweight & Cross-Platform Ready"
    fb = fd.textbbox((0, 0), footer_text, font=font_mini)
    fw = fb[2] - fb[0]
    fd.text((cx - fw // 2, h - 42 * scale), footer_text, font=font_mini, fill=(148, 163, 184, 210))

    final_img = img.convert("RGB").resize((out_w, out_h), Image.Resampling.LANCZOS)
    final_img.save(output_path, "BMP")
    print(f"Created modern supersampled wizard large image: {output_path} ({out_w}x{out_h})")


def create_wizard_small(output_path):
    # Render 4x supersampled (660x660) then downsample cleanly to 165x165
    # Matches Inno Setup's clean white modern header background seamlessly
    scale = 4
    out_size = 165
    size = out_size * scale

    img = Image.new("RGBA", (size, size), (255, 255, 255, 255))

    # Soft shadow under app tile
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shadow)
    pad = 16 * scale
    sd.rounded_rectangle(
        [pad, pad + 5 * scale, size - pad, size - pad + 5 * scale],
        radius=32 * scale,
        fill=(15, 23, 42, 48),
    )
    shadow = shadow.filter(ImageFilter.GaussianBlur(radius=8 * scale))
    img = Image.alpha_composite(img, shadow)

    # Crisp dark slate squircle tile with emerald edge accent
    tile = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    td = ImageDraw.Draw(tile)
    td.rounded_rectangle(
        [pad, pad, size - pad, size - pad],
        radius=32 * scale,
        fill=(13, 21, 36, 255),
        outline=(0, 224, 168, 220),
        width=3 * scale,
    )
    # Subtle internal glow
    tile = add_radial_glow(tile, (size // 2, size // 2), 46 * scale, EMERALD, 48)
    img = Image.alpha_composite(img, tile)

    paste_deskmon_icon(img, (size // 2, size // 2), 82 * scale)

    final_img = img.convert("RGB").resize((out_size, out_size), Image.Resampling.LANCZOS)
    final_img.save(output_path, "BMP")
    print(f"Created modern supersampled wizard small image: {output_path} ({out_size}x{out_size})")


if __name__ == "__main__":
    assets_dir = os.path.join(os.path.dirname(__file__), "assets")
    os.makedirs(assets_dir, exist_ok=True)

    large_path = os.path.join(assets_dir, "wizard_large.bmp")
    small_path = os.path.join(assets_dir, "wizard_small.bmp")

    create_wizard_large(large_path)
    create_wizard_small(small_path)
