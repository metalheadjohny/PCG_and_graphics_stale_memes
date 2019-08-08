#include "StrifeLevel.h"
#include "Math.h"
#include "ShaderUtils.h"

namespace Strife
{

	void StrifeLevel::init(Systems& sys)
	{
		skyboxCubeMapper.LoadFromFiles(device, "../Textures/night.dds");

		sceneTex.Init(device, _sys.getWinW() / 2, _sys.getWinH() / 2);
		screenRectangleNode = postProcessor.AddUINODE(device, postProcessor.getRoot(), SVec2(0, 0), SVec2(1, 1), .999999f);

		skybox.LoadModel(device, "../Models/Skysphere.fbx");

		Mesh scrQuadMesh = Mesh(SVec2(0., 0.), SVec2(1.f, 1.f), device, .999999f);	//1.777777f
		screenQuad.meshes.push_back(scrQuadMesh);

		_sys._renderer._cam.SetProjectionMatrix(DirectX::XMMatrixPerspectiveFovLH(0.5 * PI, randy._screenAspect, 1.f, 1000.f));

		//32000.f in lux but that doesn't work... not sure how to do any of this
		LightData lightData(SVec3(1.f, 1.f, 1.f), 1.f, SVec3(0.8f, 0.8f, 1.0f), .2f, SVec3(0.3f, 0.5f, 1.0f), 0.7f);

		float edge = 256;
		Procedural::Terrain terrain(2, 2, SVec3(edge, 1, edge));
		terrain.setOffset(-edge * .5, 0.f, -edge * .5);
		terrain.CalculateNormals();

		floor = Model(terrain, device);
		floor.transform = SMatrix::CreateTranslation(terrain.getOffset());


		//set up initial cloud definition - most of them can be changed through the gui later
		csDef.celestial = PointLight(lightData, SVec4(0., 999., 999., 1.0f));
		csDef.celestial.alc = SVec3(1., 1., 1.);
		csDef.rgb_sig_absorption = SVec3(.05f);	//SVec3(1.); quite a few can work tbh
		csDef.eccentricity = 0.66f;
		csDef.globalCoverage = .5f;
		csDef.scrQuadOffset = 1.f;
		csDef.heightMask = SVec2(600, 1000);
		csDef.repeat = SVec4(4096.f, 32.f, 4069.f, 1.f);	//density factor in .w
		
		//load 2D textures
		csDef.weather = Texture(device, "../Textures/DensityTypeTexture.png");
		csDef.blue_noise = Texture(device, "../Textures/blue_noise_64_tiled.png");

		//create/load 3D tectures
		//Create3D();
		Create3DOneChannel();
		csDef.baseVolume = baseSrv;
		
		CreateFine3D();
		csDef.fineVolume = fineSrv;
	}



	void StrifeLevel::procGen(){}



	void StrifeLevel::update(const RenderContext& rc)
	{
		if (_sys._inputManager.IsKeyDown((short)'M') && sinceLastInput > .33f)
		{
			inman.ToggleMouseMode();
			sinceLastInput = 0;
		}

		if(!inman.GetMouseMode())
			updateCam(rc.dTime);

		sinceLastInput += rc.dTime;
	}



	void StrifeLevel::draw(const RenderContext& rc)
	{
		rc.d3d->ClearColourDepthBuffers();

		//terrain
		shady.light.SetShaderParameters(context, floor.transform, *rc.cam, csDef.celestial, rc.dTime);
		floor.Draw(context, shady.light);
		
		//skybox
		//randy.RenderSkybox(*rc.cam, skybox, skyboxCubeMapper);

		//cloudscape, blend into background which depends on the time of the day... or use anything idk...

		sceneTex.SetRenderTarget(context);

		rc.d3d->TurnOnAlphaBlending();

		shady.strife.SetShaderParameters(context, *rc.cam, csDef, rc.elapsed);
		screenQuad.Draw(context, shady.strife);
		shady.strife.ReleaseShaderParameters(context);

		rc.d3d->TurnOffAlphaBlending();


		context->RSSetViewports(1, &_sys._D3D.viewport);
		context->OMSetRenderTargets(1, &_sys._D3D.m_renderTargetView, _sys._D3D.GetDepthStencilView());

		postProcessor.draw(context, shady.HUD, sceneTex.srv);

		//GUI
		if(inman.GetMouseMode())
			ToolGUI::Render(csDef);

		rc.d3d->EndScene();
	}











	bool StrifeLevel::Create3D()
	{
		D3D11_TEXTURE3D_DESC desc;
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		int size = 128;

		std::vector<float> yeetFloat;
		size_t sheetSize = size * size * 4;
		size_t yeetSize = size * sheetSize;
		yeetFloat.reserve(yeetSize);

		std::vector<float> flVec;
		flVec.reserve(sheetSize);

		//not really optimal but it's still quite fast
		for (int i = 0; i < size; ++i)
		{
			std::stringstream ss;
			ss << std::setw(3) << std::setfill('0') << (i + 1);
			flVec = Texture::GetFloatsFromFile("../Textures/Generated/my3DTextureArray." + ss.str() + ".tga");
			yeetFloat.insert(yeetFloat.end(), flVec.begin(), flVec.end());
		}

		desc.Width = size;
		desc.Height = size;
		desc.Depth = size;
		desc.MipLevels = 8;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		//D3D11_SUBRESOURCE_DATA texData;
		std::vector<D3D11_SUBRESOURCE_DATA> texData(desc.MipLevels);

		float texelByteWidth = 4 * sizeof(float);	//RGBA format with each being a float32

		for (int i = 0; i < desc.MipLevels; ++i)
		{
			texData[i].pSysMem = (void *)yeetFloat.data();
			texData[i].SysMemPitch = desc.Width * texelByteWidth;
			texData[i].SysMemSlicePitch = texData[i].SysMemPitch * desc.Height;
		}

		HRESULT hr = device->CreateTexture3D(&desc, &texData[0], &baseTexId);
		if (FAILED(hr))
		{
			OutputDebugStringA("Can't create texture3d. \n");
			exit(42);
		}

		shaderResourceViewDesc.Format = desc.Format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		shaderResourceViewDesc.Texture3D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture3D.MipLevels = desc.MipLevels;

		if (FAILED(device->CreateShaderResourceView(baseTexId, &shaderResourceViewDesc, &baseSrv)))
		{
			OutputDebugStringA("Can't create shader resource view. \n");
			exit(43);
		}

		context->GenerateMips(baseSrv);

		return true;
	}



