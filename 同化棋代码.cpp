// �����Ҫ��ͷ�ļ�
#include <algorithm>  // �ṩ������㷨����
#include <cstring>    // �ṩ�ڴ����������memcpy��memset��
#include <ctime>      // �ṩʱ����غ���
#include <iostream>   // �ṩ�������������

// ������������ʹ�õĳ���
#define MAX1 100      // ���������ֵ
#define MAX0 50       // �ϴ�������ֵ
#define MIN1 -100     // ��С������ֵ
#define MIN0 -50      // ��С������ֵ
#define limitT 0.9    // ����ʱ������(��)
#define detectN 3     // ���ʱ�����ȼ���
#define initDepth 4   // ��ʼ�������

using namespace std;

// �����������飬���ڿ����޸�����״̬
static uint8_t mask[4] = { 0x15, 0, 0, 0x15 };
// ������ʾ�ַ���#��ʾ�ո�b��ʾ���ӣ�w��ʾ����
static char code[4] = { '#', '#', 'b', 'w' };
// 24�������ƫ���������ڼ�����ܵ��ƶ�λ��
// ǰ8��������λ�ã���16������Ծλ��
static int8_t delta[24][2] = {
        {1, 1},  {0, 1},   {-1, 1},  {-1, 0},  {-1, -1}, {0, -1}, {1, -1}, {1, 0},
        {2, 0},  {2, 1},   {2, 2},   {1, 2},   {0, 2},   {-1, 2}, {-2, 2}, {-2, 1},
        {-2, 0}, {-2, -1}, {-2, -2}, {-1, -2}, {0, -2},  {1, -2}, {2, -2}, {2, -1} };

// ȫ��״̬����
static bool timeUp = false;  // ʱ���Ƿ�����ı�־
static bool itSTOP = false;  // �����Ƿ�Ӧ��ֹͣ�ı�־
static clock_t clk0;         // ��ʼʱ���ʱ�Ӽ�¼

// ����һ������ƶ��ṹ��
struct moveStep {
    int8_t startX, startY;  // ��ʼλ������
    int8_t endX, endY;      // Ŀ��λ������
};

// ������������������ڶ�ȡmoveStep�ṹ��
istream& operator>>(istream& is, moveStep& m) {
    int startX, startY, endX, endY;
    is >> startX >> startY >> endX >> endY;
    m.startX = startX;
    m.startY = startY;
    m.endX = endX;
    m.endY = endY;
    return is;
}

// ���������������������moveStep�ṹ��
ostream& operator<<(ostream& os, moveStep& m) {
    os << int(m.startX) << ' ';
    os << int(m.startY) << ' ';
    os << int(m.endX) << ' ';
    os << int(m.endY);
    return os;
}

// ǰ������
class storedNode;
class storedTree;

// �����࣬��ʾ��Ϸ��״̬
class chessBoard {
    friend class storedNode;  // ������Ԫ�࣬ʹstoredNode���Է���˽�г�Ա
    friend class storedTree;  // ������Ԫ�࣬ʹstoredTree���Է���˽�г�Ա
    // �����һ�У�ʹoperator<�ܹ�����storedVal
    friend bool operator<(const storedNode& N1, const storedNode& N2);

    // ������Ϣ�洢��0/1-�ո� 2-���� 3-����
    // ʹ��λ�����Ч�洢7x7����
    uint16_t info[7];  // ÿ����16λ������ʾ��ÿ2λ��ʾһ��λ�õ�״̬
    int8_t currColor;  // ��ǰ�ж�����ɫ��2-��ɫ��3-��ɫ
    int8_t storedVal;  // �洢��ǰ���̵�����ֵ

public:
    // Ĭ�Ϲ��캯��
    chessBoard() {}

