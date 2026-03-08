import os
import glob
import subprocess
import sys


def main():
    dataset_dir = (
        r"C:\Users\Administrator\Desktop\optimizeroughness\optimizeroughness\dataset"
    )
    out_dir = r"C:\Users\Administrator\Desktop\UnrealEngine\Engine\Plugins\Runtime\PCACompressed\Resources\MLScripts\TestOutput\Batch100_SVD"
    runner_script = r"C:\Users\Administrator\Desktop\UnrealEngine\Engine\Plugins\Runtime\PCACompressed\Resources\MLScripts\pca_runner.py"

    os.makedirs(out_dir, exist_ok=True)

    # Find all base color textures
    search_pattern = os.path.join(
        dataset_dir, "*_B.tga"
    )  # case insensitive typically on windows
    base_color_files = (
        glob.glob(search_pattern)
        + glob.glob(os.path.join(dataset_dir, "*_B.TGA"))
        + glob.glob(os.path.join(dataset_dir, "*_B.png"))
    )

    # Remove duplicates
    base_color_files = list(set(base_color_files))

    valid_groups = []

    for bc_file in base_color_files:
        # Extract prefix, e.g. "ST_CJ大山石001_01H"
        base_name = os.path.basename(bc_file)
        prefix = base_name[: base_name.lower().rfind("_b.")]

        # Check for matching Normal and Roughness
        nor_file_variations = [
            os.path.join(dataset_dir, f"{prefix}_B_Nor.tga"),
            os.path.join(dataset_dir, f"{prefix}_B_nor.tga"),
            os.path.join(dataset_dir, f"{prefix}_B_Nor.TGA"),
            os.path.join(dataset_dir, f"{prefix}_Nor.tga"),
            os.path.join(dataset_dir, f"{prefix}_NOR.tga"),
        ]

        mre_file_variations = [
            os.path.join(dataset_dir, f"{prefix}_B_MRE.tga"),
            os.path.join(dataset_dir, f"{prefix}_B_Mre.tga"),
            os.path.join(dataset_dir, f"{prefix}_B_mre.tga"),
            os.path.join(dataset_dir, f"{prefix}_B_MRE.TGA"),
            os.path.join(dataset_dir, f"{prefix}_MRE.tga"),
        ]

        nor_file = next((f for f in nor_file_variations if os.path.exists(f)), None)
        mre_file = next((f for f in mre_file_variations if os.path.exists(f)), None)

        if nor_file and mre_file:
            valid_groups.append(
                {"prefix": prefix, "B": bc_file, "NOR": nor_file, "MRE": mre_file}
            )

        if len(valid_groups) >= 100:
            break

    print(f"Found {len(valid_groups)} complete texture groups.")

    # Process them
    for i, group in enumerate(valid_groups):
        print(f"[{i + 1}/{len(valid_groups)}] Processing {group['prefix']}...")

        cmd = [
            sys.executable,
            runner_script,
            "--out_dir",
            out_dir,
            "--name",
            group["prefix"],
            "--iters",
            "300",  # Reduced slightly to speed up the batch processing, 300 is usually enough for L1 loss < 0.01 with early stopping
            "--inputs",
            group["B"],
            group["NOR"],
            group["MRE"],
        ]

        # Run process
        try:
            # Capturing output so it doesn't spam too much, just printing standard status
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            # Find the final loss line
            for line in result.stdout.split("\n"):
                if "Done. Loss:" in line or "Target Reached" in line:
                    print("  ->", line.strip())
        except subprocess.CalledProcessError as e:
            print(f"  -> Error processing {group['prefix']}: {e}")
            print(e.stderr)


if __name__ == "__main__":
    main()
