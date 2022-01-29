#pragma once
#include "Level.h"
#include "Model.h"
#include "Light.h"
#include "Scene.h"
#include "NavGrid.h"
#include "AStar.h"
#include "Skybox.h"
#include "GUI.h"
#include "CSM.h"
#include "RenderStage.h"
#include "FPSCounter.h"

#include "CParentLink.h"
#include "CTransform.h"
#include "CModel.h"
#include "CSkModel.h"

#include "LevelAsset.h"

#include "RuntimeAnimation.h"

#include "SkAnimRenderer.h"

#include "ComputeShader.h"


// Clean version of TDLevel without all the accumulated cruft.
class RenderingTestLevel : public Level
{
private:

	GeoClipmap _geoClipmap;
	Scene _scene;
	SceneEditor _sceneEditor;
	FPSCounter _fpsCounter;

	// Make it cozy in here for now, but move these to world/scene/renderer ultimately!
	DirectionalLight _dirLight;
	CBuffer _dirLightCB;
	CBuffer _positionBuffer;
	CBuffer _skMatsBuffer;
	Skybox _skybox;

	SkAnimRender _skAnimRenderer;

	std::vector<RenderStage> _stages;

	CBuffer _frustumBuffer;
	SBuffer _spheresBuffer;
	ID3D11ShaderResourceView* _spheresSRV;

	SBuffer _resultBuffer;
	ID3D11UnorderedAccessView* _resultUAV;

	ComputeShader _cullShader;

	VertexShader _fullScreenVS;
	PixelShader _fullScreenPS;

public:

	RenderingTestLevel(Engine& sys)
		: Level(sys), _scene(_sys, AABB(SVec3(), SVec3(500.f * .5)), 5), _geoClipmap(3, 4, 10.), _fpsCounter(64)
	{
	}


	void init(Engine& sys) override final
	{
		auto device = S_DEVICE;
		auto context = S_CONTEXT;

		// All of this should not have to be here! Goal of this refactor is to kill it.
		_sys._shaderCache.createAllShadersBecauseIAmTooLazyToMakeThisDataDriven(&sys._shaderCompiler);

		Material* skyBoxMat = new Material(_sys._shaderCache.getVertShader("FSTriangleVS"), _sys._shaderCache.getPixShader("skyboxTrianglePS"), true);

		_skybox = Skybox(device, "../Textures/day.dds", skyBoxMat);

		LightData lightData(SVec3(0.1, 0.7, 0.9), .03f, SVec3(0.8, 0.8, 1.0), .2, SVec3(0.3, 0.5, 1.0), 0.7);

		_dirLight = DirectionalLight(lightData, SVec4(0, -1, 0, 0));
		_dirLight.createCBuffer(device, _dirLightCB);
		_dirLight.updateCBuffer(context, _dirLightCB);

		_skMatsBuffer.init(device, CBuffer::createDesc(sizeof(SMatrix) * 200 /*numBones*/));

		std::vector<SMatrix> wat(200);
		_skMatsBuffer.update(context, wat.data(), sizeof(SMatrix) * 200);
		_skMatsBuffer.bindToVS(context, 1);

		_geoClipmap.init(device);

		_sceneEditor.init(&_scene, &S_INMAN);

		_scene._csm.init(device, 1024u, 1024u, S_SHCACHE.getVertShader("csmVS"));

		S_RANDY._cam._controller->setFlying(true);

		//auto _renderGroup = _scene._registry.group<CTransform, CSkModel>();
		//auto _physGroup = _scene._registry.group<CTransform, SphereHull>();

		auto modelPtr = _sys._skModelManager.getBlocking(9916003768089073041);

		auto vsPtr = sys._shaderCache.getVertShader("basicVS");
		auto psPtr = sys._shaderCache.getPixShader("phongPS");

		for (auto& mesh : modelPtr->_meshes)
		{
			mesh.setupMesh(device);
			mesh._material->setVS(vsPtr);
			mesh._material->setPS(psPtr);
		}

		// @TODO test this
		//BuildRuntimeAnimation(*modelPtr->_skeleton.get(), *modelPtr->_anims[0].get());

		for (UINT i = 0; i < 100; ++i)
		{
			auto entity = _scene._registry.create();
			_scene._registry.emplace<CSkModel>(entity, modelPtr.get());
			_scene._registry.emplace<CTransform>(entity, SMatrix::CreateTranslation(SVec3(i / 10, 0, (i % 10)) * 100.f));
			_scene._registry.emplace<CEntityName>(entity, "Entity name");
		}

		_positionBuffer.init(device, CBuffer::createDesc(sizeof(SMatrix)));
		_positionBuffer.bindToVS(context, 0);

		{
			// This works as expected. Thanks Skypjack! JSON is a poor choice for matrices though (or anything that should load fast)
			// so it's a good idea to use a binary format if nothing at least for part of the data. In general, only serialization test runs are to use json
			
			//std::ofstream ofs("AAAAAA.json", std::ios::binary);
			//cereal::JSONOutputArchive joa(ofs);
			//LevelAsset::serializeScene(joa, _scene._registry);
		}

		//_sys._renderer._mainStage = RenderStage(
		//	S_RANDY.device(),
		//	&(S_RANDY._cam),
		//	&(S_RANDY.d3d()->_renderTarget),
		//	&(S_RANDY.d3d()->_viewport));
			

		/*
		RenderStage shadowStage(
			S_RANDY.device(),
			&(S_RANDY._cam),
			&(S_RANDY.d3d()->_renderTarget),
			&(S_RANDY.d3d()->_viewport));

		RenderStage mainStage(
			S_RANDY.device(),
			&(S_RANDY._cam),
			&(S_RANDY.d3d()->_renderTarget),
			&(S_RANDY.d3d()->_viewport));

		_stages.push_back(std::move(shadowStage));
		_stages.push_back(std::move(mainStage));
		*/
		
		/* Do not discard this, it works as intended
		constexpr uint32_t test_sphere_count = 1024 * 1024;

		_frustumBuffer.init(device, CBuffer::createDesc(sizeof(SVec4) * 6));

		_spheresBuffer = SBuffer(device, sizeof(SVec4), test_sphere_count, D3D11_BIND_UNORDERED_ACCESS);
		SBuffer::CreateSBufferSRV(device, _spheresBuffer.getPtr(), test_sphere_count, _spheresSRV);

		_resultBuffer = SBuffer(device, sizeof(float), test_sphere_count, D3D11_BIND_UNORDERED_ACCESS);
		SBuffer::createSBufferUAV(device, _resultBuffer.getPtr(), test_sphere_count, _resultUAV);

		_cullShader.createFromFile(device, L"Shaders/FrustumCull.hlsl");
		*/

		_fullScreenVS = VertexShader(sys._shaderCompiler, L"Shaders/FullScreenTriNoBufferVS.hlsl", {});
		_fullScreenPS = PixelShader(sys._shaderCompiler, L"Shaders/FullScreenTriNoBufferPS.hlsl", {});
	}


