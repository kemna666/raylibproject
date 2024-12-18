#define RAYGUI_IMPLEMENTATION
#include<stdio.h>
#include <raylib.h>
#include <raygui.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#define NUM_FRAMES_PER_LINE     5
#define NUM_LINES               5

enum PROCESS {GAMESTART, LOADED, MENU};
//玩家朝向
typedef enum FACE{LEFT,RIGHT} Face;
enum PROCESS game = GAMESTART;
Vector2 pos = {0, 0};
Vector2 initpos;
int CharactorMoveFrame = 0;
int remainingJumps = 2;  // 初始剩余跳跃次数为2次
bool isJumping = false;  // 用于标记角色是否正在跳跃
double lastJumpTime = 0;  // 上次跳跃的时间，初始化为0
const double jumpCooldown = 0.1;  // 跳跃冷却时间，单位为秒，可根据实际调整
int BossMoveFrame = 0;

typedef struct Boss {
	Texture2D Bosstpian[12];
	Vector2 position;
	Rectangle BossCenter;
	int blood;
	struct Boss *next;
	Vector2 velocity;
} Boss;
//子弹的结构体
typedef struct point {
	Texture2D PointImage;
	Vector2 Position;
	float speed; // 子弹速度
	bool collided;  // 新增标志位，表示子弹是否已碰撞
	Rectangle collisionBox;//碰撞箱
	float hastraveled;
} PointShoot;
//障碍物&平台的结构体
typedef struct Barrier Barrier;
struct Barrier {
	Texture2D tupian;
	Rectangle collisionBox;//碰撞箱
	Barrier* next;
};
void InitPointShoot(PointShoot pointshoot[], Texture2D texture, Vector2 position, float speed);
void LoadMap();
void GenerateBoss(Vector2 Position, const char Imagepath[]);
//将障碍物整理为链表
PointShoot pointshoot[10];
int count;
Barrier	 * Head = NULL;
Boss * HeadBoss = NULL;

//平台绘制以及加平台
void InsertPlatform(Barrier* head, Rectangle shape, char imagepath[]) {
	Barrier *current = Head;
	Texture2D tupian = LoadTexture(imagepath);
	Barrier* newbarrier = (Barrier*)malloc(sizeof(Barrier));
	newbarrier->tupian = tupian;
	newbarrier->collisionBox = shape;
	newbarrier->next = NULL;
	if (Head == NULL) {
		Head = newbarrier;
	} else {
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = newbarrier;
	}
}
//画血条
void DrawBossHealthBar(Boss *boss) {
	// 血条的宽度与Boss的宽度相同
	int healthBarWidth = boss->BossCenter.width+20;
	// 计算血条的当前长度，基于Boss的血量
	int healthBarLength = (boss->blood * healthBarWidth) / 100;
	// 血条的位置
	Vector2 healthBarPosition = { boss->BossCenter.x, boss->BossCenter.y - 10 };
	
	// 绘制血条的背景
	DrawRectangle((int)healthBarPosition.x, (int)(healthBarPosition.y - 5), healthBarWidth, 5, GRAY);
	// 绘制血条的当前血量
	DrawRectangle((int)healthBarPosition.x, (int)(healthBarPosition.y - 5), healthBarLength, 5, RED);
	char healthText[6]; // 用于存储血量数值的字符串
	sprintf(healthText, "%d/100", boss->blood); // 格式化字符串
	DrawText(healthText, (int)healthBarPosition.x, (int)(healthBarPosition.y - 20), 10, BLACK); // 绘制文本
}
void DrawPlatformTexture() {
	Barrier *current = Head;
	Boss *currentboss = HeadBoss;
	BeginDrawing();
	while (current != NULL) {
		DrawTexture(current->tupian, current->collisionBox.x, current->collisionBox.y, WHITE);
		current = current->next;
	}
	while (currentboss != NULL) {
		BossMoveFrame++;
		DrawTexture(currentboss->Bosstpian[BossMoveFrame], currentboss->position.x, currentboss->position.y, WHITE);
		if (BossMoveFrame >= 12) {
			BossMoveFrame = 0;
		}
		DrawBossHealthBar(currentboss);
		currentboss = currentboss->next;
	}
	EndDrawing();
}

