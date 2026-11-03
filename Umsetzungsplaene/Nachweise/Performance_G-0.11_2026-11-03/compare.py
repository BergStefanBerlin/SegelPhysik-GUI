import csv
import hashlib
import json
import pathlib
import sys

from PIL import Image, ImageChops


before_path, after_path, output_dir = map(pathlib.Path, sys.argv[1:])
before = json.loads(before_path.read_text(encoding="utf-8"))
after = json.loads(after_path.read_text(encoding="utf-8"))
output_dir.mkdir(parents=True, exist_ok=True)
rows = []
comparisons = []
for model in ("420", "folkeboot"):
    for old in before[model]:
        new = next(
            item for item in after[model]
            if (item["views"], item["simple"]) == (old["views"], old["simple"])
        )
        for stage in ("state", "same_profile"):
            initial, final = old[stage]["median_ms"], new[stage]["median_ms"]
            rows.append({
                "model": model, "workspace": "isolated", "auxiliary": old["views"],
                "simple": old["simple"], "stage": stage,
                "before_ms": initial, "after_ms": final,
                "speedup": initial / final if final else None,
                "reduction_percent": (1 - final / initial) * 100 if initial else None,
            })
        for scene in new["scene_work"]:
            if not scene["active"]:
                assert all(scene[field] == 0 for field in ("states", "preparations", "profiles", "renders"))
        assert all(scene["profiles"] == 0 for scene in new["scene_work"])
    for old in before[model + "_window"]:
        new = next(
            item for item in after[model + "_window"]
            if (item["mode"], item["auxiliary"], item["simple"])
            == (old["mode"], old["auxiliary"], old["simple"])
        )
        workspace = 1 if old["mode"] == "static" else 2
        suffix = f".{model}.{workspace}.{int(old['auxiliary'])}.{int(old['simple'])}.bmp"
        old_image_path, new_image_path = pathlib.Path(str(before_path) + suffix), pathlib.Path(str(after_path) + suffix)
        with Image.open(old_image_path) as source, Image.open(new_image_path) as target:
            source, target = source.convert("RGB"), target.convert("RGB")
            assert source.size == target.size
            difference = ImageChops.difference(source, target)
            channels = difference.split()
            maximum = ImageChops.lighter(ImageChops.lighter(channels[0], channels[1]), channels[2])
            changed = source.width * source.height - maximum.histogram()[0]
            comparisons.append({
                "model": model, "workspace": old["mode"],
                "auxiliary": old["auxiliary"], "simple": old["simple"],
                "table_exact": old["table_reference"] == new["table_reference"],
                "image_exact": changed == 0, "changed_pixels": changed,
                "maximum_channel_difference": max(pair[1] for pair in difference.getextrema()),
                "image_size": source.size,
                "before_sha256": hashlib.sha256(old_image_path.read_bytes()).hexdigest(),
                "after_sha256": hashlib.sha256(new_image_path.read_bytes()).hexdigest(),
            })
        for stage in ("display_state", "visible_table", "profile", "force_menu"):
            initial, final = old[stage]["median_ms"], new[stage]["median_ms"]
            rows.append({
                "model": model, "workspace": old["mode"], "auxiliary": old["auxiliary"],
                "simple": old["simple"], "stage": stage,
                "before_ms": initial, "after_ms": final,
                "speedup": initial / final if final else None,
                "reduction_percent": (1 - final / initial) * 100 if initial else None,
            })
with (output_dir / "timing-summary.csv").open("w", newline="", encoding="utf-8") as file:
    writer = csv.DictWriter(file, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
report = {
    "before_revision": before["revision"], "after_revision": after["revision"],
    "input_snapshots": "same frozen G-0.10/C-0.9 snapshots in both executables",
    "images_and_tables": comparisons,
    "all_tables_exact": all(item["table_exact"] for item in comparisons),
    "all_images_exact": all(item["image_exact"] for item in comparisons),
    "inactive_scene_work_zero": True, "unchanged_profiles_zero": True,
    "timings": rows,
}
(output_dir / "comparison.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
for row in rows:
    if row["stage"] == "display_state":
        print(f"{row['model']:9} {row['workspace']:7} aux={int(row['auxiliary'])} "
              f"simple={int(row['simple'])} {row['before_ms']:.3f} -> {row['after_ms']:.3f} ms "
              f"({row['speedup']:.2f}x)")
print("Exact tables:", report["all_tables_exact"], "Exact images:", report["all_images_exact"])