	bool StrifeLevel::CreateFine3D()
	{
		D3D11_TEXTURE3D_DESC desc;
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

		int size = 32;
		int numFloats = 4;
		int texelByteWidth = numFloats * sizeof(float);

		std::vector<float> floatVector;
		size_t sheetSize = size * size * numFloats;
		size_t volumeFloatSize = size * sheetSize;
		floatVector.reserve(volumeFloatSize);

		float z = 1.f / 32.f;

		//fill out with good ole Perlin fbm
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				for (int k = 0; k < size; ++k)
				{
					//floatVector.emplace_back(fabs(Texture::Perlin3DFBM(i * z, j * z, k * z, 2.f, .5f, 3u)));
					floatVector.emplace_back(Sebh::Cells(z * SVec3(i, j, k),  1));
					floatVector.emplace_back(Sebh::Cells(z * SVec3(i, j, k), 2));
					floatVector.emplace_back(Sebh::Cells(z * SVec3(i, j, k), 3));
					floatVector.emplace_back(0.f);	//dx crying over no 24 bit format so we have this...
				}
			}
		}

		desc.Width = size;
		desc.Height = size;
		desc.Depth = size;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA texData;

		texData.pSysMem = (void *)floatVector.data();
		texData.SysMemPitch = desc.Width * texelByteWidth;
		texData.SysMemSlicePitch = texData.SysMemPitch * desc.Height;

		HRESULT hr = device->CreateTexture3D(&desc, &texData, &fineTexId);
		if (FAILED(hr))
		{
			OutputDebugStringA("Can't create texture3d. \n");
			exit(42);
		}

		shaderResourceViewDesc.Format = desc.Format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		shaderResourceViewDesc.Texture3D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture3D.MipLevels = desc.MipLevels;

		if (FAILED(device->CreateShaderResourceView(fineTexId, &shaderResourceViewDesc, &fineSrv)))
		{
			OutputDebugStringA("Can't create shader resource view. \n");
			exit(43);
		}

		return true;
	}



	bool StrifeLevel::Create3DOneChannel()
	{
		D3D11_TEXTURE3D_DESC desc;
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		int size = 128;
		int numChannels = 1;

		std::vector<float> finalArray;
		size_t sheetSizeProcessed = size * size * numChannels;
		size_t finalSize = size * sheetSizeProcessed;
		finalArray.reserve(finalSize);

		std::vector<float> flVec;
		size_t sheetSizeInitial = size * size * 4;
		flVec.reserve(sheetSizeInitial);

		std::vector<float> processedVec;
		processedVec.reserve(sheetSizeProcessed);

		//not really optimal but it's still quite fast
		for (int i = 0; i < size; ++i)
		{
			std::stringstream ss;
			ss << std::setw(3) << std::setfill('0') << (i + 1);
			flVec = Texture::GetFloatsFromFile("../Textures/Generated/my3DTextureArray." + ss.str() + ".tga");

			for (int j = 0; j < flVec.size(); j += 4)
			{
				//processedVec.emplace_back(m.x * flVec[j] + m.y * flVec[j + 1] + m.z * flVec[j + 2] + m.w * flVec[j + 3]);
				processedVec.emplace_back(/*Math::clamp(0., 1., */Math::remap(flVec[j + 1] * flVec[j], flVec[j+2] * .5f, 1., 0., 1.)/*)*/);
			}

			finalArray.insert(finalArray.end(), processedVec.begin(), processedVec.end());
			processedVec.clear();
		}

		desc.Width = size;
		desc.Height = size;
		desc.Depth = size;
		desc.MipLevels = 8;
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		//D3D11_SUBRESOURCE_DATA texData;
		std::vector<D3D11_SUBRESOURCE_DATA> texData(desc.MipLevels);

		float texelByteWidth = numChannels * sizeof(float);	//RGBA format with each being a float32

		for (int i = 0; i < desc.MipLevels; ++i)
		{
			texData[i].pSysMem = (void *)finalArray.data();
			texData[i].SysMemPitch = desc.Width * texelByteWidth;
			texData[i].SysMemSlicePitch = texData[i].SysMemPitch * desc.Height;
		}

		HRESULT hr = device->CreateTexture3D(&desc, &texData[0], &baseTexId);
		if (FAILED(hr))
		{
			OutputDebugStringA("Can't create texture3d. \n");
			exit(42);
		}

		shaderResourceViewDesc.Format = desc.Format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		shaderResourceViewDesc.Texture3D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture3D.MipLevels = desc.MipLevels;

		if (FAILED(device->CreateShaderResourceView(baseTexId, &shaderResourceViewDesc, &baseSrv)))
		{
			OutputDebugStringA("Can't create shader resource view. \n");
			exit(43);
		}

		context->GenerateMips(baseSrv);

		return true;
	}

}