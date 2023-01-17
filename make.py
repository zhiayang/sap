#!/usr/bin/env python3
from typing import *  # pyright: ignore
import functools
import subprocess
import os
import threading
import multiprocessing


def nologger(*args, **kwargs):
    _ = args, kwargs


def printlogger(*args, **kwargs):
    print(*args, **kwargs)


log = printlogger
verboselog = nologger
veryverboselog = nologger
veryveryverboselog = nologger


def set_verbosity(verbosity):
    global log
    global verboselog
    global veryverboselog
    global veryveryverboselog
    log = printlogger if verbosity >= 0 else nologger
    verboselog = printlogger if verbosity >= 1 else nologger
    veryverboselog = printlogger if verbosity >= 2 else nologger
    veryveryverboselog = printlogger if verbosity >= 3 else nologger


def shell(cmd):
    verboselog("shell:", cmd)
    return subprocess.check_output(cmd, shell=True).decode()


def execvp(cmd):
    verboselog("execvp:", " ".join(cmd))
    return subprocess.check_output(cmd).decode()


def veryverboseshell(cmd):
    veryverboselog("shell:", cmd)
    return subprocess.check_output(cmd, shell=True).decode()


def veryverboseexecvp(cmd):
    veryverboselog("execvp:", " ".join(cmd))
    return subprocess.check_output(cmd).decode()


