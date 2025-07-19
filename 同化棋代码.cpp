// 引入必要的头文件
#include <algorithm>  // 提供排序等算法函数
#include <cstring>    // 提供内存操作函数如memcpy、memset等
#include <ctime>      // 提供时间相关函数
#include <iostream>   // 提供输入输出流功能

// 定义评估函数使用的常量
#define MAX1 100      // 最大正评估值
#define MAX0 50       // 较大正评估值
#define MIN1 -100     // 最小负评估值
#define MIN0 -50      // 较小负评估值
#define limitT 0.9    // 搜索时间限制(秒)
#define detectN 3     // 检测时间的深度级别
#define initDepth 4   // 初始搜索深度

using namespace std;

// 棋子掩码数组，用于快速修改棋盘状态
static uint8_t mask[4] = { 0x15, 0, 0, 0x15 };
// 棋盘显示字符：#表示空格，b表示黑子，w表示白子
static char code[4] = { '#', '#', 'b', 'w' };
// 24个方向的偏移量，用于计算可能的移动位置
// 前8个是相邻位置，后16个是跳跃位置
static int8_t delta[24][2] = {
        {1, 1},  {0, 1},   {-1, 1},  {-1, 0},  {-1, -1}, {0, -1}, {1, -1}, {1, 0},
        {2, 0},  {2, 1},   {2, 2},   {1, 2},   {0, 2},   {-1, 2}, {-2, 2}, {-2, 1},
        {-2, 0}, {-2, -1}, {-2, -2}, {-1, -2}, {0, -2},  {1, -2}, {2, -2}, {2, -1} };

// 全局状态变量
static bool timeUp = false;  // 时间是否用完的标志
static bool itSTOP = false;  // 迭代是否应该停止的标志
static clock_t clk0;         // 开始时间的时钟记录

// 定义一步棋的移动结构体
struct moveStep {
    int8_t startX, startY;  // 起始位置坐标
    int8_t endX, endY;      // 目标位置坐标
};

// 重载输入运算符，用于读取moveStep结构体
istream& operator>>(istream& is, moveStep& m) {
    int startX, startY, endX, endY;
    is >> startX >> startY >> endX >> endY;
    m.startX = startX;
    m.startY = startY;
    m.endX = endX;
    m.endY = endY;
    return is;
}

// 重载输出运算符，用于输出moveStep结构体
ostream& operator<<(ostream& os, moveStep& m) {
    os << int(m.startX) << ' ';
    os << int(m.startY) << ' ';
    os << int(m.endX) << ' ';
    os << int(m.endY);
    return os;
}

// 前置声明
class storedNode;
class storedTree;

// 棋盘类，表示游戏的状态
class chessBoard {
    friend class storedNode;  // 声明友元类，使storedNode可以访问私有成员
    friend class storedTree;  // 声明友元类，使storedTree可以访问私有成员
    // 添加这一行，使operator<能够访问storedVal
    friend bool operator<(const storedNode& N1, const storedNode& N2);

    // 棋盘信息存储：0/1-空格 2-黑子 3-白子
    // 使用位运算高效存储7x7棋盘
    uint16_t info[7];  // 每行用16位整数表示，每2位表示一个位置的状态
    int8_t currColor;  // 当前行动方颜色：2-黑色，3-白色
    int8_t storedVal;  // 存储当前棋盘的评估值

public:
    // 默认构造函数
    chessBoard() {}

    // 从输入流构造棋盘，用于游戏初始化
    chessBoard(istream& is) {
        // 初始化棋盘为空
        memset(info, 0, 7 * sizeof(uint16_t));
        // 设置初始黑白棋子在棋盘角落
        info[0] = 0xC008;  // |B|W| 在第一行放置黑白棋子
        info[6] = 0x800C;  // |W|B| 在最后一行放置白黑棋子
        currColor = 2;     // 黑方先行

        // 读取当前回合数
        int turnID;
        is >> turnID;
        moveStep tmp;

        // 重放之前的所有移动步骤以还原当前局面
        for (int i = 1; i != turnID; ++i) {
            // 读取敌方移动
            is >> tmp;
            if (tmp.endX >= 0) procStep(tmp);  // 如果有效则处理
            // 读取我方移动
            is >> tmp;
            procStep(tmp);  // 处理我方移动
        }

        // 读取当前回合敌方的移动
        is >> tmp;
        if (tmp.endX >= 0) procStep(tmp);
    }

