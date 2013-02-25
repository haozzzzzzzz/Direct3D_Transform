#include <d3d9.h>//Direct3D头文件
#include <d3dx9.h>//D3DX库的头文件

//释放Com对象宏
#define SAFE_RELEASE(p) {if(p){(p)->Release();(p)=NULL;}}


wchar_t*g_pClassName=L"Transform";//窗口类名
wchar_t*g_pWindowName=L"空间坐标变换";//窗口标题名

LPDIRECT3DDEVICE9 g_pd3dDevice=NULL;//D3D设备接口
LPDIRECT3DINDEXBUFFER9 g_pIndexBuf=NULL;//索引缓存接口
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuf=NULL;//顶点缓存接口

/*顶点结构*/
struct CUSTOMVERTEX
{
	FLOAT _x,_y,_z;//顶点位置
	DWORD _color;//顶点颜色
	CUSTOMVERTEX(FLOAT x,FLOAT y,FLOAT z,DWORD color):_x(x),_y(y),_z(z),_color(color){}
};

//顶点格式
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)

HRESULT InitDirect3D(HWND hWnd);//初始化Direct3D
void Direct3DRender();//渲染图形
void Direct3DCleanup();//清理Direct3D资源

//顶点缓存
void CreateVertexBuffer();
//坐标转换 绘制之前先对立方体的坐标进行转换
void SetTransform();
//绘制立方体
void DrawPrimitive();

//窗口过程函数
LRESULT CALLBACK WinMainProc(HWND,UINT,WPARAM,LPARAM);


//-----------------------------------------
//NAME : WinMain();
//DESC : Windows应用程序入口函数
//-----------------------------------------

int WINAPI WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd )
{
	//设计窗口类
	WNDCLASS wndclass;
	wndclass.cbClsExtra=0;
	wndclass.cbWndExtra=0;
	wndclass.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.hCursor=LoadCursor(NULL,IDC_ARROW);
	wndclass.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wndclass.hInstance=hInstance;
	wndclass.lpfnWndProc=WinMainProc;
	wndclass.lpszClassName=g_pClassName;
	wndclass.lpszMenuName=NULL;
	wndclass.style=CS_HREDRAW|CS_VREDRAW;

	//注册窗口
	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL,L"注册窗口失败",L"错误提示",NULL);
		return 1;
	}


	//创建窗口
	HWND hWnd=CreateWindow(g_pClassName,g_pWindowName,WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,640,480,NULL,NULL,hInstance,NULL);
	if (!hWnd)
	{
		MessageBox(NULL,L"创建窗口失败",L"错误提示",NULL);
		return 1;
	}

	//初始化Direct3D
	InitDirect3D(hWnd);

	//显示窗口
	ShowWindow(hWnd,SW_SHOWDEFAULT);

	//更新窗口
	UpdateWindow(hWnd);

	//处理消息
	MSG msg;
	ZeroMemory(&msg,sizeof(msg));

	while (msg.message!=WM_QUIT)
	{
		//从消息队列中取消息并珊瑚队列中消息
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Direct3DRender();//绘制3D场景
		}
	}

	//取消注册窗口
	UnregisterClass(g_pClassName,wndclass.hInstance);
	return 0;
}

//-------------------------------------
//NAME : WinMainProc()
//DESC : 窗口过程函数
//-------------------------------------

LRESULT CALLBACK WinMainProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:	//客户区重绘
		Direct3DRender();//渲染图形
		ValidateRect(hWnd,NULL);//更新客户区显示
		break;
	case WM_DESTROY://窗口销毁消息
		Direct3DCleanup();//清理Direct3D
		PostQuitMessage(0);//退出
		break;
	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE)//ESC键
		{
			DestroyWindow(hWnd);//销毁窗口，并发送一条WM_DESTROY消息
		}
		break;
	}
	return DefWindowProc(hWnd,msg,wParam,lParam);
}

