#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <functional>
#include <random>
#include <ctime>

#pragma comment (lib, "Msimg32.lib")

#define BW 600
#define BH 600
#define SPEEDMAX 20
#define ACCELMAX 5

#define RGBColor //OR MONOTONE
#define Dissipate //Alpha - Blending
#define Circle

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void BallMove();

std::function<int()> RandVal = std::bind(std::uniform_int_distribution < int > {MININT,MAXINT}, std::default_random_engine{ (unsigned int)time(0) });


void PNoise(HDC&,int,int);
double lerp(double, double,double);
void GradientVector(std::vector<std::vector<std::pair<double, double>>>& src, int GridSize);
double NoiseAtPoint(std::vector<std::vector<std::pair<double, double>>>& grV, double x, double y);


class Ball
{
private:
	int x, y;
	double vx, vy;
	double ax, ay;
	int size;
	HBRUSH Brush;
	static std::function<int()> GetSpeed;
	static std::function<int()> GetPos;
	static std::function<int()> GetColor;
	static std::function<int()> GetSize;
	void Move();
	void Accelerate(int,int,bool);
	void Friction();
	void Invert();
	friend void BallMove();
	friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
public:
	Ball() :ax{RandVal()%ACCELMAX}, ay{RandVal()%ACCELMAX},x{ abs(RandVal() % BW) }, y{ abs(RandVal() % BH) }, vx{ RandVal() % SPEEDMAX }, vy{ RandVal() % SPEEDMAX }, Brush{CreateSolidBrush(RGB(RandVal() % 256, RandVal() % 256, RandVal() % 256)) }, size{ RandVal() % 10 + 3 }{};
	~Ball(){ DeleteObject(Brush); }
};

void Ball::Move()
{
	vx += ax;
	vy += ay;
	x += vx;
	y += vy;
	
	if (abs(vx) > SPEEDMAX) vx *= SPEEDMAX / abs(vx) * 0.9;
	if (abs(vy) > SPEEDMAX) vy *= SPEEDMAX / abs(vy) * 0.9;

	Friction();

	if (y < 0) vy = abs(vy);
	else if (y > BH) vy = -abs(vy);
	if (x < 0) vx = abs(vx);
	else if (x > BW) vx = -abs(vx);
}

void Ball::Accelerate(int Mx, int My, bool Noise)
{
	static std::vector<std::vector<std::pair<double, double>>> Grvx;
	static std::vector<std::vector<std::pair<double, double>>> Grvy;
	if (!Grvx.size()) GradientVector(Grvx, SPEEDMAX*2); //Make into gradient vector
	if (!Grvy.size()) GradientVector(Grvy, SPEEDMAX*2);


	if (!Noise)
	{
		double angle = atan2(double(My-y),double(Mx-x));
		int rsqr = (My - y)*(My - y) + (Mx - x)*(Mx - x);
		//ax = lerp(0.001, ax, (Mx - x));
		//ay = lerp(0.001, ay, (My - y)); //weighted average.
		//ax = lerp(0.1, ax, cos(angle) / rsqr);
		//ay = lerp(0.1, ax, sin(angle) / rsqr);
		if (rsqr>3000)
		{
			ax = cos(angle) * 1000*size/ rsqr;
			ay = sin(angle) * 1000*size/ rsqr;
		}
		else
		{
			ax = -cos(angle) * 1000*size/ rsqr;
			ay = -sin(angle) * 1000*size/ rsqr;
		}

	}
	else
	{
		ax = ACCELMAX * NoiseAtPoint(Grvx, vx + SPEEDMAX, vy + SPEEDMAX);
		ay = ACCELMAX * NoiseAtPoint(Grvy, vy + SPEEDMAX, vx + SPEEDMAX);
	}

}

void Ball::Friction()
{
	/*
	if (ax > 0) --ax;
	else if (ax < 0) ++ax;
	if (ay > 0) --ay;
	else if (ax < 0) ++ay;

	if (vx>SPEEDMAX/2) --vx;
	else if (vx < -SPEEDMAX/2) ++vx;
	if (vy >SPEEDMAX/2) --vy;
	else if (vx < -SPEEDMAX/2) ++vy;
	*/
	if(abs(vx)>SPEEDMAX/2)vx *= 0.9;
	if (abs(vy)>SPEEDMAX / 2)vy *= 0.9;
	ax *= 0.9;
	ay *= 0.9;
}
void Ball::Invert()
{
	vx = -vx;
	vy = -vy;
}

HINSTANCE g_hInst;
LPCTSTR Title = L"Particle Movement";
HWND hMainWnd;

