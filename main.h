//d3d11 w2s base for some games by n7

#define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL; }

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;


//globals
bool showmenu = false;
bool initonce = false;

//item states
bool wallhack = 1;
bool chams = 0;
bool esp = 1;
bool aimbot = 1;
int aimfov = 4;
int aimkey = 0;
int aimsens = 2;
int aimspeed_isbasedon_distance = 1;		
float AimSpeed;                             
int aimspeed = 0;                           //5 slow dont move mouse faster than 5 pixel, 100 fast
int aimheight = 0;							//aim height value
bool autoshoot = 0;							//autoshoot
int as_xhairdst = 7.0;						//autoshoot activates below this crosshair distance

DWORD Daimkey = VK_RBUTTON;		//aimkey
bool IsPressed = false;
bool targetfound = false;
//int preaim = 0;				//preaim to not aim behind

bool modelrecfinder = 1;
int countStride = -1;
int countIndexCount = -1;
int countveWidth = -1;
int countpscWidth = -1;
bool wtsfinder = 1;

int check_draw_result=0;
int check_drawindexed_result = 0;
int check_drawindexedinstanced_result = 0;

//rendertarget
ID3D11RenderTargetView* mainRenderTargetViewD3D11;

//wh
ID3D11DepthStencilState* DepthStencilState_TRUE = NULL; //depth off
ID3D11DepthStencilState* DepthStencilState_FALSE = NULL; //depth off
ID3D11DepthStencilState* DepthStencilState_ORIG = NULL; //depth on

//shader
ID3D11PixelShader* sGreen = NULL;
ID3D11PixelShader* sMagenta = NULL;

//pssetshaderresources
//D3D11_SHADER_RESOURCE_VIEW_DESC  Descr;
//D3D11_TEXTURE2D_DESC texdesc;

//Viewport
float ViewportWidth;
float ViewportHeight;
float ScreenCenterX;
float ScreenCenterY;

int countnum = -1;
int g_dwLastAction = 0;
int vscStartSlot = 0;
int validvscStartSlot = 0;

bool method1 = 1;
bool method2 = 0;
bool method3 = 0;
bool method4 = 0;

//==========================================================================================================================

//get dir
using namespace std;
#include <fstream>

// getdir & log
char dlldir[320];
char* GetDirFile(char* name)
{
	static char pldir[320];
	strcpy_s(pldir, dlldir);
	strcat_s(pldir, name);
	return pldir;
}

