#!/usr/bin/env python3
"""Tarball-based 3rd-party builder for SkyEngine (github git 不可达时的替代方案).

复刻 python/third_party.py 的产出布局:
  3rd/output/<name>/{include,src,bin,lib/{Debug,Release}}
差异: 用 codeload tarball 代替 git clone; 用 patch -p1 代替 git apply。
"""
import os
import shutil
import subprocess
import sys
import time

ENJ = os.path.expanduser("~/code/SkyEngine")
INTER = os.path.join(ENJ, "3rd", "intermediate")
OUT = os.path.join(ENJ, "3rd", "output")
PATCHES = os.path.join(ENJ, "cmake", "patches")
LOGPATH = os.path.join(ENJ, "build_3rd_tarball.log")

# name, repo, tag, patch, cmake options, source_subdir, header_fallback_dir
PKGS = [
    ("taskflow",    "taskflow/taskflow",                            "v3.7.0",               None,            {"TF_BUILD_TESTS": "OFF", "TF_BUILD_EXAMPLES": "OFF"}, None, "taskflow"),
    ("rapidjson",   "Tencent/rapidjson",                            "v1.1.0",               "rapidjson.patch", {"RAPIDJSON_BUILD_DOC": "OFF", "RAPIDJSON_BUILD_EXAMPLES": "OFF", "RAPIDJSON_BUILD_TESTS": "OFF"}, None, "include"),
    ("vma",         "GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator", "v3.2.1",             None,            {}, None, "include"),
    ("imgui",       "ocornut/imgui",                                "v1.88",                "imgui.patch",   {}, None, None),
    ("cxxopts",     "jarro2783/cxxopts",                            "v3.2.1",               None,            {"CXXOPTS_BUILD_EXAMPLES": "OFF", "CXXOPTS_BUILD_TESTS": "OFF"}, None, "include"),
    ("googletest",  "google/googletest",                            "v1.17.0",              None,            {"INSTALL_GTEST": "ON"}, None, None),
    ("sdl",         "libsdl-org/SDL",                               "release-2.32.6",       None,            {"SDL_SHARED": "OFF", "SDL_STATIC": "ON", "SDL_TEST_LIBRARY": "OFF"}, None, None),
    ("zlib",        "madler/zlib",                                "v1.3.1",               None,            {}, None, None),
    ("stb",         "nothings/stb",                               "master",               "stb.patch",     {}, None, None),
    ("meshoptimizer", "zeux/meshoptimizer",                       "v0.23",                None,            {}, None, None),
    ("GKlib",       "KarypisLab/GKlib",                           "METIS-v5.1.1-DistDGL-0.5", "GKlib.patch", {"CMAKE_POLICY_VERSION_MINIMUM": "3.5"}, None, None),
    ("metis",       "KarypisLab/METIS",                           "v5.2.1",               "metis.patch",   {"CMAKE_POLICY_VERSION_MINIMUM": "3.5"}, None, None),
    ("assimp",      "assimp/assimp",                              "v6.0.2",               None,            {"BUILD_SHARED_LIBS": "ON", "CMAKE_DEBUG_POSTFIX": "", "ASSIMP_BUILD_TESTS": "OFF", "ASSIMP_WARNINGS_AS_ERRORS": "OFF", "ASSIMP_BUILD_USD_IMPORTER": "ON", "ASSIMP_BUILD_ASSIMP_TOOLS": "OFF", "ASSIMP_BUILD_SAMPLES": "OFF", "ASSIMP_INSTALL": "ON", "XCODE_CODE_SIGN_IDENTITY": ""}, None, None),
    ("SPIRV-Cross", "KhronosGroup/SPIRV-Cross",                     "vulkan-sdk-1.4.313.0", None,            {"SPIRV_CROSS_CLI": "OFF", "SPIRV_CROSS_ENABLE_TESTS": "OFF"}, None, None),
]

logf = open(LOGPATH, "a", buffering=1)


def log(msg):
    print(msg, file=logf, flush=True)
    print(msg, flush=True)