    // ���������������̣�������Ϸ��ʼ��
    chessBoard(istream& is) {
        // ��ʼ������Ϊ��
        memset(info, 0, 7 * sizeof(uint16_t));
        // ���ó�ʼ�ڰ����������̽���
        info[0] = 0xC008;  // |B|W| �ڵ�һ�з��úڰ�����
        info[6] = 0x800C;  // |W|B| �����һ�з��ð׺�����
        currColor = 2;     // �ڷ�����

        // ��ȡ��ǰ�غ���
        int turnID;
        is >> turnID;
        moveStep tmp;

        // �ط�֮ǰ�������ƶ������Ի�ԭ��ǰ����
        for (int i = 1; i != turnID; ++i) {
            // ��ȡ�з��ƶ�
            is >> tmp;
            if (tmp.endX >= 0) procStep(tmp);  // �����Ч����
            // ��ȡ�ҷ��ƶ�
            is >> tmp;
            procStep(tmp);  // �����ҷ��ƶ�
        }

        // ��ȡ��ǰ�غϵз����ƶ�
        is >> tmp;
        if (tmp.endX >= 0) procStep(tmp);
    }

    // �������캯��
    chessBoard(const chessBoard& B) {
        memcpy(info, B.info, 7 * sizeof(uint16_t));
        currColor = B.currColor;
        storedVal = B.storedVal;
    }

    // ��������Ƿ���������
    bool inMap(int8_t x, int8_t y) {
        return x >= 0 && x <= 6 && y >= 0 && y <= 6;
    }

    // ��ȡָ��λ�õ�����״̬
    int8_t pos(int8_t x, int8_t y) { return (info[x] >> ((y + 1) << 1)) & 3; }

    // ���ָ��λ���Ƿ��е�ǰ��ҵ�����
    bool avail(int8_t x, int8_t y) {
        return ((info[x] >> ((y + 1) << 1)) & 3) == currColor;
    }

    // ���ָ��λ���Ƿ�Ϊ��
    bool empty(int8_t x, int8_t y) { return !((info[x] >> ((y + 1) << 1)) & 2); }

    // ����һ���壬��������״̬
    void procStep(const moveStep& m) {
        int8_t startX = m.startX, startY = m.startY, endX = m.endX, endY = m.endY;

        // �������Ծ�ƶ��������ʼλ�õ�����
        if (abs(endX - startX) == 2 || abs(endY - startY) == 2)
            info[startX] ^= (2 << ((startY + 1) << 1));

        // ���Ŀ��λ�õľ�״̬������������
        info[endX] &= ~(3 << ((endY + 1) << 1));
        info[endX] |= (currColor << ((endY + 1) << 1));

        // ����Ŀ��λ����Χ��״̬����Ϸ������أ�
        for (int8_t i = -1; i <= 1; ++i)
            if (endX + i >= 0 && endX + i <= 6) {
                info[endX + i] &= ~(mask[0] << (endY << 1));
                info[endX + i] |= (mask[currColor] << (endY << 1));
            }

        // �л���ǰ��ң�2 -> 3 �� 3 -> 2��
        currColor = 5 - currColor;
    }

    // ���㵱ǰ���̵�����ֵ
    void val() {
        int8_t sum[4] = { 0 };  // ͳ�Ƹ�����������

        // �����������̼��������������
        for (uint8_t i = 0; i != 7; ++i) {
            uint16_t line = info[i];
            for (uint8_t j = 0; j != 7; ++j) ++sum[(line >> ((j + 1) << 1)) & 3];
        }

        // ����ֵΪ��ǰ�����������ȥ����������
        storedVal = sum[currColor] - sum[5 - currColor];
    }

    // �վ���������һ���޷��ƶ�ʱ����ʤ��
    void end(int8_t level) {
        int8_t sum[4] = { 0 };

        // ͳ�Ƹ�����������
        for (uint8_t i = 0; i != 7; ++i) {
            uint16_t line = info[i];
            for (uint8_t j = 0; j != 7; ++j) ++sum[(line >> ((j + 1) << 1)) & 3];
        }

        // �����ǰ������������ڵ���24���ж�Ϊʧ�ܣ�����Ϊʤ��
        // level�������ֲ�ͬ��ȵ��վ�״̬
        storedVal = sum[currColor] <= 24 ? MIN1 + level : MAX1 + level;
    }

    // ��ӡ��ǰ����״̬
    void print() {
        for (int8_t i = 0; i != 7; ++i) {
            for (int8_t j = 0; j != 7; ++j) cout << code[pos(i, j)];
            cout << endl;
        }
        cout << int(storedVal) << endl;
    }
};

// �������ڵ��࣬�̳���������
class storedNode : public chessBoard {
    friend class storedTree;  // ��Ԫ�࣬�ɷ���˽�г�Ա