	void fakeRenderSystem(ID3D11DeviceContext* context, entt::registry& registry)
	{
		// Can try a single buffer for position
		//_positionBuffer.bindToVS(context, 0);

		//auto& mainPass = _sys._renderer._mainStage;
		//mainPass.prepare(context, _sys._clock.deltaTime(), _sys._clock.totalTime());

		auto group = _scene._registry.group<CTransform, CSkModel>();

		group.each([&context, &posBuffer = _positionBuffer, &skBuffer = _skMatsBuffer](CTransform& transform, CSkModel& renderComp)
			{
				auto skModel = renderComp.skModel;

				if (!skModel)
					return;

				posBuffer.bindToVS(context, 0);
				skBuffer.bindToVS(context, 1);

				for (auto& mesh : skModel->_meshes)
				{
					mesh.bind(context);
					posBuffer.updateWithStruct(context, transform.transform.Transpose());

					Material* mat = mesh._material.get();

					mat->getVS()->bind(context);
					mat->getPS()->bind(context);

					// Set shaders and textures.
					mat->bindTextures(context);

					context->DrawIndexed(mesh._indexBuffer.getIdxCount(), 0, 0);
				}
			});
	}


	void update(const RenderContext& rc) override final
	{
		_scene.update();
		_fpsCounter.tickFast(rc.dTime);


		static uint64_t frameCount{ 0u };
		if (frameCount++ % 512 == 0)
		{
			AnimationInstance instance = AnimationInstance(_scene._registry.get<CSkModel>(entt::entity{ 0 }).skModel->_anims[0]);
			//_skAnimRenderer.addInstance();
		}

		_skAnimRenderer.update(_scene._registry, rc.dTime);
	}


	void draw(const RenderContext& rc) override final
	{
		const auto context = rc.d3d->getContext();

		//_sys._renderer.setDefaultRenderTarget();

		_dirLight.updateCBuffer(context, _dirLightCB);
		_dirLight.bind(context, _dirLightCB);

		_scene.draw();

		//_sys._renderer.setDefaultRenderTarget();

		fakeRenderSystem(rc.d3d->getContext(), _scene._registry);

		S_RANDY.d3d()->setRSWireframe();
		_geoClipmap.draw(context);
		S_RANDY.d3d()->setRSSolidCull();

		_skybox.renderSkybox(*rc.cam, S_RANDY);

		// testing full screen shader - works, confirmed
		//S_RANDY.d3d()->setRSSolidNoCull();
		//context->VSSetShader(_fullScreenVS._vsPtr.Get(), nullptr, 0);
		//context->PSSetShader(_fullScreenPS._psPtr.Get(), nullptr, 0);
		//context->Draw(3, 0);

		GUI::BeginFrame();

		_sceneEditor.display();

		std::vector<GuiElement> guiElems =
		{
			{"Octree",	std::string("OCT node count " + std::to_string(_scene._octree.getNodeCount()))},
			{"Octree",	std::string("OCT hull count " + std::to_string(_scene._octree.getHullCount()))},
			{"FPS",		std::string("FPS: " + std::to_string(_fpsCounter.getAverageFPS()))},
			{"Culling", std::string("Objects culled:" + std::to_string(_scene._numCulled))}
		};
		GUI::RenderGuiElems(guiElems);

		GUI::EndFrame();

		rc.d3d->present();

		/* Do not discard this, it works as intended
		// Instead of a real cull we will just get anything right of the origin to pass to see if the shader works
		std::array<SPlane, 6> planes;
		for (auto& p : planes)
		{
			p.x = 1;
			p.y = 0;
			p.z = 0;
			p.w = 0;
		}

		_frustumBuffer.update(S_RANDY.context(), &planes, sizeof(SPlane) * 6);
		_frustumBuffer.bindToCS(S_RANDY.context(), 0);

		std::vector<ID3D11ShaderResourceView*> wat1{_spheresSRV};
		std::vector<ID3D11UnorderedAccessView*> wat2{_resultUAV};
		_cullShader.execute(S_RANDY.context(), {16, 16, 1}, wat1, wat2);
		*/
	}

};