#include <map>
#include <string>
#include <vector>

#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags

#include "ogldev_util.h"


#define MAX_NUM_BONES_PER_VERTEX 4

struct VertexBoneData
{
    uint BoneIDs[MAX_NUM_BONES_PER_VERTEX] = { 0 };
    float Weights[MAX_NUM_BONES_PER_VERTEX] = { 0.0f };

    VertexBoneData()
    {
    }

    void AddBoneData(uint BoneID, float Weight)
    {
        for (uint i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(BoneIDs) ; i++) {
            if (Weights[i] == 0.0) {
                BoneIDs[i] = BoneID;
                Weights[i] = Weight;
                //printf("Adding bone %d weight %f at index %i\n", BoneID, Weight, i);
                return;
            }
        }

        // should never get here - more bones than we have space for
        assert(0);
    }
};


std::vector<VertexBoneData> vertex_to_bones;
std::vector<int> mesh_base_vertex;
std::map<std::string,uint> bone_name_to_index_map;

static int space_count = 0;

void print_space()
{
    for (int i = 0 ; i < space_count ; i++) {
        printf(" ");
    }
}


void print_assimp_matrix(const aiMatrix4x4& m)
{
    print_space(); printf("%f %f %f %f\n", m.a1, m.a2, m.a3, m.a4);
    print_space(); printf("%f %f %f %f\n", m.b1, m.b2, m.b3, m.b4);
    print_space(); printf("%f %f %f %f\n", m.c1, m.c2, m.c3, m.c4);
    print_space(); printf("%f %f %f %f\n", m.d1, m.d2, m.d3, m.d4);
}


int get_bone_id(const aiBone* pBone)
{
    int bone_id = 0;
    std::string bone_name(pBone->mName.C_Str());

    if (bone_name_to_index_map.find(bone_name) == bone_name_to_index_map.end()) {
        // Allocate an index for a new bone
        bone_id = bone_name_to_index_map.size();
        bone_name_to_index_map[bone_name] = bone_id;
    }
    else {
        bone_id = bone_name_to_index_map[bone_name];
    }

    return bone_id;
}

void parse_single_bone(int mesh_index, const aiBone* pBone)
{
    printf("      Bone '%s': num vertices affected by this bone: %d\n", pBone->mName.C_Str(), pBone->mNumWeights);

    int bone_id = get_bone_id(pBone);
    //    printf("      Bone id %d\n", bone_id);

    print_assimp_matrix(pBone->mOffsetMatrix);

    for (int i = 0 ; i < pBone->mNumWeights ; i++) {
        //        if (i == 0) printf("\n");
        const aiVertexWeight& vw = pBone->mWeights[i];
        //          printf("       %d: vertex id %d weight %.2f\n", i, vw.mVertexId, vw.mWeight);

        uint global_vertex_id = mesh_base_vertex[mesh_index] + vw.mVertexId;
        //        printf("Vertex id %d ", global_vertex_id);

        assert(global_vertex_id < vertex_to_bones.size());
        vertex_to_bones[global_vertex_id].AddBoneData(bone_id, vw.mWeight);
    }

    printf("\n");
}


void parse_mesh_bones(int mesh_index, const aiMesh* pMesh)
{
    for (int i = 0 ; i < pMesh->mNumBones ; i++) {
        parse_single_bone(mesh_index, pMesh->mBones[i]);
    }
}


void parse_meshes(const aiScene* pScene)
{
    printf("*******************************************************\n");
    printf("Parsing %d meshes\n\n", pScene->mNumMeshes);

    int total_vertices = 0;
    int total_indices = 0;
    int total_bones = 0;

    mesh_base_vertex.resize(pScene->mNumMeshes);

    for (int i = 0 ; i < pScene->mNumMeshes ; i++) {        const aiMesh* pMesh = pScene->mMeshes[i];
        int num_vertices = pMesh->mNumVertices;
        int num_indices = pMesh->mNumFaces * 3;
        int num_bones = pMesh->mNumBones;
        mesh_base_vertex[i] = total_vertices;
        printf("  Mesh %d '%s': vertices %d indices %d bones %d\n\n", i, pMesh->mName.C_Str(), num_vertices, num_indices, num_bones);
        total_vertices += num_vertices;
        total_indices  += num_indices;
        total_bones += num_bones;

        vertex_to_bones.resize(total_vertices);

        if (pMesh->HasBones()) {
            parse_mesh_bones(i, pMesh);
        }

        printf("\n");
    }

    printf("\nTotal vertices %d total indices %d total bones %d\n", total_vertices, total_indices, total_bones);
}


void parse_node(const aiNode* pNode)
{
    print_space(); printf("Node name: '%s' num children %d num meshes %d\n", pNode->mName.C_Str(), pNode->mNumChildren, pNode->mNumMeshes);
    print_space(); printf("Node transformation:\n");
    print_assimp_matrix(pNode->mTransformation);

    space_count += 4;

    for (int i = 0 ; i < pNode->mNumChildren ; i++) {
        printf("\n");
        print_space(); printf("--- %d ---\n", i);
        parse_node(pNode->mChildren[i]);
    }

    space_count -= 4;
}


void parse_heirarchy(const aiScene* pScene)
{
    printf("\n*******************************************************\n");
    printf("Parsing the node heirarchy\n");

    parse_node(pScene->mRootNode);
}


void parse_scene(const aiScene* pScene)
{
    parse_meshes(pScene);

    parse_heirarchy(pScene);
}


int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: %s <model filename>\n", argv[0]);
        return 1;
    }

    char* filename = argv[1];
    Assimp::Importer Importer;
    const aiScene* pScene = Importer.ReadFile(filename, ASSIMP_LOAD_FLAGS);

    if (!pScene) {
        printf("Error parsing '%s': '%s'\n", filename, Importer.GetErrorString());
        return 1;
    }

    parse_scene(pScene);

    return 0;
}
