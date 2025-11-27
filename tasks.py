import os
import glob

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
    _run(c, "mkdir -p third_party")
    _run(c, "git status")
    _run(c, "git add . || true")
    _run(c, "git commit -m \"WIP\" || true")

    if _run(c, f"git remote get-url \"{name}\"", warn=True, echo=False) == 0:
        _pr_error(f"{name} already exists")
        return 1

    _run(c, f"git remote add \"{name}\" \"{url}\"")
    _run(c, f"git subtree add --prefix=\"third_party/{name}\" \"{name}\" \"{tag}\" --squash")


@task
def build(c, example="", rootfs=True, linux=True, uboot=True, optee=True, tfa=True):
    _pr_info(f"Building {example}...")

    example_dir = os.path.join(EXAMPLES_PATH, example)
    build_dir = os.path.join(BUILD_PATH, example)

    try:
      if rootfs:
        build_rootfs(c, example)
      if linux:  
        build_linux(c, example)
      if uboot:
        build_uboot(c, example)
      if optee:
        build_optee(c, example)
      if tfa:
        build_tfa(c, example)
        
    except Exception:
        _pr_error(f"Building {example} failed")
        raise

    _pr_info(f"Building {example} completed")


@task
def build_rootfs(c, example):
    _pr_info("Building rootfs...")
    global env    
    env = {}
    
    example_path = os.path.join(EXAMPLES_PATH, example, "buildroot")
    if os.path.exists(example_path):
        _load_env(example_path, env)

    config_path = os.path.join(example_path, "build_config")
    if not os.path.exists(config_path):
        _pr_error(f"{example_path}/build_config doesn't exist");
        return
    
    _run(c, f"mkdir -p {BUILD_PATH}")        
    with c.cd(os.path.join(THIRD_PARTY_PATH, "buildroot")):
      _run(c, f"cp {config_path} .config")
      _run_make(c, "make")
      _run(c, f"cp output/images/rootfs.tar {BUILD_PATH}/")      

    _pr_info("Building rootfs completed")


@task
def build_linux(c, example):
    global env    
    env = {
        "CROSS_COMPILE": TOOLCHAIN_PATH,
        "ARCH":"arm",
    }

    _pr_info("Building linux...")    

    example_path = os.path.join(EXAMPLES_PATH, example, "linux")
    if os.path.exists(example_path):
        _load_env(example_path, env)

    config_path = os.path.join(example_path, "build_config")
    if not os.path.exists(config_path):
        config_path = None
        _pr_warn(f"{example_path}/build_config doesn't exist");
        
    dts_path = os.path.join(example_path, "dts")
    if not os.path.exists(dts_path):
        dts_path = None
        _pr_warn(f"{example_path}/dts doesn't exist");
        
    _run(c, f"mkdir -p {BUILD_PATH}")        
    with c.cd(os.path.join(THIRD_PARTY_PATH, "linux")):
      _run_make(c, "make multi_v7_defconfig")
      
      if config_path:
          _run(c, f"scripts/kconfig/merge_config.sh .config {config_path}")

      if dts_path:
          _run(c, f"cp {dts_path}/* arch/arm/boot/dts/st/")
          
      _run_make(c, "make -j 8 zImage st/stm32mp135f-dk.dtb")
      _run(c, f"cp arch/arm/boot/zImage {BUILD_PATH}/")
      _run(c, f"cp arch/arm/boot/dts/st/stm32mp135f-dk.dtb {BUILD_PATH}/")

    _pr_info("Building linux completed")
      
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
        _pr_warn(f"{example_path}/build_config doesn't exist");

    env_path = os.path.join(example_path, "uboot.env")
    if not os.path.exists(env_path):
        env_path = None
        _pr_warn(f"{example_path}/uboot_env doesn't exist");

    _run(c, f"mkdir -p {BUILD_PATH}")        
    uboot_path =  os.path.join(THIRD_PARTY_PATH, "u-boot")
    with c.cd(uboot_path):
        _run_make(c, "make stm32mp13_defconfig")

        if config_path:
            _run(c, f"scripts/kconfig/merge_config.sh .config {config_path}")

        if env_path:
            _run(c, f"cp {env_path} board/st/stm32mp1/uboot.env")
            
        _run_make(c, "make -j 4 all")
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

    _run(c, f"mkdir -p {BUILD_PATH}")    
    optee_path =  os.path.join(THIRD_PARTY_PATH, "optee-os")
    with c.cd(optee_path):
        _run_make(c, "make -j 4 all")
        with c.cd(os.path.join("out", "arm-plat-stm32mp1", "core")):
            _run(c, f"cp tee.bin tee-raw.bin tee-*_v2.bin {BUILD_PATH}")

    _pr_info("Building optee-os completed")

