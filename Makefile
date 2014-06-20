# Disable built-in rules
.SUFFIXES:

# Delete target file in case of error, if it was indeed modified
.DELETE_ON_ERROR:

# Compiler flags
INTELFLAGS=-vec-report0  -xHost -static-intel -ipo -inline-level=2 -no-inline-factor

CC = icc
CFLAGS = -Wall -O3 ${INTELFLAGS}

CXX = icpc
CXXFLAGS = -std=c++11 -Wall `pkg-config --cflags protobuf` -O3 ${INTELFLAGS} ${FLAGS}

LFLAGS = `pkg-config --libs-only-L protobuf`
LIBS = `pkg-config --libs-only-l protobuf` -lz


PROTODIR = gen
BINDIR = bin
DATDIR = dat
MESHDIR = meshes


## run-builder
.PHONY: run-builder
run-builder: $(BINDIR)/run-builder

$(BINDIR)/run-builder: run-builder.cc mesh-builder.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o $(PROTODIR)/mesh.pb.cc mesh.h utils.h $(PROTODIR)/mesh.pb.h | $(BINDIR)
	$(CXX) -o $@ $< mesh-builder.cc $(PROTODIR)/mesh.pb.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Mesh-printer
.PHONY: mesh-printer
mesh-printer: $(BINDIR)/mesh-printer

$(BINDIR)/mesh-printer: mesh-printer.cc $(PROTODIR)/mesh.pb.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o mesh.h utils.h | $(BINDIR)
	$(CXX) -o $@ $< $(PROTODIR)/mesh.pb.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Cell computation runner
.PHONY run-cell-computation:
run-cell-computation: $(BINDIR)/run-cell-computation

$(BINDIR)/run-cell-computation: run-cell-computation.cc mesh-builder.cc $(PROTODIR)/mesh.pb.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o mesh.h utils.h cell-computation.h | $(BINDIR)
	$(CXX) -o $@ $< mesh-builder.cc $(PROTODIR)/mesh.pb.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Edge computation runner
.PHONY run-edge-computation:
run-edge-computation: $(BINDIR)/run-edge-computation

$(BINDIR)/run-edge-computation: run-edge-computation.cc mesh-builder.cc $(PROTODIR)/mesh.pb.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o mesh.h utils.h edge-computation.h | $(BINDIR)
	$(CXX) -o $@ $< mesh-builder.cc $(PROTODIR)/mesh.pb.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Airfoil computation runner
.PHONY run-airfoil-computation:
run-airfoil-computation: $(BINDIR)/run-airfoil-computation

$(BINDIR)/run-airfoil-computation: run-airfoil-computation.cc mesh-builder.cc $(PROTODIR)/mesh.pb.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o mesh.h utils.h cell-computation.h edge-computation.h airfoil.h | $(BINDIR)
	$(CXX) -o $@ $< mesh-builder.cc $(PROTODIR)/mesh.pb.cc zip/izipfile.cpp zip/ozipfile.cpp zip/zip.o zip/unzip.o $(CXXFLAGS) $(LFLAGS) $(LIBS)


## Proto files
$(PROTODIR)/%.pb.h $(PROTODIR)/%.pb.cc $(PROTODIR)/%_pb2.py: %.proto | $(PROTODIR)
	protoc $< --cpp_out $(PROTODIR) --python_out $(PROTODIR)

## Zip library
zip/unzip.o: zip/unzip.c
	cd zip && $(CC) -o unzip.o -c unzip.c -lz $(CFLAGS)

zip/zip.o: zip/zip.c
	cd zip && $(CC) -o zip.o -c zip.c -lz $(CFLAGS)


## Directories
$(BINDIR):
	mkdir $(BINDIR)
$(PROTODIR):
	mkdir $(PROTODIR)
$(DATDIR):
	mkdir $(DATDIR)
$(MESHDIR):
	mkdir $(MESHDIR)


## Clean
.PHONY: clean
clean:
	$(RM) -r $(BINDIR)
	$(RM) gen/mesh.pb.h gen/mesh.pb.cc mesh_pb2.py zip/zip.o zip/unzip.o


## Meshes
detect_and_append_structure.py: mesh-printer

# Detect structure
%.p: %.p.part detect_and_append_structure.py
	./detect_and_append_structure.py $(PY_DETECT_FLAGS) $< $@

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
