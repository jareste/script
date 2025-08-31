#!/bin/bash

SCRIPT="./ft_script"

check_fds() {
    local pid=$1
    if [ -d "/proc/$pid/fd" ]; then
        echo "Open FDs for PID $pid:"
        ls -la /proc/$pid/fd/ | wc -l
        ls -la /proc/$pid/fd/
    fi
}

check_children() {
    echo "=== Checking for zombie/orphan processes ==="
    ps -eo pid,ppid,state,comm | grep -E "(Z|ft_script|bash|sh)" || echo "No suspicious processes"
}

test_fd_leaks() {
    echo "=== Testing FD leaks ==="
    
    initial_fds=$(ls /proc/$$/fd | wc -l)
    echo "Initial FDs: $initial_fds"
    
    for i in {1..10}; do
        $SCRIPT -q -O "test_$i.log" -c "echo test $i" >/dev/null 2>&1
    done

    $SCRIPT -q -O "test_11.log" -o 3 -c "echo test 11" >/dev/null 2>&1

    final_fds=$(ls /proc/$$/fd | wc -l)
    echo "Final FDs: $final_fds"
    
    for i in {1..10}; do
        rm "test_$i.log"
    done
    rm "test_11.log"

    if [ $final_fds -gt $initial_fds ]; then
        echo "❌ FD LEAK DETECTED: $((final_fds - initial_fds)) FDs leaked"
        return 1
    else
        echo "✅ No FD leaks detected"
        return 0
    fi
}

test_process_cleanup() {
    echo "=== Testing process cleanup ==="
    
    $SCRIPT -q -O test_bg.log &
    script_pid=$!
    
    sleep 0.5
    
    children=$(pgrep -P $script_pid | wc -l)
    echo "Children of $script_pid: $children"
    
    kill -TERM $script_pid 2>/dev/null
    sleep 0.5
    
    zombies=$(ps -eo pid,ppid,state | grep -c "Z" || true)
    echo "Zombie processes: $zombies"
    
    kill -KILL $script_pid 2>/dev/null || true
    wait $script_pid 2>/dev/null || true

    rm "test_bg.log"
    if [ $zombies -gt 0 ]; then
        echo "❌ ZOMBIE PROCESSES DETECTED"
        return 1
    else
        echo "✅ No zombie processes"
        return 0
    fi
}

echo "=== RESOURCE CHECK STARTING ==="
check_children
test_fd_leaks
test_process_cleanup
check_children
echo "=== RESOURCE CHECK COMPLETE ==="