#include<cstdio>
#include<graphics.h>//这个是easyx的头文件，可以实现图片操作
#include "tools.h"//这个tool是用来消除png图片的黑边的
#include<iostream>
#include<time.h>
#include"vector2.h"
#include<math.h>
#include<cstdlib>
//播放音乐用的头文件
#include<mmsystem.h>
#pragma comment(lib,"winmm.lib")


using namespace std;

//窗口的大小，后续可以更改
#define WIN_WIDTH 900
#define WIN_HIGHT 600
#define ZM_MAX 20

//卡牌枚举，用来表示卡牌的顺序和数量
enum { WAN_DOU, XIANG_RI_KUI, JIAN_GUO, TU_DOU, DA_ZUI_HUA, ZHI_WU_COUNT };
int zhiWuCosts[] = { 0, 100,50,50,25,150 };

enum { PU_TONG, MAO_ZI, TIE_TONG, ZM_COUNT };


//图片
IMAGE imgBg;//背景图片
IMAGE imgBar;//卡牌槽
IMAGE imgShovel;
IMAGE imgShovelSlot;
IMAGE imgGameMenuButton;
IMAGE imgGameMenu;

IMAGE imgCards[ZHI_WU_COUNT];//植物卡牌图片数组

IMAGE test;

//植物的状态，这是植物的种类和动图
IMAGE* imgZhiWu[ZHI_WU_COUNT+1][20+1];//利用指针，因为每个植物有不同的图片数量，取一个最大值


int curX, curY;//移动植物过程中，鼠标的位置，现在修改为鼠标的实时位置，可以进行界面操作了

int curZhiWu;
bool shovel = false;

enum { GOING, WIN, FAIL };
int killCount;//已经杀掉的僵尸数量
int zmCount;//已经出现的僵尸个数
int gameStatus;//游戏状态

typedef struct zhiwu
{
	int type;//0表示没有植物，1表示第一种植物，后面以此类推
	int frameIndex;//序列的帧序号
	int shootTime;
	int blood;//死亡计数器，吃几口才死
	int cost;
	int x, y;
	int timer;

}zhiwu;//结构体植物类型

zhiwu map[5][9];//创建一个二维数组，表示地图上的每一个格子，每个格子都有植物的类型和植物的帧序列号两种属性

enum { SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_PRODUCT };

typedef struct sunshineBall
{
	double x, y;//当前位置
	int frameIndex;//表示当前图片帧的序号
	int destY;//飘落的目标位置的y坐标，因为x不会变
	bool used;//是否在使用
	int timer;

	double xoff;
	double yoff;
	double t;//贝塞尔曲线的时间点0到1之间
	vector2 p1, p2, p3, p4;
	vector2 pCur;//当前时刻阳光的位置
	double speed;
	int status;

}sunshineBall;

//预先准备阳光，存到数组里面，要用的时候拿

struct sunshineBall balls[10];
IMAGE imgSunshineBall[29];

int sunshine;


struct zm {
	double x, y;
	int row;
	int frameIndex;
	int type;
	bool used;
	int blood;
	int speed;
	bool dead;
	bool eating;
}zm;
struct zm zms[10];
IMAGE* imgZM[ZM_COUNT+1][22+1];
IMAGE imgZMDead[20];
IMAGE* imgZMEat[ZM_COUNT+1][21+1];

struct bullet
{
	int x, y;
	int row;
	bool used;
	int speed;
	bool blast;//子弹是否发生爆炸
	int frameIndex;
};
struct bullet bullets[30];
IMAGE imgBulletNormal;
IMAGE imgBullBlast[4];
IMAGE imgZmStand[22];


//判断文件是否存在
bool fileExist(const char* name)
{
	FILE* fp = fopen(name, "r");
	if (fp == NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		return true;

	}
}

