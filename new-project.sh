#!/bin/bash
# ============================================================================
#  new-project.sh —— 从 00-demo 复制一个新 Pico 2 项目
#
#  用法：
#      ./new-project.sh 02-servo
#      ./new-project.sh --list          看现有项目
#
#  设计（为了不增加多余的东西）：
#      直接拿 00-demo 当模板，不额外维护 .template/ 目录。
#      所以想让新项目默认带什么设置，改 00-demo 就行 —— 只有一个地方要同步。
# ============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$ROOT/00-demo"                     # 模板 = 00-demo
ENV_SH="$HOME/.pico-sdk/env.sh"
NAME="${1:-}"

C_OK=$'\033[32m'; C_ERR=$'\033[31m'; C_WARN=$'\033[33m'; C_0=$'\033[0m'
ok()   { echo "${C_OK}✅${C_0} $*"; }
err()  { echo "${C_ERR}❌${C_0} $*" >&2; }
warn() { echo "${C_WARN}⚠${C_0}  $*"; }

if [ "$NAME" = "--list" ] || [ "$NAME" = "-l" ]; then
    echo "现有项目（$ROOT）:"
    for d in "$ROOT"/*/; do
        [ -f "$d/CMakeLists.txt" ] || continue
        b=$(basename "$d")
        u=$(ls "$d"/build/*.uf2 2>/dev/null | head -1)
        printf "  %-16s %s\n" "$b" "${u:+已编译（$(stat -c%s "$u") 字节）}"
    done
    exit 0
fi

if [ -z "$NAME" ]; then
    echo "用法: $0 <项目名>"
    echo "例:   $0 02-servo"
    echo "      $0 --list"
    exit 1
fi

if ! echo "$NAME" | grep -qE '^[A-Za-z0-9][A-Za-z0-9_.-]*$'; then
    err "项目名只能用字母/数字/下划线/点/横线"
    exit 1
fi

if [ ! -f "$SRC/CMakeLists.txt" ]; then
    err "模板项目不存在: $SRC"
    err "（这个脚本以 00-demo 为模板，别删它）"
    exit 1
fi

DST="$ROOT/$NAME"
if [ -e "$DST" ]; then
    err "'$NAME' 已经存在了"
    exit 1
fi

OLD="$(grep -oE '^project\([^ )]+' "$SRC/CMakeLists.txt" | head -1 | sed 's/^project(//')"
echo "从 $OLD 复制出 $NAME ..."

# ── 复制（排除 build/）
mkdir -p "$DST"
tar -C "$SRC" --exclude=./build -cf - . | tar -C "$DST" -xf -

# ── 换掉 CMakeLists.txt 里的项目名（9 处）
python3 - "$DST/CMakeLists.txt" "$OLD" "$NAME" <<'PY'
import re, sys
p, old, new = sys.argv[1], sys.argv[2], sys.argv[3]
t = open(p, encoding="utf-8").read()
t = re.sub(r'(?<![A-Za-z0-9_])' + re.escape(old) + r'(?![A-Za-z0-9_])', new, t)
open(p, "w", encoding="utf-8").write(t)
PY

# ── 先配置一次 CMake，生成 build/compile_commands.json
#    扩展生成的 settings.json 里 cmake.configureOnOpen=false，
#    不配置的话 C/C++ 扩展会报「找不到此文件的编译信息」，补全也用不了。
echo "配置 CMake ..."
# 显式指定 Release：pico-sdk 不指定时也是 Release，
# 但扩展新建项目时会写成 Debug，显式写死可避免两个项目不一致。
# 想在 VS Code 里切换：Ctrl+Shift+P → Switch Build Type
if ( source "$ENV_SH" 2>/dev/null || true; cd "$DST"; \
     cmake -B build -G Ninja -DPICO_BOARD=pico2 -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1 ); then
    ok "已生成 build/compile_commands.json"
else
    warn "CMake 配置失败 —— 在 VS Code 里跑一次 'Compile Pico Project' 即可"
fi

echo
ok "建好了: ${DST/#$HOME/~}"
echo "   main.c           改这个（stdio 已开，printf 能直接看到）"
echo "   CMakeLists.txt   加新 .c 文件时才需要改"
echo
echo "打开:  code \"$DST\""
echo "编译:  Ctrl+Shift+P → Compile Pico Project"
