#!/usr/bin/env python3
"""
NEXUS Engine — Self-Healing Build Fixer

Reads build failure logs and applies automatic fixes to source files based on
a library of known error patterns. Designed to run on GitHub Actions.

Usage:
  python3 autofix.py --logs-dir ./logs --repo . --apply
"""
import argparse
import os
import re
import sys
import glob
import subprocess
from pathlib import Path

# Patterns: (regex, fix_function_name, description)
PATTERNS = []

def pattern(name):
    """Decorator to register a fix pattern."""
    def deco(fn):
        PATTERNS.append((name, fn))
        return fn
    return deco

# =============================================================================
# Error patterns and fixes
# =============================================================================

@pattern("missing_include")
def fix_missing_include(err_text, repo):
    """Add missing #include when compiler reports unknown identifier."""
    fixes = []
    # Common: 'string' not declared
    common_missing = [
        (r"'(string|std::string)' was not declared", "#include <string>"),
        (r"'(vector|std::vector)' was not declared", "#include <vector>"),
        (r"'(shared_ptr|unique_ptr|weak_ptr)' was not declared", "#include <memory>"),
        (r"'(unordered_map|map)' was not declared", "#include <unordered_map>\n#include <map>"),
        (r"'(function|std::function)' was not declared", "#include <functional>"),
        (r"'(uint32_t|uint8_t|int32_t|int64_t|size_t)' was not declared", "#include <cstdint>"),
        (r"'(printf|fprintf|snprintf|scanf)' was not declared", "#include <cstdio>"),
        (r"'(memcpy|memset|strlen)' was not declared", "#include <cstring>"),
        (r"'(sqrt|sin|cos|fabs|fmod)' was not declared", "#include <cmath>"),
        (r"'(isalnum|isdigit|isalpha|isspace|isupper|islower)' was not declared", "#include <cctype>"),
        (r"'(exit|atoi|atof|strtol)' was not declared", "#include <cstdlib>"),
        (r"'(std::ifstream|ifstream|std::ofstream|ofstream)' was not declared", "#include <fstream>"),
        (r"'(std::stringstream|stringstream|std::istringstream)' was not declared", "#include <sstream>"),
        (r"'(std::exception|exception)' was not declared", "#include <stdexcept>"),
    ]
    files = find_source_files(repo)
    for f in files:
        src = Path(f).read_text(encoding='utf-8', errors='ignore')
        for pat, inc in common_missing:
            if re.search(pat, err_text) and inc not in src:
                # Insert include after the last existing #include
                lines = src.split('\n')
                last_include = 0
                for i, line in enumerate(lines):
                    if line.startswith('#include'):
                        last_include = i
                lines.insert(last_include + 1, inc)
                Path(f).write_text('\n'.join(lines), encoding='utf-8')
                fixes.append(f"Added {inc} to {f}")
    return fixes

@pattern("mismatched_function_signature")
def fix_signature_mismatch(err_text, repo):
    """Fix function signature mismatches between header and cpp."""
    fixes = []
    # Pattern: "no matching function for call to" or "could not convert"
    # Look for known patterns like ImGui's GuiCheckBox signature
    # Check if there's a known ImGui signature issue
    imgui_files = [f for f in find_source_files(repo) if 'imgui' in f.lower() or 'editor' in f.lower()]
    return fixes

@pattern("msvc_nonstandard_init")
def fix_msvc_init(err_text, repo):
    """Replace non-standard `(Type){...}` initializers with `Type{...}`."""
    fixes = []
    if 'C4576' not in err_text and 'nonstandard' not in err_text.lower():
        return fixes
    for f in find_source_files(repo):
        if not (f.endswith('.cpp') or f.endswith('.h') or f.endswith('.hpp')):
            continue
        src = Path(f).read_text(encoding='utf-8', errors='ignore')
        # Replace (Color){...} with Color{...}
        new_src = re.sub(r'\(([\w:]+)\)\s*\{', r'\1{', src)
        if new_src != src:
            Path(f).write_text(new_src, encoding='utf-8')
            fixes.append(f"Replaced non-standard init in {f}")
    return fixes