//游戏初始化
void gameInit()
{
	loadimage(&imgBg, "res/map/Background_1.jpg");//这里的文件路径一定要写对，不用英文
	loadimage(&imgBar, "res/bar5.png");
	loadimage(&imgShovel, "res/shovel.png");
	loadimage(&imgShovelSlot, "res/shovelSlot.png");
	loadimage(&imgGameMenu, "res/pauseMenu.png");
	loadimage(&imgGameMenuButton, "res/pause.png");

	memset(imgZhiWu, 0, sizeof(imgZhiWu));
	memset(imgZM, 0, sizeof(imgZM));
	memset(imgZMEat, 0, sizeof(imgZMEat));
	memset(map, 0, sizeof(map));//让一开始草地上都没有植物

	killCount = 0;
	zmCount = 0;
	gameStatus = GOING;
	//录入卡牌，注意卡牌是从1开始命名的
	char name[100];//这里的name是路径
	for (int i = 0; i < ZHI_WU_COUNT; i++)
	{
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
		loadimage(&imgCards[i], name);


		//录入植物状态
		for (int j = 0; j < 20; j++)
		{
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
			//先判断文件是否存在，如果存在就加载图片
			if (fileExist(name))
			{
				imgZhiWu[i][j] = new IMAGE;
				loadimage(imgZhiWu[i][j], name);
			}
			else
			{
				break;
			}
		}
	}

	sunshine = 100;

	//录入阳光图片
	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunshine/%d 拷贝.png", i + 1);
		loadimage(&imgSunshineBall[i], name);
	}

	srand(time(NULL));

	initgraph(WIN_WIDTH, WIN_HIGHT);
	HWND hwnd = GetHWnd();
	SetWindowPos(hwnd, NULL, 100, 88, 0, 0, SWP_NOSIZE | SWP_NOSIZE);
	//两个参数分别是窗口的宽度和高度；

	//设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 50;
	f.lfHeight = 25;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;//抗锯齿效果
	settextstyle(&f);
	setbkmode(TRANSPARENT);//设置背景透明
	setcolor(BLACK);//设置字体颜色



	memset(zms, 0, sizeof(zms));
	for (int i = 0; i < ZM_COUNT; i++) {
		//僵尸行走
		for (int j = 0; j < 22; j++)
		{
			sprintf_s(name, sizeof(name), "res/zm/%d/%d.png", i, j + 1);
			//先判断文件是否存在，如果存在就加载图片
			if (fileExist(name))
			{
				imgZM[i][j] = new IMAGE;
				loadimage(imgZM[i][j], name);
			}
			else
			{
				break;
			}
		}
	}
	for (int i = 0; i < ZM_COUNT; i++) {
		//僵尸吃
		for (int j = 0; j < 21; j++) {
			sprintf_s(name, sizeof(name), "res/zm_eat/%d/%d.png", i, j + 1);
			if (fileExist(name)) {
				imgZMEat[i][j] = new IMAGE;
				loadimage(imgZMEat[i][j], name);
			}
			else {
				break;
			}
		}
	}


	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZmStand[i], name);
	}



	//初始化僵尸的数据
	//memset(zms, 0, sizeof(zms));
	//for (int i = 0; i < 22; i++)
	//{
	//	sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
	//	loadimage(&imgZM[i], name);
	//}
	//for (int i = 0; i < 11; i++) {
	//	sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
	//	loadimage(&imgZmStand[i], name);
	//}

	////僵尸吃植物的图片
	//for (int i = 0; i < 21; i++) {
	//	sprintf_s(name, "res/zm_eat/%d.png", i + 1);
	//	loadimage(&imgZMEat[i], name);
	//}

	loadimage(&imgBulletNormal, "res/caidoge.png");
	memset(bullets, 0, sizeof(bullets));

	//初始化豌豆子弹的图片
	loadimage(&imgBullBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++)
	{
		double k = (i + 1) * 0.2;
		loadimage(&imgBullBlast[i], "res/bullets/bullet_blast.png", imgBullBlast[3].getwidth() * k, imgBullBlast[3].getheight() * k, true);
	}
	for (int i = 0; i < 20; i++) {
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZMDead[i], name);
	}
}
//游戏窗口渲染
void drawZM()
{
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmCount; i++)
	{
		if (zms[i].used)
		{
			IMAGE* img = NULL;
			if (zms[i].dead) {
				img = imgZMDead+ zms[i].frameIndex;
			}
			else if (zms[i].eating) {
				//img = (imgZMEat[zms[i].type]);
				img = imgZMEat[zms[i].type][zms[i].frameIndex];
			}
			else {
				//img = (imgZM[zms[i].type]);
				img = imgZM[zms[i].type][zms[i].frameIndex];
			}
			
			
			putimagePNG(zms[i].x,
				zms[i].y - img->getheight(),
				img);
		}


	}
}
void drawSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++)
	{
		//if (balls[i].used || balls[i].xoff)
		if (balls[i].used)
		{
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}


	}

	char scoreText[10];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	outtextxy(283 - 115, 73, scoreText);
}