@task
def build_tfa(c, example):
    global env
    env = {
        "CROSS_COMPILE": TOOLCHAIN_PATH,
        "CC": str(TOOLCHAIN_PATH) + "gcc",
        "LD": str(TOOLCHAIN_PATH) + "ld",
        "BL32": os.path.join(BUILD_PATH, "tee-header_v2.bin"),
        "BL32_EXTRA1": os.path.join(BUILD_PATH, "tee-pager_v2.bin"),
        "BL32_EXTRA2": os.path.join(BUILD_PATH, "tee-pageable_v2.bin"),
        "BL33": os.path.join(BUILD_PATH, "u-boot-nodtb.bin"),
        "BL33_CFG": os.path.join(BUILD_PATH, "u-boot.dtb"),
        "ARM_ARCH_MAJOR": "7",
        "ARCH": "aarch32",
        "PLAT": "stm32mp1",
        "DTB_FILE_NAME": "stm32mp135f-dk.dtb",
        "AARCH32_SP": "optee",
        "STM32MP15_OPTEE_RSV_SHM": "0",
        "STM32MP_EMMC": "1",
        "STM32MP_SDMMC": "1",
    }
    
    _pr_info("Building tf-a...")
    
    example_path = os.path.join(EXAMPLES_PATH, example, "tf-a")
    if os.path.exists(example_path):
        _load_env(example_path, env)

    _run(c, f"mkdir -p {BUILD_PATH}")
    tfa_path =  os.path.join(THIRD_PARTY_PATH, "tf-a")        
    with c.cd(tfa_path):
      _run_make(c, "make -j 4 all fip")
      _run(c, f"cp build/stm32mp1/*/fip.bin {BUILD_PATH}/fip.bin")
      _run(c,
          f"cp build/stm32mp1/*/tf-a-stm32mp135f-dk.stm32 {BUILD_PATH}/tf-a-stm32mp135f-dk.stm32"
      )

    _pr_info("Building tf-a completed")    

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
        clean_uboot(c)
        clean_optee(c)
        clean_tfa(c)
    except Exception:
        _pr_error("Cleaning failed")
        raise

    _pr_info("Clean up completed.")

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
def deploy_to_sdcard(c, dev="sda"):
    if not os.path.exists("/dev/disk/by-partlabel/fsbl1"):
        raise ValueError("No /dev/disk/by-partlabel/fsbl1")
    if not os.path.exists("/dev/disk/by-partlabel/fsbl2"):
        raise ValueError("No /dev/disk/by-partlabel/fsbl2")
    if not os.path.exists("/dev/disk/by-partlabel/fip"):
        raise ValueError("No /dev/disk/by-partlabel/fip")

    with c.cd(BUILD_PATH):
        c.run(
            "sudo dd if=tf-a-stm32mp135f-dk.stm32 of=/dev/disk/by-partlabel/fsbl1 bs=1K conv=fsync"
        )
        c.run(
            "sudo dd if=tf-a-stm32mp135f-dk.stm32 of=/dev/disk/by-partlabel/fsbl2 bs=1K conv=fsync"
        )
        c.run("sudo dd if=fip.bin of=/dev/disk/by-partlabel/fip bs=1K conv=fsync")
        
    c.run("sudo sync")

@task
def deploy_to_tftp(c, directory="/srv/tftp"):
    if not os.path.exists(directory):
        raise ValueError(f"{directory} does not exists")

    with c.cd(BUILD_PATH):
        _run(c, # Copy linux artifacts
            f"sudo cp zImage stm32mp135f-dk.dtb {directory}"
        )

@task
def deploy_to_nfs(c, directory="/srv/nfs"):
    if not os.path.exists(directory):
        raise ValueError(f"{directory} does not exists")

    with c.cd(BUILD_PATH):
        _run(c, f"sudo tar xvf rootfs.tar -C {directory}")
    
    
###############################################
#                Private API                  #
###############################################
def _pr_info(message: str):
    print(f"\033[94m[INFO] {message}\033[0m")


def _pr_warn(message: str):
    print(f"\033[93m[WARN] {message}\033[0m")


def _pr_debug(message: str):
    print(f"\033[96m[DEBUG] {message}\033[0m")


def _pr_error(message: str):
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

    _pr_warn(f"{example_path}/build_env doesn't exist");
    return None
    
