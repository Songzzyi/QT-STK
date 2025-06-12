#ifndef QSTKEARTH_H
#define QSTKEARTH_H
#pragma once

#include <ActiveQt/QAxWidget>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QWidget>
#include <QMessageBox>
#include <chrono>   // 获取高精度时间作为种子
#include <random>   // 提供随机数引擎
#include<vector>
#include <QFileDialog>
#include <QVBoxLayout>
#include <cassert>    //assert()
#include <QDebug>   // 控制台打印函数
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()

#include "stk.h"
#include "TimeManager.h"


class QSTKEarth : public QWidget {
    Q_OBJECT
public:
    // 获取QSTKEarth单例实例，使用双重检测锁机制确保线程安全，确保返回的是唯一实例
    static QSTKEarth& getInstance()
    {
        if (instance == NULL) // 第一次检测
        {
            QMutexLocker locker(&mutex); // 加互斥锁 --- 确保多线程下只有一个线程进入临界区
            if (NULL == instance) // 第二次检测
                instance = new QSTKEarth;
        }
        return *instance;
    }

private:
    TimeManager timeManager;    // 实例化时间类
    static QMutex mutex; // 实例互斥锁
    static QAtomicPointer<QSTKEarth> instance;
    QSTKEarth(const QSTKEarth&); // 禁止拷贝构造函数
    QSTKEarth(QWidget* parent = 0); // 禁止默认构造函数
    IAgStkObjectRootPtr m_pRoot; // STK根对象智能指针
    IAgSTKXApplicationPtr m_app; // STK应用程序智能指针
    // 记录已经生成的地面设施（Facilities）的位置（经纬度）--->避免重复位置
    std::vector<std::pair<double,double>> existingFacilities;
    std::mt19937 rng;
    // 全局时间变量
    QString ScenarioStartTime;
    QString ScenarioStopTime;
    // 标志位 - 判断是否是第一次初始化场景
    bool notFirstInit;

signals:

public slots:
    // test-zoomIn/Out
    void ZoomIn(); // 缩放功能 - 未完成

public:
    bool enableControl; // 控制STK功能的使能标志
    bool isPlayForward;  // 判断点击继续后是 前向(true)/后向(false) 播放
    static int SateIndex;  // 全局卫星数
    static int UEIndex;    // 全局用户数
    static int SenIndex;   // 全局传感器数
    ~QSTKEarth();
    // 控制STK场景的各种方法
    void PauseSTK(); // 暂停STK动画
    void StartSTK(); // 开始STK动画
    void FasterSTK(); // 加速STK动画
    void SlowerSTK(); // 减速STK动画
    void ResetSTK(); // 重置STK动画
    void NewScenario(); // 创建新场景
    // void LoadScenario(); // 加载场景
    void UnloadStkScence(); // 卸载场景
    void AddRandomSatellite();    // 随机添加卫星
    void AddSatelliteWithSensor();    // 随机添加带有传感器的卫星
    void AddRamdonFacility(); // 随机添加地面设施
    void PlayForward(); // 前向播放
    void PlayBackward();    // 后向播放
    void Pause();   // 暂停
    void Rewind();  // 重置场景
    void SenserioOnOff(const QString& satelliteName, const QString& sensorName, bool onOff);
    void SatelliteUEConnect(const QString& satelliteName, const QString& facilityName, bool onOff);
    void AddSpecificSatellite();    // 添加特定轨道参数与传感器的卫星
    void AddFacility(double lat, double lon); // 根据经纬度增加地面站
    void CreatSpecificSenserio();

};

#endif // QSTKEARTH_H