void drawCards() {
	//卡牌槽渲染
	for (int i = 0; i < ZHI_WU_COUNT; i++)
	{
		int x = 338 - 112 + 64 * i;
		putimagePNG(x, 6, &imgCards[i]);
	}
}
void drawZhiWu() {
	//地图的上种植的植物的渲染
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type > 0)
			{
				int x = 255 - 112 + j * 81;
				int y = 90 + i * 98;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				putimagePNG(map[i][j].x, map[i][j].y, imgZhiWu[zhiWuType][index]);
			}
		}
	}
	//拖动植物渲染,放在后面可以让拖动的植物显示在上面
	if (curZhiWu > 0)
	{
		IMAGE* img = imgZhiWu[curZhiWu - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, imgZhiWu[curZhiWu - 1][0]);
	}
}
void drawBullets() {
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);

	for (int i = 0; i < bulletMax; i++)
	{

		if (bullets[i].used)
		{
			if (bullets[i].blast) {
				IMAGE* img = &imgBullBlast[bullets[i].frameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
			else {
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}

		}
	}
}
void drawBars() {
	putimage(-112, 0, &imgBg);//前面两个函数是图片的位置，距离左侧x，和上面y
	putimagePNG(250 - 112, 0, &imgBar);
	putimagePNG(250 - 112 + 620, 0, &imgShovelSlot);
	putimagePNG(250 - 112 + 620, 0, &imgShovel);

	putimagePNG(250 - 112 + 620 + 65 + 3, 2, &imgGameMenuButton);
}
void drawShovel() {
	if (shovel) {
		putimagePNG(curX - 10, curY - imgShovel.getheight() * 0.7, &imgShovel);
	}
}
void updateWindow() {
	BeginBatchDraw();
	drawBars();
	drawCards();
	drawZhiWu();
	drawShovel();
	drawSunshine();
	drawZM();
	drawBullets();
	EndBatchDraw();
}

//修改了收集阳光的方式，鼠标指向阳光就能收集，不需要点击，不然阳光太多了就点不过来，自动收集会减少游戏体验
void collectSunshine(ExMessage* msg)
{
	int count = sizeof(balls) / sizeof(balls[0]);
	int w = imgSunshineBall[0].getheight();
	int h = imgSunshineBall[0].getheight();
	for (int i = 0; i < count; i++)
	{
		if (balls[i].used)
		{

			int x = balls[i].pCur.x;
			int y = balls[i].pCur.y;
			if (msg->x > x && msg->x   <x + w && msg->y   >y &&
				msg->y < y + h)
			{

				balls[i].status = SUNSHINE_COLLECT;

				PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);


				balls[i].p1 = balls[i].pCur;
				balls[i].p4 = vector2(262 - 122, 0);
				balls[i].t = 0;
				double distance = dis(balls[i].p1 - balls[i].p4);
				double off = 8;
				balls[i].speed = 1.0 / (distance / 8);


			}

		}
	}
}
void pickShovel(ExMessage* msg) {
	int x = msg->x;
	int y = msg->y;

	if (msg->message == WM_LBUTTONDOWN && shovel == false) {
		if (x > 250 - 112 + 620 - 20 && x < 250 - 112 + 620 + 65 - 20 &&
			y>0 && y < 70) {
			shovel = true;
		}
	}
	else if (shovel && msg->message == WM_LBUTTONDOWN) {
		if (msg->x >= 240 - 112 && msg->y >= 80) {
			int row = (msg->y - 76) / 98;
			int col = (msg->x - (246 - 112)) / 82;
			if (map[row][col].type == 0) {
				shovel = false;
				return;
			}
			else {
				shovel = false;
				memset(&map[row][col], 0, sizeof(map[row][col]));

			}

		}
		else {
			shovel = false;
		}
	}

}
//种植物函数，封装起来
void plant(ExMessage* msg) {
	static int status = 0;//表示没有选中植物
	if (msg->message == WM_LBUTTONDOWN && status == 0)//在没有选中植物时点击鼠标
	{
		//卡槽中选植物
		if (msg->x >= 338 - 122 && msg->x <= 338 - 122 + 64 * ZHI_WU_COUNT && msg->y < 96)//种植植物
		{

			int index = (msg->x - (338 - 112)) / 64;
			if (sunshine >= zhiWuCosts[index + 1]) {
				curZhiWu = index + 1;

				curX = msg->x;
				curY = msg->y;

				status = 1;
			}
			//如果阳光不够待优化
			else {
				char scoreText[20];
				sprintf_s(scoreText, sizeof(scoreText), "阳光不够了");
				outtextxy(338 + index * 64, 100, scoreText);
			}


		}
	}
	else if (msg->message == WM_LBUTTONDOWN && status == 1)
	{
		if (msg->x >= 240 - 112 && msg->y >= 80)//在地图上种
		{

			int row = (msg->y - 76) / 98;
			int col = (msg->x - (246 - 112)) / 82;
			if (map[row][col].type == 0)
			{
				map[row][col].type = curZhiWu;
				sunshine -= zhiWuCosts[map[row][col].type];

				map[row][col].frameIndex = 0;

				if (map[row][col].type == JIAN_GUO + 1) {//特殊植物
					map[row][col].blood = 1000;
				}
				//普通植物
				else {
					map[row][col].blood = 100;
				}

				map[row][col].shootTime = 0;

				/*int x = 255 + j * 81;
				int y = 90 + i * 98;*/
				map[row][col].x = 256 - 112 + col * 81;
				map[row][col].y = 90 + row * 98;

				curZhiWu = 0;
				status = 0;
			}
		}
		//在bar工具栏上点击，植物选错了可以退回
		else if (msg->x >= 338 - 122 && msg->x <= 1277.4 - 122 && msg->y <= 200.9)
		{
			curZhiWu = 0;
			status = 0;
		}
	}


}
//游戏进行中的菜单
void gameMenu(ExMessage* msg) {
	int x = msg->x;
	int y = msg->y;

	if (msg->message == WM_LBUTTONDOWN && x >= 250 - 112 + 620 + 65 + 5 && x <= 250 - 112 + 620 + 65 + 133 && y < 42) {
		while (1) {
			BeginBatchDraw();
			putimagePNG(442.5 - 170, 49.4, &imgGameMenu);
			EndBatchDraw();
			peekmessage(msg);

			if (msg->message == WM_LBUTTONDOWN && msg->x >= 324.6
				&& msg->y >= 447.2
				&& msg->x <= 655.7
				&& msg->y <= 522.9) {
				return;
			}
		}
	}
}
//游戏的鼠标操作
void userClick()
{
	static int status = 0;//表示没有选中植物
	ExMessage msg;//这是easyx自带的结构体类型，在头文件里面定义了
	if (peekmessage(&msg))
	{
		if (msg.message == WM_MOUSEMOVE)
		{
			curX = msg.x;
			curY = msg.y;//这里不要渲染，不要混淆函数的功能
		}
		plant(&msg);
		//收集阳光
		collectSunshine(&msg);

		pickShovel(&msg);
		gameMenu(&msg);


	}
}


