//如果代码出现了BUG，请检查：
//1.是否把双等号写成了单等号，或者反过来
//2.是否把k写成i,j或者反过来
//3.是否写了函数但是没有调用
//4.是否是visual studio自己的问题，它有可能欠重装了
//5.是否把int类型的变量写成了bool
//6.实在不行就在你认为错误的地方打几个断点，求求了，打断点没有那么难的。
//已解决问题：
//1.点击卡牌栏拖动植物的过程中，如果鼠标移动轨迹刚好经过阳光，会同时触发点击阳光的效果，而原版并不是这样的。（可解决，但我没有，每次检测点击的时候用break隔断即可）
//2.现在会有两只僵尸吃同一个植物，植物被吃完之后只有一个僵尸恢复行走，另一只仍然维持正在吃的状态（已解决，每一帧都要检查一下这只僵尸所在的区块是否存在植物）
//3.现在把阳光球抽搐的问题解决了
//4.解决了播放音频时会卡顿的问题
//5.解决了向日葵生产的阳光乱飞的问题，虽然不知道最后是怎么解决了，但就是变正常了
//6.解决了如果把向日葵生产阳光的速度调高，就会发现如果向日葵在同一个位置落下两个或以上阳光，其中一个阳光会静止不动，也没有办法交互，不知道是哪里的问题
//未解决：
//我发现向日葵产生的阳光好像不会随时间消失了，解决了，但是向日葵产生的阳光在消失的前几秒会卡顿
//目前我们所有的x坐标都是偏小的，这个更改起来实在是太麻烦了，有空的时候会解决的
#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<ctime>
#include<graphics.h>
#include<mmsystem.h>
#include<cmath>
#include<math.h>
#include"tools.h"
#include"vector2.h"
#pragma comment(lib,"winmm.lib")

using namespace std;

#define img_width 900
#define img_height 600
#define pool_size 10//阳光池大小

enum {wan_dou, xiang_ri_kui, plant_count };//xiang_ri_kui=0；
enum { going, win, fail };
IMAGE img_bg;
IMAGE img_tools_bar;
IMAGE img_plant_card[plant_count];
IMAGE* plant_gif[plant_count][20];//这个数组是为了保存植物的一系列动图
IMAGE* sunshine_gif[30];//保存阳光动图
IMAGE zombie_gif[22];
IMAGE bullet_normal;//子弹只有完整和破碎两种状态
IMAGE bullet_blast_gif[4];
IMAGE zombie_dead_gif[20];
IMAGE zombie_eat_gif[21];
IMAGE zombie_stand_gif[11];
IMAGE game_success;
IMAGE game_fail;
int game_state;//记录游戏的状态
int kill_count;//记录击杀僵尸的数量
int max_count;//记录需要击杀的僵尸数目，杀完了这一关就通关了
int plant_kind = 0;//全局变量，用来记录鼠标选中的植物种类，为0就是啥也没有
int cur_x=-100;//用来记录鼠标的实时位置
int cur_y=-100;
typedef struct sunshine {
	int value=50;
	int width;//用于对齐
}sunshine;
sunshine sunshine_own;

typedef struct node_1 {//种植格子
	int plant_type;//0说明没有植物
	int frame_num;//帧序号
	int health;
}node_1;
node_1 plant_block[3][9];//三行九列的种植地块

typedef struct node_2 {//阳光球
	int x;
	int y;
	int dest_y;
	int frame_num;
	bool used;
	bool if_click;
	int life_time;
	float dx;//被点中之后向目标点移动时，dx/dy即为轨迹斜率，dx,dy是单位时间阳光球坐标偏移量
	float dy;
	bool if_from_sunflower;//是否是来自向日葵
	vector2 p1,p2,p3,p4;//贝塞尔曲线的四个关键点
	vector2 pur;//是一个结构体，包含了当前的x,y坐标，定义重复了，但是修改起来过于麻烦，所以只在阳光球从向日葵上掉落的时候使用这个结构体
	float t;//贝塞尔曲线参数
	bool if_ground;//是否落地
}node_2;
node_2 sunshine_ball_pool[10];//阳光池

typedef struct node_3 {//僵尸
	int x, y;
	int row;//记录僵尸的行数
	int frame_num;
	bool used;
	int speed;
	int health;
	bool if_dead;
	int dead_frame_num;
	bool if_eat;
	int eat_frame_num;//wrnmd!这里刚开始的时候设置成bool了，=（
}node_3;
node_3 zombie_pool[10];//僵尸池

typedef struct node_4 {//子弹
	int x, y;
	int frame_num;//爆炸帧序号
	bool if_blast;//是否爆炸
	int row;
	int speed;
	bool used;
}node4;
node4 bullet_pool[30];//子弹池

