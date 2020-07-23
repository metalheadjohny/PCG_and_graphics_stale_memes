#pragma once
#include "AssimpWrapper.h"
#include <stack>
#include <memory>


class SkeletonLoader
{
private:
	std::set<aiBone*> _bones;
	std::set<aiNode*> _boneNodes;



	void findInfluenceBones(const aiScene* scene, std::set<aiBone*>& boneSet)
	{
		for (UINT i = 0; i < scene->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[i];

			for (UINT j = 0; j < mesh->mNumBones; ++j)	// For each bone referenced by the mesh
				if (mesh->mBones[j])
					boneSet.insert(mesh->mBones[j]);
		}
	}



	void findAllBoneNodes(const std::set<aiBone*>& aiBones, std::set<aiNode*>& nodes)
	{
		std::vector<aiNode*> temp;
		temp.reserve(aiBones.size());

		for (aiBone* aiBone : aiBones)
		{
			temp.push_back(aiBone->mNode);
			nodes.insert(aiBone->mNode);
		}

		for (aiNode* node : temp)
		{
			aiNode* parent = node->mParent;

			while (parent != nullptr)
			{
				if (parent->mParent != nullptr)
				{
					if (!nodes.insert(parent).second)	// Exit if already added
						break;
				}
				parent = parent->mParent;
			}
		}
	}



	aiNode* findSkeletonRoot(aiNode* node, const std::set<aiNode*>& boneNodes)
	{
		aiNode* result = nullptr;

		auto nodeFound = boneNodes.find(node);

		if (nodeFound != boneNodes.end())
		{
			result = *nodeFound;
		}
		else
		{
			for (UINT i = 0; i < node->mNumChildren; ++i)
			{
				result = findSkeletonRoot(node->mChildren[i], boneNodes);
				if (result)
					break;
			}
		}

		return result;
	}



	void makeLikeATree(aiNode* node, std::vector<Bone>& boneVec, Bone* parent)
	{
		if (_boneNodes.count(node) == 0)
			return;

		Bone bone;
		bone._index = boneVec.size();
		bone._localMatrix = AssimpWrapper::aiMatToSMat(node->mTransformation);
		bone._name = node->mName.C_Str();

		// A bit slow but it's not too important given the usual data size and this being offline
		for (aiBone* aiBone : _bones)
		{
			if (aiBone->mNode == node)
			{
				bone._offsetMatrix = AssimpWrapper::aiMatToSMat(aiBone->mOffsetMatrix);
			}
		}

		bone._parent = parent;

		boneVec.push_back(bone);

		if (bone._parent)
			bone._parent->_children.push_back(&boneVec.back());

		for (UINT i = 0; i < node->mNumChildren; ++i)
			makeLikeATree(node->mChildren[i], boneVec, &boneVec[bone._index]);
	}



	void loadNodesAsBones(aiNode* node, std::vector<Bone>& bones, Bone* parent)
	{
		Bone b;
		b._name = node->mName.C_Str();
		b._localMatrix = AssimpWrapper::aiMatToSMat(node->mTransformation);
		b._index = bones.size();
		b._parent = parent;
		// Offset matrices are not present, they depend on the model we attach this to
		bones.push_back(b);

		Bone* thisBone = &bones.back();
		
		if (parent)
			parent->_children.push_back(thisBone);

		parent = thisBone;

		for (UINT i = 0; i < node->mNumChildren; ++i)
		{
			loadNodesAsBones(node->mChildren[i], bones, parent);
		}
	}


public:

	std::unique_ptr<Skeleton> loadSkeleton(const aiScene* scene)
	{
		aiNode* sceneRoot = scene->mRootNode;

		findInfluenceBones(scene, _bones);

		findAllBoneNodes(_bones, _boneNodes);

		// Find the closest aiNode to the root
		aiNode* skelRoot = findSkeletonRoot(sceneRoot, _boneNodes);

		if (!skelRoot)
			return nullptr;

		std::unique_ptr<Skeleton> skeleton = std::make_unique<Skeleton>();
		skeleton->_bones.reserve(_bones.size());

		makeLikeATree(skelRoot, skeleton->_bones, nullptr);

		return skeleton;
	}



	std::unique_ptr<Skeleton> loadStandalone(const aiScene* scene)
	{
		std::unique_ptr<Skeleton> skelly = std::make_unique<Skeleton>();

		UINT boneCount = AssimpWrapper::countChildren(scene->mRootNode) + 1;
		
		skelly->_bones.reserve(boneCount);

		loadNodesAsBones(scene->mRootNode, skelly->_bones, nullptr);

		return skelly;
	}
};


/*
// Is this smart or dirty? We will never know... Got another solution though
	void traverseUp(std::set<aiNode*>& prev)
	{
		if (prev.size() == 0)	// 1. At some point, this returns
			return;

		std::set<aiNode*> current;

		for (aiNode* node : prev)
		{
			aiNode* parent = node->mParent;
			if(parent != nullptr)
				if(parent->mParent != nullptr)
					current.insert(node->mParent);
		}

		traverseUp(current);	// 2. Then we exit out of here
		prev.insert(current.begin(), current.end()); // 3. And merge into initial set
	}
*/