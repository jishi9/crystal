#include "mesh-builder.h"
#include "utils.h"


int main(int argc, const char* argv[]) {
  const char* usage = "USAGE:\n"
    "    bin/run-builder [build-from|build-from-msh] [in_filename] [out_filename]\n";
// Obsolete options
//    "    bin/run-builder [test|testquad] [filename]\n"
//    "    bin/run-builder build [35|big] [filename]\n"
  check(argc >= 3, usage);

  // Read mesh
  if (match("test", argv[1])) {
    check(argc == 3, usage);
    const char* filename = argv[2];
    readMesh<4>(filename);
  }

  // Read quad mesh
  else if (match("testquad", argv[1])) {
    check(argc == 3, usage);
    const char* filename = argv[2];
    readQuadMesh(filename);
  }

  // Generate quad mesh
  else if (match("build", argv[1])) {
    check(argc == 4, usage);
    const char* filename = argv[3];

    if (match("35", argv[2])) {
      build35Mesh(filename);
    }
    else if (match("big", argv[2])) {
      buildBigMesh(filename);
    }
    else {
      check(false, "Invalid build option");
    }
  }

  // Build quad mesh from dat file
  else if (match("build-from", argv[1])) {
    check(argc == 4, usage);

    const char* in_filename = argv[2];
    const char* out_filename = argv[3];

    buildQuadMeshPart(in_filename, out_filename);
  }

  // Build quad mesh from msh file
  else if (match("build-from-msh", argv[1])) {
    check(argc == 4, usage);

    const char* in_filename = argv[2];
    const char* out_filename = argv[3];

    buildQuadMeshPartFromMshFile(in_filename, out_filename);
  }


  else {
    check(false, usage);
  }
}
