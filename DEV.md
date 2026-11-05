# 植物战棋开发指南

一份从零到可玩、可长期迭代的完整开发路线。

------

## 一、项目定位

**一句话**：酒馆战棋的完整机制 + 植物主题换皮。

**核心原则**：

- 机制照搬酒馆战棋，不重新设计平衡
- 只换主题、名字、数据
- 先跑通战斗，再做循环，最后做内容
- 所有内容数据驱动，加植物不改代码

**不做的东西**（至少前期不做）：

- 站位、前后排、网格
- 联网
- 3D
- 复杂动画

------

## 二、技术选型

| 部分      | 选择            | 理由                   |
| :-------- | :-------------- | :--------------------- |
| 语言      | C++17           | 稳定、够用、编译快     |
| 构建      | CMake           | 跨平台、生态好         |
| 窗口/渲染 | raylib          | 2D 快速、API 简单      |
| UI        | Dear ImGui      | 调试 + 游戏 UI 都能用  |
| 数据      | nlohmann/json   | 单头文件、好用         |
| 随机      | std::mt19937    | 固定种子可回放         |
| 日志      | spdlog 或自己写 | 战斗日志是核心调试工具 |
| 版本控制  | Git             | 从第一天就用           |

**目录结构**：

text

```
plant-battlegrounds/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── core/           # 战斗、事件、随机
│   ├── data/           # 数据加载、模板
│   ├── game/           # 花园循环、经济、AI
│   ├── ui/             # 渲染、ImGui
│   └── utils/          # 日志、工具
├── data/
│   ├── plants.json
│   ├── gardeners.json
│   └── keywords.json
├── assets/
└── tests/
```



------

## 三、核心架构

### 1. 数据层

所有内容用模板定义，运行时实例化。

cpp

```
// 植物模板（从 JSON 加载，不变）
struct PlantTemplate {
    std::string id;
    std::string name;
    int tier;
    int attack, health;
    std::string race;
    std::vector<Keyword> keywords;
    std::vector<Effect> battlecry;   // 播种
    std::vector<Effect> deathrattle; // 凋零
    std::vector<Effect> aura;        // 光合
    PlantTemplate golden;            // 金色版本
};

// 植物实例（运行时，会变）
struct Plant {
    int templateId;
    int attack, health;
    int maxHealth;
    bool hasThorns = false;
    bool hasShield = false;
    bool hasFastGrow = false;
    bool hasPoison = false;
    bool hasReborn = false;
    int goldenLevel = 0;
    std::vector<Effect> temporaryEffects;  // 临时增益
};
```



**关键原则**：模板只读，实例可变。战斗时复制实例，不影响花园状态。

### 2. 事件系统（心脏）

战斗结算必须事件驱动，不能用简单循环。

cpp

```
enum class EventType {
    Attack, Damage, Death, Deathrattle, Battlecry,
    Summon, Buff, ShieldPop, Reborn, PhaseStart, PhaseEnd
};

struct GameEvent {
    EventType type;
    int sourceId;
    int targetId;
    int value;
    int priority;
};

class EventQueue {
public:
    void push(GameEvent e);
    void resolve();  // 循环处理直到空
private:
    std::deque<GameEvent> queue;
};
```



**为什么**：亡语召唤新植物 → 新植物有战吼 → 战吼造成伤害 → 杀死另一个植物 → 触发亡语……必须顺序化。

### 3. 关键词系统（策略模式）

每个关键词独立结算，不要 if-else 堆。

cpp

```
class KeywordHandler {
public:
    virtual void onAttack(Plant& self, Plant& target, EventQueue& q) = 0;
    virtual void onDamaged(Plant& self, int dmg, EventQueue& q) = 0;
    virtual void onDeath(Plant& self, EventQueue& q) = 0;
    virtual void onSummon(Plant& self, EventQueue& q) = 0;
};

class ThornsHandler : public KeywordHandler { ... };      // 嘲讽
class ShieldHandler : public KeywordHandler { ... };      // 圣盾
class PoisonHandler : public KeywordHandler { ... };      // 剧毒
class RebornHandler : public KeywordHandler { ... };      // 复生
```



### 4. 战斗结算

cpp

```
class CombatResolver {
public:
    CombatResult resolve(CombatSide& left, CombatSide& right) {
        // 1. 深拷贝双方植物
        // 2. 交替攻击循环
        // 3. 每次攻击：选攻击者 → 选目标 → 伤害 → 事件队列
        // 4. 死亡统一处理（标记待死，结算完再移除）
        // 5. 检查一方全灭
        // 6. 返回结果
    }
};
```



**伤害结算固定顺序**：

text

```
攻击 → 伤害 → 荆棘反伤 → 毒刺附加 → 死亡检查 → 凋零 → 播种 → 再生
```



### 5. 花园循环

cpp

```
class GardenState {
    int sunlight = 3;       // 金币
    int gardenTier = 1;     // 酒馆等级
    int health = 40;
    int shield = 0;
    std::vector<Plant> board;   // 一排 7 个
    std::vector<Plant> hand;
    std::vector<Plant> nursery; // 商店
    GardenerID gardener;
};

class GardenLoop {
    void startTurn();
    void refresh();          // 浇水
    void buy(int index);     // 种植
    void sell(int index);    // 铲除
    void freeze();           // 保鲜
    void upgradeTier();      // 升级花园
    void combine();          // 光合融合
    void endTurn();
};
```



### 6. 随机与回放

cpp