//开始界面
void GameStartInterface() {
	//开始游戏的界面
	Image startGameBg = LoadImage("Assets\\wallhaven-x6loqd_1280x960.png");
	Texture2D bgstart = LoadTextureFromImage(startGameBg);
	BeginDrawing();
	DrawTexture(bgstart, 0, 0, WHITE);
	//判断按钮是否点击
	if (GuiButton((Rectangle) {
	120, 150, 150, 60
}, "开始游戏")) {
		LoadMap();
		game = LOADED;
	} else if (GuiButton((Rectangle) {
	700, 500, 150, 60
}, "退出游戏")) {
		//关闭游戏

	} else if (GuiButton((Rectangle) {
	600, 400, 150, 60
}, "菜单测试")) {
		game = MENU;
	}
	EndDrawing();
	UnloadTexture(bgstart);
}
// 解析文件并构建链表的函数
void parseFile(const char* filename, Barrier** head) {
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	char line[256];
	Rectangle shape;
	char imagePath[256];
	int parsingPosition = 0;  // 用于标记是否正在解析坐标部分
	bool parsingcharactor = 0;
	bool parsingBoss = 0;  // 新增标志位，用于标记是否正在解析Boss相关信息
	while (fgets(line, sizeof(line), file) != NULL) {
		// 跳过注释行（以#开头的行）
		if (line[0] == '#') {
			continue;
		}
		if (strstr(line, "[charactor]") != NULL) {
			parsingcharactor = 1;
			continue;
		}
		if (parsingcharactor) {
			if (strstr(line, "positionchar") != NULL) {
				sscanf(line, "positionchar = {%f,%f}", &initpos.x, &initpos.y);
			}
		}
		// 判断是否是[platform]开头，找到对应板块开始解析
		if (strstr(line, "[platform]") != NULL) {
			parsingPosition = 1;
			continue;
		}

		if (strstr(line, "[Boss]") != NULL) { // 新增判断，找到[Boss]板块开始解析
			parsingBoss = 1;
			continue;
		}

		if (parsingPosition) {
			// 解析坐标部分
			if (strstr(line, "position") != NULL) {
				sscanf(line, "position = {%f,%f,%f,%f};", &shape.x, &shape.y, &shape.height, &shape.width);
			}
			// 解析图像路径部分
			else if (strstr(line, "image") != NULL) {
				sscanf(line, "image = %s", imagePath);
				InsertPlatform(*head, shape, imagePath);
			}
		}

		if (parsingBoss) {  // 解析Boss相关信息
			Vector2 bossPosition;
			if (strstr(line, "positionboss") != NULL) {
				sscanf(line, "positionboss = {%f,%f};", &bossPosition.x, &bossPosition.y);
			} else if (strstr(line, "ImageBoss") != NULL) {
				sscanf(line, "ImageBoss = %s", imagePath);
				printf(imagePath);
				GenerateBoss(bossPosition, imagePath);  // 调用生成Boss的函数，并传入坐标和图片路径
			}
		}
	}
	fclose(file);
}
//加载地图
void LoadMap() {
	int numTexturesToDraw = 1280 / 16;
	Rectangle buttom = {0, 704, 16, 16};
	InitPointShoot(pointshoot, LoadTexture("Assets\\fruits\\bullet.png"), (Vector2){-1,-70}, 15);
	//底部平台
	for (int i = 0; i < numTexturesToDraw; i++) {
		InsertPlatform(Head, buttom, "Assets\\platforms\\4.png");
		buttom.x += 16;
	}
	parseFile("config.txt", &Head);
	pos = initpos;
	count = 0;
}
//加载子弹
void InitPointShoot(PointShoot pointshoot[], Texture2D texture, Vector2 position, float speed) {
	for(int i=0;i<10;i++){
		pointshoot[i].PointImage = texture;
		pointshoot[i].Position = position;
		pointshoot[i].speed = speed;
		pointshoot[i].collided = false;
		pointshoot[i].collisionBox = (Rectangle) {
			position.x, position.y, 1, 16
		};
	}
}
void UpdatePointShoot(Face direction) {	
	for(int i=0;i<10;i++){
	if (pointshoot->Position.x < GetScreenWidth()) {
		switch (direction) {
		case LEFT:
			pointshoot[i].Position.x -= pointshoot[i].speed;
			break;
		case RIGHT:
			pointshoot[i].Position.x += pointshoot[i].speed;
			break;
		default:
			break;
		}
		pointshoot[i].collisionBox.x = pointshoot[i].Position.x;
		pointshoot[i].collisionBox.y = pointshoot[i].Position.y;
		pointshoot[i].hastraveled++;
		if(pointshoot[i].hastraveled>=80){
			pointshoot[i].hastraveled = 0;
			pointshoot[i].collided =false;
			pointshoot[i].Position = (Vector2){-10,-70};
		}
	}
}
}
void DrawPointShoot() {
for(int i=0;i<10;i++){
		DrawTexture(pointshoot[i].PointImage, pointshoot[i].Position.x, pointshoot[i].Position.y + 10, WHITE);
}
}
bool CheckCollision(Rectangle rec1, Rectangle rec2) {
	return !((rec1.x + rec1.width < rec2.x) ||
		(rec2.x + rec2.width < rec1.x) ||
		(rec1.y + rec1.height < rec2.y) ||
		(rec2.y + rec2.height < rec1.y));
}
void CheckPlayerDown(Rectangle playerCenter) {
	bool collision = false;
	Barrier *current = Head;
	while (current != NULL) {
		if (CheckCollision(playerCenter, current->collisionBox)) {
			collision = true;
			break;
		}
		current = current->next;
	}
	if (!collision) {
		pos.y += 10; // 应用重力
	} else {
		pos.y = current->collisionBox.y - playerCenter.height; // 将玩家底部与平台顶部对齐
		isJumping = false; // 角色已经落地，停止跳跃状态
	}
}
void Destoryall(){
	Head =NULL;
	HeadBoss=NULL;
}