typedef struct node_5 {//站场僵尸
	int x, y;
	int frame_num;
	int speed;
}node_5;
node_5 stand_zombie[9];//站场僵尸池，只需要负责在片头巡视场景的时候抽抽，流哈喇子

//判断文件是否存在
bool file_exist(const char* name) {
	FILE* fp = fopen(name, "r");
	if (fp==NULL) {//如果没有这个文件
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}
int self_rand(int x, int y) {//传入两个参数，返回一个这之间的随机数,y>x
	int res = rand() % (y - x + 1) + x;
	return res;
}

void creat_sunshine_ball() {//自然创建阳光球
	static int timer_1 = 0;
	int clock_1 = self_rand(100, 150);//单位是ms
	/*timer_1 += getDelay();//奇怪，这里用这个老师自定义的函数为什么不行*/
	timer_1++;
	if (timer_1 > clock_1) {//每3-4秒生成一个阳光
		for (int i = 0; i < pool_size; i++) {
			if (!sunshine_ball_pool[i].used && sunshine_ball_pool[i].if_click == false) {
				sunshine_ball_pool[i].used = true;
				sunshine_ball_pool[i].x = self_rand(253, 700);
				sunshine_ball_pool[i].y = 0;
				sunshine_ball_pool[i].dest_y = self_rand(175, 415);
				sunshine_ball_pool[i].life_time = 0;
				sunshine_ball_pool[i].dx = 0;
				sunshine_ball_pool[i].dy = 0;
				sunshine_ball_pool[i].if_click = false;
				sunshine_ball_pool[i].if_from_sunflower = false;
				sunshine_ball_pool[i].if_ground = false;
				timer_1 = 0;
				break;//每轮循环只创造一个阳光即可
			}
		}
	}
};
//向日葵生成阳光，
void creat_sunshine_by_sunflower() {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (plant_block[i][j].plant_type == xiang_ri_kui + 1) {//这个地块种的是向日葵
				int sunshine_max = sizeof(sunshine_ball_pool) / sizeof(sunshine_ball_pool[0]);
				static int timer = 0;
				timer++;
				if (timer > 300) {//调整这里可以改变向日葵生成阳光的速度
					//待会补充一个计时器，每个向日葵都必须要100帧才能生产一个阳光
					for (int k = 0; k < sunshine_max; k++) {
						if (!sunshine_ball_pool[k].used&& sunshine_ball_pool[k].if_click==false) {//这个阳光球没有被使用，且没有被点击，因为被点击的阳光球是false
							timer = 0;
							sunshine_ball_pool[k].used = true;
							sunshine_ball_pool[k].x = 225 + 82 * (j - 1)+50;//如果用曲线的话，这个点可以当作p2/p3
							sunshine_ball_pool[k].y = 178 + 102 * (i - 1)+40;
							sunshine_ball_pool[k].dest_y = 178 + 102 * (i - 1)+102;//可以当p4
							sunshine_ball_pool[k].if_click == false;
							sunshine_ball_pool[k].life_time = 0;
							sunshine_ball_pool[k].dx = 0;
							sunshine_ball_pool[k].dy = 4;
							sunshine_ball_pool[k].if_from_sunflower = true;
							sunshine_ball_pool[k].t = 0;//做测试
							//初始化贝塞尔曲线，p4终点，p1起点
							sunshine_ball_pool[k].p2 = vector2(225 + 82 * (j - 1) + 80, 178 + 102 * (i - 1) + 60);
							sunshine_ball_pool[k].p3 = vector2(225 + 82 * (j - 1) + 40, 178 + 102 * (i - 1) + 100);
							sunshine_ball_pool[k].p1 = vector2(225 + 82 * (j - 1) + 100, 178 + 102 * (i - 1) + 90);
							sunshine_ball_pool[k].p4 = vector2(225 + 82 * (j - 1)+40, 178 + 102 * (i - 1) + 102);
							sunshine_ball_pool[k].if_ground = false;//没有落地
							
							break;//一次只产一个
						}
					}
				}
			}
		}
	}
}
//隐约察觉到这里的逻辑有一丝丝的问题
void update_sunshine_ball() {//阳光球的逻辑改动了，直到阳光球回到终点，状态才会变成false
	for (int i = 0; i < pool_size; i++) {
		//更新被点中的阳光球的状态
		if (sunshine_ball_pool[i].if_click) {
			//
			////为了提高轨迹精度
			float angle = atan2(sunshine_ball_pool[i].y ,(sunshine_ball_pool[i].x - 271));
			sunshine_ball_pool[i].dx = 20 * cos(angle);//调整这里可以改变被点击的阳光飞跃的速度
			sunshine_ball_pool[i].dy = 20 * sin(angle);

			sunshine_ball_pool[i].x -= sunshine_ball_pool[i].dx;
			sunshine_ball_pool[i].y -= sunshine_ball_pool[i].dy;
			if (sunshine_ball_pool[i].x < 290 && sunshine_ball_pool[i].x>252&& sunshine_ball_pool[i].y <= 0) {//到达终点之后，//这里一定要是y<=0呀！不然的话阳光会抽搐
				sunshine_own.value += 25;//再加上阳光值
				sunshine_ball_pool[i].dx = 0;
				sunshine_ball_pool[i].dy = 0;
				sunshine_ball_pool[i].if_click = false;
				sunshine_ball_pool[i].used = false;//

			}
		}
		//这里有重复定义，有空再优化
		else if (sunshine_ball_pool[i].used) {//阳光正在使用，但没有被点击
			sunshine_ball_pool[i].frame_num = (sunshine_ball_pool[i].frame_num + 1) % 29;//更新帧动画
			
			if (sunshine_ball_pool[i].if_from_sunflower == false && sunshine_ball_pool[i].if_ground == false) {//如果阳光球来自自然生成，，，这里把判断写成赋值了，西八！
				if (sunshine_ball_pool[i].life_time == 0) {//还没有落到地面
					sunshine_ball_pool[i].y += 5;
				}
				if (sunshine_ball_pool[i].y >= sunshine_ball_pool[i].dest_y) {//落到地面且过了100帧
					sunshine_ball_pool[i].life_time++;
					if (sunshine_ball_pool[i].life_time > 100) {
						sunshine_ball_pool[i].used = false;
						sunshine_ball_pool[i].if_click = false;
					}
				}
			}
			else if(sunshine_ball_pool[i].if_from_sunflower == true&&sunshine_ball_pool[i].if_ground == false){//如果阳光球来自向日葵,且还没有落地
				//sunshine_ball_pool[i].t = 0.1;
				if (sunshine_ball_pool[i].t >= 1) {
					sunshine_ball_pool[i].if_ground = true;//t=1，落地了
					sunshine_ball_pool[i].life_time = 0;
				}
				else {
					vector2 tmp = calcBezierPoint(sunshine_ball_pool[i].t, sunshine_ball_pool[i].p1, sunshine_ball_pool[i].p2, sunshine_ball_pool[i].p3, sunshine_ball_pool[i].p4);
					sunshine_ball_pool[i].x = tmp.x;
					sunshine_ball_pool[i].y = tmp.y;
					sunshine_ball_pool[i].t += 0.1;
				}
				
			}
			else if (sunshine_ball_pool[i].if_ground == true) {//如果阳光球已经落地
				sunshine_ball_pool[i].life_time++;

				if (sunshine_ball_pool[i].life_time > 100) {
					sunshine_ball_pool[i].used = false;
					sunshine_ball_pool[i].if_click = false;
				}
			}
		}
	}
};


