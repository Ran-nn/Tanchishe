#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<time.h>
#include<windows.h>
#include<stdlib.h>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

//数据库部分开始
#include <cstdlib>
#define _WIN32_WINNT 0x0601
#include <iostream>
#include <windows.h>
#include <wininet.h>
#include <string>
#include <sstream>

#pragma comment(lib, "wininet.lib")

// 函数用于发送 GET 请求
std::string sendGetRequest(const std::string& url) {
    HINTERNET hInternet = InternetOpen(L"APIRequest", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        std::cerr << "InternetOpen failed: " << GetLastError() << std::endl;
        return "";
    }

    std::wstring wideUrl(url.begin(), url.end());
    HINTERNET hConnect = InternetOpenUrl(hInternet, wideUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        std::cerr << "InternetOpenUrl failed: " << GetLastError() << std::endl;
        InternetCloseHandle(hInternet);
        return "";
    }

    char buffer[1024];
    std::string response;
    DWORD bytesRead;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        response.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return response;
}

// 生成 GET 请求的 URL
std::string createGetUrl(const std::string& baseUrl, const std::string& username, const std::string& password) {
    return baseUrl + "?username=" + username + "&password=" + password;
}

// 检查响应中是否包含特定消息
bool checkResponseForMessage(const std::string& response, const std::string& message) {
    return response.find(message) != std::string::npos;
}

// 注册函数
bool registerUser(const std::string& username, const std::string& password) {
    std::string baseUrl = "http://127.0.0.1:5000/register";
    std::string url = createGetUrl(baseUrl, username, password);
    std::string response = sendGetRequest(url);

    return checkResponseForMessage(response, "User registered successfully");
}

// 登录函数，验证账号密码
bool loginUser(const std::string& username, const std::string& password) {
    std::string baseUrl = "http://127.0.0.1:5000/verify";
    std::string url = createGetUrl(baseUrl, username, password);
    std::string response = sendGetRequest(url);

    return checkResponseForMessage(response, "Password verified successfully");
}

// 记录游戏历史记录
void recordGameHistory(const std::string& username, const std::string& start_time, const std::string& end_time, int score) {
    std::string baseUrl = "http://127.0.0.1:5000/game_history";
    std::string url = baseUrl + "?username=" + username + "&start_time=" + start_time + "&end_time=" + end_time + "&score=" + std::to_string(score);
    std::string response = sendGetRequest(url);
    std::cout << "Game history response: " << response << std::endl;
}

// 将时间字符串转换为时间戳
std::time_t convertToTimestamp(const std::string& timeStr) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        std::cerr << "时间字符串解析失败" << std::endl;
        return -1;
    }
    return std::mktime(&tm);
}

// 计算两个时间字符串的时差
long calculateTimeDifference(const std::string& timeStr1, const std::string& timeStr2) {
    std::time_t timestamp1 = convertToTimestamp(timeStr1);
    std::time_t timestamp2 = convertToTimestamp(timeStr2);
    if (timestamp1 == -1 || timestamp2 == -1) {
        return -1;
    }
    return std::abs(timestamp2 - timestamp1);
}

// 获取用户游戏历史记录
void getGameHistory(const std::string& username) {
    std::string baseUrl = "http://127.0.0.1:5000/get_game_history";
    std::string url = baseUrl + "?username=" + username;
    std::string response = sendGetRequest(url);
    using json = nlohmann::json;
    try {
        json history_list = json::parse(response);
        std::cout << "用户 " << username << " 的游戏历史记录：" << std::endl;
        int i = 0;
        for (const auto& history : history_list) {
            std::cout << "开始时间: " << history["start_time"] << std::endl;
            std::cout << "结束时间: " << history["end_time"] << std::endl;
            std::cout << "得分: " << history["score"] << std::endl;
            std::cout << "游玩时长(秒数): " << calculateTimeDifference(history["start_time"], history["end_time"]) << std::endl;
            std::cout << "------------------------" << std::endl;
           
        }
    }
    catch (const json::parse_error& e) {
        std::cerr << "解析 JSON 数据时出错: " << e.what() << std::endl;
    }
}

//数据库部分结束

#define U 1
#define D 2
#define L 3 
#define R 4       //蛇的状态，U：上 ；D：下；L:左 R：右

typedef struct SNAKE //蛇身的一个节点
{
    int x;
    int y;
    struct SNAKE* next;
}snake;