void creatSunshine()
{
	static int count = 0;
	static int fre = 200;
	count++;

	if (count >= fre)
	{
		fre = 100 + rand() % 200;
		count = 0;
		//从阳光池中找可以使用的阳光
		int ballMax = sizeof(balls) / sizeof(balls[0]);
		int i = 0;
		for (; i < ballMax && balls[i].used; i++);
		if (i >= ballMax)return;

		balls[i].used = true;
		balls[i].frameIndex = 0;

		//balls[i].y = 60;
		//balls[i].x = 260 + rand() % (900 - 260);//阳光x范围
		//balls[i].destY = 200 + (rand() % 4) * 90;
		balls[i].timer = 0;

		//balls[i].xoff = 0;
		//balls[i].yoff = 0;*/
		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(260 - 112 + rand() % (900 - (260 - 112)), 60);
		balls[i].p4 = vector2(balls[i].p1.x - 112, 200 + (rand() % 4) * 90);
		int off = 2;
		double distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.0 / (distance / off);


	}

	int ballMax = sizeof(balls) / sizeof(balls[0]);
	//向日葵生产阳光
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == XIANG_RI_KUI + 1) {
				map[i][j].timer++;
				if (map[i][j].timer > 200) {
					map[i][j].timer = 0;

					int k = 0;
					for (; k < ballMax && balls[k].used; k++);
					if (k >= ballMax) return;

					balls[k].used = true;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (100 + rand() % 50) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w,
						map[i][j].y + imgZhiWu[XIANG_RI_KUI][0]->getheight() -
						imgSunshineBall[0].getheight());
					balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
					balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 100);
					balls[k].status = SUNSHINE_PRODUCT;
					balls[k].speed = 0.05;
					balls[k].t = 0;
				}
			}
		}

	}
}


void updateSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN) {
				struct sunshineBall* sun = &balls[i];//用指针指向它，写起来简单。后面就不用一直写【i】
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);//p1+t*(p4-p1);
				if (sun->t >= 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer > 100) {
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t > 1) {
					sun->used = false;
					sunshine += 25;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
				if (sun->t > 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}

		}

	}
}

void creatZM()
{
	if (zmCount >= ZM_MAX) {
		return;
	}

	static int zmFre = 200+100;
	static int count = 0;

	
	count++;
	if (count > zmFre) {
		count = 0;
		
		
		zmFre = 250 - rand() % 20-zmCount;

		int i = 0;
		int zmMax = sizeof(zms) / sizeof(zms[0]);
		for (; i < zmMax && zms[i].used; i++);//找一个还没有用过的僵尸
		if (i < zmMax) {
			memset(&zms[i], 0, sizeof(zms[i]));

			zms[i].used = true;
			zms[i].x = WIN_WIDTH;

			zms[i].row = rand() % 5;
			zms[i].y = 73.5 + 99.06 * (zms[i].row + 1);//174.7


			zms[i].speed = 2 + rand() % 2+ zmCount/5;

			if (zmCount <=2) {
				zms[i].type = PU_TONG ;
			}
			else if (zmCount <= 8) {
				zms[i].type = rand() % (ZM_COUNT - 1);
			}
			else{
				zms[i].type = rand() % (ZM_COUNT-1)+1;
			}

			switch (zms[i].type) {
			case PU_TONG :zms[i].blood = 100; break;
			case MAO_ZI :zms[i].blood = 200; break;
			case TIE_TONG :zms[i].blood = 300; break;
			default: cerr << "bug zmtype"; exit(-1);
			}
			//zms[i].blood = 100;
			zms[i].dead = false;
			zmCount++;
		}
	}

}

void updateZM()
{
	//更新僵尸的位置
	int zmMax = sizeof(zms) / sizeof(zms[0]);

	static int count = 0;
	count++;
	if (count > 3)
	{
		count = 0;
		for (int i = 0; i < zmMax; i++)
		{
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;
				if (zms[i].x <= 0)
				{
					//printf("GAME OVER\n");
					//MessageBox(NULL, "your brain has been eaten by zoombie", "game over", 0);
					//exit(0);//游戏结束
					gameStatus = FAIL;
				}
			}

		}
	}


	//更新僵尸的图片帧
	static int count2 = 0;
	count2++;
	if (count2 > 1)
	{
		count2 = 0;
		for (int i = 0; i < zmMax; i++)
		{
			int zmType = zms[i].type;

			if (zms[i].used) {
				if (zms[i].dead) {
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 20) {
						zms[i].used = false;
						killCount++;
						if (killCount >= ZM_MAX) {
							gameStatus = WIN;
						}
					}
				}
				else if (zms[i].eating) {
					zms[i].frameIndex++;
					if (imgZMEat[zmType][zms[i].frameIndex] == NULL) {
						zms[i].frameIndex = 0;
					}
				}
				else {
					zms[i].frameIndex++;
					if (imgZM[zmType][zms[i].frameIndex] == NULL) {
						zms[i].frameIndex = 0;
					}
				}

			}
		}
	}

}