    // 拷贝构造函数
    chessBoard(const chessBoard& B) {
        memcpy(info, B.info, 7 * sizeof(uint16_t));
        currColor = B.currColor;
        storedVal = B.storedVal;
    }

    // 检查坐标是否在棋盘内
    bool inMap(int8_t x, int8_t y) {
        return x >= 0 && x <= 6 && y >= 0 && y <= 6;
    }

    // 获取指定位置的棋子状态
    int8_t pos(int8_t x, int8_t y) { return (info[x] >> ((y + 1) << 1)) & 3; }

    // 检查指定位置是否有当前玩家的棋子
    bool avail(int8_t x, int8_t y) {
        return ((info[x] >> ((y + 1) << 1)) & 3) == currColor;
    }

    // 检查指定位置是否为空
    bool empty(int8_t x, int8_t y) { return !((info[x] >> ((y + 1) << 1)) & 2); }

    // 处理一步棋，更新棋盘状态
    void procStep(const moveStep& m) {
        int8_t startX = m.startX, startY = m.startY, endX = m.endX, endY = m.endY;

        // 如果是跳跃移动，清除起始位置的棋子
        if (abs(endX - startX) == 2 || abs(endY - startY) == 2)
            info[startX] ^= (2 << ((startY + 1) << 1));

        // 清除目标位置的旧状态并设置新棋子
        info[endX] &= ~(3 << ((endY + 1) << 1));
        info[endX] |= (currColor << ((endY + 1) << 1));

        // 更新目标位置周围的状态（游戏规则相关）
        for (int8_t i = -1; i <= 1; ++i)
            if (endX + i >= 0 && endX + i <= 6) {
                info[endX + i] &= ~(mask[0] << (endY << 1));
                info[endX + i] |= (mask[currColor] << (endY << 1));
            }

        // 切换当前玩家（2 -> 3 或 3 -> 2）
        currColor = 5 - currColor;
    }

    // 计算当前棋盘的评估值
    void val() {
        int8_t sum[4] = { 0 };  // 统计各类棋子数量

        // 遍历整个棋盘计算各类棋子数量
        for (uint8_t i = 0; i != 7; ++i) {
            uint16_t line = info[i];
            for (uint8_t j = 0; j != 7; ++j) ++sum[(line >> ((j + 1) << 1)) & 3];
        }

        // 评估值为当前玩家棋子数减去对手棋子数
        storedVal = sum[currColor] - sum[5 - currColor];
    }

    // 终局评估，当一方无法移动时评估胜负
    void end(int8_t level) {
        int8_t sum[4] = { 0 };

        // 统计各类棋子数量
        for (uint8_t i = 0; i != 7; ++i) {
            uint16_t line = info[i];
            for (uint8_t j = 0; j != 7; ++j) ++sum[(line >> ((j + 1) << 1)) & 3];
        }

        // 如果当前玩家棋子数少于等于24，判定为失败，否则为胜利
        // level用于区分不同深度的终局状态
        storedVal = sum[currColor] <= 24 ? MIN1 + level : MAX1 + level;
    }

    // 打印当前棋盘状态
    void print() {
        for (int8_t i = 0; i != 7; ++i) {
            for (int8_t j = 0; j != 7; ++j) cout << code[pos(i, j)];
            cout << endl;
        }
        cout << int(storedVal) << endl;
    }
};

// 博弈树节点类，继承自棋盘类
class storedNode : public chessBoard {
    friend class storedTree;  // 友元类，可访问私有成员