//全局变量//
int score = 0, add = 10;//总得分与每次吃食物得分。
int status, sleeptime = 200;//每次运行的时间间隔
snake* head, * food;//蛇头指针，食物指针
snake* q;//遍历蛇的时候用到的指针
int endgamestatus = 0; //游戏结束的情况，1：撞到墙；2：咬到自己；3：主动退出游戏。

//声明全部函数//
void Pos();
void creatMap();
void initsnake();
int biteself();
void createfood();
void cantcrosswall();
void snakemove();
void pause();
void gamecircle();
void welcometogame();
void endgame();
void gamestart();

void Pos(int x, int y)//设置光标位置
{
    COORD pos;
    HANDLE hOutput;
    pos.X = x;
    pos.Y = y;
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(hOutput, pos);
}

void creatMap()//创建地图
{
    int i;
    for (i = 0; i < 58; i += 2)//打印上下边框
    {
        Pos(i, 0);
        printf("■");
        Pos(i, 26);
        printf("■");
    }
    for (i = 1; i < 26; i++)//打印左右边框
    {
        Pos(0, i);
        printf("■");
        Pos(56, i);
        printf("■");
    }
}

std::string start_time_str;
std::string end_time_str;
std::string his_username;
//系统时间获取
// 函数用于返回当前系统时间（包含秒数）
std::string getCurrentTime() {
    std::time_t currentTime = std::time(nullptr);
    std::tm* localTime = std::localtime(&currentTime);

    char timeString[80];
    // 格式化时间字符串
    std::strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localTime);

    return std::string(timeString);
}

void initsnake()//初始化蛇身
{
    start_time_str = getCurrentTime();
    snake* tail;
    int i;
    tail = (snake*)malloc(sizeof(snake));//从蛇尾开始，头插法，以x,y设定开始的位置//
    tail->x = 24;
    tail->y = 5;
    tail->next = NULL;
    for (i = 1; i <= 4; i++)
    {
        head = (snake*)malloc(sizeof(snake));
        head->next = tail;
        head->x = 24 + 2 * i;
        head->y = 5;
        tail = head;
    }
    while (tail != NULL)//从头到为，输出蛇身
    {
        Pos(tail->x, tail->y);
        printf("■");
        tail = tail->next;
    }
}

int biteself()//判断是否咬到了自己
{
    snake* self;
    self = head->next;
    while (self != NULL)
    {
        if (self->x == head->x && self->y == head->y)
        {
            return 1;
        }
        self = self->next;
    }
    return 0;
}

void createfood()//随机出现食物
{
    snake* food_1;
    srand((unsigned)time(NULL));
    food_1 = (snake*)malloc(sizeof(snake));
    while ((food_1->x % 2) != 0)    //保证其为偶数，使得食物能与蛇头对其
    {
        food_1->x = rand() % 52 + 2;
    }
    food_1->y = rand() % 24 + 1;
    q = head;
    while (q->next == NULL)
    {
        if (q->x == food_1->x && q->y == food_1->y) //判断蛇身是否与食物重合
        {
            free(food_1);
            createfood();
        }
        q = q->next;
    }
    Pos(food_1->x, food_1->y);
    food = food_1;
    printf("■");
}

void cantcrosswall()//不能穿墙
{
    if (head->x == 0 || head->x == 56 || head->y == 0 || head->y == 26)
    {
        endgamestatus = 1;
        endgame();
    }
}