//log
void Log(const char* fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile((PCHAR)"log.txt", ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}

#include <D3Dcompiler.h> //generateshader
#pragma comment(lib, "D3dcompiler.lib")
//generate shader func
HRESULT GenerateShader(ID3D11Device * pDevice, ID3D11PixelShader * *pShader, float r, float g, float b)
{
	char szCast[] = "struct VS_OUT"
		"{"
		" float4 Position : SV_Position;"
		" float4 Color : COLOR0;"
		"};"

		"float4 main( VS_OUT input ) : SV_Target"
		"{"
		" float4 col;"
		" col.a = 1.0f;"
		" col.r = %f;"
		" col.g = %f;"
		" col.b = %f;"
		" return col;"
		"}";

	ID3D10Blob* pBlob;
	char szPixelShader[1000];

	sprintf_s(szPixelShader, szCast, r, g, b);

	ID3DBlob* error;

	HRESULT hr = D3DCompile(szPixelShader, sizeof(szPixelShader), "shader", NULL, NULL, "main", "ps_4_0", NULL, NULL, &pBlob, &error);

	if (FAILED(hr))
		return hr;

	hr = pDevice->CreatePixelShader((DWORD*)pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, pShader);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

ID3D11Texture2D* texc = nullptr;
ID3D11ShaderResourceView* textureColor;
void GenerateTexture(uint32_t pixelcolor, DXGI_FORMAT format)//DXGI_FORMAT_R32G32B32A32_FLOAT DXGI_FORMAT_R8G8B8A8_UNORM
{
	//static const uint32_t pixelcolor = 0xff00ff00; //0xff00ff00 green, 0xffff0000 blue, 0xff0000ff red
	D3D11_SUBRESOURCE_DATA initData = { &pixelcolor, sizeof(uint32_t), 0 };

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = 1;
	desc.Height = 1;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	pDevice->CreateTexture2D(&desc, &initData, &texc);

	D3D11_SHADER_RESOURCE_VIEW_DESC srdes;
	memset(&srdes, 0, sizeof(srdes));
	srdes.Format = format;
	srdes.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srdes.Texture2D.MostDetailedMip = 0;
	srdes.Texture2D.MipLevels = 1;
	pDevice->CreateShaderResourceView(texc, &srdes, &textureColor);
}

void AimAtPos(float x, float y)
{
	float TargetX = 0;
	float TargetY = 0;

	//X Axis
	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX = -(ScreenCenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX = x - ScreenCenterX;
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}

	//Y Axis
	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}


	if (aimspeed > 0)
	{
		//dont move mouse more than 50 pixels at time (ghetto HFR)
		float dirX = TargetX > 0 ? 1.0f : -1.0f;
		float dirY = TargetY > 0 ? 1.0f : -1.0f;
		TargetX = dirX * fmin(aimspeed, abs(TargetX));
		TargetY = dirY * fmin(aimspeed, abs(TargetY));
		mouse_event(MOUSEEVENTF_MOVE, (float)TargetX, (float)TargetY, NULL, NULL);
	}
	else
		if (TargetX != 0 && TargetY != 0)
			mouse_event(MOUSEEVENTF_MOVE, (float)TargetX, (float)TargetY, NULL, NULL);
}

//==========================================================================================================================

//w2s stuff
struct vec2
{
	float x, y;
};

struct vec3
{
	float x, y, z;
};

struct vec4
{
	float x, y, z, w;
};

void MapBuffer(ID3D11Buffer * pStageBuffer, void** ppData, UINT * pByteWidth)
{
	D3D11_MAPPED_SUBRESOURCE subRes;
	HRESULT res = pContext->Map(pStageBuffer, 0, D3D11_MAP_READ, 0, &subRes);

	D3D11_BUFFER_DESC desc;
	pStageBuffer->GetDesc(&desc);

	if (FAILED(res))
	{
		//Log("Map stage buffer failed {%d} {%d} {%d} {%d} {%d}", (void*)pStageBuffer, desc.ByteWidth, desc.BindFlags, desc.CPUAccessFlags, desc.Usage);
		SAFE_RELEASE(pStageBuffer); return;
	}
	
	*ppData = subRes.pData;
	
	if (pByteWidth)
		*pByteWidth = desc.ByteWidth;
}

void UnmapBuffer(ID3D11Buffer * pStageBuffer)
{
	pContext->Unmap(pStageBuffer, 0);
}

ID3D11Buffer* pStageBufferA = NULL;
ID3D11Buffer* CopyBufferToCpuA(ID3D11Buffer * pBufferA)
{
	D3D11_BUFFER_DESC CBDescA;
	pBufferA->GetDesc(&CBDescA);

	if (pStageBufferA == NULL)
	{
		//Log("onceA");
		// create buffer
		D3D11_BUFFER_DESC descA;
		descA.BindFlags = 0;
		descA.ByteWidth = CBDescA.ByteWidth;
		descA.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		descA.MiscFlags = 0;
		descA.StructureByteStride = 0;
		descA.Usage = D3D11_USAGE_STAGING;

		if (FAILED(pDevice->CreateBuffer(&descA, NULL, &pStageBufferA)))
		{
			//Log("CreateBuffer failed when CopyBufferToCpuA {}");
		}
	}

	if (pStageBufferA != NULL)
		pContext->CopyResource(pStageBufferA, pBufferA);

	return pStageBufferA;
}

ID3D11Buffer* pStageBufferB = NULL;
ID3D11Buffer* CopyBufferToCpuB(ID3D11Buffer * pBufferB)
{
	D3D11_BUFFER_DESC CBDescB;
	pBufferB->GetDesc(&CBDescB);

	if (pStageBufferB == NULL)
	{
		//Log("onceB");
		// create buffer
		D3D11_BUFFER_DESC descB;
		descB.BindFlags = 0;
		descB.ByteWidth = CBDescB.ByteWidth;
		descB.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		descB.MiscFlags = 0;
		descB.StructureByteStride = 0;
		descB.Usage = D3D11_USAGE_STAGING;

		if (FAILED(pDevice->CreateBuffer(&descB, NULL, &pStageBufferB)))
		{
			//Log("CreateBuffer failed when CopyBufferToCpuB {}");
		}
	}

	if (pStageBufferB != NULL)
		pContext->CopyResource(pStageBufferB, pBufferB);

	return pStageBufferB;
}

//get distance
float GetDst(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct AimEspInfo_t
{
	float vOutX, vOutY, vOutZ;
	float CrosshairDst;
};
std::vector<AimEspInfo_t>AimEspInfo;


//w2s
int WorldViewCBnum = 2; //2
int ProjCBnum = 1;//1
int matProjnum = 16;//16

ID3D11Buffer* pWorldViewCB = nullptr;
ID3D11Buffer* pProjCB = nullptr;

ID3D11Buffer* m_pCurWorldViewCB = NULL;
ID3D11Buffer* m_pCurProjCB = NULL;

float matWorldView[4][4];
float matProj[4][4];

float* worldview;
float* proj;

void AddModel(ID3D11DeviceContext * pContext)
{
	pContext->VSGetConstantBuffers(WorldViewCBnum, 1, &pWorldViewCB);	//WorldViewCBnum
	pContext->VSGetConstantBuffers(ProjCBnum, 1, &pProjCB);				//ProjCBnum

	if (pWorldViewCB == nullptr)
	{
		SAFE_RELEASE(pWorldViewCB); return;
	}
	if (pProjCB == nullptr)
	{
		SAFE_RELEASE(pProjCB); return;
	}

	if (pWorldViewCB != nullptr && pProjCB != nullptr)
	{
		m_pCurWorldViewCB = CopyBufferToCpuA(pWorldViewCB);
		m_pCurProjCB = CopyBufferToCpuB(pProjCB);
	}

	if (m_pCurWorldViewCB == nullptr)
	{
		SAFE_RELEASE(m_pCurWorldViewCB); return;
	}
	if (m_pCurProjCB == nullptr)
	{
		SAFE_RELEASE(m_pCurProjCB); return;
	}
	
	if (m_pCurWorldViewCB != nullptr && m_pCurProjCB != nullptr)
	{
		MapBuffer(m_pCurWorldViewCB, (void**)&worldview, NULL);
		memcpy(matWorldView, &worldview[0], sizeof(matWorldView));
		UnmapBuffer(m_pCurWorldViewCB);
		MapBuffer(m_pCurProjCB, (void**)&proj, NULL);
		memcpy(matProj, &proj[matProjnum], sizeof(matProj));			//matProjnum
		UnmapBuffer(m_pCurProjCB);
	}


	if(method1==1)
	{
		DirectX::XMVECTOR Pos = XMVectorSet(0.0f, 0.0f, aimheight, 1.0f);
		DirectX::XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply((FXMMATRIX)*matWorldView, (FXMMATRIX)*matProj);//multipication order matters

		//normal
		DirectX::XMMATRIX WorldViewProj = viewProjMatrix; //normal

		float mx = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[0] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[0] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[0] + WorldViewProj.r[3].m128_f32[0];
		float my = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[1] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[1] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[1] + WorldViewProj.r[3].m128_f32[1];
		float mz = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[2] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[2] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[2] + WorldViewProj.r[3].m128_f32[2];
		float mw = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[3] + WorldViewProj.r[3].m128_f32[3];

		float xx, yy;
		xx = ((mx / mw) * (ViewportWidth / 2.0f)) + (ViewportWidth / 2.0f);
		yy = (ViewportHeight / 2.0f) - ((my / mw) * (ViewportHeight / 2.0f)); //- or + depends on the game

		AimEspInfo_t pAimEspInfo = { static_cast<float>(xx), static_cast<float>(yy), static_cast<float>(mw) };
		AimEspInfo.push_back(pAimEspInfo);
	}

	if(method2==1)
	{
		DirectX::XMVECTOR Pos = XMVectorSet(0.0f, 0.0f, aimheight, 1.0f);
		DirectX::XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply((FXMMATRIX)*matWorldView, (FXMMATRIX)*matProj);//multipication order matters

		//transpose
		DirectX::XMMATRIX WorldViewProj = DirectX::XMMatrixTranspose(viewProjMatrix); //transpose

		float mx = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[0] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[0] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[0] + WorldViewProj.r[3].m128_f32[0];
		float my = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[1] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[1] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[1] + WorldViewProj.r[3].m128_f32[1];
		float mz = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[2] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[2] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[2] + WorldViewProj.r[3].m128_f32[2];
		float mw = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[3] + WorldViewProj.r[3].m128_f32[3];

		float xx, yy;
		xx = ((mx / mw) * (ViewportWidth / 2.0f)) + (ViewportWidth / 2.0f);
		yy = (ViewportHeight / 2.0f) - ((my / mw) * (ViewportHeight / 2.0f)); //- or + depends on the game

		AimEspInfo_t pAimEspInfo = { static_cast<float>(xx), static_cast<float>(yy), static_cast<float>(mw) };
		AimEspInfo.push_back(pAimEspInfo);
	}

	if (method3 == 1)
	{
	 //
	}

	if (method4 == 1)
	{
	//
	}

}


#include <string>
#include <fstream>
void SaveCfg()
{
	ofstream fout;
	fout.open(GetDirFile("w2sf.ini"), ios::trunc);
	fout << "Wallhack " << wallhack << endl;
	fout << "Chams " << chams << endl;
	fout << "Esp " << esp << endl;
	fout << "Aimbot " << aimbot << endl;
	fout << "Aimkey " << aimkey << endl;
	fout << "Aimfov " << aimfov << endl;
	fout << "Aimspeedisbasedondistance " << aimspeed_isbasedon_distance << endl;
	fout << "Aimspeed " << aimspeed << endl;
	fout << "Aimheight " << aimheight << endl;
	fout << "Autoshoot " << autoshoot << endl;
	fout << "AsXhairdst " << as_xhairdst << endl;
	fout << "Modelrecfinder " << modelrecfinder << endl;
	fout << "Wtsfinder " << wtsfinder << endl;
	fout << "Method1 " << method1 << endl;
	fout << "Method2 " << method2 << endl;
	fout << "Method3 " << method3 << endl;
	fout << "Method4 " << method4 << endl;
	fout << "WorldViewCBnum " << WorldViewCBnum << endl;
	fout << "ProjCBnum " << ProjCBnum << endl;
	fout << "matProjnum " << matProjnum << endl;
	fout.close();
}

void LoadCfg()
{
	ifstream fin;
	string Word = "";
	fin.open(GetDirFile("w2sf.ini"), ifstream::in);
	fin >> Word >> wallhack;
	fin >> Word >> chams;
	fin >> Word >> esp;
	fin >> Word >> aimbot;
	fin >> Word >> aimkey;
	fin >> Word >> aimfov;
	fin >> Word >> aimspeed_isbasedon_distance;
	fin >> Word >> aimspeed;
	fin >> Word >> aimheight;
	fin >> Word >> autoshoot;
	fin >> Word >> as_xhairdst;
	fin >> Word >> modelrecfinder;
	fin >> Word >> wtsfinder;
	fin >> Word >> method1;
	fin >> Word >> method2;
	fin >> Word >> method3;
	fin >> Word >> method4;
	fin >> Word >> WorldViewCBnum;
	fin >> Word >> ProjCBnum;
	fin >> Word >> matProjnum;
	fin.close();
}