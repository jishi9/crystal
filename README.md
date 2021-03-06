Crystal
=======

Crystal is a group of algorithms for extracting regions of uniform quadrilateral structure in an unstructured mesh and reorganizing the mesh representation to expose said structure in order to enable
efficient exploitation of the underlying structure, all whilst preserving the mesh model.


# Building a mesh file (.p.part)

A mesh can be built from either a .dat file - e.g. as generated by [naca0012.m](https://github.com/OP2/OP2-Common/blob/master/apps/mesh_generators/naca0012.m) -
 or a .msh file generated by Gmsh.

The mesh builder binary file can be built by running `make run-builder`.

## Example: Build mesh file from a `.dat` file.
```
bin/run-builder build-from-dat example_mesh.dat example_mesh.p.part
```
## Example: Build mesh file from a `.msh` file.
```
bin/run-builder build-from-msh example_mesh.msh example_mesh.p.part
```


# Detecting structured regions

Structure detection is performed on a `.p.part` file, and produces a `.p` file.

For detecting a single structured region use `structure-detection/detect_and_append_structure.py`. For detecting multiple structured regions use `structure-detection/detect_multiple_structure.py`.

Make sure that the following has been performed:

1. The binary file `bin/mesh-printer` has been compiled. If not run `make mesh-printer`

2. The files `obj/protogen/mesh_pb2.py` and `obj/protogen/__init__.py` have been created, along with a symlink `structure-detection/protogen` to the `obj/protogen/` directory. If not run `make obj/protogen/mesh_pb2.py`.

3. The `LD_LIBRARY_PATH` environment variable must have a path to the protocol buffer `lib` directory. Normally this is done simply using `source .env`


## Example: Detect single structured region
Note that single detection may fail and will not be retried.

Run detection using a random seed.
```
structure-detection/detect_and_append_structure.py example_mesh.p.part example_mesh.p
```

Run detection using a system-seeded RNG. Runs are deterministic, including the start node.
```
structure-detection/detect_and_append_structure.py --random_seed=1234 example_mesh.p.part example_mesh.p
```

Run detection using a given start node. Note that the RNG is still system-seeded, so determinism is not guaranteed.
```
structure-detection/detect_and_append_structure.py --start_node=999 example_mesh.p.part example_mesh.p
```

Run detection using a given start node and a given RNG seed. Runs are deterministic.
```
structure-detection/detect_and_append_structure.py --start_node=999 --random_seed=123 example_mesh.p.part example_mesh.p
```


## Example: Detect multiple structured regions
This method is more flexible, as it attempts to detect multiple structured regions, as well as coping with failed detection.

Run detection using a system-seeded RNG, with the default limits.
```
structure-detection/detect_multiple_structure.py example_mesh.p.part example_mesh.p
```

### Flags:
`--random_seed` and `--start_node` are as before. Note that `start_node` determines the first attempted node, the remaining are determined through the RNG.

`--max_regions` determines the maximum number of structured regions to detect. The default value is 50.

`--max_fail` determines the maximum number of consecutive detection failures before giving up. The default value is 25.

`--max_rows` and `--max_cols` determine the maximum number of rows and columns, respectively, that each structured region may have.

`--cells-etc` enables detection of structured cell and edge regions, in addition to structured vertex regions. This is the default behavior.

`--no-cells-etc` results in only structured vertex regions being detected.


# Running airfoil computation

To run the airfoil computation:

1. Create a `.p` file using the above instructions, `example_mesh.p` say.
2. Create the binary file `bin/run-airfoil-computation` using `make run-airfoil-computation`.
3. Make sure that the `LD_LIBRARY_PATH` environment variable has a path to the protocol buffer `lib` directory. Normally this is done simply using `source .env`
4. Run with `bin/run-airfoil-computation example_mesh.p`