//将图片放入内存，方便待会加载
void gameinit() {
	//设置击杀僵尸目标数
	max_count = 10;
	kill_count = 0;
	game_state = going;
	//修改字符集为多字节字符集,没用要加L
	loadimage(&img_bg,"res/bg.jpg");//相当于是把图片和变量名联系起来
	loadimage(&img_tools_bar, "res/bar5.png");
	//加载子弹图片
	loadimage(&bullet_normal, "res/bullets/bullet_normal.png");
	loadimage(&bullet_blast_gif[3], "res/bullets/bullet_blast.png");
	loadimage(&game_success, "res/gameWin.png");
	loadimage(&game_fail, "res/gameFail.png");
	//因为只有一张子弹爆炸的图片，所以我们将图片缩小，放到帧序列中，实现爆炸动画
	for (int i = 0; i < 3; i++) {
		int width = bullet_blast_gif[3].getwidth();
		int height = bullet_blast_gif[3].getheight();
		float k = 0.25 * (i + 1);
		loadimage(&bullet_blast_gif[i], "res/bullets/bullet_blast.png", width*k, height*k,true);
	}
	
	memset(sunshine_ball_pool, 0, sizeof(sunshine_ball_pool));
	memset(plant_block, 0, sizeof(plant_block));
	memset(sunshine_gif, 0, sizeof(sunshine_gif));
	memset(plant_gif, 0, sizeof(plant_gif));//初始化plant_gif内空间为0，这样一来遇到0就知道结束了
	memset(zombie_pool, 0, sizeof(zombie_pool));
	memset(bullet_pool, 0, sizeof(bullet_pool));
	memset(stand_zombie, 0, sizeof(stand_zombie));
	//memset(zombie_gif, 0, sizeof(zombie_gif));这里如果初始化，下面的load就会访问越界，所以不要给image类型的数组初始化


	char name[64];
	//加载植物动图和植物卡片
	for (int i = 0; i < plant_count; i++) {
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);//name中就是这个式子生成的路径字符串
		loadimage(&img_plant_card[i], name);
		for (int j = 0; j < 20; j++) {
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
			if (file_exist(name)) {
				plant_gif[i][j] = new IMAGE;//给那些连续图片生成内存空间
				loadimage(plant_gif[i][j], name);
			}
		}
	}
	//加载阳光动图
	for (int i = 0; i < 29; i++) {
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		if (file_exist(name)) {
			sunshine_gif[i] = new IMAGE;
			loadimage(sunshine_gif[i], name);
		}
	}
	//加载僵尸动图
	for (int i = 0; i < 22; i++) {
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&zombie_gif[i], name);
	}
	//加载僵尸死亡动图
	for (int i = 0; i < 20; i++) {
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&zombie_dead_gif[i], name);
	}
	//加载僵尸吃植物的动图
	for (int i = 0; i < 21; i++) {
		sprintf_s(name, sizeof(name), "res/zm_eat/%d.png", i + 1);
		loadimage(&zombie_eat_gif[i], name);
	}
	//加载站场僵尸动图
	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
		loadimage(&zombie_stand_gif[i], name);
	}
	//初始化窗口，如果第三个参数为1，则图形窗口和控制台窗口会同时存在
	initgraph(img_width, img_height);
	//加载并修改字体
	LOGFONT f;//字体结构体
	gettextstyle(&f);
	f.lfHeight = 25;
	f.lfWeight = 10;
	sunshine_own.width = 30;
	strcpy(f.lfFaceName, "Segoe UI BLACK");
	f.lfQuality=ANTIALIASED_QUALITY;//抗锯齿
	settextstyle(&f);//将当前的文本样式设置为f所描述的样式
	setbkmode(TRANSPARENT);
	setcolor(BLACK);
}