//菜单
void GameMenu() {
	BeginDrawing();
	if (GuiButton((Rectangle) {
	120, 150, 150, 30
}, "回到标题画面")) {
		game = GAMESTART;
		//销毁Boss和Platform
		Destoryall();
		pos.x = 0;
		pos.y = 0;
	}
	if (GuiButton((Rectangle) {
	120, 250, 150, 30
}, "返回游戏")) {
		game = LOADED;
	}
	EndDrawing();
}

//生成怪物
void GenerateBoss(Vector2 Position, const char Imagepath[]) {
	Boss* current = HeadBoss;
	Boss * newboss = (Boss*)malloc(sizeof(Boss));
	char buffer[80];
	for (int i = 0; i < 12; i++) {
		sprintf(buffer, "%s%d.png", Imagepath, i + 1);
		newboss->Bosstpian[i] = LoadTexture(buffer);

	}
	newboss->position = Position;
	newboss->BossCenter = (Rectangle) {
		Position.x, Position.y, 16, 24
	};
	newboss->velocity = (Vector2){2.0f, 0.0f};
	newboss->blood = 100;
	newboss->next = NULL;
	if (HeadBoss == NULL) {
		HeadBoss = newboss;
	} else {
		while (current->next != NULL) {
			current = current->next;
		}
		Boss *lastNode = current;  // 保存当前最后一个节点
		lastNode->next = newboss;  // 将最后一个节点的next指针指向新节点
	}
}
//boss移动
void UpdateBossPosition() {
	Boss *current = HeadBoss;
	Barrier *platform = Head;
	while(platform!=NULL){
		while (current != NULL) {
			current->velocity.x = (rand() % 4) - 2;
			if(current->position.x>10&&current->position.x<1240){
			current->position.x += current->velocity.x;
			current->position.y += current->velocity.y;
			}
			current = current->next;
		}
		platform = platform->next;
	}
	
}
//如果Boss没血就删除
void DeleteBoss(Boss **BossHead) {
	Boss *current = *BossHead;
	Boss *temp;
	while (current != NULL) {
		if (current->blood <= 0) {
			temp = current;
			*BossHead = current->next; // 更新头指针
			free(temp);
			current = *BossHead; // 重新指向头节点
		} else {
			current = current->next;
		}
	}
}
//更新boss碰撞箱
void UpdateBossCollisionBox() {
	Boss *currentboss = HeadBoss;
	while (currentboss != NULL) {
		currentboss->BossCenter.x = currentboss->position.x;
		currentboss->BossCenter.y = currentboss->position.y;
		currentboss = currentboss->next;
	}
}
//处理boss重力
void CheckBossDown() {
	bool collision = false;
	Barrier * current = Head;
	Boss *currentboss = HeadBoss;
	while (currentboss != NULL) {
		while (current != NULL) {
			if (CheckCollision(currentboss->BossCenter,current->collisionBox)) {
				collision = true;
				break;
			}
			current = current->next;
		}
		if (!collision) {
			currentboss->position.y += 10;
		} else {
			currentboss->position.y = current->collisionBox.y - 24; // 将Boss底部与平台顶部对齐
		}
		UpdateBossCollisionBox();
		currentboss = currentboss->next;
	}
}
//处理Boss行为