void shoot()
{
	//判断这一行有没有僵尸
	static int count = 0;
	if (++count < 3)return;
	count = 0;
	int ZM_in_lines[5] = { 0,0,0,0,0 };


	int zmCount = sizeof(zms) / sizeof(zms[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);


	int dangerX = WIN_WIDTH - (*imgZM[0][0]).getwidth() / 2;
	for (int i = 0; i < zmCount; i++)
	{
		if (zms[i].used && zms[i].x < dangerX)
		{
			ZM_in_lines[zms[i].row] = 1;
		}
	}


	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == WAN_DOU + 1 && ZM_in_lines[i]) {

				map[i][j].shootTime++;
				if (map[i][j].shootTime > 20) {
					map[i][j].shootTime = 0;
					int k = 0;
					for (; k < bulletMax && bullets[k].used; k++);
					if (k < bulletMax) {
						bullets[k].used = true;
						bullets[k].row = i;
						bullets[k].speed = 6;

						bullets[k].blast = false;
						bullets[k].frameIndex = 0;

						int zwX = 255 - 112 + j * 81;
						int zwY = 90 + i * 98;

						bullets[k].x = zwX + imgZhiWu[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = zwY + 5;

					}
				}

			}

		}
	}

}
void checkBullet2ZM()
{
	int bCount = sizeof(bullets) / sizeof(bullets[0]);
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < bCount; i++) {
		if (bullets[i].used == false || bullets[i].blast)continue;

		for (int k = 0; k < zCount; k++) {
			if (zms[k].used == false) {
				continue;
			}
			int x1 = zms[k].x + 80;
			int x2 = zms[k].x + 110;
			int x = bullets[i].x;

			if (bullets[i].row == zms[k].row && x > x1 && x < x2 && zms[k].dead == false) {
				zms[k].blood -= 20;
				bullets[i].blast = true;
				bullets[i].speed = 0;

				if (zms[k].blood <= 0) {
					zms[k].dead = true;
					zms[k].speed = 0;
					zms[k].frameIndex = 0;

				}
				break;

			}
		}
	}
}
void checkZM2ZhiWu() {
	int Zcount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < Zcount; i++) {
		if (zms[i].dead)continue;
		int row = zms[i].row;
		for (int k = 0; k < 9; k++) {
			if (map[row][k].type == 0 && !zms[i].eating)continue;
			int zhiWuX = 255 - 112 + k * 81;
			int x1 = zhiWuX + 10;
			int x2 = zhiWuX + 60;
			int x3 = zms[i].x + 80;
			if (x3 > x1 && x3 < x2) {

				//map[row][k].catched ||
				if (zms[i].eating) {

					map[row][k].blood--;

					if (map[row][k].blood <= 0 || map[row][k].type == 0) {
						map[row][k].type = 0;
						zms[i].eating = false;
						zms[i].frameIndex = 0;
						zms[i].speed = 2 + rand() % 2;
					}
				}
				else {

					zms[i].eating = true;
					zms[i].speed = 0;
					zms[i].frameIndex = 0;
				}
			}
			/*
			int x = 255 + j * 81;
			int y = 90 + i * 98;*/

		}
	}
}
void collisionCheck() {
	checkBullet2ZM();//检查植物子弹打僵尸
	checkZM2ZhiWu();
}
void updateBullets()
{
	int countMax = sizeof(bullets) / sizeof(bullets[0]);

	for (int i = 0; i < countMax; i++)
	{
		if (bullets[i].used)
		{
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > WIN_WIDTH)
			{
				bullets[i].used = false;
			}
			if (bullets[i].blast)
			{
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex >= 4)
				{
					bullets[i].used = false;
				}
			}
		}
	}
}
void updateZhiWu() {
	static int count = 0;
	if (++count < 2)return;
	count = 0;
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{

			if (map[i][j].type > 0)//如果这里有植物
			{

				map[i][j].frameIndex++;

				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;

				//如果植物的图片动画播放完毕返回第一张
				if (imgZhiWu[zhiWuType][index] == NULL)
				{
					map[i][j].frameIndex = 0;
				}
			}
		}
	}
}
void updateGame()//更新游戏数据，如种植物，生产阳光
{
	//遍历整个地图
	updateZhiWu();


	creatSunshine();//创建阳光
	updateSunshine();//更新阳光状态

	creatZM();//创建僵尸
	updateZM();//更新僵尸的状态

	shoot();
	updateBullets();

	collisionCheck();//检查豌豆子弹和僵尸的碰撞
}
void thanksUI() {
	IMAGE	imgthanks;
	loadimage(&imgthanks, "res/thanks1.png");
	ExMessage msg;
	while (1) {
		putimagePNG(0, 0, &imgthanks);

		peekmessage(&msg);
		if (msg.message == WM_LBUTTONUP) {
			return;
		}
	}

}