//------------------------------------
//NAME : InitDirect3D()
//DESC : Direct3D初始化
//------------------------------------
HRESULT InitDirect3D(HWND hWnd)
{
	//创建IDirect3D接口
	LPDIRECT3D9 pD3D=NULL;//IDirect3D9接口
	pD3D=Direct3DCreate9(D3D_SDK_VERSION);//创建IDirect3D9接口对象
	//D3D_SDK_VERSION参数表示当前使用的DirectX SDK版本，用于确保应用程序所有包含的头文件在编译时能够与DirectX运行时的DLL相匹配

	if (pD3D==NULL)
	{
		return E_FAIL;
	}


	//获取硬件设备信息
	//IDirect3D9接口提供了GetDeviceCaps方法获取指定设备的性能参数，该方法将所取得的硬件设备信息保存到一个D3DCAPS9结构中
	/*
	HRESULT IDirect3D9::GetDeviceCaps( 
	UINT  Adapter, 
	D3DDEVTYPE  DeviceType, 
	D3DCAPS9  *pCaps 
	);
	*/
	D3DCAPS9 caps;
	int vp=0;
	pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,&caps);
	if (caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		vp=D3DCREATE_HARDWARE_VERTEXPROCESSING;
	}
	else
		vp=D3DCREATE_SOFTWARE_VERTEXPROCESSING;




	//创建Direct3D设备接口
	//在创建IDirect3DDevice9接口对象之前，需要准备一个D3DPRESENT_PARAMETERS结构类型的参数，用于说明如何创建Direct3D设备
	/*
	typedef struct _D3DPRESENT_PARAMETERS_ {
	UINT 			BackBufferWidth;//后台缓冲区宽度
	UINT 			BackBufferHeight; //后台缓冲区高度
	D3DFORMAT   		BackBufferFormat;//...保存像素的格式
	UINT 			BackBufferCount;//..数量
	D3DMULTISAMPLE_TYPE  	MultiSampleType;//多重取样类型
	DWORD   			MultiSampleQuality;//多重取样质量
	D3DSWAPEFFECT  		SwapEffect;//后台缓冲区内容复制到前台缓冲区的方式
	HWND 			hDeviceWindow;//绘制图形的窗口句柄
	BOOL 			Windowed;
	BOOL 			EnableAutoDepthStencil;//Direct3D是否为应用程序自动管理内存深度
	D3DFORMAT   		AutoDepthStencilFormat;//方式
	DWORD   			Flags;//附加属性 通常为0
	UINT 			FullScreen_RefreshRateInHz;//全屏模式时指定屏幕刷新率
	UINT 			PresentationInterval;//指定平面的翻转模式，在窗口模式下 其取值为0
	} D3DPRESENT_PARAMETERS;
	*/
	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory(&d3dpp, sizeof(d3dpp));


	d3dpp.BackBufferWidth=640;
	d3dpp.BackBufferHeight=480;
	d3dpp.BackBufferFormat=D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount=1;
	d3dpp.MultiSampleType=D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality=0;
	d3dpp.SwapEffect=D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow=hWnd;
	d3dpp.Windowed=TRUE;
	d3dpp.EnableAutoDepthStencil=TRUE;
	d3dpp.AutoDepthStencilFormat=D3DFMT_D24S8;
	d3dpp.Flags=0;
	d3dpp.FullScreen_RefreshRateInHz=D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;

	//在填充D3DPRESENT_PARAMETERS结构后，可以调用IDirect3D9接口的CreateDeivce方法创建IDirect3DDevice9接口对象
	/*
	HRESULT IDirect3DDevice9::CreateDevice( 
	UINT     Adapter, //表示将创建的IDirect3DDevice9接口对象所代表的显卡类型
	D3DDEVTYPE  DeviceType, 
	HWND   hFocusWindow, 
	DWORD  BehaviorFlags, //Direct3D设备进行3D运算的方式
	D3DPRESENT_PARAMETERS  *pPresentationParameters,
	IDirect3DDevice9  **ppReturnedDeviceInterface 
	);
	*/
	//IDirect3D9接口的CreateDevice函数返回一个HRESULT类型的返回值，可以通过SUCCESSED和FALIED宏判断该函数的执行结果。如果CreateDevice函数执行成功，SUCCESSED宏将返回TRUE，而FAILED宏则返回FALSE。
	pD3D->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hWnd,vp,&d3dpp,&g_pd3dDevice);

	pD3D->Release();//释放Direct3D接口


	//创建顶点缓存
	CreateVertexBuffer();
	return S_OK;
}

//------------------------------------
//NAME : Direct3DRender()
//DESC : 绘制3D场景
//------------------------------------
void Direct3DRender()
{
	/*
	Direct3D的绘制过程就是：绘制→显示→绘制→显示。
	但是，每当开始绘制图形之前，都需要通过IDirect3DDevice9接口的Clear方法将后台缓存中的内容进行清空，并设置表面的填充颜色等

	HRESULT IDirect3DDevice9::Clear( 
	DWORD   Count,
	const D3DRECT *pRects, //指定对表面指定的矩形区域进行清除操作，数组中包含的矩形的数量由Count参数指定
	DWORD    Flags, //指定了需要清除的表面，该参数可以取值于D3DCLEAR_TARGET、D3DCLEAR_ZBUFFER和D3DCLEAR_STENCIL，分别表示对后台缓存、深度缓存和模板缓存进行清除
	D3DCOLOR Color, //用于指定在清除缓存内容后设置的背景色，可以通过D3DCOLOR_XRGB宏将RGB值转换给该参数
	float        Z, //指定当清除缓存内容后设置深度缓存的值
	DWORD    Stencil 
	); 

	*/
	g_pd3dDevice->Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(0,0,0),1.0f,0);//黑色背景

	//开始绘制
	g_pd3dDevice->BeginScene();

	//在绘制立方体之前，分别取得绕x,y,z轴旋转变换的矩阵，然后将他们组合并让立方体同时绕这3个轴旋转
	SetTransform();

	/*图形绘制的实际过程*/
	DrawPrimitive();

	//结束绘制
	g_pd3dDevice->EndScene();

	//翻转
	g_pd3dDevice->Present(NULL,NULL,NULL,NULL);
}