void ProcessBoss(Boss *BossHead, PointShoot pointshoot[]) {
	Boss *currentBoss = BossHead;
	while (currentBoss != NULL) {
		for(int i=0;i<10;i++){
		if (!pointshoot[i].collided && CheckCollision(pointshoot[i].collisionBox, currentBoss->BossCenter)) {
			currentBoss->blood --;
			pointshoot[i].collided = true; // 标记子弹已碰撞
		}
		}
		DeleteBoss(&HeadBoss);
		currentBoss = currentBoss->next;
	}
}
void CheckBossCollisionBox() {
	Boss * currentboss = HeadBoss;
	while (currentboss != NULL) {
		DrawRectangle((int)currentboss->BossCenter.x, (int)currentboss->BossCenter.y, currentboss->BossCenter.width, currentboss->BossCenter.height, RED);
		currentboss = currentboss->next;
	}
}
//在游戏左上角放角色血条
void DrawHealthBar(int health, int maxHealth) {
	// 血条的宽度和高度
	const int healthBarWidth = 200;
	const int healthBarHeight = 20;
	// 血条的位置（左上角）
	int healthBarX = 10;
	int healthBarY = 10;
	
	// 计算血条的当前长度，基于角色的血量
	int healthBarLength = (health * healthBarWidth) / maxHealth;
	
	// 绘制血条的背景
	DrawRectangle(healthBarX, healthBarY, healthBarWidth, healthBarHeight, GRAY);
	// 绘制血条的当前血量
	Color color = GREEN;
	if(health<=50&&health>=30){
		color = YELLOW;
	}else if(health<=30&&health>=20){
		color = PINK;
	}else if(health<20){
		color = RED;
	}else{
		color = GREEN;
	}
	DrawRectangle(healthBarX, healthBarY, healthBarLength, healthBarHeight, color);
	
}
void DrawHealthText(int health, int maxHealth) {
	char healthText[8];
	sprintf(healthText, "%d/%d", health, maxHealth); // 格式化字符串
	
	// 文本的位置（左上角，血条下方）
	int textX = 10; // 与血条相同的X坐标
	int textY = 30; // 血条下方的Y坐标，根据你的UI设计调整
	
	// 绘制文本
	DrawText(healthText, textX, textY, 20, BLACK); // 使用默认字体和大小绘制文本
}