void creat_zombie() {//每个类都要有创建，改变数据，渲染
	static int timer=0;
	int rander = self_rand(300, 420);
	timer++;
	if (timer > rander) {
		//int pool_max = sizeof(zombie_pool) / sizeof(zombie_pool[0]);
		int pool_max = 10;
		for (int i = 0; i < pool_max; i++) {
			if (!zombie_pool[i].used) {
				zombie_pool[i].used = true;
				zombie_pool[i].x = img_width;
				zombie_pool[i].row = self_rand(0, 2);
				zombie_pool[i].y = 135 + zombie_pool[i].row * 98;//这种随机数因为太过于随即所以会导致僵尸分布不均匀
				zombie_pool[i].speed = 1;
				zombie_pool[i].frame_num = 0;
				zombie_pool[i].health = 100;
				zombie_pool[i].dead_frame_num = 0;
				zombie_pool[i].if_eat = false;//一开始不吃东西
				zombie_pool[i].eat_frame_num = 7;
				timer = 0;
			    break;//一次循环只需要创建一只僵尸就可以
			}
			//timer = 0;
			//break;//一次循环只需要创建一只僵尸就可以
			//上面两行放错地方了，导致每次循环都会把timer归0，应当放到if循环里
		}
	}
};

void update_zombie() {
	int pool_max = sizeof(zombie_pool) / sizeof(zombie_pool[0]);
	static int timer = 0;
	timer++;
	int clock = 2;
	if (timer >= clock) {//每两帧才会更新一次僵尸的数据，调整这里可以改变僵尸动作的快慢
		for (int i = 0; i < pool_max; i++) {
			if (zombie_pool[i].if_dead) {//僵尸是否已经死亡，重申一下，我的定义里，僵尸在彻底消失之前都是被使用着的
				zombie_pool[i].dead_frame_num++;
				if (zombie_pool[i].dead_frame_num > 19) {
					zombie_pool[i].used = false;
					zombie_pool[i].if_dead = false;
				}
			}
			else if (zombie_pool[i].used) {//僵尸还在使用
				//如果僵尸没死，而且在吃东西
				if (zombie_pool[i].if_eat) {
					zombie_pool[i].eat_frame_num = (zombie_pool[i].eat_frame_num + 1) % 21;
					timer = 0;
				}
				else {//如果僵尸正常走动
					//移动并改变帧数
					zombie_pool[i].x -= zombie_pool[i].speed;
					zombie_pool[i].frame_num = (zombie_pool[i].frame_num + 1) % 22;
					timer = 0;
					if (zombie_pool[i].x < 192) {
						/*printf("game over\n");
						MessageBox(NULL, "over", "over", 0);
						exit(0);*/
						game_state = fail;
					}
				}
			}
		}
	}
};
void creat_bullet() {
	//首先遍历僵尸池，根据活着的僵尸所在的行数初始化if_zombie数组，这个数组记录了每一行是否有僵尸
	int if_zombie[3] = { 0 };
	int pool_max = sizeof(zombie_pool) / sizeof(zombie_pool[0]);
	for (int i = 0; i < pool_max; i++) {
		if (zombie_pool[i].used&& zombie_pool[i].x<850) {
			if_zombie[zombie_pool[i].row] = 1;//记录一下哪一行有僵尸
		}
	}
	//遍历种植地块
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (plant_block[i][j].plant_type == wan_dou+1) {//plant_type的0是没有植物
				if (if_zombie[i]) {//如果这一行有僵尸且这个植物是豌豆
					int bullet_max = sizeof(bullet_pool) / sizeof(bullet_pool[0]);
					//开始真正的创建子弹,二十帧创建一次
					static int timer = 0;//我去，这里原来是27个不同的timer
					timer++;
					if (timer > 40) {//疑问：static其实只在内层的for循环中起作用吧，导致同一行中的所有列的豌豆其实共用了一个timer，因此豌豆的个数不会增加单位时间内子弹的数量
						timer = 0;
						for (int k = 0; k < bullet_max; k++) {
							if (!bullet_pool[k].used) {//如果没有被使用
								bullet_pool[k].used = true;
								bullet_pool[k].row = i;
								bullet_pool[k].speed = 8;
								bullet_pool[k].if_blast = false;
								bullet_pool[k].frame_num = 0;
								//253 + 82 * j, 177 + 102 * i,这个是豌豆射手的坐标，而豌豆的实际坐标需要微调
								bullet_pool[k].x = 253 + 82 * j + 50;
								bullet_pool[k].y = 177 + 102 * i + 5;
								break;//一次只用找到一个未使用的子弹即可
							}
						}
					}
				}
			}
		}
	}
	
}
void update_bullet() {//?
	int bullet_max = sizeof(bullet_pool) / sizeof(bullet_pool[0]);
	for (int i = 0; i < bullet_max; i++) {
		if (bullet_pool[i].used) {
			bullet_pool[i].x += bullet_pool[i].speed;
			if (bullet_pool[i].x > img_width) {
				bullet_pool[i].used = false;
			}
			//如果爆炸
			if (bullet_pool[i].if_blast) {
				bullet_pool[i].frame_num += 1;//只有四帧，并不能循环播放
				if (bullet_pool[i].frame_num > 3) {
					bullet_pool[i].used = false;//?
					bullet_pool[i].if_blast = false;//这里不停止的话会访问越界，一定要记得设置blast为false!!
				}
			}
		}
	}
}

