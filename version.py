# version.py

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from typing import Any
    env: Any

Import("env")  # type: ignore

with open("version.txt") as f:
    version = f.read().strip()

env.Append(
    BUILD_FLAGS=[
        f'-D FW_VERSION=\\"{version}\\"'
    ]
)