//主函数
int main(void) {
	const int screenWidth = 1280;
	const int screenHeight = 720;
	double speed = 5;
	int blood = 100;
	srand(time(NULL));
	HeadBoss = NULL;
	InitWindow(screenWidth, screenHeight, "测试");
	// 读取字体文件到内存
	int fileSize;
	unsigned char *fontFileData = LoadFileData("Assets\\NSimSun-02.ttf", &fileSize); // 替换为你的字体文件路径
	// 收集所有需要的字符，这里以常用的中文字符为例
	const char *allChineseChars = "一二三四五六七八九十"
	                              "开始退出游戏回到标题画面返回游戏菜单测试成功发射子弹了恭喜你我要重来通失败关了！，"
	                              "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
	                              "!@#$%^&*()-_=+[]{}|;:'\",.<>/?`~";
	int codepointsCount;
	int *codepoints = LoadCodepoints(allChineseChars, &codepointsCount);
	// 加载字体到内存
	Font font = LoadFontFromMemory(".ttf", fontFileData, fileSize, 32, codepoints, codepointsCount);
	// 释放码点表
	UnloadCodepoints(codepoints);
	Texture2D charactorIDLE[4];
	Texture2D charactorMove[16];
	Texture2D charactorDeath[4];
	Texture2D charactorHITTEN[4];
	for (int i = 0; i < 4; i++) {
		char bufferA[] = "";
		char bufferB[] = "";
		char bufferC[] = "";
		sprintf(bufferA, "Assets\\charactors\\spilt\\IDLE_%d.png", i + 1);
		charactorIDLE[i] = LoadTexture(bufferA);
		sprintf(bufferB, "Assets\\charactors\\spilt\\Death_%d.png", i + 1);
		charactorDeath[i] = LoadTexture(bufferB);
		sprintf(bufferC, "Assets\\charactors\\spilt\\Hit_%d.png", i + 1);
		charactorHITTEN[i] = LoadTexture(bufferC);
	}
	for (int i = 0; i < 16; i++) {
		char bufferD[] = "";
		sprintf(bufferD, "Assets\\charactors\\spilt\\move_%d.png", i + 1);
		charactorMove[i] = LoadTexture(bufferD);
	}
	//给玩家建立碰撞箱
	Rectangle playerCenter = { pos.x, pos.y, 32, 32};
	InitAudioDevice();
	Music bgm = LoadMusicStream("Assets\\music\\bgm.mp3");
	bgm.looping = true;
	Face facing = RIGHT;
	SetTargetFPS(30);
	SetTraceLogLevel(LOG_WARNING);
	GuiSetFont(font);
	GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
	PlayMusicStream(bgm);
	Sound hit = LoadSound("Assets\\sounds\\hurt.wav");
	Sound jumpsound = LoadSound("Assets\\sounds\\jump.wav");
	//游戏主循环
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(RAYWHITE);
		UpdateMusicStream(bgm);
		switch (game) {
			case GAMESTART:
				GameStartInterface();
				continue;
			case LOADED:
				//正常进入关卡
				DrawPlatformTexture();
				BeginDrawing();
				DrawHealthBar(blood, 100); 
				DrawHealthText(blood, 100);
				CharactorMoveFrame++;
				if (CharactorMoveFrame >= 16) {
					CharactorMoveFrame = 0;
				}
				if (IsKeyDown(KEY_A)) {
					if (pos.x >= 0) {
						pos.x = pos.x - speed;
					}
					facing =LEFT;
					DrawTextureV(charactorMove[CharactorMoveFrame], pos, WHITE);
				} else if (IsKeyDown(KEY_D)) {
					//向右移动
					if (pos.x < GetScreenWidth() - 20) {
						pos.x = pos.x + speed;
					}
					facing = RIGHT;
					DrawTextureV(charactorMove[CharactorMoveFrame], pos, WHITE);
				}
				if (IsKeyReleased(KEY_A) || IsKeyReleased(KEY_D)) {
					CharactorMoveFrame = 0;
				} else if (IsKeyDown(KEY_LEFT_SHIFT)) {
					DrawTextureV(charactorMove[CharactorMoveFrame], pos, WHITE);
				} else {
					int a = CharactorMoveFrame / 8;
					DrawTextureV(charactorIDLE[a], pos, WHITE);
				}
				//处理角色移动
				if (GetKeyPressed() == KEY_R) {
					Destoryall();
					LoadMap();
					blood =100;
					StopMusicStream(bgm);
					PlayMusicStream(bgm);
				}
				//只允许跳两次
				if (IsKeyPressed(KEY_LEFT_SHIFT) && !isJumping && remainingJumps > 0) {
					pos.y -= 60; // 向上跳跃
					isJumping = true; // 设置跳跃状态
					remainingJumps--; // 减少跳跃次数
					lastJumpTime = GetTime(); // 更新上次跳跃时间
					PlaySound(jumpsound); // 播放跳跃声音
				}
				if (IsKeyReleased(KEY_LEFT_SHIFT)) {
					lastJumpTime = GetTime();
					isJumping = false;
					remainingJumps = 2;
				}
				if (IsKeyDown(KEY_M)) {
					game = MENU;
				}
				if (blood == 0) {
					if (CharactorMoveFrame >= 4) {
						CharactorMoveFrame = 0;
					}
					DrawTextureV(charactorDeath[CharactorMoveFrame], pos, WHITE);
				}
			UpdateBossPosition();
			if(HeadBoss==NULL){
				DrawTextEx(font,"恭喜你通关了！",(Vector2){560,350},150,10,GREEN);
				if(GuiButton((Rectangle){610,330,60,60},"返回标题画面")){
					blood = 100;
					game = GAMESTART;
				}
			}
				if(!isJumping){
				CheckPlayerDown(playerCenter);
				}
				
				CheckBossDown();
				if (IsKeyPressed(KEY_Z)) {
					// 发射子弹
					pointshoot[count].Position = pos; // 设置子弹初始位置为角色位置
					count++;
				}
				if(count>=9){
					count = 0;
				}
				//更新角色碰撞箱
				playerCenter =(Rectangle){pos.x,pos.y,32,32};
				Boss *currentboss = HeadBoss;
				while(currentboss!=NULL){
					if(blood>0&&CheckCollision(playerCenter,currentboss->BossCenter)){
					blood --;
					PlaySound(hit);
					}		
				currentboss = currentboss->next;
			}
				UpdatePointShoot(facing); // 更新子弹位置
				DrawPointShoot(); // 绘制子弹
				ProcessBoss(HeadBoss, pointshoot);
				if(blood<=0){
					DrawTextEx(font,"你失败了！",(Vector2){453,350},60,10,RED);
					if(GuiButton((Rectangle){600,419,140,50},"返回标题画面")){
						blood = 100;
						game = GAMESTART;
					}else if(GuiButton((Rectangle){453,419,100,50},"我要重来")){
						blood =100;
						Destoryall();
						LoadMap();
					}
				}
				continue;
			case MENU:
				//菜单界面
				GameMenu();
				continue;
		}

		EndDrawing();
	}
	for (int i = 0; i < 4 ; i++) {
		UnloadTexture(charactorDeath[i]);
		UnloadTexture(charactorHITTEN[i]);
		UnloadTexture(charactorIDLE[i]);
	}
	for (int i = 0; i < 16; i++) {
		UnloadTexture(charactorMove[i]);
	}
	// 释放字体
	UnloadFont(font);
	// 释放字体文件数据
	UnloadFileData(fontFileData);
	UnloadMusicStream(bgm);
	UnloadSound(hit);
	UnloadSound(jumpsound);
	CloseAudioDevice();

	CloseWindow();
	return 0;
}
