# Disable built-in rules
.SUFFIXES:

# Delete target file in case of error, if it was indeed modified
.DELETE_ON_ERROR:



## Source directories
CORECOMP = core-computation
STRUCTUREDETECT = structure-detection
PROTODEFDIR = proto-def
DATDIR = dat
ZIP_SRC_PATH = third-party/zip
PROTO_SRC_PATH = third-party/protobuf-2.5.0

## Generated directories
BINDIR = bin
MESHDIR = meshes
PROTOGENDIR = obj/protogen
PROTO_LIB_PATH = obj/protobuf
ZIP_LIB_PATH = obj/zip

## Create directories
$(BINDIR):
	mkdir -p $(BINDIR)
$(PROTOGENDIR):
	mkdir -p $(PROTOGENDIR)
$(DATDIR):
	mkdir -p $(DATDIR)
$(MESHDIR):
	mkdir -p $(MESHDIR)
$(ZIP_LIB_PATH):
	mkdir -p $(ZIP_LIB_PATH)


## Enable automatic package flags
# **NOTE**
#   To use your system-wide  installation of protocol buffers, simply deleting
#   the assignment to PKG_CONFIG_PATH would likely work.
#
#   If you have a custom installation location, set PKG_LIB_PATH.
#
#   ALSO make sure to update LD_LIBRARY_PATH (in .env) accordingly
export PKG_CONFIG_PATH = ${PROTO_LIB_PATH}/lib/pkgconfig/



## Compiler flags
INTELFLAGS=-vec-report0 -xHost -static-intel -ipo -inline-level=2 -no-inline-factor

CC = icc
CFLAGS = -Wall -O3 ${INTELFLAGS}

CXX = icpc
CXXFLAGS = -std=c++11 -Wall -Ithird-party -Iobj `pkg-config --cflags protobuf` -O3 ${INTELFLAGS} ${FLAGS}

LFLAGS = `pkg-config --libs-only-L protobuf`
LIBS = `pkg-config --libs-only-l protobuf` -lz



## Zip library
$(ZIP_LIB_PATH)/unzip.o: $(ZIP_SRC_PATH)/unzip.c | $(ZIP_LIB_PATH)
	cd $(ZIP_SRC_PATH) && $(CC) -o ${CURDIR}/$@ -c unzip.c -lz $(CFLAGS)

$(ZIP_LIB_PATH)/zip.o: $(ZIP_SRC_PATH)/zip.c | $(ZIP_LIB_PATH)
	cd $(ZIP_SRC_PATH) && $(CC) -o ${CURDIR}/$@ -c zip.c -lz $(CFLAGS)



## Protocol buffers library
$(PROTO_LIB_PATH)/bin/protoc $(PROTO_LIB_PATH)/lib $(PROTO_LIB_PATH)/include:
	mkdir -p $(PROTO_LIB_PATH)
	cd "${PROTO_SRC_PATH}" && ./configure --prefix="${CURDIR}/${PROTO_LIB_PATH}"
	$(MAKE) -C ${PROTO_SRC_PATH}
	$(MAKE) -C ${PROTO_SRC_PATH} check
	$(MAKE) -C ${PROTO_SRC_PATH} install


## run-builder
.PHONY: run-builder
run-builder: $(BINDIR)/run-builder

$(BINDIR)/run-builder: ${CORECOMP}/run-builder.cc ${CORECOMP}/mesh-builder.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o $(PROTOGENDIR)/mesh.pb.cc ${CORECOMP}/mesh.h ${CORECOMP}/utils.h $(PROTOGENDIR)/mesh.pb.h | $(BINDIR)
	$(CXX) -o $@ $< ${CORECOMP}/mesh-builder.cc $(PROTOGENDIR)/mesh.pb.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Mesh-printer
.PHONY: mesh-printer
mesh-printer: $(BINDIR)/mesh-printer

$(BINDIR)/mesh-printer: ${CORECOMP}/mesh-printer.cc $(PROTOGENDIR)/mesh.pb.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o ${CORECOMP}/mesh.h ${CORECOMP}/utils.h | $(BINDIR)
	$(CXX) -o $@ $< $(PROTOGENDIR)/mesh.pb.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Cell computation runner
.PHONY run-cell-computation:
run-cell-computation: $(BINDIR)/run-cell-computation

$(BINDIR)/run-cell-computation: ${CORECOMP}/run-cell-computation.cc ${CORECOMP}/mesh-builder.cc $(PROTOGENDIR)/mesh.pb.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o ${CORECOMP}/mesh.h ${CORECOMP}/utils.h ${CORECOMP}/cell-computation.h | $(BINDIR)
	$(CXX) -o $@ $< ${CORECOMP}/mesh-builder.cc $(PROTOGENDIR)/mesh.pb.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Edge computation runner
.PHONY run-edge-computation:
run-edge-computation: $(BINDIR)/run-edge-computation

