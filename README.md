# MAT Modules
Some commonly used libraries for computing the 3D medial axis given a 3D triangle mesh (or tetrahedral mesh).

Starter code is from the paper "[MATFP](https://github.com/ningnawang/MATFP): Computing Medial Axis Transform with Feature Preservation via Restricted Power Diagram".

The extended work "[MATTopo](https://github.com/ningnawang/mattopo): Topology-preserving Medial Axis Transform with Restricted Power Diagram" heavily replies on this repo using tag **v1.0**.

## 1. Lib using GPU (set option **USE_GPU** as ON)

### 1.1. dist2mat 
Given a 3D sample, compute its closest medial element (sphere/cone/slab) on the given medial mesh.

### 1.2. rpd3d & rpd3d_api
Libs and APIs for computing 3D RPD using CUDA.

### 1.3. matfun_fix



### 1.4. IO_CUDA
Some IO related libs related to CUDA 3D RPD output.

## 2. Lib NOT using GPU (set option **USE_GPU** as OFF)
### 2.1. inputs
- SurfaceMesh (extends GEO::Mesh)
- TetMesh
- AABBWrapper
- Sharp/Concave Feature Detection

### 2.2. matbase
- medial_sphere
- medial_primitives
- medial_mesh


### 2.4. matfun
- shrinking
- updating (sphere-optimization)
- thinning

### 2.3. IO_CXX
Some IO related libs.

### 2.5. rpd3d_base
- 3D triangulation using CGAL
- ConvexCellHost

## TODO list:
1. Update CMakeLists.txt to remove some unused external dependencies, such as `polyscope`.
2. write more docs 
