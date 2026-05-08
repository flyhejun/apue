# APUE 代码错误集 — 从错误中学习

> 本文档记录了本次代码审查中发现的所有错误，按类型分类，附带错误原因、正确写法和防错要点。

---

## 一、C 语言基础错误

### 错误 1：数组名不能赋值

**文件**: `client/socket_client.c`

```c
// 错误写法
char host[64];
host = optarg;        // 编译错误：数组名是常量地址，不能被赋值

// 正确写法
char host[64];
strncpy(host, optarg, sizeof(host) - 1);
host[sizeof(host) - 1] = '\0';  // 确保以 null 结尾
```

**原理**: C 语言中数组名是一个常量指针（指向数组首元素的地址），不能出现在赋值号左边。`strncpy` 才是把字符串复制进字符数组的正确方式。

**记忆口诀**: 数组名是"门牌号"，你不能改变门牌号，只能往屋里搬东西。

---

### 错误 2：取地址符 `&` 多余

**文件**: `client/socket_client.c`

```c
// 错误写法
char time[64];
get_time(&(data.time), sizeof(data.time));
// &(data.time) 的类型是 char(*)[64]，即"指向数组的指针"
// 而函数期望的是 char*，即"指向字符的指针"

// 正确写法
get_time(data.time, sizeof(data.time));
// data.time 会自动退化为 char*（数组到指针的退化）
```

**原理**: C 语言中 `char arr[64]` 在传参时会自动退化为 `char*`。加了 `&` 反而类型不匹配。

**类型对比**:
```
data.time        → char*     (指向首字符的指针)  ✓
&(data.time)     → char(*)[64]  (指向整个数组的指针)  ✗ 类型不匹配
```

---

### 错误 3：`continue` 在循环外使用

**文件**: `client/socket_client.c`

```c
// 错误写法
if(temporary_repo(&db) < 0)
{
    continue;    // 编译错误：这里不在任何循环内部
}

// 正确写法
if(temporary_repo(&db) < 0)
{
    log_error("初始化本地数据库失败");
    return -1;   // 初始化失败应直接退出
}
```

**原理**: `continue` 只能在 `for`/`while`/`do-while` 循环体内使用。在循环外使用是语法错误。

**记忆口诀**: `continue` 和 `break` 只在循环/switch 中出现，函数内用 `return`。

---

### 错误 4：结构体用指针运算符 `->` 访问非指针变量

**文件**: `client/socket_client.c`

```c
// 错误写法
socket_t sock;        // sock 是结构体变量，不是指针
write(sock->fd, ...); // 错误：-> 只能用于指针

// 正确写法
socket_t sock;
write(sock.fd, ...);  // 用 . 访问结构体成员
```

**原理**:
```
结构体变量  → 用 .   (点号)    例: sock.fd
结构体指针  → 用 -> (箭头)    例: sock_ptr->fd
```

**记忆口诀**: 点号给变量，箭头给指针。变量用点，指针用箭。

---

## 二、变量未定义 / 拼写错误

### 错误 5：使用未定义的变量名

**文件**: `client/socket.c`

```c
// 错误写法 — addr 从未定义
int addr_len = sizeof(addr);   // addr 是什么？编译器不知道

// 错误写法 — domain_name 从未定义
getaddrinfo(domain_name, ...); // 应该用 sock->serv_host

// 错误写法 — servip 从未定义
if(!servip || !port)           // 应该用 host
```

**正确写法**:
```c
// 直接使用 p->ai_addrlen，不需要额外变量
rs = connect(sockfd, p->ai_addr, p->ai_addrlen);

// 使用正确的变量名
getaddrinfo(sock->serv_host, port_buf, &hints, &result);
if(!host[0] || !port)
```

**防错要点**: 写完代码后，全局搜索每个变量名，确认它在当前作用域内有定义。编译器报 "undeclared identifier" 时，先检查拼写，再检查作用域。

---

### 错误 6：函数名拼写错误