Ball Balls[500];

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	g_hInst = hInstance;

	WNDCLASS WndClass;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = Title;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_VREDRAW | CS_HREDRAW;
	RegisterClass(&WndClass);

	hMainWnd = CreateWindow(Title, Title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	ShowWindow(hMainWnd, nCmdShow);

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	static BLENDFUNCTION Bf;
	
	static HDC MemDC;
	static HBITMAP NewBit, OldBit;
	static bool Gravity = false;
	static bool Noise = false;
	static int Mx, My;
	switch (iMsg)
	{
	case WM_CREATE:
		SetTimer(hWnd, 0, 30, 0);
		SetTimer(hWnd, 1, 3000, 0);//inverter
#ifdef Dissipate
		Bf.AlphaFormat = AC_SRC_ALPHA;
		Bf.BlendFlags = 0;
		Bf.BlendOp = AC_SRC_OVER;
		Bf.SourceConstantAlpha = 20;

		hdc = GetDC(hWnd);

		MemDC = CreateCompatibleDC(hdc);
		NewBit = CreateCompatibleBitmap(hdc, 600, 600);
		OldBit = (HBITMAP)SelectObject(MemDC, NewBit);
		RECT R;
		SetRect(&R, 0, 0, 600, 600);
		FillRect(MemDC, &R, (HBRUSH)GetStockObject(WHITE_BRUSH));
		//FillRect(MemDC, &R, CreateSolidBrush(RGB(30,30,250)));
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE);
		SetWindowPos(hWnd, NULL, 0, 0, R.right - R.left, R.bottom - R.top, SWP_NOMOVE);
#endif
		break;
	case WM_LBUTTONDOWN:
		Gravity = true;
		Noise = false;
		Mx = LOWORD(lParam);
		My = HIWORD(lParam);
		break;
	case WM_LBUTTONUP:
		Gravity = false;
		break;
	case WM_RBUTTONDOWN:
		Gravity = true;
		Noise = true;
		Mx = LOWORD(lParam);
		My = HIWORD(lParam);
		break;
	case WM_RBUTTONUP:
		Gravity = false;
		break;
	case WM_MOUSEMOVE:
		Mx = LOWORD(lParam);
		My = HIWORD(lParam);
		break;
	case WM_KEYDOWN:
		hdc = GetDC(hWnd);
		PNoise(hdc, 30,20); //resolution = how zoomed, N = how big
		ReleaseDC(hWnd, hdc);
		break;
	case WM_TIMER:
		switch (wParam)
		{
		case 0:
			if (Gravity)
			{
				for (auto& B : Balls)
				{
					B.Accelerate(Mx, My, Noise);
				}
			}
			BallMove();
			break;
		case 1:
			for (auto& B : Balls)
			{
				B.Invert();
			}
			break;
		}
		break;
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
#ifdef Dissipate
		AlphaBlend(hdc, 0, 0, 600,600, MemDC, 0, 0, 600, 600, Bf);
#endif
		
		SelectObject(hdc, GetStockObject(NULL_PEN));
	
		for (auto& B : Balls)
		{
			SelectObject(hdc, B.Brush);
#ifdef Ninja
			int S = B.size;
			while (S)
			{
				Ellipse(hdc, B.x - S, B.y - S/2, B.x + S/2, B.y + S/2);
				Ellipse(hdc, B.x - S/2, B.y - S, B.x + S/2, B.y + S/2);
				Ellipse(hdc, B.x - S/2, B.y - S/2, B.x + S, B.y + S/2);
				Ellipse(hdc, B.x - S/2, B.y - S/2, B.x + S/2, B.y + S);
				S /= 2;
			}
#endif
#ifdef Circle
			Ellipse(hdc, B.x - B.size, B.y - B.size, B.x + B.size, B.y + B.size);
#endif
		}

		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
#ifdef Dissipate
		SelectObject(MemDC, OldBit);
		DeleteObject(NewBit);
		DeleteDC(MemDC);
#endif
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}

void BallMove()
{
	for (auto& B : Balls)
	{
		B.Move();
	}
	InvalidateRect(hMainWnd, NULL, FALSE);
}


std::pair<double,double>&& RandVect()
{
	double angle = (RandVal() % 360)*3.14159265358979 / 180;
	return std::make_pair(cos(angle), sin(angle));
}
void GradientVector(std::vector<std::vector<std::pair<double, double>>>& src, int GridSize)
{
	src.clear();
	for (int i = 0; i < GridSize + 2; i++)
	{
		src.push_back(std::vector<std::pair<double, double>>());
		for (int j = 0; j < GridSize + 2; j++)
		{
			src.back().push_back(RandVect());
		}
	}
}
double DotProduct(std::pair<double,double> a, std::pair<double,double> b)
{
	return a.first*b.first + a.second*b.second;
}
double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
double lerp(double t, double a, double b){ return (1 - t)*a + t*b; }

double NoiseAtPoint(std::vector<std::vector<std::pair<double, double>>>& grV, double x, double y)
{
	int J = (int)x; //floor
	int I = (int)y;
	x -= J;
	y -= I;

	double u = fade(x);
	double v = fade(y);
	return lerp(v,
		lerp(u, DotProduct(grV[I][J], { x, y }), DotProduct(grV[I][J + 1], { x - 1, y })),
		lerp(u, DotProduct(grV[I + 1][J], { x, y - 1 }), DotProduct(grV[I + 1][J + 1], { x - 1, y - 1 }))
		);
}

//N = Size, Res = Resolution(ZOOM), 
void PNoise(HDC& hdc, int N,int Res)
{
#ifdef RGBColor
	std::vector<std::vector<std::pair<double, double>>> GrvR;
	std::vector<std::vector<std::pair<double, double>>> GrvG;
	std::vector<std::vector<std::pair<double, double>>> GrvB;
	GradientVector(GrvR, N);
	GradientVector(GrvG, N);
	GradientVector(GrvB, N);
#endif

#ifdef Monotone
	std::vector<std::vector<std::pair<double, double>>> Grv;
	GradientVector(Grv, N); //5*5 Grid(0~5)
#endif
	for (double i = 0; i < N; i += 1.0/Res)
	{
		for (double j = 0; j < N; j += 1.0/Res)
		{
			//int C = 128 + 128 * NoiseAtPoint(Grv, j, i);
#ifdef RGBColor
			int R = 256 * NoiseAtPoint(GrvR, j, i) / sqrt(0.5);
			int G = 256 * NoiseAtPoint(GrvG, j, i) / sqrt(0.5);
			int B = 256 * NoiseAtPoint(GrvB, j, i) / sqrt(0.5);
			SetPixel(hdc, j*Res, i*Res, RGB(R, G, B));
#endif

#ifdef Monotone
			int C = 256 * NoiseAtPoint(Grv, j, i) / sqrt(0.5);
			SetPixel(hdc, j * Res, i * Res, RGB(C, C, C));
#endif
		}
	}
}