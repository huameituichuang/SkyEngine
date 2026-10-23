#!/bin/bash
set -u
export CMAKE_POLICY_VERSION_MINIMUM=3.5
export PATH="$HOME/code/SkyEngine/.build-shim:$PATH"
ENJ=$HOME/code/SkyEngine
LOG=$ENJ/build_3rd.log
PKGS="taskflow rapidjson vma imgui cxxopts googletest sdl glslang SPIRV-Cross boost"
for p in $PKGS; do
  echo "=== [$(date +%H:%M:%S)] building $p ===" >> $LOG
  ok=0
  for attempt in 1 2 3; do
    python3 $ENJ/python/third_party.py -i $ENJ/3rd/intermediate -o $ENJ/3rd/output -e $ENJ -p MacOS-arm -t $p >> $LOG 2>&1
    rc=$?
    if [ $rc -eq 0 ]; then ok=1; break; fi
    echo "=== attempt $attempt failed for $p rc=$rc, retrying ===" >> $LOG
    # 清掉可能残留的半成品 clone，下次重新克隆
    if [ ! -d "$ENJ/3rd/intermediate/$p/.git" ]; then rm -rf "$ENJ/3rd/intermediate/$p"; fi
    sleep 5
  done
  if [ $ok -ne 1 ]; then echo "=== FAILED $p after 3 attempts ===" >> $LOG; exit 1; fi
  echo "=== [$(date +%H:%M:%S)] done $p ===" >> $LOG
done
echo "ALL_DONE" >> $LOG
