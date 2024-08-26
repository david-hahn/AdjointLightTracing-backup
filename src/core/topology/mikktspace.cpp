#include <tamashii/core/topology/topology.hpp>
#include <mikktspace.h>
#include <tamashii/core/scene/model.hpp>

T_BEGIN_NAMESPACE
class MikkTSpaceTangents {

public:
    MikkTSpaceTangents();
    void calculate(Mesh* aMesh);

private:

    SMikkTSpaceInterface iface{};
    SMikkTSpaceContext mContext{};

    static int get_vertex_index(const SMikkTSpaceContext* aContext, int iFace, int iVert);

    static int get_num_faces(const SMikkTSpaceContext* aContext);
    static int get_num_vertices_of_face(const SMikkTSpaceContext* aContext, int iFace);
    static void get_position(const SMikkTSpaceContext* aContext, float* outpos, int iFace, int iVert);

    static void get_normal(const SMikkTSpaceContext* aContext, float* outnormal, int iFace, int iVert);

    static void get_tex_coords(const SMikkTSpaceContext* aContext, float* outuv, int iFace, int iVert);

    static void set_tspace_basic(const SMikkTSpaceContext* aContext, const float* tangentu, float fSign, int iFace, int iVert);

};
T_END_NAMESPACE

T_USE_NAMESPACE
void topology::calcMikkTSpaceTangents(Mesh* aMesh) {
    switch (aMesh->getTopology()) {
        case Mesh::Topology::POINT_LIST:
        case Mesh::Topology::LINE_LIST:
        case Mesh::Topology::LINE_STRIP:
            return;
        case Mesh::Topology::TRIANGLE_STRIP:
        case Mesh::Topology::TRIANGLE_FAN:
            spdlog::error("MikkTSpace: currently only implemented for TRIANGLE_LIST");
            return;
    };

    if (!aMesh->hasTexCoords0()) spdlog::warn("MikkTSpace: Mesh has no UVs");
    MikkTSpaceTangents mtst;
    mtst.calculate(aMesh);
}

MikkTSpaceTangents::MikkTSpaceTangents()
{
    iface.m_getNumFaces = get_num_faces;
    iface.m_getNumVerticesOfFace = get_num_vertices_of_face;

    iface.m_getNormal = get_normal;
    iface.m_getPosition = get_position;
    iface.m_getTexCoord = get_tex_coords;
    iface.m_setTSpaceBasic = set_tspace_basic;

    mContext.m_pInterface = &iface;
}

void MikkTSpaceTangents::calculate(Mesh* aMesh) {
    mContext.m_pUserData = aMesh;

    genTangSpaceDefault(&this->mContext);
    aMesh->hasTangents(true);
}

int MikkTSpaceTangents::get_num_faces(const SMikkTSpaceContext* aContext) {
	const Mesh* working_mesh = static_cast<Mesh*> (aContext->m_pUserData);

    const int i_size = static_cast<int>(working_mesh->getPrimitiveCount());
    return i_size;
}

int MikkTSpaceTangents::get_num_vertices_of_face(const SMikkTSpaceContext* aContext,
    const int iFace) {
	const Mesh* working_mesh = static_cast<Mesh*> (aContext->m_pUserData);

    if (working_mesh->getTopology() == Mesh::Topology::TRIANGLE_LIST) {
        return 3;
    }
    throw std::logic_error("no vertices with less than 3 and more than 3 supported");
}

void MikkTSpaceTangents::get_position(const SMikkTSpaceContext* aContext,
    float* outpos,
    const int iFace, const int iVert) {

    Mesh* workingMesh = static_cast<Mesh*> (aContext->m_pUserData);

    const auto index = get_vertex_index(aContext, iFace, iVert);
    const auto vertex = workingMesh->getVerticesArray()[index];

    outpos[0] = vertex.position.x;
    outpos[1] = vertex.position.y;
    outpos[2] = vertex.position.z;
}

void MikkTSpaceTangents::get_normal(const SMikkTSpaceContext* aContext,
    float* outnormal,
    const int iFace, const int iVert) {
    Mesh* workingMesh = static_cast<Mesh*> (aContext->m_pUserData);

    const auto index = get_vertex_index(aContext, iFace, iVert);
    const auto vertex = workingMesh->getVerticesArray()[index];

    outnormal[0] = vertex.normal.x;
    outnormal[1] = vertex.normal.y;
    outnormal[2] = vertex.normal.z;
}

void MikkTSpaceTangents::get_tex_coords(const SMikkTSpaceContext* aContext,
    float* outuv,
    const int iFace, const int iVert) {
    Mesh* workingMesh = static_cast<Mesh*> (aContext->m_pUserData);

    const auto index = get_vertex_index(aContext, iFace, iVert);
    const auto vertex = workingMesh->getVerticesArray()[index];

    outuv[0] = vertex.texture_coordinates_0.x;
    outuv[1] = vertex.texture_coordinates_0.y;
}

void MikkTSpaceTangents::set_tspace_basic(const SMikkTSpaceContext* aContext,
    const float* tangentu,
    const float fSign, const int iFace, const int iVert) {
    Mesh* workingMesh = static_cast<Mesh*> (aContext->m_pUserData);

    const auto index = get_vertex_index(aContext, iFace, iVert);
    auto* vertex = &workingMesh->getVerticesArray()[index];

    vertex->tangent.x = tangentu[0];
    vertex->tangent.y = tangentu[1];
    vertex->tangent.z = tangentu[2];
    vertex->tangent.w = fSign;
}

int MikkTSpaceTangents::get_vertex_index(const SMikkTSpaceContext* aContext, const int iFace, const int iVert) {
    Mesh* workingMesh = static_cast<Mesh*> (aContext->m_pUserData);

    const auto faceSize = get_num_vertices_of_face(aContext, iFace);

    const auto indicesIndex = (iFace * faceSize) + iVert;

    if (workingMesh->hasIndices()) return workingMesh->getIndicesArray()[indicesIndex];
    else return indicesIndex;
}