$(BINDIR)/run-edge-computation: ${CORECOMP}/run-edge-computation.cc ${CORECOMP}/mesh-builder.cc $(PROTOGENDIR)/mesh.pb.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o ${CORECOMP}/mesh.h ${CORECOMP}/utils.h ${CORECOMP}/edge-computation.h | $(BINDIR)
	$(CXX) -o $@ $< ${CORECOMP}/mesh-builder.cc $(PROTOGENDIR)/mesh.pb.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Airfoil computation runner
.PHONY run-airfoil-computation:
run-airfoil-computation: $(BINDIR)/run-airfoil-computation

$(BINDIR)/run-airfoil-computation: ${CORECOMP}/run-airfoil-computation.cc ${CORECOMP}/mesh-builder.cc $(PROTOGENDIR)/mesh.pb.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o ${CORECOMP}/mesh.h ${CORECOMP}/utils.h ${CORECOMP}/cell-computation.h ${CORECOMP}/edge-computation.h ${CORECOMP}/airfoil.h | $(BINDIR)
	$(CXX) -o $@ $< ${CORECOMP}/mesh-builder.cc $(PROTOGENDIR)/mesh.pb.cc $(ZIP_SRC_PATH)/izipfile.cpp $(ZIP_SRC_PATH)/ozipfile.cpp $(ZIP_LIB_PATH)/zip.o $(ZIP_LIB_PATH)/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Proto files
$(PROTOGENDIR)/%.pb.h $(PROTOGENDIR)/%.pb.cc $(PROTOGENDIR)/%_pb2.py: $(PROTODEFDIR)/%.proto | $(PROTO_LIB_PATH)/bin/protoc $(PROTO_LIB_PATH)/lib $(PROTO_LIB_PATH)/include $(PROTOGENDIR)
	cd $(PROTODEFDIR) && $(CURDIR)/$(PROTO_LIB_PATH)/bin/protoc $*.proto --cpp_out $(CURDIR)/$(PROTOGENDIR) --python_out $(CURDIR)/$(PROTOGENDIR)
	echo "# This file allows importing python modules from the directory it is in." > $(PROTOGENDIR)/__init__.py



## Clean
.PHONY: clean
clean:
	$(RM) -r $(BINDIR) $(PROTOGENDIR)


.PHONY: cleanlibs
cleanlibs:
	$(RM) -r $(PROTO_LIB_PATH) $(ZIP_LIB_PATH)
	ls $(PROTO_SRC_PATH)
	if [ -f $(PROTO_SRC_PATH)/Makefile ] ; then $(MAKE) -C $(PROTO_SRC_PATH) distclean ; fi


## Meshes
detect_and_append_structure.py: mesh-printer

# Detect structure
%.p: %.p.part $(STRUCTUREDETECT)/detect_and_append_structure.py
	$(STRUCTUREDETECT)/detect_and_append_structure.py $(PY_DETECT_FLAGS) $< $@

# Special flags
$(MESHDIR)/airfoil.p: PY_DETECT_FLAGS = --random_seed 123467
$(MESHDIR)/airfoil_refilled.p: PY_DETECT_FLAGS = --random_seed 680349 # Corresponds to 123467
$(MESHDIR)/35mesh.p: PY_DETECT_FLAGS = --random_seed 123467
$(MESHDIR)/bigmesh.p: PY_DETECT_FLAGS = --random_seed 123467
$(MESHDIR)/side-skew.p: PY_DETECT_FLAGS = --random_seed 1 --start_node 3

.PHONY: airfoil
airfoil: $(MESHDIR)/airfoil.p | $(MESHDIR)

.PHONY: airfoil_refilled
airfoil_refilled: $(MESHDIR)/airfoil_refilled.p | $(MESHDIR)

.PHONY: 35mesh
35mesh: $(MESHDIR)/35mesh.p | $(MESHDIR)
	echo TODO: 35mesh
	exit 1

.PHONY: bigmesh
bigmesh: $(MESHDIR)/bigmesh.p | $(MESHDIR)
	echo TODO: bigmesh
	exit 1

.PHONY: side-skew
side-skew: $(MESHDIR)/side-skew.p | $(MESHDIR)

# Mesh parts (without structure detection)
$(MESHDIR)/airfoil.p.part: $(BINDIR)/run-builder $(DATDIR)/airfoil.dat | $(MESHDIR)
	$(BINDIR)/run-builder build-from $(DATDIR)/airfoil.dat $@

$(MESHDIR)/airfoil_refilled.p.part: $(BINDIR)/run-builder $(DATDIR)/airfoil_refilled.dat | $(MESHDIR)
	$(BINDIR)/run-builder build-from $(DATDIR)/airfoil_refilled.dat $@

$(MESHDIR)/35mesh.p.part: $(BINDIR)/run-builder | $(MESHDIR)
	$(BINDIR)/run-builder build 35 $@

$(MESHDIR)/bigmesh.p.part: $(BINDIR)/run-builder | $(MESHDIR)
	$(BINDIR)/run-builder build big $@

$(MESHDIR)/side-skew.p.part: $(BINDIR)/run-builder gen-sideskewmesh.py | $(MESHDIR)
	./gen-sideskewmesh.py | $(BINDIR)/run-builder build-from /dev/stdin $@
