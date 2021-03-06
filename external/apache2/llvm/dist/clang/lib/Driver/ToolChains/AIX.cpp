//===--- AIX.cpp - AIX ToolChain Implementations ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AIX.h"
#include "Arch/PPC.h"
#include "CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Options.h"
#include "clang/Driver/SanitizerArgs.h"
#include "llvm/Option/ArgList.h"

using AIX = clang::driver::toolchains::AIX;
using namespace clang::driver;
using namespace clang::driver::tools;

using namespace llvm::opt;

void aix::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                               const InputInfo &Output,
                               const InputInfoList &Inputs, const ArgList &Args,
                               const char *LinkingOutput) const {
  const AIX &ToolChain = static_cast<const AIX &>(getToolChain());
  ArgStringList CmdArgs;

  const bool IsArch32Bit = ToolChain.getTriple().isArch32Bit();
  const bool IsArch64Bit = ToolChain.getTriple().isArch64Bit();
  // Only support 32 and 64 bit.
  if (!(IsArch32Bit || IsArch64Bit))
    llvm_unreachable("Unsupported bit width value.");

  // Force static linking when "-static" is present.
  if (Args.hasArg(options::OPT_static))
    CmdArgs.push_back("-bnso");

  // Specify linker output file.
  assert((Output.isFilename() || Output.isNothing()) && "Invalid output.");
  if (Output.isFilename()) {
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());
  } 

  // Set linking mode (i.e., 32/64-bit) and the address of
  // text and data sections based on arch bit width.
  if (IsArch32Bit) {
    CmdArgs.push_back("-b32");
    CmdArgs.push_back("-bpT:0x10000000");
    CmdArgs.push_back("-bpD:0x20000000");
  } else {
    // Must be 64-bit, otherwise asserted already.
    CmdArgs.push_back("-b64");
    CmdArgs.push_back("-bpT:0x100000000");
    CmdArgs.push_back("-bpD:0x110000000");
  }

  auto getCrt0Basename = [&Args, IsArch32Bit] {
    // Enable gprofiling when "-pg" is specified.
    if (Args.hasArg(options::OPT_pg))
      return IsArch32Bit ? "gcrt0.o" : "gcrt0_64.o";
    // Enable profiling when "-p" is specified.
    else if (Args.hasArg(options::OPT_p))
      return IsArch32Bit ? "mcrt0.o" : "mcrt0_64.o";
    else
      return IsArch32Bit ? "crt0.o" : "crt0_64.o";
  };

  if (!Args.hasArg(options::OPT_nostdlib)) {
    CmdArgs.push_back(
        Args.MakeArgString(ToolChain.GetFilePath(getCrt0Basename())));
  }

  // Specify linker input file(s).
  AddLinkerInputs(ToolChain, Inputs, Args, CmdArgs, JA);

  // Add directory to library search path.
  Args.AddAllArgs(CmdArgs, options::OPT_L);
  ToolChain.AddFilePathLibArgs(Args, CmdArgs);

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
    // Support POSIX threads if "-pthreads" or "-pthread" is present.
    if (Args.hasArg(options::OPT_pthreads, options::OPT_pthread))
      CmdArgs.push_back("-lpthreads");

    CmdArgs.push_back("-lc");
  }

  const char *Exec = Args.MakeArgString(ToolChain.GetLinkerPath());
  C.addCommand(std::make_unique<Command>(JA, *this, Exec, CmdArgs, Inputs));
}

/// AIX - AIX tool chain which can call ld(1) directly.
// TODO: Enable direct call to as(1).
AIX::AIX(const Driver &D, const llvm::Triple &Triple, const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  getFilePaths().push_back(getDriver().SysRoot + "/usr/lib");
}

auto AIX::buildLinker() const -> Tool * { return new aix::Linker(*this); }