//------------------------------------
//NAME : Direct3DCleanup()
//DESV : 清理Direct3D，并释放COM接口
//------------------------------------
void Direct3DCleanup()
{
	SAFE_RELEASE(g_pVertexBuf);
	SAFE_RELEASE(g_pIndexBuf);
	SAFE_RELEASE(g_pd3dDevice);
}

//--------------------------------------
//NAME : CreateVertexBuffer()
//DESC : 创建图形顶点缓存
//--------------------------------------
void CreateVertexBuffer()
{
	//顶点缓存（Vertex Buffer）是Direct3D用来保存顶点数据的内存空间，可以位于系统内存和图形卡的显存中。
	//当Direct3D绘制图形时，将根据这些顶点结构创建一个三角形列表来描述物体的形状和轮廓
	/*
	在Direct3D中，顶点缓存由IDirect3DVertexBuffer9接口对象表示。在使用顶点缓存之前，需要定义描述顶点的顶点结构，然后可以通过IDirect3DDevice9接口的CreateVertexBuffer方法创建顶点缓存

	HRESULT IDirect3DDevice9::CreateVertexBuffer(
	UINT       Length,
	DWORD    Usage, //指定使用缓存的一些附加属性
	DWORD    FVF, //将要存储在顶点缓存中的灵活顶点格式
	D3DPOOL   Pool, //一个D3DPOOL枚举类型，用于指定存储顶点缓存或索引缓冲的内存位置，在默认情况下时位于显存中
	IDirect3DVertexBuffer9**  ppVertexBuffer, //IDirect3DVertexBuffer9接口类型的变量的指针，分别用于接收创建的顶点缓存和索引缓存的指针
	HANDLE*   pSharedHandle
	);

	*/
	//创建顶点缓存
	g_pd3dDevice->CreateVertexBuffer(8*sizeof(CUSTOMVERTEX),0,D3DFVF_CUSTOMVERTEX,D3DPOOL_DEFAULT,&g_pVertexBuf,NULL);

	//填充顶点数据
	CUSTOMVERTEX*pVertices=NULL;

	/*
	在取得IDirect3DVertexBuffer9接口对象的指针后，就可以通过这该接口的Lock方法获取指向顶点缓存的存储区的指针，并通过该指针访问缓存中的数据

	HRESULT IDirect3DVertexBuffer9::Lock(
	UINT     OffsetToLock, //在存储区中加锁的起始位置
	UINT     SizeToLock, //以字节为单位的加锁的存储区大小，值为0表示整个缓存存储区
	BYTE  **ppbData, //用于接收被锁定的存储区起始位置的指针，该参数指向的存储区的读写方式由Flags参数指定
	DWORD  Flags
	);

	*/

	//锁定缓存区
	g_pVertexBuf->Lock(0,0,(void**)&pVertices,0);
	pVertices[0]=CUSTOMVERTEX(-5.0f,5.0f,-5.0f,D3DCOLOR_XRGB(255,0,0));
	pVertices[1]=CUSTOMVERTEX(-5.0f,5.0f,5.0f,D3DCOLOR_XRGB(0,255,0));
	pVertices[2]=CUSTOMVERTEX(5.0f,5.0f,5.0f,D3DCOLOR_XRGB(0,0,255));
	pVertices[3]=CUSTOMVERTEX(5.0f,5.0f,-5.0f,D3DCOLOR_XRGB(255,255,0));
	pVertices[4]=CUSTOMVERTEX(-5.0f,-5.0f,-5.0f,D3DCOLOR_XRGB(0,0,255));
	pVertices[5]=CUSTOMVERTEX(-5.0f,-5.0f,5.0f,D3DCOLOR_XRGB(255,255,0));
	pVertices[6]=CUSTOMVERTEX(5.0f,-5.0f,5.0f,D3DCOLOR_XRGB(255,0,0));
	pVertices[7]=CUSTOMVERTEX(5.0f,-5.0f,-5.0f,D3DCOLOR_XRGB(0,255,0));

	//当使用Lock方法对应缓存区进行加锁并完成数据的访问后，需要调用IDirect3DVertexBuffer9接口的Unlock方法对缓存进行解锁
	g_pVertexBuf->Unlock();

	//创建索引缓存
	g_pd3dDevice->CreateIndexBuffer(36*sizeof(WORD),0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&g_pIndexBuf,NULL);

	//填充索引数据
	WORD*pIndices=NULL;
	g_pIndexBuf->Lock(0,0,(void**)&pIndices,0);

	//顶面由两个三角形组成
	pIndices[0]=0,pIndices[1]=1,pIndices[2]=2;
	pIndices[3]=0,pIndices[4]=2,pIndices[5]=3;

	//正面
	pIndices[6]=0,pIndices[7]=3,pIndices[8]=7;
	pIndices[9]=0,pIndices[10]=7,pIndices[11]=4;

	//左侧面
	pIndices[12]=0,pIndices[13]=4,pIndices[14]=5;
	pIndices[15]=0,pIndices[16]=5,pIndices[17]=1;

	//右侧面
	pIndices[18]=2,pIndices[19]=6,pIndices[20]=7;
	pIndices[21]=2,pIndices[22]=7,pIndices[23]=3;

	//背面
	pIndices[24]=2,pIndices[25]=5,pIndices[26]=6;
	pIndices[27]=2,pIndices[28]=1,pIndices[29]=5;

	//底面
	pIndices[30]=4,pIndices[31]=6,pIndices[32]=5;
	pIndices[33]=4,pIndices[34]=7,pIndices[35]=6;
	g_pIndexBuf->Unlock();
}


