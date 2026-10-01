#!/usr/bin/env python3
import os
import sys
import argparse
import fnmatch
import subprocess
import tarfile

# Defensive import fallback for older Python versions
try:
    import tomllib
except ImportError:
    try:
        import tomli as tomllib
    except ImportError:
        print("❌ Error: Missing TOML parser library. Please upgrade to Python 3.11+ or install via: pip install tomli")
        sys.exit(1)

print("🚀 Koala Build Script Started...")

def check_native_libraries(pkg_name, natives_list, pkg_dir):
    """Verify if the required native shared libraries (.so) can be found in standard search paths."""
    if not natives_list:
        return

    search_paths = [
        "/usr/lib",
        "/usr/local/lib",
        "/lib",
        os.path.join(pkg_dir, "native")
    ]

    env_ld_path = os.environ.get("LD_LIBRARY_PATH")
    if env_ld_path:
        search_paths.extend(env_ld_path.split(os.pathsep))

    for native_lib in natives_list:
        filename = f"lib{native_lib}.so"
        found = False

        for path in search_paths:
            full_path = os.path.join(path, filename)
            if os.path.exists(full_path):
                print(f"🔍 Found required native library: {full_path}")
                found = True
                break

        if not found:
            print(f"❌ Error: Package [{pkg_name}] requires native library '{filename}' but it cannot be found.")
            sys.exit(1)

def resolve_files(package_dir, files_patterns, excludes_patterns):
    """Filter and return matching source files based on files and excludes patterns."""
    matched_files = set()
    all_files = os.listdir(package_dir)

    for pattern in files_patterns:
        for filename in all_files:
            if fnmatch.fnmatch(filename, pattern):
                matched_files.add(filename)

    final_files = []
    for filename in matched_files:
        should_exclude = False
        for ext_pattern in excludes_patterns:
            if fnmatch.fnmatch(filename, ext_pattern):
                should_exclude = True
                break
        if not should_exclude:
            final_files.append(os.path.join(package_dir, filename))

    return sorted(final_files)

def load_workspace(ws_dir):
    """Load and parse __ws__.toml and all subpackage __pkg__.toml files."""
    ws_toml = os.path.join(ws_dir, "__ws__.toml")
    if not os.path.exists(ws_toml):
        print(f"❌ Error: Workspace config file not found: {ws_toml}")
        sys.exit(1)

    with open(ws_toml, "rb") as f:
        ws_data = tomllib.load(f)["workspace"]

    packages_meta = {}

    for pkg_dir_name in ws_data.get("packages", []):
        pkg_dir = os.path.join(ws_dir, pkg_dir_name)
        pkg_toml = os.path.join(pkg_dir, "__pkg__.toml")

        if not os.path.exists(pkg_toml):
            continue

        with open(pkg_toml, "rb") as f:
            pkg_data = tomllib.load(f)["package"]

        pkg_name = pkg_data["name"]
        packages_meta[pkg_name] = {
            "dir_name": pkg_dir_name,
            "dir_path": pkg_dir,
            "version": pkg_data.get("version", "0.1.0"),
            "files": pkg_data.get("files", ["*.kl"]),
            "excludes": pkg_data.get("excludes", []),
            "dependencies": pkg_data.get("dependencies", []),
            "natives": pkg_data.get("natives", []),
            "compiler": pkg_data.get("compiler", None),
            "options": pkg_data.get("options", [])
        }

    return ws_data, packages_meta

def topological_sort(packages):
    """Perform topological sort to determine correct compilation order based on dependencies."""
    order = []
    visited = {name: 0 for name in packages}

    def visit(name):
        if name not in packages:
            return
        if visited[name] == 1:
            print(f"❌ Error: Circular dependency detected involving component: {name}")
            sys.exit(1)
        if visited[name] == 0:
            visited[name] = 1
            for dep in packages[name]["dependencies"]:
                visit(dep)
            visited[name] = 2
            order.append(name)

    for name in packages:
        if visited[name] == 0:
            visit(name)

    return order