void snakemove()//蛇前进,上U,下D,左L,右R
{
    snake* nexthead;
    cantcrosswall();

    nexthead = (snake*)malloc(sizeof(snake));
    if (status == U)
    {
        nexthead->x = head->x;
        nexthead->y = head->y - 1;
        if (nexthead->x == food->x && nexthead->y == food->y)//如果下一个有食物//
        {
            nexthead->next = head;
            head = nexthead;
            q = head;
            while (q != NULL)
            {
                Pos(q->x, q->y);
                printf("■");
                q = q->next;
            }
            score = score + add;
            createfood();
        }
        else                                               //如果没有食物//
        {
            nexthead->next = head;
            head = nexthead;
            q = head;
            while (q->next->next != NULL)
            {
                Pos(q->x, q->y);
                printf("■");
                q = q->next;
            }
            Pos(q->next->x, q->next->y);
            printf("  ");
            free(q->next);
            q->next = NULL;
        }
    }
    if (status == D)
    {
        nexthead->x = head->x;
        nexthead->y = head->y + 1;
        if (nexthead->x == food->x && nexthead->y == food->y)  //有食物
        {
            nexthead->next = head;
            head = nexthead;
            q = head;
            while (q != NULL)
            {
                Pos(q->x, q->y);
                printf("■");
                q = q->next;
            }
            score = score + add;
            createfood();
        }
        else                               //没有食物
        {
            nexthead->next = head;
            head = nexthead;
            q = head;
            while (q->next->next != NULL)
            {
                Pos(q->x, q->y);
                printf("■");
                q = q->next;
            }
            Pos(q->next->x, q->next->y);
            printf("  ");
            free(q->next);
            q->next = NULL;
        }
    }
    if (status == L)
    {
        nexthead->x = head->x - 2;
        nexthead->y = head->y;
        if (nexthead->x == food->x && nexthead->y == food->y)//有食物
        {
            nexthead->next = head;
            head = nexthead;
            q = head;
            while (q != NULL)
            {
                Pos(q->x, q->y);
                printf("■");
                q = q->next;
            }
            score = score + add;
            createfood();
        }
        else                                //没有食物
        {
            nexthead->next = head;
            head = nexthead;
            q = head;
            while (q->next->next != NULL)
            {
                Pos(q->x, q->y);
                printf("■");
                q = q->next;
            }
            Pos(q->next->x, q->next->y);
            printf("  ");
            free(q->next);
            q->next = NULL;
        }
    }
    if (status == R)
    {
        nexthead->x = head->x + 2;
        nexthead->y = head->y;
        if (nexthead->x == food->x && nexthead->y == food->y)//有食物
        {
            nexthead->next = head;
            head = nexthead;
            q = head;
            while (q != NULL)
            {
                Pos(q->x, q->y);
                printf("■");
                q = q->next;
            }
            score = score + add;
            createfood();
        }
        else                                         //没有食物
        {
            nexthead->next = head;
            head = nexthead;
            q = head;
            while (q->next->next != NULL)
            {
                Pos(q->x, q->y);
                printf("■");
                q = q->next;
            }
            Pos(q->next->x, q->next->y);
            printf("  ");
            free(q->next);
            q->next = NULL;
        }
    }
    if (biteself() == 1)       //判断是否会咬到自己
    {
        endgamestatus = 2;
        endgame();
    }
}

void pause()//暂停
{
    while (1)
    {
        Sleep(300);
        if (GetAsyncKeyState(VK_SPACE))
        {
            break;
        }

    }
}



void gamecircle(const std::string& username)//控制游戏        
{
   
    his_username = username;
    time_t startTime = time(nullptr);
    Pos(64, 8);
    std::cout << "用户" << username << "正在游戏中" << std::endl;

    Pos(64, 15);
    printf("不能穿墙，不能咬到自己\n");
    Pos(64, 16);
    printf("用↑.↓.←.→分别控制蛇的移动.");
    Pos(64, 17);
    printf("F1 为加速，F2 为减速\n");
    Pos(64, 18);
    printf("ESC ：退出游戏.space：暂停游戏. F5: 查看历史记录");
    Pos(64, 20);
    status = R;
    while (1)
    {
        Pos(64, 10);
        printf("得分：%d  ", score);
        Pos(64, 11);
        printf("每个食物得分：%d分", add);
        if (GetAsyncKeyState(VK_UP) && status != D)
        {
            status = U;
        }
        else if (GetAsyncKeyState(VK_DOWN) && status != U)
        {
            status = D;
        }
        else if (GetAsyncKeyState(VK_LEFT) && status != R)
        {
            status = L;
        }
        else if (GetAsyncKeyState(VK_RIGHT) && status != L)
        {
            status = R;
        }
        else if (GetAsyncKeyState(VK_SPACE))
        {
            pause();
        }
        else if (GetAsyncKeyState(VK_ESCAPE))
        {
            endgamestatus = 3;
            break;
        }
        else if (GetAsyncKeyState(VK_F1))
        {
            if (sleeptime >= 50)
            {
                sleeptime = sleeptime - 30;
                add = add + 2;
                if (sleeptime == 320)
                {
                    add = 2;//防止减到1之后再加回来有错
                }
            }
        }
        else if (GetAsyncKeyState(VK_F2))
        {
            if (sleeptime < 350)
            {
                sleeptime = sleeptime + 30;
                add = add - 2;
                if (sleeptime == 350)
                {
                    add = 1;  //保证最低分为1
                }
            }
        }
        Sleep(sleeptime);
        snakemove();
    }
    time_t endTime = time(nullptr);
    //start_time_str = std::ctime(&startTime);
    //end_time_str = std::ctime(&endTime);
    //start_time_str.pop_back(); // 去掉换行符
    //end_time_str.pop_back();   // 去掉换行符
    
    

}