void collision_check() {//碰撞检测
	int zombie_max = sizeof(zombie_pool) / sizeof(zombie_pool[0]);
	int bullet_max = sizeof(bullet_pool) / sizeof(bullet_pool[0]);
	for (int j = 0; j < bullet_max; j++) {
		if (bullet_pool[j].used&& bullet_pool[j].if_blast==false) {
			for (int i = 0; i < zombie_max; i++) {
				//如果僵尸没有死亡，且僵尸和豌豆行号相同，则可能碰撞
				if (zombie_pool[i].if_dead==false && zombie_pool[i].used && zombie_pool[i].row == bullet_pool[j].row) {
					if (bullet_pool[j].x >= zombie_pool[i].x+60) {
						bullet_pool[j].if_blast = true;
						zombie_pool[i].health -= 10;
						bullet_pool[j].speed = 0;
						if (zombie_pool[i].health <= 0) {//僵尸健康值归0
							zombie_pool[i].if_dead = true;
							//击杀数增加
							kill_count++;
							if (kill_count > max_count) {
								game_state = win;
							}
						}
					}
				}
			}
		}
	}
}
//检测僵尸是否在吃植物，以及植物是否被吃光
void eat_check() {
	int zm_max = sizeof(zombie_pool) / sizeof(zombie_pool[0]);
	for (int k = 0; k < zm_max; k++) {
		if (zombie_pool[k].used && zombie_pool[k].if_dead == false) {//如果僵尸正在使用且没有死亡
			int row = zombie_pool[k].row;//僵尸所在的行数
			if (zombie_pool[k].if_eat&&plant_block[row][(zombie_pool[k].y - 255) / 82].plant_type == 0) {//如果僵尸所在的这个区块上并没有植物
				zombie_pool[k].if_eat = false;//解决了问题2
			}
			for (int j = 0; j < 9; j++) {//只需要判断一下同一行的植物就可以了
				int plant_x = 253 + 82 * j;
				if (plant_block[row][j].plant_type >0 && zombie_pool[k].x <= plant_x-15) {//当僵尸碰到植物的时候，改变僵尸的状态
					zombie_pool[k].if_eat = true;
					zombie_pool[k].speed = 0;//吃东西的时候停下俩
					/*这里不能每一帧都吃，这样子吃的太快了*/
					static int timer = 0;
					timer++;
					if (timer > 15) {//二十帧吃一次
						timer = 0;
						plant_block[row][j].health -= 10;
						if (plant_block[row][j].health <= 0) {//吃光植物
							plant_block[row][j].plant_type = 0;
							zombie_pool[k].if_eat = false;
							zombie_pool[k].speed = 1;
						}
					}
				}
			}
		}
	}
}