**文件**: `common/packet.c`

```c
// 错误写法
cJSON_AddStringiToObject(root, "ID", data->id);
//                           ^ 多了一个 i

// 正确写法
cJSON_AddStringToObject(root, "ID", data->id);
```

**防错要点**: 调用第三方库函数时，不要凭记忆手写，应该：
1. 查头文件中的声明
2. 使用 IDE 的自动补全
3. 编译报错时，对比头文件确认函数名

---

## 三、头文件错误

### 错误 7：`#include` 放在 `#ifndef` 保护之外

**文件**: `common/packet.h`

```c
// 错误写法
#include <stddef.h>    // ← 在 #ifndef 之外
#include <stdlib.h>

#ifndef PACKET_H       // ← 头文件保护从这里才开始
#define PACKET_H
...

// 正确写法
#ifndef PACKET_H       // ← 头文件保护从第一行开始
#define PACKET_H

#include <stddef.h>    // ← 在 #ifndef 内部
#include <stdlib.h>
...
#endif
```

**原理**: 如果 `#include` 在 `#ifndef` 外面，当多个文件包含此头文件时，这些标准库头文件会被重复包含，可能引发重定义错误。

**记忆口诀**: 头文件保护是城墙，所有内容都在城墙内。

---

### 错误 8：使用了类型但未包含定义该类型的头文件

**文件**: `common/database.h`

```c
// 错误写法 — 用了 sqlite3* 类型但没 include
extern int temp_data_in(sqlite3 *db, char *json_buf);
extern sqlite3_stmt* data_exist(sqlite3 *db);

// 正确写法 — 在声明前包含头文件
#include <sqlite3.h>
extern int temp_data_in(sqlite3 *db, char *json_buf);
```

**防错要点**: 头文件中用到的每个类型，都要确保有对应的 `#include`。不要依赖 `.c` 文件中的 include 来"补救"——头文件应该能独立编译。

---

### 错误 9：头文件保护名不匹配

**文件**: `server/socket.h`

```c
// 错误写法 — 这是 server 的 socket.h，但保护名用了 CLI
#ifndef SOCKET_CLI_H    // ← 这是 client 的名字
#define SOCKET_CLI_H

// 正确写法
#ifndef SOCKET_SOCK_H
#define SOCKET_SOCK_H
```

**防错要点**: 头文件保护名应与文件名对应。推荐用 `文件名_H` 的格式，例如 `socket.h` → `SOCKET_H`。

---

## 四、函数签名不匹配

### 错误 10：函数声明与实现参数列表不同

**文件**: `common/packet.h` vs `common/packet.c`

```c
// 头文件中的声明
int date_packet(char *time, double *temperature, char *buf, size_t buf_len);
//               ^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^ 4个参数

// 实际实现
int date_packet(data_t *data, char *buf, size_t buf_len);
//               ^^^^^^^^^^^  3个参数
```

**后果**: 调用者按声明传 4 个参数，实现只收 3 个 → 栈被破坏 → 未定义行为（崩溃、数据错乱）。

**防错要点**: 修改函数签名后，必须同步更新 `.h` 和 `.c` 两处。编译时开启 `-Wall` 可以捕获部分此类错误。

---

## 五、逻辑 / 运行时错误

### 错误 11：`sizeof` 传了字节数而非元素个数

**文件**: `server/socket_server.c`

```c
// 错误写法
struct epoll_event events[MAX_EVENTS];
socket_epoll(epfd, listen_fd, events, sizeof(events), ep_fds);
//                                     ^^^^^^^^^^^^^^
// sizeof(events) = 12 * 1024 = 12288 字节
// 但 epoll_wait 期望的是元素个数 1024

// 正确写法
socket_epoll(epfd, listen_fd, events, MAX_EVENTS, ep_fds);
```

**原理**: `sizeof(数组)` 返回的是**字节数**，不是元素个数。
```
元素个数 = sizeof(数组) / sizeof(数组[0])
```

