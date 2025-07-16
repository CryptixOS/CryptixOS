import subprocess
import sys
from dataclasses import dataclass
from typing import Optional, List, Union
from helix.log import trace, error, fail


@dataclass
class CommandResult:
    cmd: List[str]
    stdout: str
    stderr: str
    code: int

    @property
    def success(self) -> bool:
        return self.code == 0


class ShellError(Exception):
    def __init__(self, cmd: List[str], code: int, stdout: str, stderr: str):
        self.cmd = cmd
        self.code = code
        self.stdout = stdout
        self.stderr = stderr
        super().__init__(f"Command failed ({code}): {' '.join(cmd)}")


def _run_internal(
    cmd: Union[str, List[str]],
    raise_on_failure: bool = True,
    cwd: Optional[str] = None,
    env: Optional[dict] = None,
    input: Optional[Union[str, bytes]] = None,
    encoding: str = "utf-8",
    stream: bool = False
) -> CommandResult:
    if isinstance(cmd, str):
        cmd_list = cmd.split()
    else:
        cmd_list = cmd

    trace(f"running {cmd_list}")

    try:
        if stream:
            proc = subprocess.Popen(
                cmd_list,
                stdin=subprocess.PIPE if input is not None else None,
                stdout=sys.stdout,
                stderr=sys.stderr,
                cwd=cwd,
                env=env,
                text=True if isinstance(input, str) else False
            )
            proc.communicate(input=input)
            code = proc.returncode
            result = CommandResult(cmd_list, "", "", code)
        else:
            proc = subprocess.run(
                cmd_list,
                input=input,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=cwd,
                env=env,
                check=False,
                text=True if isinstance(input, str) else None
            )
            result = CommandResult(
                cmd=cmd_list,
                stdout=proc.stdout.strip() if proc.stdout else "",
                stderr=proc.stderr.strip() if proc.stderr else "",
                code=proc.returncode,
            )

        if raise_on_failure and result.code != 0:
            fail(f'`{result.cmd}` failed with exit code: `{result.code}`,\n\terror message => {result.stderr}\n{result.stdout}')
            raise ShellError(result.cmd, result.code, result.stdout, result.stderr)

        return result

    except FileNotFoundError:
        fail(f'command not found => `{cmd_list[0]}`')
        raise ShellError(cmd_list, 127, "", f"Command not found: {cmd_list[0]}")


def run(
    cmd: Union[str, List[str]],
    *,
    cwd: Optional[str] = None,
    env: Optional[dict] = None,
    input: Optional[Union[str, bytes]] = None,
    encoding: str = "utf-8",
    stream: bool = False
) -> CommandResult:
    return _run_internal(cmd, raise_on_failure=True, cwd=cwd, env=env, input=input, encoding=encoding, stream=stream)


def try_run(
    cmd: Union[str, List[str]],
    *,
    cwd: Optional[str] = None,
    env: Optional[dict] = None,
    input: Optional[Union[str, bytes]] = None,
    encoding: str = "utf-8",
    stream: bool = False
) -> CommandResult:
    return _run_internal(cmd, raise_on_failure=False, cwd=cwd, env=env, input=input, encoding=encoding, stream=stream)

