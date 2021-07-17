#pragma once
#include "AssimpWrapper.h"
#include "Mesh.h"
#include "Skeleton.h"


namespace MeshImporter
{

	static VertSignature CreateVertSignature(aiMesh* aiMesh)
	{
		VertSignature vertSig;

		// Can this even not?
		if (aiMesh->HasPositions())
			vertSig.addAttribute(VAttribSemantic::POS, VAttribType::FLOAT3);

		// Texture coordinates, slightly more involved but still simple, preserves original layout.
		UINT numUVChannels = aiMesh->GetNumUVChannels();

		UINT prevNumComponents = 0u;	// Will never be equal to the actual data the first time.
		VAttribType uvwType;

		for (int i = 0; i < numUVChannels; ++i)
		{
			UINT uvw = aiMesh->mNumUVComponents[i];

			if (uvw != prevNumComponents)
			{
				switch (uvw)
				{
				case 1: uvwType = VAttribType::FLOAT;	break;
				case 2: uvwType = VAttribType::FLOAT2;	break;
				case 3: uvwType = VAttribType::FLOAT3;	break;
				}

				vertSig.addAttribute(VAttribSemantic::TEX_COORD, uvwType);
			}
			else
			{
				++(vertSig._attributes.back()._numElements);
			}
			prevNumComponents = uvw;
		}

		// Normals, quite simple.
		if (aiMesh->HasNormals())
			vertSig.addAttribute(VAttribSemantic::NORMAL, VAttribType::FLOAT3);

		// Assimp ensures these two attributes both exist if either does.
		if (aiMesh->HasTangentsAndBitangents())
		{
			vertSig.addAttribute(VAttribSemantic::TANGENT, VAttribType::FLOAT3);
			vertSig.addAttribute(VAttribSemantic::BITANGENT, VAttribType::FLOAT3);
		}

		// For skeletal meshes, this is needed. Doesn't work yet
		if (aiMesh->HasBones())
		{
			vertSig.addAttribute(VAttribSemantic::B_IDX, VAttribType::UINT4);
			vertSig.addAttribute(VAttribSemantic::B_WEIGHT, VAttribType::FLOAT4);
		}

		return vertSig;
	}


	static void ImportVertexData(VertSignature vertSig, std::vector<uint8_t>& vertPool, aiMesh* aiMesh, const Skeleton* skeleton = nullptr)
	{
		UINT vertByteWidth = vertSig.getVertByteWidth();
		UINT vertPoolSize = vertByteWidth * aiMesh->mNumVertices;

		vertPool.resize(vertPoolSize);	// Memcpy doesn't increase size() so we hackerino
		
		// Pack interleaved, starting with positions
		if (aiMesh->HasPositions())
		{
			UINT posOffset = vertSig.getOffsetOf(VAttribSemantic::POS);
			uint8_t* dst = vertPool.data() + posOffset;
			for (UINT i = 0; i < aiMesh->mNumVertices; ++i)
			{
				memcpy(dst, &aiMesh->mVertices[i], sizeof(aiVector3D));
				dst += vertByteWidth;
			}
		}

		// There are potentially multiple texture coordinate channels, to be stored at an accumulating offset
		// Initial offset is the offset to first channel, all channels will always be contiguous per vertex
		if (vertSig.countAttribute(VAttribSemantic::TEX_COORD))
		{
			UINT tcOffset = vertSig.getOffsetOf(VAttribSemantic::TEX_COORD);
			UINT numUVChannels = aiMesh->GetNumUVChannels();

			for (UINT i = 0; i < numUVChannels; ++i)
			{
				// Each tc set can have a size of 1, 2 or 3 floats (u, uv, uvw) which is copied every time
				UINT tcByteWidth = aiMesh->mNumUVComponents[i] * sizeof(float);

				// The offset of this texture coordinate channel in the first vertex is specified here
				uint8_t* dst = vertPool.data() + tcOffset;

				for (UINT j = 0; j < aiMesh->mNumVertices; ++j)
				{
					memcpy(dst, &aiMesh->mTextureCoords[i][j], tcByteWidth);
					dst += vertByteWidth;
				}
				// For every new set, we shift the offset again by the size of the previously written set
				tcOffset += tcByteWidth;
			}
		}


		if (aiMesh->HasNormals())
		{
			UINT nrmOffset = vertSig.getOffsetOf(VAttribSemantic::NORMAL);
			uint8_t* dst = vertPool.data() + nrmOffset;
			for (UINT i = 0; i < aiMesh->mNumVertices; ++i)
			{
				memcpy(dst, &aiMesh->mNormals[i], sizeof(aiVector3D));
				dst += vertByteWidth;
			}
		}


		if (aiMesh->HasTangentsAndBitangents())
		{
			UINT tanOffset = vertSig.getOffsetOf(VAttribSemantic::TANGENT);
			uint8_t* dst = vertPool.data() + tanOffset;
			for (UINT i = 0; i < aiMesh->mNumVertices; ++i)
			{
				memcpy(dst, &aiMesh->mTangents[i], sizeof(aiVector3D));
				dst += vertByteWidth;
			}

			UINT btOffset = vertSig.getOffsetOf(VAttribSemantic::BITANGENT);
			dst = vertPool.data() + btOffset;
			for (UINT i = 0; i < aiMesh->mNumVertices; ++i)
			{
				memcpy(dst, &aiMesh->mBitangents[i], sizeof(aiVector3D));
				dst += vertByteWidth;
			}
		}


		if (aiMesh->HasBones() && skeleton)
		{
			UINT biOffset = vertSig.getOffsetOf(VAttribSemantic::B_IDX);

			for (UINT i = 0; i < aiMesh->mNumBones; ++i)
			{
				aiBone* aiBone = aiMesh->mBones[i];
				uint32_t boneIndex = skeleton->getInfluenceBoneIndex(aiBone->mName.C_Str());

				for (UINT j = 0; j < aiBone->mNumWeights; ++j)
				{
					float weight = aiBone->mWeights[j].mWeight;

					uint32_t vertID = aiBone->mWeights[j].mVertexId;
					uint32_t vertByteOffset = vertID * vertByteWidth;
					
					uint8_t* boneDataPtr = vertPool.data() + biOffset + vertByteOffset;
					VertBoneData* vbd = reinterpret_cast<VertBoneData*>(boneDataPtr);
					vbd->addBoneData(boneIndex, weight);
				}
			}
		}
	}


	static Mesh ImportFromAssimp(aiMesh* aiMesh, Skeleton* skeleton, const std::string& path)
	{
		Mesh mesh;

		mesh._vertSig = MeshImporter::CreateVertSignature(aiMesh);
		mesh._vertexBuffer._primitiveTopology = AssimpWrapper::getPrimitiveTopology(aiMesh);
		ImportVertexData(mesh._vertSig, mesh._vertices, aiMesh, skeleton);
		AssimpWrapper::loadIndices(aiMesh, mesh._indices);

		return mesh;
	}
};