**记忆口诀**: sizeof 算的是字节，要个数得除以单个大小。

---

### 错误 12：`while(1)` 循环内放了 `return`

**文件**: `client/socket_client.c`

```c
// 错误写法
while(1)
{
    // ... 业务逻辑 ...
    close(fd1);   // ← 每次循环都关闭？
    return 0;     // ← 第一次就退出了！死循环形同虚设
}

// 正确写法
while(1)
{
    // ... 业务逻辑 ...
}
// 循环结束后的清理（实际上 while(1) 永远不会到这里）
close(sock.fd);
return 0;
```

**防错要点**: `while(1)` 内的 `return` 会导致立即退出。如果你的循环只执行了一次就结束，检查是不是有 `return` 或 `break` 误放。

---

### 错误 13：`getopt_long` 格式串参数格式错误

**文件**: `client/socket_client.c`

```c
// 错误写法 — 'd' 后面缺冒号，表示 d 不接受参数
getopt_long(argc, argv, "i:p:h:s:d", opts, NULL);
//                                 ^ 缺了 :

// 正确写法 — 'd' 需要参数（域名），加冒号
getopt_long(argc, argv, "i:p:hs:d:", opts, NULL);
//                                 ^
```

**getopt 格式串规则**:
```
字符     含义
c        选项 c 不需要参数
c:       选项 c 需要一个参数（必须跟值）
c::      选项 c 可选参数（可跟可不跟）
```

---

## 六、类型安全错误

### 错误 14：`volatile sig_atomic_t` 类型不一致

**文件**: `server/socket_server.h` vs `server/socket_server.c`

```c
// 头文件声明
extern int g_stop;                    // 类型: int

// 实际定义
volatile sig_atomic_t g_stop = 0;     // 类型: volatile sig_atomic_t

// 正确写法 — 声明和定义必须一致
extern volatile sig_atomic_t g_stop;
```

**原理**: 信号处理函数中使用的变量必须用 `volatile sig_atomic_t` 类型，保证在信号中断时的原子性和可见性。声明不一致会导致链接器或运行时行为异常。

---

## 七、环境适配错误

### 错误 15：Makefile 中硬编码绝对路径

**文件**: `client/Makefile`, `server/Makefile`

```makefile
# 错误写法 — 硬编码了服务器路径
CFLAGS := -Wall -g -I/home/iot26/hejunfei/common
LIB := /home/iot26/hejunfei/lib/libmylib.a
TARGET := /home/iot26/hejunfei/bin/cli_prog

# 正确写法 — 使用相对路径
CFLAGS := -Wall -g -I../common
LIB := ../lib/libmylib.a
TARGET := cli_prog
```

**防错要点**: Makefile 中永远使用相对路径或变量，不要写死绝对路径。换台机器就编译不过了。

---

## 八、内存 / 资源泄漏

### 错误 16：`while(1)` 死循环导致资源清理代码不可达

**文件**: `client/socket_client.c`

```c
// 错误写法
while(1)
{
    // ... 业务逻辑 ...
}
close(sock.fd);      // ← 永远执行不到，fd 泄漏
sqlite3_close(db);   // ← 永远执行不到，db 泄漏

// 正确写法 — 用信号控制循环退出
static volatile sig_atomic_t g_running = 1;

static void sig_handler(int signum) { g_running = 0; }

signal(SIGINT, sig_handler);

while(g_running)     // ← 收到 Ctrl+C 后退出
{
    // ...
}
close(sock.fd);      // ← 现在可以执行到了
sqlite3_close(db);
```

**原理**: `while(1)` 是无条件死循环，后面的代码永远不会执行。进程退出时 OS 会回收，但：
- 数据库可能有未 flush 的数据丢失
- socket 的 FIN 包不会发送，对端需要等超时才知道你断了

**记忆口诀**: 死循环要留"出口"，信号处理是钥匙。

---

### 错误 17：函数关闭了不属于自己的资源（越权释放）