    moveStep from;     // ����˽ڵ���ƶ�����
    uint16_t num;      // �ӽڵ�����
    storedNode* chld;  // ָ���ӽڵ������ָ��

public:
    // Ĭ�Ϲ��캯��
    storedNode() : chessBoard(), num(0xFFFF), chld(NULL) {}

    // �����̹���ڵ�
    storedNode(const chessBoard& B) : chessBoard(B) {
        num = 0xFFFF;  // 0xFFFF��ʾ��δ�����ӽڵ�
        chld = NULL;   // ��ʼ�ӽڵ�ָ��Ϊ��
    }

    // �ݹ�ɾ���ڵ㼰�������ӽڵ�
    void deleteNode() {
        if (num == 0xFFFF) return;  // ���δչ���ӽڵ���ֱ�ӷ���

        // �ݹ�ɾ�������ӽڵ�
        for (uint16_t i = 0; i != num; ++i) chld[i].deleteNode();

        // ɾ���ӽڵ�����
        if (!chld) delete[] chld;
    }

    // �������п��е��ƶ��������ӽڵ�
    void calcAvail(bool dsort) {
        // �����δ�����ӽڵ�
        if (!(uint16_t(~num))) {
            moveStep av[600];  // �洢���п��ܵ��ƶ�
            num = 0;           // �����ӽڵ������

            // ��������Ѱ�����п��ܵ��ƶ�
            for (int8_t x0 = 0; x0 != 7; ++x0)
                for (int8_t y0 = 0; y0 != 7; ++y0)
                    if (empty(x0, y0)) {  // ���Ŀ��λ��Ϊ��
                        uint8_t i;

                        // �ȼ��8�����ڷ�����ƶ�
                        for (i = 0; i != 8; ++i) {
                            int8_t x1 = x0 + delta[i][0], y1 = y0 + delta[i][1];
                            if (!inMap(x1, y1)) continue;  // �����������������
                            if (!avail(x1, y1)) continue;  // ������ǵ�ǰ��ҵ�����������

                            // �����ƶ����貢��ӵ�����
                            moveStep tmp = { x1, y1, x0, y0 };
                            av[num++] = tmp;
                            break;  // �ҵ�һ�����ڵ��ƶ���Ͳ��ټ���������ڷ���
                        }

                        // �ټ��16����Ծ������ƶ�
                        for (i = 8; i != 24; ++i) {
                            int8_t x1 = x0 + delta[i][0], y1 = y0 + delta[i][1];
                            if (!inMap(x1, y1)) continue;  // �����������������
                            if (!avail(x1, y1)) continue;  // ������ǵ�ǰ��ҵ�����������

                            // �����ƶ����貢��ӵ�����
                            moveStep tmp = { x1, y1, x0, y0 };
                            av[num++] = tmp;
                        }
                    }

            // �����ӽڵ������ڴ�
            chld = new storedNode[num];

            // Ϊÿ���ӽڵ�����״̬����������
            for (uint16_t i = 0; i != num; ++i) {
                chld[i] = chessBoard(*this);   // ���Ƶ�ǰ����״̬
                chld[i].procStep(av[i]);       // ִ���ƶ�����
                chld[i].from = av[i];          // ��¼�ƶ�����
                chld[i].val();                 // ����������״̬
            }
        }

        // �����Ҫ�����ӽڵ�������򣨱���Alpha-Beta��֦��
        if (dsort) sort(chld, chld + num);
    }

    // ��ȡ����ƶ�������ֵ��С���ӽڵ��Ӧ���ƶ���
    moveStep best() {
        stable_sort(chld, chld + num);  // �ȶ�����ȷ����ͬ����ֵ���ƶ�����˳��
        return chld[0].from;           // ��������ֵ��С���ӽڵ��Ӧ���ƶ�
    }

    // ��ӡ�����ӽڵ����Ϣ�������ã�
    void printAll() {
        for (uint16_t i = 0; i != num; ++i) {
            cout << chld[i].from << endl;
            chld[i].print();
            cout << int(chld[i].storedVal) << endl;
            cout << endl;
        }
    }