void update_game() {//更新游戏数据
	//更新植物帧序号
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (plant_block[i][j].plant_type > 0) {
				plant_block[i][j].frame_num++;
				if (plant_gif[plant_block[i][j].plant_type - 1][plant_block[i][j].frame_num] == NULL) {
					plant_block[i][j].frame_num = 0;//帧序号重置为0
				}
			}
		}
	}
	//随机生成阳光球
	//但不能时时刻刻都创造
	
		creat_sunshine_ball();
		creat_sunshine_by_sunflower();
		update_sunshine_ball();
		creat_zombie();
		update_zombie();
		creat_bullet();
		update_bullet();
		collision_check();
		eat_check();
}

void draw_zombie() {
	//int pool_max = sizeof(zombie_pool) / sizeof(zombie_pool[0]);
	int pool_max = 10;
	for (int i = 0; i < pool_max; i++) {
		if (zombie_pool[i].if_dead) {//僵尸死亡但仍被使用，僵尸变成灰（20帧）之后才弃用
			putimagePNG(zombie_pool[i].x, zombie_pool[i].y,&zombie_dead_gif[zombie_pool[i].dead_frame_num]);
		}
		else if (zombie_pool[i].used) {
			if (zombie_pool[i].if_eat) {//吃东西
				putimagePNG(zombie_pool[i].x, zombie_pool[i].y, &zombie_eat_gif[zombie_pool[i].eat_frame_num]);
			}
			else {//正常走路
				putimagePNG(zombie_pool[i].x, zombie_pool[i].y, &zombie_gif[zombie_pool[i].frame_num]);
			}
		}
	}
}
void draw_bullet() {
	int bullet_max = sizeof(bullet_pool) / sizeof(bullet_pool[0]);
	for (int i = 0; i < bullet_max; i++) {
		if (bullet_pool[i].if_blast) {
			
			putimagePNG(bullet_pool[i].x, bullet_pool[i].y, &bullet_blast_gif[bullet_pool[i].frame_num]);

		}
		if (bullet_pool[i].used) {
			putimagePNG(bullet_pool[i].x, bullet_pool[i].y, &bullet_normal);
		}
	}
}



