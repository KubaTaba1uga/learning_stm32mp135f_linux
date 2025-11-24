###############################################
#                Imports                      #
###############################################
import glob
import os
import shutil
import subprocess
import tempfile

from invoke import task

###############################################
#                Public API                   #
###############################################
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
LINUX_PATH = os.path.join(THIRD_PARTY_PATH, "linux")
OPTEE_PATH = os.path.join(THIRD_PARTY_PATH, "optee-os")
UBOOT_PATH = os.path.join(THIRD_PARTY_PATH, "u-boot")
TFA_PATH = os.path.join(THIRD_PARTY_PATH, "tf-a")

@task
def install(c):
    """
    Install Ansible and sshpass on the system if they are not already installed.

    Usage:
        inv install
    """

    _pr_info("Installing Dependencies...")
    try:
        result = c.run(
            """
                  sudo apt-get update && sudo apt-get install -y \
                    build-essential device-tree-compiler \
                    clang \
                    make \
                    bison \
                    xxd   \
                    stlink-tools
            """,
            warn=True,
        )
        if result.ok:
            _pr_info("Dependencies installed successfully.")
        else:
            _pr_error("Unable to install dependencies.")
    except Exception as e:
        _pr_error(f"Error installing dependencies: {e}")


@task
def clean(c, bytecode=False, extra=""):
    """
    Clean up build and temporary files recursively.

    This task removes specified patterns of files and directories,
    including build artifacts, temporary files, and optionally Python
    bytecode files.

    Args:
        bytecode (bool, optional): If True, also removes Python bytecode files (.pyc). Defaults to False.
        extra (str, optional): Additional pattern to remove. Defaults to "".

    Usage:
        inv clean
        inv clean --bytecode
        inv clean --extra='**/*.log'
    """
    patterns = [
      "build/*",
      "*/*~*",
      "*/#*",
      "**/*~*",
      "**/*#*",
      "*~*",
      "*#*",
      "**/.#*"   
    ]
    
    if bytecode:
        patterns.append("**/*.pyc")
    if extra:
        patterns.append(extra)

    for pattern in patterns:
        _pr_info(f"Removing files matching pattern '{pattern}'")

        # Use glob to find files recursively and remove each one
        for path in glob.glob(pattern, recursive=True):
            if os.path.isfile(path) or os.path.islink(path):
                os.remove(path)
                print(f"Removed file {path}")
            elif os.path.isdir(path):
                shutil.rmtree(path)
                print(f"Removed directory {path}")
    try:
        # clean_optee(c)
        clean_uboot(c)
        # clean_tfa(c)
    except Exception:
        _pr_error("Cleaning failed")
        raise

    _pr_info("Clean up completed.")


@task
def build_uboot(c, example):
    _pr_info("Building uboot...")    

    example_dir = os.path.join(EXAMPLES_PATH, example, "u-boot")
    
    env = {
        "CROSS_COMPILE": TOOLCHAIN_PATH,
    }
    env_path = os.path.join(example_dir, "build_env")
    if os.path.exists(env_path):
       with open(env_path, "r") as fp:
           for line in fp.readlines():
               pair = line.split("=")
               if len(pair)==2:
                   key, value = pair
                   env[key] = value

    for key, value in env.items():
        os.environ[key] = value
        
    config_path = os.path.join(example_dir, "build_config")
    if not os.path.exists(config_path):
        config_path = None
                   
    uboot_env_path = os.path.join(example_dir, "uboot_env")    
    if not os.path.exists(uboot_env_path):
        uboot_env_path = None
    
    try:
        with c.cd(UBOOT_PATH):
            # if not config_path:
            _run_make(c, "stm32mp13_defconfig", env)
            # else:
                # c.run(f"cp {config_path} ./.config")

            # if uboot_env_path:
            #     c.run("scripts/config --disable CONFIG_USE_ENV_MMC_PARTITION")
            #     c.run("scripts/config --enable CONFIG_USE_DEFAULT_ENV_FILE")
            #     c.run("scripts/config --enable CONFIG_ENV_IS_DEFAULT")
            #     c.run("scripts/config --enable CONFIG_ENV_IS_NOWHERE")
            #     c.run("scripts/config --disable CONFIG_ENV_IS_IN_MMC")
            #     c.run("scripts/config --set-str CONFIG_DEFAULT_ENV_FILE env.txt")
            #     c.run("scripts/config --set-str CONFIG_ENV_SOURCE_FILE env.txt")                
                
            #     c.run(f"cp {uboot_env_path} env.txt")                
                
            _run_make(c, "-j 4 all", env)
            c.run(f"mkdir -p {BUILD_PATH}")
            c.run(f"cp u-boot-nodtb.bin u-boot.dtb {BUILD_PATH}")

    except Exception as err:
        for key in list(env.keys()):
            env.pop(key)
        
        print(err)
        _pr_error("Building uboot failed")
        raise err

    _pr_info("Building uboot completed")


