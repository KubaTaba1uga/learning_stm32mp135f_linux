import os

from invoke import task

ROOT_PATH = os.path.dirname(os.path.abspath(__file__))
THIRD_PARTY_PATH = os.path.join(ROOT_PATH, "third_party")
BUILD_PATH = os.path.join(ROOT_PATH, "build")
EXAMPLES_PATH = os.path.join(ROOT_PATH, "examples")
TOOLCHAIN_PATH = os.path.join(
    THIRD_PARTY_PATH,
    "arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf",
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

    _pr_info("Building u-boot...")
    
    example_path = os.path.join(EXAMPLES_PATH, example, "u-boot")
    if os.path.exists(example_path):
        _load_env(example_path, env)
        
    config_path = os.path.join(example_path, "build_config")
    if not os.path.exists(config_path):
        config_path = None

    uboot_path =  os.path.join(THIRD_PARTY_PATH, "u-boot")
    with c.cd(uboot_path):
        _run_make(c, "make stm32mp13_defconfig")

        if config_path:
            _run(c, f"scripts/kconfig/merge_config.sh .config {config_path}")
            
        _run_make(c, "make -j 4 all")
        _run(c, f"mkdir -p {BUILD_PATH}")
        _run(c, f"cp u-boot-nodtb.bin u-boot.dtb {BUILD_PATH}")

    _pr_info("Building u-boot completed")

@task
def build_optee(c, example):
    global env    
    env = {
        "CROSS_COMPILE": TOOLCHAIN_PATH,
        "CROSS_COMPILE_core": TOOLCHAIN_PATH,
        "CROSS_COMPILE_ta_arm32": TOOLCHAIN_PATH,
        "CFG_USER_TA_TARGETS": "ta_arm32",
        "CFG_ARM64_core": "n",
        "PLATFORM": "stm32mp1-135F_DK",
        "CFG_IN_TREE_EARLY_TAS": "trusted_keys/f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c",        
    }
    
    _pr_info("Building optee-os...")
    
    example_path = os.path.join(EXAMPLES_PATH, example, "optee-os")
    if os.path.exists(example_path):
        _load_env(example_path, env)

    optee_path =  os.path.join(THIRD_PARTY_PATH, "optee-os")
    with c.cd(optee_path):
        _run_make(c, "make -j 4 all")
        _run(c, f"mkdir -p {BUILD_PATH}")
        with c.cd(os.path.join("out", "arm-plat-stm32mp1", "core")):
            c.run(f"cp tee.bin tee-raw.bin tee-*_v2.bin {BUILD_PATH}")

    _pr_info("Building optee-os completed")
    
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

def _run_make(c, cmd):
    if env:
        return c.run(f"{cmd} " + " ".join([f"{arg}={env[arg]}" for arg in env]), env=env)
    return c.run(cmd)

def _load_env(example_path, env):
    env_path = os.path.join(example_path, "build_env")
    if os.path.exists(env_path):
       with open(env_path, "r") as fp:
           for line in fp.readlines():
               delim = line.find("=")
               if delim == -1:
                   continue

               key, value = line[:delim], line[delim+1:]
               
               env[key] = value.strip('\n')
       return env_path
    return None
    