```
class RNG {
    std::mt19937 engine;
public:
    explicit RNG(uint32_t seed) : engine(seed) {}
    int range(int lo, int hi);
    template<typename T> T choice(const std::vector<T>& v);
};
```



**固定种子** = 战斗可重放、可调试、可做回放功能。

------

## 四、数据格式

### plants.json

json

```
{
  "plants": [
    {
      "id": "sunflower",
      "name": "向日葵",
      "tier": 1,
      "attack": 1,
      "health": 3,
      "race": "flower",
      "keywords": [],
      "battlecry": [
        { "type": "gain_sunlight", "value": 1 }
      ],
      "golden": {
        "attack": 2,
        "health": 6,
        "battlecry": [
          { "type": "gain_sunlight", "value": 2 }
        ]
      }
    }
  ]
}
```



### keywords.json

json

```
{
  "keywords": [
    { "id": "thorns", "name": "荆棘", "description": "必须被优先攻击" },
    { "id": "shield", "name": "护盾", "description": "免疫一次伤害" },
    { "id": "fast_grow", "name": "速生", "description": "每回合攻击两次" },
    { "id": "poison", "name": "毒刺", "description": "击杀目标" },
    { "id": "reborn", "name": "再生", "description": "死亡后复活一次" },
    { "id": "deathrattle", "name": "凋零", "description": "死亡时触发" },
    { "id": "battlecry", "name": "播种", "description": "打出时触发" },
    { "id": "aura", "name": "光合", "description": "持续增益" }
  ]
}
```



------

## 五、开发阶段

### 第一阶段：核心战斗（1-2 周）

**目标**：控制台里两个固定花园自动战斗，输出日志。

任务：

1. 搭 CMake 项目
2. 定义 `Plant`、`CombatSide`
3. 写 `EventQueue`
4. 写 `CombatResolver`
5. 实现基础关键词：荆棘、护盾、速生、毒刺
6. 输出战斗日志

**验收**：

text

```
[攻击] 仙人掌(2/3) -> 向日葵(1/2)
[伤害] 向日葵 受到 2 点伤害，剩余 0
[死亡] 向日葵 死亡
[凋零] 向日葵 触发：给相邻植物+1/+1
[增益] 玫瑰 获得 +1/+1
[结果] 左侧胜利，存活 2 个植物
```



### 第二阶段：花园循环（2-4 周）

任务：

1. 阳光、浇水、种植、铲除
2. 花园等级、升级费用
3. 光合融合（三连）
4. 保鲜（冻结）
5. 简单 AI 对手
6. 回合流程

**验收**：能完整打完一局，从 1 级花园到分出胜负。

### 第三阶段：关键词完整化（1-2 月）

任务：

1. 凋零、播种、再生、光合
2. 关键词交互（护盾 vs 毒刺、再生 vs 毒刺）
3. 金色植物特殊效果
4. 光环实时计算

### 第四阶段：内容扩张

任务：

1. 园丁技能
2. 种族系统（花卉、藤蔓、菌类、树木、荆棘、水生、食虫）
3. 30+ 植物
4. raylib 渲染 + ImGui UI
5. 动画、音效

### 第五阶段：长期

- 赛季机制
- 新关键词
- 平衡调整
- 每日挑战
- 可能的联网

------

## 六、必须提前想清楚的坑

1. **战斗必须深拷贝**：战斗不能影响花园里的植物，否则三连、buff 会乱。
2. **光环实时计算**：光合不要直接改数值，用 `getEffectiveAttack()` 叠加。
3. **死亡延迟处理**：血量 ≤ 0 标记为待死，当前事件结算完再统一移除。否则会出现“已死还在攻击”。
4. **递归深度限制**：播种召唤的幼苗如果也有播种，可能无限递归。限制深度或标记“本回合已触发”。
5. **固定随机种子**：战斗可重放、可调试。
6. **伤害来源区分**：毒刺、护盾、凋零伤害要区分来源，否则关键词交互会错。
7. **金色植物**：不是简单数值翻倍，很多是效果翻倍，要单独处理。
8. **事件优先级**：同一时机多个触发，要有明确优先级，否则结果不确定。

------

## 七、调试工具（越早做越好）

1. **战斗日志**：每个事件都打印，带缩进表示嵌套
2. **ImGui 状态查看器**：实时看双方花园、手牌、阳光
3. **时间加速**：跳过动画，直接看结果
4. **手动触发**：调试时手动触发某个关键词
5. **种子输入**：输入种子重现某场战斗
6. **热重载 JSON**：改数据不重启

------

## 八、第一周具体任务清单

**Day 1-2：项目搭建**

- CMake 项目

- raylib + ImGui + nlohmann/json 集成

- 目录结构

- Git 初始化

**Day 3-4：数据层**

- `PlantTemplate`、`Plant`

- JSON 加载

- 一个测试植物

**Day 5-7：战斗核心**

- `EventQueue`

- `CombatResolver`

- 荆棘、护盾、速生、毒刺

- 控制台战斗日志

- 两个固定花园自动战斗

**周末验收**：跑起来，看到植物互相打，日志完整。

------

## 九、长期迭代原则

1. **数据驱动**：所有内容从 JSON 加载
2. **系统分离**：渲染、逻辑、输入、音频独立
3. **存档版本化**：存档带版本号
4. **调试工具早做**：控制台、时间加速、状态查看
5. **内容管线**：改数值不重新编译
6. **组件化**：别用继承堆，用组合
7. **固定种子**：所有随机可复现
8. **测试**：关键词交互写单元测试