void update_window() {//按帧绘制


	BeginBatchDraw();//缓冲的目的是将那些一个个输出的图片存在一起输出

	putimage(0, 0, &img_bg);//背景，0,0是以左上角顶点为基准的坐标
	putimagePNG(250, 5, &img_tools_bar);//工具栏
	for (int i = 0; i < plant_count; i++) {//植物卡
		putimagePNG(333+65*i,10,&img_plant_card[i]);
	}
	
	//循环遍历种植区快，渲染出所有植物
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (plant_block[i][j].plant_type) {
				putimagePNG(253 + 82 * j, 177 + 102 * i, plant_gif[plant_block[i][j].plant_type-1][plant_block[i][j].frame_num]);
			}
		}
	}

	//渲染鼠标拖动途中的图片，放到渲染过程的最后，这样我们拖动植物时，是最上面的图层，而不会被覆盖
	if (plant_kind > 0) {
		IMAGE* img = plant_gif[plant_kind - 1][0];
		putimagePNG(cur_x - img->getwidth() / 2, cur_y - img->getheight() / 2, img);
	}
	for (int i = 0; i < pool_size; i++) {
		if (sunshine_ball_pool[i].used|| sunshine_ball_pool[i].dx|| sunshine_ball_pool[i].dy) {
			putimagePNG(sunshine_ball_pool[i].x, sunshine_ball_pool[i].y, sunshine_gif[sunshine_ball_pool[i].frame_num]);
		}
	}
	char score_sunshine[8];
	sprintf_s(score_sunshine, sizeof(score_sunshine), "%d", sunshine_own.value);
	outtextxy(279, 76, score_sunshine);//输出分数

	draw_zombie();//绘制僵尸
	draw_bullet();

	EndBatchDraw();
};
//点击阳光球时，进行处理，这里想大改逻辑，规定阳光球只有当回收的时候才能为“”
void deal_with_sunshine(ExMessage* mes) {
	int dx = sunshine_gif[0]->getwidth();
	int dy = sunshine_gif[0]->getheight();
	for (int i = 0; i < pool_size; i++) {//遍历阳光池，判断那些阳光有没有被选中
		if (sunshine_ball_pool[i].used&& sunshine_ball_pool[i].if_click==false) {//正在使用且没有被点击
			if (mes->x > sunshine_ball_pool[i].x && mes->x< sunshine_ball_pool[i].x + dx && mes->y>sunshine_ball_pool[i].y && mes->y < sunshine_ball_pool[i].y + dy) {
				
				//mciSendString("play res/sunshine.mp3", NULL, 0, NULL);//这里要防卡顿：替换为异步音频播放，避免阻塞主线程。
				mciSendString("play sunshine_audio from 0", NULL, 0, NULL);
				float angle = atan2(mes->y, mes->x - 271);//要小心除数为0,所以改用了atan2
				//float angle = atan2(mes->x - sunshine_ball_pool[i].x - 290, mes->y - sunshine_ball_pool[i].y);
				sunshine_ball_pool[i].dx = 15 * cos(angle);
				sunshine_ball_pool[i].dy = 15 * sin(angle);
				//sunshine_ball_pool[i].used = false;
				sunshine_ball_pool[i].if_click = true;//被点中
				///break;
				//如果在这里加上一个break，就可以实现一次只收集一个阳光？
			}

		}
	}
}