int startUI(){

	IMAGE imgBg, imgMenu1, imgMenu2, imgAchievement1, imgAchievement2;
	loadimage(&imgBg, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");
	loadimage(&imgAchievement1, "res/Achievements_pedestal.png");
	loadimage(&imgAchievement2, "res/Achievements_pedestal_press.png");


	int flag = 0;
	ExMessage msg;


	while (1)
	{
		if (peekmessage(&msg))
		{
			curX = msg.x;
			curY = msg.y;

			if (curX >= 481.5 && curX <= 812.5 && curY >= 82.5 && curY <= 227.5) {
				flag = 1;
			}
			else if (curX >= 382 && curY >= 410 && curX <= 514.5 && curY <= 575.5) {
				flag = 2;
			}
			else {
				flag = 0;
			}

			if (msg.message == WM_LBUTTONUP)
			{
				if (flag == 1) {
					return 1;
				}
				if (flag == 2) {
					thanksUI();
				}

			}
		}


		BeginBatchDraw();
		putimage(0, 0, &imgBg);
		putimagePNG(481.5, 82.5, flag == 1 ? &imgMenu2 : &imgMenu1);
		putimagePNG(382, 410, flag == 2 ? &imgAchievement2 : &imgAchievement1);
		EndBatchDraw();
	}


}
void viewScence() {
	int xMin = WIN_WIDTH - imgBg.getwidth();//900-1400=-500
	vector2 points[9] = {
		{550,80},{530,160},{630,170},{530,200},{515,270},
		{565,370},{605,340},{705,280},{690,340}
	};
	int index[9];//对僵尸站立图片的索引
	for (int i = 0; i < 9; i++) {
		index[i] = rand() % 11;
	}

	int count = 0;
	for (int x = 0; x >= xMin; x -= 4) {
		BeginBatchDraw();
		putimage(x, 0, &imgBg);

		count++;
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x - xMin + x,
				points[k].y,
				&imgZmStand[index[k]]);
			if (count >= 10) {
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10)count = 0;
		EndBatchDraw();
		Sleep(5);
	}

	//停留1s左右
	for (int i = 0; i < 20; i++) {
		BeginBatchDraw();

		putimage(xMin, 0, &imgBg);
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x, points[k].y, &imgZmStand[index[k]]);
			index[k] = (index[k] + 1) % 11;
		}
		EndBatchDraw();
		Sleep(30);
	}
	//切换回来
	for (int x = xMin; x <= -112; x += 4) {
		BeginBatchDraw();

		putimage(x, 0, &imgBg);

		count++;
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x + x - xMin, points[k].y, &imgZmStand[index[k]]);
			if (count >= 10) {
				index[k] = (index[k] + 1) % 11;
			}

		}
		if (count >= 10)count = 0;

		EndBatchDraw();
		Sleep(5);
	}
}
void barsDown() {
	int height = imgBar.getheight();
	for (int y = -height; y <= 0; y++) {
		BeginBatchDraw();
		putimagePNG(-112, 0, &imgBg);
		putimagePNG(250 - 112, y, &imgBar);
		putimagePNG(250 - 112 + 620, y, &imgShovelSlot);
		putimagePNG(250 - 112 + 620 - 20, y, &imgShovel);
		for (int i = 0; i < ZHI_WU_COUNT; i++) {
			int x = 338 - 112 + 64 * i;
			putimagePNG(x, y + 6, &imgCards[i]);

		}
		EndBatchDraw();
	}
}
bool checkOver() {
	int ret = false;
	if (gameStatus == WIN) {
		Sleep(2000);
		loadimage(0, "res/win2.png");
		mciSendString("res/win.mp3", 0, 0, 0);

		ret = true;
	}
	else if (gameStatus == FAIL) {
		loadimage(0, "res/fail2.png");
		mciSendString("res/lose.mp3", 0, 0, 0);
		ret = true;
	}
	return ret;
}
int main()
{
	gameInit();
	startUI();
	//int timer = 0;//这个是计数器
	//bool flag = true;
	viewScence();
	barsDown();
	int timer = 0;

	bool flag = true;
	while (1)
	{
		//限制卡牌更新速度

		userClick();
		timer += getDelay();
		if (timer > 15)
		{
			flag = true;
			timer = 0;
		}
		if (flag)
		{
			flag = false;
			updateGame();
			updateWindow();
			if (checkOver())break;
		}

	}
	system("pause");
	return 0;

}