    moveStep from;     // 到达此节点的移动步骤
    uint16_t num;      // 子节点数量
    storedNode* chld;  // 指向子节点数组的指针

public:
    // 默认构造函数
    storedNode() : chessBoard(), num(0xFFFF), chld(NULL) {}

    // 从棋盘构造节点
    storedNode(const chessBoard& B) : chessBoard(B) {
        num = 0xFFFF;  // 0xFFFF表示尚未计算子节点
        chld = NULL;   // 初始子节点指针为空
    }

    // 递归删除节点及其所有子节点
    void deleteNode() {
        if (num == 0xFFFF) return;  // 如果未展开子节点则直接返回

        // 递归删除所有子节点
        for (uint16_t i = 0; i != num; ++i) chld[i].deleteNode();

        // 删除子节点数组
        if (!chld) delete[] chld;
    }

    // 计算所有可行的移动并创建子节点
    void calcAvail(bool dsort) {
        // 如果尚未计算子节点
        if (!(uint16_t(~num))) {
            moveStep av[600];  // 存储所有可能的移动
            num = 0;           // 重置子节点计数器

            // 遍历棋盘寻找所有可能的移动
            for (int8_t x0 = 0; x0 != 7; ++x0)
                for (int8_t y0 = 0; y0 != 7; ++y0)
                    if (empty(x0, y0)) {  // 如果目标位置为空
                        uint8_t i;

                        // 先检查8个相邻方向的移动
                        for (i = 0; i != 8; ++i) {
                            int8_t x1 = x0 + delta[i][0], y1 = y0 + delta[i][1];
                            if (!inMap(x1, y1)) continue;  // 如果超出棋盘则跳过
                            if (!avail(x1, y1)) continue;  // 如果不是当前玩家的棋子则跳过

                            // 创建移动步骤并添加到数组
                            moveStep tmp = { x1, y1, x0, y0 };
                            av[num++] = tmp;
                            break;  // 找到一个相邻的移动后就不再检查其他相邻方向
                        }

                        // 再检查16个跳跃方向的移动
                        for (i = 8; i != 24; ++i) {
                            int8_t x1 = x0 + delta[i][0], y1 = y0 + delta[i][1];
                            if (!inMap(x1, y1)) continue;  // 如果超出棋盘则跳过
                            if (!avail(x1, y1)) continue;  // 如果不是当前玩家的棋子则跳过

                            // 创建移动步骤并添加到数组
                            moveStep tmp = { x1, y1, x0, y0 };
                            av[num++] = tmp;
                        }
                    }

            // 分配子节点数组内存
            chld = new storedNode[num];

            // 为每个子节点设置状态并进行评估
            for (uint16_t i = 0; i != num; ++i) {
                chld[i] = chessBoard(*this);   // 复制当前棋盘状态
                chld[i].procStep(av[i]);       // 执行移动步骤
                chld[i].from = av[i];          // 记录移动步骤
                chld[i].val();                 // 评估新棋盘状态
            }
        }

        // 如果需要，对子节点进行排序（便于Alpha-Beta剪枝）
        if (dsort) sort(chld, chld + num);
    }

    // 获取最佳移动（评估值最小的子节点对应的移动）
    moveStep best() {
        stable_sort(chld, chld + num);  // 稳定排序确保相同评估值的移动保持顺序
        return chld[0].from;           // 返回评估值最小的子节点对应的移动
    }

    // 打印所有子节点的信息（调试用）
    void printAll() {
        for (uint16_t i = 0; i != num; ++i) {
            cout << chld[i].from << endl;
            chld[i].print();
            cout << int(chld[i].storedVal) << endl;
            cout << endl;
        }
    }

    // 打印调试信息
    void printDebug() {
        cout << num << " :: ";
        for (uint16_t i = 0; i != num; ++i)
            cout << chld[i].from << " : " << int(chld[i].storedVal) << " / ";
    }
};

// 重载比较运算符，用于排序节点
// 评估值越小的节点越"小"，在排序后会排在前面
bool operator<(const storedNode& N1, const storedNode& N2) {
    return N1.storedVal < N2.storedVal;
}

