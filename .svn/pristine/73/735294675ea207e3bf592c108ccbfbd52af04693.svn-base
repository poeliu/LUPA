##===- projects/sample/tools/sample/Makefile ---------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of the tool.
#
TOOLNAME=lupa

#
# List libraries that we'll need
# We use LIBS because sample is a dynamic library.
#
USEDLIBS = core.a alias.a util.a statistic.a

#
# List llvm libraries that we'll need
#
LLVMLIBS = LLVMSupport.a LLVMCore.a LLVMBitReader.a LLVMAsmParser.a LLVMAnalysis.a LLVMTransformUtils.a LLVMScalarOpts.a LLVMTarget.a

#
# Link all of the libraries
#
LINK_COMPONENTS = all
#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