//---------------------------------------
//NAME : SetTransform()
//DESC : 坐标转换
//---------------------------------------
void SetTransform()
{
	//设置世界变换矩阵
	D3DXMATRIX matWorld,Rx,Ry,Rz;
	D3DXMatrixIdentity(&matWorld);//单位矩阵
	D3DXMatrixRotationX(&Rx,::timeGetTime()/1000.f);//绕x轴旋转
	D3DXMatrixRotationY(&Ry,::timeGetTime()/1000.f);//绕y轴旋转
	D3DXMatrixRotationZ(&Rz,::timeGetTime()/1000.f);//绕z轴旋转

	matWorld=Rx*Ry*Rz*matWorld;
	g_pd3dDevice->SetTransform(D3DTS_WORLD,&matWorld);

	//设置取景变换矩阵
	D3DXMATRIX matView;
	D3DXVECTOR3 vEye(0.0f,0.0f,-30.0f);
	D3DXVECTOR3 vAt(0.0f,0.0f,0.0f);
	D3DXVECTOR3 vUp(0.0f,1.0f,0.0f);
	D3DXMatrixLookAtLH(&matView,&vEye,&vAt,&vUp);
	g_pd3dDevice->SetTransform(D3DTS_VIEW,&matView);

	//设置投影变换矩阵
	D3DXMATRIX matProj;
	D3DXMatrixPerspectiveFovLH(&matProj,D3DX_PI/4.0f,1.0f,1.0f,1000.0f);
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION,&matProj);
}

//----------------------------------------
//Name : DrawPrimitive()
//DESC : 绘制顶点缓存中的图形
//----------------------------------------
void DrawPrimitive()
{
	//设置渲染状态
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING,false);
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);



	//渲染三角形

	//需要通过IDirect3DDevice9接口的SetStreamSource方法将包含几何体信息的顶点缓存与渲染流水线相关联
	/*
	HRESULT IDirect3DDevice9::SetStreamSource(
	UINT  StreamNumber, //用于指定与该顶点缓存建立连接的数据流
	IDirect3DVertexBuffer9 *pStreamData, //包含顶点数据的顶点缓存指针
	UINT  OffsetInBytes, //在数据流中以字节为单位的偏移量
	UINT  Stride //在顶点缓冲中存储的每个顶点结构的大小，可以通过sizeof运算符计算顶点结构的实际大小
	);

	*/
	//渲染三角形
	g_pd3dDevice->SetStreamSource(0,g_pVertexBuf,0,sizeof(CUSTOMVERTEX));

	//调用IDirect3DDevice9接口的SetFVF方法指定顶点格式
	g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

	//将物体模型的顶点填充到顶点缓存，并设置数据流输入源和顶点格式后，可以调用IDirect3DDevice9接口的DrawPrimitive方法根据顶点缓存中的顶点绘制模型。
	/*
	HRESULT IDirect3DDevice9::DrawPrimitive(
	D3DPRIMITIVETYPE  PrimitiveType,
	UINT  StartVertex,
	UINT  PrimitiveCount
	);

	*/
	//利用顶点缓存绘制矩形
	//g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,2);


	//利用索引缓存绘制矩形
	g_pd3dDevice->SetIndices(g_pIndexBuf);
	g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,8,0,12);
}