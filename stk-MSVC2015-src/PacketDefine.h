#ifndef PACKETDEFINE_H
#define PACKETDEFINE_H
#pragma once
#include <QString>
#include "PacketHandlerClass.h"
const QString ControlChannelFree_color = " rgb(0, 85, 0)";
const QString ServiceChannelFree_color = " rgb(0, 62, 188)";
const QString OccupiedChannel_color = " rgb(255, 170, 0)";
#pragma pack(push, 1)   // 1字节对齐 - 不默认插入填充字节

#define HEAD_ID_MATCH    0x22        // 用于与 HEAD_ID 匹配
#define TAIL_ID_MATCH    0x23        // 用于与 TAIL_ID 匹配
#define QT_IDENTITY_MATCH   0x78     // 用于与 QT_IDENTITY 匹配
//#define TYPE_StringMsg_MATCH  0x55         // 控制台发消息收到的回复，直接打印输出，不用进队列定时弹出

// 用于与 PRIORITY 匹配
#define PRI1             0x01        // 第一优先级(最低)
#define PRI2             0x02        // 第二优先级
#define PRI3             0x03        // 第三优先级
#define PRI4             0x04        // 第四优先级
#define PRI5             0x05        // 第五优先级(最高)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef u8 u48[6];


// 内部包的类型
typedef enum
{
    TYPE_TF    = 0x40,  // 时频资源包类型
    TYPE_EVENT = 0x41,  // 事件包类型
    TYPE_MIB   = 0x42,  // 宽波束覆盖
    TYPE_RAREQ = 0x43,  // 用户随机接入请求
    TYPE_RARSP = 0x44,  // 卫星回复包括分配的基带板号
    TYPE_StringMsg_MATCH = 0x55,         // 控制台发消息收到的回复，直接打印输出，不用进队列定时弹出
    TYPE_UEDISCON = 0x46// 用户离线通知
} DATA_TYPE;

// 外部大包 - 头部
typedef struct
{
    u8  HEAD_ID;
    u8  QT_IDENTITY;
    u16 LEN_EXTER;       // LEN_EXTER 为 DATA_EXTER 数组的长度
    u8  DATA_EXTER[0];   // 零长度数组，不占结构体大小，仅用于允许结构体访问尾部连续内存
} MESSAGE_HEAD_EXTER;

// 外部大包 - 尾部
typedef struct
{
    u8 TAIL_ID;
} MESSAGE_TAIL_EXTER;

// 内部包 - 也就是大包中的 DATA_EXTER[LEN_EXTER]
typedef struct
{
    u8  DATA_TYPE;
    u8  PRIORITY;
    u8  TIME[13];    // 收到的会是字符串，包括‘/0’长度为13，aa:bb:cc.ddd 对应为 时:分:秒.毫秒
    u16 LEN_INTER;      // 有效数据数组 DATA_INTER 的长度
    u8  DATA_INTER[0];
} MESSAGE_INTER;

// 有效数据数组 DATA_INTER 定义如下
// 1、时间频率资源包 Time-frequency resources
typedef struct
{
    u8 DATA[8][10][4];
} TF_MESSAGE;

// 2、宽波束覆盖，卫星广播MIB包时发送
typedef struct
{
    u8 SAT_ID;
} MIB;

// 3、随机接入请求，卫星收到用户发起接入请求时发送
typedef struct
{
    u8 SAT_ID;
    u48 UE_ADDRESS;
} RAREQ;

// 4、卫星分配跳波束波束号时发送
typedef struct
{
    u8 SAT_ID;
    u48 UE_ADDRESS;
    u16 Beam_num;   // 基带板
} RARSP;

// 5、日志直接打印输出
typedef struct
{
    u8 StirngData[0];
} NewStringMsg;

// 定义统一的传给包处理器的 事件结构体
typedef struct
{
    u8 priority;
    u8 dataType;
    u32 time;    // 记录与前一个数据包的组包时间间隔，单位ms
    QByteArray data;    // QByteArray 可以自动管理大小和内存
} EVENT;

// 定义 StringMsg 结构体
struct StringMsg
{
    u8 HEAD_ID;
    u8 TYPE_StringMsg;
    u16 StrLen;
    u8 data[];
    //u8 TAIL_ID;
};

// 定义用户离线 UEDISCON 结构体
typedef struct
{
    u8 SAT_ID;
    u48 UE_ADDRESS;
    u16 Beam_num;   // 基带板
} UEDISCON;

//// 时间频率资源包 Time-frequency
//typedef struct {
//    u8 HEAD_ID;
//    u8 MESSAGE_TYPE;
//    u16 LEN;
//    u8 DATA[8][10][4];
//    u8 TILE_ID;
//}TF_MESSAGE;


#pragma pack(pop)
#endif // PACKETDEFINE_H
