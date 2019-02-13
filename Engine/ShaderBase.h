#pragma once
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <fstream>
#include <vector>
#include <string>
#include "Math.h"
#include "lightclass.h"
#include "ShaderDataStructs.h"

class Model;

class ShaderBase
{
public:
	ShaderBase();
	~ShaderBase();

	virtual bool Initialize(ID3D11Device*, HWND, const std::vector<std::wstring> filePaths,
		D3D11_INPUT_ELEMENT_DESC* layout, unsigned int layoutSize, const D3D11_SAMPLER_DESC& samplerDesc);
	virtual bool SetShaderParameters(ID3D11DeviceContext*, Model& m, const SMatrix& v, const SMatrix& p,
		const PointLight& dLight, const SVec3& eyePos, float deltaTime);
	virtual bool ReleaseShaderParameters(ID3D11DeviceContext*);
	virtual void ShutdownShader();
	


	ID3D11InputLayout* _layout;
	ID3D11SamplerState* _sampleState;

	ID3D11VertexShader* _vertexShader;
	ID3D11PixelShader* _pixelShader;

	ID3D11Buffer* _matrixBuffer;
	ID3D11Buffer* _variableBuffer;
	ID3D11Buffer* _lightBuffer;

protected:

	std::vector<std::wstring> filePaths;
	ID3D11ShaderResourceView* unbinder[1] = { nullptr };
	virtual void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR);
};