def fetch(name, repo, tag):
    dest = os.path.join(INTER, name)
    marker = os.path.join(dest, ".tarball_ok")
    if os.path.isfile(marker):
        log(f"[{name}] already fetched, skip download")
        return
    url = f"https://codeload.github.com/{repo}/tar.gz/{tag}"
    tgz = f"/tmp/sky3rd-{name}.tar.gz"
    for attempt in range(1, 80):
        r = subprocess.run(
            ["curl", "-sL", "-o", tgz, "--max-time", "3600",
             "-w", "http=%{http_code} size=%{size_download} speed=%{speed_download}", url],
            capture_output=True, text=True)
        ok = (r.returncode == 0
              and subprocess.run(["tar", "tzf", tgz], stdout=subprocess.DEVNULL,
                                 stderr=subprocess.DEVNULL).returncode == 0)
        if ok:
            break
        log(f"[{name}] download attempt {attempt} failed: rc={r.returncode} {r.stdout.strip()}")
        time.sleep(3)
    else:
        raise RuntimeError(f"{name}: download failed after retries")
    tmp = dest + ".tmp"
    shutil.rmtree(tmp, ignore_errors=True)
    shutil.rmtree(dest, ignore_errors=True)
    os.makedirs(tmp)
    subprocess.run(["tar", "xzf", tgz, "-C", tmp, "--strip-components", "1"], check=True)
    os.rename(tmp, dest)
    open(marker, "w").write(url + "\n")
    log(f"[{name}] fetched ok: {r.stdout.strip()}")


def apply_patch(name, patch):
    if not patch:
        return
    done = os.path.join(INTER, name, ".patch_applied")
    if os.path.isfile(done):
        return
    with open(os.path.join(PATCHES, patch), "rb") as f:
        subprocess.run(["patch", "-p1", "-f"], stdin=f, cwd=os.path.join(INTER, name),
                       check=True, stdout=logf, stderr=subprocess.STDOUT)
    open(done, "w").write(patch + "\n")
    log(f"[{name}] patch applied")


def copy_install(name, inst, bt):
    for sub in ("include", "src", "bin"):
        s = os.path.join(inst, sub)
        d = os.path.join(OUT, name, sub)
        if os.path.isdir(s):
            shutil.rmtree(d, ignore_errors=True)
            os.makedirs(os.path.dirname(d), exist_ok=True)
            shutil.copytree(s, d)
    s = os.path.join(inst, "lib")
    d = os.path.join(OUT, name, "lib", bt)
    if os.path.isdir(s):
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(os.path.dirname(d), exist_ok=True)
        shutil.copytree(s, d)


def build(name, options, source, header_fallback):
    src = os.path.join(INTER, name)
    if source:
        src = os.path.join(src, source)
    env = dict(os.environ, CMAKE_POLICY_VERSION_MINIMUM="3.5")
    for bt in ("Debug", "Release"):
        b = os.path.join(INTER, name, f"build_MacOS-arm_{bt}")
        inst = os.path.join(b, "install")
        if os.path.isdir(os.path.join(OUT, name, "lib", bt)) or (
                header_fallback and os.path.isdir(os.path.join(OUT, name, "include"))):
            log(f"[{name}] {bt} already installed, skip")
            continue
        cmd = ["cmake", "-S", src, "-B", b, "-G", "Xcode",
               f"-DCMAKE_BUILD_TYPE={bt}", f"-DCMAKE_INSTALL_PREFIX={inst}",
               "-DBUILD_TESTING=OFF",
               f"-D3RD_PATH={OUT}", f"-D3RD_FIND_PATH={ENJ}/cmake/thirdparty"]
        for k, v in options.items():
            cmd.append(f"-D{k}={v}")
        r = subprocess.run(cmd, env=env, stdout=logf, stderr=subprocess.STDOUT)
        if r.returncode != 0 and header_fallback:
            log(f"[{name}] cmake failed, header fallback: copy {header_fallback}")
            d = os.path.join(OUT, name, "include")
            shutil.rmtree(d, ignore_errors=True)
            shutil.copytree(os.path.join(src, header_fallback), d)
            return
        if r.returncode != 0:
            raise RuntimeError(f"{name}: cmake configure failed ({bt})")
        subprocess.run(["cmake", "--build", b, "--config", bt, "--", "-jobs", "12"],
                       check=True, env=env, stdout=logf, stderr=subprocess.STDOUT)
        subprocess.run(["cmake", "--install", b, "--config", bt],
                       check=True, env=env, stdout=logf, stderr=subprocess.STDOUT)
        copy_install(name, inst, bt)
        log(f"[{name}] {bt} built+installed")


def main():
    failed = []
    for name, repo, tag, patch, options, source, hf in PKGS:
        log(f"===== [{time.strftime('%H:%M:%S')}] {name} =====")
        try:
            fetch(name, repo, tag)
            apply_patch(name, patch)
            build(name, options, source, hf)
        except Exception as e:
            log(f"===== {name} FAILED: {e} =====")
            failed.append(name)
            continue
        log(f"===== [{time.strftime('%H:%M:%S')}] {name} done =====")
    if failed:
        log(f"TARBALL_BUILD_FAILED: {failed}")
        sys.exit(1)
    log("TARBALL_BUILD_ALL_DONE")


if __name__ == "__main__":
    main()
