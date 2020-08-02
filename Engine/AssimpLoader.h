#pragma once
#include "Level.h"
#include "AssImport.h"
#include "Engine.h"
#include "Scene.h"
#include "GUI.h"
#include "SkeletalModelInstance.h"
#include "Sampler.h"
#include <memory>



class AssimpLoader : public Level
{
private:

	Scene _scene;

	FileBrowser _browser;

	std::vector<std::unique_ptr<AssImport>> _previews;
	AssImport* _curPreview;

	// For previewing models in 3d
	Material _skelAnimMat;
	PointLight _pointLight;

	// Floor 
	std::unique_ptr<Mesh> _floorMesh;
	Renderable _floorRenderable;
	Actor _terrainActor;

public:

	AssimpLoader(Engine& sys) : Level(sys), _scene(sys, AABB(SVec3(), SVec3(500.f * .5)), 5)
	{
		_browser = FileBrowser("C:\\Users\\Senpai\\source\\repos\\PCG_and_graphics_stale_memes\\Models\\Animated");
		_curPreview = nullptr;
	}



	void init(Engine& sys) override
	{
		LightData ld = LightData(SVec3(1.), .2, SVec3(1.), .6, SVec3(1.), .7);

		_pointLight = PointLight(ld, SVec4(0., 300., 0., 1.));

		_pointLight.createCBuffer(S_DEVICE);
		ID3D11DeviceContext* context;
		S_DEVICE->GetImmediateContext(&context);
		_pointLight.updateCBuffer(context);
		_pointLight.bind(context);

		ShaderCompiler shc;

		shc.init(S_DEVICE);	// Temporary

		std::vector<D3D11_INPUT_ELEMENT_DESC> ptn_layout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		std::vector<D3D11_INPUT_ELEMENT_DESC> ptn_biw_layout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BONE_ID" , 0, DXGI_FORMAT_R32G32B32A32_UINT,		0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "BONE_W"  , 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		D3D11_SAMPLER_DESC regularSD = Sampler::createSamplerDesc();

		D3D11_BUFFER_DESC WMBufferDesc = CBuffer::createDesc(sizeof(WMBuffer));
		CBufferMeta WMBufferMeta(0, WMBufferDesc.ByteWidth);
		WMBufferMeta.addFieldDescription(CBUFFER_FIELD_CONTENT::TRANSFORM, 0, sizeof(WMBuffer));

		//D3D11_BUFFER_DESC BoneBufferDesc = ShaderCompiler::createBufferDesc(sizeof(SMatrix) * 96);
		//CBufferMeta BoneBufferMeta(1, BoneBufferDesc.ByteWidth);

		VertexShader* saVS = new VertexShader(shc, L"AnimaVS.hlsl", ptn_biw_layout, { WMBufferDesc });
		saVS->describeBuffers({ WMBufferMeta });

		PixelShader* phong = new PixelShader(shc, L"lightPS.hlsl", { regularSD }, { });
		//phong->describeBuffers({ lightBufferMeta });

		_skelAnimMat.setVS(saVS);
		_skelAnimMat.setPS(phong);


		VertexShader* basicVS = new VertexShader(shc, L"lightVS.hlsl", ptn_layout, { WMBufferDesc });
		basicVS->describeBuffers({ WMBufferMeta });

		// Generate the floor, assign a material and render
		Procedural::Terrain terrain;
		float _tSize = 500.f;
		terrain = Procedural::Terrain(2, 2, SVec3(_tSize));
		terrain.setOffset(-_tSize * .5f, -0.f, -_tSize * .5f);
		terrain.CalculateTexCoords();
		terrain.CalculateNormals();
		_floorMesh = std::make_unique<Mesh>(terrain, S_DEVICE);
		_floorRenderable = Renderable(*_floorMesh);
		_floorRenderable.mat->setVS(basicVS);
		_floorRenderable.mat->setPS(phong);
		_terrainActor.addRenderable(_floorRenderable, 500);
		_terrainActor._collider.collidable = false;
	}



	void update(const RenderContext& rc) override {}



	void draw(const RenderContext& rc) override
	{
		rc.d3d->ClearColourDepthBuffers();

		S_RANDY.render(_floorRenderable);

		if(_curPreview)
			_curPreview->draw(S_CONTEXT, rc.dTime);

		GUI::beginFrame();

		auto selected = _browser.display();

		if (selected.has_value())
		{
			if (!alreadyLoaded(selected.value()))
			{
				_previews.push_back(std::make_unique<AssImport>());

				if (!_previews.back()->loadAiScene(rc.d3d->GetDevice(), selected.value().path().string(), 0u, &S_RESMAN))
				{
					_previews.pop_back();
				}
			}
		}


		ImGui::Begin("Content");
		ImGui::BeginTabBar("Loaded scenes", ImGuiTabBarFlags_AutoSelectNewTabs);
		ImGui::NewLine();

		for (UINT i = 0; i < _previews.size(); i++)
		{
			std::string sceneName = _previews[i]->getPath().filename().string();
			if (ImGui::BeginTabItem(sceneName.c_str()))
			{
				_curPreview = _previews[i].get();

				if(!_previews[i]->displayPreview(sceneName))
				{
					_previews.erase(_previews.begin() + i);
					_curPreview = nullptr;
				}

				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();

		ImGui::End();

		GUI::endFrame();

		rc.d3d->EndScene();
	}



	bool alreadyLoaded(const std::filesystem::directory_entry& selected)
	{
		for (auto& p : _previews)
			if (p->getPath() == selected.path())
				return true;

		return false;
	}
};