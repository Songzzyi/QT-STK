#ifndef PACKETHANDLERCLASS_H
#define PACKETHANDLERCLASS_H
#pragma once
#include <QByteArray>
#include <queue>
#include <QMutex>
#include <QString>
#include "QT_STK.h"
#include "PacketDefine.h"

extern std::queue<TF_MESSAGE> TFmessageQueue;
extern QMutex TFqueueMutex;
extern std::queue<EVENT> EventQueue;
extern QMutex EventQueueMutex;

// 包处理 - 抽象基类函数
class PacketHandler
{
public:
    virtual ~PacketHandler() = default;
    virtual void handlePacket(EVENT& event) = 0;
};

// TFMessage 需要压入TF队列，定时弹出
// TFMessage 包处理 - 继承PacketHandler
class TFMessagePacketHandler: public PacketHandler
{
public:
    // 重写虚函数
    void handlePacket(EVENT& event) override
    {
        qDebug() << "Do TF handlePacket_1";

        if(event.data.size() == sizeof(TF_MESSAGE))
        {
            // 这里第二个data()函数返回一个指向字节数组的指针
            TF_MESSAGE* received_msg = reinterpret_cast<TF_MESSAGE*>(event.data.data());
            //qDebug() << "Do TF HandlePacket_2";
            // 加锁保护队列
            QMutexLocker locker( & TFqueueMutex);
            TFmessageQueue.push( * received_msg);
            //qDebug() << "Message pushed to TFmessageQueue. Current size:" << TFmessageQueue.size();
        }
    }
};

// 三种需要压入事件队列的事件包，定时弹出
// MIB
class MIBPacketHandler: public PacketHandler
{
public:
    // 重写虚函数
    void handlePacket(EVENT& event) override
    {
        qDebug() << "Do MIB handlePacket";
        if(event.data.size() == sizeof(MIB))
        {
            QMutexLocker locker( & EventQueueMutex);
            EventQueue.push(event);
            qDebug() << "MIB pushed to EventQueue. Current size:" << EventQueue.size();
        }
    }
};

// RAREQ
class RAREQPacketHandler: public PacketHandler
{
public:
    // 重写虚函数
    void handlePacket(EVENT& event) override
    {
        qDebug() << "Do RAREQ handlePacket";
        if(event.data.size() == sizeof(RAREQ))
        {
            QMutexLocker locker( & EventQueueMutex);
            EventQueue.push(event);
            qDebug() << "RAREQ pushed to EventQueue. Current size:" << EventQueue.size();
        }
    }
};

// RARSP
class RARSPPacketHandler: public PacketHandler
{
public:
    // 重写虚函数
    void handlePacket(EVENT& event) override
    {
        qDebug() << "Do RARSP handlePacket";
        if(event.data.size() == sizeof(RARSP))
        {
            QMutexLocker locker( & EventQueueMutex);
            EventQueue.push(event);
            qDebug() << "RARSP pushed to EventQueue. Current size:" << EventQueue.size();
        }
    }
};

// StringData 打印输出
class StringMessageHandler: public PacketHandler
{
public:
    void handlePacket(EVENT& event) override
    {
        qDebug() << "Do StringMessage handlePacket";
        QString StringData = QString::fromUtf8(event.data);
        // 这里有什么办法能够调用QT_STK下的msgLog？
        // 希望输出 StringData
    }
};


// 针对TF包与事件包分流器 - 主要功能：两大类包 压入对应数据队列
// 这里每个return语句被执行，函数会立即退出，不会执行后续的case，无需break
class PacketHandlerFactory
{
public:
    // 静态成员函数 - 无需创建实例即可调用
    static PacketHandler* createHandler(u8 messageType)
    {
        switch(messageType)
        {
            case TYPE_TF:
                return new TFMessagePacketHandler();
            case TYPE_MIB:
                return new MIBPacketHandler();
            case TYPE_RAREQ:
                return new RAREQPacketHandler();
            case TYPE_RARSP:
                return new RARSPPacketHandler();
            case TYPE_StringMsg_MATCH:
                return new StringMessageHandler();
            default:
                return nullptr;
        }
    }
};



#endif // PACKETHANDLERCLASS_H