**文件**: `common/database.c`

```c
// 错误写法 — data_exist 不拥有 db，不应该关闭它
sqlite3_stmt* data_exist(sqlite3 *db)
{
    ...
    if(stmt == NULL)
    {
        sqlite3_close(db);  // ← BUG: db 是调用者的！
        return NULL;
    }
    return stmt;
}

// 正确写法 — 只管自己的资源
sqlite3_stmt* data_exist(sqlite3 *db)
{
    ...
    if(stmt == NULL)
    {
        return NULL;         // ← db 留给调用者管理
    }
    return stmt;
}
```

**原理**: 资源由谁创建/打开，就由谁负责关闭。这条规则叫 **所有权 (ownership)** 原则。

如果 `data_exist` 关闭了 `db`：
- 调用者后续用 `db` → use-after-close → 未定义行为
- 调用者最后再 `sqlite3_close(db)` → double-close → 崩溃

**所有权速查表**:
```
谁打开        谁关闭           备注
sqlite3_open  sqlite3_close    同一作用域
malloc        free             同一所有权链
fopen         fclose           同一文件句柄
socket()      close()          同一 fd
getaddrinfo() freeaddrinfo()   同一 result
```

---

### 错误 18：函数返回值未检查，失败后继续使用无效资源

**文件**: `server/socket_server.c`

```c
// 错误写法
temporary_repo(&db);           // ← 没检查返回值
log_info("数据库连接成功");     // ← 失败了也打印成功
// ... 后续用 db 操作 → 可能崩溃

// 正确写法
if(temporary_repo(&db) < 0)
{
    log_error("数据库初始化失败");
    close(listen_fd);
    return -4;
}
log_info("数据库连接成功");
```

**防错要点**: 所有返回错误码的函数，调用后必须先判断再使用。特别是涉及资源分配的函数（open/malloc/socket/sqlite3_open）。

---

## 总结：错误分类统计

| 错误类别 | 数量 | 涉及文件 |
|----------|------|----------|
| C 语言基础（数组、指针、控制流） | 4 | socket_client.c |
| 变量未定义 / 拼写 | 2 | socket.c, packet.c |
| 头文件错误 | 3 | packet.h, database.h, socket.h |
| 函数签名不匹配 | 1 | packet.h |
| 逻辑 / 运行时 | 3 | socket_server.c, socket_client.c |
| 类型安全 | 1 | socket_server.h |
| 环境适配 | 1 | 所有 Makefile |
| 内存 / 资源泄漏 | 3 | socket_client.c, database.c, socket_server.c |
| **合计** | **18** | |

---

## 编码检查清单（每次写完代码自查）

### 基础语法
- [ ] 每个变量在使用前都有定义
- [ ] 函数声明（.h）和实现（.c）的签名一致
- [ ] 数组传参用 `strncpy`，不是 `=`
- [ ] 结构体变量用 `.`，结构体指针用 `->`
- [ ] `sizeof` 用于数组时，除以 `sizeof(元素)` 得到个数
- [ ] `while(1)` 内没有意外的 `return`
- [ ] `continue`/`break` 只在循环/switch 内

### 头文件
- [ ] 头文件保护 `#ifndef` 包裹了所有内容
- [ ] 头文件中用到的每个类型都有对应的 `#include`
- [ ] 第三方库函数名从头文件确认，不凭记忆手写

### 资源管理
- [ ] 每个 `open`/`malloc`/`socket`/`sqlite3_open` 都有对应的 `close`/`free`/`sqlite3_close`
- [ ] 错误路径上也要释放已分配的资源
- [ ] 不关闭不属于自己（不拥有所有权）的资源
- [ ] `while(1)` 死循环有信号处理等退出机制
- [ ] 函数返回值（尤其是资源分配函数）必须检查

### 构建
- [ ] Makefile 用相对路径，不用绝对路径
- [ ] 信号变量用 `volatile sig_atomic_t`
