# Assimilation-Chess-AI-
本项目为同化棋，在Botzone天梯榜能排前100名。基于Alpha-Beta剪枝算法与迭代加深搜索框架，针对同化棋的规则特性进行深度优化。通过位运算高效模拟棋盘状态（每2位存储一个棋子），结合预定义的移动方向掩码（24个方向偏移量）快速生成合法移动。评估函数聚焦“棋子数量差”与“同化效果”，优先选择能最大化己方棋子、削弱对手的策略。在搜索过程中，通过维护Alpha（最大下界）和Beta（最小上界）值剪去无效分支，同时采用迭代加深策略（从初始深度4逐步增加），在0.9秒时间限制内平衡计算效率与决策质量，最终输出最优落子方案。

几个Visio文件是模型图，方便理解，也可以答辩用
