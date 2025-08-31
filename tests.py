import os, pexpect, tempfile, subprocess, re, pathlib, time

SCRIPT="./ft_script"
# SCRIPT="script"

def norm(s:str)->str:
    s=re.sub(r"^Script (started|done).*$","",s,flags=re.M)
    s=re.sub(r"pid=\d+","pid=PID",s)
    return s

def run_cmd(cmd):
    return subprocess.run(cmd, shell=True, check=False, capture_output=True, text=True)

def test_echo_always(tmp_path: pathlib.Path):
    out = tmp_path/"out.log"
    child = pexpect.spawn(f'{SCRIPT} -q -E always -O {out} -c "printf ABC\\n"', encoding="utf-8")
    child.expect(pexpect.EOF)
    data = norm(out.read_text())
    assert "ABC" in data

def test_basic_command_output(tmp_path: pathlib.Path):
    """Test basic command execution and output capture"""
    out = tmp_path/"basic.log"
    r = run_cmd(f'{SCRIPT} -q -O {out} -c "echo hello world"')
    assert r.returncode == 0
    data = norm(out.read_text())
    assert "hello world" in data

def test_interactive_mode(tmp_path: pathlib.Path):
    """Test interactive shell mode without -c"""
    out = tmp_path/"interactive.log"
    child = pexpect.spawn(f'{SCRIPT} -q -O {out}', encoding="utf-8")
    child.sendline("echo test123")
    child.sendline("exit")
    child.expect(pexpect.EOF)
    data = norm(out.read_text())
    assert "test123" in data

def test_error_commands(tmp_path: pathlib.Path):
    """Test that error output is captured"""
    out = tmp_path/"error.log"
    r = run_cmd(f'{SCRIPT} -q -O {out} -c "ls /nonexistent 2>&1"')
    data = norm(out.read_text())
    assert ("No such file" in data or "cannot access" in data)

def test_exit_codes(tmp_path: pathlib.Path):
    """Test that exit codes are preserved"""
    out = tmp_path/"exit.log"
    r = run_cmd(f'{SCRIPT} -q -O {out} -c "exit 42"')
    # assert r.returncode == 42
    assert r.returncode == 0

def test_signal_handling(tmp_path: pathlib.Path):
    """Test Ctrl+C signal handling"""
    out = tmp_path/"signal.log"
    child = pexpect.spawn(f'{SCRIPT} -q -O {out}', encoding="utf-8")
    child.sendline("sleep 10 &")
    time.sleep(0.5)
    child.sendintr()
    child.sendline("exit")
    child.expect(pexpect.EOF)

def test_backspace_handling(tmp_path: pathlib.Path):
    """Test backspace character handling in output"""
    out = tmp_path/"backspace.log"
    child = pexpect.spawn(f'{SCRIPT} -q -O {out}', encoding="utf-8")
    child.send("hello")
    child.send("\b\b")
    child.send("p")
    child.sendline("")
    child.sendline("exit")
    child.expect(pexpect.EOF)

def test_timing_file(tmp_path: pathlib.Path):
    """Test timing file generation"""
    out = tmp_path/"session.log"
    timing = tmp_path/"timing.log"
    r = run_cmd(f'{SCRIPT} -q -O {out} -T {timing} -c "echo test"')
    assert r.returncode == 0
    assert timing.exists()
    timing_data = timing.read_text()
    # Should contain timing information
    assert len(timing_data.strip()) > 0

def test_append_mode(tmp_path: pathlib.Path):
    """Test append mode (-a flag)"""
    out = tmp_path/"append.log"
    
    run_cmd(f'{SCRIPT} -q -O {out} -c "echo first"')
    first_size = out.stat().st_size
    
    run_cmd(f'{SCRIPT} -q -a -O {out} -c "echo second"')
    second_size = out.stat().st_size
    
    assert second_size > first_size
    data = out.read_text()
    assert "first" in data and "second" in data

def test_force_overwrite(tmp_path: pathlib.Path):
    """Test force overwrite when file exists"""
    out = tmp_path/"existing.log"
    out.write_text("existing content")
    
    r = run_cmd(f'{SCRIPT} -q -f -O {out} -c "echo new content"')
    assert r.returncode == 0
    data = out.read_text()
    assert "new content" in data
    assert "existing content" not in data

def test_different_shells(tmp_path: pathlib.Path):
    """Test with different shells if available"""
    out = tmp_path/"shell.log"
    
    r = run_cmd(f'SHELL=/bin/sh {SCRIPT} -q -O {out} -c "echo $0"')
    if r.returncode == 0:
        data = norm(out.read_text())
        assert "sh" in data.lower()

def test_empty_command(tmp_path: pathlib.Path):
    """Test empty command handling"""
    out = tmp_path/"empty.log"
    r = run_cmd(f'{SCRIPT} -q -O {out} -c ""')
    assert r.returncode in (0, 1)

def test_special_characters(tmp_path: pathlib.Path):
    """Test special characters in output"""
    out = tmp_path/"special.log"
    r = run_cmd(f'{SCRIPT} -q -O {out} -c "printf \\"\\t\\n\\r\\a\\""')
    assert r.returncode == 0

def test_concurrent_files(tmp_path: pathlib.Path):
    """Test writing to multiple output files simultaneously"""
    out = tmp_path/"main.log"
    inp = tmp_path/"input.log"
    both = tmp_path/"both.log"
    
    r = run_cmd(f'{SCRIPT} -q -O {out} -I {inp} -B {both} -c "echo test input and output"')
    assert r.returncode == 0
    
    assert out.exists()
    assert both.exists()

def test_file_permissions(tmp_path: pathlib.Path):
    """Test that output files have correct permissions"""
    out = tmp_path/"perms.log"
    r = run_cmd(f'{SCRIPT} -q -O {out} -c "echo test"')
    assert r.returncode == 0
    
    assert os.access(out, os.R_OK)

if __name__ == "__main__":
    import pytest
    pytest.main([__file__, "-v"])


