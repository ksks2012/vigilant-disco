import csv, json, os

csv_path = "./Bead List with RGB Values - Bead List.csv"
out_dir = "./etc/palettes"

# Read CSV
rows_by_brand = {"Hama": [], "Perler": [], "Artkal": [], "Nabbi": []}
with open(csv_path) as f:
    reader = csv.reader(f)
    next(reader)  # skip header
    for row in reader:
        if len(row) <= 20 or not row[20]:
            continue
        brand = row[20]
        if brand not in rows_by_brand:
            continue
        code = row[1].strip()
        name = row[0].strip()
        r_str, g_str, b_str = row[2].strip(), row[3].strip(), row[4].strip()
        
        # Skip entries with missing RGB or discontinued/unknown codes
        if not r_str or not g_str or not b_str:
            continue
        if code.startswith('x') or code.startswith('u'):
            continue
        
        try:
            r, g, b = int(r_str), int(g_str), int(b_str)
        except ValueError:
            continue
        
        rows_by_brand[brand].append({
            "code": code,
            "name": name,
            "r": r, "g": g, "b": b
        })

# Brand metadata
brand_meta = {
    "Hama":   {"size": "Midi (5mm)"},
    "Perler": {"size": "Midi (5mm)"},
    "Artkal": {"size": "S-Series (5mm)"},
    "Nabbi":  {"size": "Standard (5mm)"},
}

for brand, entries in rows_by_brand.items():
    palette = []
    for e in entries:
        display_name = f"{e['code']} {e['name']}" if e['name'] else e['code']
        palette.append({
            "code": e['code'],
            "name": display_name,
            "color": [e['r'], e['g'], e['b']]
        })
    
    obj = {
        "brand": brand,
        "size": brand_meta[brand]["size"],
        "palette": palette
    }
    
    out_path = os.path.join(out_dir, f"{brand.lower()}.json")
    with open(out_path, 'w') as f:
        json.dump(obj, f, indent=4, ensure_ascii=False)
    
    print(f"{brand}: {len(palette)} colours -> {out_path}")