@task
def build(c, example):
    _pr_info("Building...")

    example_dir = os.path.join(EXAMPLES_PATH, example)
    build_dir = os.path.join(BUILD_PATH, example)          

    if not os.path.exists(example_dir):
        _pr_error(f"{example_dir} does not exists")        
        return 1
    try:
        
        # build_optee(c)
        build_uboot(c)
        # build_tfa(c)
                  
        _pr_info(f"Building {example}...")
                
        c.run(f"mkdir -p {build_dir}")
        
        _pr_info(f"Building {example} completed")
                
    except Exception:
        _pr_error(f"Building {example} failed")
        raise

    _pr_info("Build completed")


@task
def clean_tfa(c):
    with c.cd(os.path.join(THIRD_PARTY_PATH, "tf-a")):
        c.run("make clean")
        c.run("rm -rf build")


@task
def clean_optee(c):
    with c.cd(os.path.join(THIRD_PARTY_PATH, "optee-os")):
        c.run("make clean")
        c.run("rm -rf out")


@task
def clean_uboot(c):
    with c.cd(os.path.join(THIRD_PARTY_PATH, "u-boot")):
        c.run("make clean")


@task
def deploy_via_usb(c):
    if not os.environ.get("STM32_PRG_PATH"):
        raise ValueError("set STM32_PRG_PATH to path where")

    with c.cd(BUILD_PATH):
        c.run(
            """sudo $STM32_PRG_PATH/STM32_Programmer_CLI -c port=usb1 \
                      -d tf-a-stm32mp135f-dk.stm32 0x1 -s 0x1           \
                      -d fip.bin 0x3 -s 0x3                             
        """
        )


@task
def deploy_to_sdcard(c, dev="sda"):
    with c.cd(BUILD_PATH):
        if not os.path.exists("/dev/disk/by-partlabel/fsbl1"):
            raise ValueError("No /dev/disk/by-partlabel/fsbl1")
        if not os.path.exists("/dev/disk/by-partlabel/fsbl2"):
            raise ValueError("No /dev/disk/by-partlabel/fsbl2")
        if not os.path.exists("/dev/disk/by-partlabel/fip"):
            raise ValueError("No /dev/disk/by-partlabel/fip")

        c.run(
            "sudo dd if=tf-a-stm32mp135f-dk.stm32 of=/dev/disk/by-partlabel/fsbl1 bs=1K conv=fsync"
        )
        c.run(
            "sudo dd if=tf-a-stm32mp135f-dk.stm32 of=/dev/disk/by-partlabel/fsbl2 bs=1K conv=fsync"
        )
        c.run("sudo dd if=fip.bin of=/dev/disk/by-partlabel/fip bs=1K conv=fsync")
        c.run("sudo sync")


        
###############################################
#                Private API                  #
###############################################
def _command_exists(command):
    try:
        subprocess.run(
            ["which", command], stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        return True
    except Exception:
        return False


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


def _run_make(ctx, command, env):
    ctx.run(
        f"make {command} " + " ".join([f"{arg}={env[arg]}" for arg in env]),
        env=env,
    )