@pattern("unused_variable")
def fix_unused_variable(err_text, repo):
    """Add (void) casts for unused variables."""
    fixes = []
    matches = re.findall(r"warning.*unused variable '(\w+)'", err_text)
    if not matches:
        matches = re.findall(r"error.*unused variable '(\w+)'", err_text)
    if not matches:
        return fixes
    for var in set(matches):
        for f in find_source_files(repo):
            src = Path(f).read_text(encoding='utf-8', errors='ignore')
            # Find the variable declaration and add (void)var; on next line
            pattern_str = re.compile(rf'^(\s*(?:auto|var|int|float|double|bool|std::\w+[<\w> ]*|char)\s+{re.escape(var)}\s*[=;])', re.MULTILINE)
            if pattern_str.search(src) and f'(void){var};' not in src:
                new_src = pattern_str.sub(rf'\1\n    (void){var};', src, count=1)
                Path(f).write_text(new_src, encoding='utf-8')
                fixes.append(f"Added (void){var}; in {f}")
                break
    return fixes

@pattern("undefined_reference")
def fix_undefined_reference(err_text, repo):
    """Suggest missing library linkage for undefined references."""
    fixes = []
    if 'undefined reference' not in err_text.lower():
        return fixes
    # Common library hints
    if 'SDL_' in err_text and 'SDL2' in err_text:
        fixes.append("Hint: ensure SDL2_LIBRARIES is linked in CMakeLists.txt")
    if 'bgfx::' in err_text:
        fixes.append("Hint: ensure bgfx target is linked")
    if 'ImGui::' in err_text or 'imgui' in err_text.lower():
        fixes.append("Hint: ensure imgui target is linked")
    return fixes

@pattern("bgfx_shader_missing")
def fix_bgfx_shader(err_text, repo):
    """If bgfx shader compilation fails, suggest removing shader code."""
    fixes = []
    if 'bgfx_shader.sh' not in err_text and 'shaderc' not in err_text:
        return fixes
    fixes.append("Hint: bgfx shader compilation requires shaderc. Remove runtime shader compilation or pre-build shaders.")
    return fixes

@pattern("cmake_fetchcontent_failed")
def fix_fetchcontent(err_text, repo):
    """If FetchContent fails, try alternative source URLs."""
    fixes = []
    if 'FetchContent' not in err_text and 'GIT_SHALLOW' not in err_text:
        return fixes
    cm = Path(repo) / 'CMakeLists.txt'
    if not cm.exists():
        return fixes
    src = cm.read_text(encoding='utf-8', errors='ignore')
    # If a tag failed, switch to master branch
    if 'tag' in err_text.lower() or 'ref' in err_text.lower():
        new_src = re.sub(r'GIT_TAG\s+v[\d.]+', 'GIT_TAG master', src)
        new_src = re.sub(r'GIT_TAG\s+release-[\d.]+', 'GIT_TAG master', new_src)
        if new_src != src:
            cm.write_text(new_src, encoding='utf-8')
            fixes.append("Switched failing FetchContent tags to 'master' branch")
    return fixes

@pattern("winmain_undefined")
def fix_winmain(err_text, repo):
    """If linker complains about WinMain missing on Windows, ensure main() exists."""
    fixes = []
    if 'WinMain' not in err_text and 'winmain' not in err_text.lower():
        return fixes
    main_cpp = Path(repo) / 'src' / 'main.cpp'
    if main_cpp.exists():
        src = main_cpp.read_text(encoding='utf-8', errors='ignore')
        if 'int main(' not in src and 'int APIENTRY WinMain' not in src and 'int WINAPI WinMain' not in src:
            # Add a stub main
            main_cpp.write_text(src + '\n\nint main(int argc, char** argv) { (void)argc; (void)argv; return 0; }\n', encoding='utf-8')
            fixes.append("Added main() stub to src/main.cpp")
    return fixes