// 博弈树类，管理整个搜索过程
class storedTree {
    storedNode root;  // 根节点
    int8_t depth;     // 当前搜索深度

public:
    // 构造函数，从标准输入读取初始棋盘状态
    storedTree() {
        chessBoard B(cin);
        root = storedNode(B);
    }

    // 析构函数（已注释掉）
    // ~storedTree() { root.deleteNode(); }

    // Alpha-Beta深度优先搜索
    // N: 当前节点，n: 剩余深度，x: Alpha值
    void abDFS(storedNode& N, int8_t n, int8_t& x) {
        if (n == 0)
            // 已到达搜索深度限制，直接评估节点值
            N.val();
        else if (!(uint16_t(~N.num)) ||
            (N.storedVal >= MIN0 && N.storedVal <= MAX0)) {
            // 如果节点尚未展开或者评估值在一定范围内
            N.storedVal = MIN1;  // 初始化为最小值

            // 计算所有可行移动
            N.calcAvail(n != depth);

            // 如果无可行移动，评估终局状态
            if (N.num == 0) N.end(depth - n);

            uint16_t i;
            // 遍历所有子节点，进行Alpha-Beta剪枝
            for (i = 0; i != N.num; ++i) {
                // 递归计算子节点的评估值
                abDFS(N.chld[i], n - 1, N.storedVal);

                // Alpha-Beta剪枝：如果当前节点的值已经大于父节点的Beta值，则中断搜索
                if (N.storedVal > -x) {
                    ++i;
                    break;
                }

                // 如果时间用完，中断搜索
                if (timeUp) break;
            }

            // 将未搜索的子节点的评估值设为最大
            for (; i < N.num; ++i) N.chld[i].storedVal = MAX0;
        }

        // 在特定深度检查是否时间用完
        if (n == detectN)
            timeUp = (clock() - clk0) / double(CLOCKS_PER_SEC) > limitT;

        // 更新Alpha值
        if (x < -N.storedVal) x = -N.storedVal;
    }

    // 执行一次Alpha-Beta搜索，返回最佳移动
    moveStep alpha_beta(int8_t n) {
        // 计算根节点的所有可行移动
        root.calcAvail(true);

        // 初始Alpha值为最小负值
        int8_t ans = MIN1;

        // 执行Alpha-Beta搜索
        abDFS(root, n, ans);

        // 如果根节点的评估值超出了某个范围，标记为可以停止迭代
        if (root.storedVal <= MIN0 || root.storedVal >= MAX0) itSTOP = true;

        // 返回最佳移动
        return root.best();
    }

    // 迭代加深搜索，在时间限制内不断增加搜索深度
    moveStep iteration() {
        // 记录开始时间
        clk0 = clock();

        // 设置初始搜索深度
        depth = initDepth;

        // 执行第一次搜索
        moveStep best = alpha_beta(depth);

        // 当时间未用完且未达到终止条件时，继续增加搜索深度
        while (!timeUp && !itSTOP) best = alpha_beta(++depth);

        // 返回找到的最佳移动
        return best;
    }

    // 打印调试信息
    void printDebug() {
        cout << int(depth) << " - ";
        root.printDebug();
    }

    // 打印最佳路径
    void printBest() {
        cout << endl;
        storedNode* p = &root;
        while (true) {
            p->print();
            if (p->num == 0 || p->num == 0xFFFF) break;
            stable_sort(p->chld, p->chld + p->num);
            p = &(p->chld[0]);
        }
    }
};

// 主函数
int main() {
    // 关闭同步以提高I/O效率
    istream::sync_with_stdio(false);

    // 创建博弈树
    storedTree T;

    // 执行迭代加深搜索找到最佳移动
    moveStep ans = T.iteration();

    // 输出最佳移动
    cout << ans << endl;

    // 注释掉调试输出
    // T.printDebug();
    // T.printBest();

    return 0;
}
