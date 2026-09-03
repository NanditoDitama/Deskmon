#!/usr/bin/env python3
"""
Deskmon - I18n Auto-Sync & Auto-Translator Script
==================================================
Script ini secara otomatis:
1. Memindai (scan) seluruh file *.qml di folder qml/.
2. Mencari semua panggilan I18n.t("...").
3. Membaca kamus I18n.qml yang sudah ada.
4. Mendeteksi teks baru yang belum ada di kamus.
5. Menerjemahkan teks baru tersebut secara otomatis ke Bahasa Indonesia menggunakan Google Translate API (tanpa library eksternal).
6. Memperbarui file qml/theme/Lang.qml secara otomatis!
"""

import os
import re
import json
import urllib.request
import urllib.parse

REPO_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QML_DIR = os.path.join(REPO_DIR, "qml")
I18N_FILE = os.path.join(QML_DIR, "theme", "Lang.qml")

def translate_text(text, source_lang="en", target_lang="id"):
    """Menerjemahkan teks via Google Translate API (gratis, tanpa API key/pip)."""
    try:
        url = (
            "https://translate.googleapis.com/translate_a/single?client=gtx&sl="
            + source_lang
            + "&tl="
            + target_lang
            + "&dt=t&q="
            + urllib.parse.quote(text)
        )
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=8) as response:
            data = json.loads(response.read().decode("utf-8"))
            translated = "".join([part[0] for part in data[0] if part[0]])
            return translated
    except Exception as e:
        print(f"  [WARN] Gagal menerjemahkan '{text}': {e}. Menggunakan fallback teks asli.")
        return text

def scan_qml_for_i18n_keys(qml_dir):
    """Memindai seluruh file QML untuk menemukan semua kunci I18n.t()."""
    pattern = re.compile(r'(?:I18n|Lang)\.(?:t|tr)\(\s*["\']([^"\']+)["\'](?:\s*,\s*["\']([^"\']+)["\'])?')
    found_keys = {} # key -> fallback

    for root, _, files in os.walk(qml_dir):
        for f in files:
            if f.endswith(".qml") and f != "Lang.qml":
                file_path = os.path.join(root, f)
                with open(file_path, "r", encoding="utf-8", errors="ignore") as file:
                    content = file.read()
                    # Bersihkan komentar
                    clean = re.sub(r'//.*', '', content)
                    clean = re.sub(r'/\*.*?\*/', '', clean, flags=re.DOTALL)
                    matches = pattern.findall(clean)
                    for key, fallback in matches:
                        if key not in found_keys:
                            found_keys[key] = fallback if fallback else key

    return found_keys

def parse_existing_i18n(file_path):
    """Membaca kamus teks 'en' dan 'id' yang sudah ada di I18n.qml."""
    if not os.path.exists(file_path):
        return {}, {}

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Ekstrak blok "en": { ... } dan "id": { ... }
    en_dict = {}
    id_dict = {}

    entry_pattern = re.compile(r'["\']([^"\']+)["\']\s*:\s*["\']([^"\']*)["\']')

    # Cari blok "en": { ... }
    en_match = re.search(r'["\']en["\']\s*:\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}', content)
    if en_match:
        for k, v in entry_pattern.findall(en_match.group(1)):
            en_dict[k] = v

    # Cari blok "id": { ... }
    id_match = re.search(r'["\']id["\']\s*:\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}', content)
    if id_match:
        for k, v in entry_pattern.findall(id_match.group(1)):
            id_dict[k] = v

    return en_dict, id_dict