def execute_compile_workspace(ws_dir, output_dir):
    """Actively run the compiler and dynamically manage KOALA_PATH environment variable."""
    ws_data, packages = load_workspace(ws_dir)
    print(f"🏗  Building Workspace: {ws_data.get('name', 'unknown')} (v{ws_data.get('version', '0.1.0')})")

    # Defensive trailing slash patch discovered from GDB debugging
    abs_output_dir = os.path.abspath(output_dir)
    if not abs_output_dir.endswith(os.sep):
        abs_output_dir += os.sep

    print("\n📦 Subpackage Dependency Mapping:")
    for pkg_name, meta in packages.items():
        deps_str = ", ".join(meta['dependencies']) if meta['dependencies'] else 'None'
        print(f"  ▪ [{pkg_name}] -> Dependencies: {deps_str}")

    compile_order = topological_sort(packages)
    print(f"\n📋 Calculated Compilation Order:\n  {' -> '.join([packages[p]['dir_name'] for p in compile_order])}")

    env = os.environ.copy()
    current_koala_path = env.get("KOALA_PATH", "")
    env["KOALA_PATH"] = f"{abs_output_dir}{os.pathsep}{current_koala_path}" if current_koala_path else abs_output_dir
    print(f"🌐 Dynamic KOALA_PATH set to: {env['KOALA_PATH']}\n")

    package_src_mappings = {}
    package_klc_paths = {} # To keep track of exactly generated artifacts per package name

    for pkg_name in compile_order:
        pkg = packages[pkg_name]
        check_native_libraries(pkg_name, pkg["natives"], pkg["dir_path"])

        src_files = resolve_files(pkg["dir_path"], pkg["files"], pkg["excludes"])
        if not src_files:
            continue

        package_src_mappings[pkg["dir_name"]] = src_files

        # 🛠️ ZERO HARDCODING FIX:
        # The structure directory path is determined purely by the package name mapping!
        # e.g., name = "std/io" -> output_klc_path = "dist/std/io.klc"
        # e.g., name = "std/xxx/yyy" -> output_klc_path = "dist/std/xxx/yyy.klc"
        output_klc_path = os.path.join(abs_output_dir, f"{pkg_name}.klc")
        package_klc_paths[pkg["dir_name"]] = f"{pkg_name}.klc"

        # Ensure deep namespace subfolders exist dynamically
        os.makedirs(os.path.dirname(output_klc_path), exist_ok=True)

        cmd = []
        if pkg["compiler"]:
            cmd.append(pkg["compiler"])
            if pkg["options"]:
                cmd.extend(pkg["options"])
            cmd.extend(["-o", output_klc_path])
        else:
            cmd = ["koala", "-c", "-o", output_klc_path]
            if pkg["options"]:
                cmd.extend(pkg["options"])

        base_dir = os.path.dirname(os.path.abspath(ws_dir))
        cmd.append(os.path.relpath(pkg["dir_path"], base_dir))
        cmd.append(f"--package-name={pkg_name}")

        print(f"🚀 Running: {' '.join(cmd)}")
        try:
            result = subprocess.run(cmd, check=True, capture_output=True, text=True, env=env)
            if result.stdout:
                print(result.stdout.strip())
        except subprocess.CalledProcessError as e:
            print(f"❌ Compilation failed for [{pkg_name}]:\n{e.stderr}")
            sys.exit(1)

        print(f"✅ Generated artifact: {output_klc_path}\n")

    return ws_data, package_src_mappings, package_klc_paths

def pack_workspace_combined(ws_dir, output_dir, ws_data, package_src_mappings, package_klc_paths):
    """Bundle __ws__.toml, subpackage sources, and generated .klc artifacts into a workspace named tarball."""
    # 🛠️ AUTOMATIC NAMING FIX: Target tarball name is derived purely from workspace.name
    tar_filename = f"{ws_data['name']}.tar"
    tar_path = os.path.abspath(os.path.join(output_dir, "..", tar_filename))

    print(f"📦 Packaging BOTH sources and compiled artifacts into: {tar_path}...")
    ws_toml = os.path.join(ws_dir, "__ws__.toml")

    abs_output_dir = os.path.abspath(output_dir)
    if not abs_output_dir.endswith(os.sep):
        abs_output_dir += os.sep

    with tarfile.open(tar_path, "w") as tar:
        tar.add(ws_toml, arcname="__ws__.toml")

        for dir_name, src_files in package_src_mappings.items():
            pkg_toml = os.path.join(ws_dir, dir_name, "__pkg__.toml")
            if os.path.exists(pkg_toml):
                tar.add(pkg_toml, arcname=os.path.join(dir_name, "__pkg__.toml"))

            for src_file in src_files:
                tar.add(src_file, arcname=os.path.relpath(src_file, ws_dir))

            # Grab the structural namespaced .klc and store it with its exact layout position
            klc_relative_layout = package_klc_paths.get(dir_name)
            if klc_relative_layout:
                klc_physical_path = os.path.join(abs_output_dir, klc_relative_layout)
                if os.path.exists(klc_physical_path):
                    tar.add(klc_physical_path, arcname=klc_relative_layout)

    print(f"✨ Combined source and binary tarball [{tar_filename}] generation completed successfully.")

def main():
    parser = argparse.ArgumentParser(description="Koala Language Workspace Production Zero-Config Compiler & Packager")
    parser.add_argument("--src", required=True, help="Path to workspace root")
    # 🛠️ DEFAULTS FALLBACK: Make output folder completely optional, default to local './dist'
    parser.add_argument("--output", default="./dist", help="Directory for compiled .klc files (default: ./dist)")

    args = parser.parse_args()

    try:
        ws_data, src_mappings, klc_paths = execute_compile_workspace(args.src, args.output)
        # Always run packaging automatically using workspace metadata definitions
        pack_workspace_combined(args.src, args.output, ws_data, src_mappings, klc_paths)
    except Exception as e:
        print(f"❌ Fatal Runtime Exception: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
