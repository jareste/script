#!/usr/bin/env bash
###############################################################################
# Config
###############################################################################
# SCRIPT="script"
SCRIPT="./ft_script"
QUIET="-q"
export SCRIPT QUIET

# Colores
GREEN='\033[0;32m'
RED='\033[0;31m'
YEL='\033[0;33m'
NC='\033[0m'

pass=0
fail=0
TMPROOT="$(mktemp -d -t ft_script_tests.XXXXXX)"
trap 'rm -rf "$TMPROOT"' EXIT

###############################################################################
# Helpers
###############################################################################
norm() {
    sed -E '/^Script (started|done).*/d; s/pid=[0-9]+/pid=PID/g'
}

ok()   { printf "${GREEN}[OK]${NC}   %s\n" "$1"; ((pass++)); }
bad()  { printf "${RED}[FAIL]${NC} %s\n" "$1"; ((fail++)); }
info() { printf "${YEL}[INFO]${NC} %s\n" "$1"; }

run_cmd() {
    bash -c "$1"
}

need_expect() {
    if ! command -v expect >/dev/null 2>&1; then
        info "Missing 'expect', Install it for executing interactive tests (apt-get install expect, brew install expect, etc.)"
        return 1
    fi
}

mkout() {
    local name="$1"
    local dir
    dir="$(mktemp -d "$TMPROOT/$name.XXXX")"
    printf "%s\n" "$dir"
}

###############################################################################
# Tests
###############################################################################
test_echo_always() {
    local dir out rc
    dir="$(mkout echo_always)"
    out="$dir/out.log"

    need_expect || return 0
    OUTPATH="$out" expect <<'EOF' > /dev/null
        set timeout 10
        set SCRIPT $env(SCRIPT)
        set QUIET  $env(QUIET)
        set OUT    $env(OUTPATH)
        spawn {*}$SCRIPT $QUIET -E always -O $OUT -c {printf ABC\n}
        expect eof
EOF
    rc=$?
    if [[ $rc -ne 0 ]]; then bad "echo_always: rc=$rc"; return; fi
    if norm <"$out" | grep -q "ABC"; then ok "echo_always"; else bad "echo_always: didn't capture ABC"; fi
}

test_basic_command_output() {
    local dir out
    dir="$(mkout basic)"
    out="$dir/basic.log"
    if bash -c "$SCRIPT -c 'echo hello world' $QUIET -O '$out'"; then
        if norm <"$out" | grep -q "hello world"; then ok "basic_command_output"; else bad "basic_command_output: missing text"; fi
    else
        bad "basic_command_output: rc!=0"
    fi
}

test_interactive_mode() {
    local dir out
    dir="$(mkout interactive)"
    out="$dir/interactive.log"

    need_expect || return 0
    OUTPATH="$out" expect <<'EOF' >/dev/null
        set timeout 10
        set SCRIPT $env(SCRIPT)
        set QUIET  $env(QUIET)
        set OUT    $env(OUTPATH)
        spawn {*}$SCRIPT $QUIET -O $OUT
        expect -re {.*}
        send "echo test123\r"
        send "exit\r"
        expect eof
EOF
     if norm <"$out" | grep -q "test123"; then ok "interactive_mode"; else bad "interactive_mode: didn't capture 'test123'"; fi
}

test_error_commands() {
    local dir out
    dir="$(mkout errors)"
    out="$dir/error.log"
    bash -c "$SCRIPT $QUIET -O '$out' -c 'ls /nonexistent 2>&1'"
    if norm <"$out" | grep -Eq "No such file|cannot access"; then ok "error_commands"; else bad "error_commands"; fi
}

test_exit_codes() {
    local dir out
    dir="$(mkout exit_code)"
    out="$dir/exit.log"
    if bash -c "$SCRIPT $QUIET -O '$out' -c 'exit 42'"; then
        ok "exit_codes (aceptando rc==0)"
    else
        bad "exit_codes: rc!=0"
    fi
}

test_signal_handling() {
    local dir out
    dir="$(mkout signal)"
    out="$dir/signal.log"

    need_expect || return 0
    OUTPATH="$out" expect <<'EOF' >/dev/null
        set timeout 0.1
        set SCRIPT $env(SCRIPT)
        set QUIET  $env(QUIET)
        set OUT    $env(OUTPATH)
        spawn {*}$SCRIPT $QUIET -O $OUT
        expect -re {.*}
        send "cat\r"
        after 25
        send \003
        send "exit\r"
        expect eof
EOF
      ok "signal_handling (Ctrl+C doesn't block)"
}

test_backspace_handling() {
    local dir out
    dir="$(mkout backspace)"
    out="$dir/backspace.log"

    need_expect || return 0
    OUTPATH="$out" expect <<'EOF' >/dev/null
        set timeout 10
        set SCRIPT $env(SCRIPT)
        set QUIET  $env(QUIET)
        set OUT    $env(OUTPATH)
        spawn {*}$SCRIPT $QUIET -O $OUT
        expect -re {.*}
        send "hello"
        send "\b\b"
        send "p"
        send "\r"
        send "exit\r"
        expect eof
EOF
    ok "backspace_handling"
}

test_timing_file() {
    local dir out timing
    dir="$(mkout timing)"
    out="$dir/session.log"
    timing="$dir/timing.log"
    if bash -c "$SCRIPT $QUIET -O '$out' -T '$timing' -c 'echo test'"; then
        if [[ -s "$timing" ]]; then ok "timing_file"; else bad "timing_file: timing empty"; fi
    else
        bad "timing_file: rc!=0"
    fi
}