void user_click() {//用于对鼠标操作进行处理
	ExMessage mes;//这是一个用于保存鼠标状态的结构体
	
	if (peekmessage(&mes)) {//如果鼠标动了
		if (mes.message == WM_LBUTTONDOWN) {//点击左键了
			cur_x = mes.x;//这里也更新一下当前鼠标坐标的话就不会出现残影了吧
			cur_y = mes.y;
			if (mes.x > 333 && mes.x < 333 + 65 * plant_count && mes.y>10 && mes.y < 100) {//判断是否是从工具栏选择植物
				plant_kind = (mes.x - 333) / 65+1;//获取植物种类，plant_kind=0时，就是没有植物
				//printf("%d\n", plant_kind);
			}
			deal_with_sunshine(&mes);
		}
		else if (mes.message == WM_MOUSEMOVE&&plant_kind>0) {//鼠标移动
			cur_x = mes.x;
			cur_y = mes.y;

		}
		else if (mes.message == WM_LBUTTONUP) {//鼠标抬起
			int row = (mes.y - 177) / 102;
			int col = (mes.x - 253) / 82;
			if (row >= 0 && row <= 2 && col >= 0 && col <= 8&&plant_block[row][col].plant_type==0) {//区块没有被占用，且已选中植物，占据这个区块
				//
					plant_block[row][col].plant_type= plant_kind;
					plant_block[row][col].frame_num = 0;
					plant_block[row][col].health = 100;//植物的血量先设置为100
				
				/*printf("%d, %d\n", row, col);*/
			}
			plant_kind = 0;//鼠标松开后，全局变量归0
		}
	}
}
void start_ui() {//初始界面，缓存，循环渲染，检测鼠标信息改变变量三步走，我这里的逻辑和老师的不一样
	IMAGE ui_bg, ui_menu1, ui_menu2;
	loadimage(&ui_bg, "res/menu.png");
	loadimage(&ui_menu1, "res/menu1.png");
	loadimage(&ui_menu2, "res/menu2.png");
	//int flag = 0;
	while (1) {
		BeginBatchDraw();
		putimage(0, 0, &ui_bg);
		
		putimagePNG(473, 70, &ui_menu1);
		
		ExMessage meg;
		if (peekmessage(&meg)) {
			cur_x = meg.x;//全局变量保存鼠标位置，这样鼠标不动的时候也能知道他的位置
			cur_y = meg.y;
		}

		if (cur_x > 473 && cur_x < 973 && cur_y>70 && cur_y < 210) {//鼠标放上去时，颜色变暗
			putimagePNG(473, 70, &ui_menu2);
			if (meg.message == WM_LBUTTONDOWN) {//如果这个时候左键点了下去
				return;//进入主界面
			}
		}
		EndBatchDraw();
	}
}
void view_scenes_2() {

}
//片头巡场
//再次重申一下，putimage里的坐标是图片左上角顶点在窗口中的坐标
void view_scenes_1() {
	vector2 pointer[9] = { {550,80},{530,160},{630,170},{530,200},{515,270},{565,370},{605,304},{705,280},{690,340} };
	for (int i = 0; i < 9; i++) {//我是为了方便理解，但其实没必要整这个
		stand_zombie[i].x = pointer[i].x + 500;//这是僵尸在总背景图片上的位置
		stand_zombie[i].y = pointer[i].y;
		stand_zombie[i].frame_num = self_rand(0,11);
	}
	//背景板的坐标向左移动
	int offset_screen = img_width - img_bg.getwidth();//900-1400=-500,
	int timer = 0;
	for (int x = 0; x >= offset_screen; x-=2) {
		BeginBatchDraw();
		putimage(x, 0, &img_bg);//背景，随时间往左移动	
		timer++;
		for (int i = 0; i < 9; i++) {
			putimagePNG(stand_zombie[i].x + x, stand_zombie[i].y, &zombie_stand_gif[stand_zombie[i].frame_num]);
		}
		//每隔10帧，更新僵尸的动画帧
		if (timer > 2) {
			for (int i = 0; i < 9; i++) {
				stand_zombie[i].frame_num = (stand_zombie[i].frame_num + 1) % 11;
				timer = 0;
			}
		}

		Sleep(10);
		EndBatchDraw();
	}
	//Sleep(300);//这里可能要再优化一下这里如何保持僵尸动作无缝衔接的情况下停留300帧？
	//下面也是我根据老师的讲解举一反三的，而且我这里的处理比老师的处理丝滑
	for (int i = 0; i < 100; i++) {
		BeginBatchDraw();
		putimage(-500, 0, &img_bg);
		for (int i = 0; i < 9; i++) {
			putimagePNG(stand_zombie[i].x-500, stand_zombie[i].y, &zombie_stand_gif[stand_zombie[i].frame_num]);
		}
		timer++;
		if (timer > 2) {
			for (int i = 0; i < 9; i++) {
				stand_zombie[i].frame_num = (stand_zombie[i].frame_num + 1) % 11;
				timer = 0;
			}
		}
		Sleep(10);
		EndBatchDraw();
	}
	//场景往回拉
	for (int x = 0; x <= -1 * offset_screen; x += 2) {//x>0,
		BeginBatchDraw();
		putimage(-500+x, 0, &img_bg);//背景，随时间往左移动	
		timer++;
		for (int i = 0; i < 9; i++) {
			putimagePNG(stand_zombie[i].x + x-500, stand_zombie[i].y, &zombie_stand_gif[stand_zombie[i].frame_num]);
		}
		//每隔10帧，更新僵尸的动画帧
		if (timer > 2) {
			for (int i = 0; i < 9; i++) {
				stand_zombie[i].frame_num = (stand_zombie[i].frame_num + 1) % 11;
				timer = 0;
			}
		}
		Sleep(10);
		EndBatchDraw();
	}

}
void bars_down() {
	for (int y = -1*img_tools_bar.getheight(); y <= 0; y++) {
		BeginBatchDraw();
		putimage(0, 0, &img_bg);
		putimagePNG(250, y+5, &img_tools_bar);
		for (int i = 0; i < plant_count; i++) {
			putimagePNG(333 + 65 * i, y + 10, &img_plant_card[i]);
		}
		Sleep(10);
		EndBatchDraw();
	}
}
bool check_game_state() {
	if (game_state == going) {
		return true;
	}
	else {
		BeginBatchDraw();
		if (game_state == win) {
			putimage(0, 0, &game_success);
		}
		else if (game_state == fail) {
			putimage(0, 0, &game_fail);
		}
		EndBatchDraw();
		Sleep(100);
		return false;
	}
}
//场景往回拉，只需要在1的基础上改一下就好了
int main() {
	int timer = 0;//计时器
	mciSendString("open res/sunshine.mp3 alias sunshine_audio", NULL, 0, NULL);//预先加载音频文件，防止卡顿
	srand(time(0));
	gameinit();
	start_ui();
	view_scenes_1();
	bars_down();
	//view_scenes_2();
	while (1) {//循环user_click是为了一直处理用户的鼠标操作，而循环图片加载大概是为了实现动态植物的效果
		timer += getDelay();//获得距离上一次操作的时间差
		update_window();
		user_click();
		if (timer > 40) {//每隔40ms渲染一帧动图
			update_game();//用于更新游戏数据
			timer = 0;
			if (check_game_state()==false) {
				break;
			}
		}
	}
	system("pause");
	mciSendString("close sunshine_audio", NULL, 0, NULL);//关闭音频文件
	return 0;
}