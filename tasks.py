import os

from invoke import task

ROOT_PATH = os.path.dirname(os.path.abspath(__file__))
THIRD_PARTY_PATH = os.path.join(ROOT_PATH, "third_party")
BUILD_PATH = os.path.join(ROOT_PATH, "build")
EXAMPLES_PATH = os.path.join(ROOT_PATH, "examples")
TOOLCHAIN_PATH = os.path.join(
    THIRD_PARTY_PATH,
    "arm-gnu-toolchain-11.3.rel1-x86_64-arm-none-linux-gnueabihf",
    "bin",
    "arm-none-linux-gnueabihf-",
)

env = None

@task
def add_repo(c, name, tag, url):
    _run("mkdir -p third_party")
    _run("git status")
    _run("git add . || true")
    _run("git commit -m \"WIP\" || true")

    if _run(f"git remote get-url \"{name}\"", warn=True, echo=False) == 0:
        _pr_error(f"{name} already exists")
        return 1

    _run(f"git remote add \"{name}\" \"{url}\"")
    _run(f"git subtree add --prefix=\"third_party/{name}\" \"{name}\" \"{tag}\" --squash")


@task
def build_uboot(c, example):
    global env    
    env = {
        "CROSS_COMPILE": TOOLCHAIN_PATH,        
    }

    example_path = os.path.join(EXAMPLES_PATH, example, "u-boot")
    if not os.path.exists(example_path):
        _pr_error(f"{example} does not exist")
        return 1
    
    env_path = os.path.join(example_dir, "build_env")
    if os.path.exists(env_path):
       with open(env_path, "r") as fp:
           for line in fp.readlines():
               delim = line.find("=")
               if delim == -1:
                   continue

               key, value = line[:delim], line[delim+1:]
               
               env[key] = value.strip('\n')

    print(env)
    
###############################################
#                Private API                  #
###############################################
def _pr_info(message: str):
    """
    Print an informational message in blue color.

    Args:
        message (str): The message to print.

    Usage:
        pr_info("This is an info message.")
    """
    print(f"\033[94m[INFO] {message}\033[0m")


def _pr_warn(message: str):
    """
    Print a warning message in yellow color.

    Args:
        message (str): The message to print.

    Usage:
        pr_warn("This is a warning message.")
    """
    print(f"\033[93m[WARN] {message}\033[0m")


def _pr_debug(message: str):
    """
    Print a debug message in cyan color.

    Args:
        message (str): The message to print.

    Usage:
        pr_debug("This is a debug message.")
    """
    print(f"\033[96m[DEBUG] {message}\033[0m")


def _pr_error(message: str):
    """
    Print an error message in red color.

    Args:
        message (str): The message to print.

    Usage:
        pr_error("This is an error message.")
    """
    print(f"\033[91m[ERROR] {message}\033[0m")
        

def _run(c, *args, **kwargs):
    if env:
        return c.run(*args, **kwargs, env=env)
    return c.run(*args, **kwargs)