class Options:
    CAT: Any
    CC: Any
    CXXHDR: Any
    INCLUDES: Any
    OUTPUT_BIN: Any
    SPECIAL_HDRS_COMPDB: Any
    UNAME_IDENT: Any
    CFLAGS: Any
    CXXLIBOBJ: Any
    LDFLAGS: Any
    OUTPUT_DIR: Any
    SPECIAL_HEADERS: Any
    USE_CORETEXT: Any
    CLANG_PCH_FASTER: Any
    CXXOBJ: Any
    LINKER_OPT_FLAGS: Any
    PCH_INCLUDE_FLAGS: Any
    TESTDEPS: Any
    USE_FONTCONFIG: Any
    COMMON_CFLAGS: Any
    CXXSRC: list[str]
    MINIZ_OBJS: Any
    PRECOMP_GCH: Any
    TESTOBJ: Any
    UTF8PROC_OBJS: Any
    CXX: Any
    LLD: Any
    CXX_COMPDB: Any
    MINIZ_SRCS: Any
    PRECOMP_HDR: Any
    TESTS: Any
    UTF8PROC_SRCS: Any
    CXXDEPS: Any
    CXX_COMPDB_HDRS: Any
    NONGCH_CXXFLAGS: Any
    PRECOMP_INCLUDE: Any
    TESTSRC: Any
    WARNINGS: Any
    CXXFLAGS: Any
    DEFINES: Any
    OPT_FLAGS: Any
    PRECOMP_OBJ: Any
    TEST_DIR: Any
    WERROR: Any
    SAP_LIB: Any

    def __init__(self):
        CAT = "cat"

        WARNINGS = """
        -Wno-padded
        -Wno-cast-align
        -Wno-unreachable-code
        -Wno-packed
        -Wno-missing-noreturn
        -Wno-float-equal
        -Wno-unused-macros
        -Wextra
        -Wconversion
        -Wpedantic
        -Wall
        -Wno-unused-parameter
        -Wno-trigraphs
        """.split()

        WERROR = """
        -Werror
        -Wno-error=unused-parameter
        -Wno-error=unused-variable
        -Wno-error=unused-function
        -Wno-unused-but-set-variable
        """.split()

        WARNINGS += WERROR

        OPT_FLAGS = "-O0 -fsanitize=address".split()
        LINKER_OPT_FLAGS = []
        COMMON_CFLAGS = ["-g"] + OPT_FLAGS

        OUTPUT_DIR = "build"
        TEST_DIR = OUTPUT_DIR + "/test"

        CC = "clang"
        CXX = "clang++"
        LLD = "ld.lld"

        CFLAGS = COMMON_CFLAGS + "-std=c99 -fPIC -O3".split()
        CXXFLAGS = (
            COMMON_CFLAGS + "-Wno-old-style-cast -std=c++20 -fno-exceptions".split()
        )
        NONGCH_CXXFLAGS = []
        LDFLAGS = []

        CXXSRC = sorted(shell('find source -iname "*.cpp" -print').split())
        CXXOBJ = [src.replace(".cpp", ".cpp.o") for src in CXXSRC]
        CXXLIBOBJ = [src for src in CXXSRC if src != f"{OUTPUT_DIR}/source/main.cpp.o"]
        CXXDEPS = [obj.replace(".o", ".d") for obj in CXXOBJ]

        CXXHDR = shell('find source -iname "*.h" -print').split()

        SPECIAL_HEADERS = [
            "source/include/" + h
            for h in "defs.h pool.h util.h units.h error.h types.h".split()
        ]
        SPECIAL_HDRS_COMPDB = [h + ".special_compile_db" for h in SPECIAL_HEADERS]

        CXX_COMPDB_HDRS = [h for h in CXXHDR if h not in SPECIAL_HEADERS]
        CXX_COMPDB = [h + ".compile_db" for h in (CXX_COMPDB_HDRS + CXXSRC)]

        TESTSRC = shell('find test -iname "*.cpp" -print').split()
        TESTOBJ = [src.replace(".cpp", ".cpp.o") for src in TESTSRC]
        TESTDEPS = [obj.replace(".o", ".d") for obj in TESTOBJ]
        TESTS = [src.replace("test/", TEST_DIR).removesuffix(".cpp") for src in TESTSRC]

        UTF8PROC_SRCS = ["external/utf8proc/utf8proc.c"]
        UTF8PROC_OBJS = ["external/utf8proc/utf8proc.c.o"]

        MINIZ_SRCS = ["external/miniz/miniz.c"]
        MINIZ_OBJS = ["external/miniz/miniz.c.o"]

        PRECOMP_HDR = "source/include/precompile.h"
        PRECOMP_GCH = f"{OUTPUT_DIR}/{PRECOMP_HDR}.gch"
        PRECOMP_INCLUDE = f"{OUTPUT_DIR}/{PRECOMP_HDR}"
        PRECOMP_OBJ = f"{OUTPUT_DIR}/{PRECOMP_HDR}.gch.o"

        UNAME_IDENT = shell("uname").strip()
        if "clang" in CXX and UNAME_IDENT == "Darwin":
            CLANG_PCH_FASTER = "-fpch-instantiate-templates".split()
            PCH_INCLUDE_FLAGS = ["-include-pch", PRECOMP_GCH]
        else:
            CLANG_PCH_FASTER = []
            PCH_INCLUDE_FLAGS = ["-include", PRECOMP_INCLUDE]

        DEFINES = []
        INCLUDES = "-Isource/include -Iexternal".split()

        USE_FONTCONFIG = UNAME_IDENT == "Linux"
        USE_CORETEXT = UNAME_IDENT == "Darwin"

        OUTPUT_BIN = f"{OUTPUT_DIR}/sap"
        SAP_LIB = f"{OUTPUT_DIR}/sap.o"

        if USE_FONTCONFIG:
            DEFINES += ["-DUSE_FONTCONFIG=1"]
            NONGCH_CXXFLAGS += shell("pkg-config --cflags fontconfig").split()
            LDFLAGS += shell("pkg-config --libs fontconfig").split()

        if USE_CORETEXT:
            DEFINES += ["-DUSE_CORETEXT=1"]
            LDFLAGS += "-framework Foundation -framework CoreText".split()

        # Silence pyright pls thanks
        _ = (
            CAT,
            LINKER_OPT_FLAGS,
            CC,
            LLD,
            CFLAGS,
            CXXFLAGS,
            CXXLIBOBJ,
            CXXDEPS,
            SPECIAL_HDRS_COMPDB,
            CXX_COMPDB,
            TESTDEPS,
            TESTS,
            UTF8PROC_SRCS,
            UTF8PROC_OBJS,
            MINIZ_SRCS,
            MINIZ_OBJS,
            PRECOMP_OBJ,
            CLANG_PCH_FASTER,
            PCH_INCLUDE_FLAGS,
            INCLUDES,
            OUTPUT_BIN,
            SAP_LIB,
        )

        self.__dict__.update(locals())


