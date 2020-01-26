#pragma once
#include "Texture.h"
#include "ShaderManager.h"
#include "Shader.h"

//contains everything that the attached shaders need to be set to run... I guess?
class Material
{
protected:

	//most important sorting criteria
	VertexShader* _vertexShader;
	PixelShader* _pixelShader;

public:

	//second most important sorting criteria
	std::vector<std::pair<TextureRole, Texture*>> textures;
	
	//uint8_t roles[8];		//each element is a role, with shader offset number as value

	//determines whether it goes to the transparent or opaque queue... different sorting
	bool opaque;

	//determines how many textures are added by the shader itself
	//won't ever have 256 textures so that's even too big but can't help it for now without bitsets, meh...
	unsigned char texturesAdded = 0u;	

	//don't really sort by these, should be mostly uniform for my uses...
	unsigned int stride = sizeof(Vert3D);
	unsigned int offset = 0u;
	D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	Material();
	~Material();
	
	void setVS(VertexShader* vs);
	void setPS(PixelShader* ps);

	inline VertexShader* getVS() { return _vertexShader; }
	inline PixelShader* getPS() { return _pixelShader; }
};