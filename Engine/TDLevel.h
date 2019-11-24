#pragma once
#include "Level.h"
#include "Math.h"
#include "Model.h"
#include "Light.h"
#include "SceneGraph.h"
#include "Octree.h"
#include "AssetLoader.h"
#include "ProjectLoader.h"

class TDLevel : public Level
{
public:
	TDLevel(Systems& sys) : Level(sys) {};

	void init(Systems& sys) override;
	void update(const RenderContext& rc) override;
	void draw(const RenderContext& rc) override;
	void demolish() { this->~TDLevel(); };

private:

	SceneGraph _sg;
	Octree _oct;

	Model floorModel;
	PointLight pLight;
	
	Model skybox;
	CubeMapper skyboxCubeMapper;

	std::vector<Actor> dubois;
	Model boiModel;
	Model debugModel;

	AssetLoader assLoader;
	ProjectLoader pjLoader;
};