    // ��ӡ������Ϣ
    void printDebug() {
        cout << num << " :: ";
        for (uint16_t i = 0; i != num; ++i)
            cout << chld[i].from << " : " << int(chld[i].storedVal) << " / ";
    }
};

// ���رȽ����������������ڵ�
// ����ֵԽС�Ľڵ�Խ"С"��������������ǰ��
bool operator<(const storedNode& N1, const storedNode& N2) {
    return N1.storedVal < N2.storedVal;
}

// �������࣬����������������
class storedTree {
    storedNode root;  // ���ڵ�
    int8_t depth;     // ��ǰ�������

public:
    // ���캯�����ӱ�׼�����ȡ��ʼ����״̬
    storedTree() {
        chessBoard B(cin);
        root = storedNode(B);
    }

    // ������������ע�͵���
    // ~storedTree() { root.deleteNode(); }

    // Alpha-Beta�����������
    // N: ��ǰ�ڵ㣬n: ʣ����ȣ�x: Alphaֵ
    void abDFS(storedNode& N, int8_t n, int8_t& x) {
        if (n == 0)
            // �ѵ�������������ƣ�ֱ�������ڵ�ֵ
            N.val();
        else if (!(uint16_t(~N.num)) ||
            (N.storedVal >= MIN0 && N.storedVal <= MAX0)) {
            // ����ڵ���δչ����������ֵ��һ����Χ��
            N.storedVal = MIN1;  // ��ʼ��Ϊ��Сֵ

            // �������п����ƶ�
            N.calcAvail(n != depth);

            // ����޿����ƶ��������վ�״̬
            if (N.num == 0) N.end(depth - n);

            uint16_t i;
            // ���������ӽڵ㣬����Alpha-Beta��֦
            for (i = 0; i != N.num; ++i) {
                // �ݹ�����ӽڵ������ֵ
                abDFS(N.chld[i], n - 1, N.storedVal);

                // Alpha-Beta��֦�������ǰ�ڵ��ֵ�Ѿ����ڸ��ڵ��Betaֵ�����ж�����
                if (N.storedVal > -x) {
                    ++i;
                    break;
                }

                // ���ʱ�����꣬�ж�����
                if (timeUp) break;
            }

            // ��δ�������ӽڵ������ֵ��Ϊ���
            for (; i < N.num; ++i) N.chld[i].storedVal = MAX0;
        }

        // ���ض���ȼ���Ƿ�ʱ������
        if (n == detectN)
            timeUp = (clock() - clk0) / double(CLOCKS_PER_SEC) > limitT;

        // ����Alphaֵ
        if (x < -N.storedVal) x = -N.storedVal;
    }

    // ִ��һ��Alpha-Beta��������������ƶ�
    moveStep alpha_beta(int8_t n) {
        // ������ڵ�����п����ƶ�
        root.calcAvail(true);

        // ��ʼAlphaֵΪ��С��ֵ
        int8_t ans = MIN1;

        // ִ��Alpha-Beta����
        abDFS(root, n, ans);

        // ������ڵ������ֵ������ĳ����Χ�����Ϊ����ֹͣ����
        if (root.storedVal <= MIN0 || root.storedVal >= MAX0) itSTOP = true;

        // ��������ƶ�
        return root.best();
    }

    // ����������������ʱ�������ڲ��������������
    moveStep iteration() {
        // ��¼��ʼʱ��
        clk0 = clock();

        // ���ó�ʼ�������
        depth = initDepth;

        // ִ�е�һ������
        moveStep best = alpha_beta(depth);

        // ��ʱ��δ������δ�ﵽ��ֹ����ʱ�����������������
        while (!timeUp && !itSTOP) best = alpha_beta(++depth);

        // �����ҵ�������ƶ�
        return best;
    }

    // ��ӡ������Ϣ
    void printDebug() {
        cout << int(depth) << " - ";
        root.printDebug();
    }

    // ��ӡ���·��
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

// ������
int main() {
    // �ر�ͬ�������I/OЧ��
    istream::sync_with_stdio(false);

    // ����������
    storedTree T;

    // ִ�е������������ҵ�����ƶ�
    moveStep ans = T.iteration();

    // �������ƶ�
    cout << ans << endl;

    // ע�͵��������
    // T.printDebug();
    // T.printBest();

    return 0;
}
