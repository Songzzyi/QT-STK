#ifndef QT_STK_H
#define QT_STK_H
#pragma once

#include <QWidget>
#include <QDateTime>  // 用于获取当前时间
#include <QByteArray>
#include <QString>
#include <QApplication>

// 下面三个涉及网络模块---需要添加：QT += network
#include <QUdpSocket>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QThread>  // 包含 QThread 头文件
#include <QMessageBox>
#include <queue>
#include <QTimer>   // 定时器
#include <QUuid>    // 产生 事件UUID

#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QApplication>
#include <QFont> // 用于设置字体

#include "ui_QT_STK.h"
#include "QSTKEarth.h"
#include "stk.h"
#include "PacketHandlerClass.h"

namespace Ui
{
    class QT_STK;
}

// 自定义代理以实现居中显示
class CenterAlignDelegate : public QStyledItemDelegate
{
public:
    // 使用基类的构造函数
    using QStyledItemDelegate::QStyledItemDelegate;

    // 重写 initStyleOption 函数，以实现居中对齐
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
    {
        // 调用基类的 initStyleOption 函数
        QStyledItemDelegate::initStyleOption(option, index);
        // 设置文本居中显示
        option->displayAlignment = Qt::AlignCenter;
    }
};

class QT_STK : public QWidget
{
    Q_OBJECT

public:
    explicit QT_STK(QWidget* parent = 0);   // explicit是什么关键字?
    ~QT_STK();
    static u8 previousTime[13];
    void msgLog(const QString& msg);
//    // 单例模式实现类实例的传递
//    static QT_STK& getInstance();

private slots:
    void on_pb_init_clicked();
    void on_pb_del_clicked();
    void on_pb_addSat_clicked();
    void on_pb_addFac_clicked();
    void on_pb_forward_clicked();
    void on_pb_backward_clicked();
    void on_pb_pause_clicked();
    void on_pb_rewind_clicked();
    void on_pb_slow_clicked();
    void on_pb_fast_clicked();
    void on_pb_connect_clicked();
    void on_pb_disconnect_clicked();
    void on_pb_udpsend_clicked();
    void on_cb_autoScroll_stateChanged(int arg1);
    void on_cb_TFShowTime_currentIndexChanged(const QString& test);
    void on_cb_cmd_currentTextChanged(const QString& text);
    void onSocketError(QAbstractSocket::SocketError socketError);
    void on_cb_EventShowTime_currentTextChanged(const QString& text);
    void on_Animation_duration_textChanged(const QString& arg1);
    void on_pb_clear_log_clicked();

private:
    Ui::QT_STK* ui;
    QThread* udpListenerThread;
    QUdpSocket* u_udp;
    QUdpSocket* udpListener;
    bool autoScrollEnabled; // 标志位：判断日志输出自动滚动
    QTimer* TFShowTimer;  // TF资源池显示定时器
    QTimer* EVENTShowTimer;  // EVENT事件包弹出定时器
    int animationDuration;  // 动作持续时间（单位：ms）
    QString currentMIBEventId;
    //QString currentRAREQEventId;
    //QString currentRARSPEventId;
    void udpDataSend();
    void resizeEvent(QResizeEvent* event) override; // 重写resizeEvent函数来动态调整tableWidget列宽
    void readPendingDatagrams();
    void setupUdpListener();
    void processTFQueue();
    void processEventQueue();
    void MIBHandler(EVENT& event);
    void RAREQHandler(EVENT& event);
    void RARSPHandler(EVENT& event);
    u32 TIME_to_EVENTtime(u8* previousTimePtr, const u8 currentTime[13]);
    void twUEsStateChange(QString UEname, QString state);
    void twUEsStateOffline_All();
    void adjustEventTimer();

signals:
    void messageReceived(const QString& msg);   // signals关键字 -> 用于声明信号，不需要在类中实现
};

#endif // QT_STK_H
