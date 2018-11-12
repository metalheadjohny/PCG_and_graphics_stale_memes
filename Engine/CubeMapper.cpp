#include "CubeMapper.h"



CubeMapper::CubeMapper(unsigned int edgeLength) : edgeLength(edgeLength)
{
}



CubeMapper::~CubeMapper()
{
}



void CubeMapper::Init(ID3D11Device* device) {

	HRESULT res;

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = edgeLength;
	texDesc.Height = edgeLength;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//DXGI_FORMAT_R32G32B32A32_FLOAT
	texDesc.CPUAccessFlags = 0;
	texDesc.SampleDesc = { 1, 0 };
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	res = device->CreateTexture2D(&texDesc, 0, &cm_id);
	if (FAILED(res)) {
		OutputDebugStringA("Can't create cube map texture. \n");
		exit(520);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	srvd.Format = texDesc.Format;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvd.TextureCube.MipLevels = texDesc.MipLevels;
	srvd.TextureCube.MostDetailedMip = 0;

	res = device->CreateShaderResourceView(cm_id, &srvd, &cm_srv);
	if (FAILED(res)) {
		OutputDebugStringA("Can't create shader resource view. \n");
		exit(521);
	}

	D3D11_RENDER_TARGET_VIEW_DESC rtvd;
	rtvd.Format = texDesc.Format;
	rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvd.Texture2D.MipSlice = 0;
	rtvd.Texture2DArray.ArraySize = 1;

	for (int i = 0; i < 6; i++) {

		rtvd.Texture2DArray.FirstArraySlice = i;

		res = device->CreateRenderTargetView(cm_id, &rtvd, &(cm_rtv[i]));
		if (FAILED(res)) {
			OutputDebugStringA("Can't create 6 render target views. \n");
			exit(522);
		}
	}


	D3D11_TEXTURE2D_DESC depthTexDesc;
	ZeroMemory(&depthTexDesc, sizeof(depthTexDesc));
	depthTexDesc.Width = edgeLength;
	depthTexDesc.Height = edgeLength;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.ArraySize = 1;
	depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthTexDesc.CPUAccessFlags = 0;
	depthTexDesc.SampleDesc = { 1, 0 };
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTexDesc.MiscFlags = 0;

	res = device->CreateTexture2D(&depthTexDesc, 0, &cm_depth_id);
	if (FAILED(res)) {
		OutputDebugStringA("Can't create cube map depth texture. \n");
		exit(523);
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	dsvd.Format = depthTexDesc.Format;
	dsvd.Flags = 0;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Texture2D.MipSlice = 0;

	res = device->CreateDepthStencilView(cm_depth_id, &dsvd, &cm_depthStencilView);
	if (FAILED(res)) {
		OutputDebugStringA("Can't create cube map texture. \n");
		exit(524);
	}

	cm_viewport.Width = (float)edgeLength;
	cm_viewport.Height = (float)edgeLength;
	cm_viewport.MinDepth = 0.0f;
	cm_viewport.MaxDepth = 1.0f;
	cm_viewport.TopLeftX = 0;
	cm_viewport.TopLeftY = 0;

	lens = DirectX::XMMatrixPerspectiveFovLH(PI * 0.5f, 1.0f, 0.1f, 200.0f);	//2.0 * atan(edgeLength / (edgeLength - 0.5))	
}

void CubeMapper::UpdateCams(const SVec3& pos) {

	cameras[0] = DirectX::XMMatrixLookAtLH(pos, SVec3(pos.x + 1.f, pos.y, pos.z), SVec3::Up);
	cameras[1] = DirectX::XMMatrixLookAtLH(pos, SVec3(pos.x - 1.f, pos.y, pos.z), SVec3::Up);
	cameras[2] = DirectX::XMMatrixLookAtLH(pos, SVec3(pos.x, pos.y + 1.f, pos.z), SVec3::Forward);	//flipped because simplemath
	cameras[3] = DirectX::XMMatrixLookAtLH(pos, SVec3(pos.x, pos.y - 1.f, pos.z), SVec3::Backward);	//is right handed by default...
	cameras[4] = DirectX::XMMatrixLookAtLH(pos, SVec3(pos.x, pos.y, pos.z + 1.f), SVec3::Up);	//this could be wrong possible @TODO
	cameras[5] = DirectX::XMMatrixLookAtLH(pos, SVec3(pos.x, pos.y, pos.z - 1.f), SVec3::Up);
}