void welcometogame()//开始界面
{
    Pos(40, 12);
    printf("欢迎来到贪食蛇游戏！");
    Pos(40, 25);
    system("pause");
    system("cls");
    Pos(25, 12);
    printf("用↑.↓.←.→分别控制蛇的移动， F1 为加速，2 为减速\n");
    Pos(25, 13);
    printf("加速将能得到更高的分数。\n");
    system("pause");
    system("cls");
}

void endgame()//结束游戏
{
    printf("\n 开始记录游戏数据... \n");
    end_time_str = getCurrentTime();
    recordGameHistory(his_username, start_time_str, end_time_str, score);
    system("cls");
    Pos(24, 12);
    if (endgamestatus == 1)
    {
        printf("对不起，您撞到墙了。游戏结束!");
    }
    else if (endgamestatus == 2)
    {
        printf("对不起，您咬到自己了。游戏结束!");
    }
    else if (endgamestatus == 3)
    {
        printf("您已经结束了游戏。");
    }
    Pos(24, 13);
    printf("您的得分是%d\n", score);
    exit(0);
    return;
}

void gamestart()//游戏初始化
{
    system("mode con cols=100 lines=30");
    welcometogame();
    creatMap();
    initsnake();
    createfood();
}


void endgameWithPrompt(const std::string& username) {
    system("cls");

    while (true) { // 新增循环，持续检测按键
        std::cout << "\n按 F5 查看历史分数，按 F6 退出程序..." << std::endl;

        // 检测 F5 按键
        if (GetAsyncKeyState(VK_F5) & 0x8000) { // 高位为1表示按键按下
            system("cls");
            getGameHistory(username); // 调用查看历史分数的函数
            std::cout << "\n按任意键返回..." << std::endl;
            
        }

        // 检测 F4 按键退出
        if (GetAsyncKeyState(VK_F4) & 0x8000) {
            system("cls");
            std::cout << "程序退出..." << std::endl;
            break; // 跳出循环，结束程序
        }

        Sleep(100); // 控制循环频率，避免高CPU占用
    }
}

int main()
{
    int choice;
    std::string username, password;

    while (true) {
        std::cout << "请选择操作：" << std::endl;
        std::cout << "1. 登录" << std::endl;
        std::cout << "2. 注册" << std::endl;
        std::cin >> choice;

        if (choice == 1) {
            std::cout << "请输入用户名: ";
            std::cin >> username;
            std::cout << "请输入密码: ";
            std::cin >> password;

            if (loginUser(username, password)) {
                std::cout << "登录成功！" << std::endl;
                system("cls"); // 调用清屏函数
                std::cout << "用户" << username << "正在游戏中" << std::endl;
                break;
            }
            else {
                std::cout << "密码错误，请重新选择操作。" << std::endl;
            }
        }
        else if (choice == 2) {
            std::cout << "请输入要注册的用户名: ";
            std::cin >> username;
            std::cout << "请输入要注册的密码: ";
            std::cin >> password;

            if (registerUser(username, password)) {
                std::cout << "注册成功，请登录。" << std::endl;
            }
            else {
                std::cout << "注册失败，请重新选择操作。" << std::endl;
            }
        }
        else {
            std::cout << "无效的选择，请重新输入。" << std::endl;
        }
    }

    system("cls");
    std::cout << "\n F5查看历史分数...F6键返回..." << std::endl;
    while (1) {
        // 检测 F5 按键
        if (GetAsyncKeyState(VK_F5) & 0x8000) { // 高位为1表示按键按下
            system("cls");
            getGameHistory(username); // 调用查看历史分数的函数
            std::cout << "\n F6 退出..." << std::endl;
        }
        if (GetAsyncKeyState(VK_F6) & 0x8000) { // 高位为1表示按键按下
            break;
        }
        Sleep(100);

    }
   
    gamestart();
    gamecircle(username);
    endgame();
    endgameWithPrompt(username);
    return 0;
}