test_append_mode() {
    local dir out first_size second_size
    dir="$(mkout append)"
    out="$dir/append.log"
    bash -c "$SCRIPT $QUIET -O '$out' -c 'echo first'"
    first_size=$(stat -c '%s' "$out" 2>/dev/null || stat -f '%z' "$out")
    bash -c "$SCRIPT $QUIET -a -O '$out' -c 'echo second'"
    second_size=$(stat -c '%s' "$out" 2>/dev/null || stat -f '%z' "$out")

    if [[ "$second_size" -gt "$first_size" ]] && grep -q "first" "$out" && grep -q "second" "$out"; then
        ok "append_mode"
    else
        bad "append_mode"
    fi
}

test_force_overwrite() {
    local dir out
    dir="$(mkout overwrite)"
    out="$dir/existing.log"
    printf "existing content\n" > "$out"
    if bash -c "$SCRIPT $QUIET -f -O '$out' -c 'echo new content'"; then
        if grep -q "new content" "$out" && ! grep -q "existing content" "$out"; then
        ok "force_overwrite"
        else
        bad "force_overwrite: unexpected content"
        fi
    else
        bad "force_overwrite: rc!=0"
    fi
}

test_empty_command() {
    local dir out rc
    dir="$(mkout empty)"
    out="$dir/empty.log"
    set +e
    bash -c "$SCRIPT $QUIET -O '$out' -c ''"
    rc=$?
    set -e
    if [[ $rc -eq 0 || $rc -eq 1 ]]; then ok "empty_command"; else bad "empty_command: rc=$rc"; fi
}

test_special_characters() {
    local dir out
    dir="$(mkout special)"
    out="$dir/special.log"
    if bash -c "$SCRIPT $QUIET -O '$out' -c 'printf \"\\t\\n\\r\\a\"'"; then
        ok "special_characters"
    else
        bad "special_characters"
    fi
}

test_concurrent_files() {
    local dir out inp both
    dir="$(mkout concurrent)"
    out="$dir/main.log"
    inp="$dir/input.log"
    both="$dir/both.log"

    if bash -c "$SCRIPT $QUIET -O '$out' -I '$inp' -B '$both' -c 'echo test input and output'"; then
        if [[ -f "$out" && -f "$both" ]]; then ok "concurrent_files"; else bad "concurrent_files: missing files"; fi
    else
        bad "concurrent_files: rc!=0"
    fi
}

test_file_permissions() {
    local dir out
    dir="$(mkout perms)"
    out="$dir/perms.log"
    if bash -c "$SCRIPT $QUIET -O '$out' -c 'echo test'"; then
        if [[ -r "$out" ]]; then ok "file_permissions"; else bad "file_permissions: cant read"; fi
    else
        bad "file_permissions: rc!=0"
    fi
}

test_umask_permissions() {
    local dir out mode
    dir="$(mkout umask)"
    out="$dir/umask.log"
    ( umask 022; bash -c "$SCRIPT $QUIET -O '$out' -c 'echo x'" ) || { bad "umask_permissions: rc!=0"; return; }
    mode=$(stat -c '%a' "$out" 2>/dev/null || stat -f '%Lp' "$out")
    [[ "$mode" == "644" ]] && ok "umask_permissions (644)" || bad "umask_permissions: $mode"
}

test_exec_failure() {
    local dir out
    dir="$(mkout execfail)"
    out="$dir/execfail.log"
    set +e
    bash -c "$SCRIPT $QUIET -O '$out' -c 'no_such_command_xyz_123'"
    rc=$?
    set -e
    if [[ $rc -ne 0 ]]; then bad "rc!=0" ; else ok "rc==0"; fi
}

test_utf8_multibyte() {
    local dir out s="áéíóú ñ ☺ 漢字"
    dir="$(mkout utf8)"
    out="$dir/utf8.log"
    if bash -c "$SCRIPT $QUIET -O '$out' -c 'printf \"%s\n\" \"$s\"'"; then
        LC_ALL=C.UTF-8 grep -qxF "$s" "$out" && ok "utf8_multibyte" || bad "utf8_multibyte: mismatch"
    else
        bad "utf8_multibyte: rc!=0"
    fi
}

test_timing_monotonic() {
    local dir out timing
    dir="$(mkout timing2)"
    out="$dir/out.log"
    timing="$dir/time.log"
    if bash -c "$SCRIPT $QUIET -O '$out' -T '$timing' -c 'printf A; sleep 0.1; printf B; sleep 0.1; printf C'"; then
        [[ -s "$timing" ]] || { bad "timing_monotonic: vacío"; return; }
        awk '
        NF!=2 {bad=1}
        {
            if ($1 < 0) neg=1;
        }
        END { if (bad||neg) exit 1; else exit 0 }
        ' "$timing"
        [[ $? -eq 0 ]] && ok "timing_monotonic" || bad "timing_monotonic"
    else
        bad "timing_monotonic: rc!=0"
    fi
}

###############################################################################
# Runner
###############################################################################
main() {
    echo "Running SCRIPT=$SCRIPT"
    test_echo_always
    test_basic_command_output
    test_interactive_mode
    test_error_commands
    test_exit_codes
    test_signal_handling
    test_backspace_handling
    test_timing_file
    test_append_mode
    test_force_overwrite
    test_empty_command
    test_special_characters
    test_concurrent_files
    test_file_permissions
    test_umask_permissions
    test_exec_failure
    test_utf8_multibyte
    test_timing_monotonic


    echo
    echo -e "Results: ${GREEN}${pass} OK${NC}, ${RED}${fail} FAIL${NC}"
    [[ $fail -eq 0 ]] || exit 1
}

set -u
main "$@"
