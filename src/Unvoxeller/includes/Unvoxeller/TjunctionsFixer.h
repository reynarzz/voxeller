#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <unordered_map>
#include <vector>
#include <array>
#include <cmath> 

//---------------------------------------------------------------------------
// Typedefs
//---------------------------------------------------------------------------

//using TriMesh = OpenMesh::TriMesh_ArrayKernelT<>;
using Vec3f   = OpenMesh::Vec3f;
using EHandle = TriMesh::EdgeHandle;
using VHandle = TriMesh::VertexHandle;
using Point   = TriMesh::Point;

// 3‐integer key for a grid cell
struct GridKey {
    int x, y, z;
    bool operator==(GridKey const& o) const {
        return x==o.x && y==o.y && z==o.z;
    }
};

// Hash for GridKey
struct GridKeyHash {
    size_t operator()(GridKey const& k) const noexcept {
        // Classic 3D hashing
        size_t h = std::hash<int>()(k.x);
        h = h * 31 + std::hash<int>()(k.y);
        h = h * 31 + std::hash<int>()(k.z);
        return h;
    }
};

//---------------------------------------------------------------------------
// Helper: compute grid key from point
//---------------------------------------------------------------------------

static GridKey key_of(const Vec3f &p, float inv_cell) {
    return {
        int(std::floor(p[0] * inv_cell)),
        int(std::floor(p[1] * inv_cell)),
        int(std::floor(p[2] * inv_cell))
    };
}

//---------------------------------------------------------------------------
// Main function
//---------------------------------------------------------------------------

void splitTJunctionsFast(TriMesh& mesh, float eps = 1e-4f) {
    using EHandle = TriMesh::EdgeHandle;
    using VHandle = TriMesh::VertexHandle;
    using Point = TriMesh::Point;

    // 1) Request and compute normals (for interpolation)
    mesh.request_vertex_normals();
    mesh.request_vertex_texcoords2D();
    mesh.update_normals();

    // 2) Collect boundary edges
    std::vector<EHandle> bEdges;
    bEdges.reserve(mesh.n_edges() / 10);
    for (auto eh : mesh.edges()) {
        if (!mesh.status(eh).deleted() && mesh.is_boundary(eh))
            bEdges.push_back(eh);
    }
    if (bEdges.empty()) {
        mesh.release_vertex_normals();
        mesh.release_vertex_texcoords2D();
        return;
    }

    // 3) Compute average boundary‐edge length → cell size
    double sumLen = 0.0;
    for (auto eh : bEdges) {
        auto heh = mesh.halfedge_handle(eh, 0);
        auto p0 = mesh.point(mesh.from_vertex_handle(heh));
        auto p1 = mesh.point(mesh.to_vertex_handle(heh));
        sumLen += (p1 - p0).norm();
    }
    float avgLen = float(sumLen / bEdges.size());
    float cell_size = avgLen * 2.0f;        // tweak as desired
    float inv_cell = 1.0f / cell_size;
    float eps2 = eps * eps;

    // 4) Build the uniform‐grid: map cell → boundary‐edge list
    std::unordered_map<GridKey, std::vector<EHandle>, GridKeyHash> grid;
    grid.reserve(bEdges.size() * 2);
    for (auto eh : bEdges) {
        auto heh = mesh.halfedge_handle(eh, 0);
        Point p0 = mesh.point(mesh.from_vertex_handle(heh));
        Point p1 = mesh.point(mesh.to_vertex_handle(heh));
        Point minb = p0.minimize(p1) - Point(eps, eps, eps);
        Point maxb = p0.maximize(p1) + Point(eps, eps, eps);

        GridKey k0 = key_of(minb, inv_cell);
        GridKey k1 = key_of(maxb, inv_cell);
        for (int x = k0.x; x <= k1.x; ++x)
            for (int y = k0.y; y <= k1.y; ++y)
                for (int z = k0.z; z <= k1.z; ++z) {
                    grid[{x, y, z}].push_back(eh);
                }
    }

    // 5) Detect phase (parallel, early‐exit per vertex)
    struct SplitOp { EHandle eh; Point pos; float t; };
    std::vector<SplitOp> ops;
    ops.reserve(mesh.n_vertices() / 10);

#pragma omp parallel
    {
        std::vector<SplitOp> local_ops;
#pragma omp for schedule(dynamic)
        for (int vidx = 0; vidx < (int)mesh.n_vertices(); ++vidx) {
            auto vh = mesh.vertex_handle(vidx);
            if (mesh.status(vh).deleted()) continue;

            Point p = mesh.point(vh);
            GridKey kc = key_of(p, inv_cell);
            bool found = false;

            for (int dx = -1; dx <= 1 && !found; ++dx)
                for (int dy = -1; dy <= 1 && !found; ++dy)
                    for (int dz = -1; dz <= 1 && !found; ++dz) {
                        GridKey k{ kc.x + dx, kc.y + dy, kc.z + dz };
                        auto it = grid.find(k);
                        if (it == grid.end()) continue;

                        for (auto eh : it->second) {
                            if (mesh.status(eh).deleted()) continue;
                            auto heh = mesh.halfedge_handle(eh, 0);
                            auto v0 = mesh.from_vertex_handle(heh);
                            auto v1 = mesh.to_vertex_handle(heh);
                            if (vh == v0 || vh == v1) continue;

                            Point p0 = mesh.point(v0), p1 = mesh.point(v1);
                            auto dir = p1 - p0;
                            float d2 = dir.sqrnorm();
                            if (d2 < 1e-12f) continue;

                            float t = ((p - p0) | dir) / d2;
                            if (t <= 0.0f || t >= 1.0f) continue;

                            Point proj = p0 + dir * t;
                            if ((proj - p).sqrnorm() > eps2) continue;

                            local_ops.push_back({ eh, proj, t });
                            found = true;
                            break;
                        }
                    }
        }
#pragma omp critical
        ops.insert(ops.end(), local_ops.begin(), local_ops.end());
    }

    // 6) Split phase (sequential to avoid iterator invalidation)
    for (auto& op : ops) {
        if (mesh.status(op.eh).deleted()) continue;
        auto heh = mesh.halfedge_handle(op.eh, 0);
        auto v0 = mesh.from_vertex_handle(heh);
        auto v1 = mesh.to_vertex_handle(heh);

        auto n0 = mesh.normal(v0), n1 = mesh.normal(v1);
        auto uv0 = mesh.texcoord2D(v0), uv1 = mesh.texcoord2D(v1);

        auto new_vh = mesh.split(op.eh, op.pos);
        mesh.set_normal(new_vh, n0 + (n1 - n0) * op.t);
        mesh.set_texcoord2D(new_vh, uv0 + (uv1 - uv0) * op.t);
    }

    // 7) Clean up
    mesh.release_vertex_normals();
    mesh.release_vertex_texcoords2D();
}