@functools.cache
def mtime(path: str):
    return os.path.getmtime(path) if os.path.isfile(path) else 0.0


T = TypeVar("T")


def split_into_parts(xs: list[T], n: int) -> list[list[T]]:
    if len(xs) <= n:
        return [[x] for x in xs]
    else:
        return [xs[len(xs) * i // n : len(xs) * (i + 1) // n] for i in range(n)]


def run_jobs(jobs) -> list[threading.Thread]:
    threads = [threading.Thread(None, job) for job in jobs]
    for t in threads:
        t.start()
    return threads


def join_jobs(threads: list[threading.Thread]):
    for t in threads:
        t.join()


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("target", nargs="*")
    parser.add_argument("-j", default=multiprocessing.cpu_count(), type=int)
    parser.add_argument("-v", "--verbose", default=0, action="count")

    args = parser.parse_args()

    if "help" in args.target:
        parser.print_help()
        print(args)
        return

    if args.verbose:
        set_verbosity(args.verbose)

    verboselog(args)

    opt = Options()

    @functools.cache
    def newer_than_bin(path):
        return mtime(path) >= mtime(opt.OUTPUT_BIN)

    @functools.cache
    def cpp_needs_recompile(path, output):
        deppath = f"{opt.OUTPUT_DIR}/{path}.d"
        if not os.path.isfile(deppath):
            # log(
            #     f"warning: no dep file for {path}, if you run with make it'll generate this but otherwise you'll need to generate it manually, unless you don't mind always rebuilding this file"
            # )
            verboselog(f"checking {path}: needs recompile due to no dep file")
            return True
        with open(deppath, "r") as f:
            deps = f.read().replace("\\\n", "").split(":")[1].split()
            veryveryverboselog(f"  {path}:", *deps)
            res = any(mtime(dep) >= mtime(output) for dep in deps)
            if res:
                verboselog(
                    f"checking {path}:",
                    "needs recompile due to",
                    [dep for dep in deps if mtime(dep) >= mtime(output)],
                    "being new",
                )
            else:
                veryverboselog(f"checking {path}: up to date")
            return res

    semaphore = threading.Semaphore(args.j)

    def c_job(obj, src):
        def job():
            with semaphore:
                log(src)
                verboselog("c_job", obj, src)
                shell(f"mkdir -p {os.path.dirname(obj)}")
                execvp(
                    [
                        opt.CC,
                        *opt.CFLAGS,
                        *"-MMD -MP -c -o".split(),
                        obj,
                        src,
                    ]
                )

        return job

    def sap_dep_job(src: str):
        def job():
            target = f"{opt.OUTPUT_DIR}/{src}.o"
            out = f"{opt.OUTPUT_DIR}/{src}.d"
            veryverboselog("# generating deps for", src)
            veryverboseshell(f"mkdir -p {os.path.dirname(out)}")
            # @$(CXX) -include source/include/precompile.h $(CXXFLAGS) $(NONGCH_CXXFLAGS) $(WARNINGS) $(INCLUDES) $(DEFINES) -MMD -MF $@ -E -o - $< >/dev/null
            veryverboseexecvp(
                [
                    opt.CXX,
                    "-include",
                    opt.PRECOMP_HDR,
                    *opt.CXXFLAGS,
                    *opt.NONGCH_CXXFLAGS,
                    *opt.WARNINGS,
                    *opt.WERROR,
                    *opt.DEFINES,
                    *opt.INCLUDES,
                    *"-MMD -MP -MF".split(),
                    out,
                    "-MT",
                    target,
                    *"-E -o /dev/null".split(),
                    src,
                ]
            )

        return job

    def sap_gch_job(gch: str, hdr: str):
        def job():
            with semaphore:
                log("# precompiling header", gch)
                verboselog("sap_gch_job", gch, hdr)
                shell(f"mkdir -p {opt.OUTPUT_DIR}")
                execvp(
                    [
                        opt.CXX,
                        *opt.CXXFLAGS,
                        *opt.NONGCH_CXXFLAGS,
                        *opt.WARNINGS,
                        *opt.WERROR,
                        *opt.INCLUDES,
                        *opt.CLANG_PCH_FASTER,
                        *opt.DEFINES,
                        *"-MMD -MP -x c++-header -o".split(),
                        gch,
                        hdr,
                    ]
                )

        return job

    def sap_src_with_gch_job(i: int, srcs: list[str]):
        def job():
            with semaphore:
                log(*srcs)
                verboselog("sap_src_job", i, srcs)
                shell(f"mkdir -p {opt.OUTPUT_DIR}")
                open(f"{opt.OUTPUT_DIR}/{i}.cpp", "wb").write(
                    subprocess.check_output([opt.CAT, *srcs])
                )
                execvp(
                    [
                        opt.CXX,
                        *opt.PCH_INCLUDE_FLAGS,
                        *opt.CXXFLAGS,
                        *opt.NONGCH_CXXFLAGS,
                        *opt.WARNINGS,
                        *opt.WERROR,
                        *opt.DEFINES,
                        *opt.INCLUDES,
                        *"-c -o".split(),
                        f"{opt.OUTPUT_DIR}/{i}.o",
                        f"{opt.OUTPUT_DIR}/{i}.cpp",
                    ]
                )

        return job

    def sap_src_job(i: int, srcs: list[str]):
        def job():
            with semaphore:
                log(*srcs)
                verboselog("sap_src_job", i, srcs)
                shell(f"mkdir -p {opt.OUTPUT_DIR}")
                open(f"{opt.OUTPUT_DIR}/{i}.cpp", "wb").write(
                    subprocess.check_output([opt.CAT, *srcs])
                )
                execvp(
                    [
                        opt.CXX,
                        "-include",
                        opt.PRECOMP_HDR,
                        *opt.CXXFLAGS,
                        *opt.NONGCH_CXXFLAGS,
                        *opt.WARNINGS,
                        *opt.WERROR,
                        *opt.DEFINES,
                        *opt.INCLUDES,
                        *"-c -o".split(),
                        f"{opt.OUTPUT_DIR}/{i}.o",
                        f"{opt.OUTPUT_DIR}/{i}.cpp",
                    ]
                )

        return job

    def link_new_sap_lib_job(num_lib_objs: int):
        def job():
            with semaphore:
                log(opt.SAP_LIB)
                shell(f"mkdir -p {opt.OUTPUT_DIR}")
                for i in range(num_lib_objs):
                    shell(
                        f"objconv -felf64 {opt.OUTPUT_DIR}/{i}.o {opt.OUTPUT_DIR}/{i}.o.elf"
                    )
                if os.path.isfile(opt.SAP_LIB):
                    shell(f"objconv -felf64 {opt.SAP_LIB} {opt.SAP_LIB}.elf")

                cmd = f"{opt.LLD} --allow-multiple-definition -r -o {opt.SAP_LIB}.merged --start-group"
                for i in range(num_lib_objs):
                    cmd += f" {opt.OUTPUT_DIR}/{i}.o.elf"
                cmd += " --end-group"
                if os.path.isfile(opt.SAP_LIB):
                    cmd += f" {opt.SAP_LIB}.elf"
                shell(cmd)

                shell(f"llvm-objcopy --weaken {opt.SAP_LIB}.merged {opt.SAP_LIB}.weak")

                if opt.UNAME_IDENT == "Darwin":
                    shell(f"objconv -fmac64 {opt.SAP_LIB}.weak {opt.SAP_LIB}.new")
                else:
                    shell(f"objconv -felf64 {opt.SAP_LIB}.weak {opt.SAP_LIB}.new")

        return job

    def link_sap_job(num_lib_objs):
        def job():
            with semaphore:
                log(opt.OUTPUT_BIN)
                shell(f"mkdir -p {opt.OUTPUT_DIR}")
                cmd = [
                    opt.CXX,
                    *opt.CXXFLAGS,
                    *opt.WARNINGS,
                    *opt.WERROR,
                    *opt.DEFINES,
                    *opt.LDFLAGS,
                    "-o",
                    opt.OUTPUT_BIN,
                    *[f"{opt.OUTPUT_DIR}/{i}.o" for i in range(num_lib_objs)],
                    opt.SAP_LIB,
                    *opt.UTF8PROC_OBJS,
                    *opt.MINIZ_OBJS,
                ]
                execvp(cmd)

        return job

    if "build" in args.target or not args.target:  # default target
        outdated_miniz_objs = list(filter(newer_than_bin, opt.MINIZ_OBJS))
        outdated_utf8proc_objs = list(filter(newer_than_bin, opt.UTF8PROC_OBJS))

        if os.path.isfile(opt.SAP_LIB):
            outdated_srcs = [
                src for src in opt.CXXSRC if cpp_needs_recompile(src, opt.SAP_LIB)
            ]
        else:
            outdated_srcs = opt.CXXSRC

        outdated_gch_hdr = (
            [opt.PRECOMP_HDR]
            if not os.path.isfile(opt.PRECOMP_GCH)
            or cpp_needs_recompile(opt.PRECOMP_HDR, opt.PRECOMP_GCH)
            else []
        )

        verboselog(outdated_miniz_objs)
        verboselog(outdated_utf8proc_objs)
        verboselog(outdated_srcs)
        verboselog(outdated_gch_hdr)

        lib_jobs: list[Callable[[], None]] = []
        obj_jobs: list[Callable[[], None]] = []
        dep_jobs: list[Callable[[], None]] = []
        if outdated_miniz_objs:
            for obj, src in zip(opt.MINIZ_OBJS, opt.MINIZ_SRCS):
                lib_jobs.append(c_job(obj, src))
                verboselog("c obj", src)
        if outdated_utf8proc_objs:
            for obj, src in zip(opt.UTF8PROC_OBJS, opt.UTF8PROC_SRCS):
                lib_jobs.append(c_job(obj, src))
                verboselog("c obj", src)

        num_lib_objs = 0
        if outdated_srcs:
            for src in outdated_srcs:
                dep_jobs.append(sap_dep_job(src))
            for i, srcs in enumerate(split_into_parts(outdated_srcs, args.j)):
                num_lib_objs += 1
                if outdated_gch_hdr:
                    obj_jobs.append(sap_src_job(i, srcs))
                else:
                    obj_jobs.append(sap_src_with_gch_job(i, srcs))
        obj_threads = run_jobs(obj_jobs)
        lib_threads = run_jobs(lib_jobs)

        gch_threads = []
        if outdated_gch_hdr:
            shell(f"mkdir -p {os.path.dirname(opt.PRECOMP_INCLUDE)}")
            shell(f"cp {opt.PRECOMP_HDR} {opt.PRECOMP_INCLUDE}")
            gch_threads = run_jobs([sap_gch_job(opt.PRECOMP_GCH, opt.PRECOMP_HDR)])

        dep_threads = run_jobs(dep_jobs)

        join_jobs(obj_threads)
        sap_lib_threads = (
            run_jobs([link_new_sap_lib_job(num_lib_objs)]) if obj_threads else []
        )
        join_jobs(lib_threads)
        join_jobs(sap_lib_threads)
        if sap_lib_threads and os.path.isfile(opt.SAP_LIB + ".new"):
            shell(f"mv {opt.SAP_LIB}.new {opt.SAP_LIB}")
        sap_threads = (
            run_jobs([link_sap_job(0)]) if lib_threads or obj_threads else []
        )
        if obj_threads:
            log("compile_commands.json")
            shell("make compile_commands.json")
        join_jobs(sap_lib_threads)
        join_jobs(gch_threads)
        join_jobs(sap_threads)
        join_jobs(dep_threads)
    else:
        print("idk what target you want man")
        print(args)
        exit(1)


if __name__ == "__main__":
    main()