def sync_and_translate():
    print("=" * 60)
    print("  DESKMON I18N AUTO-SYNC & AUTO-TRANSLATOR")
    print("=" * 60)

    print(f"1. Memindai file QML di: {QML_DIR}")
    qml_keys = scan_qml_for_i18n_keys(QML_DIR)
    print(f"   Ditemukan {len(qml_keys)} pemanggilan I18n.t() di seluruh QML.")

    print(f"2. Membaca kamus yang ada di: {I18N_FILE}")
    en_dict, id_dict = parse_existing_i18n(I18N_FILE)
    print(f"   Terdaftar saat ini: {len(en_dict)} kunci 'en', {len(id_dict)} kunci 'id'.")

    # Deteksi kunci baru
    new_keys = []
    for k, default_val in qml_keys.items():
        if k not in en_dict or k not in id_dict:
            new_keys.append((k, default_val))

    if not new_keys:
        print("\n--> SEMUA TEKS SUDAH TERSINKRONISASI! Tidak ada kunci baru.")
        print("=" * 60)
        return

    print(f"\n3. Ditemukan {len(new_keys)} KUNCI BARU. Memulai auto-translasi ke Bahasa Indonesia...")
    for k, default_val in new_keys:
        # Isi bahasa Inggris
        if k not in en_dict:
            en_dict[k] = default_val

        # Auto-translate bahasa Indonesia jika belum ada
        if k not in id_dict:
            translated = translate_text(default_val, source_lang="en", target_lang="id")
            id_dict[k] = translated
            print(f"   [+ BARU] \"{k}\" -> \"{translated}\"")

    # Tulis ulang I18n.qml dengan rapi
    print(f"\n4. Menyimpan pembaruan ke {I18N_FILE}...")
    generate_i18n_file(I18N_FILE, en_dict, id_dict)
    print("--> SUKSES! Lang.qml telah berhasil diperbarui dan siap digunakan!")
    print("=" * 60)

def generate_i18n_file(output_path, en_dict, id_dict):
    """Menulis file I18n.qml dengan format rapi dan terstruktur."""
    en_lines = []
    for k in sorted(en_dict.keys(), key=lambda s: s.lower()):
        val = en_dict[k].replace('"', '\\"')
        en_lines.append(f'            "{k}": "{val}",')

    id_lines = []
    for k in sorted(id_dict.keys(), key=lambda s: s.lower()):
        val = id_dict[k].replace('"', '\\"')
        id_lines.append(f'            "{k}": "{val}",')

    en_content = "\n".join(en_lines).rstrip(",")
    id_content = "\n".join(id_lines).rstrip(",")

    qml_content = f'''pragma Singleton
import QtQuick

QtObject {{
    id: i18n

    // Bahasa aktif saat ini. Default: "en" (English)
    property string currentLanguage: "en"

    readonly property bool isEnglish: currentLanguage === "en"
    readonly property bool isIndonesian: currentLanguage === "id"

    // Daftar bahasa yang didukung
    readonly property var availableLanguages: [
        {{ code: "en", name: "English", flag: "🇺🇸" }},
        {{ code: "id", name: "Bahasa Indonesia", flag: "🇮🇩" }}
    ]

    // Kamus terpusat teks multi-bahasa (Otomatis disinkronkan oleh scripts/sync_i18n.py)
    readonly property var strings: ({{
        "en": {{
{en_content}
        }},
        "id": {{
{id_content}
        }}
    }})

    /**
     * Fungsi penerjemah: I18n.t("key", "Default Fallback")
     */
    function t(key, fallback) {{
        var langDict = strings[currentLanguage];
        if (langDict && langDict[key] !== undefined) {{
            return langDict[key];
        }}
        var enDict = strings["en"];
        if (enDict && enDict[key] !== undefined) {{
            return enDict[key];
        }}
        return fallback !== undefined ? fallback : key;
    }}

    /**
     * Alias singkat agar mudah diketik: Lang.tr("Logout")
     */
    function tr(key, fallback) {{
        return t(key, fallback);
    }}

    /**
     * Mengubah bahasa aktif ("en" atau "id")
     */
    function setLanguage(code) {{
        if (strings[code]) {{
            currentLanguage = code;
        }}
    }}
}}
'''

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(qml_content)

if __name__ == "__main__":
    sync_and_translate()
