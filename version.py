Import("env")

with open("version.txt") as f:
    version = f.read().strip()

env.Append(
    BUILD_FLAGS=[
        f'-D FW_VERSION=\\"{version}\\"'
    ]
)