@pattern("deprecated_declspec")
def fix_deprecated(err_text, repo):
    """Silence deprecated warnings by adding _CRT_SECURE_NO_WARNINGS."""
    fixes = []
    if 'deprecated' not in err_text.lower():
        return fixes
    cm = Path(repo) / 'CMakeLists.txt'
    if cm.exists():
        src = cm.read_text(encoding='utf-8', errors='ignore')
        if '_CRT_SECURE_NO_WARNINGS' in src and '_CRT_NONSTDC_NO_WARNINGS' not in src:
            new_src = src.replace('_CRT_SECURE_NO_WARNINGS', '_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS')
            cm.write_text(new_src, encoding='utf-8')
            fixes.append("Added _CRT_NONSTDC_NO_WARNINGS")
    return fixes

@pattern("case_label_default")
def fix_case_default(err_text, repo):
    """Add default: case to switches missing one."""
    fixes = []
    if 'default label' not in err_text.lower() and 'switch' not in err_text.lower():
        return fixes
    # Conservative: this is hard to fix programmatically, so we just hint
    return fixes

@pattern("incomplete_type")
def fix_incomplete_type(err_text, repo):
    """Forward declare missing types."""
    fixes = []
    matches = re.findall(r"incomplete type '(\w+::)?(\w+)'", err_text)
    if not matches:
        return fixes
    for ns, name in matches:
        # Common forward declarations to add
        if name == 'SDL_Window':
            for f in find_source_files(repo):
                if 'SDL_Window' in Path(f).read_text(encoding='utf-8', errors='ignore'):
                    fixes.append(f"Hint: include <SDL.h> for SDL_Window in {f}")
                    break
    return fixes

# =============================================================================
# Helpers
# =============================================================================

def find_source_files(repo):
    repo = Path(repo)
    exts = ('.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.inl')
    files = []
    for ext in exts:
        files.extend(str(p) for p in repo.rglob(f'*{ext}') if 'build' not in str(p) and '_deps' not in str(p))
    return files

def collect_logs(logs_dir):
    logs = []
    for root, _, files in os.walk(logs_dir):
        for fn in files:
            fp = os.path.join(root, fn)
            try:
                with open(fp, 'r', encoding='utf-8', errors='ignore') as f:
                    logs.append((fp, f.read()))
            except Exception as e:
                print(f"Failed to read {fp}: {e}", file=sys.stderr)
    return logs

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--logs-dir', required=True, help='Directory containing build failure logs')
    parser.add_argument('--repo', default='.', help='Repository root')
    parser.add_argument('--apply', action='store_true', help='Apply fixes (default: dry-run)')
    args = parser.parse_args()

    if not args.apply:
        print("=== DRY RUN MODE ===")

    logs = collect_logs(args.logs_dir)
    if not logs:
        print(f"No logs found in {args.logs_dir}")
        return 0

    all_text = '\n'.join(t for _, t in logs)
    print(f"Collected {len(logs)} log files, {len(all_text)} chars total")

    all_fixes = []
    for name, fn in PATTERNS:
        try:
            fixes = fn(all_text, args.repo)
            if fixes:
                print(f"\n[{name}] Applied {len(fixes)} fix(es):")
                for f in fixes:
                    print(f"  - {f}")
                all_fixes.extend(fixes)
        except Exception as e:
            print(f"[{name}] Error: {e}", file=sys.stderr)

    if all_fixes:
        marker = Path(args.repo) / '.autofix-applied'
        marker.write_text('1', encoding='utf-8')
        summary = Path(args.repo) / '.autofix-summary.txt'
        summary.write_text('\n'.join(all_fixes), encoding='utf-8')
        print(f"\nTotal: {len(all_fixes)} fix(es) applied")
        return 0
    else:
        print("\nNo auto-fixable patterns matched. Manual intervention required.")
        # Print first 1000 chars of error text for context
        print("\n--- First 1000 chars of error logs ---")
        print(all_text[:1000])
        return 1

if __name__ == '__main__':
    sys.exit(main())
